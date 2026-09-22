Clear-Host
Write-Host

$DriverName = "fortimon3"
$DriverPath = Join-Path $PSScriptRoot "fortimon3.sys"

if (-not (Test-Path $DriverPath)) {
    Write-Error "Driver file not found at: $DriverPath"
    return
}

# 1. Create the Service using sc.exe
Write-Host "Creating driver service..."
& sc.exe create $DriverName binPath= $DriverPath type= filesys start= demand 2>&1 | Out-Null

# 2. Add Mini-Filter Registry Keys (Required for Filter Manager attachment)
Write-Host "Configuring mini-filter registry keys..."
$ServiceRegPath = "HKLM:\SYSTEM\CurrentControlSet\Services\$DriverName"
$InstancesPath  = "$ServiceRegPath\Instances"
$InstanceName   = "Fortimon3 Instance"
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
    Write-Error "Failed to load driver via fltMC. Check if Test Signing is enabled or if the INF/sys metadata is valid."
    return
}
Write-Host "Driver loaded successfully." -ForegroundColor Green

# 4. Connect to Communication Port & Send Message
$Values = '\Fortimon3FilterAntiExploitPort', 0, 0L, 0, 0L, [Ref]0L
$hr = Invoke-UnmanagedMethod -Dll FltLib.dll -Function FilterConnectCommunicationPort -Values $Values
if ($hr -eq 0) {
    $hPort    = $Values[5].Value
    $PROCID    = Start-Process -FilePath Notepad -WindowStyle Normal -PassThru | select -ExpandProperty Id
    $InOutPtr = New-IntPtr -Size 8 -TypeSize ([Int32], [Int32]) -Values (0x6C6C696B, $PROCID)
    $Values   = $hPort, $InOutPtr, 8, 0L, 0, [Ref]0L
    Invoke-UnmanagedMethod -Dll FltLib.dll -Function FilterSendMessage -Values $Values | Out-Null
}

# 5. Free handle, And Unload Drivers ...
Write-Host "Unloading driver and removing registry keys..." -ForegroundColor Cyan
Free-IntPtr $hPort -Method NtHandle
& fltMC.exe unload $DriverName
& sc.exe delete $DriverName

if (Test-Path $ServiceRegPath) {
    Remove-Item -Path $ServiceRegPath -Recurse -Force -ErrorAction SilentlyContinue
}

Write-Host "Cleanup completed successfully." -ForegroundColor Green