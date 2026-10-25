#!/usr/bin/env python3
"""Bridge between the PDA's BIT carrier and a real bitchat node.

The PDA app speaks one JSON event per line (a signed NIP-01 event for chat,
unsigned legacy lines for discovery). This bridge translates that line into
real bitchat traffic and back, using the `bitchat` python
package (pip install bitchat) - so a PDA and a phone running the real bitchat
app can hold the same conversation over the BLE mesh.

    python3 tools/bitchat_bridge.py --socket /tmp/pda-bitchat.sock

The app then finds it with:

    PDA_BITCHAT_SOCK=/tmp/pda-bitchat.sock ./build/pda

Without the package (or without a bluetooth adapter) the bridge still runs and
still speaks the protocol, so the app's BIT path can be exercised end to end;
it just has no mesh behind it. The status line says which of the two it is.

The mesh side is a plain channel: every message goes out on the channel it
carries ("chan" in the event tags, which is the channel the PDA is set to -
"mesh" by default), which is what the real bitchat app calls a public
channel. --channel is the fallback for messages that come without one.
"""
import argparse
import hashlib
import json
import os
import socket
import sys
import threading
import time
import uuid

def tag_of(msg, name, default=""):
    for t in msg.get("tags") or []:
        if isinstance(t, (list, tuple)) and len(t) > 1 and t[0] == name:
            return t[1]
    return default


def unsigned_event(content, tags, created_at=None):
    """the unsigned event shape (pubkey "", no sig): the bridge holds no
    key, so what it builds is exactly what the app's identity-failure
    sends look like - the honest CHAT_SIG_NONE path on the receiver."""
    ts = int(created_at if created_at is not None else time.time())
    ser = json.dumps([0, "", ts, 1, tags, content],
                     separators=(",", ":"), ensure_ascii=False)
    eid = hashlib.sha256(ser.encode("utf-8")).hexdigest()
    return {"id": eid, "pubkey": "", "created_at": ts, "kind": 1,
            "tags": tags, "content": content}


try:
    import bitchat as bitchat_pkg
    HAVE_BITCHAT = True
except Exception:                                  # noqa: BLE001
    bitchat_pkg = None
    HAVE_BITCHAT = False


class Bridge:
    def __init__(self, sock_path, channel, peer_id, verbose=False):
        self.sock_path = sock_path
        self.channel = channel
        self.peer_id = peer_id
        self.verbose = verbose
        self.clients = []
        self.lock = threading.Lock()
        self.mesh_up = False
        self.mesh_note = "no mesh"

    def log(self, *a):
        if self.verbose:
            print("[bridge]", *a, file=sys.stderr, flush=True)

    # ---------- the app side ----------

    def serve(self):
        if os.path.exists(self.sock_path):
            os.unlink(self.sock_path)
        srv = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        srv.bind(self.sock_path)
        srv.listen(8)
        os.chmod(self.sock_path, 0o666)
        print(f"bridge listening on {self.sock_path}", flush=True)
        while True:
            conn, _ = srv.accept()
            t = threading.Thread(target=self.handle_client, args=(conn,), daemon=True)
            t.start()

    def handle_client(self, conn):
        self.log("app connected")
        buf = b""
        try:
            while True:
                data = conn.recv(8192)
                if not data:
                    break
                buf += data
                while b"\n" in buf:
                    line, buf = buf.split(b"\n", 1)
                    self.from_app(conn, line.decode("utf-8", "replace"))
        except OSError:
            pass
        finally:
            with self.lock:
                if conn in self.clients:
                    self.clients.remove(conn)
            try:
                conn.close()
            except OSError:
                pass
            self.log("app gone")

    def from_app(self, conn, line):
        line = line.strip()
        if not line:
            return
        try:
            msg = json.loads(line)
        except json.JSONDecodeError:
            return
        # a line with a t key is discovery, not chat (the carriers own that
        # dispatch); an event carries what it says in content, and its
        # channel in the chan tag
        if "t" in msg:
            return
        if msg.get("kind") != 1:
            return
        text = msg.get("content", "")
        if not text:
            return
        chan = tag_of(msg, "chan") or self.channel
        self.log("app ->", chan, text)
        self.to_mesh(msg, text, chan)

    def to_app(self, payload):
        data = (json.dumps(payload, separators=(",", ":")) + "\n").encode()
        with self.lock:
            dead = []
            for c in self.clients:
                try:
                    c.sendall(data)
                except OSError:
                    dead.append(c)
            for c in dead:
                self.clients.remove(c)

    # ---------- the mesh side ----------

    def to_mesh(self, msg, text, chan=None):
        if not HAVE_BITCHAT:
            self.log("no bitchat package - message dropped")
            return
        chan = chan or self.channel
        try:
            m = bitchat_pkg.BitchatMessage(
                id=msg.get("id", uuid.uuid4().hex[:8]),
                sender=tag_of(msg, "name") or "PDA",
                content=text,
                timestamp=time.time(),
                is_relay=False,
                original_sender=None,
                is_private=False,
                recipient_nickname=None,
                sender_peer_id=self.peer_id,
                mentions=[],
                channel=chan,
                encrypted_content=None,
                is_encrypted=False,
                delivery_status="pending",
            )
            if self.channel:
                bitchat_pkg.send_encrypted_channel_message(m, self.channel)
            else:
                bitchat_pkg.send_message(m)
            self.mesh_up = True
            self.mesh_note = "mesh up"
        except Exception as e:                      # noqa: BLE001
            self.mesh_up = False
            self.mesh_note = f"mesh down: {e}"
            self.log("mesh send failed:", e)

    def from_mesh(self):
        """Read the mesh and push whatever arrives into the app."""
        if not HAVE_BITCHAT:
            return
        while True:
            try:
                packet = bitchat_pkg.receive_packet()
                if packet is None:
                    time.sleep(0.5)
                    continue
                msg = bitchat_pkg.decode_message(packet.payload)
                if msg is None or not msg.content:
                    continue
                self.log("mesh ->", msg.sender, msg.content)
                # into the app as an unsigned event: the tags are what the
                # legacy fields were - name from the sender, room from the
                # channel, no chan tag (a missing chan passes the app's
                # filter, so behavior is unchanged). No signature: real
                # bitchat's own sender identity is out of slice A's scope.
                self.to_app(unsigned_event(
                    msg.content,
                    [["name", msg.sender], ["room", (self.channel or "").upper()]]))
            except Exception as e:                  # noqa: BLE001
                self.mesh_note = f"mesh down: {e}"
                self.log("mesh receive failed:", e)
                time.sleep(2)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--socket", default="/tmp/pda-bitchat.sock")
    ap.add_argument("--channel", default="mesh")
    ap.add_argument("--peer-id", default=None, help="48 hex chars; random if omitted")
    ap.add_argument("-v", "--verbose", action="store_true")
    args = ap.parse_args()

    peer = args.peer_id or uuid.uuid4().hex[:48]
    b = Bridge(args.socket, args.channel, peer, args.verbose)
    if not HAVE_BITCHAT:
        print("bitchat package not installed - the bridge speaks the protocol "
              "but has no mesh behind it (pip install bitchat)", flush=True)
    threading.Thread(target=b.from_mesh, daemon=True).start()
    b.serve()


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        pass
