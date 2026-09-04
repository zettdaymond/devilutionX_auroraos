# -*- coding: utf-8 -*-
"""Финальный конвейер: пост без BLUR -> апскейл x2 (DAT) -> JPEG в assets."""
import base64
import io
import json
import os
import urllib.request

from PIL import Image, ImageEnhance

opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))

SRC = r"D:\pr\Aurora\devilutionX_auroraos\.screenshots\bg_candidates"
ASSETS = r"D:\pr\Aurora\devilutionX_auroraos\Source\platform\aurora_os\devilution_launcher\assets"
OUT = os.path.join(SRC, "previews")


def post_noblur(name):
    """Color 0.90 + градиент к низу, БЕЗ blur."""
    img = Image.open(os.path.join(SRC, name)).convert("RGB")
    img = ImageEnhance.Color(img).enhance(0.90)
    w, h = img.size
    grad = Image.new("L", (1, h))
    for y in range(h):
        t = y / (h - 1)
        grad.putpixel((0, y), int(255 * (1.0 - 0.30 * max(0.0, (t - 0.55) / 0.45))))
    img = Image.composite(img, Image.new("RGB", img.size, (0, 0, 0)), grad.resize(img.size))
    return img


def upscale_x2(img):
    buf = io.BytesIO()
    img.save(buf, "PNG")
    payload = {
        "image": base64.b64encode(buf.getvalue()).decode("ascii"),
        "resize": 2,
        "upscaler_1": "DAT x2",
        "upscaler_2": "None",
        "upscale_first": True,
    }
    req = urllib.request.Request(
        "http://127.0.0.1:7860/sdapi/v1/extra-single-image",
        data=json.dumps(payload).encode("utf-8"),
        headers={"Content-Type": "application/json"},
    )
    with opener.open(req, timeout=600) as resp:
        result = json.load(resp)
    return Image.open(io.BytesIO(base64.b64decode(result["image"]))).convert("RGB")


JOBS = [
    ("cemetery_d50_8702.png", "fin_cemetery_8702.jpg"),
    ("fogcemetery_d62_8901.png", "fin_cemetery_d62.jpg"),
    ("fogvillage_d68_8801.png", "fin_village_d68.jpg"),
]

for src_name, fin_name in JOBS:
    pre = post_noblur(src_name)
    up = upscale_x2(pre)
    print(fin_name, "upscaled to", up.size)
    up.save(os.path.join(OUT, fin_name), "JPEG", quality=90)
print("done ->", OUT)
