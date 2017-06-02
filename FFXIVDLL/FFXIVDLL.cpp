#include "FFXIVDLL.h"
#include "ExternalPipe.h"
#include "Hooks.h"
#include "GameDataProcess.h"
#include "OverlayRenderer.h"
#include "Tools.h"

FFXIVDLL::FFXIVDLL(HMODULE instance) :
	hInstance(instance)
{
	TCHAR fn[1024];
	GetModuleFileName(hInstance, fn, sizeof(fn));
	wsprintf(wcsrchr(fn, L'\\') + 1, L"FFXIVDLLInfo_%d.txt", GetCurrentProcessId());
	FILE *f = _wfopen(fn, L"r");
	if (f == nullptr)
		return;
	int n;
	fscanf_s(f, "%d", &n);
	ffxivHwnd = (HWND)n;

	Languages::initialize();

	hUnloadEvent = CreateEvent(NULL, true, false, NULL);

	pPipe = new ExternalPipe(hUnloadEvent);
	pDataProcess = new GameDataProcess(this, f, hUnloadEvent);
	pHooks = new Hooks(this, f);

	fclose(f);
	DeleteFile(fn);

	pPipe->AddChat("/e Initialized");
	pHooks->Activate();
}


FFXIVDLL::~FFXIVDLL()
{

	pPipe->AddChat("/e Unloading");
	Sleep(100);

	SetEvent(hUnloadEvent);

	delete pHooks;
	delete pDataProcess;
	delete pPipe;

	CloseHandle(hUnloadEvent);
}

DWORD WINAPI FFXIVDLL::SelfUnloaderThread(PVOID p) {
	HINSTANCE inst = ((FFXIVDLL*)p)->hInstance;
	FreeLibraryAndExitThread(inst, 0);
}

void FFXIVDLL::spawnSelfUnloader() {
	CloseHandle(CreateThread(NULL, NULL, FFXIVDLL::SelfUnloaderThread, this, NULL, NULL));
}