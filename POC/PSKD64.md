APSoft PCIScope's PSKD64 driver exposes memory-access operations through \.\PSKD64. 
IOCTL 0x220044 dispatches sub-operations 0x701E and 0x701F for caller-selected virtual or physical memory reads and writes without constraining the target address. 
Public testing demonstrates arbitrary kernel read/write on Windows 11, including process protection removal and security-process termination.