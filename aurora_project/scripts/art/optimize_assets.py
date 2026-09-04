"""Re-encode launcher artwork as compact palette PNGs.

JPEG is not an option: the Aurora device build of SDL_image has no JPEG
symbols (verified in the static lib), so JPG arts would not load. A
256-colour palette PNG keeps the painterly look (Floyd-Steinberg dither
hides gradient banding) while cutting file size ~60-70%. Hero arts are
also downscaled 1216->960: they are never displayed larger than ~900px
on the highest-DPI phone, and the decoded texture shrinks ~37%.
"""
import os
from PIL import Image

ASSETS = r"D:\pr\Aurora\devilutionX_auroraos\Source\platform\aurora_os\devilution_launcher\assets"

JOBS = [
    # (file, target width or None to keep size)
    ("bg.png", None),
    ("hero_diablo.png", 960),
    ("hero_hellfire.png", 960),
    ("hero_demo.png", 960),
]

for name, width in JOBS:
    path = os.path.join(ASSETS, name)
    before = os.path.getsize(path)
    im = Image.open(path).convert("RGB")
    if width is not None and im.width > width:
        im = im.resize((width, round(im.height * width / im.width)), Image.LANCZOS)
    quantized = im.quantize(colors=256, method=Image.MEDIANCUT, dither=Image.FLOYDSTEINBERG)
    quantized.save(path, optimize=True)
    after = os.path.getsize(path)
    print(f"{name}: {before//1024} KB -> {after//1024} KB  ({im.width}x{im.height}, palette)")
