# -*- coding: utf-8 -*-
"""Diablo hero v3: трофейный 'adventuring party' вместо перечисления позиций."""
import base64
import io
import json
import os
import urllib.request

opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))

OUT = r"D:\pr\Aurora\devilutionX_auroraos\.screenshots\hero_candidates"

NEG = ("text, watermark, signature, logo, blurry, modern, photorealistic, "
       "deformed, extra limbs, bad anatomy, crowd, four people, "
       "bright colors, low quality, faces, close-up")

PROMPT = (
    "Dark fantasy digital painting, a classic adventuring party of three — "
    "an armored warrior, a rogue archer and a mage with a glowing staff — "
    "seen small from behind, walking together towards the giant arched doors "
    "of a gothic cathedral at night. The warrior's round shield hangs on his back, "
    "the archer carries a longbow. "
    "The cathedral facade towers above them into the fog, "
    "torches burn on the stone walls, dying embers float in the air, "
    "deep browns and dark reds palette (#1A0F0A, #8B1A10), golden torchlight accents (#C7A568), "
    "darker at the bottom, oil painting style, 90s dark fantasy video game box art"
)

for seed in (9121, 9122, 9123, 9124):
    payload = {
        "prompt": PROMPT,
        "negative_prompt": NEG,
        "width": 1216,
        "height": 832,
        "steps": 30,
        "cfg_scale": 6.0,
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
    name = "trio3_{}.png".format(seed)
    with io.open(os.path.join(OUT, name), "wb") as f:
        f.write(raw)
    print(name, "ok", len(raw))
print("done")
