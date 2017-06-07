using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace DpsViewer
{
    partial class Program
    {

        [DllImport("kernel32")]
        public static extern IntPtr CreateRemoteThread(
          IntPtr hProcess,
          IntPtr lpThreadAttributes,
          uint dwStackSize,
          UIntPtr lpStartAddress, // raw Pointer into remote process
          IntPtr lpParameter,
          uint dwCreationFlags,
          out IntPtr lpThreadId
        );

        [DllImport("kernel32", SetLastError = true)]
        public static extern IntPtr CreateRemoteThread(
          IntPtr hProcess,
          IntPtr lpThreadAttributes,
          uint dwStackSize,
          UIntPtr lpStartAddress, // raw Pointer into remote process
          ulong lpParameter,
          uint dwCreationFlags,
          out IntPtr lpThreadId
        );

        [DllImport("kernel32")]
        public static extern IntPtr CreateRemoteThread(
          IntPtr hProcess,
          IntPtr lpThreadAttributes,
          uint dwStackSize,
          UIntPtr lpStartAddress, // raw Pointer into remote process
          uint lpParameter,
          uint dwCreationFlags,
          out IntPtr lpThreadId
        );

        [DllImport("kernel32.dll")]
        public static extern IntPtr OpenProcess(
            UInt32 dwDesiredAccess,
            Int32 bInheritHandle,
            Int32 dwProcessId
            );

        [DllImport("kernel32.dll")]
        public static extern Int32 CloseHandle(
        IntPtr hObject
        );

        [DllImport("kernel32.dll", SetLastError = true, ExactSpelling = true)]
        static extern bool VirtualFreeEx(
            IntPtr hProcess,
            IntPtr lpAddress,
            UIntPtr dwSize,
            uint dwFreeType
            );

        [DllImport("kernel32.dll", CharSet = CharSet.Ansi, ExactSpelling = true)]
        public static extern UIntPtr GetProcAddress(
            IntPtr hModule,
            string procName
            );

        [DllImport("kernel32.dll", SetLastError = true, ExactSpelling = true)]
        static extern IntPtr VirtualAllocEx(
            IntPtr hProcess,
            IntPtr lpAddress,
            uint dwSize,
            uint flAllocationType,
            uint flProtect
            );

        [DllImport("kernel32.dll")]
        static extern bool WriteProcessMemory(
            IntPtr hProcess,
            IntPtr lpBaseAddress,
            string lpBuffer,
            UIntPtr nSize,
            out IntPtr lpNumberOfBytesWritten
        );

        [DllImport("kernel32.dll", CharSet = CharSet.Auto)]
        public static extern IntPtr GetModuleHandle(
            string lpModuleName
            );

        [DllImport("kernel32", SetLastError = true, ExactSpelling = true)]
        internal static extern Int32 WaitForSingleObject(
            IntPtr handle,
            Int32 milliseconds
            );
        [DllImport("kernel32.dll")]
        static extern bool GetExitCodeThread(IntPtr hThread, out uint lpExitCode);
        [DllImport("kernel32.dll")]
        static extern bool TerminateThread(IntPtr hThread, uint dwExitCode);
        [DllImport("kernel32.dll", SetLastError = true)]
        static extern IntPtr CreateToolhelp32Snapshot(SnapshotFlags dwFlags, uint th32ProcessID);
        [Flags]
        public enum SnapshotFlags : uint
        {
            HeapList = 0x00000001,
            Process = 0x00000002,
            Thread = 0x00000004,
            Module = 0x00000008,
            Module32 = 0x00000010,
            All = (HeapList | Process | Thread | Module),
            Inherit = 0x80000000,
            NoHeaps = 0x40000000

        }
        [StructLayout(LayoutKind.Sequential, CharSet = System.Runtime.InteropServices.CharSet.Auto)]
        public struct MODULEENTRY32
        {
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
        static extern bool Module32NextW(IntPtr hSnapshot, ref MODULEENTRY32 lpme);
        [DllImport("kernel32.dll")]
        static extern bool Module32FirstW(IntPtr hSnapshot, ref MODULEENTRY32 lpme);
        [DllImport("kernel32.dll", SetLastError = true)]
        public static extern uint GetProcessId(IntPtr process);

        private static bool runRemoteFunction(IntPtr hProcess, string function, IntPtr lpParameter, out IntPtr exitCode)
        {
            UIntPtr Injector = GetProcAddress(GetModuleHandle("kernel32.dll"), function);
            IntPtr bytesout;
            exitCode = (IntPtr)0;
            if (Injector == null)
                return false;

            IntPtr hThread = CreateRemoteThread(hProcess, (IntPtr)null, 0, Injector, lpParameter, 0, out bytesout);
            if (hThread == null)
                return false;

            int Result = WaitForSingleObject(hThread, -1);
            if (Result == 0x00000080L || Result == 0x00000102L || Result == -1)
            {
                TerminateThread(hThread, 0);
                CloseHandle(hThread);
                return false;
            }
            Thread.Sleep(100);
            uint xc;
            GetExitCodeThread(hThread, out xc);
            exitCode = (IntPtr)xc;
            CloseHandle(hThread);
            return true;
        }

        private static bool runRemoteFunction(IntPtr hProcess, string function, ulong lpParameter, out IntPtr exitCode)
        {
            UIntPtr Injector = GetProcAddress(GetModuleHandle("kernel32.dll"), function);
            IntPtr bytesout;
            exitCode = (IntPtr)0;
            if (Injector == null)
                return false;

            IntPtr hThread = CreateRemoteThread(hProcess, (IntPtr)null, 0, Injector, lpParameter, 0, out bytesout);
            if (hThread == null)
                return false;


            int Result = WaitForSingleObject(hThread, -1);
            if (Result == 0x00000080L || Result == 0x00000102L || Result == -1)
            {
                TerminateThread(hThread, 0);
                CloseHandle(hThread);
                return false;
            }
            Thread.Sleep(100);
            uint xc;
            GetExitCodeThread(hThread, out xc);
            exitCode = (IntPtr)xc;
            CloseHandle(hThread);
            return true;
        }

        private static bool runRemoteFunction(IntPtr hProcess, string function, uint lpParameter, out IntPtr exitCode)
        {
            UIntPtr Injector = GetProcAddress(GetModuleHandle("kernel32.dll"), function);
            IntPtr bytesout;
            exitCode = (IntPtr)0;
            if (Injector == null)
                return false;

            IntPtr hThread = CreateRemoteThread(hProcess, (IntPtr)null, 0, Injector, lpParameter, 0, out bytesout);
            if (hThread == null)
                return false;

            int Result = WaitForSingleObject(hThread, -1);
            if (Result == 0x00000080L || Result == 0x00000102L || Result == -1)
            {
                TerminateThread(hThread, 0);
                CloseHandle(hThread);
                return false;
            }
            Thread.Sleep(100);
            uint xc;
            GetExitCodeThread(hThread, out xc);
            exitCode = (IntPtr)xc;
            CloseHandle(hThread);
            return true;
        }

        private static IntPtr GetModulePtr(IntPtr hProcess, string strDLLName)
        {
            IntPtr th32 = CreateToolhelp32Snapshot(SnapshotFlags.Module, GetProcessId(hProcess));
            MODULEENTRY32 mod = new MODULEENTRY32() { dwSize = (uint)Marshal.SizeOf(typeof(MODULEENTRY32)) };
            if (!Module32FirstW(th32, ref mod))
                return IntPtr.Zero;

            List<string> modules = new List<string>();
            do
            {
                modules.Add(mod.szModule);
                if (mod.szExePath.ToLower() == strDLLName.ToLower())
                    return mod.modBaseAddr;
            }
            while (Module32NextW(th32, ref mod));
            return IntPtr.Zero;
        }

        public static IntPtr InjectDLL(IntPtr hProcess, string strDLLName)
        {
            IntPtr exitCode = IntPtr.Zero;
            IntPtr bytesout;
            int LenWrite = strDLLName.Length + 1;
            IntPtr AllocMem = VirtualAllocEx(hProcess, (IntPtr)null, (uint)LenWrite, 0x1000, 0x40);
			IntPtr ptr = IntPtr.Zero;
			try {
                WriteProcessMemory(hProcess, AllocMem, strDLLName, (UIntPtr)LenWrite, out bytesout);
                if (runRemoteFunction(hProcess, "LoadLibraryA", AllocMem, out exitCode))
                    ptr = GetModulePtr(hProcess, strDLLName);
                return ptr;
            }
            finally
            {
                VirtualFreeEx(hProcess, AllocMem, (UIntPtr)0, 0x8000);
            }
        }

		public static bool ejectDll(IntPtr hProcess, string strDLLName) {
			IntPtr ptr = GetModulePtr(hProcess, strDLLName);
			if (ptr == IntPtr.Zero)
				return false;
			while (ptr != IntPtr.Zero) {
				ejectDll(hProcess, ptr);
				ptr = GetModulePtr(hProcess, strDLLName);
			}
			return true;
		}

        public static IntPtr ejectDll(IntPtr hProcess, IntPtr baseAddr)
        {
            IntPtr exitCode = (IntPtr)0;
            if (IntPtr.Size == 8)
            {
                if (!runRemoteFunction(hProcess, "FreeLibrary", (ulong)baseAddr.ToInt64(), out exitCode))
                    return (IntPtr)0;
            }
            else
            {
                if (!runRemoteFunction(hProcess, "FreeLibrary", (uint)baseAddr.ToInt32(), out exitCode))
                    return (IntPtr)0;
            }
            return exitCode;
        }
    }
}
