Biostar BSMEM64_W10 exposes direct physical-memory and port/PCI access through \Device\BSMEM. 
IOCTLs 0x226044 and 0x226084 map caller-selected physical addresses with MmMapIoSpace and copy data from or to the mapped region, 
providing arbitrary physical-memory read and write primitives.