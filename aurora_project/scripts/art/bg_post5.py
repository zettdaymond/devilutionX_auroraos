# -*- coding: utf-8 -*-
"""Light-постобработка house-вариантов + превью."""
import colorsys
import io
import os

from PIL import Image, ImageEnhance, ImageFilter

SRC = r"D:\pr\Aurora\devilutionX_auroraos\.screenshots\bg_candidates"
ASSETS = r"D:\pr\Aurora\devilutionX_auroraos\Source\platform\aurora_os\devilution_launcher\assets"
OUT = os.path.join(SRC, "previews")


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


def palette_stats(img, tag):
    im = img.convert("RGB").resize((256, 150))
    vis = [px for px in im.getdata() if sum(px) > 60]
    if not vis:
        return
    yellow = red = 0
    for r, g, b in vis:
        deg = colorsys.rgb_to_hsv(r / 255, g / 255, b / 255)[0] * 360
        if deg >= 330:
            deg -= 360
        if -30 <= deg <= 60:
            if deg >= 15:
                yellow += 1
            else:
                red += 1
    g = img.convert("L")
    sat = sum(colorsys.rgb_to_hsv(r / 255, g / 255, b / 255)[1] for r, g, b in vis) / len(vis)
    print(tag, 'vis%', round(100 * len(vis) / 38400, 1), 'yellow%', round(100 * yellow / len(vis), 1),
          'red%', round(100 * red / len(vis), 1), 'sat', round(sat, 2),
          'mean', round(sum(g.getdata()) / (g.size[0] * g.size[1]), 1))


def process(name, tag):
    img = Image.open(os.path.join(SRC, name)).convert("RGB")
    img = img.filter(ImageFilter.GaussianBlur(1.4))
    img = ImageEnhance.Color(img).enhance(0.90)

    w, h = img.size
    grad = Image.new("L", (1, h))
    for y in range(h):
        t = y / (h - 1)
        grad.putpixel((0, y), int(255 * (1.0 - 0.30 * max(0.0, (t - 0.55) / 0.45))))
    img = Image.composite(img, Image.new("RGB", img.size, (0, 0, 0)), grad.resize(img.size))

    img.save(os.path.join(OUT, "proc5_{}.jpg".format(tag)), "JPEG", quality=90)
    palette_stats(img, "proc5_" + tag)

    for orient, aspect in (("land", 16 / 9), ("port", 9 / 19.5)):
        prev = crop_fill(img, aspect)
        prev = prev.resize((1600, int(1600 / aspect)) if orient == "land" else (720, 1560))
        prev = Image.blend(prev, Image.new("RGB", prev.size, (0, 0, 0)), 0.28)
        prev.save(os.path.join(OUT, "prev5_{}_{}.jpg".format(tag, orient)), "JPEG", quality=88)


palette_stats(Image.open(os.path.join(ASSETS, "bg.jpg")), "bg.jpg    ")
for tag in ("ravens_drawn_8702", "ravens_village_8801", "ravens_cemetery62_8901"):
    process(tag + ".png", tag)
print("done ->", OUT)
