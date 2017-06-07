// dllmain.cpp : Defines the entry point for the DLL application.
#include "FFXIVDLL.h"

FFXIVDLL *ffxivDll;

DWORD WINAPI CreatorThread(PVOID p) {
	ffxivDll = new FFXIVDLL((HMODULE) p);
	return 0;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hModule);
		CloseHandle(CreateThread(NULL, NULL, CreatorThread, hModule, NULL, NULL));
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		delete ffxivDll;
		break;
	}
	return TRUE;
}