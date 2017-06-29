#include "ExternalPipe.h"
#include <functional>



ExternalPipe::ExternalPipe(FFXIVDLL *dll, HANDLE unloadEvent) :
	hUnloadEvent(unloadEvent),
	dll (dll) {
	TCHAR pname[256];
	wsprintf(pname, L"\\\\.\\pipe\\ffxivdll_pipe_%d", GetCurrentProcessId());
	hPipe = CreateNamedPipe(pname, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, 1, 1024 * 16, 1024 * 16, 0, NULL);
	if (hPipe == INVALID_HANDLE_VALUE) {
		int err = GetLastError();
		std::string buf;
		buf += "/e create pipe error: ";
		buf += std::to_string(err);
		dll->addChat(buf);
	} else {
		hPipeThread = CreateThread(NULL, NULL, ExternalPipe::PipeThreadExternal, this, NULL, NULL);
	}
}


ExternalPipe::~ExternalPipe() {
	if (hPipe != INVALID_HANDLE_VALUE) {
		WaitForSingleObject(hPipeThread, INFINITE);
		CloseHandle(hPipeThread);
	}
}

void ExternalPipe::SendInfo(std::string str) {
	streamQueue.push(str);
	SetEvent(hNewChatItemEvent);
}

DWORD ExternalPipe::PipeThread() {
	OVERLAPPED ov = { 0 };
	ov.hEvent = CreateEvent(NULL, true, false, NULL);
	HANDLE objects[2] = { hUnloadEvent, ov.hEvent };
	while (WaitForSingleObject(hUnloadEvent, 0) == WAIT_TIMEOUT) {
		ResetEvent(ov.hEvent);
		ConnectNamedPipe(hPipe, &ov);
		if (GetLastError() != ERROR_PIPE_CONNECTED && GetLastError() != ERROR_IO_PENDING) {
			std::string e("/e error connecting pipe: ");
			e += (int)GetLastError();
			dll->addChat(e);
			Sleep(100);
			continue;
		}
		WaitForMultipleObjects(2, objects, false, INFINITE);
		if (HasOverlappedIoCompleted(&ov)) {
			dll->addChat("/e pipe connected");
			hPipeEvent = CreateEvent(NULL, true, false, NULL);
			ResetEvent(hPipeEvent);
			HANDLE reader = CreateThread(0, 0, [](PVOID p_) -> DWORD {
				ExternalPipe *p = (ExternalPipe*) p_;
				char buffer[1024];
				DWORD dwRead;
				OVERLAPPED ov = { 0 };
				ov.hEvent = CreateEvent(NULL, true, false, NULL);
				HANDLE objects[2] = { p->hPipeEvent, ov.hEvent };
				while (WaitForSingleObject(p->hUnloadEvent, 0) == WAIT_TIMEOUT) {
					ReadFile(p->hPipe, buffer, sizeof(buffer), NULL, &ov);
					WaitForMultipleObjects(2, objects, false, INFINITE);
					if (!GetOverlappedResult(p->hPipe, &ov, &dwRead, false))
						break;
					p->dll->addChat(std::string(buffer, dwRead));
				}
				SetEvent(p->hPipeEvent);
				CloseHandle(ov.hEvent);
				TerminateThread(GetCurrentThread(), 0);
				return 0;
			}, this, 0, 0);
			HANDLE writer = CreateThread(0, 0, [](PVOID p_) -> DWORD {
				ExternalPipe *p = (ExternalPipe*) p_;
				OVERLAPPED ov = { 0 };
				ov.hEvent = CreateEvent(NULL, true, false, NULL);
				HANDLE objects[2] = { p->hPipeEvent, ov.hEvent };
				HANDLE objects_chat[3] = { p->hPipeEvent, p->hNewChatItemEvent, ov.hEvent };
				while (WaitForMultipleObjects(3, objects_chat, false, INFINITE) != WAIT_TIMEOUT && WaitForSingleObject(p->hUnloadEvent, 0) == WAIT_TIMEOUT) {
					std::string chatMessage;
					while (p->streamQueue.tryPop(&chatMessage)) {
						WriteFile(p->hPipe, chatMessage.c_str(), static_cast<DWORD>(chatMessage.size()), NULL, &ov);
						WaitForMultipleObjects(2, objects, false, INFINITE);
						if (!HasOverlappedIoCompleted(&ov))
							goto done;
					}
				}
				done:
				SetEvent(p->hPipeEvent);
				CloseHandle(ov.hEvent);
				TerminateThread(GetCurrentThread(), 0);
				return 0;
			}, this, 0, 0);
			HANDLE waits[3] = { reader, writer, hUnloadEvent };
			if (WaitForMultipleObjects(3, waits, false, INFINITE) != WAIT_TIMEOUT) {
				SetEvent(hPipeEvent);
				WaitForSingleObject(reader, INFINITE);
				WaitForSingleObject(writer, INFINITE);
			}
			CloseHandle(hPipeEvent);
			dll->addChat("/e pipe disconnected");
		}
		DisconnectNamedPipe(hPipe);
	}
	if (!HasOverlappedIoCompleted(&ov))
		CancelIoEx(hPipe, &ov);
	CloseHandle(hPipe);
	CloseHandle(ov.hEvent);
	TerminateThread(GetCurrentThread(), 0);
	return 0;
}