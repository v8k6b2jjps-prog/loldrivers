AMD's ATI DSM Dynamic Driver exposes hardware-control IOCTLs through \Device\AtiDCM. 
The interface accepts caller-controlled MSR indexes for RDMSR operations and exposes PCI/device I/O and physical-address mapping paths. 
These privileged primitives can disclose hardware state and support kernel-level tampering when the device is accessible from user mode.