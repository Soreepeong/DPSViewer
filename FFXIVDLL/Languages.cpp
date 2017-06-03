#include"Languages.h"

namespace Languages {
	int language;
	std::map<std::string, std::string> loc[_LANGUAGE_COUNT];
	std::map<std::wstring, std::wstring> loc2[_LANGUAGE_COUNT];

	const char* get(char* code) {
		if (loc[language].find(code) == loc[language].end())
			return code;
		return loc[language][code].c_str();
	}

	const wchar_t* get(wchar_t* code) {
		if (loc2[language].find(code) == loc2[language].end())
			return code;
		return loc2[language][code].c_str();
	}
	
	TCHAR *getLanguageName(int lang) {
		switch (lang) {
		case LANGUAGE_ENGLISH:
			return L"English";
		case LANGUAGE_KOREAN:
			return L"한국어";
		default:
			return L"(unknown)";
		}
	}

	void initialize() {
		loc[LANGUAGE_ENGLISH]["OPTION_WINDOW_TITLE"] = "Options###IDOptionWindow";
		loc[LANGUAGE_ENGLISH]["OPTION_HOWTO_OPEN"] = "Type /o:o into the chat box to open this window.";
		loc[LANGUAGE_ENGLISH]["OPTION_LANGUAGE"] = "Language";
		loc[LANGUAGE_ENGLISH]["OPTION_UNLOCK"] = "Unlock";
		loc[LANGUAGE_ENGLISH]["OPTION_LOCK"] = "Lock";
		loc[LANGUAGE_ENGLISH]["OPTION_DPS_HIDE"] = "Hide DPS";
		loc[LANGUAGE_ENGLISH]["OPTION_DPS_SHOW"] = "Show DPS";
		loc[LANGUAGE_ENGLISH]["OPTION_DOT_HIDE"] = "Hide DOT";
		loc[LANGUAGE_ENGLISH]["OPTION_DOT_SHOW"] = "Show DOT";
		loc[LANGUAGE_ENGLISH]["OPTION_OTHERS_HIDE"] = "Hide others";
		loc[LANGUAGE_ENGLISH]["OPTION_OTHERS_SHOW"] = "Show others";
		loc[LANGUAGE_ENGLISH]["OPTION_FONT_BOLD"] = "Bold text";
		loc[LANGUAGE_ENGLISH]["OPTION_FONT_SIZE"] = "Text size";
		loc[LANGUAGE_ENGLISH]["OPTION_FONT_NAME"] = "Font name";
		loc[LANGUAGE_ENGLISH]["OPTION_TEXT_BORDER"] = "Text border";
		loc[LANGUAGE_ENGLISH]["OPTION_TRANSPARENCY"] = "Transparency";
		loc[LANGUAGE_ENGLISH]["OPTION_METER_PADDING"] = "Padding";
		loc[LANGUAGE_ENGLISH]["OPTION_DPS_RESET_TIME"] = "DPS reset wait";
		loc[LANGUAGE_ENGLISH]["OPTION_APPLY"] = "Apply";
		loc[LANGUAGE_ENGLISH]["OPTION_CAPTURE_PATH"] = "Capture path";
		loc[LANGUAGE_ENGLISH]["OPTION_DPS_RESET"] = "Reset DPS Meter";
		loc[LANGUAGE_ENGLISH]["OPTION_QUIT"] = "Quit";
		loc2[LANGUAGE_ENGLISH][L"DPSTABLE_NAME"] = L"Name";
		loc2[LANGUAGE_ENGLISH][L"DPSTABLE_DPS"] = L"DPS";
		loc2[LANGUAGE_ENGLISH][L"DPSTABLE_TOTAL"] = L"Total";
		loc2[LANGUAGE_ENGLISH][L"DPSTABLE_CRITICAL"] = L"Crit";
		loc2[LANGUAGE_ENGLISH][L"DPSTABLE_CMH"] = L"C/M/H";
		loc2[LANGUAGE_ENGLISH][L"DPSTABLE_MAX"] = L"Max";
		loc2[LANGUAGE_ENGLISH][L"DPSTABLE_DEATH"] = L"Death";
		loc2[LANGUAGE_ENGLISH][L"DOTTABLE_NAME"] = L"Name";
		loc2[LANGUAGE_ENGLISH][L"DOTTABLE_SKILL"] = L"Skill";
		loc2[LANGUAGE_ENGLISH][L"DOTTABLE_TIME"] = L"Time";

		loc[LANGUAGE_KOREAN]["OPTION_WINDOW_TITLE"] = u8"설정###IDOptionWindow";
		loc[LANGUAGE_KOREAN]["OPTION_HOWTO_OPEN"] = u8"채팅창에 /o:o라고 입력하시면 이 창을 다시 열 수 있습니다.";
		loc[LANGUAGE_KOREAN]["OPTION_LANGUAGE"] = u8"언어";
		loc[LANGUAGE_KOREAN]["OPTION_UNLOCK"] = u8"잠금 해제";
		loc[LANGUAGE_KOREAN]["OPTION_LOCK"] = u8"잠그기";
		loc[LANGUAGE_KOREAN]["OPTION_DPS_HIDE"] = u8"DPS 숨기기";
		loc[LANGUAGE_KOREAN]["OPTION_DPS_SHOW"] = u8"DPS 보이기";
		loc[LANGUAGE_KOREAN]["OPTION_DOT_HIDE"] = u8"DOT 숨기기";
		loc[LANGUAGE_KOREAN]["OPTION_DOT_SHOW"] = u8"DOT 보이기";
		loc[LANGUAGE_KOREAN]["OPTION_OTHERS_HIDE"] = u8"타인 숨기기";
		loc[LANGUAGE_KOREAN]["OPTION_OTHERS_SHOW"] = u8"타인 보이기";
		loc[LANGUAGE_KOREAN]["OPTION_FONT_BOLD"] = u8"굵은 글자";
		loc[LANGUAGE_KOREAN]["OPTION_FONT_SIZE"] = u8"글자 크기";
		loc[LANGUAGE_KOREAN]["OPTION_FONT_NAME"] = u8"서체 이름";
		loc[LANGUAGE_KOREAN]["OPTION_TEXT_BORDER"] = u8"글자 테두리";
		loc[LANGUAGE_KOREAN]["OPTION_TRANSPARENCY"] = u8"투명도";
		loc[LANGUAGE_KOREAN]["OPTION_METER_PADDING"] = u8"간격";
		loc[LANGUAGE_KOREAN]["OPTION_DPS_RESET_TIME"] = u8"DPS 초기화 대기시간";
		loc[LANGUAGE_KOREAN]["OPTION_APPLY"] = u8"적용";
		loc[LANGUAGE_KOREAN]["OPTION_CAPTURE_PATH"] = u8"캡쳐 경로";
		loc[LANGUAGE_KOREAN]["OPTION_DPS_RESET"] = u8"DPS 초기화";
		loc[LANGUAGE_KOREAN]["OPTION_QUIT"] = u8"종료";
		loc2[LANGUAGE_KOREAN][L"DPSTABLE_NAME"] = L"이름";
		loc2[LANGUAGE_KOREAN][L"DPSTABLE_DPS"] = L"DPS";
		loc2[LANGUAGE_KOREAN][L"DPSTABLE_TOTAL"] = L"전체";
		loc2[LANGUAGE_KOREAN][L"DPSTABLE_CRITICAL"] = L"극대";
		loc2[LANGUAGE_KOREAN][L"DPSTABLE_CMH"] = L"극/빗/전";
		loc2[LANGUAGE_KOREAN][L"DPSTABLE_MAX"] = L"최대";
		loc2[LANGUAGE_KOREAN][L"DPSTABLE_DEATH"] = L"사망";
		loc2[LANGUAGE_KOREAN][L"DOTTABLE_NAME"] = L"이름";
		loc2[LANGUAGE_KOREAN][L"DOTTABLE_SKILL"] = L"스킬";
		loc2[LANGUAGE_KOREAN][L"DOTTABLE_TIME"] = L"시간";
	}

	TCHAR *getDoTName(int skill) {
		switch (language) {
		case LANGUAGE_KOREAN:
			switch (skill) {
			case 0xf8: return L"파멸의 진";
			case 0xf4: return L"골절";
			case 0x77: return L"phlebotomize";
			case 0x76: return L"chaos thrust";
			case 0x6a: return L"touch of death";
			case 0xf6: return L"demolish";
			case 0x7c: return L"venomous bite";
			case 0x81: return L"windbite";
			case 0x8f: return L"에어로";
			case 0x90: return L"에어로라";
			case 0xa1: return L"선더";
			case 0xa2: return L"선더";
			case 0xa3: return L"선더";
			case 0xb3: return L"바이오";
			case 0xb4: return L"미아즈마";
			case 0xbd: return L"바이오라";
			case 0xbc: return L"미아즈라";
			case 0x13a: return L"inferno";
			case 0x12: return L"poison";
			case 0xec: return L"choco beak";
			case 0x1ec: return L"mutilation";
			case 0x1fc: return L"shadow fang";
			case 0x356: return L"lead shot";
			case 0x2e5: return L"scourge";
			case 0x346: return L"combust";
			case 0x34b: return L"combust ii";
			case 0x2d5: return L"goring blade";
			case 0x31e: return L"에어로가";
			}
			return L"(알 수 없음)";
		default:
			switch (skill) {
			case 0xf8: return L"circle of scorn";
			case 0xf4: return L"fracture";
			case 0x77: return L"phlebotomize";
			case 0x76: return L"chaos thrust";
			case 0x6a: return L"touch of death";
			case 0xf6: return L"demolish";
			case 0x7c: return L"venomous bite";
			case 0x81: return L"windbite";
			case 0x8f: return L"aero";
			case 0x90: return L"aero ii";
			case 0xa1: return L"thunder";
			case 0xa2: return L"thunder";
			case 0xa3: return L"thunder";
			case 0xb3: return L"bio";
			case 0xb4: return L"miasma";
			case 0xbd: return L"bio ii";
			case 0xbc: return L"miasma ii";
			case 0x13a: return L"inferno";
			case 0x12: return L"poison";
			case 0xec: return L"choco beak";
			case 0x1ec: return L"mutilation";
			case 0x1fc: return L"shadow fang";
			case 0x356: return L"lead shot";
			case 0x2e5: return L"scourge";
			case 0x346: return L"combust";
			case 0x34b: return L"combust ii";
			case 0x2d5: return L"goring blade";
			case 0x31e: return L"aero iii";
			}
			return L"(unknown)";
		}
	}
}
