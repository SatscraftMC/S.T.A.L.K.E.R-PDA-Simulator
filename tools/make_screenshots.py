#!/usr/bin/env python3
"""Take the screenshots the README uses.

Every picture is the app's own window, cropped to the 800x480 client area
(the window decoration is not part of the device).

    python3 tools/make_screenshots.py
"""
import json
import os
import socket
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
APP = os.environ.get("PDA_TEST_BUILD", os.path.join(ROOT, "build", "pda"))
OUT = os.path.join(ROOT, "docs", "screenshots")


def free_port():
    s = socket.socket()
    s.bind(("127.0.0.1", 0))
    p = s.getsockname()[1]
    s.close()
    return p


# where the app's client area is in root coordinates. The window manager
# puts the window wherever there is a gap, so it is read out of the X tree
# every time instead of assumed - a click aimed at 560,300 on a desktop
# that did not put it there lands on the wallpaper, while the screenshot
# still comes out perfect (tools/ui_test.py reads the same thing).
OX, OY = 560, 300


def window_origin():
    global OX, OY
    from Xlib import display, X
    d = display.Display()
    try:
        root = d.screen().root
        utf8 = d.intern_atom("UTF8_STRING")
        wm_name = d.intern_atom("_NET_WM_NAME")
        clients = root.get_full_property(d.intern_atom("_NET_CLIENT_LIST"),
                                         X.AnyPropertyType)
        if not clients:
            return
        for wid in clients.value:
            w = d.create_resource_object("window", wid)
            try:
                prop = w.get_property(wm_name, utf8, 0, 1024)
            except Exception:
                continue
            if not prop or not prop.value:
                continue
            title = prop.value if isinstance(prop.value, bytes) \
                else str(prop.value).encode()
            if b"PDA // Simulator" not in title:
                continue
            x = y = 0
            cur = w
            while cur.id != root.id:      # the client is in a frame
                g = cur.get_geometry()
                x += g.x
                y += g.y
                cur = cur.query_tree().parent
            OX, OY = x, y
            return
    finally:
        d.close()


def focus_window():
    """raise the app's window, then read where it landed"""
    subprocess.run(["wmctrl", "-a", "PDA // Simulator"], capture_output=True)
    time.sleep(0.5)
    window_origin()


def capture(path):
    """grab the app's window and crop the decoration away"""
    focus_window()
    tmp = "/tmp/opencode/shot_tmp.png"
    subprocess.run(["gnome-screenshot", "-w", "-f", tmp], capture_output=True)
    try:
        from PIL import Image
    except ImportError:
        import shutil
        shutil.copy(tmp, path)
        return path
    im = Image.open(tmp).convert("RGB")
    w, h = im.size
    if h > 480:                       # the title bar on top
        im = im.crop((0, h - 480, w, h))
    im.save(path)
    print(f"  {os.path.relpath(path, ROOT)}  {im.size[0]}x{im.size[1]}")
    return path


def click(x, y):
    """one click, in the app's own coordinates"""
    from Xlib import display, X
    from Xlib.ext import xtest
    focus_window()
    d = display.Display()
    sx, sy = OX + x, OY + y
    xtest.fake_input(d, X.MotionNotify, x=sx, y=sy); d.sync(); time.sleep(0.08)
    xtest.fake_input(d, X.ButtonPress, 1); d.sync(); time.sleep(0.08)
    xtest.fake_input(d, X.ButtonRelease, 1); d.sync(); time.sleep(0.5)


def run(env_extra, shot_name, before=None, after=None):
    env = dict(os.environ)
    env.setdefault("DISPLAY", ":0")
    env.update(env_extra)
    p = subprocess.Popen([APP], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                         env=env)
    time.sleep(3.2)
    if before:
        before()
    capture(os.path.join(OUT, shot_name))
    if after:
        after()
    p.terminate()
    try:
        p.wait(timeout=5)
    except subprocess.TimeoutExpired:
        p.kill()
    time.sleep(0.6)


def first_place():
    """the first merchant of the first traders pack, as (lat, lon).

    The card picture wants the view to start on a real place, so one tap in
    the middle of the map is all it takes. The records are 152 bytes and
    the coordinates are the first 16 of them (src/poi.h). None when there
    is no pack: the picture is then left out rather than taken of nothing. """
    import glob
    import struct
    packs = sorted(glob.glob(os.path.join(ROOT, "assets", "poi", "*", "places.bin")))
    if not packs:
        return None
    with open(packs[0], "rb") as fh:
        rec = fh.read(152)
    if len(rec) != 152:
        return None
    lat, lon = struct.unpack("<dd", rec[:16])
    return (f"{lat}", f"{lon}")        # str() round-trips a double exactly


def main():
    os.makedirs(OUT, exist_ok=True)
    subprocess.run(["pkill", "-x", "pda"], capture_output=True)
    time.sleep(0.5)

    # the map - with the traders on, so the picture shows the merchants
    def traders_on():
        click(163 + 148 + 6, 45)      # the Traders button of the second row

    run({"PDA_START_TAB": "0", "PDA_GPS_LAT": "32.6478", "PDA_GPS_LON": "-16.9193"},
        "area-map.png", before=traders_on)

    # the merchant card: the first place of the traders pack, with the view
    # started on it, so one tap at the middle of the map is enough
    place = first_place()
    if place:
        lat, lon = place

        def card_on():
            click(163 + 148 + 6, 45)    # the Traders button of the second row
            click(400, 69 + 205)        # the merchant the view starts on

        run({"PDA_START_TAB": "0", "PDA_GPS_LAT": lat, "PDA_GPS_LON": lon},
            "merchant-card.png", before=card_on)

    # the Wallet: an invoice as a QR code, which is what the merchant scans
    def open_request():
        click(142, 390)                 # the REQUEST button of the balance screen

    run({"PDA_START_TAB": "5", "PDA_WALLET_BACKEND": "mock",
         "PDA_WALLET_MOCK": "/tmp/opencode/make_shots.mock"},
        "wallet-request.png", before=open_request)

    # the other tab
    run({"PDA_START_TAB": "6", "PDA_OTHER_PAGE": "1", "PDA_OTHER_SUB": "0"},
        "other-connections.png")
    run({"PDA_START_TAB": "6", "PDA_OTHER_PAGE": "1", "PDA_OTHER_SUB": "2"},
        "other-fips.png")
    run({"PDA_START_TAB": "6", "PDA_OTHER_PAGE": "0", "PDA_OTHER_SUB": "5"},
        "other-user.png")
    run({"PDA_START_TAB": "6", "PDA_OTHER_PAGE": "0", "PDA_OTHER_SUB": "0"},
        "other-settings.png")

    # the messages tab, with a conversation on it
    port = free_port()
    relay = subprocess.Popen([sys.executable, os.path.join(ROOT, "tools", "relay.py"),
                               "--host", "127.0.0.1", "--port", str(port)],
                              stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    time.sleep(1.0)
    peer = socket.create_connection(("127.0.0.1", port), timeout=5)

    def talk():
        msgs = [("ROOK", "anyone got the drop coords?"),
                ("WUMGUB", "not yet, rain again"),
                ("ROOK", "stash is under the water tower, north of the road, "
                         "past the broken fence"),
                ("WUMGUB", "copy that, moving out in ten")]
        for i, (who, what) in enumerate(msgs):
            peer.sendall((json.dumps({"v": 1, "t": "msg", "id": f"shot{i}",
                                       "from": who, "room": "SECTOR",
                                       "text": what, "ts": int(time.time())})
                          + "\n").encode())
            time.sleep(0.25)
        time.sleep(1.0)

    # PDA_RELAY_HOST is what asks for the uplink at all - with only the
    # port set the app stays on the local net and the picture comes out
    # with nobody in the room (net.c, net_init)
    run({"PDA_START_TAB": "1", "PDA_RELAY_HOST": "127.0.0.1",
         "PDA_RELAY_PORT": str(port), "PDA_NAME": "Satscraft",
         "PDA_TEST_SEND": "on my way, ten minutes"},
        "messages.png", before=talk)
    peer.close()
    relay.terminate()
    print("done")


if __name__ == "__main__":
    main()
