# Слайды для магазина, вариант 2 (1080x1920, 9:16):
#   -Kind features: светлый игровой кадр + wordmark DIABLO + список
#                   особенностей порта (Beaufort + FontAwesome-галки)
#   -Kind stack:    два полноширинных ландшафтных кадра стопкой
#                   (тонкий золотой переплёт) — «игра полноэкранная»
#   -Kind title:    одиночный кадр в рамке (титульник)
# Фон — hero_diablo с сильным затемнением (~20-25 люм): не спорит
# с кадрами, читается как тёмная подложка.
param(
	[string]$Kind,
	[string]$FrameA,
	[string]$FrameB,
	[string]$OutFile
)

Add-Type -AssemblyName System.Drawing
$assets = "D:\pr\Aurora\devilutionX_auroraos\Source\platform\aurora_os\devilution_launcher\assets"

$W = 1080
$H = 1920
$canvas = New-Object System.Drawing.Bitmap($W, $H)
$g = [System.Drawing.Graphics]::FromImage($canvas)
$g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
$g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
$g.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::AntiAlias

# --- Фон: вертикальный кроп hero_diablo + затемнение ---
$bg = [System.Drawing.Bitmap]::FromFile("$assets\hero_diablo.jpg")
$bgCropW = [int]($bg.Height * 9 / 16)
$bgX = [int](($bg.Width - $bgCropW) * 0.5)
$g.DrawImage($bg, (New-Object System.Drawing.Rectangle(0, 0, $W, $H)),
	(New-Object System.Drawing.Rectangle($bgX, 0, $bgCropW, $bg.Height)),
	[System.Drawing.GraphicsUnit]::Pixel)
$bg.Dispose()
$dim = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(150, 6, 6, 8))
$g.FillRectangle($dim, 0, 0, $W, $H)
$dim.Dispose()

function New-PrivateFont([string]$File, [float]$Px) {
	$pfc = New-Object System.Drawing.Text.PrivateFontCollection
	$pfc.AddFontFile($File)
	$font = New-Object System.Drawing.Font($pfc.Families[0], $Px,
		[System.Drawing.FontStyle]::Regular, [System.Drawing.GraphicsUnit]::Pixel)
	$pfc.Dispose()
	return $font
}

function Draw-FrameBmp([System.Drawing.Bitmap]$bmp, [int]$x, [int]$y, [int]$w, [int]$h) {
	$g.DrawImage($bmp, (New-Object System.Drawing.Rectangle($x, $y, $w, $h)),
		(New-Object System.Drawing.Rectangle(0, 0, $bmp.Width, $bmp.Height)),
		[System.Drawing.GraphicsUnit]::Pixel)
	$borderDark = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(200, 20, 18, 14), 6)
	$borderGold = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(220, 154, 134, 90), 2)
	$g.DrawRectangle($borderDark, ($x - 3), ($y - 3), ($w + 6), ($h + 6))
	$g.DrawRectangle($borderGold, ($x - 6), ($y - 6), ($w + 12), ($h + 12))
	$borderDark.Dispose()
	$borderGold.Dispose()
}

if ($Kind -eq "features") {
	# --- Wordmark DIABLO (Exocet) ---
	$wmFont = New-PrivateFont "$assets\exocet1.ttf" 150
	$fmt = New-Object System.Drawing.StringFormat
	$fmt.Alignment = [System.Drawing.StringAlignment]::Center
	$sz = $g.MeasureString("DIABLO", $wmFont, $W, $fmt)
	$wmY = [int](170 - $sz.Height / 2)
	$layout = New-Object System.Drawing.RectangleF(0, $wmY, $W, $sz.Height)
	$shadowRect = New-Object System.Drawing.RectangleF(0, ($wmY + 6), $W, $sz.Height)
	$shadow = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(220, 0, 0, 0))
	$gold = New-Object System.Drawing.Drawing2D.LinearGradientBrush(
		(New-Object System.Drawing.Rectangle(0, $wmY, $W, [int]$sz.Height)),
		[System.Drawing.Color]::FromArgb(255, 220, 192, 130),
		[System.Drawing.Color]::FromArgb(255, 150, 118, 62),
		[System.Drawing.Drawing2D.LinearGradientMode]::Vertical)
	$g.DrawString("DIABLO", $wmFont, $shadow, $shadowRect, $fmt)
	$g.DrawString("DIABLO", $wmFont, $gold, $layout, $fmt)
	$shadow.Dispose(); $gold.Dispose(); $wmFont.Dispose()

	# --- Игровой кадр (Тристрам, светлый) ---
	$bmp = [System.Drawing.Bitmap]::FromFile($FrameA)
	Draw-FrameBmp $bmp 40 300 1000 500
	$bmp.Dispose()

	# --- Список особенностей порта ---
	$features = @(
		"Полноэкранный ландшафтный режим",
		"Сенсорное управление и виртуальный геймпад",
		"Русская локализация",
		"Встроенный лаунчер и загрузка данных"
	)
	$goldText = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 216, 186, 126))
	$textShadow = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(200, 0, 0, 0))
	$checkFont = New-PrivateFont "$assets\fontawesome-webfont.ttf" 40
	$featFont = New-PrivateFont "$assets\Beaufort-Bold.ttf" 40
	$leftFmt = New-Object System.Drawing.StringFormat
	$leftFmt.Alignment = [System.Drawing.StringAlignment]::Near
	$y = 980
	foreach ($line in $features) {
		$shadowPt = New-Object System.Drawing.PointF(146, ($y + 3))
		$textPt = New-Object System.Drawing.PointF(143, $y)
		$checkShadowPt = New-Object System.Drawing.PointF(72, ($y + 3))
		$checkPt = New-Object System.Drawing.PointF(69, $y)
		$g.DrawString([char]0xF00C, $checkFont, $textShadow, $checkShadowPt, $leftFmt)
		$g.DrawString([char]0xF00C, $checkFont, $goldText, $checkPt, $leftFmt)
		$g.DrawString($line, $featFont, $textShadow, $shadowPt, $leftFmt)
		$g.DrawString($line, $featFont, $goldText, $textPt, $leftFmt)
		$y += 118
	}
	$goldText.Dispose(); $textShadow.Dispose(); $checkFont.Dispose(); $featFont.Dispose()
}
elseif ($Kind -eq "stack") {
	# Два полноширинных кадра 1080x540 стопкой; между ними тонкая
	# золотая линия-переплёт.
	$bmpA = [System.Drawing.Bitmap]::FromFile($FrameA)
	Draw-FrameBmp $bmpA 0 390 1080 540
	$bmpA.Dispose()
	$bmpB = [System.Drawing.Bitmap]::FromFile($FrameB)
	Draw-FrameBmp $bmpB 0 966 1080 540
	$bmpB.Dispose()
	$spine = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(200, 154, 134, 90), 2)
	$g.DrawLine($spine, 60, 948, 1020, 948)
	$spine.Dispose()
}
elseif ($Kind -eq "title") {
	$bmp = [System.Drawing.Bitmap]::FromFile($FrameA)
	Draw-FrameBmp $bmp 40 710 1000 500
	$bmp.Dispose()
}

$g.Dispose()
$canvas.Save($OutFile, [System.Drawing.Imaging.ImageFormat]::Png)
$canvas.Dispose()
Write-Output "saved $OutFile ($Kind)"
