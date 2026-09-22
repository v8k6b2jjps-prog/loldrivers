HANDLE hDevice = CreateFile(
    L"\\\\.\\WinRing0_0", // The symbolic link name registered by the driver
    GENERIC_READ | GENERIC_WRITE,
    0,
    NULL,
    OPEN_EXISTING,
    FILE_ATTRIBUTE_NORMAL,
    NULL
);

// For Reading:

DWORD64 physicalAddress = 0x00000000; // Target physical address
DWORD bytesReturned = 0;

// To read exactly a 4-byte DWORD from that address:
DeviceIoControl(
    hDevice, 
    0x80006414, 
    &physicalAddress, 
    4,                // Explicitly pass '4' so the driver sets v14 = 4
    &physicalAddress, 
    8,                // Output length MUST still be exactly 8 to pass the v2[4] != 8 check
    &bytesReturned, 
    NULL
);

// For Writing:

WINRING0_WRITE_STRUCT WriteBuffer;
WriteBuffer.PhysicalAddress = 0x00000000; // 8 bytes
WriteBuffer.Payload.DwordValue = 0x12345678; // 4 bytes

DWORD bytesReturned = 0;

DeviceIoControl(
    hDevice, 
    0x80006418, 
    &WriteBuffer, 
    12,             // Total size = 8 (address) + 4 (data). v2+16 becomes 12.
    NULL, 
    0, 
    &bytesReturned, 
    NULL
);