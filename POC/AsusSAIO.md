ASUS System Analysis IO drivers affected by CVE-2024-55408 expose privileged hardware operations through the \Device\ASUSSAIO interface. 
This 1.0.1.0 build accepts FILE_ANY_ACCESS IOCTLs, including 0x80102074 and 0x80102078, 
that map caller-selected physical addresses with MmMapIoSpace for read and write operations. 
The interface also exposes direct I/O-port access without a caller-token authorization check, 
allowing a local user to cross the user-to-kernel security boundary.