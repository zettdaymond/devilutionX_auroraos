# -*- coding: utf-8 -*-
"""Вороны для двух финалистов: fogvillage_d68_8801 и fogcemetery_d62_8901."""
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
    body = [(0.12 * s, -0.08 * s), (0.30 * s, 0.12 * s), (0.10 * s, 0.30 * s),
            (-0.10 * s, 0.32 * s), (-0.40 * s, 0.42 * s), (-0.28 * s, 0.05 * s)]
    ltop = bez((-0.18 * s, -0.12 * s), (-0.75 * s, -0.55 * s * raise_w - 0.2 * s),
               (-1.00 * s, (-1.05 * raise_w - 0.05) * s), 6)
    lbot = bez((-0.95 * s, (-0.88 * raise_w - 0.06) * s), (-0.60 * s, -0.45 * s * raise_w - 0.1 * s),
               (-0.22 * s, -0.02 * s), 6)
    return ltop + lbot + body + mirror(lbot) + mirror(ltop)


def raven_descend(s):
    body = [(0.12 * s, -0.10 * s), (0.30 * s, 0.14 * s), (0.10 * s, 0.30 * s),
            (-0.10 * s, 0.32 * s), (-0.40 * s, 0.44 * s), (-0.28 * s, 0.05 * s)]
    ltop = bez((-0.18 * s, -0.14 * s), (-0.70 * s, -0.10 * s), (-0.95 * s, 0.45 * s), 6)
    lbot = bez((-0.88 * s, 0.38 * s), (-0.55 * s, 0.20 * s), (-0.20 * s, 0.02 * s), 6)
    return ltop + lbot + body + mirror(lbot) + mirror(ltop)


def raven_perched(s):
    return [
        (-0.55 * s, -0.50 * s), (0.10 * s, -0.72 * s), (0.32 * s, -0.42 * s),
        (0.85 * s, -0.28 * s), (0.40 * s, -0.08 * s), (0.48 * s, 0.32 * s),
        (0.82 * s, 0.92 * s), (0.22 * s, 0.52 * s), (-0.32 * s, 0.56 * s), (-0.62 * s, 0.08 * s),
    ]


def add_ravens(src_name, out_name, birds, perch):
    base = Image.open(os.path.join(SRC, src_name)).convert("RGBA")
    layer = Image.new("RGBA", (W, H), (0, 0, 0, 0))

    px, py, ps, halo_alpha = perch
    halo = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    hd = ImageDraw.Draw(halo)
    hd.ellipse((W * px / 100 - 24, H * py / 100 - 19, W * px / 100 + 28, H * py / 100 + 22),
               fill=(52, 46, 34, halo_alpha))
    layer = Image.alpha_composite(layer, halo.filter(ImageFilter.GaussianBlur(6)))

    dr = ImageDraw.Draw(layer)
    for poly, cx, cy, rgb, alpha, blur in birds:
        pts = [(W * cx / 100 + qx, H * cy / 100 + qy) for qx, qy in poly]
        dr.polygon(pts, fill=rgb + (alpha,))
    pts = [(W * px / 100 + qx, H * py / 100 + qy) for qx, qy in raven_perched(ps)]
    dr.polygon(pts, fill=(12, 10, 8, 235))

    layer = layer.filter(ImageFilter.GaussianBlur(0.7))
    out = Image.alpha_composite(base, layer).convert("RGB")
    out.save(os.path.join(SRC, out_name))

    lut = [min(255, int(round(255.0 * (i / 255.0) ** 0.4))) for i in range(256)]
    out.point(lut * 3).save(os.path.join(SRC, "bright_" + out_name))
    print(out_name, "ok")


# fogvillage_d68_8801: небо A/B/C, насест — сухой сук над крышей (43.5, 43)
add_ravens(
    "fogvillage_d68_8801.png", "ravens_village_8801.png",
    [
        (rot(raven_fly(7, 1.0), -10), 43.0, 10.0, (38, 33, 25), 115, 1.3),
        (rot(raven_fly(10, 0.95), 7), 49.0, 18.0, (24, 20, 15), 155, 1.0),
        (rot(raven_fly(9, 1.05), -6), 63.0, 9.0, (26, 22, 17), 150, 1.0),
        (rot(raven_descend(8), 12), 72.0, 30.0, (32, 27, 20), 135, 1.1),
    ],
    (43.5, 43.0, 12, 60),
)

# fogcemetery_d62_8901: своя пара воронов уже есть на (47,37) — новых в стороне;
# насест — вершина высокого креста (32, 36)
add_ravens(
    "fogcemetery_d62_8901.png", "ravens_cemetery62_8901.png",
    [
        (rot(raven_fly(7, 1.0), 9), 44.0, 14.0, (38, 33, 25), 115, 1.3),
        (rot(raven_fly(10, 0.95), -7), 52.0, 10.0, (24, 20, 15), 155, 1.0),
        (rot(raven_fly(10, 1.05), 5), 60.0, 38.0, (22, 18, 14), 160, 1.0),
        (rot(raven_fly(6, 0.8), -12), 66.0, 32.0, (40, 34, 26), 110, 1.4),
    ],
    (32.0, 36.0, 11, 70),
)
print("done")
