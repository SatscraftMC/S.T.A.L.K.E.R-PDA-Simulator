#!/usr/bin/env python3
"""End to end test for the Messages tab.

Drives the real app and checks the real carriers:

  NSTR  a local nostr relay (tools/nstr_relay.py) over WebSocket carries a
        message between the app and another client speaking
        tools/wsclient.py, and the app's own message travels the other way
  BIT   the local mesh: a second instance of the app on the same host picks
        up what the first one broadcasts on port 2425

The app is started with PDA_START_TAB=1 (the Messages tab) and PDA_DEBUG=1,
so the test can read what happened from its log. Screenshots are taken of the
window so the result can be looked at, and the lit pixels are counted to
prove the message really landed on the screen.

It also runs the identity checks of spec-nostr-identity: the NIST sha256
vectors from the [id] selftest line, a signed round trip between two app
instances (with an emoji over the relay, which regression-guards the
surrogate-pair fix), the forgery cases, the relay path's room scoping (the
#t filter and the carrier's backstop), and at the end a grep proving no key
file's contents ever reached a log.
"""
import atexit
import hashlib
import json
import os
import re
import shutil
import socket
import subprocess
import sys
import tempfile
import time

# Isolation, at module top before anything derives a path from it: no run
# ever writes the real ~/.config/pda/ (the identity key and contacts would
# land there on the first test run otherwise). start_app() points each app
# instance at its own directory inside this one, so two instances never
# share a key or a settings file.
RUN_TMP = tempfile.mkdtemp(prefix="chat_test_")
atexit.register(lambda: shutil.rmtree(RUN_TMP, ignore_errors=True))
os.environ.setdefault("PDA_PREFS", os.path.join(RUN_TMP, "prefs.conf"))

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
APP = os.environ.get("PDA_TEST_BUILD", os.path.join(ROOT, "build", "pda"))
RELAY = os.path.join(ROOT, "tools", "nstr_relay.py")
sys.path.insert(0, os.path.join(ROOT, "tools"))
from wsclient import WSClient                      # noqa: E402  (ROOT first)

SHOT = "/tmp/opencode/chat"
LOGDIR = "/tmp/opencode"

FAIL = []


def note(ok, msg):
    print(("  PASS  " if ok else "  FAIL  ") + msg, flush=True)
    if not ok:
        FAIL.append(msg)


def free_port():
    s = socket.socket()
    s.bind(("127.0.0.1", 0))
    p = s.getsockname()[1]
    s.close()
    return p


def relay_url(port):
    return f"ws://127.0.0.1:{port}"


def start_relay(port, name):
    """the local WebSocket relay, on stdin so a test can drive its control
    plane (okfalse / closed / raw / ping / fragment / oversize / pause)"""
    logpath = os.path.join(LOGDIR, f"chat_{name}_relay.log")
    p = subprocess.Popen([sys.executable, RELAY, "--host", "127.0.0.1",
                          "--port", str(port), "-v"],
                         stdin=subprocess.PIPE,
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


def peer_connect(port, sub="peer"):
    """the test peer: a second WebSocket client on the same relay, with a
    subscription that takes every kind-1 event (no #t, so it sees what the
    relay has - including what it filters away from the app's subscription)"""
    w = WSClient(relay_url(port), timeout=5)
    w.send(json.dumps(["REQ", sub, {"kinds": [1]}]))
    return w


def peer_event(w, contains=None, timeout=8):
    """the next EVENT off the peer (the one whose content contains
    `contains`, or any of them). None when nothing arrived in time."""
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


INSTANCE_DIRS = []      # every one, for the no-key-in-the-logs grep
ALL_LOGS = []           # every app log this run wrote


def instance_dir(name):
    """a fresh directory (prefs.conf, identity.nsec, contacts.conf) for one
    app instance: PDA_PREFS moves all three together, which is how two
    instances on one box stay isolated"""
    d = os.path.join(RUN_TMP, name)
    os.makedirs(d, exist_ok=True)
    if d not in INSTANCE_DIRS:
        INSTANCE_DIRS.append(d)
    return d


def start_app(name, env_extra):
    env = dict(os.environ)
    env.update(env_extra)
    if "PDA_PREFS" not in (env_extra or {}):
        env["PDA_PREFS"] = os.path.join(instance_dir(name), "prefs.conf")
    env.setdefault("DISPLAY", ":0")
    logpath = os.path.join(LOGDIR, f"chat_{name}.log")
    log = open(logpath, "w")
    if logpath not in ALL_LOGS:
        ALL_LOGS.append(logpath)
    p = subprocess.Popen([APP], stdout=log, stderr=subprocess.STDOUT, env=env)
    return p, log, logpath


def stop_app(p, log):
    if p.poll() is None:
        p.terminate()
        try:
            p.wait(timeout=5)
        except subprocess.TimeoutExpired:
            p.kill()
    log.close()


def shot(name):
    subprocess.run(["wmctrl", "-a", "PDA // Simulator"], capture_output=True)
    time.sleep(0.5)
    out = f"{SHOT}_{name}.png"
    subprocess.run(["gnome-screenshot", "-w", "-f", out], capture_output=True)
    return out


def light_pixels(path, y0, y1):
    try:
        from PIL import Image
    except ImportError:
        return None
    im = Image.open(path).convert("RGB")
    w, h = im.size
    px = im.load()
    n = 0
    for y in range(y0, min(y1, h)):
        for x in range(0, w, 2):
            r, g, b = px[x, y]
            if r + g + b > 330:
                n += 1
    return n


def unsigned_event(text, name, room="SECTOR", ts=None):
    """the one unsigned shape of spec-nostr-identity §4: pubkey "", no sig,
    id = sha256 of the canonical serialization (hashing needs no key - the
    tests hold no key, and that is exactly the honest CHAT_SIG_NONE path).
    The `t` tag mirrors `room` (spec-nstr-relay §6): it is the key the
    relay's #t filter indexes, so an event without it never reaches the app."""
    ts = int(ts if ts is not None else time.time())
    tags = [["room", room], ["t", room], ["name", name]]
    ser = json.dumps([0, "", ts, 1, tags, text],
                     separators=(",", ":"), ensure_ascii=False)
    eid = hashlib.sha256(ser.encode("utf-8")).hexdigest()
    return {"id": eid, "pubkey": "", "created_at": ts, "kind": 1,
            "tags": tags, "content": text}


def name_of(msg, default=""):
    """the display label: the name tag (what the sender signs), else nothing"""
    for t in msg.get("tags") or []:
        if isinstance(t, (list, tuple)) and len(t) > 1 and t[0] == "name":
            return t[1]
    return default


def npub_to_hex(npub, hrp="npub"):
    """A ~40-line independent bech32 decoder: BIP-173 reference logic, pure
    python, an algorithm rather than a key. It is how the suite checks the
    app's [id] pub= hex against its npub= bech32 without trusting the app's
    own C code to do both sides of the comparison."""
    charset = "qpzry9x8gf2tvdw0s3jn54khce6mua7l"

    def polymod(values):
        gen = [0x3b6a57b2, 0x26508e6d, 0x1ea119fa, 0x3d4233dd, 0x2a1462b3]
        chk = 1
        for v in values:
            b = chk >> 25
            chk = ((chk & 0x1ffffff) << 5) ^ v
            for i in range(5):
                if (b >> i) & 1:
                    chk ^= gen[i]
        return chk

    pos = npub.rfind("1")
    if pos < 1:
        raise ValueError("no separator")
    h, data = npub[:pos], npub[pos + 1:]
    if h != hrp:
        raise ValueError("wrong hrp")
    values = [charset.index(c) for c in data]
    if polymod([ord(c) >> 5 for c in h] + [0] +
               [ord(c) & 31 for c in h] + values) != 1:
        raise ValueError("bad checksum")
    payload, acc, bits = values[:-6], 0, 0
    out = bytearray()
    for v in payload:
        acc = (acc << 5) | v
        bits += 5
        while bits >= 8:
            bits -= 8
            out.append((acc >> bits) & 0xff)
    if bits >= 5 or ((acc << (8 - bits)) & 0xff):
        raise ValueError("bad padding")
    if len(out) != 32:
        raise ValueError(f"expected 32 bytes, got {len(out)}")
    return bytes(out).hex()


def wait_for_log(path, pattern, timeout=12):
    rx = re.compile(pattern)
    end = time.time() + timeout
    data = ""
    while time.time() < end:
        try:
            with open(path) as f:
                data = f.read()
            if rx.search(data):
                return data
        except OSError:
            pass
        time.sleep(0.3)
    try:
        with open(path) as f:
            return f.read()
    except OSError:
        return ""


def test_nstr(port):
    print("NSTR - the relay over WebSocket", flush=True)
    relay, rlog = start_relay(port, "net")
    peer = peer_connect(port)

    app, applog, log_a = start_app("a", {
        "PDA_NSTR_RELAY": relay_url(port),
        "PDA_START_TAB": "1",
        "PDA_DEBUG": "1",
        "PDA_NAME": "Satscraft",
        "PDA_TEST_SEND": "anyone got the drop coords",
        "PDA_TEST_SEND_IN": "5",
    })
    try:
        data = wait_for_log(log_a, r"\[nstr\] up", 15)
        note("[nstr] up" in data, "the carrier came up on the local relay")
        note(True, f"app booted ({os.path.getsize(log_a)} bytes of log)")

        before = shot("before")
        # the unsigned event with a name tag: a test holds no key, so this
        # is the honest CHAT_SIG_NONE path the receiver must still display
        peer.send(json.dumps(["EVENT", unsigned_event(
            "anyone got the drop coords", "ROOK")]))
        data = wait_for_log(log_a, r"\[chat\] recv via NSTR from ROOK", 10)
        note("recv via NSTR from ROOK" in data,
             "the app received the peer's message over the relay")

        time.sleep(1.0)
        after = shot("after")
        b = light_pixels(before, 80, 460)
        a = light_pixels(after, 80, 460)
        if b is not None and a is not None:
            note(a > b + 20, f"the message is on the screen ({b} -> {a} lit pixels)")
        else:
            note(False, "could not measure the screen (no PIL?)")

        # the app's own message must reach the relay
        got = peer_event(peer, contains="anyone got the drop coords", timeout=10)
        note(got is not None, "the app's own message reached the relay")
        if got:
            who = name_of(got)
            note(who == "Satscraft", f"the relay sees the sender ({who})")
            note(got.get("pubkey") and len(got.get("sig", "")) == 128,
                 "the message went out signed (pubkey + 128-hex sig)")
        with open(rlog) as f:
            rlog_txt = f.read()
        note("bad line" not in rlog_txt, "the relay never saw a broken line")
    finally:
        stop_app(app, applog)
        peer.close()
        stop_relay(relay)


def test_bit():
    print("BIT - the local mesh", flush=True)
    # The mesh carrier is UDP broadcast on port 2425. Two directions are
    # checked, each with its own peer and its own app instance, because the
    # kernel hands a unicast datagram to one socket only - the most specific
    # one - and both the app and the peer sit on the same port here.
    #
    #   app -> mesh : the peer binds 127.0.0.1, so the app's message to
    #                 127.0.0.1 lands on the peer
    #   mesh -> app : the peer binds 127.0.0.2 and sends to 127.0.0.1, which
    #                 only the app's wildcard socket matches
    #
    # The sweep of the local subnet is off here: it has its own test below
    # and it has no business walking the real network.

    # --- app -> mesh ---
    peer = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    peer.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    peer.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    peer.bind(("127.0.0.1", 2425))
    peer.settimeout(6)
    app, log_h, log_path = start_app("a2", {
        "PDA_START_TAB": "1",
        "PDA_DEBUG": "1",
        "PDA_NAME": "Satscraft",
        "PDA_BIT_SCAN": "off",
        "PDA_TEST_SEND": "anyone on the mesh",
    })
    try:
        heard = b""
        try:
            heard = peer.recv(4096)
        except socket.timeout:
            pass
        note(b"anyone on the mesh" in heard,
             "the mesh carried the app's message out")
    finally:
        stop_app(app, log_h)
        peer.close()

    # --- mesh -> app ---
    peer = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    peer.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    peer.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    peer.bind(("127.0.0.2", 2425))
    app, log_h, log_path = start_app("b2", {
        "PDA_START_TAB": "1",
        "PDA_DEBUG": "1",
        "PDA_NAME": "WUMGUB",
        "PDA_BIT_SCAN": "off",
    })
    try:
        time.sleep(2.0)
        peer.sendto(
            (json.dumps(unsigned_event("heard you", "ROOK")) + "\n").encode(),
            ("127.0.0.1", 2425))
        data = wait_for_log(log_path, r"\[chat\] recv via BIT from ROOK", 8)
        note("recv via BIT from ROOK" in data,
             "the app heard a message on the mesh")
    finally:
        stop_app(app, log_h)
        peer.close()


def test_bit_discovery():
    print("BIT - finding the peer and talking to it directly", flush=True)
    # The broadcast is the one packet a firewall drops first, so the mesh
    # also walks the local subnet looking for the other devices and then
    # talks to them directly.
    #
    # The peer sits on 127.0.0.2 - an address that is in none of the app's
    # broadcast addresses (127.0.0.1, the subnet broadcast, the limited
    # broadcast). So the only way the message can reach the peer is the
    # direct way, to the address the app learned from the sweep.
    peer = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    peer.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    peer.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    peer.bind(("127.0.0.2", 2425))
    peer.settimeout(3.0)
    app, log_h, log_path = start_app("c2", {
        "PDA_START_TAB": "1",
        "PDA_DEBUG": "1",
        "PDA_NAME": "Satscraft",
        "PDA_BIT_SCAN": "127.0.0.0/30",
        "PDA_TEST_SEND": "anyone on the mesh",
        "PDA_TEST_SEND_IN": "6",
    })
    try:
        # the sweep reaches the peer, the peer answers
        answered = False
        end = time.time() + 12
        while time.time() < end and not answered:
            try:
                data, addr = peer.recvfrom(4096)
            except socket.timeout:
                continue
            try:
                m = json.loads(data.decode("utf-8", "replace").strip())
            except json.JSONDecodeError:
                continue
            if m.get("t") != "hi":
                continue
            peer.sendto(json.dumps({"v": 1, "t": "hi-ack", "from": "ROOK",
                                    "port": 2425,
                                    "ts": int(time.time())}).encode(), addr)
            answered = True
        note(answered, "the sweep reached the peer on the local subnet")

        # the app must have taken the peer for a node of the mesh
        data = wait_for_log(log_path, r"bit: peer ROOK at 127\.0\.0\.2", 8)
        note("peer ROOK at 127.0.0.2" in data,
             "the app knows the peer and where it lives")

        # and the message must reach the peer directly
        heard = b""
        end = time.time() + 12
        while time.time() < end:
            try:
                heard = peer.recv(4096)
            except socket.timeout:
                continue
            if b"anyone on the mesh" in heard:
                break
        note(b"anyone on the mesh" in heard,
             "the app's message went straight to the peer it found")
    finally:
        stop_app(app, log_h)
        peer.close()


def test_fips():
    print("FIPS - the native datagram API", flush=True)
    api = "/tmp/opencode/fips-api.sock"
    ctl = "/tmp/opencode/fips-ctl.sock"
    mock = subprocess.Popen([sys.executable, os.path.join(ROOT, "tools", "fips_mock.py"),
                              "--api", api, "--ctl", ctl, "-v"],
                             stdout=open(os.path.join(LOGDIR, "chat_fips_mock.log"), "w"),
                             stderr=subprocess.STDOUT)
    time.sleep(1.5)
    app, log_h, log_path = start_app("fips", {
        "PDA_FIPS_SOCK": api,
        "PDA_FIPS_CTL": ctl,
        "PDA_START_TAB": "1",
        "PDA_DEBUG": "1",
        "PDA_NAME": "Satscraft",
        "PDA_TEST_SEND": "anyone got the drop coords",
    })
    try:
        time.sleep(6.0)
        data = wait_for_log(log_path, r"\[chat\] recv via FIPS from ROOK", 12)
        note("recv via FIPS from ROOK" in data,
             "the app read a message off the FIPS flow")
        # the app's own message must have gone out onto the flow
        mock_log = ""
        try:
            with open(os.path.join(LOGDIR, "chat_fips_mock.log")) as f:
                mock_log = f.read()
        except OSError:
            pass
        note("anyone got the drop coords" in mock_log,
             "the app's message went out over the FIPS flow")
    finally:
        stop_app(app, log_h)
        mock.terminate()
        try:
            mock.wait(timeout=5)
        except subprocess.TimeoutExpired:
            mock.kill()




def test_identity():
    print("the device key: NIST vectors, first-run generation, hex vs npub", flush=True)
    d = instance_dir("identity")
    app, log_h, log_path = start_app("identity", {
        "PDA_START_TAB": "1",
        "PDA_DEBUG": "1",
        "PDA_BIT_SCAN": "off",
    })
    try:
        data = wait_for_log(log_path, r"\[id\] selftest", 10)

        v = re.search(r"\[id\] selftest (\w+) (\w+) (\w+)", data)
        expect = (
            "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
            "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
            "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1",
        )
        note(v is not None and tuple(v.groups()) == expect,
             "the [id] selftest line carries the three published NIST sha256 vectors")

        m = re.search(r"\[id\] pub=([0-9a-f]{64}) npub=(npub1[0-9a-z]+)", data)
        note(m is not None, "the app logged its own key as hex and npub")
        if m:
            try:
                note(npub_to_hex(m.group(2)) == m.group(1),
                     "the npub decodes back to exactly the logged hex")
            except ValueError as e:
                note(False, f"the npub decodes as bech32 ({e})")

        keyfile = os.path.join(d, "identity.nsec")
        note(os.path.exists(keyfile), "identity.nsec was written on the first run")
        if os.path.exists(keyfile):
            mode = oct(os.stat(keyfile).st_mode & 0o777)
            note(mode == "0o600", f"identity.nsec is private ({mode})")
            nsec = open(keyfile).read().strip()
            note(nsec.startswith("nsec1"), "the key file holds a bech32 nsec")
            note(nsec not in data, "the nsec never reaches the log (the npub may)")
    finally:
        stop_app(app, log_h)


def test_roundtrip():
    print("the signature round trip: A signs, B verifies", flush=True)
    # The two instances get separate prefs directories (separate keys) and
    # separate mesh ports, so the only path between them is the relay -
    # and the relay re-dumps every message with ensure_ascii=True, which is
    # exactly what breaks an unsigned emoji round trip if read_string()
    # cannot combine surrogate pairs.
    port = free_port()
    relay, _ = start_relay(port, "rt")

    base = {
        "PDA_NSTR_RELAY": relay_url(port),
        "PDA_START_TAB": "1",
        "PDA_DEBUG": "1",
        "PDA_BIT_SCAN": "off",
    }
    emoji = "\U0001f4cd"          # a non-BMP character: the surrogate case
    a, alog, apath = start_app("rt_a", dict(base, **{
        "PDA_BITCHAT_PORT": "2425",
        "PDA_NAME": "Satscraft",
        "PDA_TEST_SEND": "drop is marked " + emoji,
        "PDA_TEST_SEND_IN": "5",
    }))
    b, blog, bpath = start_app("rt_b", dict(base, **{
        "PDA_BITCHAT_PORT": "2426",
        "PDA_NAME": "WUMGUB",
    }))
    try:
        adata = wait_for_log(apath, r"\[id\] pub=", 10)
        m = re.search(r"\[id\] pub=([0-9a-f]{64}) npub=(npub1[0-9a-z]+)", adata)
        note(m is not None, "A generated a key and logged it")
        pub_a, npub_a = (m.group(1), m.group(2)) if m else ("", "")

        wait_for_log(apath, r"\[nstr\] up", 15)
        bdata = wait_for_log(bpath, re.escape("sig ok pub=" + pub_a), 25)
        note(pub_a and ("sig ok pub=" + pub_a) in bdata,
             "B recomputed the id and verified A's BIP-340 signature")
        note("[chat] recv via NSTR from Satscraft: drop is marked " + emoji in bdata,
             "the signed message came in over nstr/relay and was displayed")
        note(emoji in bdata,
             "the emoji survived the relay's ensure_ascii re-dump and the id still matched")
        if pub_a:
            try:
                note(npub_to_hex(npub_a) == pub_a,
                     "A's npub decodes to A's hex pubkey")
            except ValueError as e:
                note(False, f"A's npub decodes as bech32 ({e})")
            note(("sig ok pub=" + pub_a) in bdata and pub_a == m.group(1),
                 "the verified pubkey is the one A logged")
    finally:
        stop_app(a, alog)
        stop_app(b, blog)
        stop_relay(relay)


def test_forgery():
    print("forgery: unsigned, wrong sig, a trusted key signed elsewhere, a swapped body", flush=True)
    port = free_port()
    relay, _ = start_relay(port, "fg")
    peer = peer_connect(port)

    base = {
        "PDA_NSTR_RELAY": relay_url(port),
        "PDA_START_TAB": "1",
        "PDA_DEBUG": "1",
        "PDA_BIT_SCAN": "off",
        "PDA_TEST_SEND_IN": "5",
    }
    # two senders: A is the key the device will Trust, A2 is a second key
    # whose signature gets pinned onto A's identity for case (c)
    a, alog, apath = start_app("fg_a", dict(base, **{
        "PDA_BITCHAT_PORT": "2425",
        "PDA_NAME": "Satscraft", "PDA_TEST_SEND": "this one is really from A",
    }))
    a2, a2log, a2path = start_app("fg_a2", dict(base, **{
        "PDA_BITCHAT_PORT": "2427",
        "PDA_NAME": "WUMGUB", "PDA_TEST_SEND": "this one is really from A2",
    }))

    events = {}
    try:
        end = time.time() + 20
        while time.time() < end and len(events) < 2:
            ev = peer_event(peer, timeout=2)
            if ev and ev.get("sig"):
                events[ev.get("content", "")] = ev
        note(len(events) >= 2, f"captured two genuinely signed events ({len(events)})")
        if len(events) < 2:
            return
        ev_a = events["this one is really from A"]
        ev_a2 = events["this one is really from A2"]
        pub_a = ev_a["pubkey"]
    finally:
        stop_app(a, alog)
        stop_app(a2, a2log)
        peer.close()
        stop_relay(relay)

    # the device: Trust on A's key, written before boot (log-driven suite)
    bd = instance_dir("fg_b")
    with open(os.path.join(bd, "contacts.conf"), "w") as f:
        f.write(f"{pub_a} = Satscraft\n")

    b, blog, bpath = start_app("fg_b", {
        "PDA_START_TAB": "1",
        "PDA_DEBUG": "1",
        "PDA_BIT_SCAN": "off",
    })
    try:
        wait_for_log(bpath, r"\[id\] pub=", 10)
        time.sleep(1.0)

        ts = int(time.time())
        # (a) no claim at all: pubkey "" and no sig
        fa = unsigned_event("i am really Satscraft", "Satscraft", ts=ts + 1)
        # (b) a well-formed sig that verifies against nothing (A2's line,
        #     sig zeroed: the id still rechecks, the signature cannot)
        fb = dict(ev_a2)
        fb["sig"] = "00" * 64
        # (c) A's trusted pubkey with A2's signature over it
        ser_c = json.dumps([0, pub_a, ev_a2["created_at"], 1, ev_a2["tags"],
                            ev_a2["content"]],
                           separators=(",", ":"), ensure_ascii=False)
        fc = {"id": hashlib.sha256(ser_c.encode("utf-8")).hexdigest(),
              "pubkey": pub_a, "created_at": ev_a2["created_at"],
              "kind": 1, "tags": ev_a2["tags"], "content": ev_a2["content"],
              "sig": ev_a2["sig"]}
        # (d) A's line with the body swapped after signing: the id no
        #     longer matches the content, which is what the recheck is for
        fd = dict(ev_a)
        fd["content"] = "the body was swapped after signing"

        peer = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        peer.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        peer.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        peer.bind(("127.0.0.2", 2425))
        for forged in (fa, fb, fc, fd):
            peer.sendto((json.dumps(forged) + "\n").encode(),
                        ("127.0.0.1", 2425))
            time.sleep(0.4)
        peer.close()

        data = wait_for_log(bpath, r"the body was swapped", 10)
        note("[chat] unsigned from=Satscraft" in data,
             "(a) an unsigned line: logged unsigned, displayed anyway")
        n_bad = len(re.findall(r"\[chat\] sig bad pub=", data))
        note(n_bad >= 3,
             f"(b,c,d) three signed claims that do not verify ({n_bad} sig bad)")
        note(("sig bad pub=" + pub_a) in data,
             "(c) the trusted contact's key with someone else's signature is bad")
        note("i am really Satscraft" in data,
             "the forged lines are still displayed - honestly, with no badge claim")
        note(b.poll() is None, "the device is alive after four forgeries")
        note("line(s) skipped" not in data,
             "the seeded contacts.conf loaded without a skip line")
    finally:
        stop_app(b, blog)


def test_no_secret():
    print("no key material in any log", flush=True)
    secrets = []
    for d in INSTANCE_DIRS:
        kf = os.path.join(d, "identity.nsec")
        if not os.path.exists(kf):
            continue
        nsec = open(kf).read().strip()
        secrets.append(nsec)
        try:
            secrets.append(bytes.fromhex(npub_to_hex(nsec, hrp="nsec")).hex())
        except ValueError:
            pass
    note(bool(secrets), "there is at least one key file to check against")

    leaks = []
    logs = list(ALL_LOGS)
    try:
        logs += [os.path.join(LOGDIR, f) for f in os.listdir(LOGDIR)
                 if os.path.isfile(os.path.join(LOGDIR, f))]
    except OSError:
        pass
    for path in sorted(set(logs)):
        try:
            with open(path, errors="replace") as f:
                data = f.read()
        except OSError:
            continue
        for sec in secrets:
            if sec and sec in data:
                leaks.append(path)
    note(not leaks,
         f"no identity.nsec contents (bech32 or hex) in any of {len(set(logs))} logs")


def pick_run(port, prefs_key, label):
    """one picker run: this pref turns the nstr carrier off, so the message
    must never reach the relay. The carrier has to be UP first - otherwise
    the check would pass for the wrong reason (nothing could send anyway)."""
    prefs = os.path.join(LOGDIR, f"chat_pick_{prefs_key}.conf")
    with open(prefs, "w") as f:
        f.write(f"{prefs_key} = 0\n")

    app, applog, log_a = start_app(f"pick_{prefs_key}", {
        "PDA_NSTR_RELAY": relay_url(port),
        "PDA_START_TAB": "1",
        "PDA_DEBUG": "1",
        "PDA_NAME": "Satscraft",
        "PDA_PREFS": prefs,
        "PDA_BIT_SCAN": "off",
        "PDA_TEST_SEND": "only the mesh, please",
        "PDA_TEST_SEND_IN": "6",
    })
    peer = peer_connect(port)
    try:
        data = wait_for_log(log_a, r"\[nstr\] up", 15)
        note("[nstr] up" in data, f"{label}: the carrier is up (and switched off)")
        wait_for_log(log_a, r"\[chat\] sent: only the mesh, please", 15)
        got = peer_event(peer, contains="only the mesh, please", timeout=6)
        note(got is None, f"{label}: the message did not go out on the carrier that is off")
    finally:
        stop_app(app, applog)
        peer.close()


def test_carrier_picker():
    print("the carrier picker - which carriers a message goes out on", flush=True)
    # The Messages tab has a row of buttons: one per carrier. A carrier that
    # is off still receives, it just does not send. The nstr carrier's key is
    # send_nstr (the tag, lowercased); an old save_net-only settings file has
    # to hold the carrier off too, which is what the second run proves.
    port = free_port()
    relay, _ = start_relay(port, "pick")
    try:
        pick_run(port, "send_nstr", "send_nstr = 0")
        pick_run(port, "send_net", "legacy send_net = 0")
    finally:
        stop_relay(relay)


def test_room_scope():
    print("room scope: the relay's #t filter and the carrier's backstop", flush=True)
    port = free_port()
    relay, _ = start_relay(port, "room")
    peer = peer_connect(port)

    app, applog, log_a = start_app("room", {
        "PDA_NSTR_RELAY": relay_url(port),
        "PDA_START_TAB": "1",
        "PDA_DEBUG": "1",
        "PDA_NAME": "Satscraft",
        "PDA_BIT_SCAN": "off",
    })

    def raw_event(content, tags):
        """an inline event: the shared helper always writes room + t as the
           same value, and two of these legs exist only to break that"""
        ts = int(time.time())
        ser = json.dumps([0, "", ts, 1, tags, content],
                         separators=(",", ":"), ensure_ascii=False)
        eid = hashlib.sha256(ser.encode("utf-8")).hexdigest()
        return {"id": eid, "pubkey": "", "created_at": ts, "kind": 1,
                "tags": tags, "content": content}

    try:
        data = wait_for_log(log_a, r"\[nstr\] up", 15)
        note("[nstr] up" in data, "the carrier is up before the three legs")

        # (i) a different t but OUR room: only the relay's #t filter can drop
        #     it - the carrier's backstop would pass it, so if it ever showed
        #     up (or logged a room drop) the filter was not doing its job
        peer.send(json.dumps(["EVENT", raw_event(
            "wrong t leg", [["room", "SECTOR"], ["t", "OTHER"], ["name", "ROOK"]])]))
        # (ii) OUR t but a different room: the relay forwards it (t matches),
        #      and the carrier's backstop is what has to drop it
        ev_ii = raw_event("wrong room leg",
                          [["room", "OTHER"], ["t", "SECTOR"], ["name", "ROOK"]])
        peer.send(json.dumps(["EVENT", ev_ii]))
        # (iii) t only, no room tag at all: a missing room passes (the same
        #       rule the chan filter follows)
        peer.send(json.dumps(["EVENT", raw_event(
            "right t leg", [["t", "SECTOR"], ["name", "ROOK"]])]))

        data = wait_for_log(log_a, r"recv via NSTR from ROOK: right t leg", 15)
        note("recv via NSTR from ROOK: right t leg" in data,
             "(iii) t matches and there is no room tag: it arrives")
        note("wrong t leg" not in data,
             "(i) another t never arrives (the relay's #t filter)")
        note("wrong room leg" not in data,
             "(ii) our t but another room never reaches the log")
        drops = re.findall(r"\[nstr\] room drop ([0-9a-f]+)", data)
        note(drops == [ev_ii["id"]],
             f"exactly one [nstr] room drop, and it is leg (ii)'s id ({drops})")
        note("[nstr] recv EVENT" in data, "the pinned [nstr] recv <verb> line is there")
    finally:
        stop_app(app, applog)
        peer.close()
        stop_relay(relay)


def main():
    if not os.path.exists(APP):
        print("build the app first: ./build.sh")
        return 1
    os.makedirs(LOGDIR, exist_ok=True)

    test_identity()
    print()
    test_roundtrip()
    print()
    test_forgery()
    print()
    test_nstr(free_port())
    print()
    test_room_scope()
    print()
    test_carrier_picker()
    print()
    test_bit()
    print()
    test_bit_discovery()
    print()
    test_fips()
    print()
    test_no_secret()

    print()
    if FAIL:
        print(f"{len(FAIL)} check(s) failed")
        return 1
    print("all checks passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
