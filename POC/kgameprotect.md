## Summary

Adds the Microsoft-attested AMD64 `kgameprotect.sys` driver and its LOLDrivers
metadata.

- SHA-256: `6c1d596d18213e24f0c88d58ea7f3ca24114eded806b6198a8abc701251126ee`
- Device: `\\.\kgameprotect`
- Vulnerable IOCTL: `0x222048` (`METHOD_BUFFERED`, `FILE_ANY_ACCESS`)

## Vulnerability

IOCTL `0x222048` accepts a caller-controlled PID and terminates that process
without caller, registered-client, or target-PID authorization.

The dispatch branch checks only that `InputBufferLength >= 4`, reads the first
DWORD from `Irp->AssociatedIrp.SystemBuffer`, and passes it to the termination
helper. The helper calls:

```c
PsLookupProcessByProcessId(pid, &process);
ObOpenObjectByPointer(
    process,
    OBJ_KERNEL_HANDLE,
    NULL,
    PROCESS_TERMINATE,
    *PsProcessType,
    KernelMode,
    &handle);
ZwTerminateProcess(handle, STATUS_ACCESS_DENIED);
```

Static-analysis locations:

- input-length check: `0x14000272B`
- caller-controlled PID load: `0x140002743`
- call to the termination helper: `0x140002774`
- `ObOpenObjectByPointer`: `0x1400088CD`
- `ZwTerminateProcess`: `0x1400088F6`

## Impact

If a low-privilege user can open the control device, the `FILE_ANY_ACCESS`
IOCTL lets that user terminate processes without possessing a user-mode
`PROCESS_TERMINATE` handle. This can cause cross-session denial of service and
impair non-PPL security processes.

The IOCTL behavior was dynamically validated in a virtual machine, and the
YAML is marked `Verified: 'TRUE'`. This contribution does not claim privilege
escalation, arbitrary kernel memory access, or PPL bypass.

## Validation

- confirmed the sample was absent from LOLDrivers by filename and SHA-256
- verified the Authenticode signature locally
- verified the LFS object OID matches the sample SHA-256 and size (59,592 bytes)
- dynamically validated IOCTL `0x222048` in a virtual machine
- parsed the YAML successfully with PyYAML
- reviewed the IOCTL and termination paths in IDA
