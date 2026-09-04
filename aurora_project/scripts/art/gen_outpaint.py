# -*- coding: utf-8 -*-
"""Outpainting bg.jpg без фигуры: правая часть + домеривание слева через webui."""
import base64
import io
import json
import os
import urllib.request

from PIL import Image, ImageFilter

opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))

ASSETS = r"D:\pr\Aurora\devilutionX_auroraos\Source\platform\aurora_os\devilution_launcher\assets"
SRC = r"D:\pr\Aurora\devilutionX_auroraos\.screenshots\bg_candidates"
OUT = os.path.join(SRC, "previews")
os.makedirs(OUT, exist_ok=True)

CANVAS_W, CANVAS_H = 1344, 768
CUT_FRAC = 0.47  # правее фигуры и птицы на 41-45%

NEG = ("text, watermark, signature, logo, blurry, modern, photorealistic, "
       "people, person, characters, figure, hooded figure, faces, creature, monster, skull, statue, "
       "bright colors, low quality, deformed, close-up, busy composition, "
       "bright, vivid, high contrast, daylight")

PROMPT = (
    "Dark fantasy digital painting, ruined city fortifications after a siege at night, "
    "heavy fog rolling over shattered stone battlements, distant silhouettes of ravens, "
    "deep charcoal black and dark yellow-olive palette (#181800, #303018, #301818), "
    "desaturated, muted, faint dim amber glow, "
    "no characters, no text, no focal subject, very dark, dim ambient, low contrast, subdued, "
    "oil painting style, 90s dark fantasy video game background art"
)


def b64_img(im):
    buf = io.BytesIO()
    im.save(buf, "PNG")
    return base64.b64encode(buf.getvalue()).decode("ascii")


def api_img2img(payload):
    req = urllib.request.Request(
        "http://127.0.0.1:7860/sdapi/v1/img2img",
        data=json.dumps(payload).encode("utf-8"),
        headers={"Content-Type": "application/json"},
    )
    with opener.open(req, timeout=600) as resp:
        return json.load(resp)


def outpaint(kept_w, tag, seed):
    """kept_w: ширина полосы из оригинала на канве; остальное домериваем слева."""
    orig = Image.open(os.path.join(ASSETS, "bg.jpg")).convert("RGB")
    w, h = orig.size
    kept = orig.crop((int(w * CUT_FRAC), 0, w, h))
    kw = kept_w
    kh = int(round(kept.size[1] * kw / kept.size[0]))
    kept = kept.resize((kw, kh), Image.LANCZOS)
    if kh >= CANVAS_H:
        top = (kh - CANVAS_H) // 2
        kept = kept.crop((0, top, kw, top + CANVAS_H))
    else:  # не должно случиться
        kept = kept.resize((kw, CANVAS_H), Image.LANCZOS)

    canvas = Image.new("RGB", (CANVAS_W, CANVAS_H))
    px0 = CANVAS_W - kw
    canvas.paste(kept, (px0, 0))

    # Заполнение слева: зеркальная копия края полосы, сильно размытая.
    mirror = kept.crop((0, 0, min(px0, kw), CANVAS_H)).transpose(Image.FLIP_LEFT_RIGHT)
    mirror = mirror.filter(ImageFilter.GaussianBlur(28))
    canvas.paste(mirror, (max(0, px0 - mirror.size[0]), 0))
    canvas.save(os.path.join(SRC, "outpaint_canvas_{}.png".format(tag)))

    # Маска: белым то, что регенерируем (левая зона + 14px нахлёста).
    mask = Image.new("L", (CANVAS_W, CANVAS_H), 0)
    for x in range(px0 + 14):
        for row in range(CANVAS_H):
            mask.putpixel((x, row), 255)
    mask = mask.filter(ImageFilter.GaussianBlur(6))

    payload = {
        "prompt": PROMPT,
        "negative_prompt": NEG,
        "init_images": [b64_img(canvas)],
        "mask": b64_img(mask),
        "resize_mode": 0,
        "width": CANVAS_W,
        "height": CANVAS_H,
        "steps": 30,
        "cfg_scale": 5.5,
        "sampler_name": "DPM++ 2M Karras",
        "seed": seed,
        "denoising_strength": 0.85,
        "mask_blur": 8,
        "inpainting_fill": 1,
        "inpaint_full_res": False,
    }
    result = api_img2img(payload)
    img = Image.open(io.BytesIO(base64.b64decode(result["images"][0]))).convert("RGB")

    # Проход гармонизации: сгладить шов, привести тон к общему.
    payload2 = dict(payload)
    payload2.pop("mask")
    payload2["denoising_strength"] = 0.30
    payload2["init_images"] = [b64_img(img)]
    payload2["seed"] = seed + 1
    result2 = api_img2img(payload2)
    img2 = Image.open(io.BytesIO(base64.b64decode(result2["images"][0]))).convert("RGB")

    img.save(os.path.join(SRC, "out_{}_raw.png".format(tag)))
    img2.save(os.path.join(SRC, "out_{}_harm.png".format(tag)))
    print(tag, "done: raw + harmonized")


# B: полоса во всю высоту канвы (694px), слева домериваем 650px
outpaint(694, "B650", 8401)
# C: полоса шире (1010px, лёгкий вертикальный кроп), слева домериваем 334px
outpaint(1010, "C334", 8401)

# A: контроль без генерации — правая часть на всю ширину, центр-кроп по вертикали.
orig = Image.open(os.path.join(ASSETS, "bg.jpg")).convert("RGB")
w, h = orig.size
kept = orig.crop((int(w * CUT_FRAC), 0, w, h))
scale = CANVAS_W / kept.size[0]
kh = int(round(kept.size[1] * scale))
kept = kept.resize((CANVAS_W, kh), Image.LANCZOS)
top = (kh - CANVAS_H) // 2
kept.crop((0, top, CANVAS_W, top + CANVAS_H)).save(os.path.join(SRC, "out_A_crop.png"))
print("A done")
