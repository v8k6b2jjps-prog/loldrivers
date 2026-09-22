====================================================================
FILENAME: asc_kbc.sys - BYOVD CASE-TO-TARGET SUMMARY (NOTEPAD EXPORT)
====================================================================

Primary Risk:    High (Arbitrary Physical Memory Write via MDL Mapping)
Device Driver:   asc_kbc.sys (Advanced SystemCare / IObit Keyboard Controller Driver)

--------------------------------------------------------------------
IOCTL CODE: 0x80ED0000 (LowPart: -2130706432)
--------------------------------------------------------------------
- Target Operation:  Physical Memory Copy / Arbitrary Write Primitive
- Access Type:       IoAllocateMdl / MmMapIoSpace / qmemcpy
- Mechanism:         
  1. Accepts user-mode pointers and sizes packaged via `MasterIrp`.
  2. Allocates a Memory Descriptor List (`IoAllocateMdl`) and locks user-space pages into memory (`MmProbeAndLockPages`).
  3. Maps those pages into system space (`MmMapLockedPagesSpecifyCache`).
  4. Resolves a user-supplied target physical address using `MmGetPhysicalAddress`.
  5. Maps that physical target range into kernel space via `MmMapIoSpace`.
  6. Executes a raw memory copy (`qmemcpy`) from the user-controlled source buffer directly into the target physical memory address.
- Security Impact:   
  - Classic high-risk BYOVD primitive. Grants any user-mode process holding a handle to the driver the ability to write arbitrary data to any physical memory location.
  - Commonly abused by malware to patch kernel structures, disable PatchGuard/EDR solutions, or escalate privileges directly to SYSTEM.
====================================================================