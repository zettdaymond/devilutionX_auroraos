# -*- coding: utf-8 -*-
"""Постобработка кандидатов фона: под профиль темноты bg.jpg + превью как в лаунчере."""
import io
import os

from PIL import Image, ImageEnhance

SRC = r"D:\pr\Aurora\devilutionX_auroraos\.screenshots\bg_candidates"
ASSETS = r"D:\pr\Aurora\devilutionX_auroraos\Source\platform\aurora_os\devilution_launcher\assets"
OUT = os.path.join(SRC, "previews")
os.makedirs(OUT, exist_ok=True)

bg = Image.open(os.path.join(ASSETS, "bg.jpg")).convert("L")
bg_data = sorted(bg.getdata())
n = len(bg_data)
bg_mean = sum(bg_data) / n
print("bg.jpg mean", round(bg_mean, 1),
      "p50", bg_data[n // 2], "p90", bg_data[int(n * 0.9)], "p99", bg_data[int(n * 0.99)])

TARGET_MEAN = 14.0  # чуть светлее bg.jpg: заливка кодом ещё затемнит на 28%


def crop_fill(img, aspect):
    """Aspect-fill кроп по центру, как RenderBackground."""
    w, h = img.size
    cur = w / h
    if cur > aspect:
        nw = int(h * aspect)
        box = ((w - nw) // 2, 0, (w + nw) // 2, h)
    else:
        nh = int(w / aspect)
        box = (0, (h - nh) // 2, w, (h + nh) // 2)
    return img.crop(box)


def process(path, tag):
    img = Image.open(path).convert("RGB")
    g = img.convert("L")
    data = list(g.getdata())
    m = sum(data) / len(data)

    # Гамма, приводящая среднюю яркость к TARGET_MEAN.
    lo, hi = 1.0, 12.0
    for _ in range(40):
        mid = (lo + hi) / 2
        mm = sum((v / 255.0) ** mid for v in data[:0]) # placeholder skip
        # быстрая оценка по гистограмме из 256 корзин
        hist = [0] * 256
        for v in data:
            hist[v] += 1
        est = sum((i / 255.0) ** mid * c for i, c in enumerate(hist)) / len(data) * 255
        if est > TARGET_MEAN:
            lo = mid
        else:
            hi = mid
    gamma = (lo + hi) / 2
    lut = [min(255, int(round(255.0 * (i / 255.0) ** gamma))) for i in range(256)]
    img = img.point(lut * 3)

    # Лёгкая десатурация и затемнение к низу (там текст/кнопки).
    img = ImageEnhance.Color(img).enhance(0.82)
    w, h = img.size
    grad = Image.new("L", (1, h))
    for y in range(h):
        t = y / (h - 1)
        grad.putpixel((0, y), int(255 * (1.0 - 0.30 * max(0.0, (t - 0.55) / 0.45))))
    black = Image.new("RGB", img.size, (0, 0, 0))
    img = Image.composite(img, black, grad.resize(img.size))

    full = os.path.join(OUT, "proc_{}.jpg".format(tag))
    img.save(full, "JPEG", quality=90)

    # Превью: кроп под окно + шейд 28% как в RenderBackground.
    for orient, aspect in (("land", 16 / 9), ("port", 9 / 19.5)):
        prev = crop_fill(img, aspect).resize((1600, int(1600 / aspect)) if orient == "land" else (720, 1560))
        shade = Image.new("RGB", prev.size, (0, 0, 0))
        prev = Image.blend(prev, shade, 0.28)
        prev.save(os.path.join(OUT, "prev_{}_{}.jpg".format(tag, orient)), "JPEG", quality=88)

    out_mean = sum(img.convert("L").getdata()) / (img.size[0] * img.size[1])
    print(tag, "mean", round(m, 1), "-> gamma", round(gamma, 2), "-> mean", round(out_mean, 1))


for tag, name in (("wallpan8201", "bgS_wallpan_8201.png"),
                  ("wallclose8201", "bgS_wallclose_8201.png"),
                  ("field8201", "bgS_field_8201.png"),
                  ("wallpan8202", "bgS_wallpan_8202.png"),
                  ("wallclose8202", "bgS_wallclose_8202.png"),
                  ("field8202", "bgS_field_8202.png")):
    process(os.path.join(SRC, name), tag)
print("done ->", OUT)
