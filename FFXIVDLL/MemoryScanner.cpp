#include "MemoryScanner.h"
#include "Tools.h"
#include <Psapi.h>


MemoryScanner::MemoryScanner() {
	DWORD cbNeeded;
	EnumProcessModulesEx(GetCurrentProcess(), &hGame, sizeof(hGame), &cbNeeded, LIST_MODULES_32BIT | LIST_MODULES_64BIT);

	IMAGE_NT_HEADERS *peHeader = (IMAGE_NT_HEADERS*) ((DWORD_PTR) hGame + (DWORD_PTR) ((PIMAGE_DOS_HEADER) hGame)->e_lfanew);
	IMAGE_SECTION_HEADER *sectionHeaders = (IMAGE_SECTION_HEADER*) &(peHeader[1]);
	for (int i = 0; i < peHeader->FileHeader.NumberOfSections; i++, sectionHeaders++)
		if (0 == strncmp((char*) sectionHeaders->Name, ".text", 5))
			goto textSectionFound;
	return;
textSectionFound:
	mScannerThread = CreateThread(NULL, NULL, MemoryScanner::DoScanExternal, this, NULL, NULL);

#ifdef _WIN64
	// KOR v3.4 x64
	ActorMap.push_back(DataSignature("48c1e8033dffff0000742b3da80100007324488d0d", 21, true, 2, new int[2]{ 0, 0 }));

#else
	// GLO v3.57 x86
	ActorMap.push_back(DataSignature("81feffff0000743581fe58010000732d8b3cb5", 19, false, 2, new int[2]{ 0, 0 }));
	TargetMap.push_back(DataSignature("750e85d2750ab9", 7, false, 2, new int[2]{ 0, 108 }));
	PartyMap.push_back(DataSignature("85c074178b407450b9", 9, false, 2, new int[2]{ 0, 16 }));
	/*
	// KOR v3.4 x86
	ActorMap.push_back(DataSignature("81feffff0000743581fe58010000732d8b3cb5", 19, false, 2, new int[2] {0, 0}));
	TargetMap.push_back(DataSignature("750e85d2750ab9", 7, false, 2, new int[2] {0, 108}));
	PartyMap.push_back(DataSignature("85c074178b407450b9", 9, false, 2, new int[2] {0, 16}));
	*/
#endif
}


MemoryScanner::~MemoryScanner() {
}

MemoryScanner::DataSignature::DataSignature(std::string signature, int offset, bool rip, int pathcount, int *path) :
	offset(offset),
	rip(rip) {
	for (auto it = signature.begin(); it != signature.end(); it += 2) {
		char t[3] = { *it, *(&*it + 1), 0 };
		if (t[0] == '*') {
			this->signature.push_back(0);
			this->mask.push_back('?');
		} else {
			this->signature.push_back((char) strtol(t, nullptr, 16));
			this->mask.push_back('x');
		}
	}
	if (pathcount > 0) {
		for (int i = 0; i < pathcount; i++)
			this->path.push_back(path[i]);
		delete path;
	}

}

void MemoryScanner::ScanInto(PVOID *p, std::vector<DataSignature> &map) {
	*p = 0;
	for (auto it = map.begin(); it != map.end(); ++it) {
			char *ptr = (char*) Tools::FindPattern((DWORD_PTR) hGame, peHeader->OptionalHeader.SizeOfImage, (BYTE*) it->signature.c_str(), (char*) it->mask.c_str());
			if (!ptr) continue;
			ptr += it->offset;
			char *next = ptr;
			for (int i = 0; i < it->path.size(); i++) {
				ptr = *(char**) (next + it->path[i]);
				if (i + 1 < it->path.size()){
					if (i == 0 && it->rip)
						next = ptr + 4 + *(int*) ptr;
					else
						next = (char*) *(int*) ptr;
				}
			}
	}
}

DWORD MemoryScanner::DoScan() {

#ifdef _WIN64
	Result.Functions.ProcessWindowMessage = (PVOID) Tools::FindPattern((DWORD_PTR) hGame + sectionHeaders->VirtualAddress, sectionHeaders->Misc.VirtualSize, (PBYTE) "\x40\x53\x48\x83\xEC\x60\x48\x89\x6C\x24\x70\x48\x8D\x4C\x24\x30\x33\xED\x45\x33\xC9\x45\x33\xC0\x33\xD2\x89\x6C\x24\x20\xBB\x0A", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
	Result.Functions.ProcessNewLine = (PVOID) Tools::FindPattern((DWORD_PTR) hGame + sectionHeaders->VirtualAddress, sectionHeaders->Misc.VirtualSize, (PBYTE) "\x48\x89\x5C\x24\x20\x55\x56\x57\x48\x81\xEC\x10\x01\x00\x00\x48\x8B\x05\x00\x00\x00\x01\x48\x33\xC4\x48\x89\x84\x24\x00\x01\x00\x00\x41\x8B\x00\x41\x8B\xE9\x49\x8B\x00\x83\xE0\x0F\x48\x8B\xDA", "xxxxxxxxxxxxxxxxxx???xxxxxxxxxxxxxxxxxxxx?xxxxxx");
#else
	Result.Functions.ProcessWindowMessage = (PVOID) Tools::FindPattern((DWORD) hGame + sectionHeaders->VirtualAddress, sectionHeaders->Misc.VirtualSize, (PBYTE) "\x55\x8B\xEC\x83\xEC\x1C\x53\x8B\x1D\x00\x00\x00\x00\x57\x6A\x00", "xxxxxxxxx????xxx");
	Result.Functions.ProcessNewLine = (PVOID) (hGame + sectionHeaders->VirtualAddress - 1);
	do {
		Result.Functions.ProcessNewLine = (PVOID) Tools::FindPattern((DWORD) Result.Functions.ProcessNewLine + 1,
			sectionHeaders->Misc.VirtualSize - (DWORD) Result.Functions.ProcessNewLine + (DWORD) (hGame + sectionHeaders->VirtualAddress),
			(PBYTE) "\x55\x8B\xEC\x81\xEC\xAC\x00\x00\x00\xA1\x00\x00\x00\x00\x33\xC5\x89\x45\xFC\x53\x8B\x5D", "xxxxxxxxxx????xxxxxxxx");
		if (Result.Functions.ProcessNewLine && 0 != Tools::FindPattern((DWORD) Result.Functions.ProcessNewLine, 0x300,
			(PBYTE) "\xFF\x83\x7D\x10\x02\x7F\x16\x8D\x85\x54\xFF\xFF\xFF\x50\x8D\x4D\xA8\x51\x8D",
			"xxxxxxxxxxxxxxxxxxx"))
			break;
	} while (Result.Functions.ProcessNewLine && (int) (sectionHeaders->Misc.VirtualSize - (DWORD) Result.Functions.ProcessNewLine + (DWORD) (hGame + sectionHeaders->VirtualAddress)) > 0);
#endif

	ScanInto(&Result.Data.ActorMap, ActorMap);
	ScanInto(&Result.Data.TargetMap, TargetMap);
	ScanInto(&Result.Data.PartyMap, PartyMap);


	CloseHandle(mScannerThread);
	mScannerThread = INVALID_HANDLE_VALUE;
	return 0;
}

bool MemoryScanner::IsScanComplete() {
	return mScannerThread == INVALID_HANDLE_VALUE;
}