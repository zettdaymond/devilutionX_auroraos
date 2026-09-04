# -*- coding: utf-8 -*-
"""Вороны на апскейлнутом финалисте (2688x1536): светлые зоны, сплошной чёрный."""
import io
import math
import os

from PIL import Image, ImageDraw, ImageFilter

SRC = r"D:\pr\Aurora\devilutionX_auroraos\.screenshots\bg_candidates\previews"
ASSETS = r"D:\pr\Aurora\devilutionX_auroraos\Source\platform\aurora_os\devilution_launcher\assets"
W, H = 2688, 1536


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
    body = [(0.12 * s, -0.08 * s), (0.30 * s, 0.12 * s), (0.10 * s, 0.30 * s),
            (-0.10 * s, 0.32 * s), (-0.40 * s, 0.42 * s), (-0.28 * s, 0.05 * s)]
    ltop = bez((-0.18 * s, -0.12 * s), (-0.75 * s, -0.55 * s * raise_w - 0.2 * s),
               (-1.00 * s, (-1.05 * raise_w - 0.05) * s), 6)
    lbot = bez((-0.95 * s, (-0.88 * raise_w - 0.06) * s), (-0.60 * s, -0.45 * s * raise_w - 0.1 * s),
               (-0.22 * s, -0.02 * s), 6)
    return ltop + lbot + body + mirror(lbot) + mirror(ltop)


def raven_glide(s):
    body = [(0.14 * s, -0.06 * s), (0.34 * s, 0.14 * s), (0.12 * s, 0.30 * s),
            (-0.10 * s, 0.32 * s), (-0.40 * s, 0.44 * s), (-0.26 * s, 0.06 * s)]
    ltop = bez((-0.20 * s, -0.10 * s), (-0.70 * s, -0.38 * s), (-1.00 * s, -0.50 * s), 6)
    lbot = bez((-0.95 * s, -0.38 * s), (-0.55 * s, -0.18 * s), (-0.20 * s, 0.00 * s), 6)
    return ltop + lbot + body + mirror(lbot) + mirror(ltop)


def raven_perched(s):
    return [
        (-0.55 * s, -0.50 * s), (0.10 * s, -0.72 * s), (0.32 * s, -0.42 * s),
        (0.85 * s, -0.28 * s), (0.40 * s, -0.08 * s), (0.48 * s, 0.32 * s),
        (0.82 * s, 0.92 * s), (0.22 * s, 0.52 * s), (-0.32 * s, 0.56 * s), (-0.62 * s, 0.08 * s),
    ]


base = Image.open(os.path.join(SRC, "fin_cemetery_8702.jpg")).convert("RGBA")

# (полигон, cx%, cy%, радиус ореола)
birds = [
    (rot(raven_fly(18, 1.0), -10),  44.0, 27.0, 34),
    (rot(raven_fly(26, 0.95), 7),   50.0, 22.0, 44),
    (rot(raven_fly(34, 1.05), -4),  58.0, 26.0, 54),
    (rot(raven_glide(30), 12),      46.0, 33.0, 48),
    (rot(raven_fly(16, 0.8), 14),   64.0, 30.0, 30),
]

layer = Image.new("RGBA", (W, H), (0, 0, 0, 0))
halo = Image.new("RGBA", (W, H), (0, 0, 0, 0))
hd = ImageDraw.Draw(halo)
for _, cx, cy, hr in birds:
    px, py = W * cx / 100, H * cy / 100
    hd.ellipse((px - hr, py - hr * 0.7, px + hr, py + hr * 0.7), fill=(56, 50, 37, 55))
hpx, hpy = W * 18.5 / 100, H * 28.5 / 100
hd.ellipse((hpx - 50, hpy - 38, hpx + 58, hpy + 46), fill=(56, 50, 37, 85))
halo = halo.filter(ImageFilter.GaussianBlur(9))
layer = Image.alpha_composite(layer, halo)

dr = ImageDraw.Draw(layer)
for poly, cx, cy, _ in birds:
    pts = [(W * cx / 100 + qx, H * cy / 100 + qy) for qx, qy in poly]
    dr.polygon(pts, fill=(8, 7, 5, 255))
pts = [(hpx + qx, hpy + qy) for qx, qy in raven_perched(30)]
dr.polygon(pts, fill=(8, 7, 5, 255))

layer = layer.filter(ImageFilter.GaussianBlur(0.9))
out = Image.alpha_composite(base, layer).convert("RGB")

buf_check = out.copy()
out.save(os.path.join(ASSETS, "bg_alt_cemetery_8702.jpg"), "JPEG", quality=90)

g = buf_check.convert("L")
for _, cx, cy, hr in birds:
    bx, by = int(W * cx / 100), int(H * cy / 100)
    box = g.crop((bx - hr // 2, by - hr // 2, bx + hr // 2, by + hr // 2))
    around = g.crop((bx - 2 * hr, by - int(1.4 * hr), bx + 2 * hr, by + int(1.4 * hr)))
    am = sum(around.getdata()) / (around.size[0] * around.size[1])
    print("bird (%.0f%%,%.0f%%): min %d  sky %.1f  delta %.1f" % (cx, cy, min(box.getdata()), am, am - min(box.getdata())))
print("saved -> assets/bg_alt_cemetery_8702.jpg", out.size)
