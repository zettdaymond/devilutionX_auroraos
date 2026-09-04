param([string]$Exe, [string]$Args = "")

Add-Type -MemberDefinition '
[DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr lp);
public delegate bool EnumProc(IntPtr hWnd, IntPtr lp);
[DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint pid);
[DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetWindowTextW(IntPtr hWnd, System.Text.StringBuilder sb, int max);
[DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetClassNameW(IntPtr hWnd, System.Text.StringBuilder sb, int max);
[DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr hWnd);
' -Name U32b -Namespace W

$proc = Start-Process -FilePath $Exe -ArgumentList $Args -PassThru -WorkingDirectory (Split-Path $Exe)
Start-Sleep -Milliseconds 3500
$targetPid = $proc.Id

$cb = {
	param($h, $lp)
	$pid2 = 0
	[W.U32b]::GetWindowThreadProcessId($h, [ref]$pid2) | Out-Null
	if ($pid2 -eq $targetPid -and [W.U32b]::IsWindowVisible($h)) {
		$sb = New-Object System.Text.StringBuilder 256
		[W.U32b]::GetWindowTextW($h, $sb, 256) | Out-Null
		$cls = New-Object System.Text.StringBuilder 256
		[W.U32b]::GetClassNameW($h, $cls, 256) | Out-Null
		Write-Output ("hwnd={0} class=[{1}] title=[{2}]" -f $h, $cls.ToString(), $sb.ToString())
	}
	return $true
}
[W.U32b]::EnumWindows($cb, [IntPtr]::Zero) | Out-Null
Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
