SUNIX Serial Driver x64 version 10.1.0.0 is affected by CVE-2024-55412. 
This exact signed build exposes raw I/O-port operations to low-privilege callers through IOCTLs including 0x9C403C04 and 0x9C403C08. 
An attacker can use the unrestricted port read/write primitives to disclose information, 
tamper with privileged hardware state, and elevate privileges.