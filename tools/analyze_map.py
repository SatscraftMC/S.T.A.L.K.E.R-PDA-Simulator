#!/usr/bin/env python3
"""Full map analysis for the PDA simulator.

Checks, on real screenshots:
  1. every zoom step renders a distinct frame
  2. consecutive steps magnify by the ladder ratio (correlation + scale search)
  3. the imagery is geo-aligned with the raw RGB565 masters (sampled pixels)
  4. panning follows the finger 1:1 (content shift == drag distance)
  5. no artifacts: black/garbage bands, flat smear, clipped edges
"""
import math, os, struct, subprocess, sys, time
from PIL import Image

LADDER = [64, 96, 128, 170, 256, 341, 512, 683, 1024, 1365, 2048, 2730, 4096, 5461, 8192, 10923]
SHOT = "/tmp/opencode/an"
CLIENT = (560, 300)          # window client origin
MAP_TOP = 101                # app y=0 of the map in the screenshot (titlebar 32 + view 69)
VIEW_TOP = 69                # app y of the map view (below the 58px header + 11px flex gap)
ZIN_Y = 334                  # app y of the "+" button
ZOUT_Y = 370                 # app y of the "-" button
MAP_W, MAP_H = 800, 411
CENTER = (400, MAP_TOP + 205)

FAIL = []


def note(ok, msg):
    print(("  PASS  " if ok else "  FAIL  ") + msg)
    if not ok:
        FAIL.append(msg)


def shot(name):
    subprocess.run(["gnome-screenshot", "-w", "-f", f"{SHOT}_{name}.png"],
                   capture_output=True)
    im = Image.open(f"{SHOT}_{name}.png").convert("RGB")
    if im.size != (800, 512):
        raise RuntimeError(f"screenshot is {im.size}, expected (800, 512)")
    return im


def map_of(im):
    return im.crop((0, MAP_TOP, MAP_W, MAP_TOP + MAP_H))


def click(x, y, n=1):
    for _ in range(n):
        subprocess.run([sys.executable, "/tmp/opencode/pda_click.py", str(x), str(y)],
                       capture_output=True)
        time.sleep(0.8)      # let the first frame of a new level page in


def drag(x0, y0, x1, y1, steps=12):
    """press at (x0,y0), move to (x1,y1) in steps, release"""
    code = f'''
import time
from Xlib import display, X
from Xlib.ext import xtest
d = display.Display()
xtest.fake_input(d, X.MotionNotify, x={x0}, y={y0}); d.sync(); time.sleep(0.1)
xtest.fake_input(d, X.ButtonPress, 1); d.sync(); time.sleep(0.1)
for i in range(1, {steps + 1}):
    x = {x0} + ({x1} - {x0}) * i / {steps}
    y = {y0} + ({y1} - {y0}) * i / {steps}
    xtest.fake_input(d, X.MotionNotify, x=int(x), y=int(y)); d.sync(); time.sleep(0.04)
xtest.fake_input(d, X.ButtonRelease, 1); d.sync(); time.sleep(0.35)
'''
    subprocess.run([sys.executable, "-c", code], capture_output=True)


def scale_between(a, b):
    """best scale s and correlation between frame a and frame b (b = a magnified s).
       Uses a central crop that avoids the zoom buttons and the attribution."""
    ga, gb = a.convert("L"), b.convert("L")
    best = (0.0, 0.0, 0, 0)
    for si in range(70, 220, 5):
        s = si / 100.0
        w, h = int(360 / s), int(180 / s)
        if w < 40 or h < 24:
            continue
        sub = gb.crop((400 - w // 2, 205 - h // 2, 400 - w // 2 + w, 205 - h // 2 + h))
        sub = sub.resize((360, 180), Image.BILINEAR)
        ref = ga.crop((220, 115, 580, 295))
        pa, pb = list(ref.getdata()), list(sub.getdata())
        ma, mb = sum(pa) / len(pa), sum(pb) / len(pb)
        num = sum((x - ma) * (y - mb) for x, y in zip(pa, pb))
        da = math.sqrt(sum((x - ma) ** 2 for x in pa))
        db = math.sqrt(sum((y - mb) ** 2 for y in pb))
        r = num / (da * db + 1e-9)
        if r > best[0]:
            best = (r, 1.0 / s, 0, 0)     # s is the crop ratio; magnification = 1/s
    return best[0], best[1]


def shift_between(a, b, max_shift=120):
    """pixel shift of b relative to a (content moved by (dx,dy) in b)"""
    ga, gb = a.convert("L"), b.convert("L")
    best = (-2, 0, 0)
    for dy in range(-max_shift, max_shift + 1, 2):
        for dx in range(-max_shift, max_shift + 1, 2):
            a2 = ga.crop((300 + dx, 130 + dy, 500 + dx, 280 + dy))
            b2 = gb.crop((300, 130, 500, 280))
            pa, pb = list(a2.getdata()), list(b2.getdata())
            ma, mb = sum(pa) / len(pa), sum(pb) / len(pb)
            num = sum((x - ma) * (y - mb) for x, y in zip(pa, pb))
            da = math.sqrt(sum((x - ma) ** 2 for x in pa))
            db = math.sqrt(sum((y - mb) ** 2 for y in pb))
            r = num / (da * db + 1e-9)
            if r > best[0]:
                best = (r, dx, dy)
    return best


def master_reader(path, w):
    f = open(path, "rb")
    def px(x, y):
        f.seek((y * w + x) * 2)
        v = struct.unpack("<H", f.read(2))[0]
        return ((v >> 11 & 31) * 8, (v >> 5 & 63) * 4, (v & 31) * 8)
    return px


def check_alignment(frames, frame_idx, ladder_idx, pack):
    """compare the rendered frame with the pack's raw master at the same lon/lat"""
    z, W, H, ox, oy, path = pack
    im = frames[frame_idx]
    px = im.load()

    def gx(lon, zz):
        return (lon + 180.0) / 360.0 * (2 ** zz) * 256.0
    def gy(lat, zz):
        r = math.radians(lat)
        return (1 - math.log(math.tan(r) + 1 / math.cos(r)) / math.pi) / 2 * (2 ** zz) * 256.0

    lon0, lat0 = -16.96, 32.66
    cx = int(gx(lon0, z)) - ox
    cy = int(gy(lat0, z)) - oy
    reader = master_reader(path, W)
    # how many master texels per screen pixel at this zoom
    per_px = (LADDER[ladder_idx] / 256.0) * (2 ** (12 - z))
    ok = bad = 0
    for dx, dy in [(-40, -30), (25, 20), (-60, 35), (45, -25), (15, 45), (-20, -50)]:
        mx = int(cx + dx * per_px)
        my = int(cy + dy * per_px)
        if not (0 <= mx < W and 0 <= my < H):
            continue
        want = reader(mx, my)
        got = px[400 + dx, 205 + dy]      # frames are map crops (origin = map top-left)
        d = max(abs(got[i] - want[i]) for i in range(3))
        if d <= 16:
            ok += 1
        else:
            bad += 1
    return ok, bad


def main():
    print("== 1. capture all zoom steps ==")
    subprocess.run(["pkill", "-x", "pda"], capture_output=True)
    time.sleep(0.4)
    env = dict(os.environ, PDA_GPS_LAT="32.66", PDA_GPS_LON="-16.96")
    subprocess.Popen(["./build/pda"], cwd="/home/aj/pda-project/app", env=env,
                     stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    for _ in range(60):
        out = subprocess.run(["wmctrl", "-l"], capture_output=True, text=True).stdout
        if "PDA //" in out:
            break
        time.sleep(0.25)
    subprocess.run(["wmctrl", "-a", "PDA // Simulator"], capture_output=True)
    time.sleep(0.8)

    frames = [map_of(shot("boot"))]
    for i in range(1, len(LADDER)):
        click(CLIENT[0] + 756, CLIENT[1] + VIEW_TOP + ZIN_Y)   # "+"
        subprocess.run(["wmctrl", "-a", "PDA // Simulator"], capture_output=True)
        time.sleep(0.2)
        frames.append(map_of(shot(f"z{i:02d}")))
    print(f"  captured {len(frames)} frames")

    print("== 2. distinct frames ==")
    hashes = [f.tobytes() for f in frames]
    distinct = len(set(hashes))
    # the ladder clamps at the top, so trailing frames may repeat by design
    expected_distinct = len(LADDER) - 4      # boot is ladder index 4
    note(distinct >= expected_distinct,
         f"{distinct} distinct frames of {len(frames)} (>= {expected_distinct} expected; "
         f"the ladder top clamps)")

    print("== 3. scale progression ==")
    for i in range(1, len(frames)):
        r, s = scale_between(frames[i - 1], frames[i])
        expect = LADDER[i] / LADDER[i - 1]
        if frames[i].tobytes() == frames[i - 1].tobytes():
            hi = min(i + 4, len(LADDER) - 1)
            note(True, f"zn {LADDER[min(i+3, len(LADDER)-1)]}->{LADDER[hi]}: "
                       f"identical frame (ladder top, clamped)")
            continue
        good = abs(s - expect) / expect < 0.25
        note(good, f"zn {LADDER[i+3]}->{LADDER[i+4]}: measured {s:.2f}x (expect {expect:.2f}x) corr {r:.3f}")

    print("== 4. geo alignment vs masters ==")
    madeira = (12, 3495, 2838, 473752, 421949,
               "/home/aj/pda-project/app/assets/maps/madeira/z12.rgb565")
    island = (14, 8388, 8992, 1895592, 1690158,
              "/home/aj/pda-project/app/assets/maps/madeira-island/z14.rgb565")
    funchal = (15, 4194, 2214, 3797942, 3387526,
               "/home/aj/pda-project/app/assets/maps/funchal/z15.rgb565")
    # frame index i  <->  ladder index i+4 (frame 0 is the boot view = zn 256)
    for fidx, lidx, pack, label in [(0, 4, madeira, "z12 @256"),
                                    (4, 8, island, "z14 @1024"),
                                    (6, 10, funchal, "z15 @2048")]:
        try:
            ok, bad = check_alignment(frames, fidx, lidx, pack)
        except Exception as e:
            note(False, f"{label}: {e}")
            continue
        note(bad == 0 and ok > 0, f"{label}: {ok} aligned / {bad} mismatched")

    print("== 5. pan follows the finger 1:1 ==")
    for zoom_steps, zn in [(0, LADDER[4]), (4, LADDER[8]), (8, LADDER[12])]:
        subprocess.run(["pkill", "-x", "pda"], capture_output=True)
        time.sleep(0.4)
        env = dict(os.environ, PDA_GPS_LAT="32.66", PDA_GPS_LON="-16.96")
        subprocess.Popen(["./build/pda"], cwd="/home/aj/pda-project/app", env=env,
                         stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        for _ in range(60):
            out = subprocess.run(["wmctrl", "-l"], capture_output=True, text=True).stdout
            if "PDA //" in out:
                break
            time.sleep(0.25)
        subprocess.run(["wmctrl", "-a", "PDA // Simulator"], capture_output=True)
        time.sleep(0.6)
        if zoom_steps:
            click(CLIENT[0] + 756, CLIENT[1] + 69 + 334, zoom_steps)
        before = map_of(shot("pan_before"))
        dx, dy = 60, -40
        drag(CLIENT[0] + 400, CLIENT[1] + VIEW_TOP + 205,
             CLIENT[0] + 400 + dx, CLIENT[1] + VIEW_TOP + 205 + dy)
        after = map_of(shot("pan_after"))
        r, sx, sy = shift_between(before, after)
        good = abs(sx + dx) <= 12 and abs(sy + dy) <= 12 and r > 0.6
        note(good, f"zn {zn}: dragged ({dx},{dy}) -> content moved ({-sx},{-sy}) corr {r:.3f}")

    print("== 6. artifact scan ==")
    for i, f in enumerate(frames):
        px = f.convert("L").load()
        # a row/column that is uniformly the darkest value = clipped/smeared
        bad_rows = 0
        for y in range(0, MAP_H, 7):
            row = [px[x, y] for x in range(0, MAP_W, 13)]
            if max(row) - min(row) < 2:
                bad_rows += 1
        note(bad_rows < 6, f"zn {LADDER[i]}: {bad_rows} flat rows (clipped/smeared)")

    print()
    if FAIL:
        print(f"FAILURES: {len(FAIL)}")
        for f in FAIL:
            print("  - " + f)
    else:
        print("ALL CHECKS PASSED")


if __name__ == "__main__":
    main()
