====================================================================
CISMBIOS.SYS - DRIVER IOCTL SUMMARY (NOTEPAD EXPORT)
====================================================================

Driver Name:     cismbios.sys
Device Path:     \Device\CISMBIOS
Symbolic Link:   \DosDevices\CISMBIOS
Primary Risk:    High (BYOVD Arbitrary Physical Memory Mapping & Port I/O)

--------------------------------------------------------------------
IOCTL CODE: 0x80002002 (Decimal: 2147487234)
--------------------------------------------------------------------
- Target Operation:  Driver Configuration & Metadata Retrieval
- Access Type:       Information Disclosure / Internal State Query
- Mechanism:         
  1. Validates internal initialization flags (`byte_14121`).
  2. Copies internal driver metadata buffers (`word_14124`, `word_14126`, `word_14128`, `byte_1412A`) directly into the user-supplied output buffer (`MappedSystemVa`).

- Security Impact:   
  - Exposes internal configuration and layout states of parsed BIOS/SMBIOS tables to user-mode callers.

--------------------------------------------------------------------
IOCTL CODE: 0x80002006 / 0x8000200A (Decimal: 2147487238 / 2147487242)
--------------------------------------------------------------------
- Target Operation:  SMBIOS / Physical Memory Table Access
- Access Type:       Arbitrary Data Extraction / Structure Copying
- Mechanism:         
  1. Verifies size and indexing constraints against pre-mapped physical ranges.
  2. Uses `memmove` to copy specific data structures and parsed BIOS entries from kernel space directly into the user-provided output memory buffer.

- Security Impact:   
  - Allows user-mode programs to harvest raw BIOS, system management tables, and device descriptor structures from kernel memory.

--------------------------------------------------------------------
IOCTL CODE: 0x8000201E (Decimal: 2147487262)
--------------------------------------------------------------------
- Target Operation:  Direct Hardware Port I/O Execution
- Access Type:       Raw Port Read/Write Primitive
- Mechanism:         
  1. Invokes `sub_11790(1)` which executes raw CPU hardware port instructions (`__inbyte`, `__outbyte`).

- Security Impact:   
  - Bypasses standard OS abstractions to interact directly with physical hardware components and motherboard I/O ports.

--------------------------------------------------------------------
IOCTL CODE: 0x80002026 (Decimal: 2147487270)
--------------------------------------------------------------------
- Target Operation:  Low-Level Word Port Interaction
- Access Type:       Raw Port Read/Write Primitive
- Mechanism:         
  1. Executes raw word-sized port operations (`__inword`, `__outword`) using parameters supplied from user-mode.

- Security Impact:   
  - Enables direct control over system hardware buses and legacy devices, commonly abused in privilege escalation or hardware-level tampering.
====================================================================