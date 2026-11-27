Clear-Host
Write-Host

$DriverName = "FileLock"
$DriverPath = Join-Path $PSScriptRoot "FileLock.sys"

if (-not (Test-Path $DriverPath)) {
    Write-Error "Driver file not found at: $DriverPath"
    return
}

# 0. Pre-cleanup: ensure no leftover from a previous run
Write-Host "Cleaning up any previous instance..."
& fltMC.exe unload $DriverName 2>&1 | Out-Null
& sc.exe delete $DriverName 2>&1 | Out-Null
Start-Sleep -Milliseconds 500

# 1. Create the Service using sc.exe
Write-Host "Creating driver service..."
& sc.exe create $DriverName binPath= $DriverPath type= filesys start= demand 2>&1 | Out-Null

# 2. Add Mini-Filter Registry Keys (Required for Filter Manager attachment)
Write-Host "Configuring mini-filter registry keys..."
$ServiceRegPath = "HKLM:\SYSTEM\CurrentControlSet\Services\$DriverName"
$InstancesPath  = "$ServiceRegPath\Instances"
$InstanceName   = "USafe Instance"
$InstancePath   = "$InstancesPath\$InstanceName"

if (-not (Test-Path $InstancesPath)) {
    New-Item -Path $InstancesPath -Force | Out-Null
}
Set-ItemProperty -Path $InstancesPath -Name "DefaultInstance" -Value $InstanceName

if (-not (Test-Path $InstancePath)) {
    New-Item -Path $InstancePath -Force | Out-Null
}
Set-ItemProperty -Path $InstancePath -Name "Altitude" -Value "370000"
Set-ItemProperty -Path $InstancePath -Name "Flags" -Value 0

# 3. Load the Driver via fltMC
Write-Host "Loading mini-filter driver..."
& fltMC load $DriverName
if ($LASTEXITCODE -ne 0) {
    Write-Error "Failed to load driver via fltMC."
    return
}
Write-Host "Driver loaded successfully." -ForegroundColor Green

# 4. Connect to Communication Port
$PortName = '\eFileLockPort'
Write-Host "Connecting to port: $PortName"
$Values = $PortName, 0, 0L, 0, 0L, [Ref]0L
$hr = Invoke-UnmanagedMethod -Dll FltLib.dll -Function FilterConnectCommunicationPort -Values $Values
if ($hr -ne 0) {
    Write-Error ("FilterConnectCommunicationPort failed: 0x{0:X8}" -f $hr)
    return
}
$hPort = $Values[5].Value
Write-Host "Connected. Handle: $hPort" -ForegroundColor Green

# 5. Send FileLock handshake (Command 10 = IOCTL_START_RELATE)
#    Struct layout (16 bytes):
#      Offset 0  [DWORD] : Command number (10)
#      Offset 4  [DWORD] : Padding / flags (0)
#      Offset 8  [DWORD] : Caller PID — arg1 to StartRelate
#      Offset 12 [DWORD] : Relate flags — arg2 to StartRelate (0)

Write-Host "Sending FileLock handshake..."
$FileLock = New-Object byte[] 16
$CurProc = [System.Diagnostics.Process]::GetCurrentProcess().Id
[System.BitConverter]::GetBytes([uint32]10).CopyTo($FileLock, 0)
[System.BitConverter]::GetBytes([uint32]0).CopyTo($FileLock,  4)
[System.BitConverter]::GetBytes([uint32]$CurProc).CopyTo($FileLock,  8)
[System.BitConverter]::GetBytes([uint32]0).CopyTo($FileLock,  12)

$bytesRet = 0
$Values = $hPort, $FileLock, $FileLock.Length, 0L, 0, [Ref]$bytesRet
$hr = Invoke-UnmanagedMethod -Dll FltLib.dll -Function FilterSendMessage -Values $Values
Write-Host ("  FileLock result: 0x{0:X8}" -f $hr)

# 6. Launch demo target process
$PROCID = Start-Process -FilePath Notepad -WindowStyle Normal -PassThru | Select-Object -ExpandProperty Id
Write-Host "Target PID: $PROCID"
Start-Sleep -Seconds 1

# 7. Send Kill command (Command 22)
#    Struct layout (12 bytes):
#      Offset 0  [DWORD] : Command number (22)
#      Offset 4  [DWORD] : Padding (0)
#      Offset 8  [DWORD] : Target PID to terminate

Write-Host "Sending Kill command (Command 22)..."
$kill = New-Object byte[] 12
[System.BitConverter]::GetBytes([uint32]22).CopyTo($kill, 0)       # Command
[System.BitConverter]::GetBytes([uint32]0).CopyTo($kill, 4)        # Padding
[System.BitConverter]::GetBytes([uint32]$PROCID).CopyTo($kill, 8)  # Target PID
$bytesRet = 0
$Values = $hPort, $kill, $kill.Length, 0L, 0, [Ref]$bytesRet
$hr = Invoke-UnmanagedMethod -Dll FltLib.dll -Function FilterSendMessage -Values $Values
Write-Host ("  Kill result: 0x{0:X8}" -f $hr)

# 8. Cleanup
Write-Host "Unloading driver and removing registry keys..." -ForegroundColor Cyan
Free-IntPtr $hPort -Method NtHandle
& fltMC.exe unload $DriverName
& sc.exe delete $DriverName

if (Test-Path $ServiceRegPath) {
    Remove-Item -Path $ServiceRegPath -Recurse -Force -ErrorAction SilentlyContinue
}

Write-Host "Cleanup completed successfully." -ForegroundColor Green