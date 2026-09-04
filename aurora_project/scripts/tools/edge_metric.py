"""Objective icon-edge smoothness metric for launcher screenshots.

Finds golden icon clusters (the tile silhouettes), then measures the
boundary ring: fraction of ring pixels with intermediate luminance
between the icon fill and the chip background. ~>=12% means the edge is
anti-aliased; near 0% means binary stair-steps.
"""
import sys
from collections import deque

import numpy as np
from PIL import Image


def clusters(mask, min_area, max_area):
    h, w = mask.shape
    visited = np.zeros_like(mask, dtype=bool)
    out = []
    for y0, x0 in np.argwhere(mask):
        if visited[y0, x0]:
            continue
        comp = []
        queue = deque([(y0, x0)])
        visited[y0, x0] = True
        while queue:
            y, x = queue.popleft()
            comp.append((y, x))
            for ny, nx in ((y-1,x),(y+1,x),(y,x-1),(y,x+1)):
                if 0 <= ny < h and 0 <= nx < w and mask[ny, nx] and not visited[ny, nx]:
                    visited[ny, nx] = True
                    queue.append((ny, nx))
        if min_area <= len(comp) <= max_area:
            ys = [c[0] for c in comp]
            xs = [c[1] for c in comp]
            out.append((np.array(comp), min(ys), max(ys), min(xs), max(xs)))
    return out


def ring_smoothness(lum, comp_mask):
    # кольцо: маска, расширенная на 2, минус маска, сжатая на 1
    dil = comp_mask.copy()
    for _ in range(2):
        dil = dil | np.roll(dil, 1, 0) | np.roll(dil, -1, 0) | np.roll(dil, 1, 1) | np.roll(dil, -1, 1)
    ero = comp_mask.copy()
    for _ in range(1):
        nxt = ero.copy()
        nxt &= np.roll(ero, 1, 0) & np.roll(ero, -1, 0) & np.roll(ero, 1, 1) & np.roll(ero, -1, 1)
        ero = nxt
    ring = dil & ~ero
    vals = lum[ring]
    if len(vals) < 20:
        return None
    lo, hi = np.percentile(lum[comp_mask], 80), np.percentile(lum[~dil & (lum >= 0)], 10)
    lo = np.percentile(vals, 95)
    hi = np.percentile(vals, 5)
    if hi >= lo:
        return 0.0
    mid = ((vals > hi + (lo - hi) * 0.25) & (vals < lo - (lo - hi) * 0.25)).mean()
    return float(mid)


for path in sys.argv[1:]:
    a = np.asarray(Image.open(path).convert('RGB'), dtype=int)
    lum = a.sum(axis=2) / 3.0
    r, g, b = a[:,:,0], a[:,:,1], a[:,:,2]
    gold = (r > 110) & (g > 80) & (b < 120) & (r > b + 40)
    h, w = gold.shape
    lower = np.zeros_like(gold)
    lower[int(h*0.35):, :] = True
    found = []
    for comp, y0, y1, x0, x1 in clusters(gold & lower, 120, 9000):
        side = max(y1-y0+1, x1-x0+1)
        if 14 <= side <= 160:
            m = np.zeros_like(gold)
            m[comp[:,0], comp[:,1]] = True
            s = ring_smoothness(lum, m)
            if s is not None:
                found.append((y0, x0, side, s))
    if found:
        vals = [f"{s:.0%}" for *_ , s in found]
        print(f"{path.split(chr(92))[-1]}: {len(found)} icons, edge-mid shades: {vals}")
    else:
        print(f"{path.split(chr(92))[-1]}: no icons found")
