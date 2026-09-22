#include <windows.h>
#include <winioctl.h>
#include <stdio.h>

#define DRIVER_DEVICE_NAME "\\\\.\\HardwareControlDevice" // Adjust device link as needed

// Calculated IOCTL / Control codes based on dispatcher offsets from base 2244672 (0x224800)
#define IOCTL_MSR_READ          2244672 // Offset 0
#define IOCTL_MSR_WRITE         2244676 // Offset +4
#define IOCTL_PCI_QUERY         2244680 // Offset +8

int main() {
    printf("[*] Opening handle to driver...\n");

    HANDLE hDevice = CreateFileA(
        DRIVER_DEVICE_NAME,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hDevice == INVALID_HANDLE_VALUE) {
        printf("[-] Failed to open device handle. Error: %lu\n", GetLastError());
        return 1;
    }

    printf("[+] Successfully opened driver handle!\n");

    DWORD bytesReturned = 0;

    // ==========================================================
    // DEMO 1: Read CPU MSR 0x610 (RAPL Power Limit Register)
    // Triggered via sub_14000111C (Offset 0)
    // ==========================================================
    {
        ULONG64 msrValue = 0;

        printf("[*] Sending IOCTL to read MSR 0x610 via core affinity pinning...\n");

        BOOL success = DeviceIoControl(
            hDevice,
            IOCTL_MSR_READ,
            NULL, 0,
            &msrValue, sizeof(msrValue),
            &bytesReturned,
            NULL
        );

        if (success) {
            printf("[+] Successfully read MSR 0x610 value: 0x%016llX\n", msrValue);
        } else {
            printf("[-] MSR read failed. Error: %lu\n", GetLastError());
        }
    }

    // ==========================================================
    // DEMO 2: Query PCI Configuration (Host Bridge Vendor/Device ID)
    // Triggered via sub_140001258 (Offset +8)
    // ==========================================================
    {
        struct {
            DWORD vendorId;
            WORD deviceId;
            WORD reserved;
        } pciOutput = { 0 };

        printf("[*] Sending IOCTL to query PCI configuration space (0xCF8/0xCFC)...\n");

        BOOL success = DeviceIoControl(
            hDevice,
            IOCTL_PCI_QUERY,
            NULL, 0,
            &pciOutput, sizeof(pciOutput),
            &bytesReturned,
            NULL
        );

        if (success) {
            printf("[+] Successfully retrieved PCI info!\n");
            printf("    - Vendor ID: 0x%04X\n", pciOutput.vendorId);
            printf("    - Device ID: 0x%04X\n", pciOutput.deviceId);
        } else {
            printf("[-] PCI query failed. Error: %lu\n", GetLastError());
        }
    }

    CloseHandle(hDevice);
    printf("[*] Closed driver handle. Done.\n");
    return 0;
}