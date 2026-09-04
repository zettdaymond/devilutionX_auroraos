# -*- coding: utf-8 -*-
"""Hero-арты на полном DreamShaper XL 1.0 Alpha2: 30 шагов, CFG 5.5."""
import base64
import io
import json
import os
import urllib.request

opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))

OUT = r"D:\pr\Aurora\devilutionX_auroraos\.screenshots\hero_candidates"
os.makedirs(OUT, exist_ok=True)

NEG = ("text, watermark, signature, logo, blurry, modern, photorealistic, "
       "people, characters, bright colors, low quality, deformed")

PROMPTS = {
    "diablo": (
        "Dark fantasy digital painting, medieval gothic town of Tristram at night, "
        "cathedral silhouette with single lit window, dying bonfire embers floating in air, "
        "deep browns and dark reds palette (#1A0F0A, #8B1A10), golden torchlight accents (#C7A568), "
        "moody atmospheric fog, no characters, no text, no logos, "
        "composition: focal point in upper center, darker at bottom, oil painting style, "
        "90s dark fantasy video game box art"
    ),
    "hellfire": (
        "Dark fantasy digital painting, infernal demon sigil glowing above ruined monastery, "
        "orange hellfire and ember storm (#B3621A, #8B1A10 accents), deep black-brown shadows, "
        "ominous heat haze, no characters, no text, no logos, "
        "composition: focal point upper right, darker bottom left for text overlay, "
        "90s dark fantasy video game box art style"
    ),
    "demo": (
        "Dark fantasy digital painting, entrance to a dank dungeon crypt, single burning torch "
        "on stone wall, warm amber glow against cold darkness (#6B5D48 stone, #E8C77E firelight), "
        "subtle floating embers, inviting but ominous mood, no characters, no text, no logos, "
        "composition: doorway in upper center framing darkness, 90s dark fantasy game art"
    ),
}

SEEDS = {
    "diablo": [1307, 1317],
    "hellfire": [2607, 2617],
    "demo": [5111, 5121],
}

for key, seeds in SEEDS.items():
    for seed in seeds:
        payload = {
            "prompt": PROMPTS[key],
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
        name = "hero_{}_{}.png".format(key, seed)
        with io.open(os.path.join(OUT, name), "wb") as f:
            f.write(raw)
        print(name, "ok", len(raw))
print("done ->", OUT)
