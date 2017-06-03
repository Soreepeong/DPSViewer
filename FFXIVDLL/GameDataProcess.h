#pragma once
#include<Windows.h>
#include"Tools.h"
#define ZLIB_WINAPI
#include"../madler-zlib/zlib.h"
#include "FFXIVDLL.h"
#include "MedianCalculator.h"

#define DEFLATE_CHUNK_SIZE (1 << 18)

class FFXIVDLL;
class MedianCalculator;
class DPSWindowController;
class DOTWindowController;

struct PER_USER_DMG {
	int def;
	int ind;
};


#define NULL_ACTOR 0xE0000000
#define SOURCE_LIMIT_BREAK (NULL_ACTOR+1)

enum ACTOR_TYPE : uint8_t {
	ACTOR_TYPE_PC = 1
};

enum ATTACK_DAMAGE_TYPE : uint8_t {
	DMG_Unknown = 0,
	DMG_Slashing = 1,
	DMG_Piercing = 2,
	DMG_Blunt = 3,
	DMG_Magic = 5,
	DMG_Darkness = 6,
	DMG_Physical = 7,
	DMG_LimitBreak = 8,
};

enum ATTACK_ELEMENT_TYPE : uint8_t {
	ELEM_Unknown,
	ELEM_Fire,
	ELEM_Ice,
	ELEM_Air,
	ELEM_Earth,
	ELEM_Lightning,
	ELEM_Water,
	ELEM_Unaspected,
};

struct ATTACK_INFO {
	struct {
		char swingtype;
		ATTACK_DAMAGE_TYPE damagetype;
		ATTACK_ELEMENT_TYPE elementtype;
		char data0_rr;
		short damage;
		short data1_right;
	}attack[4]; // 8 x 4 bytes
	int _u4[8];
};

struct TEMPUSERINFO {
	std::string name;
	std::wstring job;
};

struct TEMPBUFF {
	int source;
	int target;
	int buffid;
	int potency;
	uint64_t applied;
	uint64_t expires;
	int simulated : 1;
	int contagioned : 1;
};

struct TEMPDMG {
	uint64_t timestamp;
	int source;
	union {
		int skill;
		int buffId;
	};
	int dmg;
	bool isDoT;
	bool isCrit;
};

#pragma pack(push, 1)
struct GAME_MESSAGE {
	uint32_t length; // 0 ~ 3
	uint32_t actor; // 4 ~ 7
	uint8_t _u1[8];  // 8 ~ 15
	enum MESSAGE_CAT1 : uint16_t {
		C1_Combat = 0x0014
	} message_cat1; // 16 ~ 17
	enum MESSAGE_CAT2 : uint16_t {
		/* Unknown cat1s
		ZoneChange = 0xCF,
		FATE = 0x143,
		MatchStartSingle = 0x6C,
		MatchStartMultiple = 0x74,
		MatchStartDutyRoulette = 0x76,
		MatchStop = 0x2DB,
		MatchConfirmed = 0x6F,
		MatchStatusInfo = 0x2DE,
		MatchFound = 0x339
		*/
		C2_DelBuff = 0x00EC,
		C2_StartCasting = 0x0110,
		C2_AddBuff = 0x0141,
		C2_Info1 = 0x0142,
		C2_TargetMarker = 0x0144,
		C2_ActorInfo = 0x0145,
		C2_UseAbility = 0x0146,
		C2_UseAoEAbility = 0x0147,
		C2_RaidMarker = 0x0335,
	} message_cat2; // 18 ~ 19
	uint8_t _u2[12]; // 20 ~ 31
	union {
		uint8_t data[65536];
		union {
			struct {
				uint16_t c1; // 32
				uint16_t _u1;
				uint32_t c5; // 36
				uint32_t c2; // 40
				uint32_t c3; // 44
				uint32_t c4; // 48

							 /*
							 c1=23: dot(c2=3) hot(c2=4) / damage=c3 / skill=c5
							 c1=15: skill cancelled / skill id=c3 / cancelled(c4=0) interrupted(c4=1)
							 c1=6: death / by=c5
							 c1=34: target icon (?)
							 c1=21: buff remove / buff=c5
							 */
			} Info1;
			struct {
				uint8_t _u1[4]; // 32
				uint32_t target; // 36
				uint8_t _u2[4]; // 40
				uint32_t skill; // 44
			} StartCasting;
			struct {
				uint8_t _u1[12]; // 32
				uint32_t skill; // 44
				uint8_t _u2[16]; // 48
				uint32_t target; // 64
				uint8_t _u3[8]; // 68
				ATTACK_INFO attack;
			} UseAbility;
			struct {
				uint8_t _u1[12]; // 32
				uint32_t skill; // 44
				uint8_t _u2[20]; // 48
				ATTACK_INFO attackToTarget[16]; // 68
				uint32_t _u3; // 1092
				struct {
					uint32_t target;
					uint32_t _u1;
				}targets[16]; // 1096
			} UseAoEAbility;
			struct {
				uint8_t _u1[12]; // 32
				uint32_t HP; // 44
				uint16_t MP; // 48
				uint16_t TP; // 50
				uint32_t MaxHP; // 52
				uint16_t MaxMP; // 56
				uint16_t buff_count; // 58
				struct {
					// size 16
					uint16_t _u1;
					uint16_t buffID;
					uint16_t extra;
					uint16_t _u2;
					float duration;
					uint32_t actorID;
				}buffs[3]; // 60
			} AddBuff;
			struct {
				uint8_t _u1[4]; // 32
				uint32_t HP; // 36
				uint32_t MaxHP; // 40
				uint16_t MaxMP; // 44
				uint16_t MP; // 46
				uint16_t TP; // 48
				struct {
					// size 12
					uint16_t buffID;
					uint16_t extra;
					float duration;
					uint32_t actorID;
				}buffs[160]; // 52
			} DelBuff;
			struct {
				struct {
					uint32_t _u1[2];
					float x;
					float z;
					float y;
				} markers[3];
			} RaidMarker;
			struct {
				uint32_t _u1; // 32
				uint32_t markerType; // 36
				uint32_t actorId; // 40
				uint32_t _u2[3]; // 44
				uint32_t targetId; // 56
			} TargetMarker;
			struct {
				uint16_t HP;
				uint16_t _u1;
				uint16_t MP;
				uint16_t TP;
				uint16_t GP;
			} ActorInfo;
		} Combat;
	};
};
#pragma pack(pop)

#define EORZEA_CONSTANT 20.571428571428573

class GameDataProcess {
	friend class Hooks;
	friend class ImGuiConfigWindow;

private:
	struct {
		int id;
		int name;
		int owner;
		int type;
		int job;
	}pActor;
	struct {
		int current;
		int hover;
		int focus;
	}pTarget;

	static int getDoTPotency(int dot);
	int getDoTDuration(int skill);

	std::map<std::wstring, D3DCOLOR> mClassColors;

	PVOID pTargetMap;
	PVOID **pActorMap;

	Tools::ByteQueue mSent, mRecv;

	int mSelfId;
	std::deque<TEMPBUFF> mActiveDoT;
	TEMPDMG mLastAttack;
	std::map<int, int> mDamageRedir;
	std::map<int, PVOID> mActorPointers;
	struct DPS_METER_TEMP_INFO {
		PER_USER_DMG totalDamage;
		TEMPDMG maxDamage;
		int deaths;
		int dotHits;
		int totalHits;
		int critHits;
		int missHits;
	};
	std::map<int, DPS_METER_TEMP_INFO> mDpsInfo;
	std::map<int, TEMPUSERINFO> mActorInfo;


	std::vector<std::pair<int, int>> mCalculatedDamages;
	uint64_t mLastIdleTime = 0;
	uint64_t mCombatResetTime = 10000;

	std::map<int, MedianCalculator> mDotApplyDelayEstimation;
	MedianCalculator mContagionApplyDelayEstimation;

	uint64_t mServerTimestamp;
	uint64_t mLocalTimestamp;

	FFXIVDLL *dll;
	HANDLE hUnloadEvent;

	bool mWindowsAdded = false;
	DPSWindowController &wDPS;
	DOTWindowController &wDOT;

	z_stream inflater;
	unsigned char inflateBuffer[DEFLATE_CHUNK_SIZE];

	void ResolveUsers();
	void AddDamageInfo(TEMPDMG dmg, bool direct);
	void CalculateDps(uint64_t timestamp);
	void UpdateOverlayMessage();
	void ProcessAttackInfo(int source, int target, int skill, ATTACK_INFO *info, uint64_t timestamp);
	void ProcessGameMessage(void *data, uint64_t timestamp, int len, bool setTimestamp);
	void PacketErrorMessage(int signature, int length);
	void ParsePacket(Tools::ByteQueue &p, bool setTimestamp);

	HANDLE hUpdateInfoThread;
	HANDLE hUpdateInfoThreadLock;
	void UpdateInfoThread();
	static DWORD WINAPI UpdateInfoThreadExternal(PVOID p) { ((GameDataProcess*)p)->UpdateInfoThread(); return 0; }

public:
	GameDataProcess(FFXIVDLL *dll, FILE *f, HANDLE unloadEvent);
	~GameDataProcess();

	inline int GetActorType(int id);
	inline int GetTargetId(int type);
	inline char* GetActorName(int id);
	inline TCHAR* GetActorJobString(int id);

	void ResetMeter();
	void ReloadLocalization();

	void OnRecv(char* buf, int len) {
		mRecv.write(buf, len);
		if (mRecv.getUsed() >= 28)
			SetEvent(hUpdateInfoThreadLock);
	}
	void OnSent(const char* buf, int len) {
		mSent.write(buf, len);
		if (mSent.getUsed() >= 28)
			SetEvent(hUpdateInfoThreadLock);
	}
};