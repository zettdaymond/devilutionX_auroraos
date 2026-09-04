# -*- coding: utf-8 -*-
"""Фон лаунчера: стена после осады в тумане, вороны — в палитре bg.jpg."""
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
       "bright, vivid, high contrast, daylight, sunshine, blue sky")

PALETTE = ("deep charcoal black and dark red-brown palette (#180000, #301818), "
           "desaturated olive-grey stone (#303018, #181818), faint dying ember glow")

PROMPTS = {
    "wallpan": (
        "Dark fantasy digital painting, panorama of ruined city fortifications after a siege at night, "
        "shattered stone battlements and breached gate, tattered banners, heavy fog rolling over the walls, "
        "distant silhouettes of ravens circling in the dark smoky sky, "
        + PALETTE + ", "
        "no characters, no text, no focal subject, very dark, dim ambient, low contrast, subdued, "
        "oil painting style, 90s dark fantasy video game background art"
    ),
    "wallclose": (
        "Dark fantasy digital painting, breached medieval city wall at night after a siege, "
        "rubble and broken stones, torn banner hanging from a splintered beam, thick fog drifting through, "
        "several ravens perched on the wreckage as dark silhouettes, "
        + PALETTE + ", "
        "no characters, no text, no focal subject, very dark, dim ambient, low contrast, subdued, "
        "oil painting style, 90s dark fantasy game background art"
    ),
    "field": (
        "Dark fantasy digital painting, scorched field before a ruined fortress wall at night, "
        "broken lances and shields half-buried in mud, distant breached battlements fading into heavy fog, "
        "ravens circling above, faint dying fires, "
        + PALETTE + ", "
        "no characters, no text, no focal subject, very dark, dim ambient, low contrast, subdued, "
        "oil painting style, 90s dark fantasy game background art"
    ),
}

SEEDS = [8201, 8202]

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
        name = "bgS_{}_{}.png".format(key, seed)
        with io.open(os.path.join(OUT, name), "wb") as f:
            f.write(raw)
        print(name, "ok", len(raw))
print("done")
