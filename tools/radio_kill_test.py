#!/usr/bin/env python3
"""The music player must die with the app, on every exit path.

Covers the v0.4.3 fix: LVGL's SDL driver used to exit(0) from inside
lv_timer_handler when the window was closed, so the shutdown that kills the
player child never ran and gst-launch-1.0 kept playing.

  0. close the window (the X button) while music plays
  1. the same with the player paused (SIGSTOP - SIGTERM alone would hang)
  2. SIGINT (Ctrl+C)
  3. SIGKILL (the prctl PR_SET_PDEATHSIG path)
  4. a theme change, which rebuilds the panels while music plays

Run it with the ordinary build, or point it at another one with
    PDA_TEST_BUILD=/tmp/build-asan/pda python3 tools/radio_kill_test.py
"""
import os
import re
import signal
import subprocess
import sys
import time

from Xlib import display, X
from Xlib.ext import xtest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
APP = os.environ.get("PDA_TEST_BUILD", os.path.join(ROOT, "build", "pda"))
LOGDIR = "/tmp/opencode/radiocheck"
os.makedirs(LOGDIR, exist_ok=True)

PLAY_X, PLAY_Y = 610, 220      # the Play plate, window coordinates
PAUSE_X, PAUSE_Y = 547, 220    # the Pause plate, window coordinates

FAIL = []


def sh(cmd):
    return subprocess.run(cmd, shell=True, capture_output=True, text=True).stdout


def note(ok, msg):
    print(("  PASS  " if ok else "  FAIL  ") + msg, flush=True)
    if not ok:
        FAIL.append(msg)
    return ok


def gst_alive():
    # -x on the process name only: a plain -f pattern matches this script's
    # own command line (and every shell that ever mentioned gst-launch)
    out = sh("pgrep -ax gst-launch-1.0 || true")
    return [l for l in out.splitlines() if l.strip()]


def win_origin():
    tree = sh("xwininfo -root -tree")
    for line in tree.splitlines():
        if '"PDA // Simulator' in line:
            wid = line.strip().split()[0]
            info = sh(f"xwininfo -id {wid}")
            m = re.search(r"Absolute upper-left X:\s+(-?\d+)", info)
            n = re.search(r"Absolute upper-left Y:\s+(-?\d+)", info)
            if m and n:
                return int(m.group(1)), int(n.group(1))
    return None


def click(sx, sy):
    d = display.Display()
    xtest.fake_input(d, X.MotionNotify, x=sx, y=sy)
    d.sync()
    time.sleep(0.08)
    xtest.fake_input(d, X.ButtonPress, 1)
    d.sync()
    time.sleep(0.08)
    xtest.fake_input(d, X.ButtonRelease, 1)
    d.sync()
    time.sleep(0.3)


def start(env_extra, name):
    env = dict(os.environ)
    env.setdefault("DISPLAY", ":0")
    env.update(env_extra)
    path = f"{LOGDIR}/{name}.log"
    f = open(path, "w")
    p = subprocess.Popen([APP], stdout=f, stderr=subprocess.STDOUT, env=env, cwd=ROOT)
    return p, f, path


def play_and_wait(p):
    """start the app on the Radio tab, click Play, return True if playing"""
    subprocess.run(["wmctrl", "-a", "PDA // Simulator"], capture_output=True)
    time.sleep(0.4)
    o = win_origin()
    if not o:
        return False
    click(o[0] + PLAY_X, o[1] + PLAY_Y)
    for _ in range(20):
        if gst_alive() and p.poll() is None:
            return True
        time.sleep(0.25)
    return False


def wait_gone(timeout=5.0):
    t = time.time()
    while time.time() - t < timeout:
        if not gst_alive():
            return True
        time.sleep(0.2)
    return False


BASE = {
    "PDA_START_TAB": "3",
    "PDA_RADIO_DIR": os.path.join(ROOT, "music", "default"),
}

print("0. window close (the X button)", flush=True)
p, f, log = start(BASE, "close")
time.sleep(6)
try:
    playing = play_and_wait(p)
    note(playing, "music is playing before the window is closed")
    subprocess.run(["wmctrl", "-c", "PDA // Simulator"], capture_output=True)
    time.sleep(4)
    note(p.poll() is not None, f"the app exited on window close (rc={p.poll()})")
    note(not gst_alive(), f"the player is gone too ({gst_alive()})")
finally:
    if p.poll() is None:
        p.kill()
    f.close()

print("1. window close while PAUSED", flush=True)
p, f, log = start(BASE, "pauseclose")
time.sleep(6)
try:
    playing = play_and_wait(p)
    note(playing, "music is playing before the pause")
    o = win_origin()
    click(o[0] + PAUSE_X, o[1] + PAUSE_Y)      # SIGSTOP the player
    time.sleep(1.0)
    stopped = sh("pgrep -x gst-launch-1.0 | head -1").strip()
    if stopped:
        pid = int(stopped)
        st = sh(f"ps -o stat= -p {pid}").strip()
        note(st.startswith("T"), f"the player really is stopped (stat={st})")
    subprocess.run(["wmctrl", "-c", "PDA // Simulator"], capture_output=True)
    time.sleep(4)
    note(p.poll() is not None, f"the app exited while the player was paused (rc={p.poll()})")
    note(not gst_alive(), f"the paused player is gone too ({gst_alive()})")
finally:
    if p.poll() is None:
        p.kill()
    f.close()

print("2. SIGINT (Ctrl+C)", flush=True)
p, f, log = start(BASE, "sigint")
time.sleep(6)
try:
    note(play_and_wait(p), "music is playing before SIGINT")
    p.send_signal(signal.SIGINT)
    time.sleep(3)
    note(p.poll() is not None, f"the app exited on SIGINT (rc={p.poll()})")
    note(not gst_alive(), f"the player is gone too ({gst_alive()})")
finally:
    if p.poll() is None:
        p.kill()
    f.close()

print("3. SIGKILL (crash / kill -9, the PDEATHSIG path)", flush=True)
p, f, log = start(BASE, "sigkill")
time.sleep(6)
try:
    note(play_and_wait(p), "music is playing before SIGKILL")
    p.send_signal(signal.SIGKILL)
    time.sleep(3)
    note(p.poll() is not None, "the app is gone")
    note(not gst_alive(),
         f"the player died with the app anyway ({gst_alive()})")
finally:
    if p.poll() is None:
        p.kill()
    f.close()

print("4. retheme (theme change rebuilds the panels) with music playing", flush=True)
p, f, log = start(BASE, "retheme")
time.sleep(6)
try:
    note(play_and_wait(p), "music is playing before the theme change")
    def wclick(cx, cy):
        o = win_origin()
        click(o[0] + cx, o[1] + cy)
    wclick(667, 22)       # the Other tab
    time.sleep(0.6)
    wclick(236, 152)      # Settings > Themes
    time.sleep(0.6)
    wclick(599, 112)      # the Colour scheme value (cycles, then rethemes)
    time.sleep(3)
    note(p.poll() is None, f"the app survived the theme change (rc={p.poll()})")
    note(not gst_alive(),
         f"the player was stopped by the rebuild, no orphan ({gst_alive()})")
    out = sh(f"grep -E 'scheme' {os.path.expanduser('~/.config/pda/prefs.conf')}")
    note(bool(out.strip()), f"the setting was cycled ({out.strip()!r})")
    bad = sh(f"grep -E 'Segmentation|SIGSEGV|assert' {log}")
    note(not bad.strip(), "no crash in the log")
finally:
    if p.poll() is None:
        p.kill()
    f.close()

print()
if FAIL:
    print(f"{len(FAIL)} check(s) failed")
    for x in FAIL:
        print("  -", x)
    sys.exit(1)
print("all checks passed")
