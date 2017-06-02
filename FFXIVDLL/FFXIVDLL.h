#pragma once
#include<Windows.h>
#include "ExternalPipe.h"
#include "Hooks.h"
#include "GameDataProcess.h"
#include "OverlayRenderer.h"

class ExternalPipe;
class Hooks;
class GameDataProcess;
class OverlayRenderer;

class FFXIVDLL
{
private:
	HMODULE hInstance;
	HANDLE hUnloadEvent = INVALID_HANDLE_VALUE;
	ExternalPipe *pPipe;
	Hooks *pHooks;
	GameDataProcess *pDataProcess;
	HWND ffxivHwnd;

public:
	FFXIVDLL(HMODULE instance);
	~FFXIVDLL();

	bool isUnloading() {
		return WAIT_OBJECT_0 == WaitForSingleObject(hUnloadEvent, 0);
	}

	HWND ffxiv() {
		return ffxivHwnd;
	}

	ExternalPipe* pipe() {
		return pPipe;
	}

	Hooks* hooks() {
		return pHooks;
	}

	GameDataProcess* process() {
		return pDataProcess;
	}

	HINSTANCE instance() {
		return hInstance;
	}

	static DWORD WINAPI SelfUnloaderThread(PVOID p);
	void spawnSelfUnloader();
};