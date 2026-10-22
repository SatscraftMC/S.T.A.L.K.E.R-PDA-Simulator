#!/usr/bin/env python3
"""Fetch the bitcoin merchants of a region and write a POI pack.

BTC Map (btcmap.org) is the set of places that accept bitcoin: everything
tagged in OpenStreetMap with ``currency:XBT=yes``. The data comes from the
Overpass instance BTC Map itself runs, with the same query they use.

    python3 tools/fetch_btc.py --name madeira --bbox 32.55,-17.45,33.25,-16.55
    python3 tools/fetch_btc.py --name madeira --lat 32.6669 --lon -16.9247 --radius-km 25

It writes a pack, the same shape as the map packs:

    assets/poi/<name>/pack.cfg     the manifest (name, count, source, date)
    assets/poi/<name>/places.bin   the places, fixed size records

The record layout must match ``poi_place_t`` in ``src/poi.h``:

    double lat, lon; char name[48]; char address[64]; char category[16];
    uint8_t flags; uint8_t pad; uint16_t year;   -> 152 bytes
"""
import argparse
import datetime
import json
import os
import struct
import sys
import urllib.parse
import urllib.request

# the record: 8 + 8 + 48 + 64 + 16 + 1 + 1 + 2 = 148, padded to 152
RECORD = struct.Struct("<dd48s64s16sBBH4x")

OVERPASS = "https://overpass.btcmap.org/api/interpreter"
# the same query BTC Map runs, over the region we want
QUERY = """
[out:json][timeout:180];
nwr["currency:XBT"="yes"]({bbox});
out meta geom;
"""

# the flags, and the tag they come from
FLAG_LIGHTNING = 0x01
FLAG_ONCHAIN = 0x02
FLAG_CONTACTLESS = 0x04
FLAG_ATM = 0x08
FLAG_VERIFIED = 0x10
FLAG_COMPANION = 0x20


def yes(value):
    """a yes/no tag, defensively.

    The data holds "yes", "Yes", "y", "no", "No", a date, and junk. Anything
    that is not a recognisable yes is not a yes - and it is not a "no" either.
    """
    if value is None:
        return False
    v = str(value).strip().lower()
    if v in ("yes", "y", "true", "1"):
        return True
    return False


def known(value):
    """is it a yes or a no at all?"""
    if value is None:
        return False
    v = str(value).strip().lower()
    return v in ("yes", "y", "true", "1", "no", "n", "false", "0")


def fetch(bbox, timeout=180):
    """the places of the region, as Overpass gives them"""
    data = urllib.parse.urlencode({"data": QUERY.format(bbox=bbox)}).encode()
    req = urllib.request.Request(OVERPASS, data=data,
                                 headers={"User-Agent": "pda-simulator/0.2 (btcmap)"})
    with urllib.request.urlopen(req, timeout=timeout) as r:
        body = r.read()
    return json.loads(body.decode("utf-8"))


def centre(element):
    """the point of an element.

    A node has its coordinates; a way or a relation is drawn at the centre of
    its bounding box - that is what BTC Map does too.
    """
    if "lat" in element and "lon" in element:
        return float(element["lat"]), float(element["lon"])
    centre_ = element.get("center")
    if centre_:
        return float(centre_["lat"]), float(centre_["lon"])
    lats = [float(p["lat"]) for p in element.get("geometry", []) if "lat" in p]
    lons = [float(p["lon"]) for p in element.get("geometry", []) if "lon" in p]
    if lats and lons:
        return (min(lats) + max(lats)) / 2.0, (min(lons) + max(lons)) / 2.0
    return None


def address_of(tags):
    """the street address, the way OSM has it"""
    street = tags.get("addr:street", "")
    number = tags.get("addr:housenumber", "")
    city = tags.get("addr:city", "")
    out = " ".join(x for x in (number, street) if x)
    if city and city.lower() not in out.lower():
        out = (out + " " + city).strip()
    return out.strip()


def clean(text, size):
    """a string that fits the field"""
    if not text:
        return ""
    text = " ".join(str(text).split())
    raw = text.encode("utf-8", "ignore")[:size - 1]
    return raw.decode("utf-8", "ignore")


def year_of(tags):
    """the year the place was last checked"""
    for key in ("survey:date", "check_date:currency:XBT", "check_date"):
        value = tags.get(key)
        if value and len(value) >= 4 and value[:4].isdigit():
            return int(value[:4])
    return 0


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--name", required=True, help="the name of the pack")
    ap.add_argument("--bbox", help="south,west,north,east")
    ap.add_argument("--lat", type=float, help="the centre of the region")
    ap.add_argument("--lon", type=float, help="the centre of the region")
    ap.add_argument("--radius-km", type=float, default=25.0)
    ap.add_argument("--out", default="assets/poi", help="where the packs go")
    args = ap.parse_args()

    if args.bbox:
        bbox = args.bbox
    elif args.lat is not None and args.lon is not None:
        # a box around the point, in degrees
        dlat = args.radius_km / 111.32
        dlon = args.radius_km / (111.32 * 0.8135)   # at Madeira's latitude
        bbox = "%.5f,%.5f,%.5f,%.5f" % (args.lat - dlat, args.lon - dlon,
                                        args.lat + dlat, args.lon + dlon)
    else:
        ap.error("give --bbox, or --lat and --lon")

    print("fetching %s ..." % bbox)
    doc = fetch(bbox)
    elements = doc.get("elements", [])
    print("  %d elements" % len(elements))

    places = []
    for e in elements:
        tags = e.get("tags")
        if not tags:
            continue
        if str(tags.get("currency:XBT", "")).strip().lower() == "no":
            continue
        point = centre(e)
        if not point:
            continue
        lat, lon = point
        if not (-90.0 <= lat <= 90.0 and -180.0 <= lon <= 180.0):
            continue

        flags = 0
        if yes(tags.get("payment:lightning")):
            flags |= FLAG_LIGHTNING
        if yes(tags.get("payment:onchain")):
            flags |= FLAG_ONCHAIN
        if yes(tags.get("payment:lightning_contactless")):
            flags |= FLAG_CONTACTLESS
        if tags.get("amenity") == "atm":
            flags |= FLAG_ATM
        if yes(tags.get("payment:lightning:requires_companion_app")):
            flags |= FLAG_COMPANION

        year = year_of(tags)
        today = datetime.date.today()
        if year and today.year - year <= 1:
            flags |= FLAG_VERIFIED

        category = tags.get("amenity") or tags.get("shop") or tags.get("tourism") or ""
        if not category and tags.get("cuisine"):
            category = "food"

        places.append((lat, lon,
                       clean(tags.get("name", ""), 48),
                       clean(address_of(tags), 64),
                       clean(category, 16),
                       flags, year))

    # the same place twice is one place
    seen = set()
    unique = []
    for p in places:
        key = (round(p[0], 6), round(p[1], 6), p[2])
        if key in seen:
            continue
        seen.add(key)
        unique.append(p)
    places = unique

    out_dir = os.path.join(args.out, args.name)
    os.makedirs(out_dir, exist_ok=True)

    bin_path = os.path.join(out_dir, "places.bin")
    with open(bin_path, "wb") as f:
        for p in places:
            f.write(RECORD.pack(p[0], p[1],
                                p[2].encode("utf-8"), p[3].encode("utf-8"),
                                p[4].encode("utf-8"), p[5], 0, p[6]))

    lats = [p[0] for p in places]
    lons = [p[1] for p in places]
    cfg_path = os.path.join(out_dir, "pack.cfg")
    with open(cfg_path, "w") as f:
        f.write("name %s\n" % args.name)
        f.write("count %d\n" % len(places))
        f.write("source overpass.btcmap.org\n")
        f.write("dataset BTC Map (btcmap.org) - OpenStreetMap contributors, ODbL\n")
        f.write("date %s\n" % datetime.date.today().isoformat())
        if places:
            f.write("lat0 %.6f\n" % min(lats))
            f.write("lat1 %.6f\n" % max(lats))
            f.write("lon0 %.6f\n" % min(lons))
            f.write("lon1 %.6f\n" % max(lons))

    print("  %d places -> %s" % (len(places), out_dir))
    print("  %d bytes" % os.path.getsize(bin_path))
    return 0


if __name__ == "__main__":
    sys.exit(main())
