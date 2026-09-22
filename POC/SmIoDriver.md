====================================================================
SMIODRIVER.SYS - DRIVER IOCTL SUMMARY (NOTEPAD EXPORT)
====================================================================

Driver Name:     SmIoDriver.sys
Device Path:     \Device\SmIoDriver
Symbolic Link:   N/A (Exposed via internal device mapping)
Primary Risk:    Critical (Arbitrary Physical Memory Mapping & Raw Port/I2C Control)

--------------------------------------------------------------------
DISPATCH FUNCTIONALITY BREAKDOWN (BY COMMAND & ROUTINE):
--------------------------------------------------------------------
* Physical Memory Mapping via `sub_140006A28` (IOCTL Code: `1073750020` / `0x40003014`):
  - Extracts physical addresses and dimensions from user buffers.
  - Invokes **`MmMapIoSpace`** to map physical memory directly into system address space without proper privilege boundaries or input validation.
  - Copies, reads, or writes arbitrary memory blocks based on user-supplied types (1, 2, or 4-byte increments) and safely unmaps via `MmUnmapIoSpace`.

* Raw Port I/O Operations (`SMIO_PORT_READ/WRITE_*`):
  - Exposes subroutines handling direct byte, word, and dword interactions with legacy hardware and I/O ports.

* I2C Bus Manipulation Routines (`SMIO_I2C_*`):
  - Implements complex command control loops (`SMIO_I2C_READ_BYTE`, `WRITE_BYTE`, `READ_BLOCK`, etc.) communicating directly with underlying system hardware buses.

====================================================================