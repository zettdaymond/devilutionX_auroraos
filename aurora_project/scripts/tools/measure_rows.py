# -*- coding: utf-8 -*-
"""Точные x-границы ярких UI-пикселей по горизонтальным полосам кадра."""
import sys
from PIL import Image

path = sys.argv[1]
img = Image.open(path).convert("RGB")
w, h = img.size
px = img.load()


def bright(r, g, b):
    return r > 120 and g > 100 and b > 60 and r >= g >= b


def band(y0, y1, label):
    xs = []
    for y in range(y0, min(y1, h)):
        for x in range(w - 45):  # без скролл-индикатора
            if bright(*px[x, y]):
                xs.append(x)
                break  # левый край строки
    if not xs:
        print("{}: пусто".format(label))
        return
    # правые края
    rights = []
    for y in range(y0, min(y1, h)):
        for x in range(w - 46, -1, -1):
            if bright(*px[x, y]):
                rights.append(x)
                break
    print("{}: текст от x={} до x={} (y {}..{})".format(label, min(xs), max(rights), y0, y1))


band(0, 60, "титул окна")
band(60, 150, "заголовок экрана")
band(150, 230, "хинт+группа Геймплей")
band(230, 700, "строки настроек")
