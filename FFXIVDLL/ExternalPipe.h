#pragma once
#include<Windows.h>
#include<string>
#include<condition_variable>
#include<deque>
#include "FFXIVDLL.h"
#include "Tools.h"

class ExternalPipe {
	friend class Hooks;
private:

	HANDLE hPipe, hPipeEvent;
	HANDLE hPipeThread = INVALID_HANDLE_VALUE;
	HANDLE hNewChatItemEvent = INVALID_HANDLE_VALUE;
	HANDLE hUnloadEvent;
	FFXIVDLL *dll;
	Tools::bqueue<std::string> streamQueue;

public:
	ExternalPipe(FFXIVDLL *dll, HANDLE unloadEvent);
	~ExternalPipe();

	void SendInfo(std::string str);

	DWORD PipeThread();
	static DWORD WINAPI PipeThreadExternal(PVOID param) { ((ExternalPipe*)param)->PipeThread(); return 0; }
};

