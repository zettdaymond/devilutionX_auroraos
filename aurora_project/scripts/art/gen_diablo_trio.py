# -*- coding: utf-8 -*-
"""Diablo hero: три героя (воин/лучница/маг) готовятся войти в собор."""
import base64
import io
import json
import os
import urllib.request

opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))

OUT = r"D:\pr\Aurora\devilutionX_auroraos\.screenshots\hero_candidates"
os.makedirs(OUT, exist_ok=True)

NEG = ("text, watermark, signature, logo, blurry, modern, photorealistic, "
       "deformed, extra limbs, extra fingers, bad anatomy, mutated hands, "
       "bright colors, low quality, close-up faces, front view faces")

PROMPT = (
    "Dark fantasy digital painting, three adventurers seen from behind, "
    "preparing to enter a massive gothic cathedral at night: "
    "an armored warrior with sword and shield, a hooded rogue archer with longbow, "
    "a sorcerer in robes holding a glowing staff. "
    "Tower over them the huge arched cathedral doors and facade, torches burning on the walls, "
    "dying bonfire embers floating in the air, "
    "deep browns and dark reds palette (#1A0F0A, #8B1A10), golden torchlight accents (#C7A568), "
    "moody atmospheric fog, "
    "composition: heroes small in the lower center, cathedral looming above them, darker at bottom, "
    "oil painting style, 90s dark fantasy video game box art"
)

for seed in (9101, 9102, 9103, 9104):
    payload = {
        "prompt": PROMPT,
        "negative_prompt": NEG,
        "width": 1216,
        "height": 832,
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
    name = "trio_{}.png".format(seed)
    with io.open(os.path.join(OUT, name), "wb") as f:
        f.write(raw)
    print(name, "ok", len(raw))
print("done")
