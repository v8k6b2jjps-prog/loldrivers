Intel Hardware Accelerated Execution Manager versions before 7.7.1 are affected by CVE-2022-21812. 
The signed 7.6.5, 7.6.5.3, and 7.7.0 builds tracked here expose the \Device\GHAX interface. 
Improper access control in the driver allows an authenticated local user to cross the user-to-kernel security boundary and elevate privileges.