Fujitsu BIOS Driver versions before 2.5.0.0 are affected by CVE-2025-65001. 
This signed 2.4.0.0 build creates the \Device\FBIOSDRV interface. 
A specially crafted request from a local authenticated administrator can trigger an out-of-bounds write, 
 potentially causing arbitrary kernel code execution or a denial of service.