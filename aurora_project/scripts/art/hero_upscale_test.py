# -*- coding: utf-8 -*-
"""Hero-арты: DAT x2 и вариант с дожаркой img2img 0.18 против резиновости."""
import base64
import io
import json
import os
import urllib.request

from PIL import Image, ImageFilter, ImageOps

opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))

ASSETS = r"D:\pr\Aurora\devilutionX_auroraos\Source\platform\aurora_os\devilution_launcher\assets"
OUT = r"D:\pr\Aurora\devilutionX_auroraos\.screenshots\hero_upscale"
os.makedirs(OUT, exist_ok=True)

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


def api(url, payload):
    req = urllib.request.Request(
        "http://127.0.0.1:7860" + url,
        data=json.dumps(payload).encode("utf-8"),
        headers={"Content-Type": "application/json"},
    )
    with opener.open(req, timeout=900) as resp:
        return json.load(resp)


def b64_img(im):
    buf = io.BytesIO()
    im.save(buf, "PNG")
    return base64.b64encode(buf.getvalue()).decode("ascii")


def upscale_dat(im):
    result = api("/sdapi/v1/extra-single-image", {
        "image": b64_img(im), "resize": 2, "upscaler_1": "DAT x2",
        "upscaler_2": "None", "upscale_first": True,
    })
    return Image.open(io.BytesIO(base64.b64decode(result["image"]))).convert("RGB")


def refine(im, prompt, seed):
    """Лёгкая дожарка микротекстуры: denoise 0.18 на полном чекпоинте."""
    result = api("/sdapi/v1/img2img", {
        "prompt": prompt, "negative_prompt": NEG,
        "init_images": [b64_img(im)], "resize_mode": 0,
        "width": im.size[0], "height": im.size[1],
        "steps": 20, "cfg_scale": 4.0, "sampler_name": "DPM++ 2M Karras",
        "seed": seed, "denoising_strength": 0.18,
    })
    return Image.open(io.BytesIO(base64.b64decode(result["images"][0]))).convert("RGB")


def edge_energy(im):
    g = ImageOps.grayscale(im)
    e = g.filter(ImageFilter.FIND_EDGES)
    w, h = e.size
    return sum(e.getdata()) / (w * h)


for key in ("diablo", "hellfire", "demo"):
    src = Image.open(os.path.join(ASSETS, "hero_%s.jpg" % key)).convert("RGB")
    e0 = edge_energy(src)

    up = upscale_dat(src)
    up.save(os.path.join(OUT, "%s_A_dat2x.jpg" % key), "JPEG", quality=90)

    ref = refine(up, PROMPTS[key], seed=1307 if key == "diablo" else (2607 if key == "hellfire" else 5111))
    ref.save(os.path.join(OUT, "%s_B_dat2x_refine.jpg" % key), "JPEG", quality=90)

    print("%-9s edges: src %.2f -> dat2x %.2f -> refine %.2f  (%dx%d)" % (
        key, e0, edge_energy(up), edge_energy(ref), up.size[0], up.size[1]))
print("done ->", OUT)
