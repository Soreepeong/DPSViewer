#pragma once
#include<Windows.h>
#include "Tools.h"
#include "MemoryScanner.h"

class Hooks;
class GameDataProcess;
class OverlayRenderer;
class ExternalPipe;
class ImGuiConfigWindow;

class FFXIVDLL
{
private:
	HMODULE mInstance;
	HANDLE hUnloadEvent = INVALID_HANDLE_VALUE;
	Hooks *mHooks;
	GameDataProcess *mDataProcess;
	HWND hFFXIVWnd;
	MemoryScanner mScanner;
	ExternalPipe *mPipe;

	BOOL WINAPI FindFFXIVWindow(HWND h);
	static BOOL WINAPI FindFFXIVWindow(HWND h, LPARAM l) {
		return ((FFXIVDLL*)l)->FindFFXIVWindow(h);
	}

public:
	FFXIVDLL(HMODULE instance);
	~FFXIVDLL();

	Tools::bqueue<std::string> mChatInjectQueue;

	void addChat(std::string s);
	void addChat(char* s);
	void sendPipe(char* msgtype, const char* data, size_t length);

	bool isUnloading();

	ImGuiConfigWindow& config();
	MemoryScanner* memory();
	HWND ffxiv();
	Hooks* hooks();
	GameDataProcess* process();
	HINSTANCE instance();

	static DWORD WINAPI SelfUnloaderThread(PVOID p);
	void spawnSelfUnloader();
};