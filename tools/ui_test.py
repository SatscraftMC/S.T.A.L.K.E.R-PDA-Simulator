#!/usr/bin/env python3
"""Bug hunt for the PDA simulator's UI.

Drives the real app and checks the things that are easy to break:

  1. every tab comes up and draws something
  2. every page of the Other tab comes up and draws something
  3. the name and the room can be typed and SAVED, and survive a restart
  4. the settings file survives a restart, and a broken one does not kill
     the app
  5. the app survives a name with quotes, unicode and a silly length
  6. the app survives being driven hard (every tab, every page, in a row)
  7. the app survives with no relay and no daemon

Run it with the ordinary build, or point it at an ASan build with
    PDA_TEST_BUILD=/tmp/build-asan/pda python3 tools/ui_test.py
"""
import json
import os
import re
import shutil
import socket
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
APP = os.environ.get("PDA_TEST_BUILD", os.path.join(ROOT, "build", "pda"))
LOGDIR = "/tmp/opencode/uitest"
SHOTS = "/tmp/opencode/uishot"
PREFS = os.path.expanduser("~/.config/pda/prefs.conf")
OX, OY = 560, 300           # the window's client origin on this desktop
DY = 45                     # the panel starts under the tab row and the gap
NAV_X = 40                  # the sections column
NAV_Y0 = 83                 # the first section
NAV_PITCH = 26
NAME_Y = 115                # the name field on the User page
NAME_X = 550                # the name field, in screen coordinates
SAVE_X = 725                # the SAVE button next to it
SUB_X = 244                 # the sub tab column
SUB_Y0 = 83                 # the first sub tab
SUB_PITCH = 24

FAIL = []


def note(ok, msg):
    print(("  PASS  " if ok else "  FAIL  ") + msg, flush=True)
    if not ok:
        FAIL.append(msg)
    return ok


def free_port():
    s = socket.socket()
    s.bind(("127.0.0.1", 0))
    p = s.getsockname()[1]
    s.close()
    return p


def start(env_extra=None, log="app"):
    env = dict(os.environ)
    env.setdefault("DISPLAY", ":0")
    env.update(env_extra or {})
    path = os.path.join(LOGDIR, f"{log}.log")
    f = open(path, "w")
    p = subprocess.Popen([APP], stdout=f, stderr=subprocess.STDOUT, env=env)
    return p, f, path


def stop(p, f):
    if p.poll() is None:
        p.terminate()
        try:
            p.wait(timeout=5)
        except subprocess.TimeoutExpired:
            p.kill()
    f.close()


def focus_window():
    subprocess.run(["wmctrl", "-a", "PDA // Simulator"], capture_output=True)
    time.sleep(0.4)


def shot(name):
    focus_window()
    subprocess.run(["gnome-screenshot", "-w", "-f", f"{SHOTS}_{name}.png"],
                   capture_output=True)
    return f"{SHOTS}_{name}.png"


def lit_pixels(path, y0=0, y1=480):
    """how many bright pixels the screen has (a blank screen has almost none)"""
    try:
        from PIL import Image
    except ImportError:
        return -1
    im = Image.open(path).convert("RGB")
    w, h = im.size
    px = im.load()
    n = 0
    for y in range(y0, min(y1, h)):
        for x in range(0, w, 2):
            r, g, b = px[x, y]
            if r + g + b > 300:
                n += 1
    return n


def click(x, y):
    focus_window()
    subprocess.run([sys.executable, "/tmp/opencode/pdaclick2.py",
                    str(OX + x), str(OY + y)], capture_output=True)
    time.sleep(0.5)


def click_once(x, y):
    """exactly one click: a button that toggles would be toggled back by
       the four clicks the ordinary click() makes"""
    from Xlib import display, X
    from Xlib.ext import xtest
    focus_window()
    d = display.Display()
    sx, sy = OX + x, OY + y
    xtest.fake_input(d, X.MotionNotify, x=sx, y=sy)
    d.sync()
    time.sleep(0.08)
    xtest.fake_input(d, X.ButtonPress, 1)
    d.sync()
    time.sleep(0.08)
    xtest.fake_input(d, X.ButtonRelease, 1)
    d.sync()
    time.sleep(0.4)


def type_text(s):
    """type a string with XTEST key events (shift for the capitals)"""
    from Xlib import display, X
    from Xlib.ext import xtest
    from Xlib import XK
    d = display.Display()
    shift = d.keysym_to_keycode(XK.string_to_keysym("Shift_L"))
    for ch in s:
        ks = XK.string_to_keysym(ch)
        if not ks:
            continue
        code = d.keysym_to_keycode(ks)
        if not code:
            continue
        upper = ch.isupper() or ch in '~!@#$%^&*()_+{}|:"<>?'
        if upper:
            xtest.fake_input(d, X.KeyPress, shift)
            d.sync()
        xtest.fake_input(d, X.KeyPress, code)
        d.sync()
        time.sleep(0.05)
        xtest.fake_input(d, X.KeyRelease, code)
        d.sync()
        if upper:
            xtest.fake_input(d, X.KeyRelease, shift)
            d.sync()
        time.sleep(0.08)


def press_enter():
    from Xlib import display, X
    from Xlib.ext import xtest
    d = display.Display()
    code = d.keysym_to_keycode(0xFF0D)
    xtest.fake_input(d, X.KeyPress, code)
    d.sync()
    time.sleep(0.08)
    xtest.fake_input(d, X.KeyRelease, code)
    d.sync()
    time.sleep(0.3)


def app_alive(p, logpath):
    """the app is alive and has not logged an error"""
    if p.poll() is not None:
        return False
    try:
        with open(logpath) as f:
            data = f.read()
    except OSError:
        return True
    bad = re.search(r"AddressSanitizer|SIGSEGV|Segmentation|assert", data)
    return bad is None


# --------------------------------------------------------------------------


def test_tabs():
    print("1. every tab comes up", flush=True)
    for tab in range(7):
        p, f, logpath = start({"PDA_START_TAB": str(tab)}, f"tab{tab}")
        time.sleep(2.6)
        ok = app_alive(p, logpath)
        path = shot(f"tab{tab}")
        n = lit_pixels(path)
        note(ok and n > 20, f"tab {tab} renders ({n} lit pixels)")
        stop(p, f)


def test_other_pages():
    print("2. every Other page comes up", flush=True)
    pages = [(0, s) for s in range(5)] + [(1, s) for s in range(5)] + \
            [(2, 0), (3, 0)]
    for page, sub in pages:
        p, f, logpath = start({"PDA_START_TAB": "6",
                                "PDA_OTHER_PAGE": str(page),
                                "PDA_OTHER_SUB": str(sub)}, f"page{page}_{sub}")
        time.sleep(2.4)
        ok = app_alive(p, logpath)
        path = shot(f"page{page}_{sub}")
        n = lit_pixels(path)
        note(ok and n > 20, f"Other page {page}.{sub} renders ({n} lit pixels)")
        stop(p, f)


def test_name_save():
    print("3. the name can be typed and saved", flush=True)
    if os.path.exists(PREFS):
        os.unlink(PREFS)

    p, f, logpath = start({"PDA_START_TAB": "6", "PDA_OTHER_PAGE": "2"}, "name")
    time.sleep(2.8)
    try:
        # the name field (the content starts at app x=306, the value column
        # 150 into it) and the SAVE button to its right
        click(NAME_X, NAME_Y)
        time.sleep(0.3)
        type_text("Satscraft")
        time.sleep(0.4)
        shot("name_typed")
        click(SAVE_X, NAME_Y)
        time.sleep(0.8)
        shot("name_saved")
        saved = ""
        if os.path.exists(PREFS):
            with open(PREFS) as f:
                saved = f.read()
        note(re.search(r"name\s*=\s*Satscraft\s*$", saved, re.M) is not None,
             f"the name was written to the settings file ({saved.strip()!r})")
    finally:
        stop(p, f)

    # and it survives a restart
    p, f, logpath = start({"PDA_START_TAB": "6", "PDA_OTHER_PAGE": "2"}, "name2")
    time.sleep(2.6)
    try:
        path = shot("name_restart")
        # the name is drawn in the field: look for the pixels of the text
        n = lit_pixels(path, DY + 63, DY + 83)
        note(n > 10, f"the name is there after a restart ({n} lit pixels in the field)")
    finally:
        stop(p, f)


def test_settings_file():
    print("4. the settings file", flush=True)
    # a broken file must not kill the app
    with open(PREFS, "w") as f:
        f.write("this is not a setting\n= = =\nname\n")
    p, f, logpath = start({"PDA_START_TAB": "6", "PDA_OTHER_PAGE": "2"}, "broken")
    time.sleep(2.6)
    note(app_alive(p, logpath), "a broken settings file does not kill the app")
    stop(p, f)

    # a name with the characters that break a JSON envelope
    os.makedirs(os.path.dirname(PREFS), exist_ok=True)
    with open(PREFS, "w") as f:
        f.write('name = Ro"ok\\Blue\nroom = Zone "A"\n')
    p, f, logpath = start({"PDA_START_TAB": "1"}, "quotes")
    time.sleep(2.6)
    ok = app_alive(p, logpath)
    stop(p, f)
    note(ok, "a name with quotes and backslashes does not kill the app")

    # a silly long name
    with open(PREFS, "w") as f:
        f.write("name = " + ("A" * 400) + "\n")
    p, f, logpath = start({"PDA_START_TAB": "6", "PDA_OTHER_PAGE": "2"}, "long")
    time.sleep(2.6)
    ok = app_alive(p, logpath)
    shot("longname")
    stop(p, f)
    note(ok, "a very long name does not kill the app")

    if os.path.exists(PREFS):
        os.unlink(PREFS)


def test_no_carriers():
    print("5. no relay, no daemon", flush=True)
    p, f, logpath = start({"PDA_START_TAB": "1",
                            "PDA_RELAY_HOST": "127.0.0.1",
                            "PDA_RELAY_PORT": str(free_port()),
                            "PDA_FIPS_SOCK": "/tmp/opencode/uitest/none.sock"},
                           "nocarriers")
    time.sleep(3.0)
    ok = app_alive(p, logpath)
    shot("nocarriers")
    stop(p, f)
    note(ok, "the app runs with no carrier at all")


def test_hard_use():
    print("6. driving it hard", flush=True)
    p, f, logpath = start({"PDA_START_TAB": "6", "PDA_OTHER_PAGE": "1"}, "hard")
    time.sleep(2.6)
    try:
        # every tab, in a row
        for tab in range(7):
            click(120 + tab * 80, 22)
        time.sleep(0.5)
        # every sub tab of the connections page
        click(120 + 6 * 80, 22)
        time.sleep(0.4)
        for sub in range(6):
            click(SUB_X, SUB_Y0 + SUB_PITCH * sub)
        time.sleep(0.5)
        # and the nav column
        for i in range(4):
            click(NAV_X, NAV_Y0 + NAV_PITCH * i)
        time.sleep(0.5)
        # save a name and walk away from the page while it flashes
        click(NAV_X, NAV_Y0 + NAV_PITCH * 2)     # the User page
        time.sleep(0.3)
        click(NAME_X, NAME_Y)       # the name field
        type_text("Satscraft")
        click(SAVE_X, NAME_Y)       # SAVE
        click(NAV_X, NAV_Y0 + NAV_PITCH * 1)     # and straight to another page
        time.sleep(1.8)             # long enough for the flash to fire
        click(NAV_X, NAV_Y0 + NAV_PITCH * 2)     # and back again
        time.sleep(0.4)
        ok = app_alive(p, logpath)
        shot("hard")
    finally:
        stop(p, f)
    note(ok, "clicking through everything does not crash the app")



def test_json_escaping():
    print("7. a name that would break the wire format", flush=True)
    port = free_port()
    relay = subprocess.Popen([sys.executable, os.path.join(ROOT, "tools", "relay.py"),
                              "--host", "127.0.0.1", "--port", str(port), "-v"],
                             stdout=open(os.path.join(LOGDIR, "relay.log"), "w"),
                             stderr=subprocess.STDOUT)
    time.sleep(1.0)
    peer = socket.create_connection(("127.0.0.1", port), timeout=5)
    peer.settimeout(5)
    name = 'Ro"ok\\Blue'
    p, f, logpath = start({"PDA_START_TAB": "1", "PDA_RELAY_PORT": str(port),
                            "PDA_NAME": name, "PDA_TEST_SEND": "hello there"},
                           "escape")
    try:
        time.sleep(3.0)
        got = None
        end = time.time() + 6
        while time.time() < end and not got:
            try:
                data = peer.recv(4096)
            except socket.timeout:
                continue
            for line in data.decode("utf-8", "replace").splitlines():
                if not line.strip():
                    continue
                try:
                    m = json.loads(line)
                except json.JSONDecodeError:
                    continue
                if m.get("t") == "msg":
                    got = m
        note(got is not None and got.get("from") == name,
             f"the relay parsed the message from {name!r} (got {got and got.get('from')!r})")
        with open(os.path.join(LOGDIR, "relay.log")) as f:
            rlog = f.read()
        note("bad line" not in rlog, "the relay did not see a broken line")
    finally:
        stop(p, f)
        peer.close()
        relay.terminate()
        try:
            relay.wait(timeout=5)
        except subprocess.TimeoutExpired:
            relay.kill()


def test_many_messages():
    print("8. a lot of messages", flush=True)
    port = free_port()
    relay = subprocess.Popen([sys.executable, os.path.join(ROOT, "tools", "relay.py"),
                              "--host", "127.0.0.1", "--port", str(port)],
                             stdout=open(os.path.join(LOGDIR, "relay2.log"), "w"),
                             stderr=subprocess.STDOUT)
    time.sleep(1.0)
    peer = socket.create_connection(("127.0.0.1", port), timeout=5)
    p, f, logpath = start({"PDA_START_TAB": "1", "PDA_RELAY_PORT": str(port)},
                          "many")
    try:
        time.sleep(2.5)
        long_msg = "a very long message " * 40
        for i in range(220):
            peer.sendall((json.dumps({"v": 1, "t": "msg", "id": f"m{i}",
                                       "from": "ROOK", "room": "SECTOR",
                                       "text": long_msg if i == 5 else f"message {i}",
                                       "ts": int(time.time())}) + "\n").encode())
        time.sleep(4.0)
        ok = app_alive(p, logpath)
        shot("many")
        note(ok, "220 messages (one of them very long) do not kill the app")
    finally:
        stop(p, f)
        peer.close()
        relay.terminate()
        try:
            relay.wait(timeout=5)
        except subprocess.TimeoutExpired:
            relay.kill()


def test_mesh_channel():
    print("9. the mesh channel", flush=True)
    peer = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    peer.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    peer.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    peer.bind(("127.0.0.2", 2425))
    p, f, logpath = start({"PDA_START_TAB": "1", "PDA_DEBUG": "1",
                            "PDA_MESH_CHANNEL": "mesh"}, "chan")
    try:
        time.sleep(2.6)
        peer.sendto((json.dumps({"v": 1, "t": "msg", "id": "c1", "from": "ROOK",
                                  "room": "SECTOR", "chan": "other",
                                  "text": "not for us",
                                  "ts": int(time.time())}) + "\n").encode(),
                    ("127.0.0.1", 2425))
        time.sleep(1.2)
        peer.sendto((json.dumps({"v": 1, "t": "msg", "id": "c2", "from": "ROOK",
                                  "room": "SECTOR", "chan": "mesh",
                                  "text": "for the mesh",
                                  "ts": int(time.time())}) + "\n").encode(),
                    ("127.0.0.1", 2425))
        time.sleep(1.5)
        data = ""
        try:
            with open(logpath) as fh:
                data = fh.read()
        except OSError:
            pass
        note("recv via BIT from ROOK: for the mesh" in data,
             "the message of our channel comes in")
        note("not for us" not in data,
             "the message of another channel is ignored")
    finally:
        stop(p, f)
        peer.close()


def test_recenter():
    print("10. the map recenter button", flush=True)
    p, f, logpath = start({"PDA_START_TAB": "0", "PDA_GPS_LAT": "32.6478",
                            "PDA_GPS_LON": "-16.9193"}, "recenter")
    try:
        time.sleep(3.0)

        def blue_dot(path):
            from PIL import Image
            im = Image.open(path).convert("RGB")
            w, h = im.size
            px = im.load()
            xs, ys = [], []
            for y in range(90, h - 60):        # the buttons live at the bottom
                for x in range(0, w - 120):   # ... and on the right
                    r, g, b = px[x, y]
                    if b > 225 and 35 < r < 95 and 145 < g < 195:
                        xs.append(x)
                        ys.append(y)
            if len(xs) < 20:
                return None
            return sum(xs) // len(xs), sum(ys) // len(ys)

        before = blue_dot(shot("rc_before"))
        # drag the map away
        drag_code = """
import time
from Xlib import display, X
from Xlib.ext import xtest
d = display.Display()
x0, y0 = %d, %d
xtest.fake_input(d, X.MotionNotify, x=x0, y=y0); d.sync(); time.sleep(0.1)
xtest.fake_input(d, X.ButtonPress, 1); d.sync(); time.sleep(0.1)
for i in range(1, 13):
    xtest.fake_input(d, X.MotionNotify, x=x0 + 8 * i, y=y0 + 5 * i)
    d.sync(); time.sleep(0.04)
xtest.fake_input(d, X.ButtonRelease, 1); d.sync(); time.sleep(0.4)
""" % (OX + 400, OY + 275)
        subprocess.run([sys.executable, "-c", drag_code], capture_output=True)
        time.sleep(0.8)
        dragged = blue_dot(shot("rc_dragged"))
        # the recenter button: the third one down on the right
        click(770, 69 + 298 + 14)
        time.sleep(1.0)
        after = blue_dot(shot("rc_after"))
        note(dragged is not None and before is not None and
             abs(dragged[0] - before[0]) > 30,
             f"the drag moved the dot ({before} -> {dragged})")
        note(after is not None and abs(after[0] - 400) < 25 and
             abs(after[1] - (275 + 32)) < 25,
             f"the recenter button put it back in the middle ({after})")
    finally:
        stop(p, f)


def test_caret():
    print("11. the text caret", flush=True)
    p, f, logpath = start({"PDA_START_TAB": "6", "PDA_OTHER_PAGE": "2"}, "caret")
    try:
        time.sleep(3.0)

        def count(path, x0, x1, y0, y1, accent):
            """the pixels of that colour in a box of the app
               (the screenshots have the window's title bar on top)"""
            from PIL import Image
            im = Image.open(path).convert("RGB")
            px = im.load()
            n = 0
            for y in range(y0 + 32, y1 + 32):
                for x in range(x0, x1):
                    r, g, b = px[x, y]
                    if accent:
                        if r > 150 and g > 110 and b < 80 and r >= g:
                            n += 1
                    else:
                        # the dim grey of the placeholder
                        if 60 < r < 160 and 60 < g < 160 and b < 120:
                            n += 1
            return n

        # the name field of the User page: the caret stands at its left edge
        click(NAME_X, NAME_Y)
        time.sleep(0.5)
        on = max(count(shot("caret_on"), 466, 500, 110, 128, True)
                 for _ in range(6))
        note(on > 0, f"the caret shows while the field has the focus ({on} pixels)")
        hidden = max(count(shot("caret_ph"), 466, 560, 112, 126, False)
                     for _ in range(3))
        note(hidden == 0, f"the placeholder steps aside while the field is edited "
                         f"({hidden} pixels)")

        # the room field takes the focus away: the caret goes with it
        click(NAME_X, NAME_Y + 20)
        time.sleep(0.5)
        off = max(count(shot("caret_off"), 466, 500, 110, 128, True)
                  for _ in range(6))
        note(off == 0, f"the caret is gone when the field loses the focus ({off} pixels)")
        back = max(count(shot("caret_back"), 466, 560, 112, 126, False)
                   for _ in range(3))
        note(back > 0, f"the placeholder comes back ({back} pixels)")
    finally:
        stop(p, f)


def test_map_layers():
    print("12. the map layers", flush=True)
    p, f, logpath = start({"PDA_START_TAB": "0", "PDA_GPS_LAT": "32.6478",
                            "PDA_GPS_LON": "-16.9193"}, "layers")
    try:
        time.sleep(3.0)

        COLOURS = {"area": (255, 255, 255),        # the rings
                   "trader": (40, 168, 200),      # the bag of coins (BTC Map)
                   "stash": (255, 211, 42),       # the yellow squares
                   "waypoint": (243, 156, 18)}    # the orange rings
        LAYER_X = {"trader": 163 + 148, "stash": 163 + 296, "waypoint": 163 + 444}

        def count(path, colour):
            from PIL import Image
            im = Image.open(path).convert("RGB")
            px = im.load()
            w, h = im.size
            n = 0
            for y in range(69 + 32, h):       # the map's area (the title bar is 32)
                for x in range(w):
                    r, g, b = px[x, y]
                    if abs(r - colour[0]) < 16 and abs(g - colour[1]) < 16 \
                       and abs(b - colour[2]) < 16:
                        n += 1
            return n

        # the markers are a few kilometres apart: zoom out until they are in view
        for i in range(3):
            click(770, 69 + 370 + 14)         # the zoom out button
        time.sleep(0.8)

        base = {k: count(shot("lay_base"), c) for k, c in COLOURS.items()}
        note(base["area"] > 0, f"the areas are on at boot ({base['area']} pixels)")

        for i, (name, colour) in enumerate(COLOURS.items()):
            if name == "area":
                continue
            x = LAYER_X[name]
            y = 45 - (list(LAYER_X).index(name))
            click_once(x, y)
            on = count(shot("lay_%s_on" % name), colour)
            click_once(x, y)
            off = count(shot("lay_%s_off" % name), colour)
            note(on > base[name] and off <= base[name],
                 f"the {name} come and go with their button "
                 f"({base[name]} -> {on} -> {off})".replace("the trader come", "the traders come")
                  .replace("the stash come", "the stashes come")
                  .replace("the waypoint come", "the waypoints come"))
    finally:
        stop(p, f)


def test_waypoints():
    print("13. the waypoints and the route", flush=True)
    p, f, logpath = start({"PDA_START_TAB": "0", "PDA_GPS_LAT": "32.6478",
                            "PDA_GPS_LON": "-16.9193"}, "waypoints")
    try:
        time.sleep(3.0)

        def count(path, what):
            """the pixels of a colour on the map (the title bar is 32)"""
            from PIL import Image
            im = Image.open(path).convert("RGB")
            px = im.load()
            w, h = im.size
            n = 0
            for y in range(69 + 32, h):
                for x in range(w):
                    r, g, b = px[x, y]
                    if what == "red":       # the pin of a waypoint
                        if r > 170 and g < 90 and b < 80:
                            n += 1
                    else:                   # the line of the route
                        if r > 160 and 130 < g < 195 and b < 80:
                            n += 1
            return n

        def right_click(x, y):
            from Xlib import display, X
            from Xlib.ext import xtest
            focus_window()
            d = display.Display()
            sx, sy = OX + x, OY + y
            xtest.fake_input(d, X.MotionNotify, x=sx, y=sy)
            d.sync()
            time.sleep(0.08)
            xtest.fake_input(d, X.ButtonPress, 3)
            d.sync()
            time.sleep(0.08)
            xtest.fake_input(d, X.ButtonRelease, 3)
            d.sync()
            time.sleep(0.4)

        red0 = count(shot("wp_none"), "red")
        line0 = count(shot("wp_none2"), "line")
        right_click(300, 200)
        red1 = count(shot("wp_one"), "red")
        line1 = count(shot("wp_one2"), "line")
        note(red1 > red0 + 40,
             f"a right click puts a waypoint down ({red0} -> {red1} pixels)")
        note(line1 > line0 + 60,
             f"the route to it is drawn ({line0} -> {line1} pixels)")
        right_click(300, 200)
        red2 = count(shot("wp_gone"), "red")
        note(red2 <= red0 + 20,
             f"right clicking it again takes it away ({red1} -> {red2})")

        # the Waypoints layer switches the route on and off
        right_click(300, 200)
        on = count(shot("wp_layer_on"), "red")
        click_once(163 + 3 * 148 + 6, 42)     # the Waypoints button
        off = count(shot("wp_layer_off"), "red")
        note(on > 40 and off <= 20,
             f"the Waypoints layer switches the route off ({on} -> {off})")
        click_once(163 + 3 * 148 + 6, 42)     # and on again
        back = count(shot("wp_layer_back"), "red")
        note(back > 40, f"and on again ({back})")

        # the button that takes the whole route away
        click_once(8 + 50, 89 + 8)
        cleared = count(shot("wp_cleared"), "red")
        note(cleared <= 20, f"CLEAR takes the whole route away ({back} -> {cleared})")
    finally:
        stop(p, f)


TESTS = {
    "tabs": test_tabs,
    "pages": test_other_pages,
    "name": test_name_save,
    "prefs": test_settings_file,
    "carriers": test_no_carriers,
    "hard": test_hard_use,
    "escape": test_json_escaping,
    "many": test_many_messages,
    "channel": test_mesh_channel,
    "recenter": test_recenter,
    "caret": test_caret,
    "layers": test_map_layers,
    "waypoints": test_waypoints,
}

def main():
    if not os.path.exists(APP):
        print("build the app first: ./build.sh")
        return 1
    os.makedirs(LOGDIR, exist_ok=True)
    subprocess.run(["pkill", "-x", "pda"], capture_output=True)
    time.sleep(0.5)

    want = sys.argv[1:] or list(TESTS.keys())
    for name in want:
        fn = TESTS.get(name)
        if not fn:
            print("no such test:", name)
            continue
        fn()
        print()

    print()
    if FAIL:
        print(f"{len(FAIL)} check(s) failed")
        for f in FAIL:
            print("  - " + f)
        return 1
    print("all checks passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
