#include "FFXIVDLL.h"
#include "Hooks.h"
#include "GameDataProcess.h"
#include "OverlayRenderer.h"
#include "Tools.h"

BOOL WINAPI FFXIVDLL::FindFFXIVWindow(HWND handle) {
	DWORD pid = 0;
	GetWindowThreadProcessId(handle, &pid);
	if (pid == GetCurrentProcessId()) {
		TCHAR title[512];
		GetWindowText(handle, title, 512);
		if (wcscmp(title, L"FINAL FANTASY XIV") == 0 && IsWindowVisible(handle)) {
			ffxivHwnd = handle;
			return false;
		}
	}
	return true;
}

FFXIVDLL::FFXIVDLL(HMODULE instance) :
	hInstance(instance)
{
	TCHAR fn[1024];
	GetModuleFileName(hInstance, fn, sizeof(fn));
	wsprintf(wcsrchr(fn, L'\\') + 1, L"FFXIVDLLInfo_%d.txt", GetCurrentProcessId());
	FILE *f = _wfopen(fn, L"r");
	if (f == nullptr)
		return;
	EnumWindows(FFXIVDLL::FindFFXIVWindow, (LPARAM) this);

	Languages::initialize();

	hUnloadEvent = CreateEvent(NULL, true, false, NULL);

	pDataProcess = new GameDataProcess(this, f, hUnloadEvent);
	pHooks = new Hooks(this);

	fclose(f);
	DeleteFile(fn);

	addChat("/e Initialized");
	pHooks->Activate();
}


FFXIVDLL::~FFXIVDLL()
{

	addChat("/e Unloading");
	Sleep(100);

	SetEvent(hUnloadEvent);

	delete pHooks;
	delete pDataProcess;

	CloseHandle(hUnloadEvent);
}

DWORD WINAPI FFXIVDLL::SelfUnloaderThread(PVOID p) {
	HINSTANCE inst = ((FFXIVDLL*)p)->hInstance;
	FreeLibraryAndExitThread(inst, 0);
}

void FFXIVDLL::spawnSelfUnloader() {
	CloseHandle(CreateThread(NULL, NULL, FFXIVDLL::SelfUnloaderThread, this, NULL, NULL));
}