# S.T.A.L.K.E.R PDA Simulator

**v0.3.6-alpha**

A handheld communicator in the spirit of the STALKER PDA: an offline map, a group chat that works over the local mesh, the FIPS mesh and the internet, contacts and a Lightning wallet.

The UI is **LVGL v9** (C) in an 800x480 window. This repository is the **desktop simulator** of a device meant to run on a Raspberry Pi 3B with a 5" 800x480 HDMI screen and physical buttons.

## Screenshots

| Map | Messages |
|-----|----------|
| ![map](docs/screenshots/area-map.png) | ![messages](docs/screenshots/messages.png) |

| Connections | Settings | User |
|-------------|----------|------|
| ![connections](docs/screenshots/other-connections.png) | ![settings](docs/screenshots/other-settings.png) | ![user](docs/screenshots/other-user.png) |

## Features

- **Offline map** with real Esri satellite imagery of Madeira, zoom 64–10923 (whole archipelago → ~1 m/px)
- **27 named places** (Funchal, Curral das Freiras, Pico Ruivo, Paúl da Serra, Porto Santo, Desertas…) drawn in tiers as you zoom
- **Four map layers** (Areas, Traders, Stashes, Waypoints) toggled by the strip under the tab bar; blue dot = GPS position
- **Waypoints & route**: right-click to place/remove; line drawn from position → waypoint → next; auto-removes at 5 m; remembered across restarts; Waypoints layer toggles it; CLEAR button in zoom column
- **Mouse & player coordinates** in top-right corner
- **Traders layer = real bitcoin merchants** from [BTC Map](https://btcmap.org) (OSM `currency:XBT=yes`) — 166 places on Madeira; bag-of-coins icon; tapping opens card (name, address, lightning/contactless/on-chain/ATM, last checked year)
- **Pack-based map system** — regions are self-contained folders, no code change to add new areas
- **Messages tab: one room, three carriers** (BIT local mesh, FIPS mesh, NET internet); messages go out on all active carriers; incoming tagged by carrier
- **Carrier picker** in Messages strip — per-carrier send toggle (off still receives); remembered in settings
- **Newest message at top** of log; scroll back for older lines
- **Mesh map** on Other → Connections → Bitchat — this device in center, peers around with names & lines; 5-min peer timeout
- **Local mesh uses broadcast + direct peer unicast** — subnet sweep finds peers, messages sent directly to known peers (bypasses firewall/AP broadcast drops); discovery row shows subnet
- **Firewall**: needs UDP 2425 (`sudo ufw allow 2425/udp`); Bitchat page warns when no peers answer
- **Other tab = control panel** — left column: Settings / Connections / User / Extras; sub-tabs across top; page below
- **Connections → Info** shows carrier status (green/grey/red); other pages show each system's settings
- **Settings → User** holds device name, room, message log switches (all editable, remembered)
- **Settings pages all work**: General, Advanced, Language (EN/PL), Themes (3 schemes × 3 accents), System
- **Recenter button** on map (zoom column) — jumps view to blue dot
- **STALKER guide aesthetic**: gritty surface, double-frame panels, crop-mark corners, hairline rules, coloured headings
- **STALKER-style UI**: parallelogram tab bar, layer toggle strip, battery gauge, live clock, gritty palette
- **GPS position** from gpsd NMEA socket (Pi); `PDA_GPS_LAT`/`PDA_GPS_LON` force position; `PDA_GPS_DEMO=1` for drift
- **Wheel zooms to cursor** (world point under pointer stays fixed); +/- buttons zoom around view center
- **Radio tab**: simple MP3 player styled like the STALKER music player -
  shuffle/loop toggles, track time, playlist and "Playing:" line in a central
  panel, parallelogram control buttons below (menu, shuffle, loop, vol-, vol+,
  prev, stop, play, next). Scans a directory for MP3 files and plays them via
  `gst-launch-1.0`. The directory is configurable (`PDA_RADIO_DIR`, saved in
  settings). Auto-advances to the next track when one finishes.

## Carriers

| Carrier | Transport | Requirements |
|---------|-----------|--------------|
| **BIT** | UDP broadcast port 2425 (bitchat channel `#mesh`) | LAN/WiFi; WiFi = radio. No Bluetooth in simulator; real bitchat node via `PDA_BITCHAT_SOCK` bridge |
| **NET** | TCP relay (`tools/relay.py`) | Relay process: `python3 tools/relay.py` on one machine; `PDA_RELAY_HOST=<ip>` on other |
| **FIPS** | `fips` daemon native API (`/run/fips/api.sock`) | Daemon running; without it badge = red `no daemon` |

When no carrier can send: `no carrier up - kept locally (BIT no link, FIPS no daemon, NET no relay)`.

## Build

```bash
sudo apt install cmake libsdl2-dev libsdl2-image-dev libpng-dev zlib1g-dev nodejs npm libglib2.0-dev
./tools/fetch_deps.sh     # fetches LVGL v9.6.0 + reassembles split map levels
./build.sh                # -> build/pda
./build/pda               # 800x480 SDL window
```

## Map Packs

```
assets/maps/<name>/
    pack.cfg        bbox + "level <z> <w> <h> <ox> <oy>" per level
    z<z>.rgb565     imagery, raw RGB565, row-major
```

App scans `assets/maps/*/pack.cfg` at startup. Override with `PDA_MAPS=/path/to/maps`.

Included packs:

| Pack | Levels | Coverage |
|------|--------|----------|
| `madeira` | z12, z13 | whole archipelago + Porto Santo + Desertas |
| `madeira-island` | z14 | island, ~9.5 m/px |
| `funchal` | z15, z16, z17 | Funchal area, down to ~1.2 m/px |

Large levels >100 MB stored as split parts (`z14.rgb565.part*`, `z17.rgb565.part*`); `tools/fetch_deps.sh` reassembles.

## Traders Pack (BTC Map)

```
assets/poi/<name>/
    pack.cfg        manifest: name, count, source, date, licence
    places.bin      152-byte fixed records
```

```bash
python3 tools/fetch_btc.py --name madeira --lat 32.6669 --lon -16.9247 --radius-km 30
# or: python3 tools/fetch_btc.py --name madeira --bbox 32.55,-17.45,33.25,-16.55
```

Madeira: 166 places, 25 KB. Whole planet: ~30k places, 4.5 MB.

## Tools

| Tool | Purpose |
|------|---------|
| `tools/fetch_map.py` | build map pack from Esri tiles |
| `tools/fetch_btc.py` | build traders pack from BTC Map |
| `tools/rgb565_pack.c` | fast RGB565 packer |
| `tools/analyze_map.py` | screenshot analysis: zoom steps, scale, geo-alignment, pan, artifacts |
| `tools/chat_test.py` | end-to-end test: relay, mesh, sweep, FIPS |
| `tools/mesh_netns_test.sh` | mesh over real link (veth + netns); needs root |
| `tools/ui_test.py` | UI bug hunt: every tab/page, settings, broken prefs, mesh channel, recenter, hard drive |
| `tools/relay.py` | NET carrier reference relay |
| `tools/bitchat_bridge.py` | bridge local mesh ↔ real bitchat node |
| `tools/fips_mock.py` | mock fips daemon for testing |
| `tools/make_screenshots.py` | regenerate docs/screenshots/ |
| `tools/gen_fonts.sh` | regenerate bitmap fonts (Share Tech Mono, VT323, DSEG7) |
| `tools/fetch_deps.sh` | fetch LVGL + reassemble split map levels |

## Status

Map screen, Messages tab (3 carriers), Other tab (all settings work) functional. TODO: wallet/QR, camera, radio, real nav stack, GPIO/evdev input, Pi port (DRM/KMS + systemd boot-to-app). Extras page empty; UI still English when language = Polski.

## License

Do whatever you want with it.