====================================================================
MINIHACKER.SYS - DRIVER IOCTL SUMMARY (NOTEPAD EXPORT)
====================================================================

Driver Name:     MiniHacker.sys
Primary Risk:    Critical (Arbitrary Physical Memory Write via MMIO Mapping)

--------------------------------------------------------------------
IOCTL / CONTROL CODE CHECK: 2236420 (Decimal) / 0x222034 (Hex)
--------------------------------------------------------------------
- Target Operation:  IOCTL Dispatch Handler / Command Router
- Access Type:       Validates IOCTL control code against `2236420` and checks buffer/option sizes (`Options != 13`).

--------------------------------------------------------------------
DISPATCH FUNCTIONALITY BREAKDOWN (BY SUB-COMMAND TYPE):
--------------------------------------------------------------------
* Sub-commands 0 & 1 (Global Variable State Configuration):
  - Stores user-supplied pointers and byte values directly into driver global variables (`qword_1400030F0` and `byte_1400030F8`).
  - Used to cache physical address targets and configuration states for later operations.

* Sub-commands 2 & 3 (Cached Memory States):
  - Updates alternative global storage targets (`qword_140003100` and `byte_140003108`).

* Default / Uncached Operation via `sub_140001150` (Arbitrary Physical Memory Write):
  - **Severe Security Flaw:** Extracts a physical address (`PHYSICAL_ADDRESS a1`) and a target byte value from the input structure.
  - Passes the physical address to **`MmMapIoSpace`** to map physical memory directly into system space with a size of 1 byte (`1ui64`).
  - Copies the user-controlled byte into the mapped physical address using `qmemcpy(v2, &v5, sizeof(char))` and unmaps it.
  - Grants user-mode applications an **arbitrary physical memory write primitive**, allowing attackers to patch kernel structures, overwrite page tables, or disable security mechanisms directly.

====================================================================