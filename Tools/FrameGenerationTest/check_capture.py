# Captures one pixel row of the RetroStationX window many times while scrolltest.nes runs,
# and reports the scroll positions seen (in NES pixels). The ROM scrolls exactly 2 pixels per
# emulated frame, so without frame generation only even positions can appear; generated
# in-between frames show odd positions.
# Usage (Windows): python check_capture.py <window title substring> [captures]
import ctypes, sys, time
from ctypes import wintypes
from math import gcd
from PIL import ImageGrab

user32 = ctypes.windll.user32
title_part = sys.argv[1]
captures = int(sys.argv[2]) if len(sys.argv) > 2 else 400

found = []
@ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
def enum_proc(hwnd, _):
    buf = ctypes.create_unicode_buffer(256)
    user32.GetWindowTextW(hwnd, buf, 256)
    if title_part in buf.value and user32.IsWindowVisible(hwnd):
        found.append(hwnd)
    return True
user32.EnumWindows(enum_proc, 0)
if not found:
    sys.exit("window not found")
hwnd = found[0]
user32.SetForegroundWindow(hwnd)
time.sleep(0.5)

rect = wintypes.RECT()
user32.GetClientRect(hwnd, ctypes.byref(rect))
origin = wintypes.POINT(0, 0)
user32.ClientToScreen(hwnd, ctypes.byref(origin))
width, height = rect.right, rect.bottom
y = origin.y + int(height * 0.6)
bbox = (origin.x, y, origin.x + width, y + 1)

lines = []
start = time.perf_counter()
for _ in range(captures):
    img = ImageGrab.grab(bbox=bbox, all_screens=True)
    lines.append(list(img.getdata()))
elapsed = time.perf_counter() - start

# Find the game area (non-black part of the line) and the NES pixel size (gcd of color run lengths)
ref = lines[0]
runs = []
run = 1
for i in range(1, len(ref)):
    if ref[i] == ref[i - 1]:
        run += 1
    else:
        runs.append(run)
        run = 1
inner = runs[1:-1]
scale = 0
for r in inner:
    scale = gcd(scale, r)
if scale == 0:
    sys.exit("could not determine pixel scale")

period = 128 * scale  # the background repeats every 16 tiles
def best_shift(line):
    best = None
    span = len(ref) - period
    for d in range(period):
        diff = 0
        for x in range(0, span, 2):
            a, b = line[x], ref[x + d]
            if a != b:
                diff += 1
        if best is None or diff < best[1]:
            best = (d, diff)
    return best

positions = {}
bad = 0
for line in lines:
    d, diff = best_shift(line)
    if diff > (len(ref) - period) // 2 * 0.05:
        bad += 1
        continue
    nes_shift = d / scale
    key = round(nes_shift, 2)
    positions[key] = positions.get(key, 0) + 1

even = sum(c for p, c in positions.items() if p == int(p) and int(p) % 2 == 0)
odd = sum(c for p, c in positions.items() if p == int(p) and int(p) % 2 == 1)
print(f"{captures} captures in {elapsed:.2f}s, NES pixel scale {scale}x, unmatched captures: {bad}")
print(f"scroll positions at even NES pixels: {even}, at odd NES pixels: {odd}")
print("distinct positions (mod 128):", sorted(positions)[:24])
