#!/usr/bin/env python3
"""Build a map pack: Esri World Imagery tiles -> RGB565 level files + pack.cfg.

A "pack" is one rectangular region of the world with one or more imagery levels
(tile zooms). The app auto-discovers every pack under assets/maps/*/pack.cfg, so
adding a new region or a deeper level for an existing region is just "drop files
in a folder" - no code change.

usage:
  fetch_map.py --name madeira --lon0 -17.35 --lon1 -16.15 --lat0 33.12 --lat1 32.30 \
               --zooms 12,13 [--out assets/maps]
  fetch_map.py --name madeira --lon0 ... --lat1 ... \
               --adopt assets/madeira_sat.rgb565:12 assets/madeira_sat_z13.rgb565:13
               (re-use already downloaded files instead of fetching)
"""
import argparse, io, math, os, shutil, subprocess, sys, time, urllib.request
from concurrent.futures import ThreadPoolExecutor
from PIL import Image

TILE = 256
BAND = 4          # tile rows kept in RAM at once (keeps memory tiny)
ESRI = ("https://server.arcgisonline.com/ArcGIS/rest/services/"
        "World_Imagery/MapServer/tile/{z}/{ty}/{tx}")


def gx(lon, z):
    """global tile-pixel X (level z) of a longitude"""
    return (lon + 180.0) / 360.0 * (2 ** z) * TILE


def gy(lat, z):
    """global tile-pixel Y (level z) of a latitude (web mercator)"""
    r = math.radians(lat)
    return (1 - math.log(math.tan(r) + 1 / math.cos(r)) / math.pi) / 2 * (2 ** z) * TILE


def level_geom(lon0, lon1, lat0, lat1, z):
    """crop rect of the bbox at level z -> (W, H, ox, oy); ox/oy = global px"""
    ox, oy = int(gx(lon0, z)), int(gy(lat0, z))
    return int(gx(lon1, z)) - ox, int(gy(lat1, z)) - oy, ox, oy


def fetch(tx, ty, z):
    url = ESRI.format(z=z, ty=ty, tx=tx)
    last = None
    for a in range(4):
        try:
            return urllib.request.urlopen(url, timeout=30).read()
        except Exception as e:
            last = e
            time.sleep(1 + a)
    raise RuntimeError(f"tile {tx},{ty} z{z}: {last}")


def fetch_level(lon0, lon1, lat0, lat1, z, out_path, tmp_raw):
    W, H, ox, oy = level_geom(lon0, lon1, lat0, lat1, z)
    tx0, tx1 = ox // TILE, (ox + W - 1) // TILE
    ty0, ty1 = oy // TILE, (oy + H - 1) // TILE
    total = (tx1 - tx0 + 1) * (ty1 - ty0 + 1)
    print(f"z{z}: {W}x{H} px, {tx1 - tx0 + 1}x{ty1 - ty0 + 1} = {total} tiles", flush=True)

    with open(tmp_raw, "wb") as raw:
        for band0 in range(ty0, ty1 + 1, BAND):
            band1 = min(band0 + BAND, ty1 + 1)
            rows = band1 - band0
            jobs = [(ty, tx) for ty in range(band0, band1) for tx in range(tx0, tx1 + 1)]
            with ThreadPoolExecutor(max_workers=4) as ex:
                tiles = list(ex.map(lambda j: fetch(j[1], j[0], z), jobs))
            mw = (tx1 - tx0 + 1) * TILE
            mosaic = Image.new("RGB", (mw, rows * TILE))
            for (ty, tx), data in zip(jobs, tiles):
                t = Image.open(io.BytesIO(data)).convert("RGB")
                mosaic.paste(t, ((tx - tx0) * TILE, (ty - band0) * TILE))
            # crop to the exact bbox rows of this band
            y0 = max(oy, band0 * TILE) - band0 * TILE
            y1 = min(oy + H, band1 * TILE) - band0 * TILE
            mosaic = mosaic.crop((ox - tx0 * TILE, y0, ox - tx0 * TILE + W, y1))
            raw.write(mosaic.tobytes())
            print(f"  band {band0 - ty0}/{ty1 - ty0 + 1} rows", flush=True)

    subprocess.run(["cc", "-O2", "-o", os.path.join(os.path.dirname(__file__), "rgb565_pack"),
                    os.path.join(os.path.dirname(__file__), "rgb565_pack.c")], check=True)
    subprocess.run([os.path.join(os.path.dirname(__file__), "rgb565_pack"), tmp_raw, out_path], check=True)
    os.remove(tmp_raw)
    want = W * H * 2
    got = os.path.getsize(out_path)
    if got != want:
        raise RuntimeError(f"packed size mismatch: {got} != {want}")
    return W, H, ox, oy


def write_cfg(path, name, lon0, lon1, lat0, lat1, levels):
    with open(path, "w") as f:
        f.write("# map pack - auto-discovered by the app (assets/maps/*/pack.cfg)\n")
        f.write(f"name = {name}\n")
        f.write("base_zoom = %d\n" % min(l[0] for l in levels))
        f.write(f"lon0 = {lon0}\nlon1 = {lon1}\nlat0 = {lat0}\nlat1 = {lat1}\n")
        for z, W, H, ox, oy in levels:
            f.write(f"level {z} {W} {H} {ox} {oy}\n")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--name", required=True)
    ap.add_argument("--lon0", type=float, required=True)
    ap.add_argument("--lon1", type=float, required=True)
    ap.add_argument("--lat0", type=float, required=True)
    ap.add_argument("--lat1", type=float, required=True)
    ap.add_argument("--zooms", default="")
    ap.add_argument("--adopt", nargs="*", default=[],
                    help="existing file:zoom pairs to re-use instead of downloading")
    ap.add_argument("--out", default="assets/maps")
    args = ap.parse_args()

    pdir = os.path.join(args.out, args.name)
    os.makedirs(pdir, exist_ok=True)
    levels = []
    tmp_raw = "/tmp/opencode/_pack_tmp.rgb"

    for spec in args.adopt:
        path, z = spec.rsplit(":", 1)
        z = int(z)
        W, H, ox, oy = level_geom(args.lon0, args.lon1, args.lat0, args.lat1, z)
        dst = os.path.join(pdir, f"z{z}.rgb565")
        if os.path.abspath(path) != os.path.abspath(dst):
            if os.path.exists(dst):
                os.remove(dst)
            shutil.move(path, dst)
        levels.append((z, W, H, ox, oy))
        print(f"adopted z{z} -> {dst} ({W}x{H})")

    if args.zooms:
        for z in [int(z) for z in args.zooms.split(",") if z]:
            dst = os.path.join(pdir, f"z{z}.rgb565")
            levels.append((z, *fetch_level(args.lon0, args.lon1, args.lat0, args.lat1, z, dst, tmp_raw)))

    levels.sort()
    write_cfg(os.path.join(pdir, "pack.cfg"), args.name, args.lon0, args.lon1,
              args.lat0, args.lat1, levels)
    print("wrote", os.path.join(pdir, "pack.cfg"))
    for z, W, H, ox, oy in levels:
        print(f"  level {z}: {W}x{H} ({(W * H * 2) / 1e6:.0f} MB) origin {ox},{oy}")


if __name__ == "__main__":
    sys.exit(main())
