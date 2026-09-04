# -*- coding: utf-8 -*-
"""Отступы контентного блока: яркие пиксели UI (фон после tanh темнее r=73)."""
import sys
from PIL import Image

path = sys.argv[1]
img = Image.open(path).convert("RGB")
w, h = img.size
px = img.load()


def bright(r, g, b):
    return r > 120 and g > 100 and b > 60 and r >= g >= b


col = [0] * w
for y in range(h):
    for x in range(w):
        if bright(*px[x, y]):
            col[x] += 1

# игнорируем скролл-индикатор у правого края (последние 40px)
for x in range(w - 40, w):
    col[x] = 0

nz = [x for x in range(w) if col[x] > 0]
print("UI columns span: {}..{} of {}".format(nz[0], nz[-1], w))
print("left margin {}, right margin {} (без зоны индикатора)".format(nz[0], (w - 40) - 1 - nz[-1]))

# Тумблеры/контролы — самые правые яркие скопления; текст — левые.
left_text = [x for x in nz if col[x] >= 3][:50]
print("leftmost text columns:", left_text[:5], "...")
