# S.T.A.L.K.E.R PDA Simulator

**v0.0.1-pre-alpha**

An open-source handheld communicator UI in the spirit of the STALKER PDA: a
gripped, button-driven device with an offline map, radio/messages, contacts and
a Lightning wallet.

This repository is the **desktop simulator** of that device. The UI is written
in **LVGL v9** (C) and runs in an 800x480 SDL2 window; the same UI code is
meant to be re-targeted at a Raspberry Pi 3B with a 5" 800x480 HDMI display
(no touch - physical buttons only), which is why nothing here depends on a
desktop environment.

```
+--------------------------------------------+
|  Area map | Messages | Contacts | Radio ... |   <- parallelogram tab bar
|  Task list | Stashes | Areas | ...        |   <- layer toggles (UI only for now)
+--------------------------------------------+
|                                            |
|              offline satellite map         |   <- drag to pan, +/- to zoom
|                                            |
+--------------------------------------------+
```

## Features so far


- **Offline map** with real Esri satellite imagery of Madeira, zoom levels
  **64 - 10923** (from "whole archipelago" down to ~1 m per pixel).
- **27 named places and areas** (Funchal, Curral das Freiras, Pico Ruivo,
  Paúl da Serra, Porto Santo, Desertas, ...) drawn on the map, revealed in
  tiers as you zoom in.
- **Pack-based map system**: every region is a self-contained folder, so adding
  a new map tile needs no code change (see below).
- STALKER-style UI: parallelogram tab bar, layer toggle strip, battery gauge,
  live clock, gritty palette.
- The **blue dot is the device's GPS position** (gpsd's NMEA socket on the Pi;
  `PDA_GPS_LAT`/`PDA_GPS_LON` to force a position, `PDA_GPS_DEMO=1` for a slow
  drift without hardware).
- **Mouse wheel zooms towards the cursor** (the world point under the pointer
  stays under the pointer); the +/- buttons zoom around the middle of the view.

## Build

Requirements (Debian/Ubuntu):

```bash
sudo apt install cmake libsdl2-dev libsdl2-image-dev libpng-dev zlib1g-dev nodejs npm
```

Then:

```bash
./tools/fetch_deps.sh     # fetches the vendored LVGL v9.6.0 (not in git)
./build.sh               # -> build/pda
./build/pda              # 800x480 SDL window, title "PDA // Simulator"
```

`tools/fetch_deps.sh` also **reassembles the map imagery** that is stored in
this repo as split parts (see *Map packs* below) - run it once after cloning.

## Map packs

A **pack** is one rectangular region of the world, stored as a folder:

```
assets/maps/<name>/
    pack.cfg        bbox + one "level <z> <w> <h> <ox> <oy>" line per level
    z<z>.rgb565     imagery, raw RGB565, row major
```

The app scans `assets/maps/*/pack.cfg` at startup, so **adding a region or a
deeper zoom level for an existing region is just dropping files in a folder** -
no code change. Point the app somewhere else with the `PDA_MAPS` environment
variable (e.g. `PDA_MAPS=/home/pi/pda-maps` on the Pi).

Levels are Esri World Imagery tiles, fetched and packed with:

```bash
python3 tools/fetch_map.py --name madeira --lon0 -17.35 --lon1 -16.15 \
                            --lat0 33.12 --lat1 32.30 --zooms 12,13
```

Already included:

| pack | levels | coverage |
|---|---|---|
| `madeira` | z12, z13 | whole archipelago + Porto Santo + Desertas |
| `madeira-island` | z14 | the island itself, ~9.5 m/px |
| `funchal` | z15, z16, z17 | Funchal area, down to ~1.2 m/px |

The two biggest levels exceed GitHub's 100 MB per-file limit, so they are
stored here as **split parts** (`z14.rgb565.part*`, `z17.rgb565.part*`).
`tools/fetch_deps.sh` puts them back together automatically.

## How the map works

- All positions (place names, markers, the view centre) are **geographic**,
  so they stay correct across packs and zoom levels.
- The world is mapped onto a pixel grid at a reference level; one grid unit
  equals `zoom/256` screen pixels, which makes the zoom scale identical for
  every pack.
- Rendering is a hand-written blit into an RGB565 canvas - no `lv_image`
  scaling, no LVGL scrolling - so zooming is deterministic and always anchored
  on the screen centre, and dragging tracks the finger 1:1 at every zoom.
- Imagery is **mmap'ed**, so the kernel pages it out instead of it sitting in
  RAM - important for the 1 GB Pi.

## Tools

| tool | what it does |
|---|---|
| `tools/fetch_map.py` | build a map pack from Esri World Imagery tiles |
| `tools/rgb565_pack.c` | fast RGB565 packer (the naive Python one takes ~1 h per level) |
| `tools/analyze_map.py` | screenshot analysis harness: zoom steps, scale progression, geo-alignment against the raw masters, pan tracking, artifact scan |
| `tools/gen_fonts.sh` | regenerate the bitmap fonts (Share Tech Mono, VT323, DSEG7) |
| `tools/fetch_deps.sh` | fetch LVGL + reassemble the split map levels |

## Status

Phase 3 (the map screen) is functional. Still to come: wallet/QR payments,
camera, radio, real navigation stack, input abstraction (sim -> GPIO/evdev),
and the Pi port (DRM/KMS + systemd boot-to-app).

## License

Do whatever you want with it.
