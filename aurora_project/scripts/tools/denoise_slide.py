# -*- coding: utf-8 -*-
"""Выборочный деноиз скриншотов: соль-перец/JPEG-зерно на тёмных фонах.

Пиксель, отличающийся от медианы своей окрестности 3x3 сильнее порога,
заменяется медианой. Края, текст и фактура с перепадом остаются на месте:
замене подлежат только одиночные выбросы (зерно, невидимое глазу на
живом экране, но заметное на статичном увеличенном PNG).

    python denoise_slide.py <in.png> <out.png> [--threshold 26]
"""
import argparse

import numpy as np
from PIL import Image


def median3x3(gray):
    h, w = gray.shape
    padded = np.pad(gray, 1, mode="edge")
    stack = np.empty((9, h, w), dtype=np.int16)
    idx = 0
    for dy in (-1, 0, 1):
        for dx in (-1, 0, 1):
            stack[idx] = padded[1 + dy:1 + dy + h, 1 + dx:1 + dx + w]
            idx += 1
    return np.median(stack, axis=0)


def denoise(arr, threshold):
    out = arr.copy()
    gray = arr.astype(np.int16).mean(axis=-1)
    med = median3x3(gray)
    delta = np.abs(gray - med)
    mask = delta > threshold
    # Медиана посчитана по люму; заменяем цветной пиксель взвешенно к
    # среднему окрестности (цвет зерно тоже уводит).
    for c in range(3):
        ch = arr[..., c].astype(np.int16)
        h, w = ch.shape
        padded = np.pad(ch, 1, mode="edge")
        nb = np.empty((9, h, w), dtype=np.int16)
        idx = 0
        for dy in (-1, 0, 1):
            for dx in (-1, 0, 1):
                nb[idx] = padded[1 + dy:1 + dy + h, 1 + dx:1 + dx + w]
                idx += 1
        out[..., c] = np.where(mask, np.median(nb, axis=0), ch).astype(np.uint8)
    return out, int(mask.sum())


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("src")
    ap.add_argument("out")
    ap.add_argument("--threshold", type=int, default=26)
    args = ap.parse_args()

    img = Image.open(args.src).convert("RGB")
    arr = np.asarray(img)
    result, replaced = denoise(arr, args.threshold)
    Image.fromarray(result).save(args.out)
    total = arr.shape[0] * arr.shape[1]
    print("denoised {} -> {} (replaced {} px = {:.2f}%)".format(
        args.src, args.out, replaced, 100.0 * replaced / total))


if __name__ == "__main__":
    main()
