# -*- coding: utf-8 -*-
"""Что находится у правого края ландшафтного кадра настроек."""
import sys
from PIL import Image

path = sys.argv[1]
img = Image.open(path).convert("RGB")
w, h = img.size
px = img.load()


def bright(r, g, b):
    return r > 120 and g > 100 and b > 60 and r >= g >= b


# для каждой полосы y по 40px: границы ярких пикселей
print("полосы y: x0..x1 (крайние яркие)")
for y0 in range(150, h, 60):
    xs = [x for y in range(y0, min(y0 + 60, h)) for x in range(w) if bright(*px[x, y])]
    if xs:
        print("  y{}..{}: {}..{}  n={}".format(y0, y0 + 60, min(xs), max(xs), len(xs)))

# точные границы тумблеров: колонки с золотыми пикселями в правой половине
col = {}
for x in range(w // 2, w):
    c = 0
    for y in range(h):
        if bright(*px[x, y]):
            c += 1
    if c > 2:
        col[x] = c
xs = sorted(col)
if xs:
    runs = []
    start = xs[0]
    prev = xs[0]
    for x in xs[1:]:
        if x - prev > 6:
            runs.append((start, prev))
            start = x
        prev = x
    runs.append((start, prev))
    print("золотые колонки правой половины (диапазоны):", runs)
