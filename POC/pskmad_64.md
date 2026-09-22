Panda Kernel Memory Access Driver versions through 1.1.0.21 are affected by CVE-2023-6330, CVE-2023-6331, and CVE-2023-6332. 
This signed 1.0.0.17 build exposes the \Device\PSMEMDriver interface and handles IOCTL 0xB3702C08, 
which can trigger an out-of-bounds write or disclose arbitrary kernel memory because request data and memory ranges are not adequately validated. 
The flaws can cause a system crash, leak sensitive kernel data, 
and may support kernel code execution when chained with another weakness.