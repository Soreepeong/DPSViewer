#include "ExternalPipe.h"
#include <functional>



ExternalPipe::ExternalPipe(HANDLE unloadEvent) :
	hUnloadEvent(unloadEvent) {
	TCHAR pname[256];
	wsprintf(pname, L"\\\\.\\pipe\\ffxivchatinject_%d", GetCurrentProcessId());
	hPipeRead = CreateNamedPipe(pname, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, 1, 1024 * 16, 1024 * 16, 0, NULL);
	if (hPipeRead == INVALID_HANDLE_VALUE) {
		int err = GetLastError();
		std::string buf;
		buf += "/e create inject pipe error: ";
		buf += std::to_string(err);
		injectQueue.push(buf);
	} else {
		hPipeReaderThread = CreateThread(NULL, NULL, ExternalPipe::PipeReaderThreadExternal, this, NULL, NULL);
	}

	wsprintf(pname, L"\\\\.\\pipe\\ffxivchatstream_%d", GetCurrentProcessId());
	hPipeWrite = CreateNamedPipe(pname, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, 1, 1024 * 16, 1024 * 16, 0, NULL);
	if (hPipeWrite == INVALID_HANDLE_VALUE) {
		int err = GetLastError();
		std::string buf;
		buf += "/e create stream pipe error: ";
		buf += std::to_string(err);
		injectQueue.push(buf);
	} else {
		hPipeWriterThread = CreateThread(NULL, NULL, ExternalPipe::PipeWriterThreadExternal, this, NULL, NULL);
		hNewChatItemEvent = CreateEvent(NULL, false, false, NULL);
	}
}


ExternalPipe::~ExternalPipe() {
	if (hPipeRead != INVALID_HANDLE_VALUE) {
		do {
			DWORD exitCode;
			GetExitCodeThread(hPipeReaderThread, &exitCode);
			if (exitCode != STILL_ACTIVE)
				break;
		} while (WAIT_TIMEOUT == WaitForSingleObject(hPipeReaderThread, 100));
		CloseHandle(hPipeReaderThread);
	}
	if (hPipeWrite != INVALID_HANDLE_VALUE) {
		do {
			DWORD exitCode;
			GetExitCodeThread(hPipeWriterThread, &exitCode);
			if (exitCode != STILL_ACTIVE)
				break;
		} while (WAIT_TIMEOUT == WaitForSingleObject(hPipeWriterThread, 100));
		CloseHandle(hPipeWriterThread);
		CloseHandle(hNewChatItemEvent);
	}
}

void ExternalPipe::AddChat(std::string str) {
	injectQueue.push(str);
	SetEvent(hNewChatItemEvent);
}

void ExternalPipe::SendInfo(std::string str) {
	streamQueue.push(str);
	SetEvent(hNewChatItemEvent);
}

DWORD ExternalPipe::PipeReaderThread() {
	char buffer[1024];
	DWORD dwRead;
	OVERLAPPED ov = { 0 };
	ov.hEvent = CreateEvent(NULL, true, false, NULL);
	HANDLE objects[2] = { hUnloadEvent, ov.hEvent };
	while (WaitForSingleObject(hUnloadEvent, 0) == WAIT_TIMEOUT) {
		ResetEvent(ov.hEvent);
		ConnectNamedPipe(hPipeRead, &ov);
		if (GetLastError() != ERROR_PIPE_CONNECTED && GetLastError() != ERROR_IO_PENDING) {
			std::string e("/e error connecting reader: ");
			e += (int)GetLastError();
			injectQueue.push(e);
			break;
		}
		WaitForMultipleObjects(2, objects, false, INFINITE);
		if (!HasOverlappedIoCompleted(&ov))
			break;

		while (WaitForSingleObject(hUnloadEvent, 0) == WAIT_TIMEOUT) {
			ReadFile(hPipeRead, buffer, sizeof(buffer), NULL, &ov);
			WaitForMultipleObjects(2, objects, false, INFINITE);
			if (!GetOverlappedResult(hPipeRead, &ov, &dwRead, false))
				break;
			injectQueue.push(std::string(buffer, dwRead));
		}
		DisconnectNamedPipe(hPipeRead);
	}
	if (!HasOverlappedIoCompleted(&ov))
		CancelIoEx(hPipeRead, &ov);
	CloseHandle(hPipeRead);
	CloseHandle(ov.hEvent);
	TerminateThread(GetCurrentThread(), 0);
	return 0;
}

DWORD ExternalPipe::PipeWriterThread() {
	OVERLAPPED ov = { 0 };
	ov.hEvent = CreateEvent(NULL, true, false, NULL);
	HANDLE objects_overlapped[2] = { hUnloadEvent, ov.hEvent };
	HANDLE objects_chat[2] = { hUnloadEvent, hNewChatItemEvent };
	while (WaitForSingleObject(hUnloadEvent, 0) == WAIT_TIMEOUT) {
		ResetEvent(ov.hEvent);
		ConnectNamedPipe(hPipeWrite, &ov);
		if (GetLastError() != ERROR_PIPE_CONNECTED && GetLastError() != ERROR_IO_PENDING) {
			std::string e("/e error connecting writer: ");
			e += (int)GetLastError();
			injectQueue.push(e);
			break;
		}
		WaitForMultipleObjects(2, objects_overlapped, false, INFINITE); // exclude newChatItemEvent
		if (!HasOverlappedIoCompleted(&ov))
			break;

		streamQueue.clear();
		while (WaitForMultipleObjects(2, objects_chat, false, INFINITE) != WAIT_TIMEOUT && WaitForSingleObject(hUnloadEvent, 0) == WAIT_TIMEOUT) {
			std::string chatMessage;
			while (streamQueue.tryPop(&chatMessage)) {
				WriteFile(hPipeWrite, chatMessage.c_str(), chatMessage.size(), NULL, &ov);
				WaitForMultipleObjects(2, objects_overlapped, false, INFINITE);
				if (!HasOverlappedIoCompleted(&ov))
					break;
			}
		}
		DisconnectNamedPipe(hPipeWrite);
	}
	if (!HasOverlappedIoCompleted(&ov))
		CancelIoEx(hPipeWrite, &ov);
	CloseHandle(hPipeWrite);
	CloseHandle(ov.hEvent);
	TerminateThread(GetCurrentThread(), 0);
	return 0;
}