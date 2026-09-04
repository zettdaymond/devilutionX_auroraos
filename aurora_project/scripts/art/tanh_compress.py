# -*- coding: utf-8 -*-
"""Компрессия ярких зон мягким tanh-плечом (см. assets/ART_PROMPTS.md).

V' = Vc * tanh(V / Vc)  -- монотонная кривая в HSV: тени не трогает
(при малых V почти тождественна), огни/блики мягко сатурируются к Vc.
Маски вида V*(1-k*smoothstep(t,V)) НЕмонотонны -- вокруг огней
расцветают жёлтые кольца, поэтому только tanh.

Пример (фон для магазинных карточек 9:16, Vc=0.35):
    python tanh_compress.py <src.jpg> <out.png> --vc 0.35 \
        --crop-aspect 9:16 --size 1080x1920 --color 0.90
"""
import argparse

import numpy as np
from PIL import Image, ImageEnhance, ImageFilter


def crop_to_aspect(img, aspect):
    w, h = img.size
    cur = w / h
    if cur > aspect:
        nw = int(h * aspect)
        box = ((w - nw) // 2, 0, (w + nw) // 2, h)
    else:
        nh = int(w / aspect)
        box = (0, (h - nh) // 2, w, (h + nh) // 2)
    return img.crop(box)


def to_hsv(arr):
    r, g, b = arr[..., 0], arr[..., 1], arr[..., 2]
    mx = np.maximum(np.maximum(r, g), b)
    mn = np.minimum(np.minimum(r, g), b)
    delta = mx - mn
    v = mx
    s = np.where(mx > 1e-6, delta / np.maximum(mx, 1e-6), 0.0)
    dz = np.zeros_like(delta)
    dz = np.where(delta > 1e-6, 60.0 * ((g - b) / np.where(delta > 1e-6, delta, 1.0)), dz)
    h = np.select(
        [mx == r, mx == g],
        [dz % 360.0, 60.0 * ((b - r) / np.where(delta > 1e-6, delta, 1.0) + 2.0)],
        default=60.0 * ((r - g) / np.where(delta > 1e-6, delta, 1.0) + 4.0),
    )
    return h, s, v


def warm_mask(h):
    # Красно-жёлтый сектор: >=330° и <=60° (как в palette_stats bg_post5).
    return (h >= 330.0) | (h <= 60.0)


def desat_warm(img, k):
    """Приглушает насыщенность только тёплых (красных/жёлтых) оттенков."""
    arr = np.asarray(img.convert("RGB"), dtype=np.float32) / 255.0
    h, s, v = to_hsv(arr)
    mx = v
    s_new = np.where(warm_mask(h), s * k, s)
    # Пересборка RGB из H/S'/V (канонические формулы, по секторам 0..5).
    c = s_new * v
    hp = np.clip(h / 60.0, 0.0, 5.9999)
    x = c * (1.0 - np.abs(hp % 2.0 - 1.0))
    z = np.zeros_like(c)
    idx = hp.astype(np.int64)
    r1 = np.choose(idx, [c, x, z, z, x, c])
    g1 = np.choose(idx, [x, c, c, x, z, z])
    b1 = np.choose(idx, [z, z, x, c, c, x])
    out = np.stack([r1, g1, b1], axis=-1) + (v - c)[..., None]
    return Image.fromarray((np.clip(out, 0.0, 1.0) * 255.0).astype(np.uint8))


def tanh_shoulder(img, vc):
    arr = np.asarray(img.convert("RGB"), dtype=np.float32) / 255.0
    r, g, b = arr[..., 0], arr[..., 1], arr[..., 2]
    mx = np.maximum(np.maximum(r, g), b)
    mn = np.minimum(np.minimum(r, g), b)
    delta = mx - mn
    v = mx
    # S=0 (серые) не трогаем; иначе сжимаем V и пересобираем RGB с тем же H/S
    scale = np.ones_like(v)
    lit = (delta > 1e-6) & (v > 1e-6)
    v_new = vc * np.tanh(v / vc)
    scale[lit] = v_new[lit] / v[lit]
    out = arr * scale[..., None]
    return Image.fromarray((np.clip(out, 0.0, 1.0) * 255.0).astype(np.uint8))


def warm_stats(img):
    arr = np.asarray(img.convert("RGB"), dtype=np.float32) / 255.0
    vis = arr.sum(axis=-1) > 60.0 / 255.0 * 3.0
    if not vis.any():
        return "red% - yellow% -"
    h, s, v = to_hsv(arr)
    hh = h[vis]
    red = ((hh >= 330.0) | (hh < 15.0)).mean() * 100.0
    yellow = ((hh >= 15.0) & (hh <= 60.0)).mean() * 100.0
    return "red% {:.1f} yellow% {:.1f}".format(red, yellow)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("src")
    ap.add_argument("out")
    ap.add_argument("--vc", type=float, required=True, help="потолок светов, 0..1")
    ap.add_argument("--color", type=float, default=None, help="десатурация, напр. 0.90")
    ap.add_argument("--desat-warm", type=float, default=None,
                    help="приглушение насыщенности только тёплых (330..60 deg) оттенков")
    ap.add_argument("--blur", type=float, default=None, help="гауссово размытие, px")
    ap.add_argument("--brightness", type=float, default=None, help="множитель яркости после всего")
    ap.add_argument("--midtone", type=float, default=None,
                    help="подъём средних тонов, gamma-показатель, напр. 1.12")
    ap.add_argument("--vignette", type=float, default=None,
                    help="затемнение к краям, 0..1, напр. 0.35")
    ap.add_argument("--crop-aspect", default=None, help="напр. 9:16")
    ap.add_argument("--size", default=None, help="напр. 1080x1920")
    args = ap.parse_args()

    img = Image.open(args.src).convert("RGB")
    if args.crop_aspect:
        aw, ah = (float(x) for x in args.crop_aspect.split(":"))
        img = crop_to_aspect(img, aw / ah)
    if args.size:
        sw, sh = (int(x) for x in args.size.split("x"))
        img = img.resize((sw, sh), Image.LANCZOS)
    if args.color is not None:
        img = ImageEnhance.Color(img).enhance(args.color)
    before = warm_stats(img)
    if args.desat_warm is not None:
        img = desat_warm(img, args.desat_warm)
    img = tanh_shoulder(img, args.vc)
    print("warm: before [{}] after [{}]".format(before, warm_stats(img)))
    if args.blur:
        img = img.filter(ImageFilter.GaussianBlur(args.blur))
    if args.midtone is not None:
        # Подъём средних тонов без задирания потолка: v' = v^(1/k).
        arr = np.asarray(img, dtype=np.float32) / 255.0
        arr = np.power(arr, 1.0 / args.midtone)
        img = Image.fromarray((np.clip(arr, 0.0, 1.0) * 255.0).astype(np.uint8))
    if args.vignette is not None:
        # Мягкая прямоугольная виньетка: гасит края (шпили/окна), центр живой.
        arr = np.asarray(img, dtype=np.float32) / 255.0
        hgt, wid = arr.shape[:2]
        nx = np.abs(np.linspace(-1.0, 1.0, wid))[None, :]
        ny = np.abs(np.linspace(-1.0, 1.0, hgt))[:, None]
        d = np.maximum(nx, ny)
        falloff = np.clip((d - 0.55) / 0.45, 0.0, 1.0)
        arr = arr * (1.0 - args.vignette * falloff)[..., None]
        img = Image.fromarray((np.clip(arr, 0.0, 1.0) * 255.0).astype(np.uint8))
    if args.brightness is not None:
        img = ImageEnhance.Brightness(img).enhance(args.brightness)
    img.save(args.out)
    arr = np.asarray(img.convert("L"), dtype=np.float32)
    print("saved {}: max {} p99.9 {} mean {:.1f}".format(
        args.out, int(arr.max()), int(np.percentile(arr, 99.9)), arr.mean()))


if __name__ == "__main__":
    main()
