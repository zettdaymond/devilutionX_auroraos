# -*- coding: utf-8 -*-
"""Ищет золотые ручки тумблеров в правой колонке скриншота."""
import sys
from PIL import Image

path = sys.argv[1] if len(sys.argv) > 1 else r"D:\pr\Aurora\devilutionX_auroraos\.screenshots\settings\portrait_top.png"
img = Image.open(path).convert("RGB")
w, h = img.size
px = img.load()

# Золотые пиксели: теплые, достаточно яркие (ручка GoldBright ~ #E8C77E)
rows = []
for y in range(0, h):
    count = 0
    for x in range(int(w * 0.75), int(w * 0.97)):
        r, g, b = px[x, y]
        if r > 175 and 130 < g < 215 and b < 150 and r > g > b:
            count += 1
    if count >= 3:
        rows.append((y, count))

# Группируем в кластеры по y
clusters = []
for y, c in rows:
    if clusters and y - clusters[-1][-1][0] <= 3:
        clusters[-1].append((y, c))
    else:
        clusters.append([(y, c)])

print("golden clusters (y_range, max_count):")
for cl in clusters:
    ys = [y for y, _ in cl]
    mc = max(c for _, c in cl)
    if len(ys) >= 8:
        print("  y {}..{}  max {}".format(ys[0], ys[-1], mc))
