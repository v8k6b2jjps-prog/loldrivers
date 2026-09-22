====================================================================
ADRMDRVSYS.SYS - DRIVER IOCTL SUMMARY (NOTEPAD EXPORT)
====================================================================

Driver Name:     ADRMDRVSYS.sys
Device Path:     \Device\ADRMDRVSYS
Symbolic Link:   \DosDevices\ADRMDRVSYS
Primary Risk:    High (CVSS 8.8 / Cataloged in LOLDrivers)

--------------------------------------------------------------------
IOCTL CODE: 0x2234D4
--------------------------------------------------------------------
- Target Operation:  PCI Memory-Mapped I/O (MMIO) Read Primitive
- Access Type:       Arbitrary Read / Information Disclosure
- Mechanism:         
  1. Accepts user-supplied bus, device, and offset parameters.
  2. Queries PCI configuration space to calculate the Base Address 
     Register (BAR) physical memory address.
  3. Maps the physical memory region into kernel virtual space 
     using MmMapIoSpace.
  4. Reads the target memory bytes and returns them directly to 
     the user-space application output buffer.

- Security Impact:   
  - Lacks proper privilege verification and address bounds checking.
  - Allows any low-privileged user-mode application to read raw 
    physical memory and hardware registers.
  - Poses a direct BYOVD (Bring Your Own Vulnerable Driver) threat, 
    often leveraged to defeat KASLR or assist in memory-dumping attacks.
====================================================================