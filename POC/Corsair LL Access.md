====================================================================
CORSAIR LL ACCESS.SYS - DRIVER IOCTL SUMMARY (NOTEPAD EXPORT)
====================================================================

Driver Name:     Corsair LL Access.sys
Device Path:     \Device\CorsairLLAccess (or similar)
Symbolic Link:   \DosDevices\CorsairLLAccess
Primary Risk:    High (BYOVD Physical Memory Mapping, MSR Read, & Port I/O)

--------------------------------------------------------------------
IOCTL CODE: 0x225348 (Decimal: 2249544)
--------------------------------------------------------------------
- Target Operation:  PCI Configuration Space Write (Bus Data)
- Access Type:       Low-Level Hardware Configuration
- Mechanism:         
  1. Invokes `sub_140001E30`, which wraps `HalSetBusDataByOffset`.
  2. Directly modifies PCI bus configuration blocks using user-supplied parameters.

- Security Impact:   
  - Allows altering hardware device configurations, enabling direct hardware manipulation or potential bus-level persistence tricks.

--------------------------------------------------------------------
IOCTL CODE: 0x2249596 (Decimal: 2249596) - 0x225358 / 0x229354
--------------------------------------------------------------------
- Target Operation:  Port I/O (Read/Write Byte, Word, Dword) & MSR Access
- Access Type:       Raw Port Manipulation & CPU Model-Specific Registers
- Mechanism:         
  1. `0x2249612` invokes `__readmsr`, permitting user-mode code to read low-level Model-Specific Registers (e.g., LSTAR, IA32_LSTAR) which are critical for OS security and kernel execution hooks.
  2. `0x229354` maps user buffers to `__outbyte`, `__outword`, `__outdword`, `__inbyte`, `__inword`, and `__indword`, granting direct port interaction.

- Security Impact:   
  - Extremely hazardous primitive: MSR reading/writing and port I/O are classic vectors used by security tools and malware alike to hook system calls, manipulate CPU state, or interact with legacy hardware components directly from userland.

--------------------------------------------------------------------
IOCTL CODE: 0x225398 & 0x22539C (Decimal: 2249624 / 2249628)
--------------------------------------------------------------------
- Target Operation:  Enumerate / Scan Physical Memory Tables
- Access Type:       Kernel Memory Discovery / Scouting
- Mechanism:         
  1. Routines `sub_140001850` and `sub_140001990` iterate through memory bounds, validating pattern structures and matching values against internal criteria.
  2. Dynamically calculates and stores offsets into user-provided buffers.

- Security Impact:   
  - Maps out kernel or physical memory layouts, making it easier for an attacker to locate target structures (such as process tokens, driver lists, or hypervisor/EDR control objects).

--------------------------------------------------------------------
IOCTL CODE: 0x225394 (Decimal: 2249556)
--------------------------------------------------------------------
- Target Operation:  Physical Memory Mapping via `MmMapIoSpace`
- Access Type:       Arbitrary Physical Read/Write Primitive
- Mechanism:         
  1. Scans physical memory starting at `0x983040` (0xF0000 range, typically BIOS/ROM area) looking for specific signatures.
  2. Maps those physical regions into system/user address spaces.

- Security Impact:   
  - Provides direct physical memory read/write capability, a quintessential BYOVD primitive leveraged to patch kernel callbacks, disable security software drivers, or escalate privileges.
====================================================================