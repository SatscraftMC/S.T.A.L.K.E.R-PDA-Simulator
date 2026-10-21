#!/usr/bin/env python3
"""Reference relay for the PDA's NET carrier.

The relay is deliberately dumb: it reads one JSON object per line from every
client and hands it to all the other clients. That is enough for the room,
and it means the relay can live anywhere - a VPS, a home box, a phone.

    python3 tools/relay.py --port 7788

It can also bridge the room to an ntfy topic, which is how a PDA that cannot
reach the relay itself still reaches the internet (ntfy is a small FOSS
pub/sub server you can host yourself):

    python3 tools/relay.py --port 7788 --ntfy https://ntfy.sh --topic pda-zone

Every client sees the same envelope the PDA uses:

    {"v":1,"t":"msg","id":"...","from":"Satscraft","room":"SECTOR",
     "text":"anyone got the drop coords?","ts":1730000000}
"""
import argparse
import json
import os
import selectors
import socket
import ssl
import sys
import threading
import time
import urllib.error
import urllib.request

BUF = 8192


class Client:
    def __init__(self, sock, addr):
        self.sock = sock
        self.addr = addr
        self.buf = b""
        self.alive = True


class Relay:
    def __init__(self, host, port, ntfy=None, topic=None, verbose=False):
        self.host = host
        self.port = port
        self.ntfy = ntfy
        self.topic = topic or "pda-zone"
        self.verbose = verbose
        self.clients = []
        self.seen = set()
        self.lock = threading.Lock()
        self.sel = selectors.DefaultSelector()

    def log(self, *a):
        if self.verbose:
            print("[relay]", *a, flush=True)

    # ---------- the room ----------

    def broadcast(self, payload, skip=None):
        data = (json.dumps(payload, separators=(",", ":")) + "\n").encode()
        with self.lock:
            dead = []
            for c in self.clients:
                if c is skip or not c.alive:
                    continue
                try:
                    c.sock.sendall(data)
                except OSError:
                    c.alive = False
                    dead.append(c)
            for c in dead:
                self.drop(c)
        return data

    def drop(self, client):
        with self.lock:
            if client in self.clients:
                self.clients.remove(client)
        try:
            self.sel.unregister(client.sock)
        except (KeyError, ValueError):
            pass
        try:
            client.sock.close()
        except OSError:
            pass
        self.log("client gone", client.addr)

    def handle_line(self, client, line):
        line = line.strip()
        if not line:
            return
        try:
            msg = json.loads(line)
        except json.JSONDecodeError:
            self.log("bad line from", client.addr, line[:80])
            return
        if not isinstance(msg, dict):
            return
        mid = msg.get("id", "")
        if mid and mid in self.seen:
            return
        if mid:
            self.seen.add(mid)
            if len(self.seen) > 2000:
                self.seen = set(list(self.seen)[-1000:])
        self.log("->", msg.get("from"), msg.get("text"))
        self.broadcast(msg, skip=client)
        if self.ntfy:
            threading.Thread(target=self.ntfy_publish, args=(msg,), daemon=True).start()

    # ---------- ntfy ----------

    def ntfy_publish(self, msg):
        try:
            body = msg.get("text", "").encode()
            url = f"{self.ntfy.rstrip('/')}/{self.topic}"
            req = urllib.request.Request(url, data=body, method="POST")
            req.add_header("Title", msg.get("from", "pda"))
            req.add_header("X-Tags", "pda")
            with urllib.request.urlopen(req, timeout=10) as r:
                r.read()
            self.log("ntfy published")
        except Exception as e:                      # noqa: BLE001 - best effort
            self.log("ntfy publish failed:", e)

    def ntfy_subscribe(self):
        """Read the topic and push what arrives into the room."""
        url = f"{self.ntfy.rstrip('/')}/{self.topic}/json"
        self.log("ntfy subscribing", url)
        while True:
            try:
                with urllib.request.urlopen(url, timeout=60) as r:
                    for raw in r:
                        if not raw.strip():
                            continue
                        try:
                            ev = json.loads(raw)
                        except json.JSONDecodeError:
                            continue
                        if ev.get("event") != "message":
                            continue
                        text = ev.get("message", "")
                        if not text:
                            continue
                        mid = ev.get("id", "")
                        with self.lock:
                            if mid and mid in self.seen:
                                continue
                            if mid:
                                self.seen.add(mid)
                        msg = {
                            "v": 1, "t": "msg", "id": mid or str(time.time()),
                            "from": ev.get("title", "NET"),
                            "room": "SECTOR", "text": text,
                            "ts": int(time.time()),
                        }
                        self.log("ntfy ->", msg["from"], text)
                        self.broadcast(msg)
            except Exception as e:                  # noqa: BLE001 - keep trying
                self.log("ntfy subscribe failed:", e)
                time.sleep(5)

    # ---------- the loop ----------

    def run(self):
        srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        srv.bind((self.host, self.port))
        srv.listen(16)
        srv.setblocking(False)
        print(f"relay listening on {self.host}:{self.port}", flush=True)
        if self.ntfy:
            print(f"bridging to ntfy topic {self.topic}", flush=True)
            threading.Thread(target=self.ntfy_subscribe, daemon=True).start()

        sel = self.sel
        sel.register(srv, selectors.EVENT_READ, None)
        while True:
            for key, _ in sel.select(timeout=1.0):
                if key.data is None:
                    conn, addr = srv.accept()
                    conn.setblocking(False)
                    c = Client(conn, addr)
                    with self.lock:
                        self.clients.append(c)
                    sel.register(conn, selectors.EVENT_READ, c)
                    self.log("client in", addr)
                else:
                    c = key.data
                    try:
                        data = c.sock.recv(BUF)
                    except (BlockingIOError, InterruptedError):
                        continue
                    except OSError:
                        self.drop(c)
                        continue
                    if not data:
                        self.drop(c)
                        continue
                    c.buf += data
                    while b"\n" in c.buf:
                        line, c.buf = c.buf.split(b"\n", 1)
                        self.handle_line(c, line.decode("utf-8", "replace"))


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--host", default="0.0.0.0")
    ap.add_argument("--port", type=int, default=7788)
    ap.add_argument("--ntfy", default=None, help="ntfy base url, e.g. https://ntfy.sh")
    ap.add_argument("--topic", default="pda-zone")
    ap.add_argument("-v", "--verbose", action="store_true")
    args = ap.parse_args()
    Relay(args.host, args.port, args.ntfy, args.topic, args.verbose).run()


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        pass
