# -*- coding: utf-8 -*-
"""Постобработка v2: сдвиг к жёлто-оливовой гамме bg.jpg + blur + гамма + градиент."""
import colorsys
import io
import os

from PIL import Image, ImageEnhance, ImageFilter

SRC = r"D:\pr\Aurora\devilutionX_auroraos\.screenshots\bg_candidates"
ASSETS = r"D:\pr\Aurora\devilutionX_auroraos\Source\platform\aurora_os\devilution_launcher\assets"
OUT = os.path.join(SRC, "previews")
os.makedirs(OUT, exist_ok=True)

TARGET_MEAN = 14.0
HUE_SHIFT = 18          # градусов к жёлтому для тёплых пикселей
WARM_LIMIT = 45         # двигаем только пиксели с hue < 45
BLUR_RADIUS = 2.2


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


def hue_shift(img, degrees, limit):
    """Поворот hue тёплых пикселей к жёлтому. PIL HSV: 0..255 == 0..360deg."""
    hsv = img.convert("HSV")
    h, s, v = hsv.split()
    shift = int(round(degrees * 255 / 360))
    lim = int(round(limit * 255 / 360))

    def trans(x):
        if x < lim:
            return min(x + shift, lim + shift)  # не заезжаем в зелёный
        return x

    return Image.merge("HSV", (h.point(trans), s, v)).convert("RGB")


def palette_stats(img, tag):
    im = img.convert("RGB").resize((256, 150))
    vis = [px for px in im.getdata() if sum(px) > 60]
    if not vis:
        return
    yellow = red = 0
    for r, g, b in vis:
        hh, ss, vv = colorsys.rgb_to_hsv(r / 255, g / 255, b / 255)
        deg = hh * 360
        if deg <= 60 or deg >= 330:
            if deg >= 330:
                deg -= 360
            if deg >= 15:
                yellow += 1
            else:
                red += 1
    sat = sum(colorsys.rgb_to_hsv(r / 255, g / 255, b / 255)[1] for r, g, b in vis) / len(vis)
    print(tag, 'visible%', round(100 * len(vis) / 38400, 1),
          'yellow%', round(100 * yellow / len(vis), 1),
          'red%', round(100 * red / len(vis), 1), 'sat', round(sat, 2))


def process(path, tag, color=0.85):
    img = Image.open(path).convert("RGB")

    img = hue_shift(img, HUE_SHIFT, WARM_LIMIT)
    img = img.filter(ImageFilter.GaussianBlur(BLUR_RADIUS))
    img = ImageEnhance.Color(img).enhance(color)

    # Гамма до целевой средней яркости (по гистограмме).
    hist = img.convert("L").histogram()
    total = sum(hist)
    lo, hi = 1.0, 12.0
    for _ in range(40):
        mid = (lo + hi) / 2
        est = sum((i / 255.0) ** mid * c for i, c in enumerate(hist)) / total * 255
        if est > TARGET_MEAN:
            lo = mid
        else:
            hi = mid
    gamma = (lo + hi) / 2
    lut = [min(255, int(round(255.0 * (i / 255.0) ** gamma))) for i in range(256)]
    img = img.point(lut * 3)

    # Затемнение к низу.
    w, h = img.size
    grad = Image.new("L", (1, h))
    for y in range(h):
        t = y / (h - 1)
        grad.putpixel((0, y), int(255 * (1.0 - 0.30 * max(0.0, (t - 0.55) / 0.45))))
    img = Image.composite(img, Image.new("RGB", img.size, (0, 0, 0)), grad.resize(img.size))

    img.save(os.path.join(OUT, "proc2_{}.jpg".format(tag)), "JPEG", quality=90)
    palette_stats(img, "proc2_" + tag)

    for orient, aspect in (("land", 16 / 9), ("port", 9 / 19.5)):
        prev = crop_fill(img, aspect)
        prev = prev.resize((1600, int(1600 / aspect)) if orient == "land" else (720, 1560))
        prev = Image.blend(prev, Image.new("RGB", prev.size, (0, 0, 0)), 0.28)
        prev.save(os.path.join(OUT, "prev2_{}_{}.jpg".format(tag, orient)), "JPEG", quality=88)


print("--- reference:")
palette_stats(Image.open(os.path.join(ASSETS, "bg.jpg")), "bg.jpg")
for tag, name, color in (("wallpan8201", "bgS_wallpan_8201.png", 0.85),
                          ("wallclose8202", "bgS_wallclose_8202.png", 1.15),
                          ("field8201", "bgS_field_8201.png", 0.48)):
    process(os.path.join(SRC, name), tag, color)
print("done ->", OUT)
