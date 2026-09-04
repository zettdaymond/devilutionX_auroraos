# -*- coding: utf-8 -*-
"""Цвет центра ручки тумблера: слева (выкл) и справа (вкл) каждой строки."""
import sys
from PIL import Image

img = Image.open(sys.argv[1]).convert("RGB")
w, h = img.size
px = img.load()


def bright(r, g, b):
    return r > 175 and 130 < g < 215 and b < 150 and r > g > b


# находим золотые кластеры (ручки/треки) в правой колонке
rows = []
for y in range(0, h):
    count = 0
    for x in range(int(w * 0.75), int(w * 0.97)):
        if bright(*px[x, y]):
            count += 1
    if count >= 3:
        rows.append(y)
clusters = []
for y in rows:
    if clusters and y - clusters[-1][-1] <= 3:
        clusters[-1].append(y)
    else:
        clusters.append([y])
clusters = [c for c in clusters if len(c) >= 8]

print("строки-тумблеры:", len(clusters))
for i, cl in enumerate(clusters):
    yc = (cl[0] + cl[-1]) // 2
    # трек-зона: ищем границы золотого по x (без скролл-индикатора у края)
    xs = [x for x in range(int(w * 0.72), w - 40) if bright(*px[x, yc])]
    if not xs:
        continue
    x0, x1 = min(xs), max(xs)
    width = x1 - x0
    state = "ON " if width > 45 else "OFF"
    # центр ручки: OFF — слева, ON — справа
    kx = x0 + 13 if width <= 45 else x1 - 13
    r, g, b = px[kx, yc]
    knob = "золотая" if bright(r, g, b) else "тёмная"
    print("  {}: {} (трек {}px), ручка {} rgb({},{},{})".format(i + 1, state, width, knob, r, g, b))
