PGPwded.sys in Symantec Encryption Desktop and Symantec Endpoint Encryption is affected by CVE-2019-9702. 
The storage filter exposes raw-disk read and write IOCTLs 0x8002206C and 0x80022070. 
Public research shows that a low-privileged user can reach these operations through a trusted PGP process, 
 overwrite sectors backing protected system files, and gain SYSTEM execution after reboot. 
Symantec Encryption Desktop is affected in all versions and Symantec Endpoint Encryption is affected before 11.3.0.