# -*- coding: utf-8 -*-
"""Inpaint воронов в небо финалиста cemetery_d50_8702."""
import base64
import io
import json
import os
import urllib.request

from PIL import Image, ImageFilter

opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))

SRC = r"D:\pr\Aurora\devilutionX_auroraos\.screenshots\bg_candidates"

NEG = ("text, watermark, signature, logo, blurry, modern, photorealistic, "
       "people, person, characters, figure, faces, creature, monster, "
       "bright colors, low quality, deformed, close-up, high contrast, daylight")

BASE = ("Dark fantasy digital painting, abandoned cemetery in a mysterious ruined village at night, "
        "leaning gravestones and old weathered crosses, crooked wooden house silhouettes in the fog, "
        "heavy fog, deep charcoal black and dark yellow-olive palette, desaturated, muted, "
        "extremely dark, dim ambient, very low contrast, subdued, "
        "oil painting style, 90s dark fantasy video game background art, ")

PROMPTS = {
    "fly": BASE + "a few ravens: several dark birds flying and circling in the foggy night sky, distant silhouettes, no characters, no text",
    "flyperch": BASE + "a few ravens: several dark birds flying in the foggy night sky and one raven perched on a bare tree branch, distant silhouettes, no characters, no text",
}


def b64_img(im):
    buf = io.BytesIO()
    im.save(buf, "PNG")
    return base64.b64encode(buf.getvalue()).decode("ascii")


init = Image.open(os.path.join(SRC, "cemetery_d50_8702.png")).convert("RGB")
W, H = init.size


def rect(mask, x0, y0, x1, y1):
    for x in range(int(W * x0 / 100), int(W * x1 / 100)):
        for y in range(int(H * y0 / 100), int(H * y1 / 100)):
            mask.putpixel((x, y), 255)


mask_fly = Image.new("L", (W, H), 0)
rect(mask_fly, 42, 3, 78, 34)

mask_fp = Image.new("L", (W, H), 0)
rect(mask_fp, 42, 3, 78, 34)
rect(mask_fp, 10, 22, 20, 31)

for key, mask in (("fly", mask_fly), ("flyperch", mask_fp)):
    m = mask.filter(ImageFilter.GaussianBlur(5))
    payload = {
        "prompt": PROMPTS[key],
        "negative_prompt": NEG,
        "init_images": [b64_img(init)],
        "mask": b64_img(m),
        "resize_mode": 0,
        "width": W,
        "height": H,
        "steps": 30,
        "cfg_scale": 5.5,
        "sampler_name": "DPM++ 2M Karras",
        "seed": 8951,
        "denoising_strength": 0.65,
        "mask_blur": 6,
        "inpainting_fill": 1,
        "inpaint_full_res": False,
    }
    req = urllib.request.Request(
        "http://127.0.0.1:7860/sdapi/v1/img2img",
        data=json.dumps(payload).encode("utf-8"),
        headers={"Content-Type": "application/json"},
    )
    with opener.open(req, timeout=600) as resp:
        result = json.load(resp)
    raw = base64.b64decode(result["images"][0])
    name = "ravens_{}_8702".format(key)
    with io.open(os.path.join(SRC, name + ".png"), "wb") as f:
        f.write(raw)
    print(name, "ok", len(raw))
print("done")
