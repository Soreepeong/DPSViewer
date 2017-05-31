#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include<Windows.h>
#include"ExternalPipe.h"
#include"Hooks.h"
#include"GameDataProcess.h"
#include"OverlayRenderer.h"
class FFXIVDLL
{
private:
	HMODULE hInstance;
	HANDLE hUnloadEvent = INVALID_HANDLE_VALUE;
	ExternalPipe *pPipe;
	Hooks *pHooks;
	GameDataProcess *pDataProcess;

public:
	FFXIVDLL(HMODULE instance);
	~FFXIVDLL();

	bool isUnloading() {
		return WAIT_OBJECT_0 == WaitForSingleObject(hUnloadEvent, 0);
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
};

