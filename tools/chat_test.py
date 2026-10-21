#!/usr/bin/env python3
"""End to end test for the Messages tab.

Drives the real app and checks the real carriers:

  NET  a relay (tools/relay.py) carries a message between the app and
       another client, and the app's own message travels the other way
  BIT  the local mesh: a second instance of the app on the same host picks
       up what the first one broadcasts on port 2425

The app is started with PDA_START_TAB=1 (the Messages tab) and PDA_DEBUG=1,
so the test can read what happened from its log. Screenshots are taken of the
window so the result can be looked at, and the lit pixels are counted to
prove the message really landed on the screen.
"""
import json
import os
import re
import socket
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
APP = os.path.join(ROOT, "build", "pda")
RELAY = os.path.join(ROOT, "tools", "relay.py")
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


def start_app(name, env_extra):
    env = dict(os.environ)
    env.update(env_extra)
    env.setdefault("DISPLAY", ":0")
    logpath = os.path.join(LOGDIR, f"chat_{name}.log")
    log = open(logpath, "w")
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


def test_net(port):
    print("NET - the relay", flush=True)
    relay = subprocess.Popen([sys.executable, RELAY, "--host", "127.0.0.1",
                              "--port", str(port), "-v"],
                             stdout=open(os.path.join(LOGDIR, "chat_relay.log"), "w"),
                             stderr=subprocess.STDOUT)
    time.sleep(1.0)
    peer = socket.create_connection(("127.0.0.1", port), timeout=5)
    peer.settimeout(0.5)

    app, applog, log_a = start_app("a", {
        "PDA_RELAY_HOST": "127.0.0.1",
        "PDA_RELAY_PORT": str(port),
        "PDA_START_TAB": "1",
        "PDA_DEBUG": "1",
        "PDA_NAME": "Satscraft",
        "PDA_TEST_SEND": "anyone got the drop coords",
    })
    try:
        time.sleep(3.0)
        note(True, f"app booted ({os.path.getsize(log_a)} bytes of log)")

        before = shot("before")
        payload = {
            "v": 1, "t": "msg", "id": "test0001", "from": "ROOK",
            "room": "SECTOR", "text": "anyone got the drop coords",
            "ts": int(time.time()),
        }
        peer.sendall((json.dumps(payload) + "\n").encode())
        data = wait_for_log(log_a, r"\[chat\] recv via NET from ROOK", 10)
        note("recv via NET from ROOK" in data, "the app received the peer's message")

        time.sleep(1.0)
        after = shot("after")
        b = light_pixels(before, 120, 460)
        a = light_pixels(after, 120, 460)
        if b is not None and a is not None:
            note(a > b + 20, f"the message is on the screen ({b} -> {a} lit pixels)")
        else:
            note(False, "could not measure the screen (no PIL?)")

        # the app's own message must reach the relay
        peer.settimeout(6)
        got = None
        end = time.time() + 6
        while time.time() < end and not got:
            try:
                line = peer.recv(4096)
            except socket.timeout:
                continue
            for l in line.decode("utf-8", "replace").splitlines():
                if not l.strip():
                    continue
                try:
                    m = json.loads(l)
                except json.JSONDecodeError:
                    continue
                if m.get("t") == "msg" and m.get("text"):
                    got = m
        note(got is not None, "the app's own message reached the relay")
        if got:
            note(got.get("from") == "Satscraft", f"the relay sees the sender ({got.get('from')})")
    finally:
        stop_app(app, applog)
        peer.close()
        relay.terminate()
        try:
            relay.wait(timeout=5)
        except subprocess.TimeoutExpired:
            relay.kill()


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
    })
    try:
        time.sleep(2.0)
        peer.sendto(
            (json.dumps({"v": 1, "t": "msg", "id": "b1", "from": "ROOK",
                         "room": "SECTOR", "text": "heard you",
                         "ts": int(time.time())}) + "\n").encode(),
            ("127.0.0.1", 2425))
        data = wait_for_log(log_path, r"\[chat\] recv via BIT from ROOK", 8)
        note("recv via BIT from ROOK" in data,
             "the app heard a message on the mesh")
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


def main():
    if not os.path.exists(APP):
        print("build the app first: ./build.sh")
        return 1
    os.makedirs(LOGDIR, exist_ok=True)

    test_net(free_port())
    print()
    test_bit()
    print()
    test_fips()

    print()
    if FAIL:
        print(f"{len(FAIL)} check(s) failed")
        return 1
    print("all checks passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
