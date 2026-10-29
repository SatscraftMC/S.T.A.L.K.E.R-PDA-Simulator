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
import atexit
import base64
import hashlib
import json
import os
import re
import shutil
import socket
import struct
import subprocess
import sys
import tempfile
import threading
import time

# Isolation, at module top *before* PREFS below is computed from it: no run
# of this suite ever writes the real ~/.config/pda/ (the identity key and
# contacts would land there on the first test run otherwise). A test that
# needs a specific config overrides PDA_PREFS for its own instances.
RUN_TMP = tempfile.mkdtemp(prefix="ui_test_")
atexit.register(lambda: shutil.rmtree(RUN_TMP, ignore_errors=True))
os.environ.setdefault("PDA_PREFS", os.path.join(RUN_TMP, "prefs.conf"))

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
APP = os.environ.get("PDA_TEST_BUILD", os.path.join(ROOT, "build", "pda"))
RELAY = os.path.join(ROOT, "tools", "nstr_relay.py")
sys.path.insert(0, os.path.join(ROOT, "tools"))
from wsclient import WSClient                      # noqa: E402  (ROOT first)

LOGDIR = "/tmp/opencode/uitest"
SHOTS = "/tmp/opencode/uishot"
# where prefs.c looks: $PDA_PREFS, then $XDG_CONFIG_HOME, then ~/.config.
# A desktop that sets XDG_CONFIG_HOME (a sandboxed one does) would put the
# file somewhere this suite could not see it - and then every check that
# reads the settings file would be reading a file nobody wrote.
PREFS = os.environ.get("PDA_PREFS") or os.path.join(
    os.environ.get("XDG_CONFIG_HOME") or os.path.expanduser("~/.config"),
    "pda", "prefs.conf")
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

APP_PID = None        # the app process the run in progress started


# --- the shared shapes of spec-nostr-identity -------------------------------
def unsigned_event(text, name, room="SECTOR", chan=None, ts=None):
    """The one unsigned event shape (spec §4): pubkey "", no sig, id =
    sha256 of the canonical serialization. Tests hold no key, so this is
    exactly the honest CHAT_SIG_NONE path the receiver must still display."""
    ts = int(ts if ts is not None else time.time())
    tags = [["room", room], ["t", room]]   # t mirrors room: the key the
    if chan:                               # relay's #t filter indexes
        tags.append(["chan", chan])
    tags.append(["name", name])
    ser = json.dumps([0, "", ts, 1, tags, text],
                     separators=(",", ":"), ensure_ascii=False)
    eid = hashlib.sha256(ser.encode("utf-8")).hexdigest()
    return {"id": eid, "pubkey": "", "created_at": ts, "kind": 1,
            "tags": tags, "content": text}


def name_of(msg, default=""):
    """the display label: the name tag (the label a sender signs)"""
    for t in msg.get("tags") or []:
        if isinstance(t, (list, tuple)) and len(t) > 1 and t[0] == "name":
            return t[1]
    return default


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


def relay_url(port):
    return f"ws://127.0.0.1:{port}"


def start_relay(port, name, tls=False, cert=None, key=None):
    """the local WebSocket relay (tools/nstr_relay.py), on stdin so a test
    can drive its control plane; --tls wraps the same server for the local
    TLS cases"""
    cmd = [sys.executable, RELAY, "--host", "127.0.0.1", "--port", str(port), "-v"]
    if tls:
        cmd += ["--tls", "--cert", cert, "--key", key]
    logpath = os.path.join(LOGDIR, f"{name}_relay.log")
    p = subprocess.Popen(cmd, stdin=subprocess.PIPE,
                         stdout=open(logpath, "w"),
                         stderr=subprocess.STDOUT, text=True)
    time.sleep(1.0)
    return p, logpath


def stop_relay(p):
    try:
        if p.stdin:
            p.stdin.close()
    except OSError:
        pass
    p.terminate()
    try:
        p.wait(timeout=5)
    except subprocess.TimeoutExpired:
        p.kill()


def peer_connect(port, sub="peer", tls=False, host="127.0.0.1", cafile=None):
    """the test peer: a WebSocket client with a subscription that takes
    every kind-1 event"""
    scheme = "wss" if tls else "ws"
    w = WSClient(f"{scheme}://{host}:{port}", timeout=5, cafile=cafile)
    w.send(json.dumps(["REQ", sub, {"kinds": [1]}]))
    return w


def peer_event(w, contains=None, timeout=8):
    """the next EVENT off the peer (the one whose content contains
    `contains`, or any). None when nothing arrived in time."""
    end = time.time() + timeout
    while time.time() < end:
        msg = w.recv(timeout=max(0.2, end - time.time()))
        if not msg:
            continue
        try:
            arr = json.loads(msg)
        except json.JSONDecodeError:
            continue
        if not isinstance(arr, list) or len(arr) < 3 or arr[0] != "EVENT":
            continue
        ev = arr[2]
        if not isinstance(ev, dict) or ev.get("kind") != 1:
            continue
        if contains is None or contains in (ev.get("content") or ""):
            return ev
    return None


def wait_for_log_from(path, pattern, start=0, timeout=20):
    """wait for `pattern` after byte offset `start` - wait_for_log() searches
    the whole file, which would match the FIRST connection's own lines when
    what we are waiting for is the second one"""
    rx = re.compile(pattern)
    end = time.time() + timeout
    while time.time() < end:
        try:
            data = _read(path)
        except OSError:
            data = ""
        if rx.search(data[start:]):
            return data
        time.sleep(0.3)
    try:
        return _read(path)
    except OSError:
        return ""


def gen_cert(name="nstr"):
    """a self-signed cert with an IP SAN, so the app's IP-form certificate
    check can pass under PDA_NSTR_CA. None when the openssl CLI is absent
    (the TLS cases then print SKIP - never a fake pass)."""
    d = os.path.join(LOGDIR, "certs")
    os.makedirs(d, exist_ok=True)
    cert, key = os.path.join(d, f"{name}.crt"), os.path.join(d, f"{name}.key")
    if not (os.path.exists(cert) and os.path.exists(key)):
        r = subprocess.run(
            ["openssl", "req", "-x509", "-newkey", "rsa:2048", "-nodes",
             "-subj", "/CN=localhost", "-addext", "subjectAltName=IP:127.0.0.1,DNS:localhost",
             "-keyout", key, "-out", cert, "-days", "2"],
            capture_output=True)
        if r.returncode != 0:
            return None, None
    return cert, key


def field_box(logpath, placeholder=None):
    """the last [field] built line (absolute client coordinates: what
    click() wants), optionally the one whose placeholder matches. Measured
    at run time - the first build of a page lays out differently from a
    later one, so a pinned constant here is a test that silently clicks
    the row above the field."""
    ms = re.findall(
        r"\[field\] built: x=(\d+) y=(\d+) w=(\d+) h=(\d+) content=\d+ "
        r"textlen=\d+ placeholder=(\S+)", _read(logpath))
    hits = [m for m in ms if placeholder is None or m[4] == placeholder]
    if not hits:
        return None
    m = hits[-1]
    return int(m[0]), int(m[1]), int(m[2]), int(m[3])


def chat_box(logpath, name):
    """the absolute box of a Messages-tab object (input / send), measured
    from PDA_DEBUG_FIELD at run time - (x, y, w, h) or None"""
    ms = re.findall(r"\[field\] chat: %s x=(\d+) y=(\d+) w=(\d+) h=(\d+)"
                    % re.escape(name), _read(logpath))
    if not ms:
        return None
    m = ms[-1]
    return int(m[0]), int(m[1]), int(m[2]), int(m[3])


def press_key(keysym, times=1):
    """press a key by name, N times (BackSpace to clear a field: the suite
    has no select-all that LVGL is known to honour)"""
    from Xlib import display, X, XK
    from Xlib.ext import xtest
    d = display.Display()
    code = d.keysym_to_keycode(XK.string_to_keysym(keysym))
    for _ in range(times):
        xtest.fake_input(d, X.KeyPress, code)
        d.sync()
        time.sleep(0.04)
        xtest.fake_input(d, X.KeyRelease, code)
        d.sync()
        time.sleep(0.04)
    d.close()


def start(env_extra=None, log="app"):
    env = dict(os.environ)
    env.setdefault("DISPLAY", ":0")
    env.update(env_extra or {})
    path = os.path.join(LOGDIR, f"{log}.log")
    f = open(path, "w")
    p = subprocess.Popen([APP], stdout=f, stderr=subprocess.STDOUT, env=env)
    global APP_PID
    APP_PID = p.pid
    return p, f, path


def stop(p, f):
    if p.poll() is None:
        p.terminate()
        try:
            p.wait(timeout=5)
        except subprocess.TimeoutExpired:
            p.kill()
    f.close()


def find_pda_window(d):
    """The PDA window, as an Xlib window object on the open display d.

    The title alone is not enough: a run that has not gone away yet leaves
    its own window on the screen with exactly the same title, and both the
    raise and the coordinate reading below would then be pointed at it -
    the screenshots come out perfect while the clicks land on a process
    that is no longer listening. _NET_WM_PID is the pid of the process
    start() launched, which is what this run's window is. """
    from Xlib import X
    root = d.screen().root
    utf8 = d.intern_atom("UTF8_STRING")
    wm_name = d.intern_atom("_NET_WM_NAME")
    pid_atom = d.intern_atom("_NET_WM_PID")
    clients = root.get_full_property(
        d.intern_atom("_NET_CLIENT_LIST"), X.AnyPropertyType)
    if not clients:
        return None

    fallback = None
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
        if APP_PID:
            try:
                pp = w.get_property(pid_atom, X.AnyPropertyType, 0, 1)
            except Exception:
                pp = None
            if pp and pp.value and int(pp.value[0]) == APP_PID:
                return w
        if fallback is None:
            fallback = w
    return fallback


def focus_window():
    wid = None
    try:
        from Xlib import display
        d = display.Display()
        try:
            w = find_pda_window(d)
            if w is not None:
                wid = "%#x" % w.id
        finally:
            d.close()
    except Exception:
        wid = None

    if wid:
        subprocess.run(["wmctrl", "-i", "-a", wid], capture_output=True)
    else:
        subprocess.run(["wmctrl", "-a", "PDA // Simulator"], capture_output=True)
    time.sleep(0.4)
    window_origin()


def window_origin():
    """Where the PDA client area really is, in root coordinates.

    OX/OY above were measured on the desktop this suite was written for,
    where the window happened to land there. It does not always: the
    window manager places it wherever there is a gap, and wmctrl -lG
    reports yet another position (the frame's, off by the title bar). So
    every click and every screenshot reads the client's real position out
    of the X tree first - otherwise the clicks land on the wallpaper
    while the screenshots still come out perfect, which is a very quiet
    way for a suite to report on nothing. """
    global OX, OY
    from Xlib import display
    d = None
    try:
        d = display.Display()
        w = find_pda_window(d)
        if w is None:
            return
        # walk up to the root: the client is reparented into the frame
        root = d.screen().root
        x = y = 0
        cur = w
        while cur.id != root.id:
            g = cur.get_geometry()
            x += g.x
            y += g.y
            cur = cur.query_tree().parent
        OX, OY = x, y
    except Exception:
        pass
    finally:
        if d is not None:
            try:
                d.close()
            except Exception:
                pass


def shot(name):
    focus_window()
    path = f"{SHOTS}_{name}.png"
    # Delete before capturing: gnome-screenshot fails now and then, and a
    # file left over from the run before is a perfectly good picture of
    # the wrong app - the hardest kind of test bug to see, because every
    # check that reads it gets an answer, just not this run's. With the
    # old file gone, a failed capture is loud instead of stale.
    for attempt in range(3):
        try:
            os.unlink(path)
        except OSError:
            pass
        r = subprocess.run(["gnome-screenshot", "-w", "-f", path],
                           capture_output=True)
        if os.path.exists(path):
            return path
        if attempt == 0 and r.stderr:
            why = r.stderr.decode("utf-8", "replace").strip().splitlines()
            print("    screenshot failed: " + (why[-1] if why else "no reason"),
                  flush=True)
        time.sleep(0.5)
    raise RuntimeError(f"screenshot {path} could not be taken")


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


# the screenshot is the *frame*: 1 px of window border on the left and the
# 32 px title bar on top (measured against a known landmark - the strip's
# accent square - and cross-checked with _NET_FRAME_EXTENTS)
SHOT_DX = 1
SHOT_DY = 32


def colour_pixels(path, x0, x1, y0, y1, rgb, tol=36):
    """how many pixels in that box are close to rgb (client coordinates in,
    screenshot pixels counted)"""
    try:
        from PIL import Image
    except ImportError:
        return -1
    im = Image.open(path).convert("RGB")
    px = im.load()
    want = ((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF)
    n = 0
    for y in range(y0 + SHOT_DY, min(y1 + SHOT_DY, im.size[1])):
        for x in range(x0 + SHOT_DX, min(x1 + SHOT_DX, im.size[0])):
            r, g, b = px[x, y]
            if abs(r - want[0]) <= tol and abs(g - want[1]) <= tol and                abs(b - want[2]) <= tol:
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
    # python-xlib resolves keysym *names*, not the punctuation characters
    # themselves (string_to_keysym(':') is 0, string_to_keysym('colon') is
    # 0x3a) - and a relay URL is all punctuation, so the settings rows could
    # not be typed into without this map.
    NAMES = {" ": "space", "/": "slash", ":": "colon", ".": "period",
             "-": "minus", "_": "underscore", "@": "at", "=": "equal",
             "+": "plus", "?": "question", ",": "comma", "%": "percent",
             "'": "apostrophe", '"': "quotedbl", "&": "ampersand"}
    d = display.Display()
    shift = d.keysym_to_keycode(XK.string_to_keysym("Shift_L"))
    for ch in s:
        ks = XK.string_to_keysym(ch) or XK.string_to_keysym(NAMES.get(ch, ""))
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


def wait_for_log(path, pattern, timeout=12):
    """poll the app log until the pattern shows up (the log-driven checks)"""
    rx = re.compile(pattern)
    end = time.time() + timeout
    while time.time() < end:
        try:
            with open(path, errors="replace") as f:
                data = f.read()
        except OSError:
            data = ""
        if rx.search(data):
            return data
        time.sleep(0.3)
    try:
        with open(path, errors="replace") as f:
            return f.read()
    except OSError:
        return ""


def target_pid(pid):
    """point the window helpers at a specific instance (start() points them
    at whatever it launched last, and these tests run several at once)"""
    global APP_PID
    APP_PID = pid


def box_lit(path, x0, x1, y0, y1):
    """bright pixels in a box of the screenshot (client coordinates in)"""
    try:
        from PIL import Image
    except ImportError:
        return -1
    im = Image.open(path).convert("RGB")
    px = im.load()
    n = 0
    for y in range(y0 + SHOT_DY, min(y1 + SHOT_DY, im.size[1])):
        for x in range(x0 + SHOT_DX, min(x1 + SHOT_DX, im.size[0])):
            r, g, b = px[x, y]
            if r + g + b > 330:
                n += 1
    return n


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


def decode_qr(path):
    """what the QR codes in this screenshot say (None when there is none)"""
    try:
        from pyzbar.pyzbar import decode
        from PIL import Image
    except ImportError as exc:
        note(False, "the wallet QR checks need pyzbar and pillow "
                    "(apt install libzbar0t64; pip install pyzbar): %s" % exc)
        return None
    for sym in decode(Image.open(path)):
        if sym.type == "QRCODE":
            return sym.data.decode("utf-8", "replace")
    return None


def last_invoice(logpath):
    """the bolt11 the wallet made last - it prints one every time it makes one"""
    try:
        with open(logpath, errors="replace") as fh:
            got = [line.split("invoice:", 1)[1].strip()
                   for line in fh if "invoice:" in line]
    except OSError:
        return None
    return got[-1] if got else None


def read_mock_balance(path):
    """what the preview backend wrote to its state file"""
    try:
        with open(path) as fh:
            m = re.search(r"balance\s*=\s*(-?\d+)", fh.read())
    except OSError:
        return None
    return int(m.group(1)) if m else None


def read_mock_history(path):
    """the history rows the preview backend kept, newest first"""
    try:
        with open(path) as fh:
            return [l.rstrip("\n") for l in fh if l.startswith("history = ")]
    except OSError:
        return []


def hist_status_pixels(path):
    """bright pixels where only a history row can put text.

    The first row's status is right-aligned at the end of client x 646..768
    and its line runs from client y 86 to 108 (the panel starts 34 px under
    the tab row; the screenshot adds the window decoration on top). The
    empty state is one line centred on x=400, the list border is at x=784
    and the scrollbar past x=775 - none of them can reach in here. """
    try:
        from PIL import Image
    except ImportError:
        return -1
    im = Image.open(path).convert("RGB")
    w, h = im.size
    off = h - 480                       # the decoration above the 800x480 client
    px = im.load()
    n = 0
    for y in range(86 + off, min(108 + off, h)):
        for x in range(700, min(776, w)):
            r, g, b = px[x, y]
            if r + g + b > 300:
                n += 1
    return n


def qr_bright(path, x0=32, x1=328, y0=150, y1=442):
    """white pixels inside the REQUEST screen's QR frame.

    The coordinates are the panel's own; the screenshot adds the window
    decoration on top (measured, not assumed). A drawn QR code fills the
    frame with white, the PAID screen that replaces it does not. """
    try:
        from PIL import Image
    except ImportError:
        return -1
    im = Image.open(path).convert("RGB")
    w, h = im.size
    off = h - 480                       # the decoration above the 800x480 client
    px = im.load()
    n = 0
    for y in range(y0 + 34 + off, min(y1 + 34 + off, h)):
        for x in range(x0, min(x1, w)):
            r, g, b = px[x, y]
            if r > 225 and g > 225 and b > 225:
                n += 1
    return n


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
    pages = [(0, s) for s in range(6)] + [(1, s) for s in range(5)] + \
            [(2, 0)]
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

    p, f, logpath = start({"PDA_START_TAB": "6", "PDA_OTHER_PAGE": "0", "PDA_OTHER_SUB": "5"}, "name")
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
    p, f, logpath = start({"PDA_START_TAB": "6", "PDA_OTHER_PAGE": "0", "PDA_OTHER_SUB": "5"}, "name2")
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
    p, f, logpath = start({"PDA_START_TAB": "6", "PDA_OTHER_PAGE": "0", "PDA_OTHER_SUB": "5"}, "broken")
    time.sleep(2.6)
    note(app_alive(p, logpath), "a broken settings file does not kill the app")
    stop(p, f)

    # a name with the characters that break a JSON event line
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
    p, f, logpath = start({"PDA_START_TAB": "6", "PDA_OTHER_PAGE": "0", "PDA_OTHER_SUB": "5"}, "long")
    time.sleep(2.6)
    ok = app_alive(p, logpath)
    shot("longname")
    stop(p, f)
    note(ok, "a very long name does not kill the app")

    if os.path.exists(PREFS):
        os.unlink(PREFS)


def test_no_carriers():
    print("5. no relay, no daemon", flush=True)
    # a URL at a port nothing is listening on: the carrier tries, says so,
    # and the app carries on. One URL, so it cannot be half-set the way
    # PDA_RELAY_PORT without PDA_RELAY_HOST was (the parked item of the
    # last baseline - now runnable)
    p, f, logpath = start({"PDA_START_TAB": "1",
                            "PDA_NSTR_RELAY": relay_url(free_port()),
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
        for i in range(3):
            click(NAV_X, NAV_Y0 + NAV_PITCH * i)
        time.sleep(0.5)
        # save a name and walk away from the page while it flashes
        click(NAV_X, NAV_Y0 + NAV_PITCH * 0)     # the Settings section
        time.sleep(0.3)
        click(SUB_X, SUB_Y0 + SUB_PITCH * 5)     # the User page
        time.sleep(0.3)
        click(NAME_X, NAME_Y)       # the name field
        type_text("Satscraft")
        click(SAVE_X, NAME_Y)       # SAVE
        click(NAV_X, NAV_Y0 + NAV_PITCH * 1)     # and straight to another page
        time.sleep(1.8)             # long enough for the flash to fire
        click(SUB_X, SUB_Y0 + SUB_PITCH * 5)     # and back again
        time.sleep(0.4)
        ok = app_alive(p, logpath)
        shot("hard")
    finally:
        stop(p, f)
    note(ok, "clicking through everything does not crash the app")



def test_json_escaping():
    print("7. a name that would break the wire format", flush=True)
    port = free_port()
    relay, rlog_path = start_relay(port, "escape")
    peer = peer_connect(port)
    name = 'Ro"ok\\Blue'
    p, f, logpath = start({"PDA_START_TAB": "1", "PDA_DEBUG": "1",
                           "PDA_NSTR_RELAY": relay_url(port),
                           "PDA_NAME": name, "PDA_TEST_SEND": "hello there",
                           "PDA_TEST_SEND_IN": "5"},
                          "escape")
    try:
        wait_for_log(logpath, r"\[nstr\] up", 15)
        got = peer_event(peer, contains="hello there", timeout=10)
        note(got is not None and name_of(got) == name,
             f"the relay parsed the message from {name!r} "
             f"(got {got and name_of(got)!r})")
        with open(rlog_path) as fh:
            rlog = fh.read()
        note("bad line" not in rlog, "the relay did not see a broken line")
        note(app_alive(p, logpath), "the app is alive with the escaped name")
    finally:
        stop(p, f)
        peer.close()
        stop_relay(relay)


def test_many_messages():
    print("8. a lot of messages", flush=True)
    port = free_port()
    relay, _ = start_relay(port, "many")
    peer = peer_connect(port)
    p, f, logpath = start({"PDA_START_TAB": "1", "PDA_DEBUG": "1",
                           "PDA_NSTR_RELAY": relay_url(port),
                           "PDA_BIT_SCAN": "off"},
                          "many")
    try:
        wait_for_log(logpath, r"\[nstr\] up", 15)
        long_msg = "a very long message " * 40
        for i in range(220):
            peer.send(json.dumps(["EVENT", unsigned_event(
                long_msg if i == 5 else f"message {i}", "ROOK")]))
        data = wait_for_log(logpath, r"message 219", 25)
        n = len(re.findall(r"\[chat\] recv via NSTR", data))
        note("message 219" in data, "the last of the 220 messages arrived")
        note(n >= 200, f"the app really received them ({n} of 220 in the log)")
        ok = app_alive(p, logpath)
        shot("many")
        note(ok, "220 messages (one of them very long) do not kill the app")
    finally:
        stop(p, f)
        peer.close()
        stop_relay(relay)


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
        peer.sendto((json.dumps(unsigned_event(
            "not for us", "ROOK", chan="other")) + "\n").encode(),
            ("127.0.0.1", 2425))
        time.sleep(1.2)
        peer.sendto((json.dumps(unsigned_event(
            "for the mesh", "ROOK", chan="mesh")) + "\n").encode(),
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
    p, f, logpath = start({"PDA_START_TAB": "6", "PDA_OTHER_PAGE": "0", "PDA_OTHER_SUB": "5"}, "caret")
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
    # a clean settings file: a route that is left over would turn its layer on
    prefs = "/tmp/opencode/uitest/layers_prefs.conf"
    if os.path.exists(prefs):
        os.unlink(prefs)
    p, f, logpath = start({"PDA_START_TAB": "0", "PDA_GPS_LAT": "32.6478",
                            "PDA_GPS_LON": "-16.9193", "PDA_PREFS": prefs}, "layers")
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
    # a clean settings file: a route that is left over would be there already
    prefs = "/tmp/opencode/uitest/waypoints_prefs.conf"
    if os.path.exists(prefs):
        os.unlink(prefs)
    p, f, logpath = start({"PDA_START_TAB": "0", "PDA_GPS_LAT": "32.6478",
                            "PDA_GPS_LON": "-16.9193", "PDA_PREFS": prefs}, "waypoints")
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
                    else:                   # the line of the route (light blue)
                        if abs(r - 136) < 20 and abs(g - 216) < 20 and abs(b - 240) < 20:
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
        time.sleep(0.4)                       # and let the map draw it
        off = count(shot("wp_layer_off"), "red")
        note(on > 40 and off <= 20,
             f"the Waypoints layer switches the route off ({on} -> {off})")
        click_once(163 + 3 * 148 + 6, 42)     # and on again
        back = count(shot("wp_layer_back"), "red")
        note(back > 40, f"and on again ({back})")

        # the button that takes the whole route away is in the column of the
        # zoom buttons, and it is only there when the route is on the map
        def accent(path, x0, y0, x1, y1):
            """the pixels of the accent colour in that box (the title bar is 32)"""
            from PIL import Image
            im = Image.open(path).convert("RGB")
            px = im.load()
            n = 0
            for y in range(y0 + 32, y1 + 32):
                for x in range(x0, x1):
                    r, g, b = px[x, y]
                    if abs(r - 200) < 12 and abs(g - 160) < 12 and abs(b - 32) < 12:
                        n += 1
            return n

        def text(path, x0, y0, x1, y1):
            """the pixels of the text colour in that box"""
            from PIL import Image
            im = Image.open(path).convert("RGB")
            px = im.load()
            n = 0
            for y in range(y0 + 32, y1 + 32):
                for x in range(x0, x1):
                    r, g, b = px[x, y]
                    if abs(r - 152) < 12 and abs(g - 156) < 12 and abs(b - 144) < 12:
                        n += 1
            return n

        BTN = (760, 69 + 266, 780, 69 + 286)     # the clear button's cross
        click_once(770, 69 + 262 + 14)            # the clear button itself
        cleared = count(shot("wp_cleared"), "red")
        note(cleared <= 20, f"CLEAR takes the whole route away ({back} -> {cleared})")
        btn0 = accent(shot("wp_btn0"), *BTN)
        note(btn0 == 0, f"the clear button goes with the route ({btn0})")
        right_click(300, 200)
        btn1 = accent(shot("wp_btn1"), *BTN)
        note(btn1 > 20, f"and comes with it ({btn1})")
        click_once(163 + 3 * 148 + 6, 42)         # the Waypoints layer off
        btn2 = accent(shot("wp_btn2"), *BTN)
        note(btn2 == 0, f"and goes with the layer ({btn1} -> {btn2})")
        click_once(163 + 3 * 148 + 6, 42)         # and on again

        # the coordinates of the mouse and of the player
        coord = text(shot("wp_coord"), 600, 69 + 2, 792, 69 + 28)
        note(coord > 20, f"the coordinates are on the map ({coord} pixels)")
    finally:
        stop(p, f)

def test_mesh_map():
    print("14. the mesh map", flush=True)
    import socket
    import json
    prefs = "/tmp/opencode/uitest/mesh_prefs.conf"
    if os.path.exists(prefs):
        os.unlink(prefs)
    p, f, logpath = start({"PDA_START_TAB": "6", "PDA_OTHER_PAGE": "1",
                            "PDA_OTHER_SUB": "1", "PDA_PREFS": prefs}, "mesh")
    peer = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        time.sleep(3.0)

        def names(path):
            """the pixels of the text colour in the mesh map: the names of
               the peers are written under their nodes"""
            from PIL import Image
            im = Image.open(path).convert("RGB")
            px = im.load()
            n = 0
            for y in range(52 + 32, 302 + 32):
                for x in range(315, 755):
                    r, g, b = px[x, y]
                    if abs(r - 152) < 12 and abs(g - 156) < 12 and abs(b - 144) < 12:
                        n += 1
            return n

        before = names(shot("mesh_before"))

        # a device on the local mesh says something: it is a node of the map
        peer.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        peer.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        peer.bind(("127.0.0.2", 2425))
        peer.sendto((json.dumps(unsigned_event(
            "anyone there", "ROOK")) + "\n").encode(),
            ("127.0.0.1", 2425))
        time.sleep(1.5)
        after = names(shot("mesh_after"))
        note(after > before + 20,
             f"the peer shows up as a node ({before} -> {after} pixels)")
    finally:
        peer.close()
        stop(p, f)


def test_wallet():
    print("15. the Wallet tab", flush=True)

    # the preview backend's state file: the tests move money, and that must
    # not touch whatever the developer's own wallet.mock holds
    mockfile = os.path.join(LOGDIR, "wallet.mock")

    # no backend configured: the honest "not set up" screen (the stub that
    # used to say "Balance: 0 sats" is gone, and must not come back)
    p, f, logpath = start({"PDA_START_TAB": "5"}, "wallet_unset")
    time.sleep(2.6)
    note(lit_pixels(shot("wallet_unset"), y0=69) > 20,
         "the wallet tab renders without a backend")
    note("Balance: 0 sats" not in open(os.path.join(
        ROOT, "src", "ui", "pda_shell.c")).read(),
        "the old balance stub is gone from the shell")
    note(app_alive(p, logpath), "the app is alive with no backend")
    stop(p, f)

    # PDA_WALLET_BACKEND=mock: the balance screen
    if os.path.exists(mockfile):
        os.unlink(mockfile)
    p, f, logpath = start({"PDA_START_TAB": "5", "PDA_WALLET_BACKEND": "mock",
                           "PDA_WALLET_MOCK": mockfile},
                          "wallet_mock")
    time.sleep(2.6)
    note(lit_pixels(shot("wallet_mock"), y0=69) > 20,
         "the wallet tab renders the mock balance")
    note(app_alive(p, logpath), "the app is alive with the mock backend")
    stop(p, f)

    # a backend that does not exist: the error screen, and the app lives
    p, f, logpath = start({"PDA_START_TAB": "5", "PDA_WALLET_BACKEND": "lightningd"},
                          "wallet_bad")
    time.sleep(2.6)
    note(lit_pixels(shot("wallet_bad"), y0=69) > 20,
         "the wallet tab renders the error of an unknown backend")
    note(app_alive(p, logpath), "the app is alive with an unknown backend")
    stop(p, f)

    # ---- REQUEST: the QR code, the amount keypad, the settle ----
    # app coordinates: the panel sits under the 34px tab row
    REQUEST_BTN = (142, 390)
    STEP_MINUS10 = (212, 101)
    MEMO_FIELD = (200, 154)
    NEW_REQUEST = (438, 289)
    BACK_BTN = (730, 49)
    DEFAULT_BALANCE = 1234567

    if os.path.exists(mockfile):
        os.unlink(mockfile)
    p, f, logpath = start({"PDA_START_TAB": "5", "PDA_WALLET_BACKEND": "mock",
                           "PDA_WALLET_MOCK": mockfile,
                           # the retheme step below jumps straight there
                           "PDA_OTHER_PAGE": "0", "PDA_OTHER_SUB": "3"},
                          "wallet_req")
    try:
        time.sleep(2.6)
        click_once(*REQUEST_BTN)
        time.sleep(1.0)
        path = shot("wallet_req")
        note(lit_pixels(path, y0=180) > 500, "the REQUEST screen comes up")

        inv1 = last_invoice(logpath)
        qr1 = decode_qr(path)
        note(qr1 is not None and inv1 is not None and qr1 == inv1,
             "the QR on the screen decodes back to the invoice the wallet made")

        # the amount keypad: -10 sats, a new invoice, a new code
        click_once(*STEP_MINUS10)
        time.sleep(1.0)
        path = shot("wallet_step")
        inv2 = last_invoice(logpath)
        qr2 = decode_qr(path)
        note(qr2 is not None and qr2 == inv2 and qr2 != inv1,
             "the amount keypad makes a new invoice and a new QR code")

        lit_before = qr_bright(path)
        note(lit_before > 5000, f"the QR code fills the frame ({lit_before} white pixels)")

        # the mock settles a few seconds later, and the screen flips
        time.sleep(8.5)
        path = shot("wallet_paid")
        lit_after = qr_bright(path)
        note(lit_after < lit_before // 10,
             f"the payment flips the screen to PAID ({lit_before} -> {lit_after} white pixels)")
        balance = read_mock_balance(mockfile)
        note(balance == DEFAULT_BALANCE + 490,
             f"the preview backend paid it out of the balance ({balance})")

        # a theme change tears every panel down and builds it again. The
        # invoice is settled and stable, so the rebuild must come back with
        # exactly the same one - and with the buttons still on it.
        click_once(600, 22)                    # the Other tab
        time.sleep(0.5)
        click_once(599, 112)                   # cycle the colour scheme
        time.sleep(2.0)
        click_once(120 + 5 * 80, 22)           # and back to the Wallet tab
        time.sleep(1.0)
        note(last_invoice(logpath) == inv2,
             "a theme change rebuilds the screen without making a new invoice")

        # NEW REQUEST: another invoice, the code comes back - and this can
        # only happen if the rebuilt panel really is the REQUEST screen
        click_once(*NEW_REQUEST)
        time.sleep(1.0)
        path = shot("wallet_newreq")
        inv3 = last_invoice(logpath)
        qr3 = decode_qr(path)
        note(qr3 is not None and qr3 == inv3 and qr3 != inv2,
             "NEW REQUEST makes another invoice and puts a QR code back")

        # the memo: what it is for goes into the invoice that is made next
        click_once(*MEMO_FIELD)
        time.sleep(0.4)
        type_text("coffee")
        time.sleep(1.2)
        path = shot("wallet_memo")
        inv4 = last_invoice(logpath)
        qr4 = decode_qr(path)
        note(qr4 is not None and qr4 == inv4 and qr4 != qr3,
             "typing a memo makes a new invoice for it")

        # and BACK goes home
        click_once(*BACK_BTN)
        time.sleep(0.8)
        note(qr_bright(shot("wallet_back")) < 500, "BACK leaves the REQUEST screen")
        note(app_alive(p, logpath), "the app is alive through the whole wallet flow")
    finally:
        stop(p, f)

    # ---- SEND and HISTORY: an invoice in, a payment out, kept on disk ----
    # panel coordinates plus the 34 px the panel sits under the tab row
    SEND_BTN = (314, 390)
    HIST_BTN = (486, 390)
    INV_FIELD = (400, 124)
    SEND_MEMO = (400, 194)
    CHECK_BTN = (104, 248)
    CANCEL_BTN = (290, 343)
    PAY_BTN = (510, 343)
    DONE_BTN = (400, 343)
    sent_invoice = "lnbc5000n1p" + "q" * 160     # 500 sats, bolt11 shaped

    if os.path.exists(mockfile):
        os.unlink(mockfile)
    p, f, logpath = start({"PDA_START_TAB": "5", "PDA_WALLET_BACKEND": "mock",
                           "PDA_WALLET_MOCK": mockfile,
                           "PDA_INVOICE": sent_invoice},
                          "wallet_send")
    try:
        time.sleep(2.6)

        # HISTORY while there is nothing in it yet
        click_once(*HIST_BTN)
        time.sleep(0.8)
        empty_path = shot("wallet_hist_empty")
        note(lit_pixels(empty_path, y0=60, y1=470) > 200,
             "the HISTORY screen comes up")
        empty = hist_status_pixels(empty_path)
        note(empty <= 5, f"nothing is drawn while the history is empty ({empty} px)")
        click_once(*BACK_BTN)
        time.sleep(0.6)

        # SEND: the invoice the simulator was handed, with a memo on it
        click_once(*SEND_BTN)
        time.sleep(0.8)
        note(lit_pixels(shot("wallet_send"), y0=180) > 100, "the SEND screen comes up")

        click_once(*SEND_MEMO)
        time.sleep(0.4)
        type_text("lunch")
        time.sleep(0.8)

        click_once(*CHECK_BTN)
        time.sleep(1.0)
        note(lit_pixels(shot("wallet_confirm"), y0=180) > 100,
             "CHECK reads the field and brings up CONFIRM PAY")
        with open(logpath, errors="replace") as fh:
            log = fh.read()
        note("[wallet] check: 500 sats" in log,
             "the invoice in the field is worth 500 sats")

        # PAY: the numbers move, and the screen says so
        click_once(*PAY_BTN)
        time.sleep(1.0)
        note(lit_pixels(shot("wallet_sent"), y0=180) > 100,
             "the payment comes back with a result screen")
        balance = read_mock_balance(mockfile)
        note(balance == DEFAULT_BALANCE - 501,
             f"the amount and its fee leave the balance ({balance})")
        with open(logpath, errors="replace") as fh:
            log = fh.read()
        note("[wallet] pay: 500 sats + 1 fee, balance 1234066" in log,
             "the backend logged exactly what moved")

        click_once(*DONE_BTN)
        time.sleep(0.6)

        # the row it left behind
        click_once(*HIST_BTN)
        time.sleep(0.8)
        path = shot("wallet_hist")
        filled = hist_status_pixels(path)
        note(filled > 10, f"a row is drawn on the HISTORY screen ({filled} px)")
        note(lit_pixels(path, y0=60, y1=470) > 200, "the HISTORY screen renders")
        rows = read_mock_history(mockfile)
        note(len(rows) == 1 and re.search(r"\|out\|500\|paid\|lunch$", rows[0]),
             f"the payment is on disk as one history row ({rows})")

        click_once(*BACK_BTN)
        time.sleep(0.5)

        # a paid invoice is not offered a second time
        click_once(*SEND_BTN)
        time.sleep(0.8)
        click_once(*CHECK_BTN)
        time.sleep(0.8)
        with open(logpath, errors="replace") as fh:
            log = fh.read()
        note("check refused: there is no invoice in the field" in log,
             "after it has been paid the field is empty and says so")
        note(app_alive(p, logpath), "the app is alive through the whole payment")
    finally:
        stop(p, f)

    # the balance and the history were written: they come back
    p, f, logpath = start({"PDA_START_TAB": "5", "PDA_WALLET_BACKEND": "mock",
                           "PDA_WALLET_MOCK": mockfile}, "wallet_reload")
    try:
        time.sleep(2.6)
        click_once(*HIST_BTN)
        time.sleep(0.8)
        filled = hist_status_pixels(shot("wallet_hist_again"))
        note(filled > 10, f"the history is on screen after a restart ({filled} px)")
        rows = read_mock_history(mockfile)
        note(len(rows) == 1, f"the history row survived the restart ({len(rows)})")
        note(read_mock_balance(mockfile) == DEFAULT_BALANCE - 501,
             "the balance survived the restart too")
        note(app_alive(p, logpath), "the app is alive on the restart")
    finally:
        stop(p, f)

    # an invoice typed into the field, with no PDA_INVOICE to help
    p, f, logpath = start({"PDA_START_TAB": "5", "PDA_WALLET_BACKEND": "mock",
                           "PDA_WALLET_MOCK": mockfile}, "wallet_typed")
    try:
        time.sleep(2.6)
        click_once(*SEND_BTN)
        time.sleep(0.8)
        click_once(*INV_FIELD)
        time.sleep(0.4)
        type_text("lnbc100n1pqqqqqqqq")       # 10 sats, bech32 data only
        time.sleep(0.8)
        click_once(*CHECK_BTN)
        time.sleep(1.0)
        with open(logpath, errors="replace") as fh:
            log = fh.read()
        note("[wallet] check: 10 sats" in log,
             "an invoice typed into the field is read back (10 sats)")
        note(lit_pixels(shot("wallet_typed_confirm"), y0=180) > 100,
             "and it brings up CONFIRM PAY")
        note(app_alive(p, logpath), "the app is alive after typing an invoice")
    finally:
        stop(p, f)

    # rubbish in the field: refused with the reason, and nothing happens
    p, f, logpath = start({"PDA_START_TAB": "5", "PDA_WALLET_BACKEND": "mock",
                           "PDA_WALLET_MOCK": mockfile,
                           "PDA_INVOICE": "not an invoice at all"},
                          "wallet_rubbish")
    try:
        time.sleep(2.6)
        click_once(*SEND_BTN)
        time.sleep(0.8)
        click_once(*CHECK_BTN)
        time.sleep(0.8)
        with open(logpath, errors="replace") as fh:
            log = fh.read()
        note("check refused: that is not a bolt11 invoice" in log,
             "a rubbish invoice is refused with a reason")
        note("[wallet] check: " not in log, "and nothing is confirmed from it")
        note("[wallet] pay" not in log, "and nothing is paid for it")
        note(app_alive(p, logpath), "the app is alive after a refused invoice")
    finally:
        stop(p, f)

    # ---- the merchant card: a tap on the map, [REQUEST], the Wallet ----
    # The pack is ours (PDA_POIS), one merchant in it, standing where the
    # view starts - and the Madeira pack has no companion-app merchant in
    # it, so the second record is written by hand: one that needs its own
    # app must not be offered [REQUEST] (BTC_MAP_INTEGRATION.md 7.1), and
    # the card is what carries the rule.
    pois = os.path.join(LOGDIR, "pois")
    pack = os.path.join(pois, "ui-test")
    os.makedirs(pack, exist_ok=True)
    with open(os.path.join(pack, "pack.cfg"), "w") as fh:
        fh.write("name ui-test\nsource ui-test\n")

    def place(name, flags, lat=32.6478, lon=-16.9193):
        """one 152-byte record - the layout src/poi.h and fetch_btc.py share"""
        rec = struct.pack("<dd", lat, lon)
        rec += name.encode()[:47].ljust(48, b"\0")
        rec += b"1 High Street".ljust(64, b"\0")
        rec += b"cafe".ljust(16, b"\0")
        rec += bytes([flags, 0]) + struct.pack("<H", 2025)
        rec += b"\0" * 4                # the tail padding the C struct has
        if len(rec) != 152:
            raise AssertionError("a place is 152 bytes, this is %d" % len(rec))
        return rec

    def write_pack(rec):
        with open(os.path.join(pack, "places.bin"), "wb") as fh:
            fh.write(rec)

    def accent(path, x0, y0, x1, y1):
        """the accent colour in a box of the app (the title bar is on top).

        The settings file below is a fresh one, so the accent is the default
        amber; and the card draws its own background over the map, so only
        the card's amber can land in here. """
        from PIL import Image
        im = Image.open(path).convert("RGB")
        w, h = im.size
        off = h - 480
        px = im.load()
        n = 0
        for y in range(y0 + off, min(y1 + off, h)):
            for x in range(x0, min(x1, w)):
                r, g, b = px[x, y]
                if abs(r - 201) < 24 and abs(g - 162) < 24 and abs(b - 39) < 24:
                    n += 1
        return n

    # where the card's own parts are, in app coordinates: the card sits at
    # the bottom left of the map (y 412), its padding and border put the
    # content at 13,417 - the name row, then the button on the right
    NAME_BOX = (13, 419, 313, 432)
    BTN_BOX = (321, 425, 419, 451)
    BTN_MID = (370, 438)
    CENTER = (400, 69 + 205)        # the view starts on the GPS fix: our place
    TRADERS = (163 + 148 + 6, 45)   # the Traders button of the second row
    CARD_ENV = {"PDA_START_TAB": "0", "PDA_WALLET_BACKEND": "mock",
                "PDA_WALLET_MOCK": mockfile, "PDA_POIS": pois,
                "PDA_GPS_LAT": "32.6478", "PDA_GPS_LON": "-16.9193"}

    def open_card(prefs, name, flags, shot_name):
        """start the app, put the Traders on, tap the place: the card"""
        for pth in (prefs, mockfile):
            if os.path.exists(pth):
                os.unlink(pth)
        env = dict(CARD_ENV)
        env["PDA_PREFS"] = prefs
        write_pack(place(name, flags))
        proc, out, log = start(env, shot_name)
        time.sleep(3.0)
        click_once(*TRADERS)        # the layer is off at boot
        time.sleep(0.6)
        click_once(*CENTER)         # a tap on the merchant
        time.sleep(0.8)
        return proc, out, log, shot(shot_name)

    card_prefs = os.path.join(LOGDIR, "card_prefs.conf")
    p, f, logpath, path = open_card(card_prefs, "Ordinary Coffee",
                                    0x01 | 0x10, "wallet_card")
    try:
        named = accent(path, *NAME_BOX)
        note(named > 50, f"the merchant's card comes up with its name on it ({named} px)")
        btn = accent(path, *BTN_BOX)
        note(btn > 50, f"and it carries a [REQUEST] button ({btn} px)")

        click_once(*BTN_MID)        # the button itself
        time.sleep(1.0)
        path = shot("wallet_card_req")
        note(qr_bright(path) > 500, "[REQUEST] opens the Wallet with a QR code on it")
        with open(logpath, errors="replace") as fh:
            log = fh.read()
        note("[wallet] memo: Ordinary Coffee" in log,
             "the merchant's name is the memo the invoice was made for")
        note(app_alive(p, logpath), "the app is alive through the merchant card")
    finally:
        stop(p, f)

    # a merchant that needs its own app will not take a plain BOLT11, so it
    # gets no button - and no invoice is asked for behind the user's back
    p, f, logpath, path = open_card(card_prefs, "Needs Its Own App",
                                    0x01 | 0x10 | 0x20, "wallet_card_comp")
    try:
        named = accent(path, *NAME_BOX)
        note(named > 50, f"its card comes up all the same ({named} px)")
        btn = accent(path, *BTN_BOX)
        note(btn <= 5, f"a companion-app merchant gets no [REQUEST] button ({btn} px)")

        click_once(*BTN_MID)        # where the button would have been
        time.sleep(1.0)
        with open(logpath, errors="replace") as fh:
            log = fh.read()
        note("[wallet] invoice:" not in log and "[wallet] memo:" not in log,
             "and nothing is asked for on its behalf")
        note(app_alive(p, logpath), "the app is alive on a companion-app merchant")
    finally:
        stop(p, f)



# ---------------------------------------------------------------------------
# spec-nostr-identity: Trust, the badge, and the key actions
#
# Geometry in client coordinates, measured against a live screenshot of the
# built app (frame offsets above; _NET_FRAME_EXTENTS agrees on the 32 px
# top). The panel starts right under the 34 px header; the Contacts and
# Messages frames sit at (6, 32) with 1 px border + 8 px padding, so their
# lists start at x=15, y=75. The Other page's content column is at x=306,
# y=PANEL_Y+38 with the same border+padding, so its rows start at 315/81 -
# IDENTITY heading (16), then Name, Room, nstr (editable, w/ [KEY]+[PASTE]),
# [NEW], [PAIR], [CODE], state (18 each, 2 px flex gaps).
PANEL_Y = 34
LIST_X = 6 + 1 + 8
LIST_Y = PANEL_Y + 32 + 1 + 8
CONTENT_X = 306 + 1 + 8
CONTENT_Y = PANEL_Y + 38 + 1 + 8

def _row_center(top):
    return top + 9

NSTR_TOP = CONTENT_Y + 16 + 2 + 18 + 2 + 18 + 2        # after the heading
NEW_TOP = NSTR_TOP + 18 + 2
# the separate import row is gone (v0.9.2): the nstr row is the editor now
PAIR_TOP = NEW_TOP + 18 + 2

KEY_BTN = (CONTENT_X + 326 + 26, _row_center(NSTR_TOP))    # [KEY], between field and [PASTE]
NEW_BTN = (CONTENT_X + 150 + 26, _row_center(NEW_TOP))      # [NEW], value col
PASTE_FIELD = (CONTENT_X + 150 + 94, _row_center(NSTR_TOP))
PASTE_BTN = (CONTENT_X + 382 + 26, _row_center(NSTR_TOP))
# the [KEY] block opens right under the nstr row: two lines, x = content+5
BLOCK_BOX = (CONTENT_X + 5, CONTENT_X + 140,
             NEW_TOP, NEW_TOP + 34)

BTN_X = LIST_X + 684               # a contact row's button column
# TRUSTED empty + one SEEN row: heading, empty line, heading, then the row
TRUST_BTN = (BTN_X + 38, LIST_Y + (16 + 2) * 3 + 2 + 8)
# one trusted row: heading, then the row - the same band the empty state
# leaves behind, so one pixel box reads both states
REMOVE_ROW = LIST_Y + 16 + 2
REMOVE_BTN = (BTN_X + 38, _row_center(REMOVE_ROW))
TRUSTED_BAND = (BTN_X, BTN_X + 78, REMOVE_ROW, REMOVE_ROW + 24)
# after a REMOVE the device drops straight back under SEEN: heading, empty,
# heading, then the rows (most recent first)
SEEN_ROW = LIST_Y + (16 + 2) * 3
SEEN_BAND = (BTN_X, BTN_X + 78, SEEN_ROW, SEEN_ROW + 46)
# the second SEEN row's [TRUST] (row pitch = 22 + 2): used to trust again
# after a REMOVE - the impostor spoke last, so it is the row above
SEEN_TRUST2_BTN = (BTN_X + 38, SEEN_ROW + 22 + 2 + 10)
# the badge sits in the name column of a message row (row x + 44)
BADGE_X = (LIST_X + 40, LIST_X + 57)
# rows are newest-first and about 20 px tall: the impostor's row is first
# when it is the newest message
ROW0_BAND = (LIST_Y, LIST_Y + 22)
ROW_REST_BAND = (LIST_Y + 22, LIST_Y + 140)


def _read(path):
    try:
        with open(path, errors="replace") as f:
            return f.read()
    except OSError:
        return ""


def _pubs(logpath):
    return re.findall(r"\[id\] pub=([0-9a-f]{64})", _read(logpath))


def test_trust():
    print("16. Trust, the verified badge, and a name spoof", flush=True)
    # Three instances: S is the key we Trust, I is an impostor using the
    # same display name (its name tag is identical, its key is not), D is
    # the device under test - all on default carrier ports, which is how
    # they hear each other on one box.
    d = os.path.join(RUN_TMP, "trust_dev")
    sd = os.path.join(RUN_TMP, "trust_s")
    idd = os.path.join(RUN_TMP, "trust_i")
    for x in (d, sd, idd):
        os.makedirs(x, exist_ok=True)
    prefs = os.path.join(d, "prefs.conf")
    quiet = {"PDA_BIT_SCAN": "off", "PDA_DEBUG": "1"}

    s1, s1f, s1log = start({
        "PDA_PREFS": os.path.join(sd, "prefs.conf"),
        "PDA_NAME": "ROOK", "PDA_START_TAB": "1",
        "PDA_TEST_SEND": "rook was here", "PDA_TEST_SEND_IN": "2",
        **quiet}, "trust_s1")
    time.sleep(1.0)
    dev, devf, devlog = start({
        "PDA_PREFS": prefs, "PDA_START_TAB": "2", **quiet}, "trust_dev")
    try:
        wait_for_log(s1log, r"\[id\] pub=", 10)
        pub_s = _pubs(s1log)[-1]
        data = wait_for_log(devlog, re.escape("sig ok pub=" + pub_s), 15)
        note(pub_s and ("sig ok pub=" + pub_s) in data,
             "the device verified the sender's signature")
        note("recv via" in data and "rook was here" in data,
             "the sender's message is in the device's log")
        time.sleep(1.5)

        click_once(*TRUST_BTN)
        time.sleep(1.5)
        contacts = _read(os.path.join(d, "contacts.conf"))
        note(pub_s in contacts, "TRUST wrote the key into contacts.conf")

        # the badge on the already-drawn row (the staleness rule: a
        # contacts_version change repaints the Messages list)
        click_once(120 + 1 * 80, 22)          # the Messages tab
        time.sleep(1.2)
        g = colour_pixels(shot("trust_badge"), BADGE_X[0], BADGE_X[1],
                          LIST_Y, LIST_Y + 140, 0x7C9E52)
        note(g >= 10, f"the trusted row shows the verified mark ({g} green px)")

        # the impostor: same name, different key
        imp, impf, implog = start({
            "PDA_PREFS": os.path.join(idd, "prefs.conf"),
            "PDA_NAME": "ROOK", "PDA_START_TAB": "1",
            "PDA_TEST_SEND": "i am rook too", "PDA_TEST_SEND_IN": "2",
            **quiet}, "trust_imp")
        wait_for_log(devlog, r"i am rook too", 15)
        pub_i = _pubs(implog)[-1] if _pubs(implog) else ""
        note(pub_i and pub_i != pub_s, "the impostor has its own key")
        target_pid(dev.pid)                  # start() points at the impostor
        time.sleep(1.5)
        shot_path = shot("trust_spoof")
        top = colour_pixels(shot_path, BADGE_X[0], BADGE_X[1],
                            ROW0_BAND[0], ROW0_BAND[1], 0x7C9E52)
        rest = colour_pixels(shot_path, BADGE_X[0], BADGE_X[1],
                             ROW_REST_BAND[0], ROW_REST_BAND[1], 0x7C9E52)
        note(top <= 5, f"the impostor's row (same name) has no mark ({top} px)")
        note(rest >= 10, f"the trusted row below it still does ({rest} px)")
        contacts = _read(os.path.join(d, "contacts.conf"))
        note(pub_s in contacts and pub_i not in contacts,
             "only the key that was Trusted is in contacts.conf")

        # --- REMOVE, in the same session: out of the file, and straight
        #     back under SEEN (a session speaker is never purged). Back to
        #     the Contacts tab first - the badge check left us on Messages,
        #     and a click at row coordinates on the wrong tab is how this
        #     test tricked itself into "REMOVEd but still in the file".
        click_once(120 + 2 * 80, 22)          # the Contacts tab
        time.sleep(1.2)
        click_once(*REMOVE_BTN)
        time.sleep(1.5)
        contacts = _read(os.path.join(d, "contacts.conf"))
        note(pub_s not in contacts, "REMOVE took the key out of contacts.conf")
        shot_path = shot("trust_removed")
        amber = colour_pixels(shot_path, *TRUSTED_BAND, 0xC9A227)
        note(amber <= 5, f"the TRUSTED section is empty again ({amber} px)")
        seen = colour_pixels(shot_path, *SEEN_BAND, 0xC9A227)
        note(seen >= 20, f"the device is back under SEEN, not purged "
                         f"({seen} amber px)")

        # --- and TRUST again (the sender's row: it spoke before the
        #     impostor, so it is the second SEEN row) so the next step can
        #     restart with a trust in place
        click_once(*SEEN_TRUST2_BTN)
        time.sleep(1.5)
        contacts = _read(os.path.join(d, "contacts.conf"))
        note(pub_s in contacts, "TRUST works a second time on the same key")

        stop(imp, impf)
        stop(dev, devf)
        stop(s1, s1f)

        # --- persistence: restart, the key is still there, key file intact
        nsec_before = _read(os.path.join(d, "identity.nsec"))
        dev, devf, devlog = start({
            "PDA_PREFS": prefs, "PDA_START_TAB": "2", **quiet}, "trust_dev2")
        time.sleep(3.0)
        target_pid(dev.pid)
        contacts = _read(os.path.join(d, "contacts.conf"))
        note(pub_s in contacts, "the contact is still in contacts.conf")
        amber = colour_pixels(shot("trust_restart"), *TRUSTED_BAND, 0xC9A227)
        note(amber >= 20, f"the TRUSTED section still shows its row + REMOVE "
                          f"({amber} amber px)")
        note(_read(os.path.join(d, "identity.nsec")) == nsec_before,
             "identity.nsec survived the restart unchanged")

        # --- REMOVE on the restarted instance (its session list is empty
        #     by design - speakers are session-only - so SEEN shows its
        #     honest empty state, and only TRUSTED is checked here)
        click_once(*REMOVE_BTN)
        time.sleep(1.5)
        contacts = _read(os.path.join(d, "contacts.conf"))
        note(pub_s not in contacts, "REMOVE took the key out of contacts.conf")
        shot_path = shot("trust_removed")
        amber = colour_pixels(shot_path, *TRUSTED_BAND, 0xC9A227)
        note(amber <= 5, f"the TRUSTED section is empty again ({amber} px)")
        stop(dev, devf)

        # --- and it stays gone after another restart
        dev, devf, devlog = start({
            "PDA_PREFS": prefs, "PDA_START_TAB": "2", **quiet}, "trust_dev3")
        time.sleep(3.0)
        contacts = _read(os.path.join(d, "contacts.conf"))
        note(pub_s not in contacts, "the contact is gone after a restart too")
        note(_read(os.path.join(d, "identity.nsec")) == nsec_before,
             "identity.nsec is still the same key")
        ok = app_alive(dev, devlog)
        stop(dev, devf)
        note(ok, "the device is alive through all of it")
    finally:
        for proc, fh in ((s1, s1f), (dev, devf)):
            try:
                stop(proc, fh)
            except Exception:
                pass


def test_key_actions():
    print("17. the nstr key: [KEY], [NEW], [PASTE], an unreadable file", flush=True)
    d = os.path.join(RUN_TMP, "keys")
    os.makedirs(d, exist_ok=True)
    prefs = os.path.join(d, "prefs.conf")
    keyfile = os.path.join(d, "identity.nsec")

    def boot(tag):
        return start({"PDA_START_TAB": "6", "PDA_OTHER_PAGE": "0",
                      "PDA_OTHER_SUB": "5", "PDA_DEBUG": "1",
                      "PDA_PREFS": prefs}, tag)

    # --- first run: a key is generated, and [KEY] reveals it only on demand
    p, f, logpath = boot("key1")
    time.sleep(3.0)
    try:
        pub0 = _pubs(logpath)[-1] if _pubs(logpath) else ""
        note(bool(pub0), "the first run generated a key")
        nsec0 = _read(keyfile).strip()

        # the reveal: closed -> open -> closed, and navigation clears it
        before = box_lit(shot("key_closed"), *BLOCK_BOX)
        click_once(*KEY_BTN)
        time.sleep(1.0)
        opened = box_lit(shot("key_open"), *BLOCK_BOX)
        note(opened >= before + 20,
             f"[KEY] reveals the npub + nsec block ({before} -> {opened} px)")
        click_once(*KEY_BTN)
        time.sleep(1.0)
        closed = box_lit(shot("key_closed2"), *BLOCK_BOX)
        note(closed <= opened - 15, f"a second press hides it ({closed} px)")
        # leaving the page and coming back must not bring it back
        click_once(*KEY_BTN)
        time.sleep(0.8)
        click_once(SUB_X, SUB_Y0 + SUB_PITCH * 2)      # Language
        time.sleep(1.0)
        click_once(SUB_X, SUB_Y0 + SUB_PITCH * 5)      # back to User
        time.sleep(1.0)
        after_nav = box_lit(shot("key_after_nav"), *BLOCK_BOX)
        note(after_nav <= closed + 5,
             f"navigation cleared the reveal ({after_nav} px)")

        # --- [NEW]: two presses (arm, then confirm)
        click_once(*NEW_BTN)          # "NEW" -> "SURE?"
        time.sleep(0.5)
        click_once(*NEW_BTN)          # regenerate
        pub1 = []
        end = time.time() + 8
        while time.time() < end:
            pub1 = _pubs(logpath)
            if len(pub1) >= 2 and pub1[-1] != pub1[0]:
                break
            time.sleep(0.4)
        note(len(pub1) >= 2 and pub1[-1] != pub1[0],
             "two presses on [NEW] regenerated the key")
        nsec1 = _read(keyfile).strip()
        note(nsec1 and nsec1 != nsec0, "identity.nsec was rewritten")
    finally:
        stop(p, f)

    # --- the new key survives a restart
    p, f, logpath = boot("key2")
    time.sleep(3.0)
    try:
        pubs = _pubs(logpath)
        note(pubs and pubs[-1] == pub1[-1],
             "the regenerated key is the one that comes back after a restart")
        note(_read(keyfile).strip() == nsec1,
             "identity.nsec is unchanged by that restart")
    finally:
        stop(p, f)

    # --- [PASTE] with a key generated at runtime (never a committed one)
    import secrets
    hexkey = secrets.token_hex(32)
    p, f, logpath = boot("key3")
    time.sleep(3.0)
    try:
        before = _pubs(logpath)[-1]
        click_once(*PASTE_FIELD)
        time.sleep(0.4)
        type_text(hexkey)
        time.sleep(0.5)
        shot("key_pasted")
        click_once(*PASTE_BTN)
        after = []
        end = time.time() + 8
        while time.time() < end:
            after = _pubs(logpath)
            if len(after) >= 2 and after[-1] != before:
                break
            time.sleep(0.4)
        note(len(after) >= 2 and after[-1] != before,
             "the pasted runtime key became the device key")
        nsec2 = _read(keyfile).strip()
        note(nsec2 and nsec2 != nsec1, "identity.nsec now holds the pasted key")

        # a paste that is not a key: nothing saved, no new key
        click_once(*PASTE_FIELD)
        time.sleep(0.4)
        type_text("definitelynotakey")
        time.sleep(0.5)
        click_once(*PASTE_BTN)
        time.sleep(2.0)
        note(len(_pubs(logpath)) == len(after),
             "an invalid paste imports nothing (no new key line)")
        note(_read(keyfile).strip() == nsec2, "an invalid paste saves nothing")
    finally:
        stop(p, f)

    # --- the runtime key also survives a restart (the npub matches)
    p, f, logpath = boot("key4")
    time.sleep(3.0)
    try:
        note(_pubs(logpath)[-1] == after[-1],
             "the imported key is the one that comes back after a restart")
        note(app_alive(p, logpath), "the app is alive on a normal boot")
    finally:
        stop(p, f)

    # --- an unreadable identity.nsec: honest failure, never a rewrite
    with open(keyfile, "w") as fh:
        fh.write("this is not a key at all\n")
    p, f, logpath = boot("key5")
    time.sleep(3.0)
    try:
        data = _read(logpath)
        note(app_alive(p, logpath), "a corrupt identity.nsec does not kill the app")
        note("[id] pub=" not in data,
             "the corrupt file is not silently regenerated (no key line)")
        note("[id] selftest" in data,
             "the sha256 selftest still logs (hashing needs no key)")
        note("system:" in data and "not a valid key" in data,
             "the honest SYSTEM line made it into the log")
        note(_read(keyfile).strip() == "this is not a key at all",
             "the corrupt file was left on disk, untouched")
        # the User page says "not set" instead of a key
        n = lit_pixels(shot("key_notset"), PANEL_Y + 40, PANEL_Y + 240)
        note(n > 0, f"the User page still renders ({n} px)")
    finally:
        stop(p, f)


def test_relay_row():
    print("19. the relay row: the URL the NSTR carrier dials", flush=True)
    port = free_port()
    relay, _ = start_relay(port, "relayrow")
    prefs = os.path.join(LOGDIR, "relayrow_prefs.conf")
    if os.path.exists(prefs):
        os.unlink(prefs)

    # (a) nothing configured: the example is the placeholder, the carrier
    #     reads `no relay`, and the row draws (EN)
    p, f, logpath = start({"PDA_START_TAB": "6", "PDA_OTHER_PAGE": "1",
                           "PDA_OTHER_SUB": "0", "PDA_DEBUG": "1",
                           "PDA_DEBUG_FIELD": "1", "PDA_PREFS": prefs},
                          "relayrow_empty")
    time.sleep(3.0)
    try:
        data = _read(logpath)
        note("placeholder=wss://relay.example.com" in data,
             "empty field: the example URL is the placeholder")
        note("[nstr] reason: no relay" in data,
             "empty field: the carrier reads `no relay`")
        n = lit_pixels(shot("relayrow_en"), DY + 40, DY + 300)
        note(n > 0, f"the Info page with the row renders in English ({n} px)")
        note(app_alive(p, logpath), "the app is alive with no relay URL")
    finally:
        stop(p, f)

    # (b) SAVE a URL: the field shows it and the carrier dials it
    p, f, logpath = start({"PDA_START_TAB": "6", "PDA_OTHER_PAGE": "1",
                           "PDA_OTHER_SUB": "0", "PDA_DEBUG": "1",
                           "PDA_DEBUG_FIELD": "1", "PDA_PREFS": prefs},
                          "relayrow_save")
    try:
        click(SUB_X, SUB_Y0 + SUB_PITCH * 0)      # rebuild: final layout
        time.sleep(1.2)
        box = field_box(logpath, "wss://relay.example.com")
        note(box is not None, "the relay field is on the Info page")
        if box:
            x0, y0, w, h = box
            click(x0 + w // 2, y0 + h // 2)
            time.sleep(0.4)
            type_text(relay_url(port))
            time.sleep(0.4)
            # the SAVE button of the same row: 232 px into the value column
            # (the geometry the name row's SAVE_X was measured on)
            click(x0 + 232 + 26, y0 + 8)
            time.sleep(1.0)
        saved = _read(prefs)
        note(re.search(r"nstr_relay\s*=\s*" + re.escape(relay_url(port)), saved) is not None,
             f"SAVE wrote the URL to the settings file ({saved.strip()!r})")
        data = wait_for_log(logpath, r"\[nstr\] up", 15)
        note("[nstr] up" in data, "the carrier dialed the saved URL and came up")
        click(SUB_X, SUB_Y0 + SUB_PITCH * 0)      # rebuild to re-read the box
        time.sleep(1.2)
        nfield = len(relay_url(port))
        note(re.search(r"\[field\] built:[^\n]*textlen=%d\b" % nfield,
                       _read(logpath)) is not None,
             f"the field shows the saved URL as its text ({nfield} chars)")

        # (c) clearing the field clears the pref (no env var in this run).
        #     Count first: the `no relay` transition lands within the second
        #     after SAVE, and a count taken then would see its own answer.
        before = _read(logpath).count("[nstr] reason: no relay")
        box = field_box(logpath)
        if box:
            x0, y0, w, h = box
            click(x0 + w // 2, y0 + h // 2)
            time.sleep(0.4)
            press_key("BackSpace", 60)
            time.sleep(0.3)
            click(x0 + 232 + 26, y0 + 8)
            time.sleep(1.0)
        saved = _read(prefs)
        note(re.search(r"nstr_relay\s*=\s*$", saved, re.M) is not None,
             f"clearing the field cleared the pref ({saved.strip()!r})")
        time.sleep(3.0)
        after = _read(logpath).count("[nstr] reason: no relay")
        note(after > before,
             "with the pref gone and no env var, the carrier goes back to `no relay`")
        note(app_alive(p, logpath), "the app is alive through save and clear")
    finally:
        stop(p, f)

    # (d) PDA_NSTR_RELAY wins: clearing the field must not disconnect
    p, f, logpath = start({"PDA_START_TAB": "6", "PDA_OTHER_PAGE": "1",
                           "PDA_OTHER_SUB": "0", "PDA_DEBUG": "1",
                           "PDA_DEBUG_FIELD": "1", "PDA_PREFS": prefs,
                           "PDA_NSTR_RELAY": relay_url(port)},
                          "relayrow_env")
    try:
        data = wait_for_log(logpath, r"\[nstr\] up", 15)
        note("[nstr] up" in data, "the env URL is what the carrier dials")
        opens = _read(logpath).count("[nstr] open ")
        click(SUB_X, SUB_Y0 + SUB_PITCH * 0)
        time.sleep(1.2)
        box = field_box(logpath)
        if box:
            x0, y0, w, h = box
            click(x0 + w // 2, y0 + h // 2)
            time.sleep(0.4)
            press_key("BackSpace", 60)
            time.sleep(0.3)
            click(x0 + 232 + 26, y0 + 8)
            time.sleep(3.0)
        data = _read(logpath)
        note(data.count("[nstr] open ") == opens and "[nstr] reason: no relay" not in data,
             "clearing the field with PDA_NSTR_RELAY set does not disconnect (env wins)")
        note(app_alive(p, logpath), "the app is alive with the env override")
    finally:
        stop(p, f)
    stop_relay(relay)

    # (e) the row renders in Polish too (the parity assertion covers the
    #     table; this covers the row actually asking for TR_RELAY)
    with open(prefs, "w") as fh:
        fh.write("lang = pl\n")
    p, f, logpath = start({"PDA_START_TAB": "6", "PDA_OTHER_PAGE": "1",
                           "PDA_OTHER_SUB": "0", "PDA_DEBUG_FIELD": "1",
                           "PDA_PREFS": prefs},
                          "relayrow_pl")
    time.sleep(3.0)
    try:
        n = lit_pixels(shot("relayrow_pl"), DY + 40, DY + 300)
        note(n > 0, f"the Info page with the row renders in Polish ({n} px)")
        note("placeholder=wss://relay.example.com" in _read(logpath),
             "the row is built in Polish too")
        note(app_alive(p, logpath), "the app is alive in Polish")
    finally:
        stop(p, f)
        if os.path.exists(prefs):
            os.unlink(prefs)


def test_room_switch():
    print("20. changing the room while up replaces the subscription", flush=True)
    port = free_port()
    relay, _ = start_relay(port, "roomsw")
    peer = peer_connect(port)
    prefs = os.path.join(LOGDIR, "roomsw_prefs.conf")
    if os.path.exists(prefs):
        os.unlink(prefs)

    p, f, logpath = start({"PDA_START_TAB": "6", "PDA_OTHER_PAGE": "0",
                           "PDA_OTHER_SUB": "5", "PDA_DEBUG": "1",
                           "PDA_DEBUG_FIELD": "1", "PDA_PREFS": prefs,
                           "PDA_NSTR_RELAY": relay_url(port)},
                          "roomsw")
    try:
        data = wait_for_log(logpath, r"\[nstr\] up", 15)
        note("[nstr] up" in data, "the carrier is up in the room it booted in")

        # change the room on the User page (its placeholder is the room)
        click(SUB_X, SUB_Y0 + SUB_PITCH * 5)
        time.sleep(1.2)
        box = field_box(logpath, "SECTOR")
        note(box is not None, "the room field is on the User page")
        if box:
            x0, y0, w, h = box
            click(x0 + w // 2, y0 + h // 2)
            time.sleep(0.4)
            press_key("BackSpace", 40)
            type_text("ALPHA")
            time.sleep(0.3)
            click(x0 + 232 + 26, y0 + 8)
            time.sleep(1.0)
        saved = _read(prefs)
        note(re.search(r"room\s*=\s*ALPHA", saved) is not None,
             f"the new room was saved ({saved.strip()!r})")

        def ev(content, room):
            ts = int(time.time())
            tags = [["room", room], ["t", room], ["name", "ROOK"]]
            ser = json.dumps([0, "", ts, 1, tags, content],
                             separators=(",", ":"), ensure_ascii=False)
            eid = hashlib.sha256(ser.encode("utf-8")).hexdigest()
            return {"id": eid, "pubkey": "", "created_at": ts, "kind": 1,
                    "tags": tags, "content": content}

        peer.send(json.dumps(["EVENT", ev("old room hello", "SECTOR")]))
        time.sleep(1.5)
        peer.send(json.dumps(["EVENT", ev("new room hello", "ALPHA")]))
        data = wait_for_log(logpath, r"recv via NSTR from ROOK: new room hello", 15)
        note("recv via NSTR from ROOK: new room hello" in data,
             "the event tagged with the NEW room's t arrives (only a replacement "
             "REQ makes that possible)")
        note("old room hello" not in data,
             "the event tagged with the OLD room's t does not arrive")
        note(app_alive(p, logpath), "the app is alive across the room switch")
    finally:
        stop(p, f)
        peer.close()
        stop_relay(relay)
        if os.path.exists(prefs):
            os.unlink(prefs)


def test_nstr_failures():
    print("21. relay failure modes: refused, tls, closed, oversize, no reply", flush=True)

    # (a) nobody is listening: refused, and the attempts are few and far
    #     between (the 2 -> 30 s backoff). Count-based: these logs carry no
    #     timestamps and none are invented.
    port = free_port()
    p, f, logpath = start({"PDA_START_TAB": "1", "PDA_DEBUG": "1",
                           "PDA_BIT_SCAN": "off",
                           "PDA_NSTR_RELAY": relay_url(port)}, "nfail_refused")
    time.sleep(14)
    data = _read(logpath)
    note("reason: refused" in data, "a port nobody answers: `refused`")
    n_open = len(re.findall(r"\[nstr\] open ", data))
    note(2 <= n_open <= 6,
         f"the opens are few and spreading, backoff holds ({n_open} in ~14 s)")
    note(app_alive(p, logpath), "the app is alive through the backoff")
    stop(p, f)

    # (b) local TLS against the harness's --tls mode. The certificate is
    #     self-signed with an IP SAN: rejected by the default store (proof
    #     verification is on), accepted under PDA_NSTR_CA (the decided
    #     per-attempt mechanism).
    cert, key = gen_cert()
    if not cert:
        print("    SKIP  local TLS: no openssl CLI on this box", flush=True)
    else:
        tport = free_port()
        trelay, _ = start_relay(tport, "nfail_tls", tls=True, cert=cert, key=key)

        p, f, logpath = start({"PDA_START_TAB": "1", "PDA_DEBUG": "1",
                               "PDA_BIT_SCAN": "off",
                               "PDA_NSTR_RELAY": f"wss://127.0.0.1:{tport}"},
                              "nfail_tls_reject")
        data = wait_for_log(logpath, r"\[nstr\] reason: tls failed", 20)
        note("reason: tls failed" in data,
             "self-signed cert, default CA store: `tls failed` (no bypass)")
        note(app_alive(p, logpath), "the app is alive after the rejected handshake")
        stop(p, f)

        p, f, logpath = start({"PDA_START_TAB": "1", "PDA_DEBUG": "1",
                               "PDA_BIT_SCAN": "off",
                               "PDA_NSTR_RELAY": f"wss://127.0.0.1:{tport}",
                               "PDA_NSTR_CA": cert,
                               "PDA_TEST_SEND": "tls round trip",
                               "PDA_TEST_SEND_IN": "5"},
                              "nfail_tls_ca")
        data = wait_for_log(logpath, r"\[nstr\] up", 20)
        note("[nstr] up" in data, "PDA_NSTR_CA: the same relay connects (IP-form check)")
        tpeer = peer_connect(tport, tls=True, cafile=cert)
        got = peer_event(tpeer, contains="tls round trip", timeout=10)
        note(got is not None, "and a message round-trips over wss://")
        note(app_alive(p, logpath), "the app is alive with the CA bundle loaded")
        stop(p, f)
        tpeer.close()
        stop_relay(trelay)

    # (c) the message layer: a ping is answered silently, a fragment is
    #     reassembled and shown, garbage never crashes, an unknown verb and
    #     a non-string verb are ignored, then CLOSED demotes honestly
    port = free_port()
    relay, rlog_path = start_relay(port, "nfail")
    peer = peer_connect(port)
    p, f, logpath = start({"PDA_START_TAB": "1", "PDA_DEBUG": "1",
                           "PDA_BIT_SCAN": "off",
                           "PDA_NSTR_RELAY": relay_url(port)}, "nfail_msg")
    try:
        data = wait_for_log(logpath, r"\[nstr\] up", 15)
        note("[nstr] up" in data, "the carrier is up for the message cases")

        relay.stdin.write("ping\n")
        relay.stdin.flush()
        time.sleep(1.5)
        with open(rlog_path) as fh:
            rlog = fh.read()
        note("[<-] pong" in rlog, "the app answered the harness's ping with a pong "
                                  "(the app itself is silent by design)")

        # as the relay delivers it: verb, subscription id, event
        frag_msg = json.dumps(["EVENT", "pda", unsigned_event(
            "the whole message at once", "ROOK")])
        relay.stdin.write("fragment " + frag_msg + "\n")
        relay.stdin.flush()
        data = wait_for_log(logpath, r"the whole message at once", 10)
        note("recv via NSTR from ROOK: the whole message at once" in data,
             "a fragmented delivery is reassembled and displayed")

        relay.stdin.write("raw [1, 2, 3]\n")
        relay.stdin.flush()
        relay.stdin.write("raw [\"WAT\", \"x\"]\n")
        relay.stdin.flush()
        relay.stdin.write("raw [\"NOTICE\", \"slow down\"]\n")
        relay.stdin.flush()
        relay.stdin.write("raw this is not json at all\n")
        relay.stdin.flush()
        time.sleep(2.0)
        data = _read(logpath)
        note("[nstr] recv ?" in data, "an array with no string verb is delivered as ?")
        note("[nstr] recv WAT" in data, "an unknown verb is read, logged and ignored")
        note("[nstr] recv NOTICE" in data, "NOTICE is surfaced to the caller, not swallowed")
        note("[nstr] bad message" in data, "garbage is dropped inside the module")
        note(app_alive(p, logpath), "malformed input does not crash or stall the app")

        relay.stdin.write("closed pda policy says no\n")
        relay.stdin.flush()
        data = wait_for_log(logpath, r"\[nstr\] reason: closed", 10)
        note("reason: closed" in data, "CLOSED for sub `pda` demotes to `closed`")

        # the carrier comes back on its own (backoff), then one frame the
        # reassembly buffer cannot hold. Wait for an `up` that comes AFTER
        # this point: the log already holds the first one.
        here = len(_read(logpath))
        data = wait_for_log_from(logpath, r"\[nstr\] up", here, 20)
        note("[nstr] up" in data[here:], "the carrier came back after the CLOSED")
        relay.stdin.write("oversize 8192\n")
        relay.stdin.flush()
        data = wait_for_log(logpath, r"\[nstr\] reason: bad ws", 10)
        note("reason: bad ws" in data, "an oversized frame closes with `bad ws`")
        note(app_alive(p, logpath), "the app is alive after the bad frame")
    finally:
        stop(p, f)
        peer.close()
        stop_relay(relay)

    # (d) a peer that accepts and never answers: the 20 s attempt deadline
    #     is what has to fire (a stall is never a stage-specific reason)
    srv = socket.socket()
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind(("127.0.0.1", 0))
    srv.listen(1)                 # accept never runs: the client's bytes die here
    dead = srv.getsockname()[1]
    p, f, logpath = start({"PDA_START_TAB": "1", "PDA_DEBUG": "1",
                           "PDA_BIT_SCAN": "off",
                           "PDA_NSTR_RELAY": relay_url(dead)}, "nfail_blackhole")
    try:
        data = wait_for_log(logpath, r"\[nstr\] reason: no reply", 30)
        note("reason: no reply" in data,
             "a peer that never answers hits the 20 s deadline: `no reply`")
        note(app_alive(p, logpath), "the app is alive through the stalled attempt")
    finally:
        stop(p, f)
        srv.close()


def test_nstr_queue():
    print("22. outbound queue pressure (the relay stops reading)", flush=True)
    # The harness `pause` stops *reading*, so the kernel buffers fill first:
    # the queue question (spec DECIDE: keep 2 slots or grow to 4) is answered
    # by evidence or recorded as not exercised - never claimed as tested.
    # SEND fires exactly once from PDA_TEST_SEND, so the loop is UI-driven.
    port = free_port()
    relay, _ = start_relay(port, "nqueue")
    peer = peer_connect(port)
    prefs = os.path.join(LOGDIR, "nqueue_prefs.conf")
    if os.path.exists(prefs):
        os.unlink(prefs)
    p, f, logpath = start({"PDA_START_TAB": "1", "PDA_DEBUG": "1",
                           "PDA_DEBUG_FIELD": "1",
                           "PDA_BIT_SCAN": "off", "PDA_PREFS": prefs,
                           "PDA_NSTR_RELAY": relay_url(port)}, "nqueue")
    wait_for_log(logpath, r"\[field\] chat: send", 10)
    ibox = chat_box(logpath, "input")
    sbox = chat_box(logpath, "send")
    note(ibox is not None and sbox is not None,
         f"the compose row's field and SEND button were measured ({ibox}, {sbox})")
    input_xy = (ibox[0] + ibox[2] // 2, ibox[1] + ibox[3] // 2) if ibox else (0, 0)
    send_xy = (sbox[0] + sbox[2] // 2, sbox[1] + sbox[3] // 2) if sbox else (0, 0)
    try:
        data = wait_for_log(logpath, r"\[nstr\] up", 15)
        note("[nstr] up" in data, "the carrier is up before the pressure")
        relay.stdin.write("pause\n")
        relay.stdin.flush()
        refused = False
        for i in range(12):
            click(*input_xy)
            time.sleep(0.3)
            type_text(f"q{i} pressure")
            time.sleep(0.3)
            click(*send_xy)
            time.sleep(0.5)
            data = _read(logpath)
            if "[nstr] send refused" in data or "nstr failed" in data:
                refused = True
                break
        if refused:
            note(True, "a refusal showed up under backpressure "
                       "([nstr] send refused / nstr failed) - see build-approach.md")
        else:
            note(True,
                 f"12 sends with the relay not reading: no refusal - the kernel "
                 f"buffers absorbed them, so the queue itself was NOT exercised "
                 f"(lean 2 kept; recorded honestly in build-approach.md)")
        note(app_alive(p, logpath), "the app is alive through the burst")
        n_sent = len(re.findall(r"\[chat\] sent: q", _read(logpath)))
        note(n_sent >= 10, f"the burst actually drove the UI ({n_sent} sends)")
    finally:
        stop(p, f)
        peer.close()
        stop_relay(relay)
        if os.path.exists(prefs):
            os.unlink(prefs)


def test_live_relay():
    """23. the live check - the slice's "So that": one signed event out to a
    real public relay, one independent subscriber back (tools/wsclient.py,
    default ssl context = real certificate validation on the way in), its id
    recomputed here rather than trusted from the wire. Offline: SKIP."""
    print("23. live check against a public wss:// relay", flush=True)
    # chosen at build: two candidates answered an unauthenticated REQ
    # (relay.damus.io started 503-ing the upgrade, so it is out), and this
    # one takes the suite's single signed event without rate-limiting it
    relay_url_live = os.environ.get("PDA_LIVE_RELAY", "wss://relay.primal.net")
    peer = None
    try:
        peer = WSClient(relay_url_live, timeout=8)
    except Exception as e:                                  # noqa: BLE001
        print(f"    SKIP  no usable relay or no internet: {e}", flush=True)
        return
    room = f"LIVE{int(time.time()) % 100000}"
    peer.send(json.dumps(["REQ", "livecheck", {"kinds": [1], "#t": [room],
                                               "limit": 50}]))
    marker = f"nstr live check {room}"
    p, f, logpath = start({"PDA_START_TAB": "1", "PDA_DEBUG": "1",
                           "PDA_DEBUG_FIELD": "1",
                           "PDA_BIT_SCAN": "off",
                           "PDA_NSTR_RELAY": relay_url_live,
                           "PDA_ROOM": room},
                          "live")
    try:
        data = wait_for_log(logpath, r"\[nstr\] up", 30)
        if "[nstr] up" not in data:
            note(False, f"the app came up on {relay_url_live}")
            return
        # send through the real compose row, once the carrier is up - a
        # PDA_TEST_SEND fires at a fixed moment and would race a TLS
        # handshake over the internet
        wait_for_log(logpath, r"\[field\] chat: send", 10)
        ibox, sbox = chat_box(logpath, "input"), chat_box(logpath, "send")
        click(ibox[0] + ibox[2] // 2, ibox[1] + ibox[3] // 2)
        time.sleep(0.4)
        type_text(marker)
        time.sleep(0.4)
        click(sbox[0] + sbox[2] // 2, sbox[1] + sbox[3] // 2)
        wait_for_log(logpath, r"\[chat\] sent:", 10)
        note("[chat] sent:" in _read(logpath), "the message was sent from the UI")
        wait_for_log(logpath, r"\[nstr\] recv OK", 30)
        note("[nstr] recv OK" in _read(logpath),
             f"the relay answered our publish with OK ({relay_url_live})")

        got = peer_event(peer, contains=marker, timeout=25)
        note(got is not None, "an independent subscriber saw the event")
        if got:
            # slice A's rule: the id is recomputed here, never trusted
            ser = json.dumps([0, got.get("pubkey", ""), got.get("created_at"),
                              got.get("kind", 1), got.get("tags", []),
                              got.get("content", "")],
                             separators=(",", ":"), ensure_ascii=False)
            eid = hashlib.sha256(ser.encode("utf-8")).hexdigest()
            note(eid == got.get("id"),
                 "its id recomputes to exactly what the relay delivered")
            note(got.get("pubkey") and len(got.get("sig", "")) == 128,
                 "it arrived signed (pubkey + 128-hex sig)")
    finally:
        stop(p, f)
        peer.close()


def test_url_forms():
    """the URL parser's table, asserted the only way the suites can: what the
    carrier's pinned debug lines say for each form (spec-nstr-relay section 1
    + the section 5 reason table)."""
    print("24. the relay URL forms the parser accepts and rejects", flush=True)

    # rejected as `bad url`, never dialed: a bare host, userinfo, an
    # uppercase scheme. Each is a per-URL-string verdict: one open, no retry.
    for bad in ("relay.example.com", "ws://user@127.0.0.1:9", "WS://127.0.0.1:9",
                "ws://127.0.0.1:0"):
        p, f, logpath = start({"PDA_START_TAB": "1", "PDA_DEBUG": "1",
                               "PDA_BIT_SCAN": "off",
                               "PDA_NSTR_RELAY": bad}, "url_bad")
        data = wait_for_log(logpath, r"\[nstr\] reason: bad url", 8)
        note("reason: bad url" in data, f"rejected as bad url: {bad!r}")
        note(data.count("[nstr] open ") <= 1,
             "a parse verdict is per URL string: one open, no retry")
        stop(p, f)

    # accepted: a path (passed through verbatim), a non-default port label,
    # and wss:// on a non-default port (the TLS legs dial exactly this)
    port = free_port()
    relay, _ = start_relay(port, "urlform")
    peer = peer_connect(port)
    p, f, logpath = start({"PDA_START_TAB": "1", "PDA_DEBUG": "1",
                           "PDA_BIT_SCAN": "off",
                           "PDA_NSTR_RELAY": f"ws://127.0.0.1:{port}/some/path"},
                          "url_path")
    data = wait_for_log(logpath, r"\[nstr\] up", 15)
    note("[nstr] up" in data, "a path after the port still connects (passed through)")
    note(f"open ws://127.0.0.1:{port}/some/path" in data,
         "the dial used the URL exactly as given")
    stop(p, f)
    peer.close()
    stop_relay(relay)


def test_nwc():
    """25. the NIP-47 wallet backend, no service needed (spec-nip47-wallet
    test plan 1-4 and 7): the paste door, the saved connection, the honest
    not-connected / unreachable faces, the source rules that keep the wallet
    away from transport and away from the device key, and the mock-wording
    gates (plan 10's grep half - the service half lives in nwc_live)."""
    print("25. the NIP-47 wallet backend (no service)", flush=True)

    # a config dir of our own: wallet.nwc and wallet.nwc.state land next to
    # prefs.conf, isolated like every other run (the global PDA_PREFS rule)
    cfg = os.path.join(LOGDIR, "nwc")
    shutil.rmtree(cfg, ignore_errors=True)
    os.makedirs(cfg)
    prefs = os.path.join(cfg, "prefs.conf")
    conn = os.path.join(cfg, "wallet.nwc")
    state = os.path.join(cfg, "wallet.nwc.state")

    # well-formed, relay on a closed port: every face below answers without
    # any wallet service (plan 1 and 3). The wallet key is G's x-only point,
    # the secret is the test's own - neither is key material in the repo's
    # source (they live in this test file, and never in a committed app).
    URI = ("nostr+walletconnect://"
           "79be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798"
           "?relay=ws%3A%2F%2F127.0.0.1%3A9"
           "&secret=000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f")
    SECRET = "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f"
    logs = []

    def boot(extra, name):
        env = {"PDA_PREFS": prefs, "PDA_START_TAB": "5", "PDA_DEBUG": "1",
               "PDA_DEBUG_FIELD": "1", "PDA_BIT_SCAN": "off",
               "PDA_WALLET_BACKEND": "nwc"}
        env.update(extra)
        proc, fh, logpath = start(env, name)
        logs.append(logpath)
        return proc, fh, logpath

    def paste_boxes(logpath):
        data = _read(logpath)
        fb = re.findall(r"\[wallet\] paste field: x=(\d+) y=(\d+) "
                        r"w=(\d+) h=(\d+)", data)
        bb = re.findall(r"\[wallet\] paste button: x=(\d+) y=(\d+) "
                        r"w=(\d+) h=(\d+)", data)
        return (tuple(map(int, fb[-1])) if fb else None,
                tuple(map(int, bb[-1])) if bb else None)

    # ---- no connection: the reason, the field, the button ----
    p, f, logpath = boot({}, "nwc_unset")
    try:
        data = wait_for_log(logpath, r"paste field", 10)
        note("no wallet connection saved - paste one from your wallet app" in data,
             "no connection: the not-set-up reason names what is missing")
        note(lit_pixels(shot("nwc_unset"), y0=69) > 20,
             "the not-set-up screen renders")
        fbox, bbox = paste_boxes(logpath)
        note(fbox is not None and bbox is not None,
             "the [PASTE CONNECTION] field and its button are up")
        if fbox is None or bbox is None:
            stop(p, f)
            return

        # ---- paste a good URI: two presses, one atomic 0600 file ----
        click_once(fbox[0] + fbox[2] // 2, fbox[1] + fbox[3] // 2)
        time.sleep(0.4)
        type_text(URI)
        time.sleep(0.6)
        click_once(bbox[0] + bbox[2] // 2, bbox[1] + bbox[3] // 2)   # arms
        time.sleep(0.5)
        click_once(bbox[0] + bbox[2] // 2, bbox[1] + bbox[3] // 2)   # accepts
        data = wait_for_log(logpath, r"could not reach", 20)

        note(os.path.exists(conn), "a pasted connection is saved")
        if os.path.exists(conn):
            mode = os.stat(conn).st_mode & 0o777
            note(mode == 0o600, "saved 0600 (%o)" % mode)
            note(open(conn).read().strip() == URI,
                 "the saved line is the URI itself, byte for byte")
        note("nwc cipher selftest ok" in data,
             "the cipher self-check ran before any network")
        note("could not reach 127.0.0.1:9 - the wallet will retry" in data,
             "an unreachable relay is the honest error reason")
        note(app_alive(p, logpath), "the app is alive with an unreachable relay")
    finally:
        stop(p, f)

    # ---- the saved connection loads after a restart, and its state file
    #      is not clobbered by a boot that never reached the service ----
    with open(state, "w") as fh:
        fh.write("balance = 42\n")
    p, f, logpath = boot({}, "nwc_reload")
    try:
        data = wait_for_log(logpath, r"could not reach", 20)
        note("could not reach 127.0.0.1:9" in data,
             "the saved connection is loaded after a restart")
        note("nwc cipher selftest ok" in data,
             "and the self-check passes on the reload")
        note(open(state).read() == "balance = 42\n",
             "a boot that never reached the service leaves the state file alone")
        note(app_alive(p, logpath), "the app is alive on the reload")
    finally:
        stop(p, f)

    # ---- a refused paste: the honest reason, and nothing written ----
    os.unlink(conn)
    p, f, logpath = boot({}, "nwc_refuse")
    try:
        wait_for_log(logpath, r"paste field", 10)
        fbox, bbox = paste_boxes(logpath)
        note(fbox is not None and bbox is not None, "the paste field is up again")
        if fbox and bbox:
            click_once(fbox[0] + fbox[2] // 2, fbox[1] + fbox[3] // 2)
            time.sleep(0.4)
            type_text("not a connection at all")
            time.sleep(0.6)
            click_once(bbox[0] + bbox[2] // 2, bbox[1] + bbox[3] // 2)
            time.sleep(0.5)
            click_once(bbox[0] + bbox[2] // 2, bbox[1] + bbox[3] // 2)
            data = wait_for_log(logpath, r"connect refused", 10)
            note("connect refused: not a wallet connection URI "
                 "(expected nostr+walletconnect://)" in data,
                 "a bad URI is refused with the honest reason")
            note(not os.path.exists(conn), "and a refusal writes nothing")
            note(app_alive(p, logpath), "the app is alive after a refused paste")
    finally:
        stop(p, f)

    # ---- a garbage connection file: error, app alive, file not clobbered ----
    with open(conn, "w") as fh:
        fh.write("garbage in a file\n")
    p, f, logpath = boot({}, "nwc_garbage")
    try:
        data = wait_for_log(logpath, r"\[wallet\] error:", 10)
        note("error: not a wallet connection URI "
             "(expected nostr+walletconnect://)" in data,
             "a garbage file reaches the error screen with its reason")
        note(open(conn).read() == "garbage in a file\n",
             "the unreadable file was not clobbered")
        note(app_alive(p, logpath), "the app is alive with a garbage file")
    finally:
        stop(p, f)
        os.unlink(conn)

    # ---- the unknown-backend string names both real backends now ----
    p, f, logpath = boot({"PDA_WALLET_BACKEND": "lightningd"}, "nwc_unknown")
    try:
        data = wait_for_log(logpath, r"unknown wallet backend", 10)
        note("this build has: mock, nwc" in data,
             "the unknown-backend string names mock, nwc")
    finally:
        stop(p, f)

    # ---- source rules: transport stays out, the device key stays out ----
    wsrc = ""
    wdir = os.path.join(ROOT, "src", "wallet")
    for name in sorted(os.listdir(wdir)):
        if name.endswith((".c", ".h")):
            wsrc += _read(os.path.join(wdir, name))
    note(not re.search(r"\bsocket\(|\bconnect\(|AF_INET|SSL_|TLS", wsrc),
         "no transport calls anywhere in src/wallet/ (spec section 4)")
    note("identity.nsec" not in wsrc and "identity_nsec(" not in wsrc,
         "src/wallet/ never touches the device key (slice A's rule)")

    # ---- plan 10's grep: every mock-only word is behind wallet_is_mock() ----
    uisrc = _read(os.path.join(ROOT, "src", "ui", "wallet_ui.c"))
    lines = uisrc.split("\n")
    ungated = []
    for i, ln in enumerate(lines):
        if not re.search(r"tr\(TR_WALLET_MOCK|tr\(TR_WALLET_NET|"
                         r"TR_WALLET_PAID_NOTE", ln):
            continue
        window = "\n".join(lines[max(0, i - 5):i + 1])
        if "wallet_is_mock()" not in window:
            ungated.append(f"{i + 1}: {ln.strip()[:60]}")
    note(not ungated,
         "every mock-only string is still gated on wallet_is_mock() "
         f"({ungated or 'all gated'})")

    # ---- secret hygiene: the URI and its secret reach no log (plan 4) ----
    leaked = []
    for lp in logs:
        d = _read(lp)
        if SECRET in d:
            leaked.append(os.path.basename(lp) + " (secret)")
        if URI in d:
            leaked.append(os.path.basename(lp) + " (uri)")
        if cfg in d:
            leaked.append(os.path.basename(lp) + " (config dir)")
    note(not leaked,
         "the secret, the URI and the config dir appear in no log "
         f"({leaked or 'clean'})")


def test_nwc_live():
    """26. the live check against the user's own wallet service
    (spec-nip47-wallet test plan 5, 6 and 10) - NEEDS .keys. Always
    registered, gated on PDA_NWC_LIVE=1: unset means an explicit skip line,
    set with no credential means a loud failure (a missing credential must
    never read as a green run). The app itself never reads .keys - this
    harness writes the URI into the isolated config dir, exactly where a
    paste would put it."""
    print("26. live NWC against the real connection", flush=True)

    if os.environ.get("PDA_NWC_LIVE") != "1":
        print("    nwc_live SKIPPED - set PDA_NWC_LIVE=1 and provide .keys "
              "to run it", flush=True)
        return

    uri = os.environ.get("PDA_NWC_URI", "").strip()
    if not uri:
        keys = os.path.join(ROOT, ".keys")
        if os.path.exists(keys):
            uri = open(keys).read().strip()
    if not uri:
        FAIL.append("nwc_live: PDA_NWC_LIVE=1 but no credential "
                    "(app/.keys or PDA_NWC_URI)")
        print("  FAIL  nwc_live needs a credential: put the Alby export in "
              "app/.keys or set PDA_NWC_URI", flush=True)
        return

    cfg = os.path.join(LOGDIR, "nwc_live")
    shutil.rmtree(cfg, ignore_errors=True)
    os.makedirs(cfg)
    conn = os.path.join(cfg, "wallet.nwc")
    with open(conn, "w") as fh:
        fh.write(uri + "\n")
    os.chmod(conn, 0o600)
    prefs = os.path.join(cfg, "prefs.conf")

    env = {"PDA_PREFS": prefs, "PDA_START_TAB": "5", "PDA_DEBUG": "1",
           "PDA_DEBUG_FIELD": "1", "PDA_BIT_SCAN": "off",
           "PDA_WALLET_BACKEND": "nwc"}
    p, f, logpath = start(env, "nwc_live")
    try:
        # READY means the first fetch landed (spec section 6)
        data = wait_for_log(logpath, r"\[wallet\] nwc balance: ", 45)
        m = re.search(r"\[wallet\] nwc balance: (-?\d+) sats", data)
        note(m is not None,
             "get_balance answered with a real number"
             + (f" ({m.group(1)} sats - compare it with your wallet app's "
                "own display)" if m else ""))
        note("[wallet] error:" not in data,
             "the wallet came up READY (no error reason)")

        # REQUEST -> a real bolt11, and the QR on screen is exactly it
        time.sleep(3.0)                      # first paint after the fetch
        click_once(142, 390)                 # the REQUEST button
        data = wait_for_log(logpath, r"\[wallet\] invoice: lnbc", 20)
        inv = last_invoice(logpath)
        note(inv is not None and inv.startswith("lnbc"),
             "make_invoice returned a real bolt11")
        path = shot("nwc_live_req")
        qr = decode_qr(path)
        note(qr is not None and inv is not None and qr == inv,
             "the QR on the screen decodes back to exactly that invoice")

        print("    MANUAL: pay this invoice with your phone wallet now "
              "(waiting up to 3 minutes for the service's answer)...",
              flush=True)
        data = wait_for_log(logpath, r"\[wallet\] nwc invoice paid", 180)
        paid = "[wallet] nwc invoice paid" in data
        note(paid,
             "lookup_invoice heard the payment (plan 6's PAID half)")
        if paid:
            rows = [ln for ln in open(os.path.join(cfg, "wallet.nwc.state"))
                    if ln.startswith("history = ")]
            note(any("|in|" in ln and "|paid|" in ln for ln in rows),
                 "the paid row is on disk")
            note(any(ln.startswith("balance = ") for ln in
                     open(os.path.join(cfg, "wallet.nwc.state"))),
                 "and the fetched balance is on disk too")

            # the row must survive a restart (plan 6's resume half)
            stop(p, f)
            p, f, logpath = start(env, "nwc_live2")
            data = wait_for_log(logpath, r"\[wallet\] nwc balance: ", 45)
            rows = [ln for ln in open(os.path.join(cfg, "wallet.nwc.state"))
                    if ln.startswith("history = ")]
            note(any("|in|" in ln and "|paid|" in ln for ln in rows),
                 "the paid row survived the restart")
            note("[wallet] invoice:" not in data,
                 "the settled invoice's line is gone (one-shot rule)")

        print("    MANUAL, not asserted here (spec test plan 6): SEND a real "
              "tiny invoice and check the balance down + the out row; an "
              "untouched invoice's EXPIRED row; URI replace semantics; the "
              "CONFIRM fee note and the SENT face with a real fee; and the "
              "mid-session relay-death face then its retry back to READY.",
              flush=True)
        note(app_alive(p, logpath), "the app is alive on the live connection")
    finally:
        stop(p, f)


def test_docs():
    print("18. i18n parity and the doc claims this build touched", flush=True)
    # every TR_ id in the enum has an EN and a PL entry
    h = _read(os.path.join(ROOT, "src", "ui", "i18n.h"))
    body = re.search(r"typedef enum \{(.*?)\} tr_id_t;", h, re.S)
    ids = set(re.findall(r"\b(TR_[A-Z0-9_]+)\b",
                         body.group(1) if body else "")) - {"TR_COUNT"}
    c = _read(os.path.join(ROOT, "src", "ui", "i18n.c"))
    en = set(re.findall(r"\[(TR_[A-Z0-9_]+)\]",
                        re.search(r"s_en\[TR_COUNT\] = \{(.*?)\n\};", c, re.S)
                        .group(1)))
    pl = set(re.findall(r"\[(TR_[A-Z0-9_]+)\]",
                        re.search(r"s_pl\[TR_COUNT\] = \{(.*?)\n\};", c, re.S)
                        .group(1)))
    note(ids == en == pl,
         f"every TR_ id has an EN and a PL entry ({len(ids)} keys, "
         f"missing EN: {sorted(ids - en)}, missing PL: {sorted(ids - pl)})")

    # the claims this build invalidates (spec-nostr-identity §11)
    docs = [os.path.join(ROOT, "README.md")]
    plan_docs = os.path.join(os.path.dirname(ROOT), "pda-plan-docs")
    for name in ("STATUS.md", "planning.md", "build-approach.md",
                 "wallet_build_plan.md"):
        docs.append(os.path.join(plan_docs, name))
    stale = []
    for path in docs:
        text = _read(path)
        if "74 keys" in text:
            stale.append(f"{os.path.basename(path)}: old key count")
        if "live peers + saved contacts, persisted" in text:
            stale.append(f"{os.path.basename(path)}: old Contacts TODO")
    note(not stale, "the key counts and the Contacts TODO are updated "
                    f"({stale or 'nothing stale'})")

    readme = _read(docs[0])
    note("Contacts and Camera are visible stub tabs" not in readme,
         "the README no longer calls Contacts a stub tab")

    # TR_COUNT is the single authority for the key count: assert README's
    # number against it instead of trusting whoever typed it
    m = re.search(r"(\d+) keys EN", readme)
    note(m is not None and int(m.group(1)) == len(ids),
         f"README says {m.group(1) if m else 'no number'} keys EN and "
         f"TR_COUNT is {len(ids)}")

    src = ""
    for dirpath, _, files in os.walk(os.path.join(ROOT, "src")):
        for name in files:
            if name.endswith((".c", ".h")):
                src += _read(os.path.join(dirpath, name))
    note("No contacts yet." not in src,
         "the dead stub string left the source")

    # no key material in the repo's source (the wallet audit's rule)
    note(not re.search(r'"nsec1', src) and
         not re.search(r'"[0-9a-f]{64}"', src),
         "no nsec literal and no 64-hex constant in src/")


# ---------------------------------------------------------------------------
# spec-nip46-signer: the fake bunker's crypto.
#
# Pure python, stdlib only (the lean of the spec's DECIDE on harness crypto -
# no crypto package is installed on this box, and nothing is vendored for a
# test). Every primitive is gated at test start on a published vector, so it
# cannot silently drift: RFC 8439's ChaCha20 block, NIP-44's own published
# payload vectors, and BIP-340's sign+verify vectors.
#
# Test-only key material lives here, as the wallet's test already does; none
# of it ever enters the app's src/ (the doc-claims grep scans src/ only).

_P = 2 ** 256 - 2 ** 32 - 977
_N = 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141
_GX = 0x79BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798
_GY = 0x483ADA7726A3C4655DA4FBFC0E1108A8FD17B448A68554199C47D08FFB10D4B8


def _inv(a):
    return pow(a, _P - 2, _P)


def _add(p1, p2):
    if p1 is None:
        return p2
    if p2 is None:
        return p1
    x1, y1 = p1
    x2, y2 = p2
    if x1 == x2 and (y1 + y2) % _P == 0:
        return None
    if p1 == p2:
        lam = (3 * x1 * x1) * _inv(2 * y1) % _P
    else:
        lam = (y2 - y1) * _inv(x2 - x1) % _P
    x3 = (lam * lam - x1 - x2) % _P
    return (x3, (lam * (x1 - x3) - y1) % _P)


def _mul(p, k):
    r = None
    while k:
        if k & 1:
            r = _add(r, p)
        p = _add(p, p)
        k >>= 1
    return r


def _lift_x(x):
    if x >= _P:
        return None
    y2 = (pow(x, 3, _P) + 7) % _P
    y = pow(y2, (_P + 1) // 4, _P)
    if y * y % _P != y2:
        return None
    if y & 1:
        y = _P - y
    return (x, y)


def _tagged(tag, body):
    """BIP-340's tagged hash: SHA256(SHA256(tag) || SHA256(tag) || body)"""
    import hashlib
    t = hashlib.sha256(tag).digest()
    return hashlib.sha256(t + t + body).digest()


def pub_hex(sk):
    """the x-only pubkey of a secret key, 64 lowercase hex"""
    p = _mul((_GX, _GY), sk % _N)
    if p is None:
        raise ValueError("zero key")
    return "%064x" % p[0]


def bip340_sign(sk, msg32, aux32):
    """BIP-340 schnorr, exactly the spec's tagged-hash construction, so a
    published vector reproduces byte for byte"""
    import hashlib
    d1 = sk % _N
    p = _mul((_GX, _GY), d1)
    d = d1 if (p[1] & 1) == 0 else _N - d1     # the BIP's own normalization
    pk = p[0].to_bytes(32, "big")              # bytes(P): the raw 32, not hex
    masked = bytes(a ^ b for a, b in
                   zip(d.to_bytes(32, "big"), _tagged(b"BIP0340/aux", aux32)))
    k0 = int.from_bytes(
        _tagged(b"BIP0340/nonce", masked + pk + msg32), "big") % _N
    if k0 == 0:
        raise ValueError("bad nonce")
    r = _mul((_GX, _GY), k0)
    if r[1] & 1:
        k0 = _N - k0
        r = _mul((_GX, _GY), k0)
    rx = r[0].to_bytes(32, "big")
    e = int.from_bytes(
        _tagged(b"BIP0340/challenge", rx + pk + msg32), "big") % _N
    s = (k0 + e * d) % _N
    return rx + s.to_bytes(32, "big")


def bip340_verify(pk_hex, msg32, sig64):
    import hashlib
    try:
        x = int(pk_hex, 16)
        r = int.from_bytes(sig64[:32], "big")
        s = int.from_bytes(sig64[32:], "big")
    except (ValueError, TypeError):
        return False
    if r >= _P or s >= _N:
        return False
    p = _lift_x(x)
    if p is None:
        return False
    e = int.from_bytes(
        _tagged(b"BIP0340/challenge",
                sig64[:32] + x.to_bytes(32, "big") + msg32), "big") % _N
    big = _add(_mul((_GX, _GY), s), _mul(p, _N - e))
    if big is None or big[1] & 1:
        return False
    return big[0] == r


# ---------- ChaCha20 (RFC 8439), counter 0 like NIP-44 asks ----------

def _rotl(v, n):
    return ((v << n) | (v >> (32 - n))) & 0xFFFFFFFF


def _qr(s, a, b, c, d):
    s[a] = (s[a] + s[b]) & 0xFFFFFFFF
    s[d] ^= s[a]
    s[d] = _rotl(s[d], 16)
    s[c] = (s[c] + s[d]) & 0xFFFFFFFF
    s[b] ^= s[c]
    s[b] = _rotl(s[b], 12)
    s[a] = (s[a] + s[b]) & 0xFFFFFFFF
    s[d] ^= s[a]
    s[d] = _rotl(s[d], 8)
    s[c] = (s[c] + s[d]) & 0xFFFFFFFF
    s[b] ^= s[c]
    s[b] = _rotl(s[b], 7)


def _blocks(key32, counter, nonce12):
    """20 rounds over one 64-byte block, starting at `counter`"""
    import struct
    const = struct.unpack("<4I", b"expand 32-byte k")
    kw = struct.unpack("<8I", key32)
    nw = struct.unpack("<3I", nonce12)
    st = list(const) + list(kw) + [counter & 0xFFFFFFFF] + list(nw)
    x = st[:]
    for _ in range(10):
        _qr(x, 0, 4, 8, 12)
        _qr(x, 1, 5, 9, 13)
        _qr(x, 2, 6, 10, 14)
        _qr(x, 3, 7, 11, 15)
        _qr(x, 0, 5, 10, 15)
        _qr(x, 1, 6, 11, 12)
        _qr(x, 2, 7, 8, 13)
        _qr(x, 3, 4, 9, 14)
    return struct.pack("<16I", *((x[i] + st[i]) & 0xFFFFFFFF
                                 for i in range(16)))


def chacha20_xor(key32, nonce12, data, counter=0):
    out = bytearray()
    off = 0
    c = counter
    while off < len(data):
        ks = _blocks(key32, c, nonce12)
        c += 1
        chunk = data[off:off + 64]
        out += bytes(a ^ b for a, b in zip(chunk, ks[:len(chunk)]))
        off += 64
    return bytes(out)


# ---------- NIP-44 v2 ----------

def _hkdf_extract(salt, ikm):
    import hmac
    return hmac.new(salt, ikm, hashlib.sha256).digest()


def _hkdf_expand(prk, info, length):
    out = b""
    t = b""
    i = 1
    while len(out) < length:
        t = _hkdf_extract(prk, t + info + bytes([i]))
        out += t
        i += 1
    return out[:length]


def _padded_len(n):
    if n <= 32:
        return 32
    fl = 0
    while (1 << (fl + 1)) <= (n - 1):
        fl += 1
    np = 1 << (fl + 1)
    chunk = 32 if np <= 256 else np // 8
    return chunk * ((n - 1) // chunk + 1)


def nip44_pad(data):
    if len(data) < 1:
        raise ValueError("empty")
    if len(data) >= 65536:
        raise ValueError("too long")
    prefix = len(data).to_bytes(2, "big")
    return prefix + data + b"\x00" * (_padded_len(len(data)) - len(data))


def nip44_unpad(b):
    if len(b) < 2:
        raise ValueError("short")
    first = int.from_bytes(b[:2], "big")
    if first == 0:
        raise ValueError("extended prefix not used by this build")
    un, off = first, 2
    if un == 0 or len(b) != off + _padded_len(un):
        raise ValueError("bad padding")
    body = b[off:off + un]
    if len(body) != un:
        raise ValueError("bad padding")
    return body


def nip44_conversation_key(sk, peer_hex):
    p = _lift_x(int(peer_hex, 16))
    if p is None:
        raise ValueError("bad peer key")
    shared = _mul(p, sk % _N)
    if shared is None:
        raise ValueError("zero shared point")
    return _hkdf_extract(b"nip44-v2", shared[0].to_bytes(32, "big"))


def nip44_encrypt(text, sk, peer_hex, nonce=None):
    import base64
    conv = nip44_conversation_key(sk, peer_hex)
    n = nonce if nonce is not None else os.urandom(32)
    keys = _hkdf_expand(conv, n, 76)
    chacha_key, chacha_nonce, hmac_key = keys[:32], keys[32:44], keys[44:]
    padded = nip44_pad(text.encode("utf-8"))
    ct = chacha20_xor(chacha_key, chacha_nonce, padded)
    mac = _hmac(hmac_key, n + ct)
    return base64.b64encode(b"\x02" + n + ct + mac).decode("ascii")


def nip44_decrypt(payload, sk, peer_hex):
    import base64
    import hmac
    if len(payload) < 132:
        raise ValueError("short payload")
    raw = base64.b64decode(payload)
    if len(raw) < 99 or raw[0] != 2:
        raise ValueError("bad payload")
    n, ct, mac = raw[1:33], raw[33:-32], raw[-32:]
    conv = nip44_conversation_key(sk, peer_hex)
    keys = _hkdf_expand(conv, n, 76)
    chacha_key, chacha_nonce, hmac_key = keys[:32], keys[32:44], keys[44:]
    if not hmac.compare_digest(_hmac(hmac_key, n + ct), mac):
        raise ValueError("bad mac")
    padded = chacha20_xor(chacha_key, chacha_nonce, ct)
    return nip44_unpad(padded).decode("utf-8")


def _hmac(key, msg):
    import hmac
    return hmac.new(key, msg, hashlib.sha256).digest()


# ---------- NIP-01 event shape ----------

def nip01_escape(s):
    """NIP-01's seven short escapes, \\u00xx for the rest under 0x20"""
    out = []
    for ch in s:
        c = ord(ch)
        if ch == '"':
            out.append('\\"')
        elif ch == "\\":
            out.append("\\\\")
        elif ch == "\b":
            out.append("\\b")
        elif ch == "\f":
            out.append("\\f")
        elif ch == "\n":
            out.append("\\n")
        elif ch == "\r":
            out.append("\\r")
        elif ch == "\t":
            out.append("\\t")
        elif c < 0x20:
            out.append("\\u%04x" % c)
        else:
            out.append(ch)
    return "".join(out)


def event_ser(pub, created_at, kind, tags, content):
    tags_json = json.dumps(tags, separators=(",", ":"), ensure_ascii=False)
    return '[0,"%s",%d,%d,%s,"%s"]' % (
        pub, created_at, kind, tags_json, nip01_escape(content))


def event_id(pub, created_at, kind, tags, content):
    return hashlib.sha256(
        event_ser(pub, created_at, kind, tags, content).encode("utf-8")
    ).hexdigest()


def make_event(sk, kind, tags, content, created_at=None):
    """a signed NIP-01 event authored by sk (x-only, even-Y lift)"""
    pub = pub_hex(sk)
    ts = int(created_at if created_at is not None else time.time())
    eid = event_id(pub, ts, kind, tags, content)
    sig = bip340_sign(sk, bytes.fromhex(eid), bytes(32))
    return {"id": eid, "pubkey": pub, "created_at": ts, "kind": kind,
            "tags": tags, "content": content, "sig": sig.hex()}


def verify_event(ev):
    """id recheck + BIP-340 under the author's own key - what the app's
   envelope_ok() demands before it decrypts anything"""
    try:
        pub, ts, kind = ev["pubkey"], int(ev["created_at"]), int(ev["kind"])
        tags, content = ev["tags"], ev["content"]
        sig = bytes.fromhex(ev["sig"])
    except (KeyError, TypeError, ValueError):
        return False
    if len(sig) != 64:
        return False
    return event_id(pub, ts, kind, tags, content) == ev["id"] and \
        bip340_verify(pub, bytes.fromhex(ev["id"]), sig)


def nip46_kats():
    """The fake bunker's crypto, proven on published vectors before it ever
    touches the app. Returns None when every vector passes, else the reason.

    RFC 8439 A.2 #1 (ChaCha20 at counter 0 - the counter NIP-44 starts
    from), the BIP-340 sign+verify vectors from the BIP's own
    test-vectors.csv, and NIP-44's payloads as published under the NIP's own
    pinned sha256. A failed KAT fails the test - never a silent skip."""
    try:
        # --- ChaCha20: RFC 8439 A.2 test vector #1 ---
        want = bytes.fromhex(
            "76b8e0ada0f13d90405d6ae55386bd28"
            "bdd219b8a08ded1aa836efcc8b770dc7"
            "da41597c5157488d7724e03fb8d84a37"
            "6a43b8f41518a11cc387b669b2ee6586")
        if chacha20_xor(bytes(32), bytes(12), bytes(64)) != want:
            return "ChaCha20 does not match RFC 8439 A.2 #1"

        # --- BIP-340: pubkey, sign and verify against the BIP's vectors ---
        for sk_h, pk_h, aux_h, msg_h, sig_h in _BIP340_VECTORS:
            sk = int(sk_h, 16)
            pk, aux, msg, sig = (pk_h.lower(), bytes.fromhex(aux_h),
                                 bytes.fromhex(msg_h), bytes.fromhex(sig_h))
            if pub_hex(sk) != pk:
                return "BIP-340 pubkey vector " + pk_h[:16]
            if bip340_sign(sk, msg, aux).hex() != sig_h.lower():
                return "BIP-340 sign vector " + pk_h[:16]
            if not bip340_verify(pk, msg, sig):
                return "BIP-340 verify vector " + pk_h[:16]
            flipped = bytes(b ^ 0xFF for b in msg)
            if flipped != msg and bip340_verify(pk, flipped, sig):
                return "BIP-340 verify accepts a wrong message"
        # pub_hex() is now itself proven, so deriving a peer from a vector's
        # secret key below is not circular
        for unp, pd in _NIP44_PAD:
            if _padded_len(unp) != pd:
                return "NIP-44 padding %d != %d" % (unp, pd)
        for sec1, pub2, conv_want in _NIP44_CONV:
            if nip44_conversation_key(int(sec1, 16), pub2) != bytes.fromhex(conv_want):
                return "NIP-44 conversation key " + pub2[:16]
        for sec1, sec2, nonce, pt, payload in _NIP44_ED:
            peer = pub_hex(int(sec2, 16))
            enc = nip44_encrypt(pt, int(sec1, 16), peer,
                                nonce=bytes.fromhex(nonce))
            if enc != payload:
                return "NIP-44 encrypt of " + repr(pt)[:24]
            if nip44_decrypt(payload, int(sec1, 16), peer) != pt:
                return "NIP-44 decrypt of " + repr(pt)[:24]
    except Exception as exc:            # a KAT that throws is a failed KAT
        return "%s: %s" % (type(exc).__name__, exc)
    return None

_BIP340_VECTORS = [
    ("0000000000000000000000000000000000000000000000000000000000000003", "F9308A019258C31049344F85F89D5229B531C845836F99B08601F113BCE036F9", "0000000000000000000000000000000000000000000000000000000000000000", "0000000000000000000000000000000000000000000000000000000000000000", "E907831F80848D1069A5371B402410364BDF1C5F8307B0084C55F1CE2DCA821525F66A4A85EA8B71E482A74F382D2CE5EBEEE8FDB2172F477DF4900D310536C0"),
    ("B7E151628AED2A6ABF7158809CF4F3C762E7160F38B4DA56A784D9045190CFEF", "DFF1D77F2A671C5F36183726DB2341BE58FEAE1DA2DECED843240F7B502BA659", "0000000000000000000000000000000000000000000000000000000000000001", "243F6A8885A308D313198A2E03707344A4093822299F31D0082EFA98EC4E6C89", "6896BD60EEAE296DB48A229FF71DFE071BDE413E6D43F917DC8DCF8C78DE33418906D11AC976ABCCB20B091292BFF4EA897EFCB639EA871CFA95F6DE339E4B0A"),
    ("C90FDAA22168C234C4C6628B80DC1CD129024E088A67CC74020BBEA63B14E5C9", "DD308AFEC5777E13121FA72B9CC1B7CC0139715309B086C960E18FD969774EB8", "C87AA53824B4D7AE2EB035A2B5BBBCCC080E76CDC6D1692C4B0B62D798E6D906", "7E2D58D8B3BCDF1ABADEC7829054F90DDA9805AAB56C77333024B9D0A508B75C", "5831AAEED7B44BB74E5EAB94BA9D4294C49BCF2A60728D8B4C200F50DD313C1BAB745879A5AD954A72C45A91C3A51D3C7ADEA98D82F8481E0E1E03674A6F3FB7"),
    ("0340034003400340034003400340034003400340034003400340034003400340", "778CAA53B4393AC467774D09497A87224BF9FAB6F6E68B23086497324D6FD117", "0000000000000000000000000000000000000000000000000000000000000000", "99999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999", "403B12B0D8555A344175EA7EC746566303321E5DBFA8BE6F091635163ECA79A8585ED3E3170807E7C03B720FC54C7B23897FCBA0E9D0B4A06894CFD249F22367"),
]

_NIP44_CONV = [
    ("315e59ff51cb9209768cf7da80791ddcaae56ac9775eb25b6dee1234bc5d2268", "c2f9d9948dc8c7c38321e4b85c8558872eafa0641cd269db76848a6073e69133", "3dfef0ce2a4d80a25e7a328accf73448ef67096f65f79588e358d9a0eb9013f1"),
    ("a1e37752c9fdc1273be53f68c5f74be7c8905728e8de75800b94262f9497c86e", "03bb7947065dde12ba991ea045132581d0954f042c84e06d8c00066e23c1a800", "4d14f36e81b8452128da64fe6f1eae873baae2f444b02c950b90e43553f2178b"),
    ("98a5902fd67518a0c900f0fb62158f278f94a21d6f9d33d30cd3091195500311", "aae65c15f98e5e677b5050de82e3aba47a6fe49b3dab7863cf35d9478ba9f7d1", "9c00b769d5f54d02bf175b7284a1cbd28b6911b06cda6666b2243561ac96bad7"),
]

_NIP44_PAD = [[16, 32], [32, 32], [33, 64], [65, 96], [250, 256], [515, 640], [65536, 65536]]

_NIP44_ED = [
    ("0000000000000000000000000000000000000000000000000000000000000001", "0000000000000000000000000000000000000000000000000000000000000002", "0000000000000000000000000000000000000000000000000000000000000001", 'a', "AgAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABee0G5VSK0/9YypIObAtDKfYEAjD35uVkHyB0F4DwrcNaCXlCWZKaArsGrY6M9wnuTMxWfp1RTN9Xga8no+kF5Vsb"),
    ("0000000000000000000000000000000000000000000000000000000000000002", "0000000000000000000000000000000000000000000000000000000000000001", "f00000000000000000000000000000f00000000000000000000000000000000f", '🍕🫃', "AvAAAAAAAAAAAAAAAAAAAPAAAAAAAAAAAAAAAAAAAAAPSKSK6is9ngkX2+cSq85Th16oRTISAOfhStnixqZziKMDvB0QQzgFZdjLTPicCJaV8nDITO+QfaQ61+KbWQIOO2Yj"),
    ("d5633530f5bcfebceb5584cfbbf718a30df0751b729dd9a789b9f30c0587d74e", "b74e6a341fb134127272b795a08b59250e5fa45a82a2eb4095e4ce9ed5f5e214", "a3e219242d85465e70adcd640b564b3feff57d2ef8745d5e7a0663b2dccceb54", '🙈 🙉 🙊 0️⃣ 1️⃣ 2️⃣ 3️⃣ 4️⃣ 5️⃣ 6️⃣ 7️⃣ 8️⃣ 9️⃣ 🔟 Powerلُلُصّبُلُلصّبُررً ॣ ॣh ॣ ॣ冗', "AqPiGSQthUZecK3NZAtWSz/v9X0u+HRdXnoGY7LczOtUf05aMF89q1FLwJvaFJYICZoMYgRJHFLwPiOHce7fuAc40kX0wXJvipyBJ9HzCOj7CgtnC1/cmPCHR3s5AIORmroBWglm1LiFMohv1FSPEbaBD51VXxJa4JyWpYhreSOEjn1wd0lMKC9b+osV2N2tpbs+rbpQem2tRen3sWflmCqjkG5VOVwRErCuXuPb5+hYwd8BoZbfCrsiAVLd7YT44dRtKNBx6rkabWfddKSLtreHLDysOhQUVOp/XkE7OzSkWl6sky0Hva6qJJ/V726hMlomvcLHjE41iKmW2CpcZfOedg=="),
]




def drag(x0, y0, x1, y1, steps=12):
    """a mouse drag: how a person with a pointer scrolls a scrollable LVGL
    column (the wheel belongs to the map). Used to bring the flow-2 code
    into view - the page cannot hold a 285 px QR above the fold with the
    doors and the state row as well, which is the QR-placement DECIDE of
    spec-nip46-signer recorded in build-approach.md."""
    from Xlib import display, X
    from Xlib.ext import xtest
    d = display.Display()
    xtest.fake_input(d, X.MotionNotify, x=x0, y=y0)
    d.sync()
    time.sleep(0.15)
    xtest.fake_input(d, X.ButtonPress, 1)
    d.sync()
    time.sleep(0.15)
    for i in range(1, steps + 1):
        x = x0 + (x1 - x0) * i // steps
        y = y0 + (y1 - y0) * i // steps
        xtest.fake_input(d, X.MotionNotify, x=x, y=y)
        d.sync()
        time.sleep(0.05)
    xtest.fake_input(d, X.ButtonRelease, 1)
    d.sync()
    time.sleep(0.4)
    d.close()


# the coordinate channel for this slice: the three [nip46] button-box lines
# printed from the first tick after a build, with slice A's constants as the
# fallback the spec allows
PAIR_TOP = NEW_TOP + 18 + 2
CODE_TOP = PAIR_TOP + 18 + 2
STATE_TOP = CODE_TOP + 18 + 2


def nip46_boxes(logpath):
    data = _read(logpath)
    out = {}
    for name in ("pair button", "code button", "unpair button"):
        ms = re.findall(r"\[nip46\] %s: x=(\d+) y=(\d+) w=(\d+) h=(\d+)"
                        % re.escape(name), data)
        if ms:
            m = ms[-1]
            out[name] = (int(m[0]), int(m[1]), int(m[2]), int(m[3]))
    return out


def nip46_box(logpath, name, fallback):
    b = nip46_boxes(logpath).get(name)
    if b:
        return b
    x, y = fallback
    return (x - 26, y - 8, 52, 16)


def test_nip46():
    """27. the NIP-46 remote signer, no service needed (spec-nip46-signer
    test plan 1): the vector gate, refuse-before-write on every pasted
    reason, the junk-session face, save-before-reach + the withheld send,
    both pairing doors, the identity swap and the B4 refusals, then the
    source rules, the secret hygiene and the literal set."""
    print("27. the NIP-46 remote signer (no service)", flush=True)

    cfg = os.path.join(LOGDIR, "nip46")
    shutil.rmtree(cfg, ignore_errors=True)
    os.makedirs(cfg)
    prefs = os.path.join(cfg, "prefs.conf")
    sess = os.path.join(cfg, "identity.bunker")
    pend = os.path.join(cfg, "identity.bunker.pending")
    nsec = os.path.join(cfg, "identity.nsec")
    logs = []

    def boot(extra, name):
        env = {"PDA_PREFS": prefs, "PDA_START_TAB": "6", "PDA_OTHER_PAGE": "0",
               "PDA_OTHER_SUB": "5", "PDA_DEBUG": "1", "PDA_DEBUG_FIELD": "1",
               "PDA_BIT_SCAN": "off"}
        env.update(extra)
        p, f, lp = start(env, name)
        logs.append(lp)
        return p, f, lp

    def pair_field(logpath):
        """[field] built is printed at build time, and the very first build
        of a page happens before the shell has placed the tab panel - the
        wallet prints its own field box from the tick for exactly that
        reason. The [nip46] button boxes are tick-time, and the field sits
        one row-width left of its button, so that is the coordinate that
        actually takes a click."""
        b = nip46_boxes(logpath).get("pair button")
        if b:
            return (b[0] - 232, b[1], 220, 17)
        return field_box(logpath, "bunker://")

    # ---- the vector gate, before the app is ever touched ----
    kat = nip46_kats()
    note(kat is None,
         "the harness's ChaCha20 / NIP-44 / BIP-340 pass their published "
         "KATs" + ("" if kat is None else " (%s)" % kat))
    if kat:
        return

    # ---- boot 1: the gate, the state row and the three doors ----
    p, f, logpath = boot({}, "nip46_boot")
    try:
        data = wait_for_log(logpath, r"\[nip46\] nip44 selftest ok", 15)
        note("[nip46] nip44 selftest ok" in data,
             "the NIP-44 vector gate ran before anything else")
        note("[nip46] nip44 selftest FAILED" not in data,
             "...and it did not fail")
        note("[nip46] state: off (no signer session)" in data,
             "a clean boot rows `off (no signer session)`")
        time.sleep(1.5)
        fbox = pair_field(logpath)
        note(fbox is not None, "the [PAIR] field is up")
        boxes = nip46_boxes(logpath)
        note(all(k in boxes for k in ("pair button", "code button",
                                      "unpair button")),
             "all three doors printed their boxes (%s)" % sorted(boxes))
        note(app_alive(p, logpath), "the app is alive with no session")
        if fbox:
            unpair = nip46_box(logpath, "unpair button",
                               (CONTENT_X + 382 + 26, _row_center(STATE_TOP)))
            before = box_lit(shot("nip46_off"), unpair[0], unpair[0] + unpair[2],
                             unpair[1], unpair[1] + unpair[3])
            note(before == 0,
                 f"[UNPAIR] is hidden while the state is off ({before} px)")
        else:
            stop(p, f)
            return
    finally:
        if p.poll() is None:
            stop(p, f)

    # ---- refuse before write: one case per pinned paste reason ----
    # (the field's Enter maps 1:1 onto the button, so a couple of these go
    # through Enter and prove that path too)
    KEY64 = "79be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798"
    cases = [
        ("not a bunker token at all", "not a bunker:// token"),
        ("bunker://" + "?" + "relay=ws://x", "missing signer key"),
        ("bunker://" + ("z" * 64) + "?relay=ws://x", "bad signer key"),
        ("bunker://" + KEY64 + "?", "token has no relay"),
        ("bunker://" + KEY64 + "?relay=http%3A%2F%2F127.0.0.1%3A9", "bad relay url"),
        ("bunker://" + KEY64 + "?relay=ws://127.0.0.1:9&secret=" + ("a" * 65),
         "secret too long"),
        ("x" * 513, "token too long"),
    ]
    for i, (tok, want) in enumerate(cases):
        p, f, logpath = boot({}, "nip46_ref%d" % i)
        try:
            # the odd cases go through the field's Enter, which has to map
            # 1:1 onto the button - first Enter arms, second accepts
            data = paste_pair(logpath, tok, want=want, enter=(i % 2 == 1))
            note(("[nip46] refused: %s" % want) in data,
                 "%s -> refused with `%s`" % (want, want))
            note(not os.path.exists(sess),
                 "...and a refused paste wrote nothing")
            note(app_alive(p, logpath), "the app is alive after a refused paste")
        finally:
            stop(p, f)

    # ---- a garbage session file: the honest error face, local key intact ----
    with open(sess, "w") as fh:
        fh.write("garbage in a file\n")
    p, f, logpath = boot({}, "nip46_junk")
    try:
        data = wait_for_log(logpath, r"\[nip46\] state: error", 12)
        note("[nip46] state: error (missing session field)" in data,
             "a junk session rows `error (missing session field)` right after boot")
        note(open(sess).read() == "garbage in a file\n",
             "the unreadable session was not clobbered")
        note(os.path.exists(nsec), "the local identity.nsec still exists")
        pub0 = _pubs(logpath)
        note(bool(pub0), "the local identity still loaded (its [id] pub= line)")

        # [PAIR] over junk with a bad token: refused, junk survives
        data = paste_pair(logpath, "still not a bunker token",
                          want="not a bunker:// token")
        note("[nip46] refused: not a bunker:// token" in data,
             "[PAIR] over junk still refuses honestly")
        note(open(sess).read() == "garbage in a file\n",
             "...and the junk file is untouched (recovery needs a valid save)")

        # [UNPAIR] is visible in error, and it deletes the junk file
        ubox = nip46_box(logpath, "unpair button",
                         (CONTENT_X + 382 + 26, _row_center(STATE_TOP)))
        lit = box_lit(shot("nip46_err"), ubox[0], ubox[0] + ubox[2],
                      ubox[1], ubox[1] + ubox[3])
        note(lit > 0, f"[UNPAIR] is visible in the error state ({lit} px)")
        click_once(ubox[0] + ubox[2] // 2, ubox[1] + ubox[3] // 2)   # arms
        time.sleep(0.5)
        click_once(ubox[0] + ubox[2] // 2, ubox[1] + ubox[3] // 2)   # accepts
        data = wait_for_log(logpath, r"\[nip46\] state: off", 12)
        note(not os.path.exists(sess), "[UNPAIR] deletes a junk session")
        note("[nip46] state: off" in data, "the row is back to off")
        note("[nip46] unpaired - local identity active (" in data,
             "and the local key is named as active again")
        note(app_alive(p, logpath), "the app is alive after unpairing junk")
    finally:
        stop(p, f)

    # ---- a valid token against a dead relay: save-before-reach, then a
    #      send that must be withheld, never unsigned ----
    TOK = ("bunker://" + KEY64 +
           "?relay=ws%3A%2F%2F127.0.0.1%3A9&secret=" + ("1f" * 16))
    p, f, logpath = boot({}, "nip46_dead")
    try:
        note(bool(paste_pair(logpath, TOK)),
             "the [PAIR] field is up for the dead-relay case")
        data = wait_for_log(logpath, r"\[nip46\] state: unreachable", 30)
        note(os.path.exists(sess), "a valid token is saved before the reach")
        if os.path.exists(sess):
            mode = os.stat(sess).st_mode & 0o777
            note(mode == 0o600, "the session file is 0600 (%o)" % mode)
            body = open(sess).read()
            note(body.split("\n", 1)[0].strip() == TOK,
                 "the session's first line is the token, byte for byte")
            note("user_pub=" in body and
                 not re.search(r"user_pub=[0-9a-f]{64}", body),
                 "user_pub= is present and empty - save-before-reach")
        note("[nip46] state: unreachable (" in data,
             "an unreachable signer is the honest state, not a swallow")
        note("[nip46] user pub:" not in data,
             "and no user key was invented on the way there")
        ubox = nip46_box(logpath, "unpair button",
                         (CONTENT_X + 382 + 26, _row_center(STATE_TOP)))
        lit = box_lit(shot("nip46_unreach"), ubox[0], ubox[0] + ubox[2],
                      ubox[1], ubox[1] + ubox[3])
        note(lit > 0, f"[UNPAIR] is visible while unreachable ({lit} px)")

        # a send in this state: withheld with the reason, never unsigned
        click(120 + 1 * 80, 22)                  # the Messages tab
        time.sleep(1.2)
        ibox = chat_box(logpath, "input")
        sbox = chat_box(logpath, "send")
        note(ibox is not None and sbox is not None,
             "the compose row is up for the withheld send")
        if ibox and sbox:
            click_once(ibox[0] + ibox[2] // 2, ibox[1] + ibox[3] // 2)
            time.sleep(0.4)
            type_text("this one never leaves the device")
            time.sleep(0.4)
            click_once(sbox[0] + sbox[2] // 2, sbox[1] + sbox[3] // 2)
            data = wait_for_log(logpath, r"kept locally, not sent \(", 25)
            note("could not build the message - kept locally, not sent (" in data,
                 "the send was withheld with the signer's reason")
            note("kept locally, not sent (" in data and
                 "[chat] sent:" in data, "the message is in the log, on the wire it is not")
        note(app_alive(p, logpath), "the app is alive through the withheld send")
    finally:
        stop(p, f)
    # the save-before-reach session from that case must not gate the flow-2
    # arms below: [CODE] honestly refuses over a valid session with
    # `unpair first`, which is the right behaviour and the wrong step here
    if os.path.exists(sess):
        os.unlink(sess)

    # ---- flow 2: the arm refuses honestly before it writes anything ----
    p, f, logpath = boot({"PDA_NSTR_RELAY": ""}, "nip46_arm_norelay")
    try:
        wait_for_log(logpath, r"\[nip46\] pair button", 15)
        cbox = nip46_box(logpath, "code button",
                         (CONTENT_X + 150 + 26, _row_center(CODE_TOP)))
        click_once(cbox[0] + cbox[2] // 2, cbox[1] + cbox[3] // 2)
        data = wait_for_log(logpath, r"\[nip46\] refused:", 12)
        note("[nip46] refused: no relay configured" in data,
             "[CODE] with no relay refuses with `no relay configured`")
        note(not os.path.exists(pend), "...and nothing was written")
    finally:
        stop(p, f)

    p, f, logpath = boot({"PDA_NSTR_RELAY": "http://x"}, "nip46_arm_badurl")
    try:
        wait_for_log(logpath, r"\[nip46\] pair button", 15)
        cbox = nip46_box(logpath, "code button",
                         (CONTENT_X + 150 + 26, _row_center(CODE_TOP)))
        click_once(cbox[0] + cbox[2] // 2, cbox[1] + cbox[3] // 2)
        data = wait_for_log(logpath, r"\[nip46\] refused:", 12)
        note("[nip46] refused: bad relay url" in data,
             "[CODE] with a non-ws URL refuses with `bad relay url`")
        note(not os.path.exists(pend), "...and nothing was written")
    finally:
        stop(p, f)

    # ---- flow 2 against a live relay: the code, the two cancel paths, and
    #      a fresh secret per arm. Every button press happens before any
    #      drag: the code sits below the fold (285 px of QR plus the doors
    #      and the state row do not both fit), so the drag is the last thing
    #      each boot does and nothing has to be dragged back. ----
    port = free_port()
    relay, _ = start_relay(port, "nip46_arm")
    secrets = []
    try:
        p, f, logpath = boot({"PDA_NSTR_RELAY": relay_url(port)}, "nip46_arm")
        try:
            wait_for_log(logpath, r"\[nip46\] pair button", 15)
            code_fallback = (CONTENT_X + 150 + 26, _row_center(CODE_TOP))
            unpair_fallback = (CONTENT_X + 382 + 26, _row_center(STATE_TOP))

            def press(name, fallback):
                b = nip46_box(logpath, name, fallback)
                click_once(b[0] + b[2] // 2, b[1] + b[3] // 2)

            press("code button", code_fallback)
            wait_for_log(logpath, r"\[nip46\] state: waiting", 15)
            time.sleep(3.0)
            data = _read(logpath)
            note("[nip46] state: waiting (waiting for the signer app)" in data,
                 "the row says `waiting (waiting for the signer app)`")
            note(os.path.exists(pend), "the pending file was written")
            note(not os.path.exists(sess), "and no session exists yet")
            ubox = nip46_box(logpath, "unpair button", unpair_fallback)
            lit = box_lit(shot("nip46_waiting"), ubox[0], ubox[0] + ubox[2],
                          ubox[1], ubox[1] + ubox[3])
            note(lit > 0, f"[UNPAIR] is visible while waiting ({lit} px)")

            # [UNPAIR] while armed: the pending-only branch is a cancel
            press("unpair button", unpair_fallback)
            time.sleep(0.5)
            press("unpair button", unpair_fallback)
            data = wait_for_log(logpath, r"\[nip46\] code canceled", 12)
            note("[nip46] code canceled" in data,
                 "[UNPAIR] on a pending-only arm cancels it")
            note(not os.path.exists(pend), "the pending file is gone")
            note(not os.path.exists(sess), "...and no session was touched")
            note("[nip46] state: off" in _read(logpath), "the row is back to off")

            # [CODE] itself, the single-press cancel while the code shows
            press("code button", code_fallback)
            wait_for_log(logpath, r"\[nip46\] state: waiting", 15)
            time.sleep(2.0)
            press("code button", code_fallback)
            data = wait_for_log(logpath, r"\[nip46\] code canceled", 12)
            note("[nip46] code canceled" in data,
                 "a second press on [CODE] cancels the code")
            note(not os.path.exists(pend), "the pending file is gone from disk")
            note("[nip46] state: off" in _read(logpath),
                 "the row is back to off")
            note(not os.path.exists(sess),
                 "and no identity changed - the session was never written")

            # arm a third time and read the code off the screen
            press("code button", code_fallback)
            wait_for_log(logpath, r"\[nip46\] state: waiting", 15)
            time.sleep(3.0)
            drag(CONTENT_X + 60, 420, CONTENT_X + 60, 160)
            time.sleep(1.0)
            tok = decode_qr(shot("nip46_code"))
            note(tok is not None and tok.startswith("nostrconnect://"),
                 "the QR decodes to a nostrconnect:// token")
            if tok:
                q = tok.split("?", 1)
                params = dict(kv.split("=", 1) for kv in q[1].split("&")
                              if "=" in kv)
                note(len(q[0]) - len("nostrconnect://") == 64,
                     "the token carries our client pubkey (64 hex)")
                note(params.get("relay", "").startswith("ws%3A%2F%2F"),
                     "the relay is percent-encoded the NIP's way")
                note(params.get("perms") == "sign_event%3A1",
                     "and perms=sign_event%3A1 is encoded too")
                note(len(params.get("secret", "")) == 32,
                     "the secret is 32 hex")
                secrets.append(params.get("secret", ""))
            note(app_alive(p, logpath), "the app is alive through the arms")
        finally:
            stop(p, f)

        # boot B: an arm is session-scoped, and re-arming mints a new secret
        p, f, logpath = boot({"PDA_NSTR_RELAY": relay_url(port)}, "nip46_arm2")
        try:
            data = wait_for_log(logpath, r"\[nip46\] state: ", 15)
            note("[nip46] nip44 selftest ok" in data,
                 "the vector gate runs on the second boot too")
            note("[nip46] dropped a leftover pairing code" in data,
                 "a restart drops the leftover pairing code")
            note("state: waiting" not in data,
                 "so the boot log has no `state: waiting` line")
            note(not os.path.exists(pend),
                 "the pending file is gone from disk after the restart")
            note("[nip46] state: off" in data,
                 "and the restart lands on clean unpaired state")
            wait_for_log(logpath, r"\[nip46\] pair button", 15)
            cb = nip46_box(logpath, "code button",
                           (CONTENT_X + 150 + 26, _row_center(CODE_TOP)))
            click_once(cb[0] + cb[2] // 2, cb[1] + cb[3] // 2)
            wait_for_log(logpath, r"\[nip46\] state: waiting", 15)
            time.sleep(3.0)
            drag(CONTENT_X + 60, 420, CONTENT_X + 60, 160)
            time.sleep(1.0)
            tok2 = decode_qr(shot("nip46_code2"))
            note(tok2 is not None and "?" in tok2,
                 "the second boot's arm shows a code too")
            if tok2 and secrets:
                params2 = dict(kv.split("=", 1)
                               for kv in tok2.split("?", 1)[1].split("&")
                               if "=" in kv)
                note(params2.get("secret") and
                     params2.get("secret") != secrets[0],
                     "a second arm issues a fresh secret")
            note(app_alive(p, logpath), "the app is alive on the second arm")
        finally:
            stop(p, f)
    finally:
        stop_relay(relay)

    # ---- the paired identity: a hand-written session with the key cached ----
    SIGNER = "f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f9"
    USER = "79be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798"
    CLIENT = "0000000000000000000000000000000000000000000000000000000000000001"
    SESSION = ("bunker://%s?relay=ws%%3A%%2F%%2F127.0.0.1%%3A9&secret=%s\n"
               "client=%s\nuser_pub=%s\n" % (SIGNER, "ab" * 16, CLIENT, USER))
    with open(sess, "w") as fh:
        fh.write(SESSION)
    os.chmod(sess, 0o600)
    nsec_before = open(nsec, "rb").read() if os.path.exists(nsec) else b""

    p, f, logpath = boot({}, "nip46_paired")
    try:
        data = wait_for_log(logpath, r"\[nip46\] user pub: ", 15)
        note(("[nip46] user pub: %s" % USER) in data,
             "a hand-written session reads its cached user key at boot")
        note("[nip46] state: paired" in _read(logpath),
             "and rows paired, not error")
        local_pub = (_pubs(logpath) or [""])[-1]
        note(bool(local_pub), "the local key still loaded underneath")

        # the [UNPAIR] is visible while paired
        time.sleep(1.5)
        ubox = nip46_box(logpath, "unpair button",
                         (CONTENT_X + 382 + 26, _row_center(STATE_TOP)))
        lit = box_lit(shot("nip46_paired"), ubox[0], ubox[0] + ubox[2],
                      ubox[1], ubox[1] + ubox[3])
        note(lit > 0, f"[UNPAIR] is visible while paired ({lit} px)")

        # B4 (a): the key door refuses while remote, and writes nothing
        click_once(*NEW_BTN)                 # arms
        time.sleep(0.5)
        click_once(*NEW_BTN)                 # refuses
        data = wait_for_log(logpath, r"new key not made: unpair the remote signer first", 12)
        note("new key not made: unpair the remote signer first" in data,
             "[NEW] while paired refuses with the pinned line")
        pfield = PASTE_FIELD
        if pfield:
            click_once(pfield[0], pfield[1])
            time.sleep(0.4)
            type_text("definitely not a key")
            time.sleep(0.4)
            # [PASTE] lives on the nstr row now (v0.9.2)
            click_once(*PASTE_BTN)
            data = wait_for_log(logpath,
                                r"key not imported: unpair the remote signer first", 12)
            note("key not imported: unpair the remote signer first" in data,
                 "[PASTE] while paired refuses with the pinned line")
        else:
            note(False, "the [PASTE] field is on the paired page")
        note(open(nsec, "rb").read() == nsec_before,
             "identity.nsec is byte-identical after both refusals")

        # unpair: the exact local npub comes back and the key file survived
        ubox = nip46_box(logpath, "unpair button",
                         (CONTENT_X + 382 + 26, _row_center(STATE_TOP)))
        click_once(ubox[0] + ubox[2] // 2, ubox[1] + ubox[3] // 2)
        time.sleep(0.5)
        click_once(ubox[0] + ubox[2] // 2, ubox[1] + ubox[3] // 2)
        data = wait_for_log(logpath, r"\[nip46\] unpaired - local identity active", 15)
        note(("[nip46] unpaired - local identity active (%s)" % local_pub) in data,
             "the local key is named active again after [UNPAIR]")
        note(not os.path.exists(sess), "the session file is gone")
        note(open(nsec, "rb").read() == nsec_before,
             "identity.nsec survived the whole pairing untouched")
        after = (_pubs(logpath) or [""])[-1]
        note(after == local_pub,
             "and the npub is the exact one from before pairing")
        note(app_alive(p, logpath), "the app is alive after the identity swap")
    finally:
        stop(p, f)

    # ---- the load vocabulary: bad client / bad user / unreadable / unwritable ----
    load_cases = [
        ("nip46_badclient",
         "bunker://%s?relay=ws%%3A%%2F%%2F127.0.0.1%%3A9&secret=%s\n"
         "client=%s\n" % (SIGNER, "ab" * 16, "z" * 64),
         "bad client key"),
        ("nip46_baduser",
         "bunker://%s?relay=ws%%3A%%2F%%2F127.0.0.1%%3A9&secret=%s\n"
         "client=%s\nuser_pub=%s\n" % (SIGNER, "ab" * 16, CLIENT, "z" * 64),
         "bad user key"),
    ]
    for name, body, want in load_cases:
        with open(sess, "w") as fh:
            fh.write(body)
        p, f, logpath = boot({}, name)
        try:
            data = wait_for_log(logpath, r"\[nip46\] state: error", 12)
            note(("[nip46] state: error (%s)" % want) in data,
                 "a hand-written session rows `error (%s)`" % want)
            note(open(sess).read() == body, "the session file was not clobbered")
        finally:
            stop(p, f)

    # an unreadable session file
    with open(sess, "w") as fh:
        fh.write(SESSION)
    os.chmod(sess, 0o000)
    p, f, logpath = boot({}, "nip46_unreadable")
    try:
        data = wait_for_log(logpath, r"\[nip46\] state: error", 12)
        note("[nip46] state: error (could not read the session file)" in data,
             "an unreadable session rows `could not read the session file`")
    finally:
        stop(p, f)
    os.chmod(sess, 0o600)

    # an unwritable config dir, right before a perfectly valid paste
    with open(sess, "w") as fh:
        fh.write("gone before the paste\n")
    os.chmod(sess, 0o600)
    p, f, logpath = boot({}, "nip46_unwritable")
    try:
        wait_for_log(logpath, r"\[nip46\] pair button", 15)
        os.chmod(cfg, 0o500)                  # read-only: nothing may be written
        data = paste_pair(logpath, TOK,
                          want="could not write the session file")
        note("[nip46] refused: could not write the session file" in data,
             "an unwritable config dir refuses with `could not write "
             "the session file` (a save failure is a refusal too)")
        note(open(sess).read() == "gone before the paste\n",
             "...and the previous file is untouched")
        os.chmod(cfg, 0o700)
        note(app_alive(p, logpath), "the app is alive with a read-only config dir")
    finally:
        try:
            os.chmod(cfg, 0o700)
        except OSError:
            pass
        stop(p, f)
    if os.path.exists(sess):
        os.unlink(sess)

    # ---- the source rules that keep this slice in its own lane ----
    isrc = ""
    for name in sorted(os.listdir(os.path.join(ROOT, "src", "identity"))):
        if name.endswith((".c", ".h")):
            isrc += _read(os.path.join(ROOT, "src", "identity", name))
    note(not re.search(r"\bsocket\(|\bconnect\(|AF_INET|SSL_|TLS", isrc),
         "no transport calls anywhere in src/identity/ (the checkable rule)")
    table_files = []
    for dirpath, _, files in os.walk(os.path.join(ROOT, "src")):
        for name in files:
            if not name.endswith((".c", ".h")):
                continue
            path = os.path.join(dirpath, name)
            if "id_signer_t" in _read(path):
                table_files.append(os.path.relpath(path, ROOT))
    note(table_files == [os.path.join("src", "identity", "identity.c")],
         "id_signer_t is named in identity.c only (%s)" % table_files)

    wsrc = ""
    wdir = os.path.join(ROOT, "src", "wallet")
    for name in sorted(os.listdir(wdir)):
        if name.endswith((".c", ".h")):
            wsrc += _read(os.path.join(wdir, name))
    note("nip46" not in wsrc and "bunker" not in wsrc and
         "nostrconnect" not in wsrc,
         "src/wallet/ never mentions this slice (its QR renderer is called, "
         "its code is not)")

    # LEAVE ROOM 1's checkable rule: the nip44/nip04 *service methods* the
    # NIP-60 slice might one day ask a signer to run stay unwired - no method
    # string and no generic request dispatch for them. (The NIP-44 *cipher*
    # item 1 requires is a different thing and lives in nip46.c as a static.)
    note(not re.search(r'"(nip44|nip04)_(encrypt|decrypt)"', isrc),
         "LEAVE ROOM 1: no nip44/nip04 service-method string in src/identity/")
    note("nip46_request" not in isrc and
         not re.search(r"method\s*==\s*\"(nip44|nip04)_", isrc),
         "LEAVE ROOM 1: no generic request seam wired to those methods")

    # ---- secret hygiene: the token, its secret, the client key and the
    #      config dir reach no log written by this group ----
    client_hex = ""
    if os.path.exists(sess):
        m = re.search(r"^client=([0-9a-f]{64})$", open(sess).read(), re.M)
        client_hex = m.group(1) if m else ""
    if not client_hex and os.path.exists(pend):
        m = re.search(r"^client=([0-9a-f]{64})$", open(pend).read(), re.M)
        client_hex = m.group(1) if m else ""
    leaked = []
    for lp in logs:
        d = _read(lp)
        if TOK in d:
            leaked.append(os.path.basename(lp) + " (token)")
        if "ab" * 16 in d:
            leaked.append(os.path.basename(lp) + " (token secret)")
        if client_hex and client_hex in d:
            leaked.append(os.path.basename(lp) + " (client key)")
        if cfg in d:
            leaked.append(os.path.basename(lp) + " (config dir)")
        if "nostrconnect://" in d:
            leaked.append(os.path.basename(lp) + " (pairing code)")
    note(not leaked,
         "the token, its secret, the pairing code, the client key and the "
         f"config dir appear in no log ({sorted(set(leaked)) or 'clean'})")

    # ---- and the app is still standing after all of it ----
    p, f, logpath = boot({}, "nip46_end")
    try:
        data = wait_for_log(logpath, r"\[nip46\] nip44 selftest ok", 15)
        note("[nip46] nip44 selftest ok" in data, "the vector gate is still green")
        note(app_alive(p, logpath), "the app is alive at the end of the group")
    finally:
        stop(p, f)


# ---------------------------------------------------------------------------
# the fake bunker: a NIP-46 signer with its own key, answering the app over
# the local relay. `mode` is how honest it is - one per rejection path the
# spec's test plan maps. Pure python, KAT-gated above; key material lives
# here, never in the app's src/.

SIGNER_SK = int("1111111111111111111111111111111111111111111111111111111111111111", 16)
USER_SK = int("2222222222222222222222222222222222222222222222222222222222222222", 16)
OTHER_SK = int("3333333333333333333333333333333333333333333333333333333333333333", 16)
WIRE_SECRET = "4f1c" * 8          # the token's secret= for the flow-1 cases


class FakeBunker:
    """A NIP-46 remote signer on the local relay, driven from a thread so the
    app's bounded pump has somebody to answer while the UI thread is inside
    it. Modes:

      ok            honest: connect / get_public_key / sign_event all correct
      impostor      a perfectly valid response from the wrong key lands first
      silent        never answers (budget expiry)
      error         sign_event comes back as an `error`
      gp_bad        get_public_key answers with something that is not 64 hex
      corrupt       a content that passes the envelope but will not decrypt
      connect_wrong connect answers with neither `ack` nor the secret
      sign_wrong_id the signed event's id does not match its own fields
      sign_other_key a different key signs a self-consistent event
      sign_swapped  the right key signs a different message
      sign_bad_sig  the right event with a mangled signature
      okfalse       the relay's own OK false on the published event
      closed        the relay closes our subscription mid-operation
    """

    def __init__(self, port, mode="ok", relay=None):
        self.url = relay_url(port)
        self.mode = mode
        self.relay = relay
        self.signer_pub = pub_hex(SIGNER_SK)
        self.user_pub = pub_hex(USER_SK)
        self.other_pub = pub_hex(OTHER_SK)
        self.requests = []          # (method, params) in arrival order
        self.secrets = []           # the connect secret, if the app sent one
        self._stop = False
        self._w = None
        self._th = threading.Thread(target=self._run, daemon=True)

    def start(self):
        self._th.start()
        time.sleep(1.0)             # the REQ is on the wire before the app pairs

    def stop(self):
        self._stop = True
        self._th.join(timeout=5)
        if self._w:
            try:
                self._w.close()
            except Exception:
                pass

    def _run(self):
        try:
            w = WSClient(self.url, timeout=5)
        except Exception as exc:
            print("    fake bunker: could not reach %s: %s"
                  % (self.url, exc), flush=True)
            return
        self._w = w
        w.send(json.dumps(["REQ", "fake", {"kinds": [24133, 24134]}]))
        while not self._stop:
            msg = w.recv(timeout=0.3)
            if not msg:
                continue
            try:
                arr = json.loads(msg)
            except json.JSONDecodeError:
                continue
            if not isinstance(arr, list) or len(arr) < 3 or arr[0] != "EVENT":
                continue
            if arr[1] != "fake":
                continue
            if isinstance(arr[2], dict):
                try:
                    self._handle(arr[2], w)
                except Exception as exc:      # a broken case must not kill the thread
                    print("    fake bunker: %s: %s"
                          % (type(exc).__name__, exc), flush=True)

    # ---------- answering ----------

    def _respond(self, w, client_pub, rid, result=None, error=None,
                 author_sk=None, encrypt_to=None, content=None):
        author = author_sk if author_sk is not None else SIGNER_SK
        if content is None:
            body = {"id": rid}
            if error is not None:
                body["error"] = error
            if result is not None:
                body["result"] = result
            content = nip44_encrypt(json.dumps(body, separators=(",", ":")),
                                    author, encrypt_to or client_pub)
        ev = make_event(author, 24133, [["p", client_pub]], content)
        w.send(json.dumps(["EVENT", ev]))

    def _signed_event(self, sk, tags, content, tamper=None, created_at=None):
        # the signer echoes the unsigned event's own created_at: sign_event
        # takes {kind, content, tags, created_at} and returns that event
        # signed, so a fresh timestamp here would produce an id the client
        # never asked for
        ev = make_event(sk, 1, tags, content, created_at=created_at)
        if tamper == "id":
            ev["id"] = "f" * 64
        elif tamper == "sig":
            s = bytearray(bytes.fromhex(ev["sig"]))
            s[10] ^= 0xFF
            ev["sig"] = s.hex()
        return ev

    def _handle(self, ev, w):
        if not verify_event(ev):
            print("    fake bunker: dropped an envelope that did not verify "
                  "(id or signature)", flush=True)
            return
        # answers from the signer side (ours or the phone's) are not requests
        if ev["pubkey"] == self.signer_pub:
            return
        client_pub = ev["pubkey"]
        try:
            plain = nip44_decrypt(ev["content"], SIGNER_SK, client_pub)
            payload = json.loads(plain)
        except Exception:
            print("    fake bunker: an envelope would not decrypt", flush=True)
            return
        rid = payload.get("id", "")
        method = payload.get("method", "")
        params = payload.get("params") or []
        self.requests.append((method, params))
        if method == "connect" and len(params) > 1:
            self.secrets.append(params[1])

        if self.mode == "silent":
            return
        if self.mode == "okfalse" and self.relay is not None:
            self.relay.stdin.write("okfalse %s relay hates that event\n" % ev["id"])
            self.relay.stdin.flush()
            return
        if self.mode == "closed" and self.relay is not None:
            self.relay.stdin.write("closed nip46 subscriber gone\n")
            self.relay.stdin.flush()
            return

        if method == "connect":
            if self.mode == "connect_wrong":
                self._respond(w, client_pub, rid, result="nope")
            else:
                self._respond(w, client_pub, rid, result="ack")
            return

        if method == "get_public_key":
            if self.mode == "gp_bad":
                self._respond(w, client_pub, rid, result="not-a-key")
            else:
                self._respond(w, client_pub, rid, result=self.user_pub)
            return

        if method == "switch_relays":
            self._respond(w, client_pub, rid, result="null")
            return

        if method == "sign_event":
            unsigned = params[0] if params else "{}"
            if self.mode == "error":
                self._respond(w, client_pub, rid, error="the signer said no")
                return
            if self.mode == "corrupt":
                junk = base64.b64encode(bytes(range(99))).decode()
                self._respond(w, client_pub, rid, content=junk)
                return
            if self.mode == "impostor":
                # a perfectly valid answer from the wrong key lands first:
                # the app's author check is the guard under test
                self._respond(w, client_pub, rid, result=self.user_pub,
                              author_sk=OTHER_SK)
            try:
                u = json.loads(unsigned)
                tags = u.get("tags") or []
                content = u.get("content", "")
                kind = int(u.get("kind", 1))
            except Exception:
                return
            sk, tamper = USER_SK, None
            if self.mode == "sign_wrong_id":
                tamper = "id"
            elif self.mode == "sign_other_key":
                sk = OTHER_SK
            elif self.mode == "sign_swapped":
                content = "a different message entirely"
            elif self.mode == "sign_bad_sig":
                tamper = "sig"
            signed = self._signed_event(sk, tags, content, tamper,
                                        created_at=u.get("created_at"))
            self._respond(w, client_pub, rid,
                          result=json.dumps(signed, separators=(",", ":")))
            return

        # anything else: an honest signer answers with an error
        self._respond(w, client_pub, rid, error="unknown method")


def wire_boot(url, cfg_name, prefs_name, extra=None):
    """a fresh config dir and the app up on the Other tab's User page,
    pointed at this run's relay. Returns (proc, fh, logpath, prefs, cfg)."""
    cfg = os.path.join(LOGDIR, cfg_name)
    shutil.rmtree(cfg, ignore_errors=True)
    os.makedirs(cfg)
    prefs = os.path.join(cfg, prefs_name)
    env = {"PDA_PREFS": prefs, "PDA_START_TAB": "6", "PDA_OTHER_PAGE": "0",
           "PDA_OTHER_SUB": "5", "PDA_DEBUG": "1", "PDA_DEBUG_FIELD": "1",
           "PDA_BIT_SCAN": "off", "PDA_NSTR_RELAY": url}
    env.update(extra or {})
    p, f, lp = start(env, cfg_name)
    return p, f, lp, prefs, cfg


# A keystroke lost while typing shows up as one of these three: the token
# stopped before the signer key was complete. The product is right to refuse
# it - but that is the harness's input channel, not what the case is testing,
# so those three are retried and anything else is asserted as it stands.
TRUNCATION_REASONS = ("bad signer key", "missing signer key",
                      "not a bunker:// token")


def paste_pair(logpath, token, want=None, attempts=3, enter=False):
    """Drive the real [PAIR] door with an arbitrary bunker:// token: click
    the field, type it, press twice (or press Enter twice, the field's
    LV_EVENT_READY path). Nothing is ever written behind it.

    `want` is the refusal the case expects; None means the paste has to take.
    Returns the log text at the moment the verdict landed, or "" if the door
    never came up."""
    data = ""
    for _ in range(attempts):
        if not wait_for_log(logpath, r"\[nip46\] pair button", 15):
            return ""
        boxes = nip46_boxes(logpath)
        if "pair button" not in boxes:
            return ""
        b = boxes["pair button"]
        fb = (b[0] - 232, b[1], 220, 17)
        click_once(fb[0] + fb[2] // 2, fb[1] + fb[3] // 2)
        time.sleep(0.4)
        type_text(token)
        time.sleep(0.5)
        if enter:
            press_enter()                       # arms
            press_enter()                       # accepts, 1:1 with the click
        else:
            click_once(b[0] + b[2] // 2, b[1] + b[3] // 2)   # arms
            time.sleep(0.4)
            click_once(b[0] + b[2] // 2, b[1] + b[3] // 2)   # accepts
        data = wait_for_log(
            logpath,
            r"\[nip46\] refused:|\[nip46\] state: paired|"
            r"\[nip46\] state: unreachable", 15)
        if want:
            if ("[nip46] refused: " + want) in data:
                return data
        elif re.search(r"\[nip46\] state: (paired|unreachable)", data):
            return data
        truncated = any(("[nip46] refused: " + r) in data
                        for r in TRUNCATION_REASONS)
        if "[nip46] refused:" in data and not truncated:
            return data                      # a real refusal: assert it
        time.sleep(0.6)                      # lost keystroke - type it again
    return data


def wire_pair(logpath, port, secret=WIRE_SECRET):
    """the local-relay flavour: a token naming this run's fake bunker"""
    token = ("bunker://%s?relay=ws%%3A%%2F%%2F127.0.0.1%%3A%d&secret=%s"
             % (pub_hex(SIGNER_SK), port, secret))
    return bool(paste_pair(logpath, token))


def hex_to_npub(hex64):
    """npub for a 64-hex key - the form a signer app shows on screen, so the
    live gate can print exactly what the user should compare against"""
    data = bytes.fromhex(hex64)

    def polymod(values):
        chk = 1
        gen = [0x3b6a57b2, 0x26508e6d, 0x1ea119fa, 0x3d4233dd, 0x2a1462b3]
        for v in values:
            b = chk >> 25
            chk = ((chk & 0x1ffffff) << 5) ^ v
            for i in range(5):
                if (b >> i) & 1:
                    chk ^= gen[i]
        return chk

    charset = "qpzry9x8gf2tvdw0s3jn54khce6mua7l"
    hrp = "npub"
    # bech32: HRP expansion + the data + 6 zero bytes, checksummed
    expand = [ord(c) >> 5 for c in hrp] + [0] + [ord(c) & 31 for c in hrp]
    data5 = []
    bits = 0
    accv = 0
    for byte in data:
        accv = (accv << 8) | byte
        bits += 8
        while bits >= 5:
            bits -= 5
            data5.append((accv >> bits) & 31)
    if bits:
        data5.append((accv << (5 - bits)) & 31)
    polymod_list = expand + data5 + [0] * 6
    mod = polymod(polymod_list) ^ 1
    checksum = [(mod >> 5 * (5 - i)) & 31 for i in range(6)]
    return hrp + "1" + "".join(charset[v] for v in data5 + checksum)


def phone_answers(port, client_pub, secret, author_sk=SIGNER_SK, result=None):
    """the phone side of flow 2: a connect response authored by the signer
    key, p-tagged to the client pubkey in the token, carrying the secret
    (or whatever the caller wants it to carry)."""
    w = WSClient(relay_url(port), timeout=5)
    try:
        w.send(json.dumps(["REQ", "phone", {"kinds": [24133, 24134]}]))
        body = {"id": "discovery",
                "result": secret if result is None else result}
        content = nip44_encrypt(json.dumps(body, separators=(",", ":")),
                                author_sk, client_pub)
        ev = make_event(author_sk, 24133, [["p", client_pub]], content)
        w.send(json.dumps(["EVENT", ev]))
        time.sleep(1.5)
    finally:
        try:
            w.close()
        except Exception:
            pass


def decode_arm(logpath, shot_name):
    """press [CODE], drag the code into view (it is below the fold) and read
    the token back off the screen. Returns the token dict or None."""
    wait_for_log(logpath, r"\[nip46\] pair button", 15)
    b = nip46_boxes(logpath).get("code button")
    if not b:
        return None
    click_once(b[0] + b[2] // 2, b[1] + b[3] // 2)
    if not wait_for_log(logpath, r"\[nip46\] state: waiting", 15):
        return None
    time.sleep(3.0)
    drag(CONTENT_X + 60, 420, CONTENT_X + 60, 160)
    time.sleep(1.0)
    tok = decode_qr(shot(shot_name))
    if not tok or "?" not in tok:
        return None
    head, _, qs = tok.partition("?")
    out = {"raw": tok,
           "client": head[len("nostrconnect://"):],
           "client_pub": head[len("nostrconnect://"):]}
    for kv in qs.split("&"):
        if "=" in kv:
            k, v = kv.split("=", 1)
            out[k] = v
    out["secret"] = unquote(out.get("secret", ""))
    out["relay"] = unquote(out.get("relay", ""))
    return out


def unquote(s):
    """percent-decode the token's values (the app encodes only RFC 3986
    unreserved bytes, so this is a strict inverse for what it writes)"""
    out = []
    i = 0
    while i < len(s):
        if s[i] == "%" and i + 2 < len(s):
            try:
                out.append(chr(int(s[i + 1:i + 3], 16)))
                i += 3
                continue
            except ValueError:
                pass
        out.append(s[i])
        i += 1
    return "".join(out)


def test_nip46_wire():
    """30. the wire round trip against a fake bunker (spec-nip46-signer test
    plan 2): the real [PAIR] door on a real relay, a second app instance
    verifying the signed message, every rejection path mapped to its pinned
    reason, the impostor answer, both flow-2 wire cases and the junk-session
    case on the wire.

    Each case gets its own relay on its own port. The relay stores every
    event it has ever seen and replays them to a new subscription, and the
    fake bunker answers in pure python (~0.6 s an event) - so a shared relay
    would hand each new fake the whole previous run's backlog to chew
    through before it ever saw this case's request."""
    print("30. the NIP-46 wire round trip (fake bunker)", flush=True)
    # this group runs two app windows: clicks must land on the one under
    # test, and focus_window() follows the module's APP_PID
    global APP_PID

    kat = nip46_kats()
    note(kat is None,
         "the harness's crypto passes its published KATs before the app is "
         "touched" + ("" if kat is None else " (%s)" % kat))
    if kat:
        return

    logs = []

    def case_up(mode, name):
        """a relay of its own, the fake listening on it, and an observer"""
        port = free_port()
        relay, rlog = start_relay(port, name + "_r")
        fake = FakeBunker(port, mode, relay=relay)
        fake.start()
        obs = peer_connect(port, sub="obs")
        return port, relay, fake, obs

    def case_down(relay, fake, obs):
        if obs:
            try:
                obs.close()
            except Exception:
                pass
        if fake:
            fake.stop()
        stop_relay(relay)

    def send_one(logpath, text):
        """one message through the real compose row"""
        if not wait_for_log(logpath, r"\[nstr\] up", 30):
            return False            # a carrier that is not up publishes nothing
        click(120 + 1 * 80, 22)                 # the Messages tab
        if not wait_for_log(logpath, r"\[field\] chat: input", 15):
            return False
        ibox = chat_box(logpath, "input")
        sbox = chat_box(logpath, "send")
        if not ibox or not sbox:
            return False
        click_once(ibox[0] + ibox[2] // 2, ibox[1] + ibox[3] // 2)
        time.sleep(0.4)
        type_text(text)
        time.sleep(0.4)
        click_once(sbox[0] + sbox[2] // 2, sbox[1] + sbox[3] // 2)
        time.sleep(1.0)
        return True

    # ---- the happy path: pair, sign, and a second instance verifying ----
    port, relay, fake, obs = case_up("ok", "nip46w_ok")
    p = pb = pbfh = None
    try:
        p, f, lp, prefs, cfg = wire_boot(relay_url(port), "nip46w_ok", "ok.conf")
        logs.append(lp)
        if not wire_pair(lp, port):
            note(False, "the [PAIR] door came up")
            stop(p, f); case_down(relay, fake, obs); return
        data = wait_for_log(
            lp, r"\[nip46\] state: unreachable|\[nip46\] user pub: ", 40)
        note(("[nip46] user pub: %s" % fake.user_pub) in data,
             "pairing completes and the signer's user key becomes ours")
        note("[nip46] state: paired (connected)" in _read(lp),
             "the state row says paired/connected once it answered")
        note(fake.requests and fake.requests[0][0] == "connect",
             "the first thing on the wire is the NIP-46 connect exchange")
        note("get_public_key" in [m for m, _ in fake.requests],
             "and get_public_key follows it (the NIP's MUST)")

        prefs_b = os.path.join(LOGDIR, "nip46w_b.conf")
        if os.path.exists(prefs_b):
            os.unlink(prefs_b)
        pb, pbfh, blog = start(
            {"PDA_PREFS": prefs_b, "PDA_START_TAB": "1",
             "PDA_NSTR_RELAY": relay_url(port), "PDA_DEBUG": "1",
             "PDA_BIT_SCAN": "off"}, "nip46w_b")
        logs.append(blog)
        wait_for_log(blog, r"\[nstr\] up", 25)
        APP_PID = p.pid          # every click from here on is instance A's

        note(send_one(lp, "the signer signed this one"),
             "the compose row took the message")
        ev = peer_event(obs, contains="the signer signed this one", timeout=25)
        note(ev is not None, "the peer on the relay received the event")
        if ev:
            note(event_id(ev["pubkey"], ev["created_at"], ev["kind"],
                          ev["tags"], ev["content"]) == ev["id"],
                 "its id recomputes to exactly what the relay delivered")
            note(ev["pubkey"] == fake.user_pub,
                 "it carries the signer's user key, not the device's")
            note(len(ev["sig"]) == 128 and
                 bip340_verify(ev["pubkey"], bytes.fromhex(ev["id"]),
                               bytes.fromhex(ev["sig"])),
                 "the signature verifies under that key")
            data2 = wait_for_log(blog,
                                 re.escape("sig ok pub=" + fake.user_pub), 30)
            note(("sig ok pub=" + fake.user_pub) in data2,
                 "a second app instance verified it with libsecp256k1")
        note(app_alive(p, lp), "the app is alive through the happy path")
    finally:
        if p is not None:
            stop(p, f)
        if pb is not None:
            stop(pb, pbfh)
        case_down(relay, fake, obs)

    # ---- an impostor answer lands first and is ignored, then unpair ----
    port, relay, fake, obs = case_up("impostor", "nip46w_imp")
    p = None
    try:
        p, f, lp, prefs, cfg = wire_boot(relay_url(port), "nip46w_imp",
                                         "imp.conf")
        logs.append(lp)
        if wire_pair(lp, port):
            data = wait_for_log(
                lp, r"\[nip46\] state: unreachable|\[nip46\] user pub: ", 40)
            note(("[nip46] user pub: %s" % fake.user_pub) in data,
                 "a perfectly valid answer from the wrong key is ignored, then "
                 "the real signer's answer is accepted")
            local_pub = (_pubs(lp) or [""])[-1]
            ub = nip46_boxes(lp).get("unpair button")
            if ub:
                click_once(ub[0] + ub[2] // 2, ub[1] + ub[3] // 2)
                time.sleep(0.5)
                click_once(ub[0] + ub[2] // 2, ub[1] + ub[3] // 2)
                data = wait_for_log(
                    lp, r"\[nip46\] unpaired - local identity active", 15)
                note(("[nip46] unpaired - local identity active (%s)"
                      % local_pub) in data,
                     "unpairing brings the local npub back")
                note(not os.path.exists(os.path.join(cfg, "identity.bunker")),
                     "and the client session file is gone")
        else:
            note(False, "the [PAIR] door came up for the impostor case")
        note(app_alive(p, lp), "the app is alive after the impostor")
    finally:
        if p is not None:
            stop(p, f)
        case_down(relay, fake, obs)

    # ---- every rejection path, mapped one to a pinned reason ----
    for mode, name, want, pair_ok in [
        ("silent", "w_silent", "signer did not answer", False),
        ("connect_wrong", "w_conn", "signer rejected the session", False),
        ("gp_bad", "w_gpbad", "signer did not give a usable key", False),
        ("okfalse", "w_okfalse",
         "relay refused: relay hates that event", False),
        ("closed", "w_closed", "subscription closed: subscriber gone", False),
        ("corrupt", "w_corrupt",
         "signer sent something this build cannot read", True),
        ("error", "w_error", "the signer said no", True),
        ("sign_wrong_id", "w_id", "signer answered with a broken id", True),
        ("sign_other_key", "w_key",
         "signer key changed - unpair and pair again", True),
        ("sign_swapped", "w_msg",
         "signer answered for a different message", True),
        ("sign_bad_sig", "w_sig", "signer answer failed verification", True),
    ]:
        port, relay, fake, obs = case_up(mode, name)
        p = None
        try:
            p, f, lp, prefs, cfg = wire_boot(relay_url(port), name,
                                             name + ".conf")
            logs.append(lp)
            if not wire_pair(lp, port):
                note(False, "[%s] the [PAIR] door came up" % name)
                continue
            # `ready (not tried)` is the save, not the outcome: wait for one
            data = wait_for_log(
                lp, r"\[nip46\] state: unreachable|\[nip46\] user pub: ", 40)
            if pair_ok:
                note(("[nip46] user pub: %s" % fake.user_pub) in data,
                     "[%s] pairing still completed" % name)
            else:
                note(("[nip46] state: unreachable (%s)" % want) in data,
                     "[%s] rows `unreachable (%s)`" % (name, want))
            note(send_one(lp, "held back by the signer"),
                 "[%s] the compose row took a message" % name)
            data = wait_for_log(lp, r"kept locally, not sent", 40)
            note(("kept locally, not sent (%s)" % want) in data,
                 "[%s] the send is withheld with `%s`" % (name, want))
            note(app_alive(p, lp), "[%s] the app is alive through it" % name)
            note(peer_event(obs, timeout=3) is None,
                 "[%s] nothing reached the relay as a chat message" % name)
        finally:
            if p is not None:
                stop(p, f)
            case_down(relay, fake, obs)

    # ---- a junk session on the wire: the local key still signs ----
    port = free_port()
    relay, rlog = start_relay(port, "nip46w_junk_r")
    obs = peer_connect(port, sub="obs")
    cfg = os.path.join(LOGDIR, "nip46w_junk")
    shutil.rmtree(cfg, ignore_errors=True)
    os.makedirs(cfg)
    with open(os.path.join(cfg, "identity.bunker"), "w") as fh:
        fh.write("garbage in a file\n")
    p = None
    try:
        p, f, lp = start({"PDA_PREFS": os.path.join(cfg, "junk.conf"),
                          "PDA_START_TAB": "6", "PDA_OTHER_PAGE": "0",
                          "PDA_OTHER_SUB": "5", "PDA_DEBUG": "1",
                          "PDA_DEBUG_FIELD": "1", "PDA_BIT_SCAN": "off",
                          "PDA_NSTR_RELAY": relay_url(port)}, "nip46w_junk")
        logs.append(lp)
        data = wait_for_log(lp, r"\[nip46\] state: error", 15)
        note("[nip46] state: error" in data,
             "a junk session is an error, not a remote identity")
        local_pub = (_pubs(lp) or [""])[-1]
        note(bool(local_pub), "the local key loaded underneath it")
        note(send_one(lp, "signed with the local key"), "a message went out")
        ev = peer_event(obs, contains="signed with the local key", timeout=25)
        note(ev is not None,
             "the peer received it - a junk session never withholds")
        if ev:
            note(ev["pubkey"] == local_pub,
                 "and it is signed with the local key, not a stranger's")
            note(event_id(ev["pubkey"], ev["created_at"], ev["kind"],
                          ev["tags"], ev["content"]) == ev["id"],
                 "with an id that recomputes")
        note(app_alive(p, lp), "the app is alive through the junk case")
    finally:
        if p is not None:
            stop(p, f)
        try:
            obs.close()
        except Exception:
            pass
        stop_relay(relay)

    # ---- flow 2 on the wire: the phone arrives while another tab is up ----
    port, relay, fake, obs = case_up("ok", "nip46w_flow2")
    p = None
    try:
        p, f, lp, prefs, cfg = wire_boot(relay_url(port), "nip46w_flow2",
                                         "flow2.conf")
        logs.append(lp)
        tok = decode_arm(lp, "nip46w_code")
        note(tok is not None, "the code came off the screen")
        if tok:
            note(not os.path.exists(os.path.join(cfg, "identity.bunker")),
                 "arming writes no session")
            click(120 + 1 * 80, 22)             # leave the User page
            time.sleep(1.0)
            phone_answers(port, tok["client_pub"], tok["secret"])
            data = wait_for_log(lp, r"\[nip46\] user pub: ", 45)
            note(("[nip46] user pub: %s" % fake.user_pub) in data,
                 "the phone answered off-page and pairing completed")
            note(os.path.exists(os.path.join(cfg, "identity.bunker")),
                 "the pending code was promoted to a real session")
            note(not os.path.exists(os.path.join(cfg,
                                                 "identity.bunker.pending")),
                 "and the pending file is gone")
        note(app_alive(p, lp), "the app is alive through flow 2")
    finally:
        if p is not None:
            stop(p, f)
        case_down(relay, fake, obs)

    # ---- flow 2 with the wrong secret: nothing is adopted ----
    port = free_port()
    relay, rlog = start_relay(port, "nip46w_bad_r")
    p = None
    try:
        p, f, lp, prefs, cfg = wire_boot(relay_url(port),
                                         "nip46w_flow2bad", "flow2bad.conf")
        logs.append(lp)
        tok = decode_arm(lp, "nip46w_code_bad")
        note(tok is not None, "the second code came off the screen")
        if tok:
            phone_answers(port, tok["client_pub"], "not-the-secret")
            time.sleep(4.0)
            note("[nip46] state: waiting" in _read(lp),
                 "a response with the wrong secret leaves the wait waiting")
            note(not os.path.exists(os.path.join(cfg, "identity.bunker")),
                 "and no session was adopted from it")
            note(app_alive(p, lp), "the app is alive after the wrong secret")
    finally:
        if p is not None:
            stop(p, f)
        stop_relay(relay)

    # ---- the wire group's own secret hygiene ----
    leaked = []
    for lp_ in logs:
        d = _read(lp_)
        if WIRE_SECRET in d:
            leaked.append(os.path.basename(lp_) + " (token secret)")
        if "%064x" % SIGNER_SK in d:
            leaked.append(os.path.basename(lp_) + " (signer key)")
        if "nostrconnect://" in d:
            leaked.append(os.path.basename(lp_) + " (pairing code)")
    note(not leaked,
         "no token secret, signer key or pairing code in any wire log "
         f"({sorted(set(leaked)) or 'clean'})")



def test_nip46_live():
    """31. the live check against the user's own signer app
    (spec-nip46-signer test plan 3) - NEEDS .keys. Always registered, gated
    on PDA_NIP46_LIVE with explicit value semantics: unset skips both
    phases, =1 is flow 1 and needs a credential, =2 is flow 2's manual scan
    and needs no credential. A *requested* run that is missing an input
    fails loudly - a missing credential must never read as green."""
    print("31. live NIP-46 against the real signer app", flush=True)

    mode = os.environ.get("PDA_NIP46_LIVE")
    if mode is None:
        print("    nip46_live SKIPPED - set PDA_NIP46_LIVE=1 (flow 1, needs "
              "app/.keys.nip46) or PDA_NIP46_LIVE=2 (flow 2, manual scan)",
              flush=True)
        return

    # check order under any set value: the relay first (both phases need it),
    # then the credential (flow 1 only) - either missing is a loud FAIL
    relay = (os.environ.get("PDA_NSTR_RELAY") or "").strip()
    if not relay:
        FAIL.append("nip46_live: PDA_NIP46_LIVE=%s but PDA_NSTR_RELAY unset"
                    % mode)
        print("  FAIL  nip46_live needs PDA_NSTR_RELAY: the public wss:// "
              "relay the second instance shares with the app", flush=True)
        return

    if mode not in ("1", "2"):
        FAIL.append("nip46_live: PDA_NIP46_LIVE=%s is not 1 or 2" % mode)
        print("  FAIL  nip46_live: PDA_NIP46_LIVE must be 1 (flow 1) or 2 "
              "(flow 2); got %r" % mode, flush=True)
        return

    token = ""
    if mode == "1":
        token = (os.environ.get("PDA_NIP46_BUNKER") or "").strip()
        if not token:
            path = os.path.join(ROOT, ".keys.nip46")
            if os.path.exists(path):
                token = open(path).read().strip()
        if not token:
            FAIL.append("nip46_live: PDA_NIP46_LIVE=1 but no credential "
                        "(app/.keys.nip46 or PDA_NIP46_BUNKER)")
            print("  FAIL  nip46_live needs a credential: put the bunker:// "
                  "token in app/.keys.nip46 or set PDA_NIP46_BUNKER",
                  flush=True)
            return
        print("    flow 1 only (PDA_NIP46_LIVE=1); the credential is read "
              "by this harness alone - the app never sees it", flush=True)
    else:
        print("    flow 2 only (PDA_NIP46_LIVE=2); no credential expected",
              flush=True)

    cfg = os.path.join(LOGDIR, "nip46_live")
    shutil.rmtree(cfg, ignore_errors=True)
    os.makedirs(cfg)
    prefs = os.path.join(cfg, "prefs.conf")
    nsec = os.path.join(cfg, "identity.nsec")
    env = {"PDA_PREFS": prefs, "PDA_START_TAB": "6", "PDA_OTHER_PAGE": "0",
           "PDA_OTHER_SUB": "5", "PDA_DEBUG": "1", "PDA_DEBUG_FIELD": "1",
           "PDA_BIT_SCAN": "off", "PDA_NSTR_RELAY": relay}
    p = f = pb = pbfh = None
    try:
        p, f, logpath = start(env, "nip46_live")
        nsec_before = open(nsec, "rb").read() if os.path.exists(nsec) else b""

        if mode == "1":
            note(bool(paste_pair(logpath, token)),
                 "the real token went through the real [PAIR] door")
        else:
            tok = decode_arm(logpath, "nip46_live_code")
            note(tok is not None, "the code rendered and read back off screen")
            print("    MANUAL: scan the QR with your signer app now "
                  "(waiting up to 2 minutes for the scan + approval)...",
                  flush=True)

        wait = 130 if mode == "2" else 60
        data = wait_for_log(logpath, r"\[nip46\] user pub: ", wait)
        got = re.search(r"\[nip46\] user pub: ([0-9a-f]{64})", data)
        note(got is not None, "the signer answered with a user pubkey")
        if got:
            npub = hex_to_npub(got.group(1))
            print("    MANUAL: compare this with what your signer app "
                  "displays: %s / %s" % (got.group(1), npub), flush=True)
        note("[nip46] state: paired (connected)" in _read(logpath),
             "the state row says paired/connected")
        print("    MANUAL: if the signer asked for approval, note what its "
              "connect and sign_event screens looked like (this feeds the "
              "perms / auth_url DECIDEs)", flush=True)

        # a message, signed by the phone, verified by a second instance
        prefs_b = os.path.join(LOGDIR, "nip46_live_b.conf")
        if os.path.exists(prefs_b):
            os.unlink(prefs_b)
        pb, pbfh, blog = start(
            {"PDA_PREFS": prefs_b, "PDA_START_TAB": "1",
             "PDA_NSTR_RELAY": relay, "PDA_DEBUG": "1",
             "PDA_BIT_SCAN": "off"}, "nip46_live_b")
        wait_for_log(blog, r"\[nstr\] up", 30)

        note(click(120 + 1 * 80, 22) is None and
             wait_for_log(logpath, r"\[field\] chat: input", 15),
             "the compose row is up")
        ibox, sbox = chat_box(logpath, "input"), chat_box(logpath, "send")
        if ibox and sbox and got:
            click_once(ibox[0] + ibox[2] // 2, ibox[1] + ibox[3] // 2)
            time.sleep(0.4)
            type_text("signed by the phone, verified here")
            time.sleep(0.4)
            click_once(sbox[0] + sbox[2] // 2, sbox[1] + sbox[3] // 2)
            data2 = wait_for_log(
                blog, re.escape("sig ok pub=" + got.group(1)), 45)
            note(("sig ok pub=" + got.group(1)) in data2,
                 "a second app instance verified the phone's signature")
            note("kept locally, not sent (" not in _read(logpath),
                 "and the message was not withheld")

        # unpair: the local identity comes straight back
        wait_for_log(logpath, r"\[nip46\] pair button", 15)
        ub = nip46_boxes(logpath).get("unpair button")
        if ub:
            click_once(ub[0] + ub[2] // 2, ub[1] + ub[3] // 2)
            time.sleep(0.5)
            click_once(ub[0] + ub[2] // 2, ub[1] + ub[3] // 2)
            data = wait_for_log(
                logpath, r"\[nip46\] unpaired - local identity active", 20)
            local = (_pubs(logpath) or [""])[-1]
            note(("[nip46] unpaired - local identity active (%s)" % local)
                 in data, "unpairing hands the device its local key back")
            note(os.path.exists(nsec) and open(nsec, "rb").read() == nsec_before,
                 "identity.nsec is byte-identical across the whole pairing")
            note(not os.path.exists(os.path.join(cfg, "identity.bunker")),
                 "and the client session file is gone")
        note(app_alive(p, logpath), "the app is alive at the end of the live run")
    finally:
        if p is not None:
            stop(p, f)
        if pb is not None:
            stop(pb, pbfh)


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
    "mesh": test_mesh_map,
    "wallet": test_wallet,
    "nwc": test_nwc,
    "nwc_live": test_nwc_live,
    "nip46": test_nip46,
    "nip46wire": test_nip46_wire,
    "nip46_live": test_nip46_live,
    "trust": test_trust,
    "keys": test_key_actions,
    "docs": test_docs,
    "relayrow": test_relay_row,
    "roomsw": test_room_switch,
    "nstrfail": test_nstr_failures,
    "nstrqueue": test_nstr_queue,
    "live": test_live_relay,
    "urlforms": test_url_forms,
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
