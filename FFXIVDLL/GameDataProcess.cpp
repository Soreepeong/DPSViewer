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
#include "ImGuiConfigWindow.h"
#include "OverlayRenderer.h"
#include "Hooks.h"

#define LIGHTER_COLOR(color,how) (((color)&0xFF000000) | (min(255, max(0, (((color)>>16)&0xFF)+(how)))<<16) | (min(255, max(0, (((color)>>8)&0xFF)+(how)))<<8) | (min(255, max(0, (((color)>>0)&0xFF)+(how)))<<0)))

GameDataProcess::GameDataProcess(FFXIVDLL *dll, HANDLE unloadEvent) :
	dll(dll),
	hUnloadEvent(unloadEvent),
	mSent(1048576 * 8),
	mRecv(1048576 * 8),
	mLastIdleTime(0),
	wDPS(*new DPSWindowController()),
	wDOT(*new DOTWindowController()) {
	mLastAttack.dmg = 0;
	mLastAttack.timestamp = 0;

	mLocalTimestamp = mServerTimestamp = Tools::GetLocalTimestamp();

	hUpdateInfoThreadLock = CreateEvent(NULL, false, false, NULL);
	hUpdateInfoThread = CreateThread(NULL, NULL, GameDataProcess::UpdateInfoThreadExternal, this, NULL, NULL);

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
	wDOT.relativePosition = 1;
	wDOT.layoutDirection = LAYOUT_DIRECTION_VERTICAL_TABLE;

	ReloadLocalization();
}

void GameDataProcess::ReloadLocalization() {
	wDPS.getChild(1)->removeAndDeleteAllChildren();
	wDOT.removeAndDeleteAllChildren();

	OverlayRenderer::Control *mTableHeaderDef = new OverlayRenderer::Control();
	mTableHeaderDef->layoutDirection = LAYOUT_DIRECTION_HORIZONTAL;
	mTableHeaderDef->addChild(new OverlayRenderer::Control(L"", CONTROL_TEXT_RESNAME, DT_CENTER));
	mTableHeaderDef->addChild(new OverlayRenderer::Control(L"#", CONTROL_TEXT_STRING, DT_CENTER));
	mTableHeaderDef->addChild(new OverlayRenderer::Control(Languages::get(L"DPSTABLE_NAME"), CONTROL_TEXT_STRING, DT_LEFT));
	mTableHeaderDef->addChild(new OverlayRenderer::Control(Languages::get(L"DPSTABLE_DPS"), CONTROL_TEXT_STRING, DT_CENTER));
	mTableHeaderDef->addChild(new OverlayRenderer::Control(Languages::get(L"DPSTABLE_TOTAL"), CONTROL_TEXT_STRING, DT_CENTER));
	mTableHeaderDef->addChild(new OverlayRenderer::Control(Languages::get(L"DPSTABLE_CRITICAL"), CONTROL_TEXT_STRING, DT_CENTER));
	mTableHeaderDef->addChild(new OverlayRenderer::Control(Languages::get(L"DPSTABLE_CMH"), CONTROL_TEXT_STRING, DT_CENTER));
	mTableHeaderDef->addChild(new OverlayRenderer::Control(Languages::get(L"DPSTABLE_MAX"), CONTROL_TEXT_STRING, DT_CENTER));
	mTableHeaderDef->addChild(new OverlayRenderer::Control(Languages::get(L"DPSTABLE_DEATH"), CONTROL_TEXT_STRING, DT_CENTER));
	wDPS.getChild(1)->addChild(mTableHeaderDef);
	wDPS.getChild(1)->setPaddingRecursive(wDPS.padding);
	mTableHeaderDef = new OverlayRenderer::Control();
	mTableHeaderDef->layoutDirection = LAYOUT_DIRECTION_HORIZONTAL;
	mTableHeaderDef->addChild(new OverlayRenderer::Control(L"", CONTROL_TEXT_RESNAME, DT_CENTER));
	mTableHeaderDef->addChild(new OverlayRenderer::Control(Languages::get(L"DOTTABLE_NAME"), CONTROL_TEXT_STRING, DT_LEFT));
	mTableHeaderDef->addChild(new OverlayRenderer::Control(Languages::get(L"DOTTABLE_SKILL"), CONTROL_TEXT_STRING, DT_CENTER));
	mTableHeaderDef->addChild(new OverlayRenderer::Control(Languages::get(L"DOTTABLE_TIME"), CONTROL_TEXT_STRING, DT_RIGHT));
	wDOT.addChild(mTableHeaderDef);
	wDPS.setPaddingRecursive(wDPS.padding);
}

GameDataProcess::~GameDataProcess() {
	WaitForSingleObject(hUpdateInfoThread, -1);
	CloseHandle(hUpdateInfoThread);
	CloseHandle(hUpdateInfoThreadLock);
}

int GameDataProcess::getDoTPotency(int dot) {
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
	return 0;
}

int GameDataProcess::getDoTDuration(int skill) {
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
		case 0x31e: return 24000;
	}
	return 0;
}


inline int GameDataProcess::GetActorType(int id) {
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
	if (id == SOURCE_LIMIT_BREAK)
		return "(Limit Break)";
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
	}
	return L"(???)";
}

void GameDataProcess::ResolveUsers() {
	int limit = 1372;
	mSelfId = 0;
	mDamageRedir.clear();
	mActorPointers.clear();
	for (int i = 0; i < limit; i++) {
		__try {
			char* ptr = ((char**)dll->memory()->Result.Data.ActorMap)[i];
			if (ptr == 0) continue;
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
	if (GetActorType(dmg.source) == ACTOR_TYPE_PC) {

		if (mLastAttack.timestamp < dmg.timestamp - mCombatResetTime) {
			mDpsInfo.clear();
			mActorInfo.clear ();
			mLastIdleTime = dmg.timestamp - 1000;
		}
		mLastAttack = dmg;

		mActorInfo[dmg.source].name = GetActorName (dmg.source);
		mActorInfo[dmg.source].job = GetActorJobString (dmg.source);

		if (direct) {
			mDpsInfo[dmg.source].totalDamage.def += dmg.dmg;
			if (mDpsInfo[dmg.source].maxDamage.dmg < dmg.dmg)
				mDpsInfo[dmg.source].maxDamage = dmg;
			if (dmg.isCrit) {
				mDpsInfo[dmg.source].critHits++;
				mDpsInfo[dmg.source].totalDamage.crit += dmg.dmg;
			}
			if (dmg.dmg == 0)
				mDpsInfo[dmg.source].missHits++;
			mDpsInfo[dmg.source].totalHits++;
		} else {
			mDpsInfo[dmg.source].totalDamage.ind += dmg.dmg;
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

	/* // Debug info
	mDpsInfo.clear();
	mLastIdleTime = timestamp - 1000;
	for (int i = 19; i <= 24+19-1; i++) {
		char test[256];
		int p = sprintf(test, "usr%i", i);
		for (int j = 0; j < i / 2; j++)
			p += sprintf(test + p, "a");
		mDpsInfo[i].totalDamage.ind = 100;
		mDpsInfo[i].totalDamage.def = 100 * i;
		mDpsInfo[i].totalDamage.crit = min(i * i, mDpsInfo[i].totalDamage.def+ mDpsInfo[i].totalDamage.ind);
		mDpsInfo[i].critHits = i * 2;
		mDpsInfo[i].missHits = i;
		mDpsInfo[i].totalHits = i * 3;
		mDpsInfo[i].maxDamage.dmg = i * 1000;
		mDpsInfo[i].maxDamage.isCrit = i % 2;
		mDpsInfo[i].deaths = i;
		mDpsInfo[i].dotHits = (int) (i*1.5);
		mActorInfo[i].job = GetActorJobString(i);
		mActorInfo[i].name = test;
	}
	//*/
	{
		CalculateDps(timestamp);

		std::lock_guard<std::recursive_mutex> guard(dll->hooks()->GetOverlayRenderer()->GetRoot()->getLock());
		{
			SYSTEMTIME s1, s2;
			Tools::MillisToLocalTime(timestamp, &s1);
			Tools::MillisToSystemTime((uint64_t) (timestamp*EORZEA_CONSTANT), &s2);
			if (mShowTimeInfo)
				pos = swprintf(res, sizeof (tmp) / sizeof (tmp[0]), L"FPS %d / LT %02d:%02d:%02d / ET %02d:%02d:%02d / ",
					dll->hooks()->GetOverlayRenderer()->GetFPS(),
					(int) s1.wHour, (int) s1.wMinute, (int) s1.wSecond,
					(int) s2.wHour, (int) s2.wMinute, (int) s2.wSecond);
			if (!mDpsInfo.empty()) {
				uint64_t dur = mLastAttack.timestamp - mLastIdleTime;
				int total = 0;
				for (auto it = mCalculatedDamages.begin(); it != mCalculatedDamages.end(); ++it)
					total += it->second;
				pos += swprintf(res + pos, sizeof (tmp) / sizeof (tmp[0]) - pos, L"%02d:%02d.%03d / %.2f (%d)",
					(int) ((dur / 60000) % 60), (int) ((dur / 1000) % 60), (int) (dur % 1000),
					total * 1000. / (mLastAttack.timestamp - mLastIdleTime), (int) mCalculatedDamages.size());
			} else {
				pos += swprintf(res + pos, sizeof (tmp) / sizeof (tmp[0]) - pos, L"00:00:000 / 0 (0)");
			}
			wDPS.getChild(0, CHILD_TYPE_NORMAL)->text = res;
			OverlayRenderer::Control &wTable = *wDPS.getChild(1, CHILD_TYPE_NORMAL);
			OverlayRenderer::Control &wTable24 = *wDPS.getChild(2, CHILD_TYPE_NORMAL);
			while (wTable.getChildCount() > 1) delete wTable.removeChild(1);
			wTable24.removeAndDeleteAllChildren();

			if (!mDpsInfo.empty()) {
				uint32_t i = 0;
				uint32_t disp = 0;
				bool dispme = false;
				uint32_t mypos = 0;
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

					wRow.addChild(new OverlayRenderer::Control(mActorInfo[it->first].job, CONTROL_TEXT_RESNAME, DT_CENTER));
					wRow.addChild(new OverlayRenderer::Control(curDps / maxDps, 1, LIGHTER_COLOR((mClassColors[mActorInfo[it->first].job] & 0xFFFFFF) | 0x70000000, 0x10), CHILD_TYPE_BACKGROUND);
					wRow.addChild(new OverlayRenderer::Control((mDpsInfo[it->first].totalDamage.def * 1000. / (mLastAttack.timestamp - mLastIdleTime)) / maxDps, 1, LIGHTER_COLOR((mClassColors[mActorInfo[it->first].job] & 0xFFFFFF) | 0x70000000, 0x20), CHILD_TYPE_BACKGROUND);
					wRow.addChild(new OverlayRenderer::Control((mDpsInfo[it->first].totalDamage.crit * 1000. / (mLastAttack.timestamp - mLastIdleTime)) / maxDps, 1, LIGHTER_COLOR((mClassColors[mActorInfo[it->first].job] & 0xFFFFFF) | 0x70000000, 0x30), CHILD_TYPE_BACKGROUND);

					swprintf(tmp, sizeof (tmp) / sizeof (tmp[0]), L"%d", i);
					wRow.addChild(new OverlayRenderer::Control(tmp, CONTROL_TEXT_STRING, DT_CENTER));

					if (dll->hooks()->GetOverlayRenderer()->GetHideOtherUserName() && it->first != mSelfId)
						wcscpy(tmp, L"...");
					else {
						MultiByteToWideChar(CP_UTF8, 0, mActorInfo[it->first].name.c_str(), -1, tmp, sizeof(tmp) / sizeof(TCHAR));
					}
					auto uname = new OverlayRenderer::Control(tmp, CONTROL_TEXT_STRING, DT_LEFT);
					uname->maxWidth = wDPS.maxNameWidth;
					wRow.addChild(uname);
					swprintf(tmp, sizeof (tmp) / sizeof (tmp[0]), L"%.2f", curDps);
					wRow.addChild(new OverlayRenderer::Control(tmp, CONTROL_TEXT_STRING, DT_CENTER));
					swprintf(tmp, sizeof (tmp) / sizeof (tmp[0]), L"%d", it->second);
					wRow.addChild(new OverlayRenderer::Control(tmp, CONTROL_TEXT_STRING, DT_CENTER));
					swprintf(tmp, sizeof (tmp) / sizeof (tmp[0]), L"%.2f%%", mDpsInfo[it->first].totalHits == 0 ? 0.f : (100.f * mDpsInfo[it->first].critHits / mDpsInfo[it->first].totalHits));
					wRow.addChild(new OverlayRenderer::Control(tmp, CONTROL_TEXT_STRING, DT_CENTER));
					swprintf(tmp, sizeof (tmp) / sizeof (tmp[0]), L"%d/%d/%d", mDpsInfo[it->first].critHits, mDpsInfo[it->first].missHits, mDpsInfo[it->first].totalHits + mDpsInfo[it->first].dotHits);
					wRow.addChild(new OverlayRenderer::Control(tmp, CONTROL_TEXT_STRING, DT_CENTER));
					swprintf(tmp, sizeof(tmp) / sizeof(tmp[0]), L"%d%s", max.dmg, max.isCrit ? L"!" : L"");
					wRow.addChild(new OverlayRenderer::Control(tmp, CONTROL_TEXT_STRING, DT_CENTER));
					swprintf(tmp, sizeof (tmp) / sizeof (tmp[0]), L"%d", mDpsInfo[it->first].deaths);
					wRow.addChild(new OverlayRenderer::Control(tmp, CONTROL_TEXT_STRING, DT_CENTER));

					wTable.addChild(&wRow);
				}
				wTable.setPaddingRecursive(wDPS.padding);
				if (wTable24.visible = mCalculatedDamages.size() > 8) {

					OverlayRenderer::Control *wRow24 = new OverlayRenderer::Control();
					wRow24->layoutDirection = CONTROL_LAYOUT_DIRECTION::LAYOUT_DIRECTION_HORIZONTAL;
					i = 0;
					for (auto it = mCalculatedDamages.begin(); it != mCalculatedDamages.end(); ++it) {
						i++;
						float curDps = (float) (it->second * 1000. / (mLastAttack.timestamp - mLastIdleTime));
						OverlayRenderer::Control &wCol24 = *(new OverlayRenderer::Control());
						wCol24.layoutDirection = CONTROL_LAYOUT_DIRECTION::LAYOUT_DIRECTION_HORIZONTAL;
						wCol24.addChild(new OverlayRenderer::Control(mActorInfo[it->first].job, CONTROL_TEXT_RESNAME, DT_CENTER));
						if (dll->hooks()->GetOverlayRenderer()->GetHideOtherUserName() && it->first != mSelfId)
							wcscpy(tmp, L"...");
						else {
							MultiByteToWideChar(CP_UTF8, 0, mActorInfo[it->first].name.c_str(), -1, tmp, sizeof(tmp) / sizeof(TCHAR));
						}
						OverlayRenderer::Control *uname = new OverlayRenderer::Control(tmp, CONTROL_TEXT_STRING, DT_LEFT);
						uname->maxWidth = wDPS.maxNameWidth;
						wCol24.addChild(uname);
						swprintf(tmp, sizeof (tmp) / sizeof (tmp[0]), L"%.2f", curDps);
						wCol24.addChild(new OverlayRenderer::Control(tmp, CONTROL_TEXT_STRING, DT_LEFT));
						wCol24.addChild(new OverlayRenderer::Control(1.f, 1.f, 0x90000000), CHILD_TYPE_BACKGROUND);
						wCol24.addChild(new OverlayRenderer::Control(curDps / maxDps, 1, LIGHTER_COLOR((mClassColors[mActorInfo[it->first].job] & 0xFFFFFF) | 0x70000000, 0x10), CHILD_TYPE_BACKGROUND);
						wCol24.addChild(new OverlayRenderer::Control((mDpsInfo[it->first].totalDamage.def * 1000. / (mLastAttack.timestamp - mLastIdleTime)) / maxDps, 1, LIGHTER_COLOR((mClassColors[mActorInfo[it->first].job] & 0xFFFFFF) | 0x70000000, 0x30), CHILD_TYPE_BACKGROUND);
						wCol24.addChild(new OverlayRenderer::Control((mDpsInfo[it->first].totalDamage.crit * 1000. / (mLastAttack.timestamp - mLastIdleTime)) / maxDps, 1, LIGHTER_COLOR((mClassColors[mActorInfo[it->first].job] & 0xFFFFFF) | 0x70000000, 0x40), CHILD_TYPE_BACKGROUND);


						wCol24.setPaddingRecursive(wDPS.padding/2);
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
				if (it->expires < timestamp || GetActorType(it->target) == ACTOR_TYPE_PC)
					it = mActiveDoT.erase(it);
				else {
					if (it->source == mSelfId) {
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
				int i = 0;
				for (auto it = buff_sort.begin(); it != buff_sort.end(); ++it, i++) {
					if (i > 9 && it->target != currentTarget && it->target != hoverTarget && it->target != focusTarget)
						break;
					OverlayRenderer::Control &wRow = *(new OverlayRenderer::Control());
					OverlayRenderer::Control *col;
					wRow.layoutDirection = CONTROL_LAYOUT_DIRECTION::LAYOUT_DIRECTION_HORIZONTAL;
					wRow.addChild(col = new OverlayRenderer::Control(currentTarget == it->target ? L"T" :
						hoverTarget == it->target ? L"M" :
						focusTarget == it->target ? L"F" : L"", CONTROL_TEXT_STRING, DT_CENTER));
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
					swprintf(tmp, sizeof(tmp) / sizeof(tmp[0]), L"%d", i);
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
		}
		wDOT.setPaddingRecursive(wDOT.padding);
	}
}

void GameDataProcess::ProcessAttackInfo(int source, int target, int skill, ATTACK_INFO *info, uint64_t timestamp) {
	for (int i = 0; i < 4; i++) {
		if (info->attack[i].swingtype == 0) continue;

		TEMPDMG dmg = { 0 };
		dmg.isDoT = false;
		dmg.timestamp = timestamp;
		dmg.skill = skill;
		if (mDamageRedir.find(source) != mDamageRedir.end())
			source = mDamageRedir[source];
		dmg.source = source;
		switch (skill) {
			case 0xc8:
			case 0xc9:
			case 0xca:
			case 0xcb:
			case 0xcc:
			case 0xcd:
			case 0x1092:
			case 0x1093:
			case 0x108f:
			case 0x108e:
			case 0x1095:
			case 0x1094:
			case 0x1096:
				source = SOURCE_LIMIT_BREAK;
				break;
		}
		switch (info->attack[i].swingtype) {
			case 1:
			case 10:
				if (info->attack[i].damage == 0) {
					dmg.dmg = 0;
					AddDamageInfo(dmg, true);
				}
				break;
			case 5:
				dmg.isCrit = true;
			case 3:
				dmg.dmg = info->attack[i].damage;
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
		/*
		char tss[512];
		sprintf(tss, "/e    => %d/%d/%d/%d/%d/%d",
			(int)info->attack[i].swingtype,
			(int)info->attack[i].damagetype,
			(int)info->attack[i].elementtype,
			(int)info->attack[i].data0_rr,
			(int)info->attack[i].damage,
			(int)info->attack[i].data1_right
			);
		dll->pipe()->AddChat(tss);
		//*/
	}
}

void GameDataProcess::ProcessGameMessage(void *data, uint64_t timestamp, int len, bool setTimestamp) {

	if (setTimestamp) {
		mServerTimestamp = timestamp;
		mLocalTimestamp = Tools::GetLocalTimestamp();
	} else
		timestamp = mServerTimestamp;

	GAME_MESSAGE *msg = (GAME_MESSAGE*) data;
	// std::string ts; char tss[512];
	switch (msg->message_cat1) {
		case GAME_MESSAGE::C1_Combat:
		{
			switch (msg->message_cat2) {
				case 0x0143:
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
										dmg.dmg = mine;
										if (mDamageRedir.find(dmg.source) != mDamageRedir.end())
											dmg.source = mDamageRedir[dmg.source];
										AddDamageInfo(dmg, false);
									}
								}
							}
						} else {
							for (auto it = mActiveDoT.begin(); it != mActiveDoT.end(); ++it) {
								if (it->source == it->target && it->buffid == msg->Combat.Info1.c5) {
									int mine = msg->Combat.Info1.c3;
									if (mine > 0) {
										TEMPDMG dmg;
										dmg.timestamp = timestamp;
										dmg.source = it->source;
										dmg.buffId = it->buffid;
										dmg.isDoT = true;
										dmg.dmg = mine;
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
						if (GetActorType(msg->actor) == ACTOR_TYPE_PC)
							mDpsInfo[msg->actor].deaths++;
						/*
						sprintf(tss, "cmsgDeath %s killed %s", getActorName(who), getActorName(msg->actor));
						dll->pipe()->sendInfo(tss);
						//*/
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
					/*
					sprintf(tss, "cmsgAddBuff %s / %d", getActorName(msg->actor),
						msg->Combat.AddBuff._u1);
					dll->pipe()->sendInfo(tss);
					//*/
					for (int i = 0; i < msg->Combat.AddBuff.buff_count && i < 5; i++) {
						if (getDoTPotency(msg->Combat.AddBuff.buffs[i].buffID) == 0)
							continue; // not an attack buff
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
				case GAME_MESSAGE::C2_UseAbility:
					// sprintf(tss, "/e Ability %s / %d / %d", GetActorName(msg->actor), msg->Combat.UseAbility.target, msg->Combat.UseAbility.skill);
					// dll->pipe()->AddChat(tss);

					ProcessAttackInfo(msg->actor, msg->Combat.UseAbility.target, msg->Combat.UseAbility.skill, &msg->Combat.UseAbility.attack, timestamp);
					break;
				case GAME_MESSAGE::C2_UseAoEAbility:
					// sprintf(tss, "/e AoE %s / %d", GetActorName(msg->actor), msg->Combat.UseAoEAbility.skill);
					// dll->pipe()->AddChat(tss);

					if (GetActorName(msg->actor), msg->Combat.UseAoEAbility.skill == 174) { // Bane
						int baseActor = NULL_ACTOR;
						for (int i = 0; i < 16; i++)
							if (msg->Combat.UseAoEAbility.targets[i].target != 0)
								for (int j = 0; j < 4; j++)
									if (msg->Combat.UseAoEAbility.attackToTarget[i].attack[j].swingtype == 11)
										baseActor = msg->Combat.UseAoEAbility.targets[i].target;
						if (baseActor != NULL_ACTOR) {
							for (int i = 0; i < 16; i++)
								if (msg->Combat.UseAoEAbility.targets[i].target != 0)
									for (int j = 0; j < 4; j++)
										if (msg->Combat.UseAoEAbility.attackToTarget[i].attack[j].swingtype == 17) {
											TEMPBUFF buff;
											uint64_t expires = -1;
											for (auto it = mActiveDoT.begin(); it != mActiveDoT.end(); ++it)
												if (it->buffid == msg->Combat.UseAoEAbility.attackToTarget[i].attack[j].damage &&
													it->source == msg->actor &&
													it->target == baseActor) {
													expires = it->expires;
													break;
												}
											if (expires != -1) {
												bool addNew = true;
												for (auto it = mActiveDoT.begin(); it != mActiveDoT.end(); ++it)
													if (it->buffid == msg->Combat.UseAoEAbility.attackToTarget[i].attack[j].damage &&
														it->source == msg->actor &&
														it->target == msg->Combat.UseAoEAbility.targets[i].target) {
														it->applied = timestamp;
														it->expires = expires;
														it->simulated = 1;
														addNew = false;
														break;
													}
												if (addNew) {
													buff.buffid = msg->Combat.UseAoEAbility.attackToTarget[i].attack[j].damage;
													buff.applied = timestamp;
													buff.expires = expires;
													buff.source = msg->actor;
													buff.target = msg->Combat.UseAoEAbility.targets[i].target;
													buff.potency = getDoTPotency(buff.buffid);
													buff.simulated = 1;

													mActiveDoT.push_back(buff);
												}
											}
										}
						}

					} else {
						for (int i = 0; i < 16; i++)
							if (msg->Combat.UseAoEAbility.targets[i].target != 0) {
								/*
								sprintf(tss, "cmsg  => Target[%i] %d / %s", i, msg->Combat.UseAoEAbility.targets[i].target, getActorName(msg->Combat.UseAoEAbility.targets[i].target));
								dll->pipe()->sendInfo(tss);
								//*/
								ProcessAttackInfo(msg->actor, msg->Combat.UseAoEAbility.targets[i].target, msg->Combat.UseAoEAbility.skill, &msg->Combat.UseAoEAbility.attackToTarget[i], timestamp);
							}
					}
					break;
				default:
					goto DEFPRT;
			}
			break;
		}
DEFPRT:
		default:
			/*
			int pos = sprintf(tss, "cmsg%04X:%04X (%s, %x) ", (int)msg->message_cat1, (int)msg->message_cat2, getActorName(msg->actor), msg->length);
			for (int i = 0; i < msg->length - 32 && i<64; i ++)
				pos += sprintf(tss + pos, "%02X ", (int)((unsigned char*)msg->data)[i]);
			dll->pipe()->sendInfo(tss);
			//*/
			break;
	}
}

void GameDataProcess::PacketErrorMessage(int signature, int length) {
	/*
	char t[1024];
	sprintf(t, "cmsgError: packet error %08X / %08X", signature, length);
	//*/
}
int tryInflate(z_stream *inf, int flags) {
	__try {
		return inflate(inf, Z_NO_FLUSH);
	} __except (EXCEPTION_EXECUTE_HANDLER) {}
	return -1;
}

void GameDataProcess::ParsePacket(Tools::ByteQueue &p, bool setTimestamp) {

	struct {
		union {
			uint32_t signature;		// 0 ~ 3		0x41A05252
			uint64_t signature_2[2];
		};
		uint64_t timestamp;		// 16 ~ 23
		uint32_t length;		// 24 ~ 27
		uint16_t _u2;			// 28 ~ 29
		uint16_t message_count;	// 30 ~ 31
		uint8_t _u3;			// 32
		uint8_t is_gzip;		// 33
		uint16_t _u4;			// 34 ~ 35
		uint32_t _u5;			// 36 ~ 39;
		uint8_t data[65536];
	} packet;

	while (p.getUsed() >= 28) {
		p.peek(&packet, 28);
		if ((packet.signature != 0x41A05252 && (packet.signature_2[0] | packet.signature_2[1])) || packet.length > sizeof(packet) || packet.length <= 40) {
			p.waste(1);
			PacketErrorMessage(packet.signature, packet.length);
			continue;
		}
		if (p.getUsed() < packet.length)
			break;
		p.peek(&packet, packet.length);
		unsigned char *buf = nullptr;
		if (packet.is_gzip) {
			z_stream inflater;
			memset(&inflater, 0, sizeof(inflater));
			inflater.avail_out = DEFLATE_CHUNK_SIZE;
			inflater.next_out = inflateBuffer;
			inflater.next_in = packet.data;
			inflater.avail_in = packet.length - 40;

			if (inflateInit(&inflater) == Z_OK) {
				int res = tryInflate(&inflater, Z_NO_FLUSH);
				if (Z_STREAM_END != res) {
					inflateEnd(&inflater);
					p.waste(1);
					continue;
				}
				inflateEnd(&inflater);
				if (inflater.avail_out == 0) {
					p.waste(1);
					continue;
				}
				buf = inflateBuffer;
			}
		} else {
			buf = packet.data;
		}
		if (packet.signature == 0x41A05252) {
			if (buf != nullptr) {
				for (int i = 0; i < packet.message_count; i++) {
					ProcessGameMessage(buf, packet.timestamp, ((char*) &packet.data - (char*) &packet), setTimestamp);
					buf += *((uint32_t*) buf);
				}
			}
		}
		p.waste(packet.length);
	}
}

void GameDataProcess::UpdateInfoThread() {
	while (WaitForSingleObject(hUnloadEvent, 0) == WAIT_TIMEOUT) {
		WaitForSingleObject(hUpdateInfoThreadLock, 50);
		ResolveUsers();
		ParsePacket(mSent, false);
		ParsePacket(mRecv, true);
		UpdateOverlayMessage();
		/*
		__try {
		} __except (EXCEPTION_EXECUTE_HANDLER) {
		}
		//*/
	}
	TerminateThread(GetCurrentThread(), 0);
}