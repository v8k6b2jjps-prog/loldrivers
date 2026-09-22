Trellix HX Agent fekern.sys 34.x is affected by CVE-2025-14963. 
The signed FireEye 34.5.0 and 34.8.0 builds tracked here expose the \Device\fekern_00 interface. 
Used as a standalone BYOVD, the vulnerable driver can provide access to LSASS memory and enable local privilege escalation. 
A fully functioning HX Agent restricts access through tamper protection.