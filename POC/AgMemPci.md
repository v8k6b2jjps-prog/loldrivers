====================================================================
AGPCIMEM.SYS - DRIVER IOCTL SUMMARY (NOTEPAD EXPORT)
====================================================================

Driver Name:     AgPciMem.sys
Device Path:     \Device\AgPciMem
Symbolic Link:   \DosDevices\AgPciMem
Primary Risk:    High (BYOVD Contiguous Memory Allocation & Mapping)

--------------------------------------------------------------------
IOCTL CODE: 0x222434 (Decimal: 2236420)
--------------------------------------------------------------------
- Target Operation:  Contiguous Physical Memory Allocation & Mapping
- Access Type:       Arbitrary Physical Memory Mapping / BYOVD Primitive
- Mechanism:         
  1. Accepts sizing parameters from user-space.
  2. Allocates a fresh, physically contiguous memory block via 
     MmAllocateContiguousMemorySpecifyCache.
  3. Allocates and builds an MDL (Memory Descriptor List) for the buffer.
  4. Maps the physical memory region directly into user-space virtual 
     address space using MmMapLockedPagesSpecifyCache.
  5. Tracks the allocation in an internal spinlock-protected linked list.

- Security Impact:   
  - Grants low-privileged user-mode apps the ability to request and map raw physical memory.
  - Commonly leveraged in BYOVD exploits to build arbitrary read/write 
    primitives, construct fake kernel objects, or assist in privilege escalation.

--------------------------------------------------------------------
IOCTL CODE: 0x222438 (Decimal: 2236424)
--------------------------------------------------------------------
- Target Operation:  Contiguous Memory Deallocation & Unmapping
- Access Type:       Resource Teardown / Cleanup Primitive
- Mechanism:         
  1. Locates a previously mapped memory block by matching its physical address 
     and file context within the driver's tracked list.
  2. Safely unmaps the pages using MmUnmapLockedPages.
  3. Frees the associated MDL and releases the contiguous memory block.

- Security Impact:   
  - Manages the resource lifecycle of mapped physical memory buffers.
====================================================================