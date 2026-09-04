"""Convert A1111 silhouette drafts into flat duotone icons (v3).

Common: threshold -> border flood fill = background. Then per mode:
  - "solid": fill every enclosed bright region; punch features found by
    strategy (see below).
  - "holes": keep large enclosed bright regions as cutouts.
Feature strategies:
  - skull: detached dark components inside the figure (sockets etc. drawn
    as black fills within white areas) mark where to punch ellipses;
    fallback = geometric bands relative to the body bbox.
  - monk: the largest enclosed bright region (the face in the drafts)
    anchors two oval eye cutouts.
  - demo: deterministic keyhole punch.
Output: gold RGBA on transparent, 512x512, LANCZOS anti-aliased.
"""
import sys
from collections import deque

import numpy as np
from PIL import Image

GOLD = (232, 199, 126)  # #E8C77E GoldBright


def _flood_from_border(bright):
    h, w = bright.shape
    outside = np.zeros((h, w), dtype=bool)
    queue = deque()
    for y in range(h):
        for x in (0, w - 1):
            if bright[y, x] and not outside[y, x]:
                outside[y, x] = True
                queue.append((y, x))
    for x in range(w):
        for y in (0, h - 1):
            if bright[y, x] and not outside[y, x]:
                outside[y, x] = True
                queue.append((y, x))
    while queue:
        y, x = queue.popleft()
        if y > 0 and bright[y - 1, x] and not outside[y - 1, x]:
            outside[y - 1, x] = True
            queue.append((y - 1, x))
        if y + 1 < h and bright[y + 1, x] and not outside[y + 1, x]:
            outside[y + 1, x] = True
            queue.append((y + 1, x))
        if x > 0 and bright[y, x - 1] and not outside[y, x - 1]:
            outside[y, x - 1] = True
            queue.append((y, x - 1))
        if x + 1 < w and bright[y, x + 1] and not outside[y, x + 1]:
            outside[y, x + 1] = True
            queue.append((y, x + 1))
    return outside


def _components(mask, min_area):
    """Yields (area, ys, xs-bbox-slices) of 4-connected components."""
    h, w = mask.shape
    visited = np.zeros((h, w), dtype=bool)
    for y0, x0 in np.argwhere(mask):
        if visited[y0, x0]:
            continue
        comp = []
        queue = deque([(y0, x0)])
        visited[y0, x0] = True
        while queue:
            y, x = queue.popleft()
            comp.append((y, x))
            for ny, nx in ((y - 1, x), (y + 1, x), (y, x - 1), (y, x + 1)):
                if 0 <= ny < h and 0 <= nx < w and mask[ny, nx] and not visited[ny, nx]:
                    visited[ny, nx] = True
                    queue.append((ny, nx))
        if len(comp) >= min_area:
            ys = [c[0] for c in comp]
            xs = [c[1] for c in comp]
            yield len(comp), slice(min(ys), max(ys) + 1), slice(min(xs), max(xs) + 1)


def _ellipse(h, w, cx, cy, rx, ry):
    yy, xx = np.mgrid[0:h, 0:w]
    return ((xx - cx) / rx) ** 2 + ((yy - cy) / ry) ** 2 <= 1.0


def process(src, dst, mode, size=512, out=GOLD, hole_min=150):
    lum = np.asarray(Image.open(src).convert("L"), dtype=np.uint8)
    h, w = lum.shape

    border = np.concatenate([lum[0, :], lum[-1, :], lum[:, 0], lum[:, -1]])
    bg = float(np.median(border))
    if bg < 128:
        lum = 255 - lum
        bg = 255 - bg
    thr = max(60, int(bg - 30))

    bright = lum >= thr
    outside = _flood_from_border(bright)
    enclosed = bright & ~outside
    body = ~outside  # dark + enclosed bright (pre-fill)

    holes = np.zeros((h, w), dtype=bool)
    punch = np.zeros((h, w), dtype=bool)
    notes = []

    if mode == "solid":
        notes.append("solid")

    elif mode == "holes":
        # Keep large enclosed bright regions as cutouts.
        for area, ys, xs in _components(enclosed, hole_min):
            comp = enclosed[ys, xs]
            holes[ys, xs] |= comp
        notes.append(f"kept holes area>={hole_min}")

    elif mode == "skull":
        # Компенсируем вытянутую вниз челюсть драфта: вертикальное сжатие
        # всей фигуры делает череп компактнее (пропорции «ширина ≈ высота»).
        squash = 0.84
        rows = (np.arange(h) / squash).astype(int)
        rows = rows[rows < h]
        body = body[rows]
        holes = holes[rows]
        punch = punch[rows]
        h = len(rows)
        notes.append(f"squash {squash}")

    elif mode == "monk":
        # Solid figure; the largest enclosed bright region is the face —
        # anchor two oval eye cutouts inside it.
        face = max(_components(enclosed, hole_min * 4), key=lambda c: c[0], default=None)
        ys, xs = np.where(body)
        y0, y1, x0, x1 = ys.min(), ys.max() + 1, xs.min(), xs.max() + 1
        bw, bh = x1 - x0, y1 - y0
        if face is not None:
            _, fys, fxs = face
            fw = fxs.stop - fxs.start
            fh = fys.stop - fys.start
            fcx = (fxs.start + fxs.stop - 1) * 0.5
            fcy = fys.start + fh * 0.42
            erx, ery = fw * 0.105, max(fh * 0.07, 6.0)
            sep = fw * 0.20
            notes.append("face-anchored eyes")
        else:
            fcx = x0 + bw * 0.5
            fcy = y0 + bh * 0.30
            erx, ery = bw * 0.06, bh * 0.035
            sep = bw * 0.115
            notes.append("fallback eyes")
        for sgn in (-1, 1):
            punch |= _ellipse(h, w, fcx + sgn * sep, fcy, erx, ery)

    elif mode == "demo":
        ys, xs = np.where(body)
        y0, y1, x0, x1 = ys.min(), ys.max() + 1, xs.min(), xs.max() + 1
        bw, bh = x1 - x0, y1 - y0
        kx = x0 + bw * 0.5
        ky = y0 + bh * 0.58
        r = bw * 0.055
        yy, xx = np.mgrid[0:h, 0:w]
        circle = (xx - kx) ** 2 + (yy - ky) ** 2 <= r * r
        slot_h = bh * 0.11
        slot_top = ky + r * 0.4
        spread = ((yy - slot_top) / slot_h).clip(0.0, 1.0)
        half_w = bw * 0.028 + spread * bw * 0.028
        slot = (yy >= slot_top) & (yy <= slot_top + slot_h) & (np.abs(xx - kx) <= half_w)
        punch |= circle | slot
        notes.append("keyhole")

    alpha_mask = body & ~holes & ~punch
    # Drop stray detached strokes from the draft: keep the main mass only.
    main = max(_components(alpha_mask, 1), key=lambda c: c[0], default=None)
    if main is not None:
        clean = np.zeros_like(alpha_mask)
        _, mys, mxs = main
        clean[mys, mxs] |= alpha_mask[mys, mxs]
        alpha_mask = clean
    if not alpha_mask.any():
        raise SystemExit(f"{src}: empty silhouette")

    # Crop, pad square, downscale — LANCZOS anti-aliases the binary mask.
    ys, xs = np.where(alpha_mask)
    y0, y1, x0, x1 = ys.min(), ys.max() + 1, xs.min(), xs.max() + 1
    bw, bh = x1 - x0, y1 - y0
    side = max(bw, bh)
    margin = int(side * 0.08) + 1
    side += 2 * margin
    canvas = np.zeros((side, side), dtype=np.uint8)
    oy = (side - bh) // 2
    ox = (side - bw) // 2
    canvas[oy:oy + bh, ox:ox + bw] = alpha_mask[y0:y1, x0:x1] * 255

    alpha = Image.fromarray(canvas).resize((size, size), Image.LANCZOS)
    rgb = Image.new("RGBA", (size, size), out + (0,))
    rgb.putalpha(alpha)
    rgb.save(dst)
    print(f"{dst}: {', '.join(notes)}, body {alpha_mask.mean() * 100:.1f}%")


if __name__ == "__main__":
    tmp = r"C:\Users\zett\AppData\Local\Temp"
    assets = r"D:\pr\Aurora\devilutionX_auroraos\Source\platform\aurora_os\devilution_launcher\assets"
    jobs = [
        ("icon_hellfire", r"icon_hellfire_raw.png", "solid"),
        ("icon_diablo", r"icon_diablo_s4826_raw.png", "skull"),
        ("icon_demo", r"icon_demo_raw.png", "demo"),
    ]
    for name, raw, mode in jobs:
        process(rf"{tmp}\{raw}", rf"{assets}\{name}.png", mode)
