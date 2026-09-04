import sys
from PIL import Image

path = sys.argv[1]
img = Image.open(path).convert("RGB")
w, h = img.size
px = img.load()

# Заголовок окна занимает верхние ~39px (рамка окна). Рабочая область ниже.
TOP = 39
vh = h - TOP

def band_stats(y0, y1):
    gold = 0      # яркие золотые пиксели (wordmark, заливка бара)
    dim_gold = 0  # приглушённое золото (рамка бара)
    light = 0     # светлый текст (имя файла)
    grey = 0      # серый текст (подпись, статус)
    for y in range(max(0, y0), min(h, y1)):
        for x in range(0, w, 2):
            r, g, b = px[x, y]
            if r > 190 and 130 < g < 210 and b < 130:
                gold += 1
            elif 120 < r < 190 and 90 < g < 160 and b < 120:
                dim_gold += 1
            elif r > 200 and g > 190 and b > 160:
                light += 1
            elif 120 < r < 200 and 110 < g < 200 and b > 100 and abs(r - g) < 40 and abs(g - b) < 40:
                grey += 1
    return gold, dim_gold, light, grey

# Сканируем полосами по 5% высоты рабочей области
print(f"size={w}x{h}, work area y>={TOP}")
for i in range(20):
    y0 = TOP + int(vh * i / 20)
    y1 = TOP + int(vh * (i + 1) / 20)
    gold, dim_gold, light, grey = band_stats(y0, y1)
    marks = []
    if gold > 30:
        marks.append(f"GOLD={gold}")
    if dim_gold > 30:
        marks.append(f"dimgold={dim_gold}")
    if light > 30:
        marks.append(f"light={light}")
    if grey > 30:
        marks.append(f"grey={grey}")
    if marks:
        print(f"band {i:2d} y[{y0:4d}..{y1:4d}] ({i*5:3d}-{(i+1)*5:3d}%): {' '.join(marks)}")

# Полоса прогресса: ищем строку с длинным горизонтальным забегом золотых пикселей
for y in range(TOP, h):
    run = 0
    best = 0
    for x in range(w):
        r, g, b = px[x, y]
        if r > 140 and g > 100 and b < 120:
            run += 1
            best = max(best, run)
        else:
            run = 0
    if best > w * 0.05:
        print(f"bar row y={y}: max gold run={best}px ({100.0*best/w:.1f}% of width)")
