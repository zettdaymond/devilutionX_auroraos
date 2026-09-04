# -*- coding: utf-8 -*-
"""Генерация кандидатов фона лаунчера через A1111 API (без прокси)."""
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
       "bright colors, low quality, deformed, close-up, busy composition")

PROMPTS = {
    # Собор изнутри: колонны уходят в туман, свечи, центр пустой и тёмный.
    "cathedral": (
        "Dark fantasy digital painting, interior of ruined gothic cathedral at night, "
        "rows of stone columns receding into fog, scattered small candle flames on the floor, "
        "deep browns and dark reds palette (#1A0F0A, #8B1A10), golden torchlight accents (#C7A568), "
        "moody atmospheric fog, no characters, no text, no focal subject, "
        "even quiet atmospheric composition, empty dark center, darker at bottom, "
        "oil painting style, 90s dark fantasy video game background art"
    ),
    # Площадь Тристрама: силуэты крыш, соборная башня вдали, угольки.
    "town": (
        "Dark fantasy digital painting, empty medieval town square of Tristram at night, "
        "gothic rooftops and timber house silhouettes, distant cathedral tower, "
        "dying bonfire embers floating in air, deep browns and dark reds palette "
        "(#1A0F0A, #8B1A10), golden torchlight accents (#C7A568), moody atmospheric fog, "
        "no characters, no text, no focal subject, wide calm composition with atmospheric depth, "
        "empty dark center, darker at bottom, oil painting style, 90s dark fantasy video game background art"
    ),
    # Крипта-коридор: арки в темноту, редкие факелы (перекликается с demo-артом).
    "crypt": (
        "Dark fantasy digital painting, view down empty dank dungeon crypt corridor, "
        "arched stone doorways receding into darkness, sparse burning torches on stone walls, "
        "warm amber glow against cold darkness (#6B5D48 stone, #E8C77E firelight), "
        "subtle floating embers, no characters, no text, no focal subject, "
        "symmetrical receding composition, empty dark center, darker at bottom, "
        "oil painting style, 90s dark fantasy game background art"
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
            "steps": 8,
            "cfg_scale": 2.0,
            "sampler_name": "DPM++ SDE Karras",
            "seed": seed,
        }
        req = urllib.request.Request(
            "http://127.0.0.1:7860/sdapi/v1/txt2img",
            data=json.dumps(payload).encode("utf-8"),
            headers={"Content-Type": "application/json"},
        )
        with opener.open(req, timeout=300) as resp:
            result = json.load(resp)
        raw = base64.b64decode(result["images"][0])
        name = "bg_{}_{}.png".format(key, seed)
        with io.open(os.path.join(OUT, name), "wb") as f:
            f.write(raw)
        info = result.get("info", "")
        print(name, "ok", len(raw))
print("done")
