#!/usr/bin/env python3
"""A mock of the fips daemon's native datagram API.

It speaks the same line protocol as the real daemon - one JSON object per
line on a unix stream socket, the flow and listener descriptors handed over
as ancillary data (SCM_RIGHTS) - so the PDA's FIPS client can be exercised
without a mesh.

    python3 tools/fips_mock.py --api /tmp/fips-api.sock --ctl /tmp/fips-ctl.sock

It then does what a node on the mesh would do: announce a peer to the
listener and talk over the flow.
"""
import argparse
import hashlib
import json
import os
import socket
import struct
import sys
import threading
import time

SELF = "npub1qm3k8vx7n2p5d0r9g4t6y1u3i8o7a2s5f4c3d2e1b0a9z8y7x6w5v4u3t2s1r0q9p"
PEER = "npub1qc3k8vx7n2p5d0r9g4t6y1u3i8o7a2s5f4c3d2e1b0a9z8y7x6w5v4u3t2s1r0q9p"


class MockFips:
    def __init__(self, api_path, ctl_path, peer=PEER, verbose=False):
        self.api_path = api_path
        self.ctl_path = ctl_path
        self.peer = peer
        self.verbose = verbose
        self.listeners = []
        self.flows = []          # (peer, port, socket)
        self.lock = threading.Lock()

    def log(self, *a):
        if self.verbose:
            print("[fips-mock]", *a, file=sys.stderr, flush=True)

    # ---------- the api socket ----------

    def serve_api(self):
        for p in (self.api_path, self.ctl_path):
            if os.path.exists(p):
                os.unlink(p)
        srv = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        srv.bind(self.api_path)
        srv.listen(8)
        ctl = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        ctl.bind(self.ctl_path)
        ctl.listen(4)
        threading.Thread(target=self.accept_loop, args=(srv, self.handle_setup),
                         daemon=True).start()
        threading.Thread(target=self.accept_loop, args=(ctl, self.handle_control),
                         daemon=True).start()
        print(f"fips mock: api {self.api_path}, control {self.ctl_path}", flush=True)

    def accept_loop(self, srv, handler):
        while True:
            conn, _ = srv.accept()
            threading.Thread(target=handler, args=(conn,), daemon=True).start()

    def handle_setup(self, conn):
        buf = b""
        while True:
            try:
                data = conn.recv(4096)
            except OSError:
                break
            if not data:
                break
            buf += data
            while b"\n" in buf:
                line, buf = buf.split(b"\n", 1)
                try:
                    cmd = json.loads(line)
                except json.JSONDecodeError:
                    continue
                self.log("setup:", cmd)
                self.do_setup(conn, cmd)
        try:
            conn.close()
        except OSError:
            pass

    def do_setup(self, conn, cmd):
        name = cmd.get("command", "")
        params = cmd.get("params") or {}
        if name == "listen":
            local = params.get("local_port", 0)
            keep, give = socket.socketpair(socket.AF_UNIX, socket.SOCK_SEQPACKET)
            with self.lock:
                self.listeners.append(keep)
            reply = {"status": "ok", "data": {"local_port": local, "node": SELF,
                                             "backlog": 16}}
            self.reply(conn, reply, give)
        elif name == "connect":
            peer = params.get("peer", "")
            remote = params.get("remote_port", 0)
            keep, give = socket.socketpair(socket.AF_UNIX, socket.SOCK_SEQPACKET)
            with self.lock:
                self.flows.append((peer, remote, keep))
            reply = {"status": "ok", "data": {"flow_id": len(self.flows), "local_port": 4600,
                                             "remote_port": remote, "peer": peer,
                                             "node": SELF, "max_payload": 1200}}
            self.reply(conn, reply, give)
        else:
            reply = {"status": "error", "data": {"errno": "ECONNREFUSED"},
                     "message": f"command {name} not understood"}
            self.reply(conn, reply, None)

    def reply(self, conn, reply, fd):
        data = (json.dumps(reply) + "\n").encode()
        try:
            if fd is not None:
                anc = [(socket.SOL_SOCKET, socket.SCM_RIGHTS,
                        struct.pack("i", fd.fileno()))]
                conn.sendmsg([data], anc)
                fd.close()          # the receiver has its own copy now
            else:
                conn.sendall(data)
        except OSError as e:
            self.log("reply failed:", e)

    # ---------- the control socket ----------

    def handle_control(self, conn):
        buf = b""
        while True:
            try:
                data = conn.recv(4096)
            except OSError:
                break
            if not data:
                break
            buf += data
            while b"\n" in buf:
                line, buf = buf.split(b"\n", 1)
                try:
                    cmd = json.loads(line)
                except json.JSONDecodeError:
                    continue
                name = cmd.get("command", "")
                if name == "show_peers":
                    reply = {"status": "ok", "data": {"peers": [
                        {"npub": self.peer, "node_addr": "a1b2c3d4",
                         "display_name": "ROOK", "transport_type": "udp"}]}}
                elif name == "show_status":
                    reply = {"status": "ok", "data": {"npub": SELF, "peer_count": 1,
                                                      "state": "ready"}}
                else:
                    reply = {"status": "error", "message": "unknown command"}
                try:
                    conn.sendall((json.dumps(reply) + "\n").encode())
                except OSError:
                    return

    # ---------- playing the peer ----------

    def announce(self, peer=None, port=4600):
        """announce an arriving flow on the listener, the way the node does"""
        peer = peer or self.peer
        arrival = {"flow_id": 1, "peer": peer, "node": SELF, "local_port": 4600,
                   "remote_port": port, "max_payload": 1200, "held": 0}
        with self.lock:
            listeners = list(self.listeners)
        for l in listeners:
            try:
                l.send(json.dumps(arrival).encode())   # no newline: the
                self.log("announced", peer)            # boundary is the message
            except OSError as e:
                self.log("announce failed:", e)

    @staticmethod
    def event(content, tags, created_at=None):
        """the unsigned event shape: pubkey "", no sig - hashing needs no
        key. Serialization matches the app's (compact, UTF-8 verbatim), so
        the id is the SHA-256 of exactly what the receiver will rebuild."""
        ts = int(created_at if created_at is not None else time.time())
        ser = json.dumps([0, "", ts, 1, tags, content],
                         separators=(",", ":"), ensure_ascii=False)
        eid = hashlib.sha256(ser.encode("utf-8")).hexdigest()
        return {"id": eid, "pubkey": "", "created_at": ts, "kind": 1,
                "tags": tags, "content": content}

    def send_to_peer(self, text, peer=None, sender="ROOK"):
        """write a message onto the flow, the way a peer would"""
        peer = peer or self.peer
        with self.lock:
            flows = list(self.flows)
        target = None
        for p, port, s in flows:
            if p == peer:
                target = s
                break
        if target is None:
            self.log("no flow for", peer)
            return False
        payload = json.dumps(self.event(text, [["name", sender]])).encode()
        target.send(payload)
        self.log("sent to peer:", text)
        return True

    def read_from_peer(self, timeout=5):
        """read what the client wrote onto the flow"""
        with self.lock:
            flows = list(self.flows)
        if not flows:
            return None
        s = flows[-1][2]
        s.settimeout(timeout)
        try:
            data = s.recv(4096)
        except (socket.timeout, OSError):
            return None
        # a NIP-01 line puts content a couple of hundred bytes in: read the
        # content out instead of a fixed window, so chat_test's assertion
        # about the message text can find it
        try:
            m = json.loads(data)
            shown = m.get("content") or m.get("text") or ""
        except json.JSONDecodeError:
            shown = ""
        self.log("got from client:", shown if shown else data[:400])
        return data


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--api", default="/tmp/fips-api.sock")
    ap.add_argument("--ctl", default="/tmp/fips-ctl.sock")
    ap.add_argument("--peer", default=PEER)
    ap.add_argument("-v", "--verbose", action="store_true")
    args = ap.parse_args()
    m = MockFips(args.api, args.ctl, args.peer, args.verbose)
    m.serve_api()
    threading.Thread(target=m.wait_and_act, daemon=True).start()
    while True:
        # log whatever the client writes onto the flow
        m.read_from_peer(timeout=0.3)
        time.sleep(0.3)


def _wait_and_act(self):
    """once a client has a listener, act like a peer on the mesh"""
    time.sleep(2)
    self.announce()
    time.sleep(1)
    self.send_to_peer("anyone got the drop coords")


MockFips.wait_and_act = _wait_and_act


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        pass
