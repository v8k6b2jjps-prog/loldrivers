cpqsysio64.sys is a Hewlett-Packard physical memory driver that ships as part of the HP ProLiant Support Pack and HP firmware ROM update utilities. 
The driver exposes physical memory read via MmMapIoSpace (IOCTL 0x152EF0) and physical memory allocation and mapping to usermode via ZwOpenSection on DevicePhysicalMemory 
 combined with ZwMapViewOfSection (IOCTL 0x153E80). 
The Compaq heritage driver name (cpq prefix) dates to the HP-Compaq acquisition. 
The driver is only 15KB and is also available in the KeServiceDescriptorTablevulnerable-drivers repository.

