#include"Tools.h"
#include<chrono>
#include <tlhelp32.h>
#include <psapi.h>
void Tools::MillisToSystemTime(UINT64 millis, SYSTEMTIME *st)
{
	UINT64 multiplier = 10000;
	UINT64 t = multiplier * millis;
	ULARGE_INTEGER li;
	li.QuadPart = t;
	FILETIME ft;
	ft.dwLowDateTime = li.LowPart;
	ft.dwHighDateTime = li.HighPart;
	::FileTimeToSystemTime(&ft, st);
}

void Tools::MillisToLocalTime(UINT64 millis, SYSTEMTIME *st)
{
	UINT64 multiplier = 10000;
	UINT64 t = multiplier * millis;
	ULARGE_INTEGER li;
	li.QuadPart = t;
	FILETIME ft, ft2;
	ft.dwLowDateTime = li.LowPart;
	ft.dwHighDateTime = li.HighPart;
	::FileTimeToLocalFileTime(&ft, &ft2);
	::FileTimeToSystemTime(&ft2, st);
}

uint64_t Tools::GetLocalTimestamp() {
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

bool Tools::DirExists(const std::wstring& dirName_in) {
	DWORD ftyp = GetFileAttributesW(dirName_in.c_str());
	if (ftyp == INVALID_FILE_ATTRIBUTES)
		return false;  //something is wrong with your path!

	if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
		return true;   // this is a directory!

	return false;    // this is not a directory!
}

DWORD Tools::GetMainThreadID(DWORD dwProcID)
{
	DWORD dwMainThreadID = 0;
	ULONGLONG ullMinCreateTime = ((ULONGLONG)~((ULONGLONG)0));

	HANDLE hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (hThreadSnap != INVALID_HANDLE_VALUE) {
		THREADENTRY32 th32;
		th32.dwSize = sizeof(THREADENTRY32);
		BOOL bOK = TRUE;
		for (bOK = Thread32First(hThreadSnap, &th32); bOK;
			bOK = Thread32Next(hThreadSnap, &th32)) {
			if (th32.th32OwnerProcessID == dwProcID) {
				HANDLE hThread = OpenThread(THREAD_QUERY_INFORMATION,
					TRUE, th32.th32ThreadID);
				if (hThread) {
					FILETIME afTimes[4] = { 0 };
					if (GetThreadTimes(hThread,
						&afTimes[0], &afTimes[1], &afTimes[2], &afTimes[3])) {
						LARGE_INTEGER li;
						li.LowPart = afTimes[0].dwLowDateTime;
						li.HighPart = afTimes[0].dwHighDateTime;
						if (li.QuadPart && (uint64_t)li.QuadPart < ullMinCreateTime) {
							ullMinCreateTime = li.QuadPart;
							dwMainThreadID = th32.th32ThreadID; // let it be main... :)
						}
					}
					CloseHandle(hThread);
				}
			}
		}
#ifndef UNDER_CE
		CloseHandle(hThreadSnap);
#else
		CloseToolhelp32Snapshot(hThreadSnap);
#endif
	}

	return dwMainThreadID;
}

bool Tools::BinaryCompare(const BYTE* pData, const BYTE* bMask, const char* szMask) {
	__try {
		for (; *szMask; ++szMask, ++pData, ++bMask)
			if (*szMask == 'x' && *pData != *bMask)
				return false;

		return (*szMask) == NULL;
	} __except(EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}
}
DWORD_PTR Tools::FindPattern(DWORD_PTR dwAddress, DWORD_PTR dwLen, BYTE *bMask, char * szMask) {
	for (DWORD_PTR i = 0; i < dwLen; i++)
		if (BinaryCompare((BYTE*)(dwAddress + i), bMask, szMask))
			return (DWORD_PTR)(dwAddress + i);

	return 0;
}

bool Tools::TestValidString(char* p) {
	__try {
		int i;
		for (i = 0; i < 128; i++)
			if (p[i] == 0)
				return true;
	} __except (EXCEPTION_EXECUTE_HANDLER) {
	}
	return false;
}