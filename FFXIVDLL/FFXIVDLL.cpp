#include "FFXIVDLL.h"
#include "Hooks.h"
#include "GameDataProcess.h"
#include "OverlayRenderer.h"
#include "Tools.h"
#include "ExternalPipe.h"
#include "QueryHandles.h"


PVOID GetLibraryProcAddress(PSTR LibraryName, PSTR ProcName) {
	return GetProcAddress(GetModuleHandleA(LibraryName), ProcName);
}

// https://forum.sysinternals.com/topic18892.html
void RemoveFFXIVMutex() {
	_NtQuerySystemInformation NtQuerySystemInformation = (_NtQuerySystemInformation) GetLibraryProcAddress("ntdll.dll", "NtQuerySystemInformation");
	_NtDuplicateObject NtDuplicateObject = (_NtDuplicateObject) GetLibraryProcAddress("ntdll.dll", "NtDuplicateObject");
	_NtQueryObject NtQueryObject = (_NtQueryObject) GetLibraryProcAddress("ntdll.dll", "NtQueryObject");
	NTSTATUS status;
	PSYSTEM_HANDLE_INFORMATION handleInfo;
	ULONG handleInfoSize = 0x10000;
	ULONG pid = GetCurrentProcessId();
	HANDLE processHandle = GetCurrentProcess();
	ULONG i;

	handleInfo = (PSYSTEM_HANDLE_INFORMATION) malloc(handleInfoSize);

	/* NtQuerySystemInformation won't give us the correct buffer size, so we guess by doubling the buffer size. */
	while ((status = NtQuerySystemInformation( SystemHandleInformation, handleInfo, handleInfoSize, NULL)) == STATUS_INFO_LENGTH_MISMATCH)
		handleInfo = (PSYSTEM_HANDLE_INFORMATION) realloc(handleInfo, handleInfoSize *= 2);

	/* NtQuerySystemInformation stopped giving us STATUS_INFO_LENGTH_MISMATCH. */
	if (!NT_SUCCESS(status))
		return;

	for (i = 0; i < handleInfo->HandleCount; i++) {
		SYSTEM_HANDLE handle = handleInfo->Handles[i];
		POBJECT_TYPE_INFORMATION objectTypeInfo;
		PVOID objectNameInfo;
		UNICODE_STRING objectName;
		ULONG returnLength;

		/* Check if this handle belongs to the PID the user specified. */
		if (handle.ProcessId != pid)
			continue;

		HANDLE dupHandle = (HANDLE) handle.Handle;

		/* Query the object type. */
		objectTypeInfo = (POBJECT_TYPE_INFORMATION) malloc(0x1000);
		if (!NT_SUCCESS(NtQueryObject(dupHandle, ObjectTypeInformation, objectTypeInfo, 0x1000, NULL )))
			continue;

		/* Query the object name (unless it has an access of 0x0012019f, on which NtQueryObject could hang. */
		if (handle.GrantedAccess == 0x0012019f) {
			free(objectTypeInfo);
			continue;
		}

		objectNameInfo = malloc(0x1000);
		if (!NT_SUCCESS(NtQueryObject( dupHandle, ObjectNameInformation, objectNameInfo, 0x1000, &returnLength ))) {
			/* Reallocate the buffer and try again. */
			objectNameInfo = realloc(objectNameInfo, returnLength);
			if (!NT_SUCCESS(NtQueryObject( dupHandle, ObjectNameInformation, objectNameInfo, returnLength, NULL ))) {
				free(objectTypeInfo);
				free(objectNameInfo);
				continue;
			}
		}

		objectName = *(PUNICODE_STRING) objectNameInfo;

		if (objectName.Length) {
			if (0 == wcsncmp(objectName.Buffer, L"\\BaseNamedObjects\\6AA83AB5-BAC4-4a36-9F66-A309770760CB", 54))
				CloseHandle((HANDLE) handle.Handle);
		}

		free(objectTypeInfo);
		free(objectNameInfo);
	}

	free(handleInfo);
}

BOOL WINAPI FFXIVDLL::FindFFXIVWindow(HWND handle) {
	DWORD pid = 0;
	GetWindowThreadProcessId(handle, &pid);
	if (pid == GetCurrentProcessId()) {
		TCHAR title[512];
		GetWindowText(handle, title, 512);
		if (wcscmp(title, L"FINAL FANTASY XIV") == 0 && IsWindowVisible(handle)) {
			hFFXIVWnd = handle;
			return false;
		}
	}
	return true;
}

FFXIVDLL::FFXIVDLL(HMODULE instance) :
	mInstance(instance)
{
	RemoveFFXIVMutex();

	EnumWindows(FFXIVDLL::FindFFXIVWindow, (LPARAM) this);

	Languages::initialize();

	hUnloadEvent = CreateEvent(NULL, true, false, NULL);

	mPipe = new ExternalPipe(this, hUnloadEvent);
	mDataProcess = new GameDataProcess(this, hUnloadEvent);
	mHooks = new Hooks(this);

	mHooks->Activate();
	mScanner.AddCallback([&] () {
		char res[512];
		sprintf(res, "/e Initialized" ": PWM %llx, PNL %llx, SFW %llx, HFW %llx, ONCI %llx, Actor %llx, Party %llx, Target %llx",
			(uint64_t) mScanner.Result.Functions.ProcessWindowMessage, 
			(uint64_t) mScanner.Result.Functions.ProcessNewLine,
			(uint64_t) mScanner.Result.Functions.ShowFFXIVWindow,
			(uint64_t) mScanner.Result.Functions.HideFFXIVWindow,
			(uint64_t) mScanner.Result.Functions.OnNewChatItem,
			(uint64_t) mScanner.Result.Data.ActorMap,
			(uint64_t) mScanner.Result.Data.PartyMap,
			(uint64_t) mScanner.Result.Data.TargetMap /**/);
		addChat(res);
	});
}


FFXIVDLL::~FFXIVDLL()
{

	addChat("/e Unloading:FFXIVDLL");

	SetEvent(hUnloadEvent);
	
	while (!mHooks->GetOverlayRenderer()->IsUnloadable())
		Sleep(50);

	delete mHooks;
	delete mDataProcess;
	delete mPipe;

	CloseHandle(hUnloadEvent);
}

DWORD WINAPI FFXIVDLL::SelfUnloaderThread(PVOID p) {
	HINSTANCE inst = ((FFXIVDLL*)p)->mInstance;
	FreeLibraryAndExitThread(inst, 0);
}

void FFXIVDLL::spawnSelfUnloader() {
	CloseHandle(CreateThread(NULL, NULL, FFXIVDLL::SelfUnloaderThread, this, NULL, NULL));
}

bool FFXIVDLL::isUnloading() {
	return WAIT_OBJECT_0 == WaitForSingleObject(hUnloadEvent, 0);
}

ImGuiConfigWindow& FFXIVDLL::config() {
	return mHooks->GetOverlayRenderer()->mConfig;
}

MemoryScanner* FFXIVDLL::memory() {
	return &mScanner;
}

HWND FFXIVDLL::ffxiv() {
	return hFFXIVWnd;
}

void FFXIVDLL::addChat(std::string s) {
	if (isUnloading())
		return;
	mChatInjectQueue.push(s);
}

void FFXIVDLL::addChat(char* s) {
	if (isUnloading())
		return;
	mChatInjectQueue.push(s);
}

void FFXIVDLL::sendPipe(char* msgtype, const char* data, size_t length) {
	mPipe->SendInfo(std::string(msgtype) + (~length ? std::string(data, length) : std::string(data)));
}

Hooks* FFXIVDLL::hooks() {
	return mHooks;
}

GameDataProcess* FFXIVDLL::process() {
	return mDataProcess;
}

HINSTANCE FFXIVDLL::instance() {
	return mInstance;
}