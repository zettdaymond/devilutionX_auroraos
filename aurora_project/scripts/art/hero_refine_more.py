# -*- coding: utf-8 -*-
"""Дожарка сильнее: 0.25 и 0.32 поверх DAT x2, все три hero."""
import base64
import io
import json
import os
import urllib.request

from PIL import Image

opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))

ASSETS = r"D:\pr\Aurora\devilutionX_auroraos\Source\platform\aurora_os\devilution_launcher\assets"
OUT = r"D:\pr\Aurora\devilutionX_auroraos\.screenshots\hero_upscale"

NEG = ("text, watermark, signature, logo, blurry, modern, photorealistic, "
       "people, characters, bright colors, low quality, deformed, smooth, plastic")

PROMPTS = {
    "diablo": (
        "Dark fantasy digital painting, medieval gothic town of Tristram at night, "
        "cathedral silhouette with single lit window, dying bonfire embers floating in air, "
        "deep browns and dark reds palette, golden torchlight accents, moody atmospheric fog, "
        "oil painting style with visible brushwork, 90s dark fantasy video game box art"
    ),
    "hellfire": (
        "Dark fantasy digital painting, infernal demon sigil glowing above ruined monastery, "
        "orange hellfire and ember storm, deep black-brown shadows, ominous heat haze, "
        "oil painting style with visible brushwork, 90s dark fantasy video game box art style"
    ),
    "demo": (
        "Dark fantasy digital painting, entrance to a dank dungeon crypt, single burning torch "
        "on stone wall, warm amber glow against cold darkness, subtle floating embers, "
        "oil painting style with visible brushwork, 90s dark fantasy game art"
    ),
}

SEEDS = {"diablo": 1307, "hellfire": 2607, "demo": 5111}


def b64_img(im):
    buf = io.BytesIO()
    im.save(buf, "PNG")
    return base64.b64encode(buf.getvalue()).decode("ascii")


def refine(im, prompt, seed, denoise):
    payload = {
        "prompt": prompt, "negative_prompt": NEG,
        "init_images": [b64_img(im)], "resize_mode": 0,
        "width": im.size[0], "height": im.size[1],
        "steps": 20, "cfg_scale": 4.0, "sampler_name": "DPM++ 2M Karras",
        "seed": seed, "denoising_strength": denoise,
    }
    req = urllib.request.Request(
        "http://127.0.0.1:7860/sdapi/v1/img2img",
        data=json.dumps(payload).encode("utf-8"),
        headers={"Content-Type": "application/json"},
    )
    with opener.open(req, timeout=900) as resp:
        result = json.load(resp)
    return Image.open(io.BytesIO(base64.b64decode(result["images"][0]))).convert("RGB")


for key in ("diablo", "hellfire", "demo"):
    base = Image.open(os.path.join(OUT, "%s_A_dat2x.jpg" % key)).convert("RGB")
    for dn in (0.25, 0.32):
        img = refine(base, PROMPTS[key], SEEDS[key], dn)
        img.save(os.path.join(OUT, "%s_C_refine%02d.jpg" % (key, int(dn * 100))), "JPEG", quality=90)
        print(key, dn, "ok")
print("done")
