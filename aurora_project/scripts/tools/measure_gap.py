# -*- coding: utf-8 -*-
"""Y-полосы яркого текста: измеряет зазор название→описание и описание→след.название."""
import sys
from PIL import Image

path = sys.argv[1]
img = Image.open(path).convert("RGB")
w, h = img.size
px = img.load()


def bright(r, g, b):
    return r > 100 and g > 85 and b > 45 and r >= g >= b and (r - b) > 30


# строка пикселей считается текстовой, если есть хотя бы 8 ярких пикселей
# в зоне контента (без заголовка окна и правого края)
counts = []
for y in range(h):
    c = 0
    for x in range(40, w - 45):
        if bright(*px[x, y]):
            c += 1
    counts.append(c)

bands = []
in_band = False
for y, c in enumerate(counts):
    if c >= 8 and not in_band:
        start = y
        in_band = True
    elif c < 8 and in_band:
        bands.append((start, y - 1))
        in_band = False
if in_band:
    bands.append((start, h - 1))

print("текстовые полосы (y0..y1, высота):")
prev = None
for y0, y1 in bands[:14]:
    gap = "" if prev is None else " зазор от предыдущей: {}".format(y0 - prev)
    print("  {}..{} ({}px){}".format(y0, y1, y1 - y0 + 1, gap))
    prev = y1 + 1
