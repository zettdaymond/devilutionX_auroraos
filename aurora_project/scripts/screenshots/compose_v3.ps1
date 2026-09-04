# Слайды для магазина, вариант 3 (1080x1920, 9:16) — плоская раскладка
# по референсу layout.png: без рамок и декора, кадры ~78% ширины,
# равноудалённые отступы, короткий CAPS-заголовок снизу слева.
# Фон — заранее прогнанный через tanh-плечо store_bg.png (scripts/art).
#   -Kind pair:  два ландшафтных кадра стопкой + подпись (-Line1/-Line2)
#   -Kind title: одиночный кадр по центру, без подписи
param(
	[string]$Kind,
	[string]$FrameA,
	[string]$FrameB,
	[string]$Line1,
	[string]$Line2,
	[string]$OutFile,
	[string]$Bg,
	[switch]$RetouchFps
)

Add-Type -AssemblyName System.Drawing
$repo = "D:\pr\Aurora\devilutionX_auroraos"
$assets = "$repo\Source\platform\aurora_os\devilution_launcher\assets"
$bgPath = if ($Bg) { $Bg } else { "$repo\.screenshots\store_bg.png" }

$W = 1080
$H = 1920
$canvas = New-Object System.Drawing.Bitmap($W, $H)
$g = [System.Drawing.Graphics]::FromImage($canvas)
$g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
$g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
$g.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::AntiAlias

# $Bg — типизированный параметр-строка; битмап называем иначе (PowerShell
# регистронезависим, присваивание в typed-переменную молча кастует к строке).
$bgBmp = [System.Drawing.Bitmap]::FromFile($bgPath)
$g.DrawImage($bgBmp, (New-Object System.Drawing.Rectangle(0, 0, $W, $H)),
	(New-Object System.Drawing.Rectangle(0, 0, $W, $H)),
	[System.Drawing.GraphicsUnit]::Pixel)
$bgBmp.Dispose()

function New-PrivateFont([string]$File, [float]$Px) {
	$pfc = New-Object System.Drawing.Text.PrivateFontCollection
	$pfc.AddFontFile($File)
	$font = New-Object System.Drawing.Font($pfc.Families[0], $Px,
		[System.Drawing.FontStyle]::Regular, [System.Drawing.GraphicsUnit]::Pixel)
	$pfc.Dispose()
	return $font
}

function Load-Frame([string]$Path) {
	$bmp = [System.Drawing.Bitmap]::FromFile($Path)
	if ($RetouchFps) {
		# Счётчик FPS: компактный блок в левом верхнем углу (замерено
		# x 0..100, y 58..98). Донор — та же тёмная зона чуть ниже.
		$patch = $bmp.Clone((New-Object System.Drawing.Rectangle(0, 100, 110, 44)), $bmp.PixelFormat)
		$gpatch = [System.Drawing.Graphics]::FromImage($bmp)
		$gpatch.DrawImage($patch, 0, 56)
		$gpatch.Dispose()
		$patch.Dispose()
	}
	return $bmp
}

# Кадры 1020x510 (94% ширины), боковые поля 30.
$fw = 1020
$fh = 510
$fx = [int](($W - $fw) / 2)

# Двойная рамка в стиле Diablo-UI лаунчера: тёмный кантик + тонкое золото.
$borderDark = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(200, 20, 18, 14), 6)
$borderGold = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(220, 154, 134, 90), 2)

function Draw-SlideFrame([System.Drawing.Bitmap]$bmp, [int]$y) {
	$g.DrawImage($bmp, (New-Object System.Drawing.Rectangle($fx, $y, $fw, $fh)),
		(New-Object System.Drawing.Rectangle(0, 0, $bmp.Width, $bmp.Height)),
		[System.Drawing.GraphicsUnit]::Pixel)
	$g.DrawRectangle($borderDark, ($fx - 3), ($y - 3), ($fw + 6), ($fh + 6))
	$g.DrawRectangle($borderGold, ($fx - 6), ($y - 6), ($fw + 12), ($fh + 12))
}

# Заголовок-подпись: CAPS Beaufort по центру верхней зоны — контекст
# читается ДО разбора кадров (правильный порядок сканирования витрины).
function Draw-Heading([string]$t1, [string]$t2) {
	$font = New-PrivateFont "$assets\Beaufort-Bold.ttf" 76
	$fmt = New-Object System.Drawing.StringFormat
	$fmt.Alignment = [System.Drawing.StringAlignment]::Center
	$fmt.LineAlignment = [System.Drawing.StringAlignment]::Near
	$goldText = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 220, 190, 130))
	$shadow1 = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(230, 0, 0, 0))
	$shadow2 = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(120, 0, 0, 0))
	$rect1 = New-Object System.Drawing.RectangleF(0, 175, $W, 110)
	$rect1s3 = New-Object System.Drawing.RectangleF(0, 178, $W, 110)
	$rect1s6 = New-Object System.Drawing.RectangleF(0, 181, $W, 110)
	$rect2 = New-Object System.Drawing.RectangleF(0, 263, $W, 110)
	$rect2s3 = New-Object System.Drawing.RectangleF(0, 266, $W, 110)
	$rect2s6 = New-Object System.Drawing.RectangleF(0, 269, $W, 110)
	$g.DrawString($t1, $font, $shadow2, $rect1s6, $fmt)
	$g.DrawString($t1, $font, $shadow1, $rect1s3, $fmt)
	$g.DrawString($t1, $font, $goldText, $rect1, $fmt)
	if ($t2) {
		$g.DrawString($t2, $font, $shadow2, $rect2s6, $fmt)
		$g.DrawString($t2, $font, $shadow1, $rect2s3, $fmt)
		$g.DrawString($t2, $font, $goldText, $rect2, $fmt)
	}
	$goldText.Dispose(); $shadow1.Dispose(); $shadow2.Dispose(); $font.Dispose()
}

if ($Kind -eq "pair") {
	Draw-Heading $Line1 $Line2
	# Равномерные зазоры: контент 1220px (заголовок + 2 кадра), воздух
	# ~170 сверху/между/снизу — низ карточки не проваливается.
	$bmpA = Load-Frame $FrameA
	Draw-SlideFrame $bmpA 515
	$bmpA.Dispose()
	$bmpB = Load-Frame $FrameB
	Draw-SlideFrame $bmpB 1195
	$bmpB.Dispose()
}
elseif ($Kind -eq "title") {
	Draw-Heading $Line1 $Line2
	# Плотный кроп 2:1 без нижней полосы копирайтов: демон + лого
	# крупнее, мёртвых чёрных полей меньше.
	$bmp = Load-Frame $FrameA
	$crop = New-Object System.Drawing.Rectangle(70, 0, 1300, 650)
	$cb = $bmp.Clone($crop, $bmp.PixelFormat)
	$bmp.Dispose()
	Draw-SlideFrame $cb 705
	$cb.Dispose()
}

$borderDark.Dispose()
$borderGold.Dispose()
$g.Dispose()
$canvas.Save($OutFile, [System.Drawing.Imaging.ImageFormat]::Png)
$canvas.Dispose()
Write-Output "saved $OutFile ($Kind)"
