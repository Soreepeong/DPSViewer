#include "FFXIVDLL.h"
#include "Hooks.h"
#include "GameDataProcess.h"
#include "OverlayRenderer.h"
#include "Tools.h"

#define NT_SUCCESS(x) (((x)) >= 0)
#define STATUS_INFO_LENGTH_MISMATCH 0xc0000004

#define SystemHandleInformation 16
#define ObjectBasicInformation 0
#define ObjectNameInformation 1
#define ObjectTypeInformation 2

typedef NTSTATUS (NTAPI *_NtQuerySystemInformation)(
	ULONG SystemInformationClass,
	PVOID SystemInformation,
	ULONG SystemInformationLength,
	PULONG ReturnLength
	);
typedef NTSTATUS (NTAPI *_NtDuplicateObject)(
	HANDLE SourceProcessHandle,
	HANDLE SourceHandle,
	HANDLE TargetProcessHandle,
	PHANDLE TargetHandle,
	ACCESS_MASK DesiredAccess,
	ULONG Attributes,
	ULONG Options
	);
typedef NTSTATUS (NTAPI *_NtQueryObject)(
	HANDLE ObjectHandle,
	ULONG ObjectInformationClass,
	PVOID ObjectInformation,
	ULONG ObjectInformationLength,
	PULONG ReturnLength
	);

typedef struct _UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _SYSTEM_HANDLE {
	ULONG ProcessId;
	BYTE ObjectTypeNumber;
	BYTE Flags;
	USHORT Handle;
	PVOID Object;
	ACCESS_MASK GrantedAccess;
} SYSTEM_HANDLE, *PSYSTEM_HANDLE;

typedef struct _SYSTEM_HANDLE_INFORMATION {
	ULONG HandleCount;
	SYSTEM_HANDLE Handles[1];
} SYSTEM_HANDLE_INFORMATION, *PSYSTEM_HANDLE_INFORMATION;

typedef enum _POOL_TYPE {
	NonPagedPool,
	PagedPool,
	NonPagedPoolMustSucceed,
	DontUseThisType,
	NonPagedPoolCacheAligned,
	PagedPoolCacheAligned,
	NonPagedPoolCacheAlignedMustS
} POOL_TYPE, *PPOOL_TYPE;

typedef struct _OBJECT_TYPE_INFORMATION {
	UNICODE_STRING Name;
	ULONG TotalNumberOfObjects;
	ULONG TotalNumberOfHandles;
	ULONG TotalPagedPoolUsage;
	ULONG TotalNonPagedPoolUsage;
	ULONG TotalNamePoolUsage;
	ULONG TotalHandleTableUsage;
	ULONG HighWaterNumberOfObjects;
	ULONG HighWaterNumberOfHandles;
	ULONG HighWaterPagedPoolUsage;
	ULONG HighWaterNonPagedPoolUsage;
	ULONG HighWaterNamePoolUsage;
	ULONG HighWaterHandleTableUsage;
	ULONG InvalidAttributes;
	GENERIC_MAPPING GenericMapping;
	ULONG ValidAccess;
	BOOLEAN SecurityRequired;
	BOOLEAN MaintainHandleCount;
	USHORT MaintainTypeList;
	POOL_TYPE PoolType;
	ULONG PagedPoolUsage;
	ULONG NonPagedPoolUsage;
} OBJECT_TYPE_INFORMATION, *POBJECT_TYPE_INFORMATION;

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
				/* We have the type name, so just display that. */
				printf(
					"[%#x] %.*S: (could not get name)\n",
					handle.Handle,
					objectTypeInfo->Name.Length / 2,
					objectTypeInfo->Name.Buffer
				);
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
			ffxivHwnd = handle;
			return false;
		}
	}
	return true;
}

FFXIVDLL::FFXIVDLL(HMODULE instance) :
	hInstance(instance)
{
	RemoveFFXIVMutex();

	EnumWindows(FFXIVDLL::FindFFXIVWindow, (LPARAM) this);

	Languages::initialize();

	hUnloadEvent = CreateEvent(NULL, true, false, NULL);

	pDataProcess = new GameDataProcess(this, hUnloadEvent);
	pHooks = new Hooks(this);

	pHooks->Activate();
	scanner.AddCallback([&] () {
		char res[512];
		sprintf(res, "/e Initialized: PWM %llx, PNL %llx, SFW %llx, HFW %llx, Actor %llx, Party %llx, Target %llx",
			(uint64_t) scanner.Result.Functions.ProcessWindowMessage, 
			(uint64_t) scanner.Result.Functions.ProcessNewLine,
			(uint64_t) scanner.Result.Functions.ShowFFXIVWindow,
			(uint64_t) scanner.Result.Functions.HideFFXIVWindow,
			(uint64_t) scanner.Result.Data.ActorMap,
			(uint64_t) scanner.Result.Data.PartyMap,
			(uint64_t) scanner.Result.Data.TargetMap);
		addChat(res);
	});
}


FFXIVDLL::~FFXIVDLL()
{

	addChat("/e Unloading");

	SetEvent(hUnloadEvent);
	
	while (!pHooks->GetOverlayRenderer()->IsUnloadable())
		Sleep(50);


	delete pHooks;
	delete pDataProcess;

	CloseHandle(hUnloadEvent);
}

DWORD WINAPI FFXIVDLL::SelfUnloaderThread(PVOID p) {
	HINSTANCE inst = ((FFXIVDLL*)p)->hInstance;
	FreeLibraryAndExitThread(inst, 0);
}

void FFXIVDLL::spawnSelfUnloader() {
	CloseHandle(CreateThread(NULL, NULL, FFXIVDLL::SelfUnloaderThread, this, NULL, NULL));
}