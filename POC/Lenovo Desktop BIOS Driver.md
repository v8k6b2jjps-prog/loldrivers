====================================================================
LBAI.SYS - DRIVER IOCTL SUMMARY (NOTEPAD EXPORT)
====================================================================

Driver Name:     LBAI
Device Path:     \Device\LBAI
Symbolic Link:   \DosDevices\LBAI
Primary Risk:    High (Contiguous Memory, MMIO, & Lower-Device Bridging)

--------------------------------------------------------------------
DISPATCH ROUTINE & CONTROL CODES (sub_1400061E8)
--------------------------------------------------------------------

IOCTL CODE: 0x222400 (Handled by sub_1400065DC)
- Operation: Contiguous Memory Allocation & Hardware Bridge
- Mechanism:         
  1. Validates input sizes (bounds check 0x1 to 0x400 bytes) and verifies a magic header (1111772496).
  2. Allocates physically contiguous memory via MmAllocateContiguousMemory.
  3. Copies the user buffer into the contiguous block and retrieves its physical address.
  4. Dispatches the data/control downstream via an internal communication helper (sub_140001444).
  5. Copies results back to the user buffer and frees the contiguous memory.

IOCTL CODE: 0x222404 (Handled by sub_140006410)
- Operation: Physical MMIO Mapping & Hardware Bridge
- Mechanism:         
  1. Validates input constraints and magic header verification.
  2. Maps physical I/O space via MmMapIoSpace using internal device addresses.
  3. Copies user data into the mapped space, then communicates downstream using sub_140001444.
  4. Copies response data back to the user buffer and unmaps the I/O space via MmUnmapIoSpace.

IOCTL CODE: 0x222408 (Handled by sub_1400067B8)
- Operation: Synchronized State Retrieval
- Mechanism:         
  1. Validates output buffer size requirements (between 4 and 8 bytes).
  2. Waits for an internal driver event state using KeWaitForSingleObject.
  3. Queries internal telemetry/status values via sub_140001300 and returns 4 bytes to user space.

Security Impact:   
  - Interfaces directly with lower hardware device stacks through raw memory mapping, contiguous allocations, and downstream request relaying.
====================================================================