# -*- coding: utf-8 -*-
"""img2img: заброшенная мистическая деревня и кладбище в ней."""
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
       "bright, vivid, high contrast, daylight")

TAIL = ("deep charcoal black and dark yellow-olive palette (#181800, #303018, #301818), "
        "desaturated, muted, faint dim amber glow, "
        "no characters, no text, no focal subject, very dark, dim ambient, low contrast, subdued, "
        "oil painting style, 90s dark fantasy video game background art")

PROMPTS = {
    "village": (
        "Dark fantasy digital painting, abandoned mysterious village at night, "
        "cluster of crooked old wooden houses with steep roofs, one faintly lit window, "
        "heavy fog drifting between the houses, ravens circling above the rooftops, "
        "dead gnarled trees, " + TAIL
    ),
    "cemetery": (
        "Dark fantasy digital painting, abandoned cemetery in a mysterious ruined village at night, "
        "leaning gravestones and old weathered crosses, crooked wooden house silhouettes in the fog behind, "
        "heavy fog rolling low over the graves, ravens perched on gravestones, "
        "dead gnarled tree, " + TAIL
    ),
}


def b64(path):
    with io.open(path, "rb") as f:
        return base64.b64encode(f.read()).decode("ascii")


INIT = b64(os.path.join(ASSETS, "bg.jpg"))

EXPERIMENTS = [
    ("village", 0.50, 8601),
    ("village", 0.50, 8602),
    ("village", 0.55, 8601),
    ("cemetery", 0.50, 8701),
    ("cemetery", 0.50, 8702),
    ("cemetery", 0.55, 8701),
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
