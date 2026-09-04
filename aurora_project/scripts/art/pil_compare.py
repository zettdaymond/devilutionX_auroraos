from PIL import Image
import math, sys

prefix = sys.argv[1] if len(sys.argv) > 1 else 'bars_444'
w, h = 96, 64

orig = open(f'D:/pr/testquest/build/{prefix}_orig.bin','rb').read()
img = Image.open(f'D:/pr/testquest/build/{prefix}.jpg')
img = img.convert('RGB')
pix = img.tobytes()

mse = 0.0
maxd = 0
for i in range(w*h):
    for c in range(3):
        d = orig[i*3+c] - pix[i*3+c]
        mse += d*d
        maxd = max(maxd, abs(d))
mse /= w*h*3
print(f"{prefix} PIL vs orig: mse={mse:.1f} psnr={10*math.log10(255*255/mse) if mse else 999:.2f} maxDiff={maxd}")
open(f'D:/pr/testquest/build/{prefix}_pil.bin','wb').write(pix)
