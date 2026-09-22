====================================================================
ZNTPORT.SYS - DRIVER IOCTL SUMMARY (NOTEPAD EXPORT)
====================================================================

Driver Name:     zntport.sys
Device Path:     \Device\zntport (Implied from uninstallation path)
Symbolic Link:   \DosDevices\zntport
Primary Risk:    Critical (BYOVD Arbitrary Physical Memory Read & Raw Port I/O)

--------------------------------------------------------------------
DISPATCH FUNCTIONALITY BREAKDOWN (BY COMMAND CODE):
--------------------------------------------------------------------
* Command Code `-251648956` & `-251648952` (Direct Kernel Memory/Buffer Read & Write):
  - Directly reads from and writes to `BaseAddress` (a non-cached memory buffer allocated with a size of 0x2000 bytes) using user-supplied offsets.

* Command Code `-251648948` (Full Buffer Dump):
  - Copies the entire 0x2000 byte kernel buffer (`BaseAddress`) back to user space if validation passes, exposing kernel-allocated data.

* Command Code `-251648832`, `-251648828` (Raw Port I/O Operations):
  - Exposes wrapper functions (`sub_10B10` to `sub_10B80`) executing `__inbyte`, `__inword`, `__indword`, `__outbyte`, `__outword`, and `__outdword`.
  - Grants user-mode code unconstrained access to read and write to any hardware port on the system.

* Command Code `-251648820` (Arbitrary Physical Memory Mapping via `MmMapIoSpace`):
  - Takes a physical address supplied by user space (`*(_DWORD *)Dest`), maps it via `MmMapIoSpace` for a length of 0x32 bytes, and copies the data back using `strcpy`.
  - **Severe Security Flaw:** Allows user-mode applications to read arbitrary physical memory, making it a classic, highly dangerous BYOVD primitive for bypassing security mechanisms or dumping sensitive physical frames.

====================================================================