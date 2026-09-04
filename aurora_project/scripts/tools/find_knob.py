# -*- coding: utf-8 -*-
"""Ищет ручку слайдера (плотный золотой диск левее зоны тумблеров)."""
import sys
from PIL import Image

img = Image.open(sys.argv[1]).convert("RGB")
w, h = img.size
px = img.load()
found = []
for y in range(0, h - 20, 2):
    x = 60
    while x < 440:
        r, g, b = px[x, y]
        if r > 200 and g > 160 and b < 140:
            run = 0
            for dx in range(0, 44):
                rr, gg, bb = px[min(x + dx, w - 1), y]
                if rr > 190 and gg > 150:
                    run += 1
                else:
                    break
            if run >= 18:
                found.append((x + run // 2, y, run))
                x += run
                continue
        x += 3
for cx, cy, run in found:
    print("slider knob: center ~({}, {}) width {}".format(cx, cy, run))
