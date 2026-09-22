WibuKey for Windows versions before 6.70 are affected by CVE-2024-45181 and CVE-2024-45182. 
This signed 6.50a WibuKey64.sys build exposes the \Device\WibuKey interface. 
Improper buffer-bound checks allow crafted local requests to read from or write to arbitrary kernel addresses, 
causing kernel memory corruption or denial of service and potentially enabling privilege escalation.