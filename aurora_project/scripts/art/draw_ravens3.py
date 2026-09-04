# -*- coding: utf-8 -*-
"""Вороны v3: разброс поз/наклонов, тон в глубине тумана, ореол за сидящим."""
import io
import math
import os

from PIL import Image, ImageDraw, ImageFilter

SRC = r"D:\pr\Aurora\devilutionX_auroraos\.screenshots\bg_candidates"
W, H = 1344, 768


def bez(p0, p1, p2, n=6):
    pts = []
    for i in range(n + 1):
        t = i / n
        x = (1 - t) ** 2 * p0[0] + 2 * (1 - t) * t * p1[0] + t ** 2 * p2[0]
        y = (1 - t) ** 2 * p0[1] + 2 * (1 - t) * t * p1[1] + t ** 2 * p2[1]
        pts.append((x, y))
    return pts


def rot(pts, deg):
    a = math.radians(deg)
    c, s = math.cos(a), math.sin(a)
    return [(x * c - y * s, x * s + y * c) for x, y in pts]


def mirror(pts):
    return [(-x, y) for x, y in pts]


def raven_fly(s, raise_w=1.0):
    """Крылья дугой: raise_w поднимает кончики (1.0 = вверх, 0.2 = почти план)."""
    body = [(0.12 * s, -0.08 * s), (0.30 * s, 0.12 * s), (0.10 * s, 0.30 * s),
            (-0.10 * s, 0.32 * s), (-0.40 * s, 0.42 * s), (-0.28 * s, 0.05 * s)]
    ltop = bez((-0.18 * s, -0.12 * s), (-0.75 * s, -0.55 * s * raise_w - 0.2 * s),
               (-1.00 * s, (-1.05 * raise_w - 0.05) * s), 6)
    lbot = bez((-0.95 * s, (-0.88 * raise_w - 0.06) * s), (-0.60 * s, -0.45 * s * raise_w - 0.1 * s),
               (-0.22 * s, -0.02 * s), 6)
    rtop = mirror(ltop)
    rbot = mirror(lbot)
    return ltop + lbot + body + rbot + rtop


def raven_descend(s):
    """Снижающийся: крылья вниз-в стороны."""
    body = [(0.12 * s, -0.10 * s), (0.30 * s, 0.14 * s), (0.10 * s, 0.30 * s),
            (-0.10 * s, 0.32 * s), (-0.40 * s, 0.44 * s), (-0.28 * s, 0.05 * s)]
    ltop = bez((-0.18 * s, -0.14 * s), (-0.70 * s, -0.10 * s), (-0.95 * s, 0.45 * s), 6)
    lbot = bez((-0.88 * s, 0.38 * s), (-0.55 * s, 0.20 * s), (-0.20 * s, 0.02 * s), 6)
    rtop = mirror(ltop)
    rbot = mirror(lbot)
    return ltop + lbot + body + rbot + rtop


def raven_perched(s):
    return [
        (-0.55 * s, -0.50 * s),
        (0.10 * s, -0.72 * s),
        (0.32 * s, -0.42 * s),
        (0.85 * s, -0.28 * s),
        (0.40 * s, -0.08 * s),
        (0.48 * s, 0.32 * s),
        (0.82 * s, 0.92 * s),
        (0.22 * s, 0.52 * s),
        (-0.32 * s, 0.56 * s),
        (-0.62 * s, 0.08 * s),
    ]


base = Image.open(os.path.join(SRC, "cemetery_d50_8702.png")).convert("RGBA")

# (полигон, cx%, cy%, rgb, альфа, blur)
birds = [
    (rot(raven_fly(7, 1.0), -10),   47.0, 12.0, (38, 33, 25), 115, 1.3),
    (rot(raven_fly(11, 0.95), 7),   58.5,  9.0, (24, 20, 15), 155, 1.0),
    (rot(raven_fly(14, 1.05), -4),  66.0, 19.0, (12, 10, 8),  200, 0.7),
    (rot(raven_descend(12), 10),    50.5, 25.5, (26, 22, 17), 150, 1.0),
    (rot(raven_fly(6, 0.8), 14),    71.5, 13.5, (42, 36, 27), 110, 1.4),
]

layer = Image.new("RGBA", (W, H), (0, 0, 0, 0))
dr = ImageDraw.Draw(layer)

# Сидящему сначала светящийся ореол тумана — прорезает силуэт из ветки.
px, py = W * 18.5 / 100, H * 28.5 / 100
halo = Image.new("RGBA", (W, H), (0, 0, 0, 0))
hd = ImageDraw.Draw(halo)
hd.ellipse((px - 26, py - 20, px + 30, py + 24), fill=(52, 46, 34, 95))
halo = halo.filter(ImageFilter.GaussianBlur(7))
layer = Image.alpha_composite(layer, halo)
dr = ImageDraw.Draw(layer)

for poly, cx, cy, rgb, alpha, blur in birds:
    pts = [(W * cx / 100 + qx, H * cy / 100 + qy) for qx, qy in poly]
    dr.polygon(pts, fill=rgb + (alpha,))

perch = raven_perched(13)
pts = [(px + qx, py + qy) for qx, qy in perch]
dr.polygon(pts, fill=(12, 10, 8, 235))

layer = layer.filter(ImageFilter.GaussianBlur(0.7))
out = Image.alpha_composite(base, layer).convert("RGB")
out.save(os.path.join(SRC, "ravens_drawn_8702.png"))

lut = [min(255, int(round(255.0 * (i / 255.0) ** 0.4))) for i in range(256)]
out.point(lut * 3).save(os.path.join(SRC, "bright_ravens_drawn_8702.png"))
print("ok")
