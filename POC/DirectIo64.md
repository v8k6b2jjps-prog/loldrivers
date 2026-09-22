It was noted that the kernel driver "DirectIo64.sys", 
which is bundled with multiple PassMark products (BurnInTest, OSForensics, and PerformanceTest), 
exposes IOCTL functionality that can be reached by any low-privileged user — including processes running at Low Integrity Level.
IOCTL code 0x8011E044 was observed invoking the ZwMapViewOfSection API to map physical memory into the user address space. Critically, 
the SectionOffset parameter passed to ZwMapViewOfSection is derived directly from a user-controlled input buffer without adequate validation. As a result, 
a low-privileged user can specify arbitrary physical memory offsets and gain read access to sensitive kernel data.
Since the driver provides a direct primitive for mapping arbitrary physical memory, 
an attacker can locate and read kernel structures (e.g., EPROCESS) from user mode without any privilege requirement beyond the ability to open a handle to the driver.

