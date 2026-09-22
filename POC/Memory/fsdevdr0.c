#include <windows.h>
#include <stdio.h>

#define DRIVER_NAME "\\\\.\\MemoryReadDevice"

// Helper function to perform a ReadFile dispatch with a specific physical address/offset
BOOL ReadDriverMemory(HANDLE hDevice, ULONGLONG physicalAddress, LPVOID outBuffer, DWORD bufferSize) {
    OVERLAPPED ov = { 0 };
    ov.Offset = (DWORD)(physicalAddress & 0xFFFFFFFF);
    ov.OffsetHigh = (DWORD)(physicalAddress >> 32);
    DWORD bytesRead = 0;
    
    return ReadFile(hDevice, outBuffer, bufferSize, &bytesRead, &ov);
}

int main() {
    HANDLE hDevice = CreateFileA(DRIVER_NAME, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hDevice == INVALID_HANDLE_VALUE) {
        printf("[-] Failed to open driver handle: %lu\n", GetLastError());
        return 1;
    }

    char buffer[4096];
    ULONGLONG targetAddress = 0x100000; // Example target address

    printf("[*] Executing read requests across driver dispatch handlers...\n");

    // Case 4: Direct Physical Memory Mapping via MmMapIoSpace
    // (Note: The driver state must be externally set to 4 beforehand)
    if (ReadDriverMemory(hDevice, targetAddress, buffer, sizeof(buffer))) {
        printf("[+] Case 4 (MmMapIoSpace) read success!\n");
    }

    // Case 5: Section Map Memory Reading via ZwMapViewOfSection
    // (Requires driver state to be set to 5)
    if (ReadDriverMemory(hDevice, targetAddress, buffer, sizeof(buffer))) {
        printf("[+] Case 5 (ZwMapViewOfSection) read success!\n");
    }

    // Case 6: Custom Callback Memory Handler
    // (Requires driver state to be set to 6)
    if (ReadDriverMemory(hDevice, targetAddress, buffer, sizeof(buffer))) {
        printf("[+] Case 6 (Callback Handler) read success!\n");
    }

    CloseHandle(hDevice);
    return 0;
}