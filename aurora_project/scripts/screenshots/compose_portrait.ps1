# Композиция магазинных портретных скринов 9:16 (1080x1920):
# ландшафтный игровой кадр 1440x720 на тёмном арт-фоне лаунчера.
# Использование: compose_portrait.ps1 -Frame <1440x720.png> -OutFile <out.png> [-RetouchFps]
param(
	[string]$Frame,
	[string]$OutFile,
	[switch]$RetouchFps,
	[string]$BgName = "hero_diablo",
	[switch]$Wordmark
)

Add-Type -AssemblyName System.Drawing
$assets = "D:\pr\Aurora\devilutionX_auroraos\Source\platform\aurora_os\devilution_launcher\assets"
$store = "D:\pr\Aurora\devilutionX_auroraos\.screenshots\store"

$W = 1080
$H = 1920
$canvas = New-Object System.Drawing.Bitmap($W, $H)
$g = [System.Drawing.Graphics]::FromImage($canvas)
$g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
$g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality

# 1. Фон: центральный вертикальный кроп арт-фонa (9:16 от его высоты).
$bg = [System.Drawing.Bitmap]::FromFile("$assets\$BgName.jpg")
$bgCropW = [int]($bg.Height * 9 / 16)
$bgX = [int](($bg.Width - $bgCropW) * 0.5)
$srcRect = New-Object System.Drawing.Rectangle($bgX, 0, $bgCropW, $bg.Height)
$dstRect = New-Object System.Drawing.Rectangle(0, 0, $W, $H)
$g.DrawImage($bg, $dstRect, $srcRect, [System.Drawing.GraphicsUnit]::Pixel)
$bg.Dispose()

# 2. Затемнение фона, чтобы кадр читался.
$dim = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(90, 8, 8, 10))
$g.FillRectangle($dim, 0, 0, $W, $H)
$dim.Dispose()

if ($Wordmark) {
	# Exocet-надпись поверх пустой верхней зоны (как обложка плитки лаунчера).
	$pfc = New-Object System.Drawing.Text.PrivateFontCollection
	$pfc.AddFontFile("$assets\exocet1.ttf")
	$fontFamily = $pfc.Families[0]
	$font = New-Object System.Drawing.Font($fontFamily, 150, [System.Drawing.FontStyle]::Regular, [System.Drawing.GraphicsUnit]::Pixel)
	$text = "DIABLO"
	$fmt = New-Object System.Drawing.StringFormat
	$fmt.Alignment = [System.Drawing.StringAlignment]::Center
	$sz = $g.MeasureString($text, $font, $W, $fmt)
	$wmY = [int](180 - $sz.Height / 2)
	# Центрирование требует layout-прямоугольник: с origin-точкой
	# Center-alignment выталкивает текст за левый край.
	$layout = New-Object System.Drawing.RectangleF(0, $wmY, $W, $sz.Height)
	$shadowRect = New-Object System.Drawing.RectangleF(0, ($wmY + 6), $W, $sz.Height)
	$shadow = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(220, 0, 0, 0))
	$gold = New-Object System.Drawing.Drawing2D.LinearGradientBrush(
		(New-Object System.Drawing.Rectangle(0, $wmY, $W, [int]$sz.Height)),
		[System.Drawing.Color]::FromArgb(255, 214, 184, 120),
		[System.Drawing.Color]::FromArgb(255, 150, 118, 62),
		[System.Drawing.Drawing2D.LinearGradientMode]::Vertical)
	$g.DrawString($text, $font, $shadow, $shadowRect, $fmt)
	$g.DrawString($text, $font, $gold, $layout, $fmt)
	$shadow.Dispose(); $gold.Dispose(); $font.Dispose(); $pfc.Dispose()
}

# 3. Игровой кадр: 1440x720 -> 960x480, по центру.
# Внимание: $Frame — параметр-строка; битмап называем иначе (PowerShell
# регистронезависим, $frame == $Frame).
$frameBmp = [System.Drawing.Bitmap]::FromFile($Frame)
if ($RetouchFps) {
	# Счётчик FPS: компактный блок в левом верхнем углу (замерено
	# x 0..100, y 58..98). Донор — та же тёмная стена чуть ниже.
	$patch = $frameBmp.Clone((New-Object System.Drawing.Rectangle(0, 100, 110, 44)), $frameBmp.PixelFormat)
	$gpatch = [System.Drawing.Graphics]::FromImage($frameBmp)
	$gpatch.DrawImage($patch, 0, 56)
	$gpatch.Dispose()
	$patch.Dispose()
}
$fw = 1000
$fh = 500
$fx = [int](($W - $fw) / 2)
$fy = [int](($H - $fh) / 2)
$frameRect = New-Object System.Drawing.Rectangle($fx, $fy, $fw, $fh)
$g.DrawImage($frameBmp, $frameRect, 0, 0, 1440, 720, [System.Drawing.GraphicsUnit]::Pixel)

# 4. Рамка: тёмный кантик + золотая линия (стиль Diablo-UI лаунчера).
$borderDark = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(200, 20, 18, 14), 6)
$borderGold = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(220, 154, 134, 90), 2)
$g.DrawRectangle($borderDark, $fx - 3, $fy - 3, $fw + 6, $fh + 6)
$g.DrawRectangle($borderGold, $fx - 6, $fy - 6, $fw + 12, $fh + 12)
$borderDark.Dispose()
$borderGold.Dispose()

$frameBmp.Dispose()
$g.Dispose()
$canvas.Save($OutFile, [System.Drawing.Imaging.ImageFormat]::Png)
$canvas.Dispose()
Write-Output "saved $OutFile"
