The 32-bit giveio driver bundled with SpeedFan creates the \.\giveio device and - 
 handles IRP_MJ_CREATE by applying a zeroed 8 KiB I/O permission map to the opening process through Ke386IoSetAccessProcess and Ke386SetIoAccessMap.
This grants unrestricted direct access to all x86 I/O ports without validating the caller.