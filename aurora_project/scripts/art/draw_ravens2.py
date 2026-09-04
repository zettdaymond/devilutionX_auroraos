# -*- coding: utf-8 -*-
"""Вороны v2: крылья кривыми Безье, сидящий на чистом конце ветки."""
import io
import os

from PIL import Image, ImageDraw, ImageFilter

SRC = r"D:\pr\Aurora\devilutionX_auroraos\.screenshots\bg_candidates"
W, H = 1344, 768


def bez(p0, p1, p2, n=6):
    """Квадратичная Безье: n точек от p0 к p2."""
    pts = []
    for i in range(n + 1):
        t = i / n
        x = (1 - t) ** 2 * p0[0] + 2 * (1 - t) * t * p1[0] + t ** 2 * p2[0]
        y = (1 - t) ** 2 * p0[1] + 2 * (1 - t) * t * p1[1] + t ** 2 * p2[1]
        pts.append((x, y))
    return pts


def raven_fly(s):
    """Летящий, крылья подняты дугой, кончики сужаются."""
    body = [(0.12 * s, -0.10 * s), (0.30 * s, 0.12 * s), (0.10 * s, 0.30 * s),
            (-0.10 * s, 0.32 * s), (-0.40 * s, 0.42 * s), (-0.28 * s, 0.05 * s)]
    left_top = bez((-0.18 * s, -0.12 * s), (-0.75 * s, -0.75 * s), (-1.00 * s, -1.05 * s), 6)
    left_bot = bez((-0.95 * s, -0.88 * s), (-0.60 * s, -0.45 * s), (-0.22 * s, -0.02 * s), 6)
    right_top = bez((0.20 * s, -0.14 * s), (0.78 * s, -0.80 * s), (1.02 * s, -1.02 * s), 6)
    right_bot = bez((0.96 * s, -0.84 * s), (0.62 * s, -0.42 * s), (0.24 * s, 0.00 * s), 6)
    return left_top + left_bot + body + right_bot + right_top


def raven_glide(s):
    """Планирующий: крылья чуть опущены дугой."""
    body = [(0.14 * s, -0.06 * s), (0.34 * s, 0.14 * s), (0.12 * s, 0.30 * s),
            (-0.10 * s, 0.32 * s), (-0.40 * s, 0.44 * s), (-0.26 * s, 0.06 * s)]
    ltop = bez((-0.20 * s, -0.10 * s), (-0.70 * s, -0.38 * s), (-1.00 * s, -0.50 * s), 6)
    lbot = bez((-0.95 * s, -0.38 * s), (-0.55 * s, -0.18 * s), (-0.20 * s, 0.00 * s), 6)
    rtop = bez((0.22 * s, -0.10 * s), (0.75 * s, -0.42 * s), (1.02 * s, -0.52 * s), 6)
    rbot = bez((0.97 * s, -0.40 * s), (0.58 * s, -0.18 * s), (0.24 * s, 0.00 * s), 6)
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

birds = [
    (raven_fly(8),   47.0, 12.0, 8,  160, 1.1),
    (raven_fly(11),  58.5,  9.0, 11, 180, 0.9),
    (raven_fly(15),  66.0, 19.0, 15, 205, 0.7),
    (raven_glide(13), 50.5, 26.0, 13, 180, 0.9),
    (raven_fly(6),   71.5, 13.5, 6, 145, 1.2),
    (raven_perched(13), 18.5, 28.5, 13, 225, 0.6),
]

layer = Image.new("RGBA", (W, H), (0, 0, 0, 0))
dr = ImageDraw.Draw(layer)
for poly, cx, cy, s, alpha, blur in birds:
    pts = [(W * cx / 100 + px, H * cy / 100 + py) for px, py in poly]
    dr.polygon(pts, fill=(10, 8, 6, alpha))
layer = layer.filter(ImageFilter.GaussianBlur(0.7))

out = Image.alpha_composite(base, layer).convert("RGB")
out.save(os.path.join(SRC, "ravens_drawn_8702.png"))

lut = [min(255, int(round(255.0 * (i / 255.0) ** 0.4))) for i in range(256)]
out.point(lut * 3).save(os.path.join(SRC, "bright_ravens_drawn_8702.png"))
print("ok")
