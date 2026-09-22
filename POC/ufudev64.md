====================================================================
UFUDEV64.SYS - DRIVER IOCTL SUMMARY (NOTEPAD EXPORT)
====================================================================

Driver Name:     ufudev64.sys
Primary Risk:    Critical (Arbitrary Physical Memory Read/Write & Contiguous Memory Allocation)

--------------------------------------------------------------------
DISPATCH FUNCTIONALITY BREAKDOWN (BY IOCTL CODE):
--------------------------------------------------------------------
* IOCTL Code `0x80102069` (Arbitrary Physical Memory Write):
  - Extracts a target physical address from user input and calls **`MmMapIoSpace`** to map a 0x1024-byte window.
  - Copies user-controlled blocks directly into physical memory, providing an unconstrained physical write primitive.

* IOCTL Code `0x8010206E` (Arbitrary Physical Memory Read):
  - Maps an arbitrary physical address range via **`MmMapIoSpace`** and copies the physical memory data back into user-supplied buffer space, allowing userland to read arbitrary physical memory (including kernel data structures).

* IOCTL Code `0x80102062` (Arbitrary Physical Memory Read/Copy to Locked Pages):
  - Maps physical memory and copies contents directly into locked memory spaces (`MmMapLockedPagesSpecifyCache`), working in tandem with the physical mapping primitives.

* IOCTL Codes `0x80102040` & `0x80102044` (Contiguous Memory Allocation):
  - Wraps `MmAllocateContiguousMemory` and `MmFreeContiguousMemory`, allowing user-mode callers to allocate raw contiguous physical memory blocks.

* IOCTL Code `0x80102048` (Physical Address Translation):
  - Wraps `MmGetPhysicalAddress`, returning the physical address translation for arbitrary virtual buffers.

====================================================================