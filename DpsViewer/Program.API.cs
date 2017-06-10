using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace DpsViewer {
	partial class Program {
		public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
		[DllImport("user32.dll", CharSet = CharSet.Unicode)]
		public static extern int GetWindowText(IntPtr hWnd, StringBuilder strText, int maxCount);
		[DllImport("user32.dll", CharSet = CharSet.Unicode)]
		public static extern int GetWindowTextLength(IntPtr hWnd);
		[DllImport("user32.dll")]
		public static extern bool EnumWindows(EnumWindowsProc enumProc, IntPtr lParam);
		[DllImport("user32.dll")]
		public static extern bool IsWindowVisible(IntPtr hWnd);
		[DllImport("user32.dll")]
		[return: MarshalAs(UnmanagedType.Bool)]
		public static extern bool SetForegroundWindow(IntPtr hWnd);
		[DllImport("user32.dll", SetLastError = true)]
		public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint processId);
		[DllImport("kernel32.dll")]
		public static extern void ExitProcess(uint uExitCode);
		[DllImport("kernel32")]
		public static extern IntPtr CreateRemoteThread(IntPtr hProcess, IntPtr lpThreadAttributes, uint dwStackSize, UIntPtr lpStartAddress, IntPtr lpParameter, uint dwCreationFlags, out IntPtr lpThreadId);
		[Flags]
		public enum ProcessAccessFlags : uint {
			All = 0x001F0FFF,
			Terminate = 0x00000001,
			CreateThread = 0x00000002,
			VirtualMemoryOperation = 0x00000008,
			VirtualMemoryRead = 0x00000010,
			VirtualMemoryWrite = 0x00000020,
			DuplicateHandle = 0x00000040,
			CreateProcess = 0x000000080,
			SetQuota = 0x00000100,
			SetInformation = 0x00000200,
			QueryInformation = 0x00000400,
			QueryLimitedInformation = 0x00001000,
			Synchronize = 0x00100000
		}
		[DllImport("kernel32.dll", SetLastError = true)]
		public static extern IntPtr OpenProcess(ProcessAccessFlags processAccess, bool bInheritHandle, int processId);
		[DllImport("kernel32.dll")]
		public static extern Int32 CloseHandle(IntPtr hObject);
		[DllImport("kernel32.dll", SetLastError = true, ExactSpelling = true)]
		public static extern bool VirtualFreeEx(IntPtr hProcess, IntPtr lpAddress, UIntPtr dwSize, uint dwFreeType);
		[DllImport("kernel32.dll", CharSet = CharSet.Ansi, ExactSpelling = true)]
		public static extern UIntPtr GetProcAddress(IntPtr hModule, string procName);
		[DllImport("kernel32.dll", SetLastError = true, ExactSpelling = true)]
		static extern IntPtr VirtualAllocEx(IntPtr hProcess, IntPtr lpAddress, uint dwSize, uint flAllocationType, uint flProtect);
		[DllImport("kernel32.dll")]
		static extern bool WriteProcessMemory(IntPtr hProcess, IntPtr lpBaseAddress, byte[] lpBuffer, UIntPtr nSize, out IntPtr lpNumberOfBytesWritten);
		[DllImport("kernel32.dll", CharSet = CharSet.Unicode)]
		public static extern IntPtr GetModuleHandleW(string lpModuleName);
		[DllImport("kernel32", SetLastError = true, ExactSpelling = true)]
		public static extern Int32 WaitForSingleObject(IntPtr handle, Int32 milliseconds);
		[DllImport("kernel32.dll")]
		public static extern bool GetExitCodeThread(IntPtr hThread, out uint lpExitCode);
		[DllImport("kernel32.dll")]
		public static extern bool TerminateThread(IntPtr hThread, uint dwExitCode);
		[DllImport("kernel32.dll", SetLastError = true)]
		public static extern IntPtr CreateToolhelp32Snapshot(SnapshotFlags dwFlags, uint th32ProcessID);
		[Flags]
		public enum SnapshotFlags : uint { HeapList = 0x00000001, Process = 0x00000002, Thread = 0x00000004, Module = 0x00000008, Module32 = 0x00000010, All = (HeapList | Process | Thread | Module), Inherit = 0x80000000, NoHeaps = 0x40000000 }
		[StructLayout(LayoutKind.Sequential, CharSet = System.Runtime.InteropServices.CharSet.Auto)]
		public struct MODULEENTRY32 {
			internal uint dwSize;
			internal uint th32ModuleID;
			internal uint th32ProcessID;
			internal uint GlblcntUsage;
			internal uint ProccntUsage;
			internal IntPtr modBaseAddr;
			internal uint modBaseSize;
			internal IntPtr hModule;
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)]
			internal string szModule;
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst = 260)]
			internal string szExePath;
		}
		[DllImport("kernel32.dll")]
		public static extern bool Module32NextW(IntPtr hSnapshot, ref MODULEENTRY32 lpme);
		[DllImport("kernel32.dll")]
		public static extern bool Module32FirstW(IntPtr hSnapshot, ref MODULEENTRY32 lpme);
		[DllImport("kernel32.dll", SetLastError = true)]
		public static extern uint GetProcessId(IntPtr process);

	}
}
