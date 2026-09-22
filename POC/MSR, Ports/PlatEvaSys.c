#include <windows.h>
#include <winioctl.h>
#include <stdio.h>

#define DRIVER_DEVICE_NAME "\\\\.\\PlatevaDevice" // Adjust based on the actual symbolic link

// IOCTL codes extracted from the switch-case statement
#define IOCTL_GET_VTX          0x888A0000
#define IOCTL_GET_VTD          0x888A0004
#define IOCTL_GET_PROCS        0x888A0008
#define IOCTL_READ_MSR         0x888A000C
#define IOCTL_WRITE_MSR        0x888A0010
#define IOCTL_GET_PCI_CONF     0x888A0014
#define IOCTL_IN_BYTE          0x888A0018
#define IOCTL_IN_WORD          0x888A001C
#define IOCTL_IN_DWORD         0x888A0020

int main() {
    printf("[*] Opening handle to PLATEVA driver...\n");

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
    // DEMO 1: Read an Arbitrary CPU MSR (0x888A000C)
    // ==========================================================
    {
        ULONG targetMSR = 0xC0000082; // Example: IA32_LSTAR (System Call Target Address)
        ULONG64 msrValue = 0;

        printf("[*] Sending IOCTL_READ_MSR for MSR index: 0x%X...\n", targetMSR);

        BOOL success = DeviceIoControl(
            hDevice,
            IOCTL_READ_MSR,
            &targetMSR,
            sizeof(targetMSR),
            &msrValue,
            sizeof(msrValue),
            &bytesReturned,
            NULL
        );

        if (success) {
            printf("[+] MSR Read Success! Value: 0x%016llX\n", msrValue);
        } else {
            printf("[-] IOCTL_READ_MSR failed. Error: %lu\n", GetLastError());
        }
    }

    // ==========================================================
    // DEMO 2: Read from a Raw Hardware I/O Port (0x888A0020)
    // ==========================================================
    {
        ULONG ioPort = 0xCF8; // PCI Address Port
        ULONG portValue = 0;

        printf("[*] Sending IOCTL_IN_DWORD to read from I/O port 0x%X...\n", ioPort);

        BOOL success = DeviceIoControl(
            hDevice,
            IOCTL_IN_DWORD,
            &ioPort,
            sizeof(ioPort),
            &portValue,
            sizeof(portValue),
            &bytesReturned,
            NULL
        );

        if (success) {
            printf("[+] Port Read Success! Port 0x%X value: 0x%08X\n", ioPort, portValue);
        } else {
            printf("[-] IOCTL_IN_DWORD failed. Error: %lu\n", GetLastError());
        }
    }

    // ==========================================================
    // DEMO 3: Query PCI Configuration Space (0x888A0014)
    // ==========================================================
    {
        struct {
            ULONG bus;
            ULONG device;
            ULONG function;
            ULONG offset;
            ULONG size;
        } pciRequest = { 0, 0, 0, 0, 4 }; // Host bridge (Bus 0, Dev 0, Func 0, Offset 0)

        ULONG pciResponse = 0;

        printf("[*] Sending IOCTL_GET_PCI_CONF for Bus 0, Dev 0, Func 0...\n");

        BOOL success = DeviceIoControl(
            hDevice,
            IOCTL_GET_PCI_CONF,
            &pciRequest,
            sizeof(pciRequest),
            &pciResponse,
            sizeof(pciResponse),
            &bytesReturned,
            NULL
        );

        if (success) {
            printf("[+] PCI Config Query Success! Data: 0x%08X\n", pciResponse);
        } else {
            printf("[-] IOCTL_GET_PCI_CONF failed. Error: %lu\n", GetLastError());
        }
    }

    CloseHandle(hDevice);
    printf("[*] Closed driver handle. Done.\n");
    return 0;
}