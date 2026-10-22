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


def capture(path):
    """grab the app's window and crop the decoration away"""
    subprocess.run(["wmctrl", "-a", "PDA // Simulator"], capture_output=True)
    time.sleep(0.6)
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
    """one click, in the app's own coordinates (the window is at 560,300)"""
    from Xlib import display, X
    from Xlib.ext import xtest
    subprocess.run(["wmctrl", "-a", "PDA // Simulator"], capture_output=True)
    time.sleep(0.4)
    d = display.Display()
    sx, sy = 560 + x, 300 + y
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


def main():
    os.makedirs(OUT, exist_ok=True)
    subprocess.run(["pkill", "-x", "pda"], capture_output=True)
    time.sleep(0.5)

    # the map - with the traders on, so the picture shows the merchants
    def traders_on():
        click(163 + 148 + 6, 45)      # the Traders button of the second row

    run({"PDA_START_TAB": "0", "PDA_GPS_LAT": "32.6478", "PDA_GPS_LON": "-16.9193"},
        "area-map.png", before=traders_on)

    # the other tab
    run({"PDA_START_TAB": "6", "PDA_OTHER_PAGE": "1", "PDA_OTHER_SUB": "0"},
        "other-connections.png")
    run({"PDA_START_TAB": "6", "PDA_OTHER_PAGE": "1", "PDA_OTHER_SUB": "2"},
        "other-fips.png")
    run({"PDA_START_TAB": "6", "PDA_OTHER_PAGE": "2", "PDA_OTHER_SUB": "0"},
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

    run({"PDA_START_TAB": "1", "PDA_RELAY_PORT": str(port), "PDA_NAME": "Satscraft",
         "PDA_TEST_SEND": "on my way, ten minutes"},
        "messages.png", before=talk)
    peer.close()
    relay.terminate()
    print("done")


if __name__ == "__main__":
    main()
