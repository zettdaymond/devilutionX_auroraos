# -*- coding: utf-8 -*-
"""Вороны на ФИНАЛЬНОМ обработанном кадре: сплошные, крупнее, с ореолом тумана."""
import io
import math
import os

from PIL import Image, ImageDraw, ImageFilter

SRC = r"D:\pr\Aurora\devilutionX_auroraos\.screenshots\bg_candidates\previews"
ASSETS = r"D:\pr\Aurora\devilutionX_auroraos\Source\platform\aurora_os\devilution_launcher\assets"
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


base = Image.open(os.path.join(SRC, "proc5_cemetery_d50_8702.jpg")).convert("RGBA")

birds = [
    (rot(raven_fly(9, 1.0), -10),  47.0, 12.0, 26),
    (rot(raven_fly(13, 0.95), 7),  58.5,  9.0, 22),
    (rot(raven_fly(18, 1.05), -4), 66.0, 19.0, 18),
    (rot(raven_glide(15), 10),     50.5, 26.0, 20),
    (rot(raven_fly(8, 0.8), 14),   71.5, 13.5, 28),
]

layer = Image.new("RGBA", (W, H), (0, 0, 0, 0))

# Ореол тумана за каждой птицей: вырезает силуэт из неба на низкой яркости.
halo = Image.new("RGBA", (W, H), (0, 0, 0, 0))
hd = ImageDraw.Draw(halo)
for _, cx, cy, hr in birds:
    px, py = W * cx / 100, H * cy / 100
    hd.ellipse((px - hr, py - hr * 0.75, px + hr, py + hr * 0.75), fill=(56, 50, 37, 60))
px, py = W * 18.5 / 100, H * 28.5 / 100
hd.ellipse((px - 26, py - 20, px + 30, py + 24), fill=(56, 50, 37, 85))
halo = halo.filter(ImageFilter.GaussianBlur(5))
layer = Image.alpha_composite(layer, halo)

dr = ImageDraw.Draw(layer)
for poly, cx, cy, _ in birds:
    pts = [(W * cx / 100 + qx, H * cy / 100 + qy) for qx, qy in poly]
    dr.polygon(pts, fill=(8, 7, 5, 255))
pts = [(px + qx, py + qy) for qx, qy in raven_perched(15)]
dr.polygon(pts, fill=(8, 7, 5, 255))

layer = layer.filter(ImageFilter.GaussianBlur(0.6))
out = Image.alpha_composite(base, layer).convert("RGB")
out.save(os.path.join(ASSETS, "bg.jpg"), "JPEG", quality=90)

# Самопроверка контраста: минимум внутри зоны птицы vs среднее неба вокруг.
g = out.convert("L")
for _, cx, cy, hr in birds:
    bx, by = int(W * cx / 100), int(H * cy / 100)
    box = g.crop((bx - hr, by - hr, bx + hr, by + hr))
    around = g.crop((bx - 2 * hr, by - 2 * hr, bx + 2 * hr, by + 2 * hr))
    print("bird (%.0f%%,%.0f%%): min %d  sky-mean %.1f  delta %.1f" % (
        cx, cy, min(box.getdata()), sum(around.getdata()) / around.size[0] / around.size[1],
        sum(around.getdata()) / around.size[0] / around.size[1] - min(box.getdata())))
print("saved -> assets/bg.jpg")
