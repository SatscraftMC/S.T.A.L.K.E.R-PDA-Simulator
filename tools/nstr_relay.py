#!/usr/bin/env python3
"""The local relay for the suites: a WebSocket + NIP-01 test double.

    python3 tools/nstr_relay.py --host 127.0.0.1 --port <free> -v

What it is: a dumb service the tests point the app at, the same idea as
relay.py one protocol over - but it speaks the real wire the app now speaks
(one complete WebSocket text message = one NIP-01 JSON array), honours the
filter grammar the room scope needs (kinds + #t), and answers like a real
relay would:

    EVENT  -> ["OK", id, true, ""]  (a known id still gets an OK, as
             ["OK", id, true, "duplicate: already have this event"];
             stored once, forwarded never)  + forwarded to matching subs
    REQ    -> the stored matching events, then ["EOSE", sub]. A second REQ
             with the same sub id *replaces* the subscription - the
             primitive the room switch depends on.
    CLOSE  -> that subscription stops

Every event it stores or forwards is json.loads -> json.dumps'd, so what
the app receives is not the bytes the sender wrote (the emoji/surrogate
regression guard relay.py carried lives on here).

The control plane is one command per stdin line, broadcast to every
connected client - the things a well-behaved relay would never send:
    okfalse <id> <msg>   ["OK", id, false, msg]
    closed <sub> <msg>   ["CLOSED", sub, msg]
    raw <text>           an arbitrary text message, as-is
    ping                 a WebSocket ping frame (the log below records the
                         app's pong - how the keep-alive is observed)
    fragment <text>      the same text across a continuation frame
    oversize <nbytes>    one oversized frame (for the app's "bad ws" close)
    pause | resume       stop/start *reading* from the sockets: real
                         backpressure, so the client's kernel buffer fills
                         and its outbound queue with it
    quit

Everything the harness sends, and every pong it receives, is printed to
stdout - that log is the suites' assertion surface. --tls puts the same
server behind python's ssl with a --cert/--key pair the suite generates.
"""
import argparse
import base64
import hashlib
import json
import select
import selectors
import socket
import ssl
import sys
import threading

BUF = 8192
GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"


def log(*a):
    print(*a, flush=True)


def server_handshake(conn, addr, vlog=None):
    """Read the HTTP upgrade, answer 101 with the accept back, and return
    whatever bytes came after the header (the first frames may share the
    segment). Raises OSError when the request is not a usable upgrade."""
    data = bytearray()
    conn.settimeout(5)
    while b"\r\n\r\n" not in data:
        chunk = conn.recv(4096)
        if not chunk:
            raise OSError("peer closed before the upgrade")
        data += chunk
        if len(data) > 8192:
            raise OSError("upgrade request too large")
    head, _, rest = bytes(data).partition(b"\r\n\r\n")
    text = head.decode("latin-1")
    lines = text.split("\r\n")
    if not lines or not lines[0].startswith("GET "):
        raise OSError("not an upgrade request")
    key = None
    upgrade = connection = ""
    for line in lines[1:]:
        if ":" not in line:
            continue
        n, _, v = line.partition(":")
        n, v = n.strip().lower(), v.strip()
        if n == "sec-websocket-key":
            key = v
        elif n == "upgrade":
            upgrade = v.lower()
        elif n == "connection":
            connection = v.lower()
    if not key or "websocket" not in upgrade or "upgrade" not in connection:
        raise OSError("not a websocket upgrade")
    accept = base64.b64encode(
        hashlib.sha1((key + GUID).encode()).digest()).decode()
    conn.sendall((f"HTTP/1.1 101 Switching Protocols\r\n"
                  "Upgrade: websocket\r\n"
                  "Connection: Upgrade\r\n"
                  f"Sec-WebSocket-Accept: {accept}\r\n\r\n").encode())
    if vlog:
        vlog("upgraded", addr)
    return rest


class Client:
    def __init__(self, sock, addr):
        self.sock = sock
        self.addr = addr
        self.buf = bytearray()
        self.subs = {}              # sub id -> [filter, ...]
        self.alive = True


class Relay:
    def __init__(self, host, port, tls_ctx=None, verbose=False):
        self.host = host
        self.port = port
        self.tls_ctx = tls_ctx
        self.verbose = verbose
        self.clients = []
        self.events = {}            # id -> event, in insertion order
        self.lock = threading.Lock()
        self.sel = selectors.DefaultSelector()
        self.paused = False
        self.running = True

    def vlog(self, *a):
        if self.verbose:
            log("[nstr_relay]", *a)

    # ---------- sending ----------

    @staticmethod
    def dump(payload):
        # default ensure_ascii=True: the re-dump the app must survive
        return json.dumps(payload, separators=(",", ":"))

    def send_text(self, client, text, also_log=True):
        data = text.encode("utf-8") if isinstance(text, str) else text
        frame = self.frame(0x1, data)
        try:
            with self.lock:
                client.sock.settimeout(5)       # a dead peer must not hang us
                client.sock.sendall(frame)
                client.sock.settimeout(None)
        except OSError:
            self.drop(client)
            return False
        if also_log:
            log("[->]", text if isinstance(text, str) else repr(text))
        return True

    def send_raw_frame(self, client, opcode, payload=b"", fin=True):
        try:
            with self.lock:
                client.sock.settimeout(5)
                client.sock.sendall(self.frame(opcode, payload, fin))
                client.sock.settimeout(None)
        except OSError:
            self.drop(client)
            return False
        return True

    @staticmethod
    def frame(opcode, payload=b"", fin=True):
        """a server frame: never masked"""
        first = (0x80 if fin else 0) | (opcode & 0x0F)
        n = len(payload)
        if n < 126:
            hdr = bytes([first, n])
        elif n <= 0xFFFF:
            hdr = bytes([first, 126]) + int(n).to_bytes(2, "big")
        else:
            hdr = bytes([first, 127]) + int(n).to_bytes(8, "big")
        return hdr + payload

    def broadcast_text(self, text):
        for c in list(self.clients):
            if c.alive:
                self.send_text(c, text)

    def broadcast_frame(self, opcode, payload=b"", fin=True, note=""):
        for c in list(self.clients):
            if c.alive:
                self.send_raw_frame(c, opcode, payload, fin)
        log("[->]", note or f"frame op={opcode} len={len(payload)}")

    def drop(self, client):
        client.alive = False
        with self.lock:
            if client not in self.clients:
                return                       # already gone
            self.clients.remove(client)
        try:
            self.sel.unregister(client.sock)
        except (KeyError, ValueError):
            pass
        try:
            client.sock.close()
        except OSError:
            pass
        self.vlog("client gone", client.addr)

    # ---------- NIP-01 ----------

    @staticmethod
    def tag_value(ev, name):
        for t in ev.get("tags") or []:
            if isinstance(t, (list, tuple)) and len(t) > 1 and t[0] == name:
                return t[1]
        return None

    @classmethod
    def matches(cls, filters, ev):
        if not filters:
            return False
        for f in filters:
            if not isinstance(f, dict):
                continue
            ok = True
            if "kinds" in f and ev.get("kind") not in (f.get("kinds") or []):
                ok = False
            if ok:
                for key, want in f.items():
                    if not (isinstance(key, str) and key.startswith("#") and
                            len(key) == 2):
                        continue
                    got = cls.tag_value(ev, key[1])
                    if got not in (want or []):
                        ok = False
                        break
            if ok:
                return True
        return False

    @staticmethod
    def limit_of(filters):
        for f in filters:
            if isinstance(f, dict) and isinstance(f.get("limit"), int):
                return f["limit"]
        return None

    def handle_message(self, client, text):
        try:
            msg = json.loads(text)
        except json.JSONDecodeError:
            log("bad line from", client.addr, text[:80])
            return
        if not isinstance(msg, list) or not msg:
            self.vlog("bad message from", client.addr, text[:80])
            log("[nstr_relay] bad message")
            return
        verb = msg[0]
        if verb == "EVENT":
            self.on_event(client, msg)
        elif verb == "REQ":
            self.on_req(client, msg)
        elif verb == "CLOSE":
            if len(msg) > 1:
                with self.lock:
                    client.subs.pop(msg[1], None)
                self.vlog("close", msg[1], client.addr)
        else:
            self.vlog("unknown verb", verb, client.addr)

    def on_event(self, client, msg):
        ev = msg[1] if len(msg) > 1 else None
        if not isinstance(ev, dict):
            return
        eid = ev.get("id", "")
        with self.lock:
            dup = bool(eid) and eid in self.events
            if not dup:
                self.events[eid] = ev
        if dup:
            self.send_text(client, self.dump(
                ["OK", eid, True, "duplicate: already have this event"]))
            return
        self.send_text(client, self.dump(["OK", eid, True, ""]))
        # forwarded to every *other* client's matching subscription
        for c in list(self.clients):
            if c is client or not c.alive:
                continue
            for subid, filters in list(c.subs.items()):
                if self.matches(filters, ev):
                    self.send_text(c, self.dump(["EVENT", subid, ev]))
                    break

    def on_req(self, client, msg):
        if len(msg) < 2:
            return
        subid = msg[1]
        filters = [f for f in msg[2:] if isinstance(f, dict)]
        with self.lock:
            client.subs[subid] = filters      # a repeat replaces: NIP-01
        with self.lock:
            stored = list(self.events.values())
        lim = self.limit_of(filters)
        sent = 0
        for ev in stored:
            if not self.matches(filters, ev):
                continue
            if lim is not None and sent >= lim:
                break
            self.send_text(client, self.dump(["EVENT", subid, ev]))
            sent += 1
        self.send_text(client, self.dump(["EOSE", subid]))

    # ---------- reading ----------

    @staticmethod
    def parse_frame(buf):
        """(fin, opcode, payload, consumed) from the front of buf, or None
        while the frame there is still incomplete. Pure: nothing is eaten
        until the whole frame is in the buffer."""
        if len(buf) < 2:
            return None
        b0, b1 = buf[0], buf[1]
        fin = bool(b0 & 0x80)
        opcode = b0 & 0x0F
        masked = bool(b1 & 0x80)
        n = b1 & 0x7F
        at = 2
        if n == 126:
            if len(buf) < at + 2:
                return None
            n = int.from_bytes(buf[at:at + 2], "big")
            at += 2
        elif n == 127:
            if len(buf) < at + 8:
                return None
            n = int.from_bytes(buf[at:at + 8], "big")
            at += 8
        key = b""
        if masked:
            if len(buf) < at + 4:
                return None
            key = bytes(buf[at:at + 4])
            at += 4
        if len(buf) < at + n:
            return None
        payload = bytes(buf[at:at + n])
        at += n
        if masked:
            payload = bytes(b ^ key[i % 4] for i, b in enumerate(payload))
        return fin, opcode, payload, at

    @staticmethod
    def read_frame(client, timeout=0.5):
        """one frame, waiting up to timeout for it to arrive. None = nothing
        ready yet (a partial frame stays buffered); EOFError = peer gone.
        The socket stays blocking: waits go through select(), and a TLS
        socket's already-decrypted bytes are seen through pending()."""
        while True:
            parsed = Relay.parse_frame(client.buf)
            if parsed:
                fin, opcode, payload, consumed = parsed
                del client.buf[:consumed]
                return fin, opcode, payload
            ready = False
            if hasattr(client.sock, "pending") and client.sock.pending() > 0:
                ready = True
            else:
                try:
                    r, _, _ = select.select([client.sock], [], [], timeout)
                except (OSError, ValueError):
                    raise EOFError
                ready = bool(r)
            if not ready:
                return None
            try:
                chunk = client.sock.recv(BUF)
            except (BlockingIOError, socket.timeout, TimeoutError):
                return None
            if not chunk:
                raise EOFError
            client.buf += chunk

    def on_frame(self, client, fin, opcode, payload):
        if opcode == 0x8:                       # close: reply in kind
            self.send_raw_frame(client, 0x8, payload)
            raise EOFError
        if opcode == 0x9:                       # ping -> pong (logged)
            self.send_raw_frame(client, 0xA, payload)
            log("[<-] pong", repr(payload[:20]))
            return
        if opcode == 0xA:
            log("[<-] pong", repr(payload[:20]))
            return
        if opcode not in (0x0, 0x1):
            self.vlog("ignoring opcode", opcode)
            return

        frag = getattr(client, "_frag", None)
        if opcode == 0x1:                       # a new message starts
            frag = []
        if frag is None:
            frag = []
        frag.append(payload)
        if not fin:
            client._frag = frag                 # a fragment: wait for the rest
            return
        client._frag = None
        data = b"".join(frag)
        self.handle_message(client, data.decode("utf-8", "replace"))

    # ---------- the loop ----------

    def run(self):
        srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        srv.bind((self.host, self.port))
        srv.listen(16)
        srv.setblocking(False)
        log(f"nstr_relay listening on {self.host}:{self.port}"
            + (" (tls)" if self.tls_ctx else ""))
        self.sel.register(srv, selectors.EVENT_READ, None)

        def accept_all():
            while True:
                try:
                    conn, addr = srv.accept()
                except (BlockingIOError, OSError):
                    return
                conn.setblocking(False)
                if self.tls_ctx:
                    try:
                        conn.settimeout(5)
                        conn = self.tls_ctx.wrap_socket(conn, server_side=True)
                    except OSError as e:
                        self.vlog("tls accept failed", e)
                        try:
                            conn.close()
                        except OSError:
                            pass
                        continue
                conn.setblocking(True)
                try:
                    rest = server_handshake(conn, addr, self.vlog)
                except OSError as e:
                    self.vlog("handshake failed", addr, e)
                    try:
                        conn.close()
                    except OSError:
                        pass
                    continue
                conn.settimeout(None)           # reads go through select()
                c = Client(conn, addr)
                c.buf += rest
                with self.lock:
                    self.clients.append(c)
                try:
                    self.sel.register(conn, selectors.EVENT_READ, c)
                except OSError:
                    pass
                self.vlog("client in", addr)

        while self.running:
            accept_all()
            try:
                events = self.sel.select(timeout=0.2)
            except (OSError, ValueError):
                events = []
            for key, _ in events:
                c = key.data
                if c is None:
                    continue
                if self.paused:                 # real backpressure: not reading
                    continue
                try:
                    while True:
                        f = self.read_frame(c, timeout=0.0)
                        if f is None:
                            break
                        self.on_frame(c, *f)
                except EOFError:
                    self.drop(c)
                except (OSError, ValueError) as e:
                    self.vlog("read error", e)
                    self.drop(c)

    # ---------- the control plane (stdin) ----------

    def command(self, line):
        line = line.rstrip("\n")
        if not line.strip():
            return
        parts = line.split(" ", 2)
        cmd = parts[0]
        if cmd == "okfalse" and len(parts) >= 3:
            self.broadcast_text(self.dump(
                ["OK", parts[1], False, parts[2]]))
        elif cmd == "closed" and len(parts) >= 3:
            self.broadcast_text(self.dump(
                ["CLOSED", parts[1], parts[2]]))
        elif cmd == "raw" and len(parts) >= 2:
            self.broadcast_text(line.split(" ", 1)[1])
        elif cmd == "ping":
            self.broadcast_frame(0x9, b"ping", True, "-> ping")
        elif cmd == "fragment" and len(parts) >= 2:
            text = line.split(" ", 1)[1].encode()
            half = max(1, len(text) // 2)
            for c in list(self.clients):
                if not c.alive:
                    continue
                self.send_raw_frame(c, 0x1, text[:half], fin=False)
                self.send_raw_frame(c, 0x0, text[half:], fin=True)
            log("[->] fragment", repr(text))
        elif cmd == "oversize" and len(parts) >= 2:
            n = int(parts[1])
            payload = b"x" * n
            self.broadcast_frame(0x1, payload, True, f"-> oversize {n}")
        elif cmd == "pause":
            self.paused = True
            log("[nstr_relay] paused (not reading)")
        elif cmd == "resume":
            self.paused = False
            log("[nstr_relay] resumed")
        elif cmd == "quit":
            self.running = False
        else:
            log("[nstr_relay] unknown command:", line)

    def read_commands(self):
        for line in sys.stdin:
            try:
                self.command(line)
            except Exception as e:              # noqa: BLE001 - a bad command
                log("[nstr_relay] command failed:", e)   # ... must not kill it
        # stdin closed or at EOF: the harness keeps serving - the suite
        # terminates it (and `quit` is the manual off switch)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--host", default="127.0.0.1")
    ap.add_argument("--port", type=int, required=True)
    ap.add_argument("--tls", action="store_true", help="wrap the server in TLS")
    ap.add_argument("--cert", default=None, help="the certificate chain for --tls")
    ap.add_argument("--key", default=None, help="the private key for --tls")
    ap.add_argument("-v", "--verbose", action="store_true")
    args = ap.parse_args()

    ctx = None
    if args.tls:
        if not (args.cert and args.key):
            print("--tls needs --cert and --key", file=sys.stderr)
            return 2
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        ctx.load_cert_chain(args.cert, args.key)

    r = Relay(args.host, args.port, ctx, args.verbose)
    threading.Thread(target=r.read_commands, daemon=True).start()
    try:
        r.run()
    except KeyboardInterrupt:
        pass
    return 0


if __name__ == "__main__":
    sys.exit(main())
