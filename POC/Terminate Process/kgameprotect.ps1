Clear-Host
Write-Host

$DriverName = "kgameprotect"
$DriverPath = Join-Path $PSScriptRoot "kgameprotect.sys"

if (-not (Test-Path $DriverPath)) {
    Write-Error "Driver file not found at: $DriverPath"
    return
}

# 1. Create Service only if it doesn't already exist
$existingService = Get-Service -Name $DriverName -ErrorAction SilentlyContinue
if (-not $existingService) {
    Write-Host "Creating driver service..."
    & sc.exe create $DriverName binPath= $DriverPath type= filesys start= demand
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Failed to create driver service via sc.exe. Error code: $LASTEXITCODE"
        return
    }

    # Configure mini-filter registry keys (only needed on initial creation)
    Write-Host "Configuring mini-filter registry keys..."
    $ServiceRegPath = "HKLM:\SYSTEM\CurrentControlSet\Services\$DriverName"
    $InstancesPath  = "$ServiceRegPath\Instances"
    $InstanceName   = "kgameprotect Instance"
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
} else {
    Write-Host "Driver service already exists. Skipping creation." -ForegroundColor Yellow
}

# 2. Load the Driver via fltMC
Write-Host "Loading mini-filter driver..."
& fltMC load $DriverName
if ($LASTEXITCODE -ne 0) {
    Write-Host "Driver may already be loaded. Continuing..." -ForegroundColor Yellow
} else {
    Write-Host "Driver loaded successfully." -ForegroundColor Green
}

# 3. Run IOCTL test execution
try {
    Write-Host "Starting target process (Notepad)..." -ForegroundColor Cyan
    $PROC = Start-Process -FilePath Notepad -WindowStyle Normal -PassThru
    $PROCID = $PROC.Id
    Write-Host "Notepad started with PID: $PROCID" -ForegroundColor Green

    Write-Host "Opening handle to driver device..." -ForegroundColor Cyan
    $hDevice = Get-FileHandle -FileName kgameprotect -AlternativeName kgameprotect -Symbolic 'Device'
    Write-Host "Driver handle acquired successfully." -ForegroundColor Green

    Write-Host "Preparing buffer with PID ($PROCID) and sending IOCTL 0x222048..." -ForegroundColor Cyan
    $inOutPtr = New-IntPtr -Size 4 -InitialValue $PROCID -ValueType int
    
    $ioctlResult = Invoke-IoctlCall -Mode 0 -FileHandle $hDevice -IoCtrlCode 0x222048 -InOutPtr $inOutPtr -InputSize 4 -OutputSize 0
    Write-Host "IOCTL call completed. Output/Result: $ioctlResult" -ForegroundColor Green

    # Brief pause to allow the driver to process the request
    Start-Sleep -Milliseconds 500

    $PROC.Refresh()
    Write-Host "Checking process status..." -ForegroundColor Cyan
    if ($PROC.HasExited) {
        Write-Host "Target process (PID $PROCID) was terminated or affected by the driver." -ForegroundColor Yellow
    } else {
        Write-Host "Target process (PID $PROCID) is still running normally." -ForegroundColor Green
        # Cleanup: close the test instance of Notepad if it's still alive
        $PROC.Kill()
    }
}
catch {
    Write-Error "Exception encountered during IOCTL execution: $_"
}
finally {
    Free-IntPtr $hDevice -Method NtHandle
    Free-IntPtr $inOutPtr -Method Auto
}

return
return

Write-Host "Execution completed. Driver and service left active." -ForegroundColor Green

# 5. Unload Driver and Cleanup Service
Write-Host "Unloading driver and removing service..." -ForegroundColor Cyan
& fltMC.exe unload $DriverName 2>&1 | Out-Null
& sc.exe delete $DriverName 2>&1 | Out-Null

Write-Host "Cleanup completed successfully." -ForegroundColor Green