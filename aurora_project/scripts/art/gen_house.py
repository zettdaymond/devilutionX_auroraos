# -*- coding: utf-8 -*-
"""img2img: мистический дом, вороны; ниже denoise — сильнее полагаемся на оригинал."""
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

PROMPT = (
    "Dark fantasy digital painting, mysterious abandoned house on the edge of a ruined town at night, "
    "crooked old wooden house with one faintly lit window, heavy fog, "
    "ravens perched on the roof and circling in the dark sky, dead gnarled tree beside the house, "
    "deep charcoal black and dark yellow-olive palette (#181800, #303018, #301818), "
    "desaturated, muted, faint dim amber glow, "
    "no characters, no text, no focal subject, very dark, dim ambient, low contrast, subdued, "
    "oil painting style, 90s dark fantasy video game background art"
)


def b64(path):
    with io.open(path, "rb") as f:
        return base64.b64encode(f.read()).decode("ascii")


INIT = b64(os.path.join(ASSETS, "bg.jpg"))

EXPERIMENTS = [
    ("house_d055_8501", 0.55, 8501),
    ("house_d055_8502", 0.55, 8502),
    ("house_d048_8501", 0.48, 8501),
    ("house_d042_8501", 0.42, 8501),
]

for name, denoise, seed in EXPERIMENTS:
    payload = {
        "prompt": PROMPT,
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
    with io.open(os.path.join(SRC, name + ".png"), "wb") as f:
        f.write(raw)
    print(name, "ok", len(raw))
print("done")
