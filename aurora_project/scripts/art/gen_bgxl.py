# -*- coding: utf-8 -*-
"""Фон лаунчера на полном DreamShaper XL 1.0 Alpha2: 30 шагов, CFG 5.5."""
import base64
import io
import json
import os
import urllib.request

opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))

OUT = r"D:\pr\Aurora\devilutionX_auroraos\.screenshots\bg_candidates"
os.makedirs(OUT, exist_ok=True)

NEG = ("text, watermark, signature, logo, blurry, modern, photorealistic, "
       "people, person, characters, faces, creature, monster, skull, statue, "
       "bright colors, low quality, deformed, close-up, busy composition, "
       "bright, vivid, high contrast, daylight")

DARK = ("very dark, dim ambient, mostly deep shadows, low contrast, subdued muted palette, "
        "quiet minimal composition, empty dark center, darker at bottom")

PROMPTS = {
    "cathedral": (
        "Dark fantasy digital painting, interior of ruined gothic cathedral at night, "
        "rows of stone columns receding into fog, few small candle flames on the floor, "
        "deep browns and dark reds palette (#1A0F0A, #8B1A10), faint golden torchlight accents (#C7A568), "
        "moody atmospheric fog, no characters, no text, no focal subject, "
        + DARK + ", oil painting style, 90s dark fantasy video game background art"
    ),
    "town": (
        "Dark fantasy digital painting, empty medieval town square of Tristram at night, "
        "gothic rooftops and timber house silhouettes, distant cathedral tower, "
        "dying bonfire embers floating in air, deep browns and dark reds palette "
        "(#1A0F0A, #8B1A10), faint golden torchlight accents (#C7A568), moody atmospheric fog, "
        "no characters, no text, no focal subject, wide calm composition with atmospheric depth, "
        + DARK + ", oil painting style, 90s dark fantasy video game background art"
    ),
    "crypt": (
        "Dark fantasy digital painting, view down empty dank dungeon crypt corridor, "
        "arched stone doorways receding into darkness, sparse dying torches on stone walls, "
        "faint warm amber glow against cold darkness (#6B5D48 stone, #E8C77E firelight), "
        "subtle floating embers, no characters, no text, no focal subject, "
        "symmetrical receding composition, "
        + DARK + ", oil painting style, 90s dark fantasy game background art"
    ),
}

SEEDS = [8101, 8102]

for key, prompt in PROMPTS.items():
    for seed in SEEDS:
        payload = {
            "prompt": prompt,
            "negative_prompt": NEG,
            "width": 1344,
            "height": 768,
            "steps": 30,
            "cfg_scale": 5.5,
            "sampler_name": "DPM++ 2M Karras",
            "seed": seed,
        }
        req = urllib.request.Request(
            "http://127.0.0.1:7860/sdapi/v1/txt2img",
            data=json.dumps(payload).encode("utf-8"),
            headers={"Content-Type": "application/json"},
        )
        with opener.open(req, timeout=600) as resp:
            result = json.load(resp)
        raw = base64.b64decode(result["images"][0])
        name = "bgXL_{}_{}.png".format(key, seed)
        with io.open(os.path.join(OUT, name), "wb") as f:
            f.write(raw)
        print(name, "ok", len(raw))
print("done")
