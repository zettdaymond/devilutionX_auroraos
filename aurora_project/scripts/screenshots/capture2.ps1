param(
	[string]$Exe,
	[string]$ArgList = "",
	[string]$OutFile,
	[int]$ScrollNotches = 0,
	[int]$DragScrollPx = 0,
	[switch]$Client
)

Add-Type -AssemblyName System.Drawing
Add-Type -MemberDefinition '
[DllImport("user32.dll")] public static extern bool SetProcessDpiAwarenessContext(IntPtr value);
[DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
[DllImport("user32.dll")] public static extern void mouse_event(uint flags, uint dx, uint dy, uint dwData, UIntPtr extra);
[DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
[DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT rect);
[DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hWnd, IntPtr hdc, uint flags);
[DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr hWnd, out RECT rect);
[DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr hWnd, ref POINT pt);
public struct RECT { public int Left; public int Top; public int Right; public int Bottom; }
public struct POINT { public int X; public int Y; }
' -Name U32 -Namespace W

# Скрипт обязан быть DPI-aware: иначе Windows виртуализирует координаты
# aware-окна SDL (720 physical -> 411 logical при 175%) и CopyFromScreen
# снимает уменьшенный огрызок. PMv2 = (HANDLE)-4.
[W.U32]::SetProcessDpiAwarenessContext([IntPtr]::new(-4)) | Out-Null

# stdout/stderr в файлы: у консольного exe иначе создаётся консольное окно,
# которое перекрывает окно SDL и попадает в CopyFromScreen.
$outLog = [System.IO.Path]::GetTempFileName()
$errLog = [System.IO.Path]::GetTempFileName()
$proc = Start-Process -FilePath $Exe -ArgumentList $ArgList -PassThru -WorkingDirectory (Split-Path $Exe) `
	-RedirectStandardOutput $outLog -RedirectStandardError $errLog
Start-Sleep -Milliseconds 3000

# окно SDL — главное окно процесса; поднимаем наверх, иначе CopyFromScreen
# снимет то, что перекрывает его (консоль с логом).
$proc.Refresh()
$h = $proc.MainWindowHandle
[W.U32]::SetForegroundWindow($h) | Out-Null
Start-Sleep -Milliseconds 500
$rect = New-Object 'W.U32+RECT'
[W.U32]::GetWindowRect($h, [ref]$rect) | Out-Null

$cx = [int](($rect.Left + $rect.Right) / 2)
$cy = [int](($rect.Top + $rect.Bottom) / 2)
[W.U32]::SetCursorPos($cx, $cy) | Out-Null
Start-Sleep -Milliseconds 900
for ($i = 0; $i -lt $ScrollNotches; $i++) {
	[W.U32]::mouse_event(0x0800, 0, 0, [uint32]4294966376, [UIntPtr]::Zero)  # -940: крупный шаг
	Start-Sleep -Milliseconds 60
}
# Drag-скролл: серия МЕДЛЕННЫХ драгов по ~500px (медленный жест не
# запускает кинетику — позиция детерминирована). DragScrollPx — суммарный путь.
if ($DragScrollPx -gt 0) {
	$drags = [math]::Ceiling($DragScrollPx / 500.0)
	for ($d = 0; $d -lt $drags; $d++) {
		$dragFrom = $cy + 250
		$dragTo = $cy - 250
		[W.U32]::SetCursorPos($cx, $dragFrom) | Out-Null
		Start-Sleep -Milliseconds 120
		[W.U32]::mouse_event(0x0002, 0, 0, 0, [UIntPtr]::Zero)  # LEFTDOWN
		$steps = 25
		for ($i = 1; $i -le $steps; $i++) {
			$y = $dragFrom + [int](($dragTo - $dragFrom) * $i / $steps)
			[W.U32]::SetCursorPos($cx, $y) | Out-Null
			Start-Sleep -Milliseconds 28
		}
		[W.U32]::mouse_event(0x0004, 0, 0, 0, [UIntPtr]::Zero)  # LEFTUP
		Start-Sleep -Milliseconds 350
	}
}
Start-Sleep -Milliseconds 800

$winRect = $rect

# Клиентская область без рамки и заголовка — для скриншотов магазина.
# Всегда снимаем PrintWindow (честный буфер окна, неуязвим к перекрытиям —
# CopyFromScreen по экранным координатам ловит чужие окна поверх), а рамку
# срезаем кропом по смещению client-origin относительно window-origin.
$crect = New-Object 'W.U32+RECT'
[W.U32]::GetClientRect($h, [ref]$crect) | Out-Null
$pt = New-Object 'W.U32+POINT'
$pt.X = 0; $pt.Y = 0
[W.U32]::ClientToScreen($h, [ref]$pt) | Out-Null
$w = $winRect.Right - $winRect.Left
$hh = $winRect.Bottom - $winRect.Top

$bmp = New-Object System.Drawing.Bitmap($w, $hh)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$hdc = $g.GetHdc()
# PW_RENDERFULLCONTENT: содержимое окна даже поверх перекрывших его окон
$ok = [W.U32]::PrintWindow($h, $hdc, 2)
$g.ReleaseHdc($hdc)
if (-not $ok) {
	# фолбэк на экранную копию
	$g.CopyFromScreen($rect.Left, $rect.Top, 0, 0, $bmp.Size)
} elseif ($Client) {
	$offX = $pt.X - $winRect.Left
	$offY = $pt.Y - $winRect.Top
	$crop = New-Object System.Drawing.Rectangle($offX, $offY, $crect.Right, $crect.Bottom)
	$cb = $bmp.Clone($crop, $bmp.PixelFormat)
	$bmp.Dispose()
	$bmp = $cb
}
$bmp.Save($OutFile, [System.Drawing.Imaging.ImageFormat]::Png)
$g.Dispose(); $bmp.Dispose()
Write-Output "printwindow=$ok"

Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
Write-Output "saved $OutFile ($($bmp.Width) x $($bmp.Height))"
