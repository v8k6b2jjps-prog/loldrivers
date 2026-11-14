// MyNetLibrary, Version=1.0.0.1, Culture=neutral, PublicKeyToken=null
// MyNetLibrary.WinRing0.PX1Driver
using System;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using MyNetLibrary.WinRing0;

public class PX1Driver : RingZeroBase
{
	[StructLayout(LayoutKind.Sequential, Pack = 4)]
	private struct WrmsrInput
	{
		public uint Register;

		public ulong Value;
	}

	[StructLayout(LayoutKind.Explicit)]
	private struct IoUnion
	{
		[FieldOffset(0)]
		public byte u8;

		[FieldOffset(0)]
		public ushort u16;

		[FieldOffset(0)]
		public uint u32;
	}

	[StructLayout(LayoutKind.Sequential, Pack = 4)]
	private struct WriteIoPortInput
	{
		public uint PortNumber;

		public IoUnion Value;
	}

	[StructLayout(LayoutKind.Sequential, Pack = 4)]
	private struct ReadPciConfigInput
	{
		public byte bus;

		public byte dev;

		public byte fn;

		public uint offset;
	}

	[StructLayout(LayoutKind.Sequential, Pack = 4)]
	private struct WritePciConfigInput
	{
		public byte bus;

		public byte dev;

		public byte fn;

		public uint offset;

		public IoUnion Value;
	}

	[StructLayout(LayoutKind.Sequential, Pack = 8)]
	private struct WriteMemoryInput
	{
		public ulong address;

		public MemoryUnion Value;
	}

	[StructLayout(LayoutKind.Explicit)]
	private struct MemoryUnion
	{
		[FieldOffset(0)]
		public byte u8;

		[FieldOffset(0)]
		public ushort u16;

		[FieldOffset(0)]
		public uint u32;

		[FieldOffset(0)]
		public ulong u64;
	}

	[Flags]
	public enum AllocationType : uint
	{
		COMMIT = 0x1000u,
		RESERVE = 0x2000u,
		RESET = 0x80000u,
		LARGE_PAGES = 0x20000000u,
		PHYSICAL = 0x400000u,
		TOP_DOWN = 0x100000u,
		WRITE_WATCH = 0x200000u
	}

	[Flags]
	public enum MemoryProtection : uint
	{
		EXECUTE = 0x10u,
		EXECUTE_READ = 0x20u,
		EXECUTE_READWRITE = 0x40u,
		EXECUTE_WRITECOPY = 0x80u,
		NOACCESS = 1u,
		READONLY = 2u,
		READWRITE = 4u,
		WRITECOPY = 8u,
		GUARD = 0x100u,
		NOCACHE = 0x200u,
		WRITECOMBINE = 0x400u
	}

	[Flags]
	public enum FreeType
	{
		DECOMMIT = 0x4000,
		RELEASE = 0x8000
	}

	private static class NativeMethods
	{
		private const string KERNEL = "kernel32.dll";

		[DllImport("kernel32.dll")]
		public static extern IntPtr VirtualAlloc(IntPtr lpAddress, UIntPtr dwSize, AllocationType flAllocationType, MemoryProtection flProtect);

		[DllImport("kernel32.dll")]
		public static extern bool VirtualFree(IntPtr lpAddress, UIntPtr dwSize, FreeType dwFreeType);
	}

	private static KernelDriver driver;

	private static string fileName;

	private static readonly StringBuilder report = new StringBuilder();

	private const uint OLS_TYPE = 32768u;

	private static IOControlCode IOCTL_OLS_GET_REFCOUNT = new IOControlCode(32768u, 2304u, IOControlCode.Access.Read);

	private static IOControlCode IOCTL_OLS_READ_MSR = new IOControlCode(32768u, 2307u, IOControlCode.Access.Read);

	private static IOControlCode IOCTL_OLS_WRITE_MSR = new IOControlCode(32768u, 2308u, IOControlCode.Access.Read);

	private static IOControlCode IOCTL_OLS_READ_IO_PORT = new IOControlCode(32768u, 2305u, IOControlCode.Access.Read);

	private static IOControlCode IOCTL_OLS_WRITE_IO_PORT_BYTE = new IOControlCode(32768u, 2306u, IOControlCode.Access.Read);

	private static IOControlCode IOCTL_OLS_WRITE_IO_PORT_DWORD = new IOControlCode(32768u, 2306u, IOControlCode.Access.Read);

	private static IOControlCode IOCTL_OLS_READ_MEMORY = new IOControlCode(32768u, 2309u, IOControlCode.Access.Read);

	private static IOControlCode IOCTL_OLS_WRITE_MEMORY = new IOControlCode(32768u, 2310u, IOControlCode.Access.Read);

	private static IOControlCode IOCTL_OLS_READ_PCI_CONFIG = new IOControlCode(32768u, 2311u, IOControlCode.Access.Read);

	private static IOControlCode IOCTL_OLS_WRITE_PCI_CONFIG = new IOControlCode(32768u, 2312u, IOControlCode.Access.Read);

	public override Assembly GetAssembly()
	{
		return typeof(PX1Driver).Assembly;
	}

	public override bool ExtractDriver(string fileName)
	{
		string value = (MyNetLibrary.WinRing0.OperatingSystem.Is64BitOperatingSystem() ? "driver-x64.sys" : "driver-x86.sys");
		string[] manifestResourceNames = GetAssembly().GetManifestResourceNames();
		byte[] array = null;
		for (int i = 0; i < manifestResourceNames.Length; i++)
		{
			if (manifestResourceNames[i].Contains(value))
			{
				using Stream stream = GetAssembly().GetManifestResourceStream(manifestResourceNames[i]);
				array = new byte[stream.Length];
				stream.Read(array, 0, array.Length);
			}
		}
		if (array == null)
		{
			return false;
		}
		try
		{
			using FileStream fileStream = new FileStream(fileName, FileMode.Create);
			fileStream.Write(array, 0, array.Length);
			fileStream.Flush();
		}
		catch (IOException)
		{
			return false;
		}
		for (int j = 0; j < 20; j++)
		{
			try
			{
				if (File.Exists(fileName) && new FileInfo(fileName).Length == array.Length)
				{
					return true;
				}
				Thread.Sleep(100);
			}
			catch (IOException)
			{
				Thread.Sleep(10);
			}
		}
		return false;
	}

	public override bool IsOpen()
	{
		if (driver != null)
		{
			return driver.IsOpen;
		}
		return false;
	}

	public override void Open()
	{
		int platform = (int)Environment.OSVersion.Platform;
		if (platform == 4 || platform == 128 || driver != null)
		{
			return;
		}
		byte[] array;
		byte[] array2;
		if (IntPtr.Size == 4)
		{
			array = RDTSC_32;
			array2 = CPUID_32;
		}
		else
		{
			array = RDTSC_64;
			array2 = CPUID_64_WINDOWS;
		}
		size = (ulong)(array.Length + array2.Length);
		codeBuffer = NativeMethods.VirtualAlloc(IntPtr.Zero, (UIntPtr)size, AllocationType.COMMIT | AllocationType.RESERVE, MemoryProtection.EXECUTE_READWRITE);
		Marshal.Copy(array, 0, codeBuffer, array.Length);
		__rdtsc = Marshal.GetDelegateForFunctionPointer(codeBuffer, typeof(RdtscDelegate)) as RdtscDelegate;
		IntPtr intPtr = (IntPtr)((long)codeBuffer + array.Length);
		Marshal.Copy(array2, 0, intPtr, array2.Length);
		__cpuid = Marshal.GetDelegateForFunctionPointer(intPtr, typeof(CpuidDelegate)) as CpuidDelegate;
		if (m_bIsCpuid = IsCpuid())
		{
			m_bIsMsr = IsMsr();
			m_bIsTsc = IsTsc();
		}
		report.Length = 0;
		driver = new KernelDriver("Driver");
		driver.Open();
		if (!driver.IsOpen)
		{
			string currentDirectory = Directory.GetCurrentDirectory();
			fileName = (MyNetLibrary.WinRing0.OperatingSystem.Is64BitOperatingSystem() ? "driver-x64.sys" : "driver-x86.sys");
			if (!File.Exists(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86) + "\\EVGA\\Kernel\\" + fileName))
			{
				File.Exists(Path.Combine(currentDirectory, "Kernel", fileName));
				Directory.CreateDirectory(Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86), "EVGA\\Kernel"));
				File.Copy(Path.Combine(currentDirectory, "Kernel", fileName), Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86), "EVGA\\Kernel", fileName), overwrite: false);
			}
			Environment.CurrentDirectory = Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86) + "\\EVGA\\Kernel";
			if (fileName != null)
			{
				if (driver.Install(Environment.CurrentDirectory + "\\" + fileName, out var errorMessage))
				{
					driver.Open();
					if (!driver.IsOpen)
					{
						driver.Delete();
						report.AppendLine("Status: Opening driver failed after install");
					}
				}
				else
				{
					string text = errorMessage;
					driver.Delete();
					Thread.Sleep(2000);
					if (driver.Install(Environment.CurrentDirectory + "\\" + fileName, out var errorMessage2))
					{
						driver.Open();
						if (!driver.IsOpen)
						{
							driver.Delete();
							report.AppendLine("Status: Opening driver failed after reinstall");
						}
					}
					else
					{
						report.AppendLine("Status: Installing driver \"" + fileName + "\" failed" + (File.Exists(fileName) ? " and file exists" : ""));
						report.AppendLine("First Exception: " + text);
						report.AppendLine("Second Exception: " + errorMessage2);
					}
				}
			}
			else
			{
				report.AppendLine("Status: Extracting driver failed");
			}
			Environment.CurrentDirectory = currentDirectory;
			try
			{
				if (File.Exists(fileName))
				{
					File.Delete(fileName);
				}
				fileName = null;
			}
			catch (IOException)
			{
			}
			catch (UnauthorizedAccessException)
			{
			}
		}
		if (!driver.IsOpen)
		{
			driver = null;
		}
	}

	public override void Close()
	{
		if (RingZeroBase._ocMailboxMutex != null)
		{
			RingZeroBase._ocMailboxMutex.Close();
			RingZeroBase._ocMailboxMutex = null;
		}
		__rdtsc = null;
		__cpuid = null;
		if (driver == null)
		{
			return;
		}
		uint outBuffer = 0u;
		driver.DeviceIOControl(IOCTL_OLS_GET_REFCOUNT, null, ref outBuffer);
		driver.Close();
		if (outBuffer <= 1)
		{
			driver.Delete();
		}
		driver = null;
		if (fileName != null && File.Exists(fileName))
		{
			try
			{
				File.Delete(fileName);
				fileName = null;
			}
			catch (IOException)
			{
			}
			catch (UnauthorizedAccessException)
			{
			}
		}
		NativeMethods.VirtualFree(codeBuffer, UIntPtr.Zero, FreeType.RELEASE);
	}

	public override bool Rdmsr(uint index, out uint eax, out uint edx)
	{
		eax = (edx = 0u);
		ulong outBuffer = 0uL;
		bool result = driver.DeviceIOControl(IOCTL_OLS_READ_MSR, index, ref outBuffer);
		edx = (uint)((outBuffer >> 32) & 0xFFFFFFFFu);
		eax = (uint)(outBuffer & 0xFFFFFFFFu);
		return result;
	}

	public override bool Rdpmc(uint counter, ref ulong pullCounter)
	{
		return false;
	}

	public override bool Wrmsr(uint index, uint eax, uint edx)
	{
		if (driver == null)
		{
			return false;
		}
		WrmsrInput wrmsrInput = new WrmsrInput
		{
			Register = index,
			Value = (((ulong)edx << 32) | eax)
		};
		return driver.DeviceIOControl(IOCTL_OLS_WRITE_MSR, wrmsrInput);
	}

	public override byte ReadIoPortByte(uint port)
	{
		if (driver == null)
		{
			return 0;
		}
		ushort num = (ushort)port;
		byte outBuffer = 0;
		driver.DeviceIOControl(IOCTL_OLS_READ_IO_PORT, num, ref outBuffer);
		return (byte)(outBuffer & 0xFF);
	}

	public override void WriteIoPortByte(uint port, byte value)
	{
		if (driver != null)
		{
			WriteIoPortInput writeIoPortInput = new WriteIoPortInput
			{
				PortNumber = (ushort)port,
				Value = 
				{
					u8 = value
				}
			};
			int num = Marshal.SizeOf((object)writeIoPortInput) - Marshal.SizeOf((object)writeIoPortInput.Value) + Marshal.SizeOf((object)value);
			driver.DeviceIOControlReSize(IOCTL_OLS_WRITE_IO_PORT_BYTE, writeIoPortInput, (uint)num);
		}
	}

	public override uint ReadIoPortDWORD(uint port)
	{
		if (driver == null)
		{
			return 0u;
		}
		ushort num = (ushort)port;
		uint outBuffer = 0u;
		driver.DeviceIOControl(IOCTL_OLS_READ_IO_PORT, num, ref outBuffer);
		return outBuffer;
	}

	public override void WriteIoPortDWORD(uint port, uint value)
	{
		if (driver != null)
		{
			WriteIoPortInput writeIoPortInput = new WriteIoPortInput
			{
				PortNumber = port,
				Value = 
				{
					u32 = value
				}
			};
			int num = Marshal.SizeOf((object)writeIoPortInput) - Marshal.SizeOf((object)writeIoPortInput.Value) + Marshal.SizeOf((object)value);
			driver.DeviceIOControlReSize(IOCTL_OLS_WRITE_IO_PORT_DWORD, writeIoPortInput, (uint)num);
		}
	}

	public override uint ReadDmiMemory<T>(ref T buffer, uint count, uint unitSize)
	{
		throw new NotImplementedException();
	}

	public override uint ReadPhysicalMemory<T>(ulong address, ref T buffer, uint count, uint unitSize)
	{
		if (driver == null || buffer == null)
		{
			return 0u;
		}
		if (driver.DeviceIOControl(IOCTL_OLS_READ_MEMORY, address, ref buffer))
		{
			return count * unitSize;
		}
		return 0u;
	}

	public override uint ReadPhysicalMemoryByte(ulong address, ref byte buffer, uint count, uint unitSize)
	{
		if (driver == null)
		{
			return 0u;
		}
		byte outBuffer = 0;
		if (!driver.DeviceIOControl(IOCTL_OLS_READ_MEMORY, address, ref outBuffer))
		{
			return 0u;
		}
		buffer = outBuffer;
		return outBuffer;
	}

	public override uint WritePhysicalMemoryByte(ulong address, byte buffer, uint count, uint unitSize)
	{
		if (driver == null)
		{
			return 0u;
		}
		WriteMemoryInput writeMemoryInput = new WriteMemoryInput
		{
			address = address,
			Value = 
			{
				u8 = buffer
			}
		};
		int num = Marshal.SizeOf((object)writeMemoryInput) - Marshal.SizeOf((object)writeMemoryInput.Value) + Marshal.SizeOf((object)buffer);
		driver.DeviceIOControlReSize(IOCTL_OLS_WRITE_MEMORY, writeMemoryInput, (uint)num);
		return 1u;
	}

	public override uint WritePhysicalMemoryUInt(ulong address, uint buffer, uint count, uint unitSize)
	{
		if (driver == null)
		{
			return 0u;
		}
		WriteMemoryInput writeMemoryInput = new WriteMemoryInput
		{
			address = address,
			Value = 
			{
				u64 = buffer
			}
		};
		int num = Marshal.SizeOf((object)writeMemoryInput) - Marshal.SizeOf((object)writeMemoryInput.Value) + Marshal.SizeOf((object)buffer);
		if (driver.DeviceIOControlReSize(IOCTL_OLS_WRITE_MEMORY, writeMemoryInput, (uint)num))
		{
			return count * unitSize;
		}
		return 0u;
	}

	public override uint WritePhysicalMemory<T>(ulong address, ref T buffer, uint count, uint unitSize)
	{
		if (driver == null)
		{
			return 0u;
		}
		int num = 0;
		object obj = buffer;
		WriteMemoryInput writeMemoryInput = new WriteMemoryInput
		{
			address = address
		};
		switch (unitSize)
		{
		case 1u:
			writeMemoryInput.Value.u8 = (byte)obj;
			num = Marshal.SizeOf((object)writeMemoryInput) - Marshal.SizeOf((object)writeMemoryInput.Value) + Marshal.SizeOf((object)writeMemoryInput.Value.u8);
			break;
		case 2u:
			writeMemoryInput.Value.u16 = (ushort)obj;
			num = Marshal.SizeOf((object)writeMemoryInput) - Marshal.SizeOf((object)writeMemoryInput.Value) + Marshal.SizeOf((object)writeMemoryInput.Value.u16);
			break;
		case 4u:
			writeMemoryInput.Value.u32 = (uint)obj;
			num = Marshal.SizeOf((object)writeMemoryInput) - Marshal.SizeOf((object)writeMemoryInput.Value) + Marshal.SizeOf((object)writeMemoryInput.Value.u32);
			break;
		case 8u:
			writeMemoryInput.Value.u64 = (ulong)obj;
			num = Marshal.SizeOf((object)writeMemoryInput) - Marshal.SizeOf((object)writeMemoryInput.Value) + Marshal.SizeOf((object)writeMemoryInput.Value.u64);
			break;
		}
		if (driver.DeviceIOControlReSize(IOCTL_OLS_WRITE_MEMORY, writeMemoryInput, (uint)num))
		{
			return count * unitSize;
		}
		return 0u;
	}

	public override bool ReadPciConfigByte(uint pciAddress, uint regAddress, out byte value)
	{
		value = byte.MaxValue;
		if (driver == null)
		{
			return false;
		}
		ReadPciConfigInput readPciConfigInput = new ReadPciConfigInput
		{
			fn = (byte)(pciAddress & 7),
			dev = (byte)((pciAddress & 0xF8) >> 3),
			bus = (byte)((pciAddress & 0xFF00) >> 8),
			offset = regAddress
		};
		bool flag = driver.DeviceIOControl(IOCTL_OLS_READ_PCI_CONFIG, readPciConfigInput, ref value);
		if (!flag)
		{
			value = byte.MaxValue;
		}
		return flag;
	}

	public override bool ReadPciConfigWord(uint pciAddress, uint regAddress, out ushort value)
	{
		value = 255;
		if (driver == null)
		{
			return false;
		}
		ReadPciConfigInput readPciConfigInput = new ReadPciConfigInput
		{
			fn = (byte)(pciAddress & 7),
			dev = (byte)((pciAddress & 0xF8) >> 3),
			bus = (byte)((pciAddress & 0xFF00) >> 8),
			offset = regAddress
		};
		bool flag = driver.DeviceIOControl(IOCTL_OLS_READ_PCI_CONFIG, readPciConfigInput, ref value);
		if (!flag)
		{
			value = ushort.MaxValue;
		}
		return flag;
	}

	public override bool ReadPciConfigDword(uint pciAddress, uint regAddress, out uint value)
	{
		value = 255u;
		if (driver == null)
		{
			return false;
		}
		ReadPciConfigInput readPciConfigInput = new ReadPciConfigInput
		{
			fn = (byte)(pciAddress & 7),
			dev = (byte)((pciAddress & 0xF8) >> 3),
			bus = (byte)((pciAddress & 0xFF00) >> 8),
			offset = regAddress
		};
		bool flag = driver.DeviceIOControl(IOCTL_OLS_READ_PCI_CONFIG, readPciConfigInput, ref value);
		if (!flag)
		{
			value = uint.MaxValue;
		}
		return flag;
	}

	public override bool WritePciConfigByte(uint pciAddress, uint regAddress, byte value)
	{
		if (driver == null)
		{
			return false;
		}
		WritePciConfigInput writePciConfigInput = new WritePciConfigInput
		{
			fn = (byte)(pciAddress & 7),
			dev = (byte)((pciAddress & 0xF8) >> 3),
			bus = (byte)((pciAddress & 0xFF00) >> 8),
			offset = regAddress,
			Value = 
			{
				u8 = value
			}
		};
		int num = Marshal.SizeOf((object)writePciConfigInput) - Marshal.SizeOf((object)writePciConfigInput.Value) + Marshal.SizeOf((object)value);
		return driver.DeviceIOControlReSize(IOCTL_OLS_WRITE_PCI_CONFIG, writePciConfigInput, (uint)num);
	}

	public override bool WritePciConfigWord(uint pciAddress, uint regAddress, ushort value)
	{
		if (driver == null)
		{
			return false;
		}
		WritePciConfigInput writePciConfigInput = new WritePciConfigInput
		{
			fn = (byte)(pciAddress & 7),
			dev = (byte)((pciAddress & 0xF8) >> 3),
			bus = (byte)((pciAddress & 0xFF00) >> 8),
			offset = regAddress,
			Value = 
			{
				u16 = value
			}
		};
		int num = Marshal.SizeOf((object)writePciConfigInput) - Marshal.SizeOf((object)writePciConfigInput.Value) + Marshal.SizeOf((object)value);
		return driver.DeviceIOControlReSize(IOCTL_OLS_WRITE_PCI_CONFIG, writePciConfigInput, (uint)num);
	}

	public override bool WritePciConfigDword(uint pciAddress, uint regAddress, uint value)
	{
		if (driver == null)
		{
			return false;
		}
		WritePciConfigInput writePciConfigInput = new WritePciConfigInput
		{
			fn = (byte)(pciAddress & 7),
			dev = (byte)((pciAddress & 0xF8) >> 3),
			bus = (byte)((pciAddress & 0xFF00) >> 8),
			offset = regAddress,
			Value = 
			{
				u32 = value
			}
		};
		int num = Marshal.SizeOf((object)writePciConfigInput) - Marshal.SizeOf((object)writePciConfigInput.Value) + Marshal.SizeOf((object)value);
		return driver.DeviceIOControlReSize(IOCTL_OLS_WRITE_PCI_CONFIG, writePciConfigInput, (uint)num);
	}

	public static string GetReport()
	{
		if (report.Length > 0)
		{
			StringBuilder stringBuilder = new StringBuilder();
			stringBuilder.AppendLine("Ring0");
			stringBuilder.AppendLine();
			stringBuilder.Append(report);
			stringBuilder.AppendLine();
			return stringBuilder.ToString();
		}
		return null;
	}
}
