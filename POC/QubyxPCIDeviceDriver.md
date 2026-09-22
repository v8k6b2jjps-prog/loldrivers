====================================================================
QUBYXPCIDEVICEDRIVER.SYS - DRIVER IOCTL SUMMARY (NOTEPAD EXPORT)
====================================================================

Driver Name:     QubyxPCIDeviceDriver.sys
Device Path:     \Device\QubyxPCIDeviceDriver
Symbolic Link:   \DosDevices\QubyxPCIDeviceDriver
Primary Risk:    Critical (Arbitrary PCI Bus Data Access & MMIO Mapping to User-Space)

--------------------------------------------------------------------
DISPATCH FUNCTIONALITY BREAKDOWN (BY IOCTL CODE):
--------------------------------------------------------------------
* IOCTL Code `2236420` (IOCTL_GET_PCI_DATA):
  - Calls **`HalGetBusDataByOffset`** specifying `PCIConfiguration`.
  - Grants user-mode code the ability to read and query raw PCI configuration space registers directly for any bus, slot, and offset.

* IOCTL Code `2236424` / `IOCTL_MAP_PCI_MEMORY` (via `sub_112A8`):
  - Translates bus addresses using `HalTranslateBusAddress` and maps physical PCI / MMIO memory ranges into system space using **`MmMapIoSpace`**.
  - **Severe Security Flaw (Section Mapping to User-Mode):** Maps the kernel-mapped physical memory block into an MDL (`IoAllocateMdl`), builds non-paged pool memory, and maps it directly into user space via **`MmMapLockedPagesSpecifyCache`** with user-mode accessibility.
  - This exposes raw physical/MMIO device memory directly to unprivileged user applications, allowing arbitrary read/write access to hardware memory windows.

* IOCTL Code `2236428` / `IOCTL_UNMAP_PCI_MEMORY` (via `sub_11470`):
  - Cleans up and unmaps locked user pages, frees MDLs, and unmaps I/O space.

====================================================================