#!/usr/bin/env bash
# The local mesh over a real network link.
#
# The other tests drive the mesh over the loopback, where everything is
# allowed in. This one puts a second device on a real link (a veth pair and
# a network namespace - the "other house" as far as the kernel is concerned)
# and checks the two things that only show up on a real link:
#
#   1. the sweep finds the other device and the message goes straight to it
#   2. what comes in is dropped by the firewall unless the port is open
#
# It needs root (a network namespace is a root thing):
#
#   sudo tools/mesh_netns_test.sh
#
set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
APP="$ROOT/build/pda"
NS=pdatest
VETH=pdat0
HOST_IP=10.99.0.1
PEER_IP=10.99.0.2
WORK=/tmp/opencode
FAIL=0

note() {
    if [ "$1" = "0" ]; then echo "  PASS  $2"; else echo "  FAIL  $2"; FAIL=$((FAIL+1)); fi
}

if [ "$(id -u)" != "0" ]; then
    echo "this test needs root: sudo $0"
    exit 1
fi
if [ ! -x "$APP" ]; then
    echo "build the app first: ./build.sh"
    exit 1
fi

cleanup() {
    [ -n "${APP_PID:-}" ] && kill "$APP_PID" 2>/dev/null
    [ -n "${PEER_PID:-}" ] && kill "$PEER_PID" 2>/dev/null
    ip netns del "$NS" 2>/dev/null
    ip link del "$VETH" 2>/dev/null
}
trap cleanup EXIT

echo "the mesh over a real link"

# --- the second device, on the other end of the veth ---
ip netns del "$NS" 2>/dev/null
ip link del "$VETH" 2>/dev/null
ip netns add "$NS"
ip link add "$VETH" type veth peer name pdat1
ip link set pdat1 netns "$NS"
ip addr add "$HOST_IP/24" dev "$VETH"
ip link set "$VETH" up
ip netns exec "$NS" ip addr add "$PEER_IP/24" dev pdat1
ip netns exec "$NS" ip link set pdat1 up
ip netns exec "$NS" ip link set lo up
ip netns exec "$NS" ip route add default via "$HOST_IP"

cat > "$WORK/ns_peer.py" <<'PY'
import json, socket, sys, time
log = open(sys.argv[1], "w")
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
s.bind((sys.argv[2], int(sys.argv[3])))
s.settimeout(1.0)
end = time.time() + float(sys.argv[4])
while time.time() < end:
    try:
        data, addr = s.recvfrom(4096)
    except socket.timeout:
        continue
    log.write(f"FROM {addr[0]}:{addr[1]}  {data.decode('utf-8','replace').strip()}\n")
    log.flush()
    try:
        m = json.loads(data.decode("utf-8", "replace").strip())
    except Exception:
        continue
    if m.get("t") == "hi":
        s.sendto(json.dumps({"v":1,"t":"hi-ack","from":"LAPTOP","port":int(sys.argv[3]),
                             "ts":int(time.time())}).encode(), addr)
PY

cat > "$WORK/ns_send.py" <<'PY'
import json, socket, sys, time
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
# an ephemeral port: the peer of this script holds the mesh port already.
# The envelope says which port the device listens on - that is what the
# mesh uses to talk to it.
# the unsigned event shape: no key here, so pubkey "" and no sig - the
# name tag carries what the legacy "from" field used to
import hashlib
ts = int(time.time())
text = "hello from the other house"
tags = [["room", "SECTOR"], ["name", "LAPTOP"]]
ser = json.dumps([0, "", ts, 1, tags, text], separators=(",", ":"), ensure_ascii=False)
eid = hashlib.sha256(ser.encode("utf-8")).hexdigest()
s.sendto((json.dumps({"id": eid, "pubkey": "", "created_at": ts, "kind": 1,
                      "tags": tags, "content": text}) +
          "").encode(), (sys.argv[2], 2425))
PY

rm -f "$WORK/ns_peer.log" "$WORK/ns_app.log"
# isolation: no run writes the real ~/.config/pda/ (the key and contacts
# would land there on the first run otherwise)
export PDA_PREFS="${PDA_PREFS:-$WORK/prefs.conf}"
export DISPLAY="${DISPLAY:-:0}" PDA_DEBUG=1 PDA_NAME=PC \
       PDA_TEST_SEND="hello from the PDA" PDA_TEST_SEND_IN=15
"$APP" > "$WORK/ns_app.log" 2>&1 &
APP_PID=$!
ip netns exec "$NS" python3 "$WORK/ns_peer.py" "$WORK/ns_peer.log" "$PEER_IP" 2425 40 &
PEER_PID=$!
sleep 6

# --- 1. the sweep finds the other device and the message goes to it ---
# The peer listens on its own address (10.99.0.2), not on the wildcard:
# a socket bound to a unicast address never gets the broadcast of the
# subnet. So a message that arrives there can only have come the direct way.
sleep 14
grep -q "peer LAPTOP at $PEER_IP" "$WORK/ns_app.log"
note $? "the sweep found the other device on the link"
grep -q "hello from the PDA" "$WORK/ns_peer.log"
note $? "the message went straight to the peer that was found (no broadcast reaches it)"

# --- 2. what comes in is dropped unless the port is open ---
if ufw status 2>/dev/null | grep -q "2425"; then
    echo "  note  the firewall holds 2425 - the rule is taken away and put back"
    ufw delete allow 2425/udp >/dev/null 2>&1
    ip netns exec "$NS" python3 "$WORK/ns_send.py" "$PEER_IP" "$HOST_IP"
    sleep 2
    ! grep -q "hello from the other house" "$WORK/ns_app.log"
    note $? "without the port open the message does not get in"
    ufw allow 2425/udp comment 'PDA local mesh (BIT)' >/dev/null 2>&1
    ip netns exec "$NS" python3 "$WORK/ns_send.py" "$PEER_IP" "$HOST_IP"
    sleep 2
    grep -q "hello from the other house" "$WORK/ns_app.log"
    note $? "with the port open the message comes in"
else
    ip netns exec "$NS" python3 "$WORK/ns_send.py" "$PEER_IP" "$HOST_IP"
    sleep 2
    grep -q "hello from the other house" "$WORK/ns_app.log"
    note $? "the message from the other device came in"
fi

echo
if [ "$FAIL" = "0" ]; then
    echo "all checks passed"
    exit 0
fi
echo "$FAIL check(s) failed"
exit 1
