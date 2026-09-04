# -*- coding: utf-8 -*-
"""Рисуем воронов поверх cemetery_d50_8702: полигональные силуэты с мягкими краями."""
import io
import os

from PIL import Image, ImageDraw, ImageFilter

SRC = r"D:\pr\Aurora\devilutionX_auroraos\.screenshots\bg_candidates"

W, H = 1344, 768


def raven_flying_up(s):
    """Крылья подняты V. s = полуразмах крыла в пикселях."""
    return [
        (0.00 * s, -0.10 * s),   # верх левого крыла
        (-0.95 * s, -1.00 * s),
        (-1.05 * s, -0.80 * s),
        (-0.25 * s, 0.05 * s),
        (-0.45 * s, 0.45 * s),   # хвост-лево
        (-0.10 * s, 0.30 * s),
        (0.10 * s, 0.34 * s),    # хвост-право
        (0.42 * s, 0.50 * s),
        (0.30 * s, 0.05 * s),
        (1.00 * s, -0.75 * s),   # правое крыло
        (0.92 * s, -0.98 * s),
    ]


def raven_glide(s):
    """Планирующий: крылья горизонтально."""
    return [
        (-1.00 * s, -0.28 * s),
        (-0.30 * s, 0.02 * s),
        (-0.42 * s, 0.42 * s),
        (-0.08 * s, 0.28 * s),
        (0.10 * s, 0.32 * s),
        (0.44 * s, 0.46 * s),
        (0.34 * s, 0.02 * s),
        (1.00 * s, -0.30 * s),
        (0.98 * s, -0.44 * s),
        (0.10 * s, -0.16 * s),
        (-0.96 * s, -0.42 * s),
    ]


def raven_perched(s):
    """Сидящий: каплевидное тело, хвост вниз, клюв."""
    return [
        (-0.55 * s, -0.55 * s),
        (0.10 * s, -0.75 * s),
        (0.35 * s, -0.45 * s),
        (0.85 * s, -0.32 * s),   # клюв
        (0.42 * s, -0.12 * s),
        (0.50 * s, 0.30 * s),
        (0.85 * s, 0.95 * s),    # хвост
        (0.25 * s, 0.55 * s),
        (-0.30 * s, 0.60 * s),
        (-0.62 * s, 0.10 * s),
    ]


base = Image.open(os.path.join(SRC, "cemetery_d50_8702.png")).convert("RGBA")

birds = [
    # (полигон, cx%, cy%, размер s в px, альфа, blur)
    (raven_flying_up(9),  47.0, 12.0, 9,  165, 1.1),
    (raven_flying_up(13), 58.5,  9.0, 13, 185, 0.9),
    (raven_flying_up(19), 66.0, 20.0, 19, 210, 0.7),
    (raven_glide(15),     50.5, 26.0, 15, 185, 0.9),
    (raven_flying_up(7),  71.5, 13.5, 7, 150, 1.2),
    (raven_perched(12),   15.0, 27.5, 12, 225, 0.6),
]

layer = Image.new("RGBA", (W, H), (0, 0, 0, 0))
dr = ImageDraw.Draw(layer)
for poly, cx, cy, s, alpha, blur in birds:
    pts = [(W * cx / 100 + px, H * cy / 100 + py) for px, py in poly]
    dr.polygon(pts, fill=(10, 8, 6, alpha))
layer = layer.filter(ImageFilter.GaussianBlur(0.8))

out = Image.alpha_composite(base, layer).convert("RGB")
out.save(os.path.join(SRC, "ravens_drawn_8702.png"))

# Яркая копия для самопроверки.
lut = [min(255, int(round(255.0 * (i / 255.0) ** 0.4))) for i in range(256)]
out.point(lut * 3).save(os.path.join(SRC, "bright_ravens_drawn_8702.png"))
print("ok")
