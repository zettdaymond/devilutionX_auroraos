# -*- coding: utf-8 -*-
"""img2img эксперименты: bg.jpg как init + refiner pass для wallpan8201."""
import base64
import io
import json
import os
import urllib.request

opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))

SRC = r"D:\pr\Aurora\devilutionX_auroraos\.screenshots\bg_candidates"
ASSETS = r"D:\pr\Aurora\devilutionX_auroraos\Source\platform\aurora_os\devilution_launcher\assets"
os.makedirs(SRC, exist_ok=True)

NEG = ("text, watermark, signature, logo, blurry, modern, photorealistic, "
       "people, person, characters, figure, hooded figure, faces, creature, monster, skull, statue, "
       "bright colors, low quality, deformed, close-up, busy composition, "
       "bright, vivid, high contrast, daylight")

PROMPT = (
    "Dark fantasy digital painting, ruined city fortifications after a siege at night, "
    "shattered stone battlements and breached gate, heavy fog rolling over the walls, "
    "distant silhouettes of ravens circling in the dark smoky sky, dying bonfire embers, "
    "deep charcoal black and dark yellow-olive palette (#181800, #303018, #301818), "
    "desaturated, muted, faint dim amber glow, "
    "no characters, no text, no focal subject, very dark, dim ambient, low contrast, subdued, "
    "oil painting style, 90s dark fantasy video game background art"
)


def b64(path):
    with io.open(path, "rb") as f:
        return base64.b64encode(f.read()).decode("ascii")


INIT_BG = b64(os.path.join(ASSETS, "bg.jpg"))
INIT_WALLPAN = b64(os.path.join(SRC, "bgS_wallpan_8201.png"))

EXPERIMENTS = [
    ("i2i_bg075_8301", INIT_BG, 0.75, 8301),
    ("i2i_bg062_8301", INIT_BG, 0.62, 8301),
    ("i2i_bg062_8302", INIT_BG, 0.62, 8302),
    ("i2i_wallpan045_8301", INIT_WALLPAN, 0.45, 8301),
]

for name, init, denoise, seed in EXPERIMENTS:
    payload = {
        "prompt": PROMPT,
        "negative_prompt": NEG,
        "init_images": [init],
        "resize_mode": 0,
        "width": 1344,
        "height": 768,
        "steps": 30,
        "cfg_scale": 5.5,
        "sampler_name": "DPM++ 2M Karras",
        "seed": seed,
        "denoising_strength": denoise,
    }
    req = urllib.request.Request(
        "http://127.0.0.1:7860/sdapi/v1/img2img",
        data=json.dumps(payload).encode("utf-8"),
        headers={"Content-Type": "application/json"},
    )
    with opener.open(req, timeout=600) as resp:
        result = json.load(resp)
    raw = base64.b64decode(result["images"][0])
    path = os.path.join(SRC, name + ".png")
    with io.open(path, "wb") as f:
        f.write(raw)
    print(name, "ok", len(raw))
print("done")
