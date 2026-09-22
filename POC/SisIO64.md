================================================================================
SISIO.SYS - DRIVER IOCTL SUMMARY (NOTEPAD EXPORT)
================================================================================

Driver Name:     SisIO.sys
Primary Risk:    High (Port I/O & Arbitrary Physical Memory Read/Write)

--------------------------------------------------------------------------------
Offset (Decimal)    Offset (Hex)    Target Function    Operation / Purpose
--------------------------------------------------------------------------------
2252800             0x226000        sub_140005274      Synchronous Handler: Returns hardcoded driver 
                                                       signature/version status (0x01000007).

2252808             0x226008        sub_1400013E0      Port Read (__inbyte, __inword, __indword) 
                                                       via queued asynchronous handler.

2252827             0x22601B        sub_140001120      Physical Memory Read (MmMapIoSpace) 
                                                       copying data from kernel/hardware to user space.

2269196             0x22A00C        sub_14000146C      Port Write (__outbyte, __outword, __outdword) 
                                                       via queued asynchronous handler.

2269215             0x22A01F        sub_140001290      Physical Memory Write (MmMapIoSpace) 
                                                       copying data from user space to physical/hardware memory.
================================================================================