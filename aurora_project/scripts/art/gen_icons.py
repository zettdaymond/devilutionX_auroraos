import base64, json, sys, urllib.request

API = "http://127.0.0.1:7860"
OUT = r"C:\Users\zett\AppData\Local\Temp"
opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))

NEGATIVE = ("photorealistic, realistic, 3d render, photography, color, gray, gradient, shading, "
            "texture, scenery, background, text, watermark, signature, border, frame, "
            "multiple objects, collage, details, skin, face details")

STYLE = ("logo emblem, solid black flat silhouette on pure white background, stencil, "
         "minimalist vector icon, one color, flat 2d shape, sharp clean edges, "
         "single centered subject, no shading, no gradients")

ICONS = {
    "icon_hellfire": {
        "seed": 4711,
        "prompt": "hooded monk head and shoulders silhouette, medieval friar cowl, " + STYLE,
    },
    "icon_diablo": {
        "seed": 4712,
        "prompt": "horned demon skull silhouette, two curved horns, " + STYLE,
    },
    "icon_demo": {
        "seed": 4713,
        "prompt": "gothic pointed arch door silhouette with keyhole cutout, " + STYLE,
    },
}

def gen(name, spec, seed_override=None):
    payload = {
        "prompt": spec["prompt"],
        "negative_prompt": NEGATIVE,
        "width": 1024,
        "height": 1024,
        "steps": 12,
        "cfg_scale": 5.0,
        "sampler_name": "DPM++ SDE",
        "scheduler": "karras",
        "seed": seed_override if seed_override is not None else spec["seed"],
    }
    req = urllib.request.Request(
        API + "/sdapi/v1/txt2img",
        data=json.dumps(payload).encode(),
        headers={"Content-Type": "application/json"},
    )
    with opener.open(req, timeout=600) as r:
        result = json.load(r)
    suffix = "" if seed_override is None else f"_s{seed_override}"
    out = rf"{OUT}\{name}{suffix}_raw.png"
    with open(out, "wb") as f:
        f.write(base64.b64decode(result["images"][0]))
    print(f"saved {out} (seed={payload['seed']})")

if __name__ == "__main__":
    which = sys.argv[1:] or list(ICONS)
    for name in which:
        gen(name, ICONS[name])
