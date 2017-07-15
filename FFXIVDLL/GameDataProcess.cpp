#include <winsock2.h>
#include <Windows.h>
#include <deque>
#include <vector>
#include <map>
#include <algorithm>
#include "resource.h"
#include "Languages.h"
#include "FFXIVDLL.h"
#include "GameDataProcess.h"
#include "MedianCalculator.h"
#include "DPSWindowController.h"
#include "DOTWindowController.h"
#include "ChatWindowController.h"
#include "ImGuiConfigWindow.h"
#include "OverlayRenderer.h"
#include "Hooks.h"
#include <psapi.h>
#include "FFXIVResources.h"

#define LIGHTER_COLOR(color,how) (((color)&0xFF000000) | (min(255, max(0, (((color)>>16)&0xFF)+(how)))<<16) | (min(255, max(0, (((color)>>8)&0xFF)+(how)))<<8) | (min(255, max(0, (((color)>>0)&0xFF)+(how)))<<0)))

GameDataProcess::GameDataProcess(FFXIVDLL *dll, HANDLE unloadEvent) :
	dll(dll),
	hUnloadEvent(unloadEvent),
	mLastIdleTime(0),
	mLastSkillRequest(0),
	wDPS(*new DPSWindowController()),
	wDOT(*new DOTWindowController()),
	wChat(*new ChatWindowController(dll)) {
	mLastAttack.amount = 0;
	mLastAttack.timestamp = 0;

	mLocalTimestamp = mServerTimestamp = Tools::GetLocalTimestamp();

	hRecvUpdateInfoThreadLock = CreateEvent(NULL, false, false, NULL);
	hRecvUpdateInfoThread = CreateThread(NULL, NULL, GameDataProcess::RecvUpdateInfoThreadExternal, this, NULL, NULL);
	hSendUpdateInfoThreadLock = CreateEvent(NULL, false, false, NULL);
	hSendUpdateInfoThread = CreateThread(NULL, NULL, GameDataProcess::SendUpdateInfoThreadExternal, this, NULL, NULL);

	TCHAR ffxivPath[MAX_PATH];
	GetModuleFileNameEx(GetCurrentProcess(), NULL, ffxivPath, MAX_PATH);
	bool kor = wcsstr(ffxivPath, L"KOREA") ? true : false;
	version = kor ? 340 : 400;

	// default colors

	wDPS.statusMap[CONTROL_STATUS_DEFAULT] = { 0, 0, 0x40000000, 0 };
	wDPS.statusMap[CONTROL_STATUS_HOVER] = { 0, 0, 0x70555555, 0 };
	wDPS.statusMap[CONTROL_STATUS_FOCUS] = { 0, 0, 0x70333333, 0 };
	wDPS.statusMap[CONTROL_STATUS_PRESS] = { 0, 0, 0x70000000, 0 };
	wDOT.statusMap[CONTROL_STATUS_DEFAULT] = { 0, 0, 0x40000000, 0 };
	wDOT.statusMap[CONTROL_STATUS_HOVER] = { 0, 0, 0x70555555, 0 };
	wDOT.statusMap[CONTROL_STATUS_FOCUS] = { 0, 0, 0x70333333, 0 };
	wDOT.statusMap[CONTROL_STATUS_PRESS] = { 0, 0, 0x70000000, 0 };

	HRSRC hResource = FindResource(dll->instance(), MAKEINTRESOURCE(IDR_TEXT_COLORDEF), L"TEXT");
	if (hResource) {
		HGLOBAL hLoadedResource = LoadResource(dll->instance(), hResource);
		if (hLoadedResource) {
			LPVOID pLockedResource = LockResource(hLoadedResource);
			if (pLockedResource) {
				DWORD dwResourceSize = SizeofResource(dll->instance(), hResource);
				if (0 != dwResourceSize) {
					struct membuf : std::streambuf {
						membuf(char* base, std::ptrdiff_t n) {
							this->setg(base, base, base + n);
						}
					} sbuf((char*) pLockedResource, dwResourceSize);
					std::istream in(&sbuf);
					std::string job;
					TCHAR buf[64];
					int r, g, b;
					while (in >> job >> r >> g >> b) {
						std::transform(job.begin(), job.end(), job.begin(), ::toupper);
						MultiByteToWideChar(CP_UTF8, 0, job.c_str(), -1, buf, sizeof(buf) / sizeof(TCHAR));
						mClassColors[buf] = D3DCOLOR_XRGB(r, g, b);
					}
				}
			}
		}
	}

	wDPS.addChild(new OverlayRenderer::Control(L"DPS Table Title Placeholder", CONTROL_TEXT_STRING, DT_LEFT));

	OverlayRenderer::Control *mTable = new OverlayRenderer::Control();
	mTable->layoutDirection = LAYOUT_DIRECTION_VERTICAL_TABLE;
	wDPS.addChild(mTable);
	mTable = new OverlayRenderer::Control();
	mTable->layoutDirection = LAYOUT_DIRECTION_VERTICAL_TABLE_SAMESIZE;
	mTable->visible = 0;
	wDPS.addChild(mTable);

	wDPS.layoutDirection = LAYOUT_DIRECTION_VERTICAL;
	wDPS.relativePosition = 1;
	wDOT.layoutDirection = LAYOUT_DIRECTION_VERTICAL_TABLE;
	wDOT.relativePosition = 1;
	wChat.relativePosition = 1;

	wDPS.text = L"DPS";
	wDOT.text = L"DOT";
	wChat.text = L"Chat";

	ReloadLocalization();
}

void GameDataProcess::ReloadLocalization() {
	wDPS.getChild(1)->removeAndDeleteAllChildren();
	wDOT.removeAndDeleteAllChildren();

	OverlayRenderer::Control *mTableHeaderDef = new OverlayRenderer::Control();
	mTableHeaderDef->layoutDirection = LAYOUT_DIRECTION_HORIZONTAL;
	if (wDPS.DpsColumns.jicon) mTableHeaderDef->addChild(new OverlayRenderer::Control(L"", CONTROL_TEXT_RESNAME, DT_CENTER));
	if (wDPS.DpsColumns.uidx) mTableHeaderDef->addChild(new OverlayRenderer::Control(L"#", CONTROL_TEXT_STRING, DT_CENTER));
	if (wDPS.DpsColumns.uname) mTableHeaderDef->addChild(new OverlayRenderer::Control(Languages::get(L"DPSTABLE_NAME"), CONTROL_TEXT_STRING, DT_LEFT));
	if (wDPS.DpsColumns.dps) mTableHeaderDef->addChild(new OverlayRenderer::Control(Languages::get(L"DPSTABLE_DPS"), CONTROL_TEXT_STRING, DT_CENTER));
	if (wDPS.DpsColumns.total) mTableHeaderDef->addChild(new OverlayRenderer::Control(Languages::get(L"DPSTABLE_TOTAL"), CONTROL_TEXT_STRING, DT_CENTER));
	if (wDPS.DpsColumns.crit) mTableHeaderDef->addChild(new OverlayRenderer::Control(Languages::get(L"DPSTABLE_CRITICAL"), CONTROL_TEXT_STRING, DT_CENTER));
	if (wDPS.DpsColumns.dh && version == 400) mTableHeaderDef->addChild(new OverlayRenderer::Control(Languages::get(L"DPSTABLE_DH"), CONTROL_TEXT_STRING, DT_CENTER));
	if (wDPS.DpsColumns.cdmh) {
		if (version == 340)
			mTableHeaderDef->addChild(new OverlayRenderer::Control(Languages::get(L"DPSTABLE_CMH"), CONTROL_TEXT_STRING, DT_CENTER));
		else
			mTableHeaderDef->addChild(new OverlayRenderer::Control(Languages::get(L"DPSTABLE_CDMH"), CONTROL_TEXT_STRING, DT_CENTER)); \
	}
	if (wDPS.DpsColumns.max) mTableHeaderDef->addChild(new OverlayRenderer::Control(Languages::get(L"DPSTABLE_MAX"), CONTROL_TEXT_STRING, DT_CENTER));
	if (wDPS.DpsColumns.death) mTableHeaderDef->addChild(new OverlayRenderer::Control(Languages::get(L"DPSTABLE_DEATH"), CONTROL_TEXT_STRING, DT_CENTER));
	wDPS.getChild(1)->addChild(mTableHeaderDef);
	wDPS.getChild(1)->setPaddingRecursive(wDPS.padding);
	mTableHeaderDef = new OverlayRenderer::Control();
	mTableHeaderDef->layoutDirection = LAYOUT_DIRECTION_HORIZONTAL;
	mTableHeaderDef->addChild(new OverlayRenderer::Control(L"", CONTROL_TEXT_RESNAME, DT_CENTER));
	// mTableHeaderDef->addChild(new OverlayRenderer::Control(L"Source", CONTROL_TEXT_STRING, DT_LEFT));
	mTableHeaderDef->addChild(new OverlayRenderer::Control(Languages::get(L"DOTTABLE_NAME"), CONTROL_TEXT_STRING, DT_LEFT));
	mTableHeaderDef->addChild(new OverlayRenderer::Control(Languages::get(L"DOTTABLE_SKILL"), CONTROL_TEXT_STRING, DT_CENTER));
	mTableHeaderDef->addChild(new OverlayRenderer::Control(Languages::get(L"DOTTABLE_TIME"), CONTROL_TEXT_STRING, DT_RIGHT));
	wDOT.addChild(mTableHeaderDef);
	wDPS.setPaddingRecursive(wDPS.padding);
}

GameDataProcess::~GameDataProcess() {
	WaitForSingleObject(hRecvUpdateInfoThread, -1);
	WaitForSingleObject(hSendUpdateInfoThread, -1);
	CloseHandle(hRecvUpdateInfoThread);
	CloseHandle(hRecvUpdateInfoThreadLock);
	CloseHandle(hSendUpdateInfoThread);
	CloseHandle(hSendUpdateInfoThreadLock);
}

int GameDataProcess::GetVersion() {
	return version;
}

int GameDataProcess::getDoTPotency(int dot) {
	if (version == 340) {
		switch (dot) {
			case 0xf8: return 30;
			case 0xf4: return 20;
			case 0x77: return 30;
			case 0x76: return 35;
			case 0x6a: return 25;
			case 0xf6: return 50;
			case 0x7c: return 40;
			case 0x81: return 50;
			case 0x8f: return 25;
			case 0x90: return 50;
			case 0xa1: return 40;
			case 0xa2: return 40;
			case 0xa3: return 40;
			case 0xb3: return 40;
			case 0xb4: return 35;
			case 0xbd: return 35;
			case 0xbc: return 10;
			case 0x13a: return 20;
			case 0x12: return 50;
			case 0xec: return 20;
			case 0x1ec: return 30;
			case 0x1fc: return 40;
			case 0x356: return 44;
			case 0x2e5: return 40;
			case 0x346: return 40;
			case 0x34b: return 45;
			case 0x2d5: return 50;
			case 0x31e: return 40;
		}
	} else if (version == 400) {
		switch (dot) {
			case 0xf8: return 30;
			case 0xf6: return 50;
			case 0x76: return 35;
			case 0x7c: return 40;
			case 0x81: return 50;
			case 0x8f: return 30;
			case 0x90: return 50;
			case 0xa1: return 40;
			case 0xa2: return 30;
			case 0x2d5: return 60;
			case 0x4b0: return 45;
			case 0x4b1: return 55;
			case 0x31e: return 40;
			case 0xa3: return 40;
			case 0x4ba: return 30;
			case 0xb3: return 40;
			case 0xb4: return 35;
			case 0xbd: return 35;
			case 0x4be: return 40;
			case 0x4bf: return 40;
			case 0x1fc: return 40;
			case 0x346: return 40;
			case 0x34b: return 50;
			case 0xec: return 20;
			case 0x13a: return 20;
		}
	}
	return 0;
}

int GameDataProcess::getDoTDuration(int skill) {
	if (version == 340) {
		switch (skill) {
			case 0xf8: return 15000;
			case 0xf4: return 30000;
			case 0x77: return 24000;
			case 0x76: return 30000;
			case 0x6a: return 30000;
			case 0xf6: return 21000;
			case 0x7c: return 18000;
			case 0x81: return 18000;
			case 0x8f: return 18000;
			case 0x90: return 12000;
			case 0xa1: return 18000;
			case 0xa2: return 21000;
			case 0xa3: return 24000;
			case 0xb3: return 18000;
			case 0xb4: return 24000;
			case 0xbd: return 30000;
			case 0xbc: return 15000;
			case 0x13a: return 15000;
			case 0x12: return 15000;
			case 0xec: return 18000;
			case 0x1ec: return 30000;
			case 0x356: return 30000;
			case 0x2e5: return 30000;
			case 0x346: return 18000;
			case 0x34b: return 30000;
			case 0x2d5: return 24000;
			case 0x31e: return 24000;
		}
	} else if (version == 400) {
		switch (skill) {
			case 0xf8: return 15000;
			case 0xf6: return 18000;
			case 0x76: return 30000;
			case 0x7c: return 18000;
			case 0x81: return 18000;
			case 0x8f: return 18000;
			case 0x90: return 18000;
			case 0xa1: return 18000;
			case 0xa2: return 21000;
			case 0x4b0: return 30000;
			case 0x4b1: return 30000;
			case 0x31e: return 24000;
			case 0xa3: return 24000;
			case 0x4ba: return 18000;
			case 0xb3: return 18000;
			case 0xb4: return 24000;
			case 0xbd: return 30000;
			case 0x4be: return 30000;
			case 0x4bf: return 30000;
			case 0x346: return 18000;
			case 0x34b: return 30000;
			case 0xec: return 18000;
			case 0x13a: return 15000;
		}
	}
	return 0;
}


inline int GameDataProcess::GetActorType(int id) {
	std::lock_guard<std::recursive_mutex> guard(mActorMutex);
	if (mActorPointers.find(id) == mActorPointers.end())
		return -1;
	char *c = (char*) (mActorPointers[id]);
	c += dll->memory()->Struct.Actor.type;
	return *c;
}

inline int GameDataProcess::GetTargetId(int type) {
	__try {
		char *c = ((char*) dll->memory()->Result.Data.TargetMap) + type;
		if (c == 0)
			return NULL_ACTOR;
		char* ptr = (char*)*(PVOID*) c;
		if (ptr == 0)
			return NULL_ACTOR;
		return *(int*) (ptr + dll->memory()->Struct.Actor.id);
	} __except (1) {
		return 0;
	}
}

inline std::string GameDataProcess::GetActorName(int id) {
	std::lock_guard<std::recursive_mutex> guard(mActorMutex);
	if (id == SOURCE_LIMIT_BREAK)
		return "(Limit Break)";
	if (id == NULL_ACTOR)
		return "(null)";
	if (mActorPointers.find(id) == mActorPointers.end()) {
		char t[256];
		sprintf(t, "%08X", id);
		return t;
	}
	char *c = (char*) (mActorPointers[id]);
	c += dll->memory()->Struct.Actor.name;
	if (!Tools::TestValidString(c))
		return "(error)";
	if (strlen(c) == 0)
		return "(empty)";
	return c;
}

inline TCHAR* GameDataProcess::GetActorJobString(int id) {
	std::lock_guard<std::recursive_mutex> guard(mActorMutex);
	if (id >= 35 || id < 0) {

		if (id == SOURCE_LIMIT_BREAK)
			return L"LB";
		if (mActorPointers.find(id) == mActorPointers.end())
			return L"(?)";
		char *c = (char*) (mActorPointers[id]);
		c += dll->memory()->Struct.Actor.job;
		if (!Tools::TestValidString(c))
			return L"(??)";
		id = *c;
	}
	switch (id) {
		case 1: return L"GLD";
		case 2: return L"PGL";
		case 3: return L"MRD";
		case 4: return L"LNC";
		case 5: return L"ARC";
		case 6: return L"CNJ";
		case 7: return L"THM";
		case 8: return L"CPT";
		case 9: return L"BSM";
		case 10: return L"ARM";
		case 11: return L"GSM";
		case 12: return L"LTW";
		case 13: return L"WVR";
		case 14: return L"ALC";
		case 15: return L"CUL";
		case 16: return L"MIN";
		case 17: return L"BTN";
		case 18: return L"FSH";
		case 19: return L"PLD";
		case 20: return L"MNK";
		case 21: return L"WAR";
		case 22: return L"DRG";
		case 23: return L"BRD";
		case 24: return L"WHM";
		case 25: return L"BLM";
		case 26: return L"ACN";
		case 27: return L"SMN";
		case 28: return L"SCH";
		case 29: return L"ROG";
		case 30: return L"NIN";
		case 31: return L"MCH";
		case 32: return L"DRK";
		case 33: return L"AST";
		case 34: return L"SAM";
		case 35: return L"RDM";
	}
	return L"(???)";
}

bool GameDataProcess::IsParseTarget(uint32_t id) {

	if (id == SOURCE_LIMIT_BREAK && dll->config().ParseFilter != 3) return true;
	if (id == 0 || id == NULL_ACTOR || GetActorType(id) != ACTOR_TYPE_PC) return false;
	if (!dll->hooks() || !dll->hooks()->GetOverlayRenderer() || dll->config().ParseFilter == 0) return true;
	if (version == 340) {
		struct PARTY_STRUCT {
			struct {
				union {
					struct {
						uint32_t _u0[4];
						uint32_t id;
						uint32_t _u1[3];
						char* name;
					};
#ifdef _WIN64
					char _u[544];
#else
					char _u[528];
#endif;
				};
			} members[24];
		};
		__try {
			PARTY_STRUCT* ptr = (PARTY_STRUCT*) ((size_t) dll->memory()->Result.Data.PartyMap);
			if (ptr == 0) return true;
			if (id == mSelfId) return true;
			if (dll->config().ParseFilter == 3) return false; // me only
			for (int i = 0; i < 8; i++)
				if (ptr->members[i].id == id)
					return true;
			if (dll->config().ParseFilter == 2) return false; // party
			for (int i = 8; i < 24; i++)
				if (ptr->members[i].id == id)
					return true;
			return false; // alliance
		} __except (EXCEPTION_EXECUTE_HANDLER) {
		}
	} else {
		struct PARTY_STRUCT {
			struct {
				union {
					struct {
						uint64_t _u0[4];
						uint32_t id;
						uint32_t _u1[3];
						char* name;
					};
					char _u[544];
				} members[24];
			};
		};
		__try {
			PARTY_STRUCT* ptr = (PARTY_STRUCT*) ((size_t) dll->memory()->Result.Data.PartyMap);
			if (ptr == 0) return true;
			if (id == mSelfId) return true;
			if (dll->config().ParseFilter == 3) return false; // me only
			for (int i = 0; i < 8; i++)
				if (ptr->members[i].id == id)
					return true;
			if (dll->config().ParseFilter == 2) return false; // party
			for (int i = 8; i < 24; i++)
				if (ptr->members[i].id == id)
					return true;
			return false; // alliance
		} __except (EXCEPTION_EXECUTE_HANDLER) {
		}
	}

	return true; // show anyway if there was any error
}

void GameDataProcess::ResolveUsersInternal() {
	int limit = 1372;
	mSelfId = 0;
	mDamageRedir.clear();
	mActorPointers.clear();
	for (int i = 0; i < limit; i++) {
		__try {
			char* ptr = ((char**) dll->memory()->Result.Data.ActorMap)[i];
#ifdef _WIN64
			if (~(size_t) ptr == 0 || (sizeof(char*) == 8 && (long long) ptr < 0x30000))
				continue;
#else
			if (~(size_t) ptr == 0 || (sizeof(char*) == 4 && (int) ptr < 0x30000))
				continue;
#endif
			if (!Tools::TestValidPtr(ptr, 4))
				continue;
			int id = *(int*) (ptr + dll->memory()->Struct.Actor.id);
			int owner = *(int*) (ptr + dll->memory()->Struct.Actor.owner);
			int type = *(char*) (ptr + dll->memory()->Struct.Actor.type);
			if (mSelfId == 0)
				mSelfId = id;
			if (owner != NULL_ACTOR)
				mDamageRedir[id] = owner;
			else
				mDamageRedir[id] = id;
			mActorPointers[id] = ptr;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
		}
	}
}
void GameDataProcess::ResolveUsers() {

	std::lock_guard<std::recursive_mutex> guard(mActorMutex);

	ResolveUsersInternal();
}

void GameDataProcess::ResetMeter() {
	mLastAttack.timestamp = 0;
}

void GameDataProcess::CalculateDps(uint64_t timestamp) {
	timestamp = mLastAttack.timestamp;

	if (mLastIdleTime != timestamp) {
		mCalculatedDamages.clear();
		for (auto it = mDpsInfo.begin(); it != mDpsInfo.end(); ++it) {
			mCalculatedDamages.push_back(std::pair<int, int>(it->first, it->second.totalDamage.def + it->second.totalDamage.ind));
		}
		std::sort(mCalculatedDamages.begin(), mCalculatedDamages.end(), [](const std::pair<int, int> &a, const std::pair<int, int> &b) -> bool {
			return a.second > b.second;
		});
	}
}

void GameDataProcess::AddDamageInfo(TEMPDMG dmg, bool direct) {
	if (IsParseTarget(dmg.source)) {

		if (mLastAttack.timestamp < dmg.timestamp - mCombatResetTime) {
			mDpsInfo.clear();
			mActorInfo.clear ();
			mLastIdleTime = dmg.timestamp - 1000;
		}
		mLastAttack = dmg;

		mActorInfo[dmg.source].name = GetActorName (dmg.source);
		mActorInfo[dmg.source].job = GetActorJobString (dmg.source);

		if (direct) {
			mDpsInfo[dmg.source].totalDamage.def += dmg.amount;
			if (mDpsInfo[dmg.source].maxDamage.amount < dmg.amount)
				mDpsInfo[dmg.source].maxDamage = dmg;
			mDpsInfo[dmg.source].totalDamage.crit += dmg.amount;
			if (dmg.isCrit)
				mDpsInfo[dmg.source].critHits++;
			if (dmg.isDirectHit)
				mDpsInfo[dmg.source].directHits++;
			if (dmg.amount == 0)
				mDpsInfo[dmg.source].missHits++;
			mDpsInfo[dmg.source].totalHits++;
		} else {
			mDpsInfo[dmg.source].totalDamage.ind += dmg.amount;
			mDpsInfo[dmg.source].dotHits++;
			dmg.isCrit = 0;
		}
	}
}

void GameDataProcess::UpdateOverlayMessage() {
	uint64_t timestamp = mServerTimestamp - mLocalTimestamp + Tools::GetLocalTimestamp();
	TCHAR res[32768];
	TCHAR tmp[512];
	int pos = 0;
	if (dll->hooks()->GetOverlayRenderer() == nullptr)
		return;

	{
		CalculateDps(timestamp);

		std::lock_guard<std::recursive_mutex> guard(dll->hooks()->GetOverlayRenderer()->GetRoot()->getLock());
		{
			if (mShowTimeInfo) {
				size_t sums[4] = { 0 };
				std::lock_guard<std::recursive_mutex> guard2(dll->process()->mSocketMapLock);
				for (auto i = dll->process()->mRecv.begin(); i != dll->process()->mRecv.end(); ++i) sums[0] += i->second->getUsed();
				for (auto i = dll->process()->mToRecv.begin(); i != dll->process()->mToRecv.end(); ++i) sums[1] += i->second->getUsed();
				for (auto i = dll->process()->mSent.begin(); i != dll->process()->mSent.end(); ++i) sums[2] += i->second->getUsed();
				for (auto i = dll->process()->mToSend.begin(); i != dll->process()->mToSend.end(); ++i) sums[3] += i->second->getUsed();
				SYSTEMTIME s1, s2;
				Tools::MillisToLocalTime(timestamp, &s1);
				Tools::MillisToSystemTime((uint64_t) (timestamp*EORZEA_CONSTANT), &s2);
				pos = swprintf(res, sizeof (tmp) / sizeof (tmp[0]), L"LT %02d:%02d:%02d ET %02d:%02d:%02d %lld %zd>>%zd %zd<<%zd\n",
					(int) s1.wHour, (int) s1.wMinute, (int) s1.wSecond,
					(int) s2.wHour, (int) s2.wMinute, (int) s2.wSecond,
					(uint64_t) dll->hooks()->GetOverlayRenderer()->GetFPS(),
					sums[0], sums[1], sums[3], sums[2]);
			}
			if (!mDpsInfo.empty()) {
				uint64_t dur = mLastAttack.timestamp - mLastIdleTime;
				int total = 0;
				for (auto it = mCalculatedDamages.begin(); it != mCalculatedDamages.end(); ++it)
					total += it->second;
				pos += swprintf(res + pos, sizeof (tmp) / sizeof (tmp[0]) - pos, L"%02d:%02d.%03d%s / %.2f (%d)",
					(int) ((dur / 60000) % 60), (int) ((dur / 1000) % 60), (int) (dur % 1000),
					!IsInCombat() ? L"" : (
						GetTickCount() % 600 < 200 ? L".  " :
						GetTickCount() % 600 < 400 ? L".. " : L"..."
						),
					total * 1000. / (mLastAttack.timestamp - mLastIdleTime), (int) mCalculatedDamages.size());
			} else {
				pos += swprintf(res + pos, sizeof (tmp) / sizeof (tmp[0]) - pos, L"00:00:000 / 0 (0)");
			}
			wDPS.getChild(0, CHILD_TYPE_NORMAL)->text = res;
			OverlayRenderer::Control &wTable = *wDPS.getChild(1, CHILD_TYPE_NORMAL);
			OverlayRenderer::Control &wTable24 = *wDPS.getChild(2, CHILD_TYPE_NORMAL);
			while (wTable.getChildCount() > 1) delete wTable.removeChild(1);
			wTable24.removeAndDeleteAllChildren();

			if (!mDpsInfo.empty() && mLastAttack.timestamp > mLastIdleTime) {
				uint32_t i = 0;
				uint32_t disp = 0;
				bool dispme = false;
				size_t mypos = 0;
				if (mCalculatedDamages.size() > 8) {
					for (auto it = mCalculatedDamages.begin(); it != mCalculatedDamages.end(); ++it, mypos++)
						if (it->first == mSelfId)
							break;
					mypos = max(4, mypos);
					mypos = min(mCalculatedDamages.size() - 4, mypos);
				}
				float maxDps = (float) (mCalculatedDamages.begin()->second * 1000. / (mLastAttack.timestamp - mLastIdleTime));
				int displayed = 0;

				for (auto it = mCalculatedDamages.begin(); it != mCalculatedDamages.end(); ++it) {
					i++;
					if (mCalculatedDamages.size() > wDPS.simpleViewThreshold && it->first != mSelfId)
						continue;
					else if (!dll->hooks()->GetOverlayRenderer()->GetUseDrawOverlayEveryone() && it->first != mSelfId)
						continue;
					OverlayRenderer::Control &wRow = *(new OverlayRenderer::Control());
					TEMPDMG &max = mDpsInfo[it->first].maxDamage;
					float curDps = (float) (it->second * 1000. / (mLastAttack.timestamp - mLastIdleTime));

					wRow.layoutDirection = CONTROL_LAYOUT_DIRECTION::LAYOUT_DIRECTION_HORIZONTAL;
					if (wDPS.DpsColumns.jicon)
						wRow.addChild(new OverlayRenderer::Control(mActorInfo[it->first].job, CONTROL_TEXT_RESNAME, DT_CENTER));
					if (maxDps > 0) {
						wRow.addChild(new OverlayRenderer::Control(curDps / maxDps, 1, LIGHTER_COLOR((mClassColors[mActorInfo[it->first].job] & 0xFFFFFF) | 0x70000000, 0x10), CHILD_TYPE_BACKGROUND);
						wRow.addChild(new OverlayRenderer::Control((mDpsInfo[it->first].totalDamage.def * 1000.f / (mLastAttack.timestamp - mLastIdleTime)) / maxDps, 1, LIGHTER_COLOR((mClassColors[mActorInfo[it->first].job] & 0xFFFFFF) | 0x70000000, 0x20), CHILD_TYPE_BACKGROUND);
						wRow.addChild(new OverlayRenderer::Control((mDpsInfo[it->first].totalDamage.crit * 1000.f / (mLastAttack.timestamp - mLastIdleTime)) / maxDps, 1, LIGHTER_COLOR((mClassColors[mActorInfo[it->first].job] & 0xFFFFFF) | 0x70000000, 0x30), CHILD_TYPE_BACKGROUND);
					}
					if (wDPS.DpsColumns.uidx) {
						swprintf(tmp, sizeof (tmp) / sizeof (tmp[0]), L"%d", i);
						wRow.addChild(new OverlayRenderer::Control(tmp, CONTROL_TEXT_STRING, DT_CENTER));
					}
					if (wDPS.DpsColumns.uname) {
						if (dll->config().hideOtherUserName && it->first != mSelfId)
							wcscpy(tmp, L"...");
						else if (dll->config().SelfNameAsYOU && it->first == mSelfId)
							wcscpy(tmp, L"YOU");
						else
							MultiByteToWideChar(CP_UTF8, 0, mActorInfo[it->first].name.c_str(), -1, tmp, sizeof(tmp) / sizeof(TCHAR));
						auto uname = new OverlayRenderer::Control(tmp, CONTROL_TEXT_STRING, DT_LEFT);
						uname->maxWidth = wDPS.maxNameWidth;
						wRow.addChild(uname);
					}
					if (wDPS.DpsColumns.dps) {
						swprintf(tmp, sizeof (tmp) / sizeof (tmp[0]), L"%.2f", curDps);
						wRow.addChild(new OverlayRenderer::Control(tmp, CONTROL_TEXT_STRING, DT_CENTER));
					}
					if (wDPS.DpsColumns.total) {
						swprintf(tmp, sizeof (tmp) / sizeof (tmp[0]), L"%d", it->second);
						wRow.addChild(new OverlayRenderer::Control(tmp, CONTROL_TEXT_STRING, DT_CENTER));
					}
					if (wDPS.DpsColumns.crit) {
						swprintf(tmp, sizeof (tmp) / sizeof (tmp[0]), L"%.2f%%", mDpsInfo[it->first].totalHits == 0 ? 0.f : (100.f * mDpsInfo[it->first].critHits / mDpsInfo[it->first].totalHits));
						wRow.addChild(new OverlayRenderer::Control(tmp, CONTROL_TEXT_STRING, DT_CENTER));
					}
					if (wDPS.DpsColumns.dh && version == 400) {
						swprintf(tmp, sizeof (tmp) / sizeof (tmp[0]), L"%.2f%%", mDpsInfo[it->first].totalHits == 0 ? 0.f : (100.f * mDpsInfo[it->first].directHits / mDpsInfo[it->first].totalHits));
						wRow.addChild(new OverlayRenderer::Control(tmp, CONTROL_TEXT_STRING, DT_CENTER));
					}
					if (wDPS.DpsColumns.cdmh) {
						if (version == 340)
							swprintf(tmp, sizeof (tmp) / sizeof (tmp[0]), L"%d/%d/%d", mDpsInfo[it->first].critHits, mDpsInfo[it->first].missHits, mDpsInfo[it->first].totalHits + mDpsInfo[it->first].dotHits);
						else
							swprintf(tmp, sizeof (tmp) / sizeof (tmp[0]), L"%d/%d/%d/%d", mDpsInfo[it->first].critHits, mDpsInfo[it->first].directHits, mDpsInfo[it->first].missHits, mDpsInfo[it->first].totalHits + mDpsInfo[it->first].dotHits);
						wRow.addChild(new OverlayRenderer::Control(tmp, CONTROL_TEXT_STRING, DT_CENTER));
					}
					if (wDPS.DpsColumns.max) {
						if (version == 400 && (max.isCrit || max.isDirectHit))
							swprintf(tmp, sizeof(tmp) / sizeof(tmp[0]), L"%d%s%s", max.amount, max.isDirectHit ? L"." : L"", max.isCrit ? L"!" : L"");
						else
							swprintf(tmp, sizeof(tmp) / sizeof(tmp[0]), L"%d%s", max.amount, max.isCrit ? L"!" : L"");
						wRow.addChild(new OverlayRenderer::Control(tmp, CONTROL_TEXT_STRING, DT_CENTER));
					}
					if (wDPS.DpsColumns.death) {
						swprintf(tmp, sizeof (tmp) / sizeof (tmp[0]), L"%d", mDpsInfo[it->first].deaths);
						wRow.addChild(new OverlayRenderer::Control(tmp, CONTROL_TEXT_STRING, DT_CENTER));
					}
					wTable.addChild(&wRow);
				}
				wTable.setPaddingRecursive(wDPS.padding);
				if (wTable24.visible = (mCalculatedDamages.size() > wDPS.simpleViewThreshold)) {

					OverlayRenderer::Control *wRow24 = new OverlayRenderer::Control();
					wRow24->layoutDirection = CONTROL_LAYOUT_DIRECTION::LAYOUT_DIRECTION_HORIZONTAL;
					i = 0;
					for (auto it = mCalculatedDamages.begin(); it != mCalculatedDamages.end(); ++it) {
						i++;
						float curDps = (float) (it->second * 1000. / (mLastAttack.timestamp - mLastIdleTime));
						OverlayRenderer::Control &wCol24 = *(new OverlayRenderer::Control());
						wCol24.layoutDirection = CONTROL_LAYOUT_DIRECTION::LAYOUT_DIRECTION_HORIZONTAL;
						wCol24.addChild(new OverlayRenderer::Control(mActorInfo[it->first].job, CONTROL_TEXT_RESNAME, DT_CENTER));
						if (dll->config().hideOtherUserName && it->first != mSelfId)
							wcscpy(tmp, L"...");
						else if (dll->config().SelfNameAsYOU && it->first == mSelfId)
							wcscpy(tmp, L"YOU");
						else
							MultiByteToWideChar(CP_UTF8, 0, mActorInfo[it->first].name.c_str(), -1, tmp, sizeof(tmp) / sizeof(TCHAR));
						OverlayRenderer::Control *uname = new OverlayRenderer::Control(tmp, CONTROL_TEXT_STRING, DT_LEFT);
						uname->maxWidth = wDPS.maxNameWidth;
						wCol24.addChild(uname);
						swprintf(tmp, sizeof (tmp) / sizeof (tmp[0]), L"%.2f", curDps);
						wCol24.addChild(new OverlayRenderer::Control(tmp, CONTROL_TEXT_STRING, DT_LEFT));
						wCol24.addChild(new OverlayRenderer::Control(1.f, 1.f, 0x90000000), CHILD_TYPE_BACKGROUND);
						wCol24.addChild(new OverlayRenderer::Control(curDps / maxDps, 1, LIGHTER_COLOR((mClassColors[mActorInfo[it->first].job] & 0xFFFFFF) | 0x70000000, 0x10), CHILD_TYPE_BACKGROUND);
						wCol24.addChild(new OverlayRenderer::Control((mDpsInfo[it->first].totalDamage.def * 1000.f / (mLastAttack.timestamp - mLastIdleTime)) / maxDps, 1, LIGHTER_COLOR((mClassColors[mActorInfo[it->first].job] & 0xFFFFFF) | 0x70000000, 0x30), CHILD_TYPE_BACKGROUND);
						wCol24.addChild(new OverlayRenderer::Control((mDpsInfo[it->first].totalDamage.crit * 1000.f / (mLastAttack.timestamp - mLastIdleTime)) / maxDps, 1, LIGHTER_COLOR((mClassColors[mActorInfo[it->first].job] & 0xFFFFFF) | 0x70000000, 0x40), CHILD_TYPE_BACKGROUND);


						wCol24.setPaddingRecursive(wDPS.padding / 2);
						wCol24.padding = wDPS.padding / 2;
						wCol24.margin = wDPS.padding;
						wRow24->addChild(&wCol24);
						if (wRow24->getChildCount() == 3) {
							wTable24.addChild(wRow24);
							wRow24 = new OverlayRenderer::Control();
							wRow24->layoutDirection = CONTROL_LAYOUT_DIRECTION::LAYOUT_DIRECTION_HORIZONTAL;
						}
					}
					wRow24->padding = 0;
					wTable24.padding = 0;
					if (wRow24->getChildCount() == 0)
						delete wRow24;
					else
						wTable24.addChild(wRow24);
				}
			}
		}
		{
			std::vector<TEMPBUFF> buff_sort;
			for (auto it = mActiveDoT.begin(); it != mActiveDoT.end(); ) {
				if (it->expires < timestamp)
					it = mActiveDoT.erase(it);
				else {
					if (it->source == mSelfId && it->target != mSelfId && it->potency > 0) {
						buff_sort.push_back(*it);
					}
					++it;
				}
			}
			while (wDOT.getChildCount() > 1)
				delete wDOT.removeChild(1);
			if (!buff_sort.empty()) {
				int currentTarget = GetTargetId(dll->memory()->Struct.Target.current);
				int focusTarget = GetTargetId(dll->memory()->Struct.Target.focus);
				int hoverTarget = GetTargetId(dll->memory()->Struct.Target.hover);
				std::sort(buff_sort.begin(), buff_sort.end(), [&](const TEMPBUFF & a, const TEMPBUFF & b) {
					if ((a.target == focusTarget) ^ (b.target == focusTarget))
						return (a.target == focusTarget ? 1 : 0) > (b.target == focusTarget ? 1 : 0);
					if ((a.target == currentTarget) ^ (b.target == currentTarget))
						return (a.target == currentTarget ? 1 : 0) > (b.target == currentTarget ? 1 : 0);
					if ((a.target == hoverTarget) ^ (b.target == hoverTarget))
						return (a.target == hoverTarget ? 1 : 0) > (b.target == hoverTarget ? 1 : 0);
					return a.expires < b.expires;
				});
				size_t i = 0;
				for (auto it = buff_sort.begin(); it != buff_sort.end(); ++it, i++) {
					if (i > 9 && it->target != currentTarget && it->target != hoverTarget && it->target != focusTarget)
						break;
					OverlayRenderer::Control &wRow = *(new OverlayRenderer::Control());
					OverlayRenderer::Control *col;
					wRow.layoutDirection = CONTROL_LAYOUT_DIRECTION::LAYOUT_DIRECTION_HORIZONTAL;
					wRow.addChild(col = new OverlayRenderer::Control(currentTarget == it->target ? L"T" :
						hoverTarget == it->target ? L"M" :
						focusTarget == it->target ? L"F" : L"", CONTROL_TEXT_STRING, DT_CENTER));

					// MultiByteToWideChar(CP_UTF8, 0, GetActorName(it->source).c_str(), -1, tmp, sizeof(tmp) / sizeof(TCHAR));
					// wRow.addChild(col = new OverlayRenderer::Control(tmp, CONTROL_TEXT_STRING, DT_CENTER));

					MultiByteToWideChar(CP_UTF8, 0, GetActorName(it->target).c_str(), -1, tmp, sizeof(tmp) / sizeof(TCHAR));
					wRow.addChild(col = new OverlayRenderer::Control(tmp, CONTROL_TEXT_STRING, DT_CENTER));
					wRow.addChild(col = new OverlayRenderer::Control(Languages::getDoTName(it->buffid), CONTROL_TEXT_STRING, DT_CENTER));
					swprintf(tmp, sizeof (tmp) / sizeof (tmp[0]), L"%.1fs%s",
						(it->expires - timestamp) / 1000.f,
						it->simulated ? (
						(getDoTDuration(it->buffid) + (it->contagioned ? 15000 : 0)) < it->expires - timestamp
							? L"(x)" :
							L"(?)") : L"");
					wRow.addChild(col = new OverlayRenderer::Control(tmp, CONTROL_TEXT_STRING, DT_CENTER));
					if (it->expires - timestamp < 8000) {
						int v = 0xB0 - (0xB0 * (int) (it->expires - timestamp) / 8000);
						v = 255 - max(0, min(255, v));
						wRow.getChild(wRow.getChildCount() - 1)->statusMap[0].foreground =
							D3DCOLOR_ARGB(0xff, 0xff, v, v);
					}
					wDOT.addChild(&wRow);
				}
				i = buff_sort.size() - i;
				if (i > 0) {
					OverlayRenderer::Control &wRow = *(new OverlayRenderer::Control());
					wRow.layoutDirection = CONTROL_LAYOUT_DIRECTION::LAYOUT_DIRECTION_HORIZONTAL;
					wRow.addChild(new OverlayRenderer::Control(L"", CONTROL_TEXT_STRING, DT_CENTER));
					wRow.addChild(new OverlayRenderer::Control(L"", CONTROL_TEXT_STRING, DT_CENTER));
					swprintf(tmp, sizeof(tmp) / sizeof(tmp[0]), L"%zd", i);
					wRow.addChild(new OverlayRenderer::Control(tmp, CONTROL_TEXT_STRING, DT_CENTER));
					wRow.addChild(new OverlayRenderer::Control(L"", CONTROL_TEXT_STRING, DT_CENTER));
					wDOT.addChild(&wRow);
				}
			}
		}
		if (!mWindowsAdded) {
			mWindowsAdded = true;
			dll->hooks()->GetOverlayRenderer()->AddWindow(&wDPS);
			dll->hooks()->GetOverlayRenderer()->AddWindow(&wDOT);
			dll->hooks()->GetOverlayRenderer()->AddWindow(&wChat);
		}
		wDOT.setPaddingRecursive(wDOT.padding);
	}
}

void GameDataProcess::ProcessAttackInfo(int source, int target, int skill, ATTACK_INFO *info, uint64_t timestamp) {
	switch (skill) {
		case 0xc8:
		case 0xc9:
		case 0xcb:
		case 0xcc:
		case 0x1eb5:
		case 0x1eb6:
		case 0xc7:
		case 0x1090:
		case 0x1091:
		case 0xca:
		case 0x1092:
		case 0x1093:
		case 0x108f:
		case 0x108e:
		case 0x1095:
		case 0x1094:
		case 0xcd:
		case 0x1096:
		case 0xd0:
		case 0x1097:
		case 0x1098:
			source = SOURCE_LIMIT_BREAK;
			break;
	}
	for (int i = 0; i < 8; i++) {
		if (info->attack[i].swingtype == 0) continue;

		TEMPDMG dmg = { 0 };
		dmg.isDoT = false;
		dmg.timestamp = timestamp;
		dmg.skill = skill;
		if (mDamageRedir.find(source) != mDamageRedir.end())
			source = mDamageRedir[source];
		dmg.source = source;
		switch (info->attack[i].swingtype) {
			case 1:
			case 10:
				if (info->attack[i].damage == 0) {
					dmg.amount = 0;
					AddDamageInfo(dmg, true);
				}
				break;
			case 5:
				dmg.isCrit = true;
			case 3:
				/*
				{
					char t[512];
					sprintf(t, "/e attack %016llx", *(uint64_t*) &info->attack[i]);
					dll->addChat(t);
				}
				//*/
				dmg.amount = info->attack[i].damage;
				if (info->attack[i].mult10)
					dmg.amount *= 10;
				AddDamageInfo(dmg, true);
				break;
			case 17:
			case 18:
			{
				int buffId = info->attack[i].damage;
				dmg.buffId = buffId;
				dmg.isDoT = true;

				if (getDoTPotency(buffId) == 0)
					continue; // not an attack buff

				bool add = true;
				for (auto it = mActiveDoT.begin(); it != mActiveDoT.end(); ) {
					if (it->source == source && it->target == target && it->buffid == buffId) {
						it->applied = timestamp;
						it->expires = timestamp + getDoTDuration(buffId) + mDotApplyDelayEstimation[it->buffid].get();
						it->simulated = 1;
						it->contagioned = 0;
						add = false;
						break;
					} else if (it->expires < timestamp) {
						it = mActiveDoT.erase(it);
					} else
						++it;
				}
				if (add) {
					TEMPBUFF b;
					b.buffid = buffId;
					b.source = source;
					b.target = target;
					b.applied = timestamp;
					b.potency = info->attack[i].buffAmount;
					b.critRate = info->attack[i].critAmount / 1000.f;
					b.potency = (int) (b.potency * (1 + b.critRate));
					b.expires = timestamp + getDoTDuration(buffId) + mDotApplyDelayEstimation[buffId].get();
					b.potency = getDoTPotency(b.buffid);
					b.simulated = 1;
					b.contagioned = 0;
					mActiveDoT.push_back(b);
				}

				break;
			}
			case 41:
			{ // Probably contagion
				if (skill == 795) { // it is contagion
					auto it = mActiveDoT.begin();
					while (it != mActiveDoT.end()) {
						if (it->expires - mContagionApplyDelayEstimation.get() < timestamp) {
							it = mActiveDoT.erase(it);
						} else if (it->source == source && it->target == target) {
							it->applied = timestamp;
							it->expires += 15000;
							it->simulated = 1;
							it->contagioned = 1;
							++it;
						} else
							++it;
					}
				}
				break;
			}
		}
	}
}

void GameDataProcess::ProcessAttackInfo(int source, int target, int skill, ATTACK_INFO_V4 *info, uint64_t timestamp) {
	switch (skill) {
		case 0xc8:
		case 0xc9:
		case 0xcb:
		case 0xcc:
		case 0x1eb5:
		case 0x1eb6:
		case 0xc7:
		case 0x1090:
		case 0x1091:
		case 0xca:
		case 0x1092:
		case 0x1093:
		case 0x108f:
		case 0x108e:
		case 0x1095:
		case 0x1094:
		case 0xcd:
		case 0x1096:
		case 0xd0:
		case 0x1097:
		case 0x1098:
			source = SOURCE_LIMIT_BREAK;
			break;
	}
	for (int i = 0; i < 8; i++) {
		if (info->attack[i].swingtype == 0) continue;

		TEMPDMG dmg = { 0 };
		dmg.isDoT = false;
		dmg.timestamp = timestamp;
		dmg.skill = skill;
		if (mDamageRedir.find(source) != mDamageRedir.end())
			source = mDamageRedir[source];
		dmg.source = source;
		switch (info->attack[i].swingtype) {

			case 1: case 3: case 5: case 6:
				dmg.amount = info->attack[i].damage;
				if (info->attack[i].mult10)
					dmg.amount *= 10;
				dmg.isCrit = info->attack[i].isCrit;
				dmg.isDirectHit = info->attack[i].isDirectHit;
				if (dmg.amount == 0 && info->attack[i].swingtype == 1)
					dmg.isMiss = 1;
				AddDamageInfo(dmg, true);
				/*
				if (dmg.source == mSelfId) {
					char test[512];
					sprintf(test, "/e %s: %d / %2X", debug_skillname(dmg.skill).c_str(), dmg.dmg, (int) info->attack[i]._u5);
					dll->addChat(test);
				}
				//*/
				break;
			case 15:
			case 16:
			case 51:
			{
				int buffId = info->attack[i].damage;

				if (getDoTPotency(buffId) == 0)
					continue; // not an attack buff

				/*{
					char test[512];
					sprintf(test, "/e %s: %d %d %d %d", debug_skillname(skill).c_str()
						, (int) info->attack[i].buffAmount
						, (int) info->attack[i].critAmount
						, (int) info->attack[i]._u1
						, (int) info->attack[i]._u4);
					dll->addChat(test);
				}//*/

				bool add = true;
				for (auto it = mActiveDoT.begin(); it != mActiveDoT.end(); ) {
					if (it->source == source && it->target == target && it->buffid == buffId) {
						it->applied = timestamp;
						it->expires = timestamp + getDoTDuration(buffId) + mDotApplyDelayEstimation[it->buffid].get();
						it->simulated = 1;
						it->contagioned = 0;
						add = false;
						break;
					} else if (it->expires < timestamp) {
						it = mActiveDoT.erase(it);
					} else
						++it;
				}
				if (add) {
					TEMPBUFF b;
					b.buffid = buffId;
					b.source = source;
					b.target = target;
					b.applied = timestamp;
					b.expires = timestamp + getDoTDuration(buffId) + mDotApplyDelayEstimation[buffId].get();
					b.potency = info->attack[i].buffAmount;
					b.critRate = info->attack[i].critAmount / 1000.f;
					b.potency = (int) (b.potency * (1 + b.critRate));
					b.simulated = 1;
					b.contagioned = 0;
					mActiveDoT.push_back(b);
				}

				break;
			}
		}
	}
}
void GameDataProcess::EmulateCancel(SOCKET s, int skid, int newcd, int spent, int seqid, uint64_t when) {
	GAME_MESSAGE nmsg;
	nmsg.length = 0x40;
	nmsg.actor = mSelfId;
	nmsg.actor_copy = mSelfId;
	nmsg._u0 = 0xE5BF0003;
	nmsg.message_cat1 = GAME_MESSAGE::C1_Combat;
	nmsg.message_cat2 = GAME_MESSAGE::C2_AbilityResponse;
	nmsg._u1 = 0x160000;
	nmsg._u2 = 0;
	nmsg.Combat.AbilityResponse.skill = 1;
	nmsg.Combat.AbilityResponse.duration_or_skid = skid;
	nmsg.Combat.AbilityResponse.u1 = 0x2BC;
	nmsg.Combat.AbilityResponse.u2 = 0; // 0x246;
	nmsg.Combat.AbilityResponse.u3 = spent; // time already elapsed
	nmsg.Combat.AbilityResponse.u4 = newcd; // cooldown
	nmsg.Combat.AbilityResponse.seqid = seqid;
	nmsg.Combat.AbilityResponse.u6 = 0;
	mRecvAdd[s].push(std::pair<uint64_t, GAME_MESSAGE>(when, nmsg));
	SetEvent(hRecvUpdateInfoThreadLock);
}
void GameDataProcess::EmulateEnableSkill(SOCKET s, uint64_t when) {
	GAME_MESSAGE nmsg;
	nmsg.length = 0x30;
	nmsg.actor = mSelfId;
	nmsg.actor_copy = mSelfId;
	nmsg._u0 = 0xE5BF0003;
	nmsg.message_cat1 = GAME_MESSAGE::C1_Combat;
	nmsg.message_cat2 = version == 340 ? GAME_MESSAGE::C2_SetSkillEnabledV3 : GAME_MESSAGE::C2_SetSkillEnabledV4;
	nmsg._u1 = 0x160000;
	nmsg._u2 = 0;
	// 02 00 04 00 00 00 00 00 00 00 00 00 00 00 00 00 
	memset(nmsg.data, 0, 0x10);
	nmsg.data[0] = 2;
	nmsg.data[2] = 4;
	mRecvAdd[s].push(std::pair<uint64_t, GAME_MESSAGE>(when, nmsg));
	SetEvent(hRecvUpdateInfoThreadLock);
}

bool GameDataProcess::IsInCombat() {
	return mLastAttack.timestamp > mServerTimestamp - mLocalTimestamp + Tools::GetLocalTimestamp() - mCombatResetTime;
}

void GameDataProcess::ProcessGameMessage(SOCKET s, GAME_MESSAGE *data, uint64_t timestamp, bool setTimestamp) {
	if (setTimestamp) {
		mServerTimestamp = timestamp;
		mLocalTimestamp = Tools::GetLocalTimestamp();
	}

	GAME_MESSAGE *msg = (GAME_MESSAGE*) data;
	switch (msg->message_cat1) {
		case GAME_MESSAGE::C1_Combat:
		{
			if (!setTimestamp) {
				switch (msg->message_cat2) {
					case GAME_MESSAGE::C2_UseAbilityRequestV3:
					case GAME_MESSAGE::C2_UseAbilityRequestV4:
						if ((version == 340 && msg->message_cat2 == GAME_MESSAGE::C2_UseAbilityRequestV3) ||
							(version == 400 && msg->message_cat2 == GAME_MESSAGE::C2_UseAbilityRequestV4)) {
							if (dll->hooks()->GetOverlayRenderer()) {
								auto &conf = dll->hooks()->GetOverlayRenderer()->mConfig;
								if (conf.ForceCancelEverySkill && IsInCombat() && FFXIVResources::IsKnownSkill(msg->Combat.AbilityRequest.skill)) {
									int cd = mCachedSkillCooldown[msg->Combat.AbilityResponse.skill];
									cd = min(max(cd, 0), 48000);
									if (cd == 0) cd = 250;
									EmulateCancel(s, msg->Combat.AbilityRequest.skill, cd, 0, msg->Combat.AbilityRequest.seqid);
									mLastSkillRequest = GetTickCount64();
								}
							}
						}
						break;
				}
			} else {
				switch (msg->message_cat2) {
					case 0xFB:
					case GAME_MESSAGE::C2_ActorInfo:
						break;
					case GAME_MESSAGE::C2_Info1:
						if (msg->Combat.Info1.c1 == 23 && msg->Combat.Info1.c2 == 3) {
							if (msg->Combat.Info1.c5 == 0) {
								int total = 0;
								for (auto it = mActiveDoT.begin(); it != mActiveDoT.end(); ++it) {
									total += it->potency;
								}
								if (total > 0) {
									for (auto it = mActiveDoT.begin(); it != mActiveDoT.end(); ++it) {
										int mine = msg->Combat.Info1.c3 * it->potency / total;
										if (mine > 0) {
											TEMPDMG dmg;
											dmg.timestamp = timestamp;
											dmg.source = it->source;
											dmg.buffId = it->buffid;
											dmg.isDoT = true;
											dmg.amount = mine;
											if (mDamageRedir.find(dmg.source) != mDamageRedir.end())
												dmg.source = mDamageRedir[dmg.source];
											AddDamageInfo(dmg, false);
										}
									}
								}
							} else {
								// sprintf(tss, "/e c5=%d", msg->Combat.Info1.c5); dll->addChat(tss);
								for (auto it = mActiveDoT.begin(); it != mActiveDoT.end(); ++it) {
									// sprintf(tss, "/e %d/%d, %d/%d", it->source, it->target, it->buffid, msg->Combat.Info1.c5); dll->addChat(tss);
									if (it->source == it->target && it->buffid == msg->Combat.Info1.c5) {
										int mine = msg->Combat.Info1.c3;
										if (mine > 0) {
											TEMPDMG dmg;
											dmg.timestamp = timestamp;
											dmg.source = it->source;
											dmg.buffId = it->buffid;
											dmg.isDoT = true;
											dmg.amount = mine;
											if (mDamageRedir.find(dmg.source) != mDamageRedir.end())
												dmg.source = mDamageRedir[dmg.source];
											AddDamageInfo(dmg, false);
										}
									}
								}
							}
						} else if (msg->Combat.Info1.c1 == 21) {
							auto it = mActiveDoT.begin();
							while (it != mActiveDoT.end()) {
								if (it->expires < timestamp)
									it = mActiveDoT.erase(it);
								else
									++it;
							}
						} else if (msg->Combat.Info1.c1 == 6) {
							// death
							int who = msg->Combat.Info1.c5;
							if (IsParseTarget(msg->actor))
								mDpsInfo[msg->actor].deaths++;
							auto it = mActiveDoT.begin();
							while (it != mActiveDoT.end()) {
								if (it->expires < timestamp || it->target == msg->actor)
									it = mActiveDoT.erase(it);
								else
									++it;
							}
						}
						break;
					case GAME_MESSAGE::C2_AddBuff:
						for (int i = 0; i < msg->Combat.AddBuff.buff_count && i < 5; i++) {
							// Track non-attack buffs too for ground AoEs
							// if (getDoTPotency(msg->Combat.AddBuff.buffs[i].buffID) == 0) continue; // not an attack buff
							auto it = mActiveDoT.begin();
							bool add = true;
							while (it != mActiveDoT.end()) {
								if (it->source == msg->Combat.AddBuff.buffs[i].actorID && it->target == msg->actor && it->buffid == msg->Combat.AddBuff.buffs[i].buffID) {
									uint64_t nexpire = timestamp + (int) (msg->Combat.AddBuff.buffs[i].duration * 1000);
									if (it->simulated) {
										if (it->contagioned) {
											mContagionApplyDelayEstimation.add((int) (timestamp - it->applied));
											//sprintf(tss, "cmsg => simulated (Contagion: %d / %d)", nexpire - it->expires, estimatedContagionDelay.get());
										} else {
											mDotApplyDelayEstimation[it->buffid].add((int) (timestamp - it->applied));
											//sprintf(tss, "cmsg => simulated (%d: %d / %d)", it->buffid, nexpire - it->expires, estimatedDelays[it->buffid].get());
										}
										//dll->pipe()->sendInfo(tss);
									}
									it->applied = timestamp;
									it->expires = nexpire;
									it->simulated = 0;
									it->contagioned = 0;
									add = false;
									break;
								} else if (it->expires < timestamp) {
									it = mActiveDoT.erase(it);
								} else
									++it;
							}
							if (add) {
								TEMPBUFF b;
								b.buffid = msg->Combat.AddBuff.buffs[i].buffID;
								b.source = msg->Combat.AddBuff.buffs[i].actorID;
								b.target = msg->actor;
								b.applied = timestamp;
								b.expires = timestamp + (int) (msg->Combat.AddBuff.buffs[i].duration * 1000);
								b.potency = getDoTPotency(b.buffid);
								b.contagioned = 0;
								b.simulated = 0;
								mActiveDoT.push_back(b);
							}
						}
						break;
					case GAME_MESSAGE::C2_AbilityResponse:
					{
						if (dll->hooks()->GetOverlayRenderer()) {
							auto &conf = dll->hooks()->GetOverlayRenderer()->mConfig;
							if (msg->Combat.AbilityResponse.skill > 7 &&
								msg->Combat.AbilityResponse.duration_or_skid >= 10 &&
								msg->Combat.AbilityResponse.duration_or_skid <= 60000 * 8 &&
								msg->actor == mSelfId &&
								FFXIVResources::IsKnownSkill(msg->Combat.AbilityResponse.skill) &&
								IsInCombat()
								) {
								if (conf.ForceCancelEverySkill && mLastSkillRequest) {
									int dur = (GetTickCount64() - mLastSkillRequest) / 10;
									dur = max(0, dur);
									EmulateCancel(s, msg->Combat.AbilityResponse.skill, msg->Combat.AbilityResponse.duration_or_skid, dur, msg->Combat.AbilityResponse.seqid);
									mCachedSkillCooldown[msg->Combat.AbilityResponse.skill] = msg->Combat.AbilityResponse.duration_or_skid;

									uint64_t at = mLastSkillRequest + msg->Combat.AbilityResponse.duration_or_skid * 10 - 600; // enable skill 0.6sec before cast ends so that the next skill can be cast immediately
									EmulateEnableSkill(s, at);

								}
							} else if (msg->Combat.AbilityResponse.skill == 1) {
							}
						}
						break;
					}
					case GAME_MESSAGE::C2_UseAbility:
						if (version == 340) {
							ProcessAttackInfo(msg->actor, msg->Combat.UseAbility.target, msg->Combat.UseAbility.skill, &msg->Combat.UseAbility.attack, timestamp);

							if (msg->actor == mSelfId && dll->hooks()->GetOverlayRenderer() && dll->hooks()->GetOverlayRenderer()->mConfig.ShowDamageDealtTwice && IsInCombat())
								mRecvAdd[s].push(std::pair<uint64_t, GAME_MESSAGE>(GetTickCount64(), *msg));
						}
						break;
					case GAME_MESSAGE::C2_UseAoEAbility:
						if (version == 340) {
							if (GetActorName(msg->actor), msg->Combat.UseAoEAbility.skill == 174) { // Bane
								SimulateBane(timestamp, msg->actor, 16, msg->Combat.UseAoEAbility.targets, msg->Combat.UseAoEAbility.attack);
							} else {
								for (int i = 0; i < 16; i++)
									if (msg->Combat.UseAoEAbility.targets[i].target != 0) {
										ProcessAttackInfo(msg->actor, msg->Combat.UseAoEAbility.targets[i].target, msg->Combat.UseAoEAbility.skill, &msg->Combat.UseAoEAbility.attack[i], timestamp);
									}
							}
							if (msg->actor == mSelfId && dll->hooks()->GetOverlayRenderer() && dll->hooks()->GetOverlayRenderer()->mConfig.ShowDamageDealtTwice && IsInCombat())
								mRecvAdd[s].push(std::pair<uint64_t, GAME_MESSAGE>(GetTickCount64(), *msg));
						}
						break;
					case GAME_MESSAGE::C2_UseAbilityV4T1:
					case GAME_MESSAGE::C2_UseAbilityV4T8:
					case GAME_MESSAGE::C2_UseAbilityV4T16:
					case GAME_MESSAGE::C2_UseAbilityV4T24:
					case GAME_MESSAGE::C2_UseAbilityV4T32:
						if (version == 400) {
							int count = msg->Combat.UseAoEAbilityV4.attackCount;
							TARGET_STRUCT *targets;
							switch (msg->message_cat2) {
								case GAME_MESSAGE::C2_UseAbilityV4T1:
									count = max(count, 1);
									targets = (TARGET_STRUCT*) ((char*) msg + 34 * 4);
									break;
								case GAME_MESSAGE::C2_UseAbilityV4T8:
									count = max(count, 8);
									targets = (TARGET_STRUCT*) ((char*) msg + 146 * 4);
									break;
								case GAME_MESSAGE::C2_UseAbilityV4T16:
									count = max(count, 16);
									targets = (TARGET_STRUCT*) ((char*) msg + 274 * 4);
									break;
								case GAME_MESSAGE::C2_UseAbilityV4T24:
									count = max(count, 24);
									targets = (TARGET_STRUCT*) ((char*) msg + 402 * 4);
									break;
								case GAME_MESSAGE::C2_UseAbilityV4T32:
									count = max(count, 32);
									targets = (TARGET_STRUCT*) ((char*) msg + 530 * 4);
									break;
							}
							for (int i = 0; i < msg->Combat.UseAoEAbilityV4.attackCount; i++) {
								if (targets[i].target != 0 && targets[i].target != NULL_ACTOR) {
									ProcessAttackInfo(msg->actor, targets[i].target, msg->Combat.UseAoEAbilityV4.skill, &msg->Combat.UseAoEAbilityV4.attack[i], timestamp);
								}
							}

							if (msg->actor == mSelfId && dll->hooks()->GetOverlayRenderer() && dll->hooks()->GetOverlayRenderer()->mConfig.ShowDamageDealtTwice && IsInCombat())
								mRecvAdd[s].push(std::pair<uint64_t, GAME_MESSAGE>(GetTickCount64(), *msg));
						}
						break;
					default:
						goto DEFPRT;
				}
			}
			break;
		}
DEFPRT:
		default:
			break;
	}
}

void GameDataProcess::SimulateBane(uint64_t timestamp, uint32_t actor, int maxCount, TARGET_STRUCT* targets, ATTACK_INFO* attacks) {
	int baseActor = NULL_ACTOR;
	for (int i = 0; i < 16; i++)
		if (targets[i].target != 0)
			for (int j = 0; j < 4; j++)
				if (attacks[i].attack[j].swingtype == 11)
					baseActor = targets[i].target;
	if (baseActor != NULL_ACTOR) {
		for (int i = 0; i < 16; i++)
			if (targets[i].target != 0)
				for (int j = 0; j < 4; j++)
					if (attacks[i].attack[j].swingtype == 17) {
						TEMPBUFF buff;
						uint64_t expires = -1;
						for (auto it = mActiveDoT.begin(); it != mActiveDoT.end(); ++it)
							if (it->buffid == attacks[i].attack[j].damage &&
								it->source == actor &&
								it->target == baseActor) {
								expires = it->expires;
								break;
							}
						if (expires != -1) {
							bool addNew = true;
							for (auto it = mActiveDoT.begin(); it != mActiveDoT.end(); ++it)
								if (it->buffid == attacks[i].attack[j].damage &&
									it->source == actor &&
									it->target == targets[i].target) {
									it->applied = timestamp;
									it->expires = expires;
									it->simulated = 1;
									addNew = false;
									break;
								}
							if (addNew) {
								buff.buffid = attacks[i].attack[j].damage;
								buff.applied = timestamp;
								buff.expires = expires;
								buff.source = actor;
								buff.target = targets[i].target;
								buff.potency = getDoTPotency(buff.buffid);
								buff.simulated = 1;

								mActiveDoT.push_back(buff);
							}
						}
					}
	}

}

void GameDataProcess::SimulateBane(uint64_t timestamp, uint32_t actor, int maxCount, TARGET_STRUCT* targets, ATTACK_INFO_V4* attacks) {
	int baseActor = NULL_ACTOR;
	for (int i = 0; i < 16; i++)
		if (targets[i].target != 0)
			for (int j = 0; j < 4; j++)
				if (attacks[i].attack[j].swingtype == 11)
					baseActor = targets[i].target;
	if (baseActor != NULL_ACTOR) {
		for (int i = 0; i < 16; i++)
			if (targets[i].target != 0)
				for (int j = 0; j < 4; j++)
					if (attacks[i].attack[j].swingtype == 17) {
						TEMPBUFF buff;
						uint64_t expires = -1;
						for (auto it = mActiveDoT.begin(); it != mActiveDoT.end(); ++it)
							if (it->buffid == attacks[i].attack[j].damage &&
								it->source == actor &&
								it->target == baseActor) {
								expires = it->expires;
								break;
							}
						if (expires != -1) {
							bool addNew = true;
							for (auto it = mActiveDoT.begin(); it != mActiveDoT.end(); ++it)
								if (it->buffid == attacks[i].attack[j].damage &&
									it->source == actor &&
									it->target == targets[i].target) {
									it->applied = timestamp;
									it->expires = expires;
									it->simulated = 1;
									addNew = false;
									break;
								}
							if (addNew) {
								buff.buffid = attacks[i].attack[j].damage;
								buff.applied = timestamp;
								buff.expires = expires;
								buff.source = actor;
								buff.target = targets[i].target;
								buff.potency = getDoTPotency(buff.buffid);
								buff.simulated = 1;

								mActiveDoT.push_back(buff);
							}
						}
					}
	}

}

int tryInflate(z_stream *inf, int flags) {
	__try {
		return inflate(inf, Z_NO_FLUSH);
	} __except (EXCEPTION_EXECUTE_HANDLER) {}
	return -1;
}

void GameDataProcess::ParsePacket(Tools::ByteQueue &in, Tools::ByteQueue &out, SOCKET s, bool setTimestamp) {
	const size_t maxLength = 0x80000;
	GAME_PACKET packetpeek;

	size_t lengthOffset = offsetof(GAME_PACKET, length) + sizeof(GAME_PACKET::length);
	size_t dataOffset = offsetof(GAME_PACKET, data);

	int lastPacketError = 0;

	while (in.getUsed() >= lengthOffset) {
		in.peek(&packetpeek, lengthOffset);
		if ((packetpeek.signature != 0x41A05252 && (packetpeek.signature_2[0] | packetpeek.signature_2[1])) || packetpeek.length > maxLength || packetpeek.length < dataOffset) {
			char t;
			in.read(&t, 1);
			out.write(&t, 1);
			if (lastPacketError != 1)
				dll->addChat("/e packet error: bad signature");
			lastPacketError = 1;
		} else if (in.getUsed() < packetpeek.length) {
			if (mLastRecv + 200 < GetTickCount64()) { // stall
				char t;
				in.read(&t, 1);
				out.write(&t, 1);
				if (lastPacketError != 2)
					dll->addChat("/e packet error: stall");
				lastPacketError = 2;
			}
		} else {
			mLastRecv = GetTickCount64();
			GAME_PACKET *packet = (GAME_PACKET*) malloc(packetpeek.length);
			in.read(packet, packetpeek.length);
			if (packetpeek.length == dataOffset || packetpeek.signature != 0x41A05252) { // empty
				out.write(packet, packet->length);
			} else {
				size_t dataLength = 0, dataBufferSize = 0;
				uint8_t *data = nullptr;

				if (setTimestamp)
					memcpy(&mInboundPacketTemplate, packet, dataOffset);

				if (packet->is_gzip) {
					z_stream inflater;
					dataBufferSize = maxLength;
					data = (uint8_t*) malloc(dataBufferSize);

					memset(&inflater, 0, sizeof(inflater));
					inflater.next_in = packet->data;
					inflater.avail_in = packet->length - offsetof(GAME_PACKET, data);

					if (inflateInit(&inflater) == Z_OK) {
						int res;
						do {
							unsigned char inflateBuffer[maxLength];
							inflater.avail_out = sizeof(inflateBuffer);
							inflater.next_out = inflateBuffer;
							res = tryInflate(&inflater, Z_NO_FLUSH);
							int len = sizeof(inflateBuffer) - inflater.avail_out;
							while (dataLength + len >= dataBufferSize)
								dataBufferSize *= 2;
							data = (uint8_t*) realloc(data, dataBufferSize);
							memcpy(data + dataLength, inflateBuffer, len);
							dataLength += len;
						} while (res == Z_OK);
						inflateEnd(&inflater);
						if (res != Z_STREAM_END) {
							out.write(packet, packet->length);
							free(data);
							free(packet);
							dll->addChat("/e packet error: inflate");
							continue;
						}
					}
				} else {
					data = packet->data;
					dataLength = packet->length - dataOffset;
				}

				uint8_t *buf2 = data;
				for (int i = 0; i < packet->message_count; i++) {
					GAME_MESSAGE *msg = ((GAME_MESSAGE*) buf2);
					ProcessGameMessage(s, msg, packet->timestamp, setTimestamp);
					mInboundSequenceId = msg->seqid;
					dll->sendPipe(setTimestamp ? "recv" : "send", (char*) buf2, msg->length);
					buf2 += msg->length;
				}
				packet->is_gzip = 0;
				packet->length = dataOffset + dataLength;
				out.write(packet, dataOffset);
				out.write(data, dataLength);

				if (dataBufferSize)
					free(data);
			}
			//*/
			free(packet);
		}
	}

	if (setTimestamp && mInboundPacketTemplate.signature == 0x41A05252) {
		std::vector<std::pair<uint64_t, GAME_MESSAGE>> messages;
		int len = 0;
		std::pair<uint64_t, GAME_MESSAGE> m;
		while (mRecvAdd[s].tryPop(&m)) {
			messages.push_back(m);
		}
		for (auto i = messages.begin(); i != messages.end(); ) {
			if (i->first <= GetTickCount64()) {
				len += i->second.length;
				// dll->addChat("/e pull: @ %llx / %llx", i->first, GetTickCount64());
				++i;
			} else {
				mRecvAdd[s].push(*i);
				i = messages.erase(i);
			}
		}
		if (!messages.empty()) {
			uint64_t timestamp = mServerTimestamp - mLocalTimestamp + Tools::GetLocalTimestamp();
			mInboundPacketTemplate.timestamp = timestamp;
			mInboundPacketTemplate.message_count = static_cast<uint16_t>(messages.size());
			mInboundPacketTemplate.is_gzip = false;
			mInboundPacketTemplate.length = dataOffset + len;
			out.write(&mInboundPacketTemplate, dataOffset);
			for (auto i = messages.begin(); i != messages.end(); ++i) {
				auto &msg = i->second;
				msg.seqid = mInboundSequenceId;
				out.write(&msg, msg.length);
			}
		}
	}
}

void GameDataProcess::TryParsePacket(Tools::ByteQueue &in, Tools::ByteQueue &out, SOCKET s, bool setTimestamp) {
	__try {
		ParsePacket(in, out, s, setTimestamp);
	} __except (1) {
		int len = in.getUsed();
		uint8_t* buf = new uint8_t[len];
		in.read(buf, len);
		out.write(buf, len);
		delete buf;
	}
}

void GameDataProcess::RecvUpdateInfoThread() {
	int mapClearer = 0;
	bool processed = false;
	while (WaitForSingleObject(hUnloadEvent, 0) == WAIT_TIMEOUT) {
		if (!processed)
			WaitForSingleObject(hRecvUpdateInfoThreadLock, 50);
		processed = false;
		ResolveUsers();
		std::set<SOCKET> availSocks;
		{
			std::lock_guard<std::recursive_mutex> guard(mSocketMapLock);
			for (auto i = mRecv.cbegin(); i != mRecv.cend(); ++i)
				if (!i->second->isEmpty())
					availSocks.insert(i->first);
		}
		for (auto ss = availSocks.cbegin(); ss != availSocks.cend(); ++ss) {
			SOCKET s = *ss;
			Tools::ByteQueue *need_process;
			if (!(need_process = mRecv[s])->isEmpty()) {
				Tools::ByteQueue *torecv;
				{
					std::lock_guard<std::recursive_mutex> guard(mSocketMapLock);
					torecv = mToRecv[s] ? mToRecv[s] : (mToRecv[s] = new Tools::ByteQueue());
				}
				TryParsePacket(*need_process, *(torecv), s, true);
				processed = true;
			}
		}
		if (++mapClearer > 100) {
			std::lock_guard<std::recursive_mutex> guard(mSocketMapLock);
			mapClearer = 0;
			for (auto i = mRecv.begin(); i != mRecv.end(); ) {
				if (i->second->isStall()) {
					delete i->second;
					i = mRecv.erase(i);
				} else
					++i;
			}
			for (auto i = mToRecv.begin(); i != mToRecv.end(); ) {
				if (i->second->isStall()) {
					delete i->second;
					i = mToRecv.erase(i);
				} else
					++i;
			}
		}
		UpdateOverlayMessage();
	}
	TerminateThread(GetCurrentThread(), 0);
}


void GameDataProcess::SendUpdateInfoThread() {
	int mapClearer = 0;
	bool processed = false;
	while (WaitForSingleObject(hUnloadEvent, 0) == WAIT_TIMEOUT) {
		if (!processed)
			WaitForSingleObject(hSendUpdateInfoThreadLock, 50);
		processed = false;
		std::set<SOCKET> availSocks;
		{
			std::lock_guard<std::recursive_mutex> guard(mSocketMapLock);
			for (auto i = mSent.cbegin(); i != mSent.cend(); ++i)
				if (!i->second->isEmpty())
					availSocks.insert(i->first);
		}
		for (auto ss = availSocks.cbegin(); ss != availSocks.cend(); ++ss) {
			SOCKET s = *ss;
			Tools::ByteQueue *need_process;
			if (!(need_process = mSent[s])->isEmpty()) {
				Tools::ByteQueue *tosend;
				{
					std::lock_guard<std::recursive_mutex> guard(mSocketMapLock);
					tosend = mToSend[s] ? mToSend[s] : (mToSend[s] = new Tools::ByteQueue());
				}
				processed = true;
				TryParsePacket(*need_process, *(tosend), s, false);

				if (tosend->getUsed() > 0) {
					DWORD result = 0, flags = 0;
					int buf2len = tosend->getUsed();
					char* buf2 = new char[buf2len];
					tosend->peek(buf2, buf2len);
					WSABUF buffer = { static_cast<ULONG>(buf2len), const_cast<CHAR *>(buf2) };
					Tools::DebugPrint(L"WSASend in SendUpdateInfoThread %d: %d\n", (int) s, buf2len);
					if (WSASend(s, &buffer, 1, &result, flags, nullptr, nullptr) == SOCKET_ERROR) {
						if (WSAGetLastError() == WSAEWOULDBLOCK) {
						}
					} else
						tosend->waste(buf2len);
					delete[] buf2;
				}
			}
		}
		if (++mapClearer > 100) {
			std::lock_guard<std::recursive_mutex> guard(mSocketMapLock);
			mapClearer = 0;
			for (auto i = mSent.begin(); i != mSent.end(); ) {
				if (i->second->isStall()) {
					delete i->second;
					i = mSent.erase(i);
				} else
					++i;
			}

			for (auto i = mToSend.begin(); i != mToSend.end(); ) {
				if (i->second->isStall()) {
					delete i->second;
					i = mToSend.erase(i);
				} else
					++i;
			}
		}
	}
	TerminateThread(GetCurrentThread(), 0);
}