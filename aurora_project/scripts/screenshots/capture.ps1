param(
	[string]$Exe,
	[string]$Args = "",
	[string]$OutFile,
	[int]$ScrollNotches = 0
)

Add-Type -AssemblyName System.Drawing
Add-Type -MemberDefinition '
[DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
[DllImport("user32.dll")] public static extern void mouse_event(uint flags, uint dx, uint dy, uint dwData, UIntPtr extra);
[DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT rect);
public struct RECT { public int Left; public int Top; public int Right; public int Bottom; }
' -Name U32 -Namespace W

$proc = Start-Process -FilePath $Exe -ArgumentList $Args -PassThru -WorkingDirectory (Split-Path $Exe)
Start-Sleep -Milliseconds 3000

$proc.Refresh()
$h = $proc.MainWindowHandle
if ($h -eq [IntPtr]::Zero) {
	# окно могло ещё не подняться — ещё раз
	Start-Sleep -Milliseconds 2000
	$proc.Refresh()
	$h = $proc.MainWindowHandle
}
$rect = New-Object 'W.U32+RECT'
[W.U32]::GetWindowRect($h, [ref]$rect) | Out-Null

# курсор в центр окна и прокрутка колесом
$cx = [int](($rect.Left + $rect.Right) / 2)
$cy = [int](($rect.Top + $rect.Bottom) / 2)
[W.U32]::SetCursorPos($cx, $cy) | Out-Null
Start-Sleep -Milliseconds 300
for ($i = 0; $i -lt $ScrollNotches; $i++) {
	[W.U32]::mouse_event(0x0800, 0, 0, [uint32](-120), [UIntPtr]::Zero)
	Start-Sleep -Milliseconds 60
}
Start-Sleep -Milliseconds 800

$w = $rect.Right - $rect.Left
$hh = $rect.Bottom - $rect.Top
$bmp = New-Object System.Drawing.Bitmap($w, $hh)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.CopyFromScreen($rect.Left, $rect.Top, 0, 0, $bmp.Size)
$bmp.Save($OutFile, [System.Drawing.Imaging.ImageFormat]::Png)
$g.Dispose(); $bmp.Dispose()

Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
Write-Output "saved $OutFile ($w x $hh)"
