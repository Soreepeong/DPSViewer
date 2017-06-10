using System;
using System.Diagnostics;
using System.Collections.Generic;
using System.Windows.Forms;
using System.Text;

namespace DpsViewer {
	partial class Program {
		public static void Main(string[] args) {
			List<Process> processes = new List<Process>();
			if(IntPtr.Size == 4)
				processes.AddRange(Process.GetProcessesByName("ffxiv"));
			else
				processes.AddRange(Process.GetProcessesByName("ffxiv_dx11"));
			processes.RemoveAll((item) => {
				try {
					return item.HasExited;
				} catch (Exception) {
					return true;
				}
			});
			if (processes.Count == 0) {
				MessageBox.Show("FFXIV is not running.");
			} else {
				foreach (var p in processes) {
					IntPtr ffxivhWnd = IntPtr.Zero;
					EnumWindows(new EnumWindowsProc((hWnd, lparam) => {
						int size = GetWindowTextLength(hWnd);
						if (size++ > 0 && IsWindowVisible(hWnd)) {
							StringBuilder sb = new StringBuilder(size);
							GetWindowText(hWnd, sb, size);
							if (sb.ToString().StartsWith("FINAL FANTASY XIV")) {
								uint pid;
								GetWindowThreadProcessId(hWnd, out pid);
								if (pid == p.Id) {
									ffxivhWnd = hWnd;
									return false;
								}
							}
						}
						return true;
					}), new IntPtr(p.Id));
					if (ffxivhWnd.ToInt32() == 0) {
						MessageBox.Show("FFXIV window not found");
						return;
					}
					string dllFN = "FFXIVDLL_" + (IntPtr.Size == 4 ? "x86" : "x64") + ".dll";

					IntPtr h = OpenProcess(ProcessAccessFlags.All, false, p.Id);
					if (h.ToInt32() != -1) {
						ejectDll(p.Handle, AppDomain.CurrentDo‌​main.BaseDirectory + dllFN);
						InjectDLL(p.Handle, AppDomain.CurrentDo‌​main.BaseDirectory + dllFN);
						CloseHandle(h);
						SetForegroundWindow(ffxivhWnd);
					}
				}
			}
		}
	}
}
