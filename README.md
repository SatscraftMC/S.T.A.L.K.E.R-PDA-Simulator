# S.T.A.L.K.E.R PDA Simulator

**v0.0.5-pre-alpha**

A handheld communicator in the spirit of the STALKER PDA: an offline map, a
group chat that works over the local mesh, the FIPS mesh and the internet,
contacts and a Lightning wallet.

The UI is **LVGL v9** (C) in an 800x480 window. This repository is the
**desktop simulator** of a device meant to run on a Raspberry Pi 3B with a 5"
800x480 HDMI screen and physical buttons - which is why nothing here depends
on a desktop environment.

| | |
|---|---|
| ![the area map](docs/screenshots/area-map.png) | ![the room](docs/screenshots/messages.png) |
| **The map** - real Esri satellite imagery of Madeira, 27 named places, 16 zoom steps from the whole archipelago down to about a metre per pixel. Drag to pan, wheel or buttons to zoom. | **The room** - one chat, every carrier. What you type goes out on the local mesh, the FIPS mesh and the internet at once; everything that comes in lands in the same log, tagged with the carrier it arrived on. |
| ![connections](docs/screenshots/other-connections.png) | ![the user page](docs/screenshots/other-user.png) |
| **Connections** - what is up and what each system is set to. | **User** - the name other devices see, the room, and what the message log shows. |

## Features

- **Offline map** with real Esri satellite imagery of Madeira, zoom levels
  **64 - 10923** (from "whole archipelago" down to ~1 m per pixel).
- **27 named places and areas** (Funchal, Curral das Freiras, Pico Ruivo,
  Paúl da Serra, Porto Santo, Desertas, ...) drawn on the map, revealed in
  tiers as you zoom in.
- **Pack-based map system**: every region is a self-contained folder, so adding
  a new map tile needs no code change (see below).
- **One room, every carrier** (the Messages tab): what you type goes out on
  every medium that is up - the local mesh, the FIPS mesh and the internet -
  and everything that comes in shows up in the same log, tagged with the
  carrier it arrived on.
- **The Other tab** is the device's control panel: a column of sections
  (Settings, Connections, User, Extras), a column of sub-tabs, and the page
  below that. *Connections* says what is connected (green, grey while it
  tries, red when it is not there) and shows the settings of each system;
  *User* is where the name other devices see and the message preferences live.
- **A recenter button** on the map puts the view back on the blue dot.
- STALKER-style UI: parallelogram tab bar, layer toggle strip, battery gauge,
  live clock, gritty palette.
- The **blue dot is the device's GPS position** (gpsd's NMEA socket on the Pi;
  `PDA_GPS_LAT`/`PDA_GPS_LON` to force a position, `PDA_GPS_DEMO=1` for a slow
  drift without hardware).
- **Mouse wheel zooms towards the cursor** (the world point under the pointer
  stays under the pointer); the +/- buttons zoom around the middle of the view.

## The Messages tab - one room, three carriers

![the messages tab](docs/screenshots/messages.png)

Everything rides the same envelope on all three carriers:

```json
{"v":1,"t":"msg","id":"7f2a91","from":"Satscraft","room":"SECTOR",
 "text":"anyone got the drop coords?","ts":1730000000}
```

| carrier | what it is | how it is wired |
|---|---|---|
| **BIT** | the local mesh - no server, no account, whoever is in earshot is in the room | UDP broadcast on port 2425 (the port the bitchat mesh uses), on one channel (`#mesh` by default, changeable). A real bitchat node can be wired in through a bridge: `PDA_BITCHAT_SOCK=/tmp/pda-bitchat.sock` |
| **FIPS** | the FIPS mesh (Free Internetworking Peering System) | the daemon's **native datagram API**: `listen`/`connect` over the api socket, one datagram per message, no framing |
| **NET** | the internet | a relay (`tools/relay.py`), which can itself bridge to an ntfy topic |

Try it:

```bash
# a relay, then two clients
python3 tools/relay.py --port 7788
PDA_RELAY_PORT=7788 ./build/pda          # the PDA
python3 - <<'EOF'
import socket, json, time
s = socket.create_connection(("127.0.0.1", 7788))
s.sendall((json.dumps({"v":1,"t":"msg","id":"x1","from":"ROOK",
  "room":"SECTOR","text":"anyone got the drop coords",
  "ts":int(time.time())})+"\n").encode())
time.sleep(1)
EOF
```

The mesh carrier needs no setup at all: start two PDAs on the same network
and they hear each other.

## The Other tab - settings, connections, user

![the other tab](docs/screenshots/other-connections.png)

A column of sections on the left (Settings, Connections, User, Extras), a
strip of sub-tabs across the top of the pane, the page below that. The
sub-tabs belong to the section: *Settings* has General / Advanced / Language /
Themes, *Connections* has Info / Bitchat / F.I.P.S / WiFi / GPS / System.

**Connections → Info** is the one that matters: it says whether each system
is up - **green** when it is, **grey** while it is trying and **red** when it
is not there. The other *Connections* pages show the settings of that system
(the mesh channel, port and bridge, the fips sockets and the FSP port, the
WiFi interface and its address, the live GPS data, the version and where the
settings file lives).

**User** holds the personalisation: the name other devices see on your
messages and the room you talk in (both editable, both remembered), and the
switches for what the message log shows. The carrier badges in the top right
of the Messages tab use the same colours: green, grey, red.

The settings are remembered in `~/.config/pda/prefs.conf` (override with
`PDA_PREFS`), so they survive a restart.

## Environment

| variable | what it does |
|---|---|
| `PDA_NAME` | the name other devices see (default `STALKER`) |
| `PDA_ROOM` | the room name (default `SECTOR`) |
| `PDA_PREFS` | where the settings file lives |
| `PDA_RELAY_HOST` / `PDA_RELAY_PORT` | where the NET relay is (default `127.0.0.1:7788`) |
| `PDA_BITCHAT_SOCK` / `PDA_BITCHAT_HOST` / `PDA_BITCHAT_PORT` | a bitchat bridge to plug into the local mesh |
| `PDA_BITCHAT_PORT` | the mesh port (default 2425) |
| `PDA_MESH_CHANNEL` | the channel the local mesh talks on (default `mesh`) |
| `PDA_FIPS_SOCK` / `PDA_FIPS_CTL` | the fips daemon's api and control sockets |
| `PDA_FIPS_PORT` | the FSP port to talk on (default 4600) |
| `PDA_START_TAB` | start on a given tab (1 = Messages, 6 = Other) - a test hook |
| `PDA_OTHER_PAGE` / `PDA_OTHER_SUB` | open the Other tab on a given page - a test hook |
| `PDA_TEST_SEND` | say this on startup - a test hook |

### GPS on the Pi

The app talks to **gpsd**'s NMEA socket (`127.0.0.1:2947`) and nothing else:

```bash
sudo apt install gpsd
# USB module:
sudo gpsd -F /dev/ttyUSB0
# UART module (BN-880 on the Pi's 40-pin header): enable the serial port in
# raspi-config (Interface Options -> Serial Port), then:
sudo gpsd -F /dev/serial0
gpspipe -w          # should print $GNRMC lines
```

The first fix needs a clear sky and can take a few minutes.

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
| `tools/chat_test.py` | end to end test of the Messages tab: the relay, the local mesh and the FIPS native API |
| `tools/ui_test.py` | bug hunt for the UI: every tab and every page, the name and the room being saved, a broken settings file, a name that would break the wire format, the mesh channel, the recenter button, and driving the app hard |
| `tools/relay.py` | the reference relay for the NET carrier (can bridge to an ntfy topic) |
| `tools/bitchat_bridge.py` | bridge between the local mesh carrier and a real bitchat node |
| `tools/fips_mock.py` | a mock of the fips daemon's native datagram API, for testing the FIPS carrier |
| `tools/make_screenshots.py` | take the pictures in `docs/screenshots/` |
| `tools/gen_fonts.sh` | regenerate the bitmap fonts (Share Tech Mono, VT323, DSEG7) |
| `tools/fetch_deps.sh` | fetch LVGL + reassemble the split map levels |

## Status

Phase 3 (the map screen) is functional, the Messages tab is a real group
chat over three carriers, and the Other tab is the settings/connections/user
panel. Still to come: wallet/QR payments, camera, radio, real navigation
stack, input abstraction (sim -> GPIO/evdev), and the Pi port (DRM/KMS +
systemd boot-to-app). The *Settings* and *Extras* pages of the Other tab are
placeholders.

## License

Do whatever you want with it.
