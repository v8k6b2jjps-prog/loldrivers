==================================================================
FILENAME: epos.sys - BYOVD CASE-TO-TARGET SUMMARY (NOTEPAD EXPORT)
==================================================================

Primary Risk:    High (Raw Physical Memory Mapping & Port I/O Primitives)
Device Name:     \Device\WNBIOS (\DosDevices\WNBIOS)

--------------------------------------------------------------------
IOCTL CODE: 0x80102060 (LowPart: -2146426816)
--------------------------------------------------------------------
- Target Operation:  Contiguous Memory Information Disclosure
- Access Type:       Shared Driver Buffer / Physical Address Query
- Mechanism:         
  1. Validates options payload size (40 bytes).
  2. Returns pre-allocated 64KB contiguous physical memory address (`MmAllocateContiguousMemory`) and its bus address back to user-mode.
- Security Impact:   
  - Exposes kernel-allocated physical memory boundaries to user-mode.
  - Useful for tracking low-level hardware structures or staging payloads.

--------------------------------------------------------------------
IOCTL CODE: 0x80102064 (LowPart: -2146426812)
--------------------------------------------------------------------
- Target Operation:  Physical Address Translation & Byte Execution (`sub_11B4F`)
- Access Type:       Port Output / Hardware Bus Routine
- Mechanism:         
  1. Resolves physical addresses via `MmGetPhysicalAddress`.
  2. Executes low-level port writing instructions (`__outbyte`) based on user-supplied parameters.
- Security Impact:   
  - Permits unprivileged interaction with system ports, enabling raw hardware component manipulation.

--------------------------------------------------------------------
IOCTL CODE: 0x80102068 (LowPart: -2146426808)
--------------------------------------------------------------------
- Target Operation:  Arbitrary Physical Memory Mapping (`sub_118F4`)
- Access Type:       ZwOpenSection / ZwMapViewOfSection
- Mechanism:         
  1. Opens `\Device\PhysicalMemory` from kernel mode.
  2. Translates bus addresses using `HalTranslateBusAddress`.
  3. Maps arbitrary physical memory ranges directly into caller memory space using `ZwMapViewOfSection`.
- Security Impact:   
  - Classic high-risk BYOVD primitive. Grants user-mode code full read/write access to arbitrary physical memory addresses, entirely bypassing OS memory boundaries.

--------------------------------------------------------------------
IOCTL CODE: 0x8010206C / 0x80102070 / etc. (Port I/O Range)
--------------------------------------------------------------------
- Target Operation:  Direct I/O Port Read/Write (`__outbyte`, `__inbyte`, `__outdword`)
- Access Type:       Hardware Port Manipulation (CMOS / ISA Ports)
- Mechanism:         
  1. Validates input sizes.
  2. Direct interaction with legacy or system ports like CMOS (`0x70`-`0x73`), keyboard controller (`0x60`), and custom registers (`0x310`).
- Security Impact:   
  - Direct hardware port control allows low-privileged applications to interact with core motherboard components, potentially corrupting system state or reading sensitive hardware registers.

--------------------------------------------------------------------
IOCTL CODE: PCI Config Read (LowPart: -2146426748)
--------------------------------------------------------------------
- Target Operation:  PCI Bus Configuration Space Enumeration
- Access Type:       Port 0xCF8 / 0xCFC Index-Data Access
- Mechanism:         
  1. Sends configuration addresses to `0xCF8` and reads target device parameters via `0xCFC`.
  2. Checks device status flags to map connected hardware controllers.
- Security Impact:   
  - Information disclosure of underlying hardware topology and configuration registers.
====================================================================