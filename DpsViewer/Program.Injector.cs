using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Threading;

namespace DpsViewer {
	partial class Program {

		private static bool runRemoteFunction(IntPtr hProcess, string function, IntPtr lpParameter, out IntPtr exitCode) {
			UIntPtr Injector = GetProcAddress(GetModuleHandleW("kernel32.dll"), function);
			IntPtr bytesout;
			exitCode = (IntPtr)0;

			if (Injector == null)
				return false;

			IntPtr hThread = CreateRemoteThread(hProcess, (IntPtr)null, 0, Injector, lpParameter, 0, out bytesout);
			if (hThread == null)
				return false;

			int Result = WaitForSingleObject(hThread, -1);
			if (Result == 0x00000080L || Result == 0x00000102L || Result == -1) {
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

		private static IntPtr GetModulePtr(IntPtr hProcess, string strDLLName) {
			IntPtr th32 = CreateToolhelp32Snapshot(SnapshotFlags.Module, GetProcessId(hProcess));
			MODULEENTRY32 mod = new MODULEENTRY32() { dwSize = (uint)Marshal.SizeOf(typeof(MODULEENTRY32)) };
			if (!Module32FirstW(th32, ref mod))
				return IntPtr.Zero;

			List<string> modules = new List<string>();
			do {
				modules.Add(mod.szModule);
				if (mod.szExePath.ToLower() == strDLLName.ToLower())
					return mod.modBaseAddr;
			} while (Module32NextW(th32, ref mod));
			return IntPtr.Zero;
		}

		public static IntPtr InjectDLL(IntPtr hProcess, string strDLLName) {
			IntPtr exitCode = IntPtr.Zero;
			IntPtr bytesout;
			byte[] arr = System.Text.Encoding.Unicode.GetBytes(strDLLName + "\0");
			IntPtr AllocMem = VirtualAllocEx(hProcess, (IntPtr)null, (uint) arr.Length, 0x1000, 0x40);
			IntPtr ptr = IntPtr.Zero;
			try {
				WriteProcessMemory(hProcess, AllocMem, arr, (UIntPtr) arr.Length, out bytesout);
				if (runRemoteFunction(hProcess, "LoadLibraryW", AllocMem, out exitCode))
					ptr = GetModulePtr(hProcess, strDLLName);
				return ptr;
			} finally {
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

		public static IntPtr ejectDll(IntPtr hProcess, IntPtr baseAddr) {
			IntPtr exitCode = (IntPtr)0;
			if (!runRemoteFunction(hProcess, "FreeLibrary", baseAddr, out exitCode))
				return IntPtr.Zero;
			return exitCode;
		}
	}
}
