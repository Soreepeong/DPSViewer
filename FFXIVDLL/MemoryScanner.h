#pragma once
#include <string>
#include <vector>
#include <Windows.h>
#include <mutex>

typedef std::function<void()> MemoryScannerCallback;

class MemoryScanner {
public:
	struct {
		struct {
			PVOID ProcessWindowMessage;
			PVOID ProcessNewLine;
			PVOID OnNewChatItem;
			PVOID HideFFXIVWindow;
			PVOID ShowFFXIVWindow;
		} Functions;
		struct {
			PVOID ActorMap;
			PVOID TargetMap;
			PVOID PartyMap;
		} Data;
	} Result;
	struct {
		struct {
			uint32_t id, name, owner, type, job;
		} Actor;
		struct {
			uint32_t current, hover, focus;
		} Target;
	} Struct;
private:

	class DataSignature {
	public: 
		DataSignature(std::string signature, int offset, bool rip, int pathcount, int *path);

		std::string signature, mask;
		int offset;
		bool rip;
		std::vector<int> path;
	};

	std::vector<DataSignature> ActorMap, TargetMap, PartyMap;
	std::vector<MemoryScannerCallback> callbacks;
	std::mutex mCallbackMutex;

	HMODULE hGame;
	IMAGE_NT_HEADERS *peHeader;
	IMAGE_SECTION_HEADER *sectionHeaders;

	void ScanInto(PVOID *p, std::vector<DataSignature> &map);
	bool ScanInto(PVOID *p, DataSignature &s);
	DWORD DoScan();
	static DWORD WINAPI DoScanExternal(PVOID p) { return ((MemoryScanner*) p)->DoScan(); }

	HANDLE mScannerThread;

public:
	MemoryScanner();
	~MemoryScanner();

	bool IsScanComplete();
	void AddCallback(MemoryScannerCallback callback);
};

