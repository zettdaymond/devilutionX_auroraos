import base64, json, sys, urllib.request

API = "http://127.0.0.1:7860"
opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))
ASSETS = r"D:\pr\Aurora\devilutionX_auroraos\Source\platform\aurora_os\devilution_launcher\assets"

NEGATIVE = ("text, watermark, signature, logo, blurry, modern, photorealistic, people, "
            "characters, bright colors, low quality, deformed, oversaturated")

ARTS = {
    "hero_diablo": {
        "seed": 1307,
        "prompt": (
            "Dark fantasy digital painting, medieval gothic town of Tristram at night, "
            "cathedral silhouette with single lit window, dying bonfire embers floating in air, "
            "deep browns and dark blood red palette, muted gold torchlight accents, "
            "moody atmospheric fog, no characters, no text, no logos, "
            "composition: focal point in upper center, darker at bottom, oil painting style, "
            "90s dark fantasy video game box art"
        ),
    },
    "hero_hellfire": {
        "seed": 2607,
        "prompt": (
            "Dark fantasy digital painting, infernal demon sigil glowing above ruined monastery, "
            "orange hellfire and ember storm, burnt orange and deep red accents, "
            "deep black-brown shadows, ominous heat haze, no characters, no text, no logos, "
            "composition: focal point upper right, darker bottom left for text overlay, "
            "90s dark fantasy video game box art style"
        ),
    },
    "hero_demo": {
        "seed": 3904,
        "prompt": (
            "Dark fantasy digital painting, entrance to a dank dungeon crypt, "
            "single burning torch on stone wall, warm amber glow against cold darkness, "
            "warm gray-brown stone, subtle floating embers, inviting but ominous mood, "
            "no characters, no text, no logos, "
            "composition: doorway in upper center framing darkness, 90s dark fantasy game art"
        ),
    },
}

def gen(name, spec, seed_override=None, suffix=""):
    payload = {
        "prompt": spec["prompt"],
        "negative_prompt": NEGATIVE,
        "width": 1216,
        "height": 832,
        "steps": 8,
        "cfg_scale": 2.0,
        "sampler_name": "DPM++ SDE",
        "scheduler": "karras",
        "seed": seed_override if seed_override is not None else spec["seed"],
        "save_images": False,
    }
    req = urllib.request.Request(
        API + "/sdapi/v1/txt2img",
        data=json.dumps(payload).encode(),
        headers={"Content-Type": "application/json"},
    )
    with opener.open(req, timeout=600) as r:
        result = json.load(r)
    out = rf"{ASSETS}\{name}{suffix}.png"
    with open(out, "wb") as f:
        f.write(base64.b64decode(result["images"][0]))
    print(f"saved {out} (seed={payload['seed']})")

if __name__ == "__main__":
    which = sys.argv[1:] or list(ARTS)
    for name in which:
        gen(name, ARTS[name])
