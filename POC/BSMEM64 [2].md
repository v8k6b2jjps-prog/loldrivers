====================================================================
BSMEM.SYS - DRIVER IOCTL SUMMARY (NOTEPAD EXPORT)
====================================================================

Driver Name:     BSMEM.sys
Device Path:     \Device\BSMEM
Symbolic Link:   \DosDevices\BSMEM
Handler Routine: sub_110BC (DriverStartIo)
Primary Risk:    High (Comprehensive Port I/O, PCI, & Physical Memory Access)

--------------------------------------------------------------------
CONTROL CODES & MAPPED OPERATIONS
--------------------------------------------------------------------

Control Code: 2252836 (0x225DD4)
- Operation: Port I/O Read Sequence
- Details:   Translates bus address, executes __outbyte, then __inbyte.

Control Code: 2252864 (0x225DF0)
- Operation: Port I/O Write Sequence
- Details:   Translates bus address, executes __outbyte, then __outbyte.

Control Code: 2252868 (0x225DF4)
- Operation: Physical Memory Read (Arbitrary)
- Details:   Maps physical address using MmMapIoSpace and copies data into user buffer.

Control Code: 2252872 (0x225DF8)
- Operation: PCI Configuration Read (2 Bytes)
- Details:   Calls HalGetBusDataByOffset to read PCI config space.

Control Code: 2252876 (0x225DFC)
- Operation: Byte Read (I/O Port / Translated Address)
- Details:   Translates bus address and executes __inbyte.

Control Code: 2252880 (0x225E00)
- Operation: Byte Write (I/O Port / Translated Address)
- Details:   Translates bus address and executes __outbyte.

Control Code: 2252888 (0x225E08)
- Operation: PCI Configuration Read (1 Byte)
- Details:   Calls HalGetBusDataByOffset for 1-byte read.

Control Code: 2252892 (0x225E0C)
- Operation: PCI Configuration Write
- Details:   Calls HalSetBusDataByOffset to write to PCI config space.

Control Code: 2252912 (0x226050)
- Operation: Word Read (16-bit)
- Details:   Translates address via PCIBus and executes __inword.

Control Code: 2252916 (0x226054)
- Operation: Word Write (16-bit)
- Details:   Translates address via PCIBus and executes __outword.

Control Code: 2252920 (0x226058)
- Operation: Dword Read (32-bit)
- Details:   Translates address via PCIBus and executes __indword.

Control Code: 2252924 (0x22605C)
- Operation: Dword Write (32-bit)
- Details:   Translates address via PCIBus and executes __outdword.

Control Code: 2252928 (0x226060)
- Operation: Physical Memory Write (Arbitrary)
- Details:   Maps physical address using MmMapIoSpace and copies arbitrary data from user buffer into physical RAM.
====================================================================