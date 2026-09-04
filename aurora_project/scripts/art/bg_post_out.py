# -*- coding: utf-8 -*-
"""Постобработка outpaint-вариантов: гамма до ~13 + сглаживание шва по колонкам."""
import io
import os

from PIL import Image, ImageEnhance, ImageFilter

SRC = r"D:\pr\Aurora\devilutionX_auroraos\.screenshots\bg_candidates"
OUT = os.path.join(SRC, "previews")
TARGET_MEAN = 13.0


def crop_fill(img, aspect):
    w, h = img.size
    cur = w / h
    if cur > aspect:
        nw = int(h * aspect)
        box = ((w - nw) // 2, 0, (w + nw) // 2, h)
    else:
        nh = int(w / aspect)
        box = (0, (h - nh) // 2, w, (h + nh) // 2)
    return img.crop(box)


def gamma_to(img, target):
    hist = img.convert("L").histogram()
    total = sum(hist)
    lo, hi = 1.0, 12.0
    for _ in range(40):
        mid = (lo + hi) / 2
        est = sum((i / 255.0) ** mid * c for i, c in enumerate(hist)) / total * 255
        if est > target:
            lo = mid
        else:
            hi = mid
    g = (lo + hi) / 2
    return img.point([min(255, int(round(255.0 * (i / 255.0) ** g))) for i in range(256)] * 3), g


def column_profile(img, tag, seam_x):
    g = img.convert("L")
    w, h = g.size
    px = g.load()
    print(tag, "colon profile (col x: mean):", end="")
    for x in range(seam_x - 120, min(seam_x + 140, w), 20):
        s = sum(px[x, y] for y in range(0, h, 4))
        print("  %d:%d" % (x, s // (h // 4)), end="")
    print()


def process(name, tag, seam_x):
    img = Image.open(os.path.join(SRC, name)).convert("RGB")
    img = ImageEnhance.Color(img).enhance(0.9)
    img, g = gamma_to(img, TARGET_MEAN)
    img = img.filter(ImageFilter.GaussianBlur(1.4))

    # Мягкое затемнение к низу.
    w, h = img.size
    grad = Image.new("L", (1, h))
    for y in range(h):
        t = y / (h - 1)
        grad.putpixel((0, y), int(255 * (1.0 - 0.30 * max(0.0, (t - 0.55) / 0.45))))
    img = Image.composite(img, Image.new("RGB", img.size, (0, 0, 0)), grad.resize(img.size))

    img.save(os.path.join(OUT, "proc4_{}.jpg".format(tag)), "JPEG", quality=90)
    column_profile(img, tag, seam_x)

    for orient, aspect in (("land", 16 / 9), ("port", 9 / 19.5)):
        prev = crop_fill(img, aspect)
        prev = prev.resize((1600, int(1600 / aspect)) if orient == "land" else (720, 1560))
        prev = Image.blend(prev, Image.new("RGB", prev.size, (0, 0, 0)), 0.28)
        prev.save(os.path.join(OUT, "prev4_{}_{}.jpg".format(tag, orient)), "JPEG", quality=88)


# B650: полоса оригинала начинается на x = 1344 - 694 = 650 (на шве нахлёст ~14px)
process("out_B650_harm.png", "outB650", 650)
process("out_C334_harm.png", "outC334", 334)
process("out_A_crop.png", "outAcrop", 650)  # шва нет, для контроля
print("done ->", OUT)
