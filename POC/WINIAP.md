====================================================================
WINIAP.SYS - DRIVER IOCTL SUMMARY (NOTEPAD EXPORT)
====================================================================

Driver Name:     WinIAP.sys
Device Path:     \Device\WinIAPX
Symbolic Link:   \DosDevices\WinIAPX
Primary Risk:    Critical (BYOVD Raw Port I/O, PCI Configuration Access & Physical Memory)

--------------------------------------------------------------------
IOCTL CODE: 0x9F3A0004 (Decimal: -1673453564 / 0x9F3A0004)
--------------------------------------------------------------------
- Target Operation:  Dispatched Command Router (Switch Case based on byte at input buffer)
- Access Type:       Multi-function Dispatcher / Low-Level Control Primitive
- Mechanism:         
  1. Validates input/output buffer sizes (must be exactly 2058 bytes).
  2. Evaluates the command index byte (`*(_BYTE *)a2`) to route execution to specific low-level hardware routines.

- Security Impact:   
  - Serves as the central gateway for all high-privilege operations exposed by the driver to user-mode code.

--------------------------------------------------------------------
DISPATCH FUNCTIONALITY BREAKDOWN (BY SUB-COMMAND):
--------------------------------------------------------------------
* Cases 2 & 3 (Keyboard Controller Port Manipulation):
  - Interacts directly with port `0x64` and `0x60` (8042 PS/2 Keyboard Controller) using `__inbyte` and `__outbyte`.
  - Commonly used in hardware-level tricks, reset routines, or legacy command injection.

* Cases 4 & 5 (Polling & Input Status):
  - Continuously polls hardware ports with timeout loops (`KeStallExecutionProcessor`), reading controller states.

* Cases 0x12 & 0x13 (PCI Configuration Space Access via I/O Ports 0xCF8 / 0xCFC):
  - Issues `__outdword` and `__indword` to the PCI Configuration Address (`0xCF8`) and Data (`0xCFC`) ports.
  - Allows user-mode programs to read and modify raw PCI configuration registers directly, bypassing kernel abstractions.

* Cases 0x14 & 0x15 (Direct Port Read/Write):
  - Performs direct `__inbyte` and `__outbyte` operations on calculated dynamic hardware port bases.

* Case 0x21 (Physical Memory / MMIO Mapping via `MmMapIoSpace`):
  - Maps physical memory at address `0xFF000000` (4275569408i64) to modify control bits (such as BIOS lock or chipset configuration registers).

====================================================================