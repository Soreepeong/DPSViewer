#pragma once
#include<Windows.h>
#include "Hooks.h"
#include "GameDataProcess.h"
#include "OverlayRenderer.h"
#include "Tools.h"

class Hooks;
class GameDataProcess;
class OverlayRenderer;

class FFXIVDLL
{
private:
	HMODULE hInstance;
	HANDLE hUnloadEvent = INVALID_HANDLE_VALUE;
	Hooks *pHooks;
	GameDataProcess *pDataProcess;
	HWND ffxivHwnd;


	BOOL WINAPI FindFFXIVWindow(HWND h);
	static BOOL WINAPI FindFFXIVWindow(HWND h, LPARAM l) {
		return ((FFXIVDLL*)l)->FindFFXIVWindow(h);
	}

public:
	FFXIVDLL(HMODULE instance);
	~FFXIVDLL();

	Tools::bqueue<std::string> injectQueue;

	bool isUnloading() {
		return WAIT_OBJECT_0 == WaitForSingleObject(hUnloadEvent, 0);
	}

	HWND ffxiv() {
		return ffxivHwnd;
	}

	void addChat(std::string s) {
		injectQueue.push(s);
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