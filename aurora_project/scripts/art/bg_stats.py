# -*- coding: utf-8 -*-
"""Статистика кандидатов фона: темнота, спокойствие центра, портретный кроп."""
import glob
import io
import os

from PIL import Image, ImageFilter

OUT = r"D:\pr\Aurora\devilutionX_auroraos\.screenshots\bg_candidates"


def lum(img):
    return img.convert("L")


def region_mean(g, box):
    return sum(g.crop(box).getdata()) / (g.crop(box).size[0] * g.crop(box).size[1])


rows = []
for path in sorted(glob.glob(os.path.join(OUT, "bg*.png"))):
    img = Image.open(path)
    g = lum(img)
    w, h = g.size

    # Центральная полоса = то, что видно в портретном окне (aspect 0.46).
    strip_w = int(h * 0.46)
    strip = g.crop(((w - strip_w) // 2, 0, (w + strip_w) // 2, h))

    # Занятость: средняя сила краёв (FIND_EDGES), нижнее значение = спокойнее.
    edges = g.filter(ImageFilter.FIND_EDGES)
    edge_mean = sum(edges.getdata()) / (w * h)

    # Доля очень ярких пикселей (пламя/блики) по всей картинке и в центре.
    full_data = list(g.getdata())
    bright_full = sum(1 for v in full_data if v > 200) / len(full_data)
    center_data = list(g.crop((w // 4, h // 4, 3 * w // 4, 3 * h // 4)).getdata())
    bright_center = sum(1 for v in center_data if v > 200) / len(center_data)

    rows.append((
        os.path.basename(path),
        round(region_mean(g, (0, 0, w, h)), 1),
        round(region_mean(strip, (0, 0, strip.size[0], strip.size[1])), 1),
        round(region_mean(g, (w // 4, h // 4, 3 * w // 4, 3 * h // 4)), 1),
        round(edge_mean, 2),
        round(bright_full * 100, 2),
        round(bright_center * 100, 2),
    ))

print("{:<28} {:>6} {:>8} {:>7} {:>6} {:>7} {:>8}".format(
    "file", "mean", "strip", "center", "edges", "brt%", "brtC%"))
for r in rows:
    print("{:<28} {:>6} {:>8} {:>7} {:>6} {:>7} {:>8}".format(*r))
