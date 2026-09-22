====================================================================
HP_WKS_SWTOOLS_DRIVER - DRIVER IOCTL SUMMARY (NOTEPAD EXPORT)
====================================================================

Driver Name:     HP Workstation Software Tools Driver
Device Path:     \\.\Device\HP_WKS_SWTOOLS_DRIVER
Primary Risk:    High (Arbitrary Physical Memory/MMIO Read/Write & Mapping Primitives)

--------------------------------------------------------------------
IOCTL CODES & MAPPED OPERATIONS
--------------------------------------------------------------------

IOCTL CODE: 0x9C40610C
- Operation: Physical / MMIO Memory Read
- Structure: _READ_INPUT (PhysicalAddress: uint64_t, TransferLength: uint32_t)
- Details:   Accepts a physical/MMIO address and read size, uses MmMapIoSpace 
             to map the region, copies the bytes back to the user-mode output 
             buffer, and unmaps the space.

IOCTL CODE: 0x9C40A110
- Operation: Physical / MMIO Memory Write
- Structure: _WRITE_INPUT (PhysicalAddress, TransferLength, SourceBufferPtr, MaxBufferSize)
- Details:   Accepts target physical/MMIO address and a user-mode source buffer pointer, 
             uses MmMapIoSpace to map the region, and writes arbitrary data directly 
             into physical memory.

IOCTL CODE: 0x9C406104
- Operation: Physical / MMIO Memory Mapping
- Structure: _MAP_INPUT -> _MAP_OUTPUT
- Details:   Maps a physical/MMIO address range via MmMapIoSpace, allocates and builds 
             an MDL, locks pages, and returns a tracking context containing kernel mapping 
             pointers (MappedIoSpace, MappedLockedPages, MdlPointer).

IOCTL CODE: 0x9C40A108
- Operation: Physical / MMIO Memory Unmap & Teardown
- Structure: _MAP_OUTPUT (Passed back as input)
- Details:   Accepts the mapping context structure to safely unmap locked pages, free MDLs, 
             and release MmMapIoSpace resources back to the system.
====================================================================

| IOCTL        | Handler         | Capability                     | Internal Operation                                                                                 | Status          |
| ------------ | --------------- | ------------------------------ | ---------------------------------------------------------------------------------------------------| --------------- |
| `0x9C406104` | `sub_1400013A8` | Map physical/MMIO region       | `MmMapIoSpace` -> `IoAllocateMdl` -> `MmBuildMdlForNonPagedPool` -> `MmMapLockedPagesSpecifyCache` | Present in code |
| `0x9C40610C` | `sub_14000173C` | Read physical/MMIO region      | `MmMapIoSpace` -> copy mapped bytes to output buffer -> `MmUnmapIoSpace`                           | Present in code |
| `0x9C40A108` | `sub_1400017BC` | Unmap previously mapped region | `MmUnmapLockedPages` -> `IoFreeMdl` -> `MmUnmapIoSpace`                                            | Present in code |
| `0x9C40A110` | `sub_140001814` | Write physical/MMIO region     | `MmMapIoSpace` -> copy caller-controlled data into mapped region -> `MmUnmapIoSpace`               | Present in code |