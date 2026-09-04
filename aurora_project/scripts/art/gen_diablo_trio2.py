# -*- coding: utf-8 -*-
"""Diablo hero v2: реквизит, читаемый со спины."""
import base64
import io
import json
import os
import urllib.request

opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))

OUT = r"D:\pr\Aurora\devilutionX_auroraos\.screenshots\hero_candidates"

NEG = ("text, watermark, signature, logo, blurry, modern, photorealistic, "
       "deformed, extra limbs, extra fingers, bad anatomy, mutated hands, "
       "four people, crowd, extra figures, "
       "bright colors, low quality, close-up faces, front view faces")

PROMPT = (
    "Dark fantasy digital painting, wide shot from behind: three small adventurers "
    "standing together in the lower center before the giant arched doors "
    "of a gothic cathedral at night. "
    "On the left an armored warrior with a large round shield slung on his back "
    "and a sword sheathed at his hip. "
    "In the middle a hooded rogue archer holding a tall curved longbow in one hand. "
    "On the right a sorcerer in dark robes with a wooden staff, "
    "its crystal tip glowing warm orange above the group. "
    "The cathedral facade towers above them into the fog, torches burn on the walls, "
    "dying embers float in the air, "
    "deep browns and dark reds palette (#1A0F0A, #8B1A10), golden torchlight accents (#C7A568), "
    "darker at the bottom, oil painting style, 90s dark fantasy video game box art"
)

for seed in (9111, 9112, 9113, 9114):
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
    name = "trio2_{}.png".format(seed)
    with io.open(os.path.join(OUT, name), "wb") as f:
        f.write(raw)
    print(name, "ok", len(raw))
print("done")
