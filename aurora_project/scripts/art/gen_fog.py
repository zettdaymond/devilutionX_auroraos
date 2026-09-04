# -*- coding: utf-8 -*-
"""img2img: деревня и кладбище в СИЛЬНОМ тумане."""
import base64
import io
import json
import os
import urllib.request

opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))

ASSETS = r"D:\pr\Aurora\devilutionX_auroraos\Source\platform\aurora_os\devilution_launcher\assets"
SRC = r"D:\pr\Aurora\devilutionX_auroraos\.screenshots\bg_candidates"

NEG = ("text, watermark, signature, logo, blurry, modern, photorealistic, "
       "people, person, characters, figure, hooded figure, faces, creature, monster, skull, statue, "
       "bright colors, low quality, deformed, close-up, busy composition, "
       "bright, vivid, high contrast, daylight, clear sky, sharp details")

TAIL = ("deep charcoal black and dark yellow-olive palette (#181800, #303018, #301818), "
        "desaturated, muted, faint dim amber glow, "
        "no characters, no text, no focal subject, extremely dark, dim ambient, very low contrast, subdued, "
        "oil painting style, 90s dark fantasy video game background art")

PROMPTS = {
    "fogvillage": (
        "Dark fantasy digital painting, abandoned mysterious village at night swallowed by extremely dense fog, "
        "thick heavy mist, only vague crooked silhouettes of wooden houses barely visible through the fog, "
        "one faint dim lit window glowing through the mist, ravens circling above the rooftops, "
        "dead gnarled trees, " + TAIL
    ),
    "fogcemetery": (
        "Dark fantasy digital painting, abandoned cemetery in a mysterious ruined village at night "
        "swallowed by extremely dense fog, thick rolling mist, "
        "leaning gravestones and old crosses barely visible through the heavy fog, "
        "vague crooked house silhouettes dissolving in the mist, ravens perched on gravestones, "
        "dead gnarled tree, " + TAIL
    ),
}


def b64(path):
    with io.open(path, "rb") as f:
        return base64.b64encode(f.read()).decode("ascii")


INIT = b64(os.path.join(ASSETS, "bg.jpg"))

EXPERIMENTS = [
    ("fogvillage", 0.50, 8801),
    ("fogvillage", 0.50, 8802),
    ("fogcemetery", 0.50, 8901),
    ("fogcemetery", 0.50, 8902),
]

for key, denoise, seed in EXPERIMENTS:
    payload = {
        "prompt": PROMPTS[key],
        "negative_prompt": NEG,
        "init_images": [INIT],
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
    name = "{}_d{:02d}_{}".format(key, int(denoise * 100), seed)
    with io.open(os.path.join(SRC, name + ".png"), "wb") as f:
        f.write(raw)
    print(name, "ok", len(raw))
print("done")
