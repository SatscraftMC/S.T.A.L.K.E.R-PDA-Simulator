#!/usr/bin/env python3
"""A stdlib-only WebSocket client for the PDA's test suites.

Handshake + frame codec + one message in / one message out, over ws:// or
wss:// (ssl.create_default_context(), so a real certificate check happens
on the way in). It is what every test peer speaks now that the internet
carrier is a relay connection, what make_screenshots.py injects with, and
what the live check's independent subscriber is built on.

    from wsclient import WSClient
    w = WSClient("ws://127.0.0.1:7788")
    w.send('["REQ","peer",{"kinds":[1]}]')
    print(w.recv(timeout=5))
"""
import base64
import hashlib
import os
import socket
import ssl
import struct

GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"


class WSError(Exception):
    pass


def parse_url(url):
    if url.startswith("wss://"):
        tls, rest = True, url[6:]
    elif url.startswith("ws://"):
        tls, rest = False, url[5:]
    else:
        raise WSError(f"bad url: {url}")
    if "?" in rest or "#" in rest:
        raise WSError(f"bad url: {url}")
    hostport, _, path = rest.partition("/")
    path = "/" + path
    host, port = hostport, 443 if tls else 80
    if hostport.startswith("["):
        end = hostport.find("]")
        if end < 0:
            raise WSError(f"bad url: {url}")
        host = hostport[1:end]
        rest2 = hostport[end + 1:]
        if rest2.startswith(":"):
            port = int(rest2[1:])
    elif ":" in hostport:
        host, p = hostport.split(":", 1)
        port = int(p)
    return tls, host, port, path


class WSClient:
    """One WebSocket connection. recv() returns one complete text message
    (fragments reassembled), answers pings with pongs itself, and returns
    None when the peer closes."""

    def __init__(self, url, timeout=5.0, cafile=None):
        """cafile: trust *only* that bundle for this connection - what the
        local TLS cases use, since the harness's self-signed certificate is
        deliberately not in the default store"""
        self.url = url
        self.tls, self.host, self.port, self.path = parse_url(url)
        self.buf = bytearray()
        self.sock = socket.create_connection((self.host, self.port), timeout=timeout)
        self.sock.settimeout(timeout)
        if self.tls:
            if cafile:
                ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
                ctx.load_verify_locations(cafile)
            else:
                ctx = ssl.create_default_context()
            self.sock = ctx.wrap_socket(self.sock, server_hostname=self.host)
        self._handshake()

    # ---------- the handshake ----------

    def _handshake(self):
        key = base64.b64encode(os.urandom(16)).decode()
        if ":" in self.host:
            hosthdr = f"[{self.host}]"
        else:
            hosthdr = self.host
        if self.port not in (443, 80):
            hosthdr = f"{hosthdr}:{self.port}"
        req = (f"GET {self.path} HTTP/1.1\r\n"
               f"Host: {hosthdr}\r\n"
               "Upgrade: websocket\r\n"
               "Connection: Upgrade\r\n"
               f"Sec-WebSocket-Key: {key}\r\n"
               "Sec-WebSocket-Version: 13\r\n\r\n")
        self.sock.sendall(req.encode())
        data = bytearray()
        while b"\r\n\r\n" not in data:
            chunk = self.sock.recv(1024)
            if not chunk:
                raise WSError("peer closed during the handshake")
            data += chunk
        head, _, rest = bytes(data).partition(b"\r\n\r\n")
        self.buf += rest
        text = head.decode("latin-1")
        first = text.split("\r\n", 1)[0]
        if " 101" not in first:
            raise WSError(f"no 101: {first!r}")
        want = base64.b64encode(
            hashlib.sha1((key + GUID).encode()).digest()).decode()
        got = ""
        for line in text.split("\r\n")[1:]:
            if ":" in line:
                n, _, v = line.partition(":")
                if n.strip().lower() == "sec-websocket-accept":
                    got = v.strip()
        if got != want:
            raise WSError("bad Sec-WebSocket-Accept")

    # ---------- frames ----------

    def _recv_exact(self, n):
        while len(self.buf) < n:
            chunk = self.sock.recv(4096)
            if not chunk:
                return False
            self.buf += chunk
        return True

    def send_frame(self, opcode, payload=b"", fin=True):
        if isinstance(payload, str):
            payload = payload.encode("utf-8")
        first = (0x80 if fin else 0) | (opcode & 0x0F)
        mask = 0x80                       # client frames are masked
        n = len(payload)
        if n < 126:
            hdr = bytes([first, mask | n])
        elif n <= 0xFFFF:
            hdr = bytes([first, mask | 126]) + struct.pack(">H", n)
        else:
            hdr = bytes([first, mask | 127]) + struct.pack(">Q", n)
        key = os.urandom(4)
        masked = bytes(b ^ key[i % 4] for i, b in enumerate(payload))
        self.sock.sendall(hdr + key + masked)

    def send(self, text):
        self.send_frame(0x1, text)

    def _read_frame(self):
        if not self._recv_exact(2):
            return None
        b0, b1 = self.buf[0], self.buf[1]
        del self.buf[:2]
        fin = bool(b0 & 0x80)
        opcode = b0 & 0x0F
        masked = bool(b1 & 0x80)
        n = b1 & 0x7F
        if n == 126:
            if not self._recv_exact(2):
                return None
            n = struct.unpack(">H", self.buf[:2])[0]
            del self.buf[:2]
        elif n == 127:
            if not self._recv_exact(8):
                return None
            n = struct.unpack(">Q", self.buf[:8])[0]
            del self.buf[:8]
        key = b""
        if masked:
            if not self._recv_exact(4):
                return None
            key = bytes(self.buf[:4])
            del self.buf[:4]
        if not self._recv_exact(n):
            return None
        payload = bytes(self.buf[:n])
        del self.buf[:n]
        if masked:
            payload = bytes(b ^ key[i % 4] for i, b in enumerate(payload))
        return fin, opcode, payload

    def recv(self, timeout=None):
        """One complete text message (or None when the peer closed)."""
        old = self.sock.gettimeout()
        if timeout is not None:
            self.sock.settimeout(timeout)
        parts = []
        try:
            while True:
                f = self._read_frame()
                if f is None:
                    return None
                fin, opcode, payload = f
                if opcode == 0x8:                      # close: answer in kind
                    try:
                        self.send_frame(0x8, payload)
                    except OSError:
                        pass
                    return None
                if opcode == 0x9:                      # ping -> pong
                    self.send_frame(0xA, payload)
                    continue
                if opcode == 0xA:                      # pong: consumed
                    continue
                if opcode == 0x2:
                    raise WSError("binary frame")
                parts.append(payload)
                if fin:
                    return b"".join(parts).decode("utf-8", "replace")
        except (socket.timeout, TimeoutError):
            return None
        finally:
            if timeout is not None:
                try:
                    self.sock.settimeout(old)
                except OSError:
                    pass

    def close(self):
        try:
            self.send_frame(0x8, b"")
        except OSError:
            pass
        try:
            self.sock.close()
        except OSError:
            pass


if __name__ == "__main__":
    import sys
    url = sys.argv[1]
    w = WSClient(url)
    for line in sys.argv[2:]:
        w.send(line)
        print(w.recv(timeout=5))
    w.close()
