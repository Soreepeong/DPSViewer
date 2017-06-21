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
		loc[LANGUAGE_ENGLISH]["OPTION_HOWTO_OPEN"] = "Type /o:o into the chat box to open this window.\nType /o to show or hide the overlays.\nPlease use Quit button before you exit FFXIV.";
		loc[LANGUAGE_ENGLISH]["OPTION_LANGUAGE"] = "Language";
		loc[LANGUAGE_ENGLISH]["OPTION_UNLOCK"] = "Unlock";
		loc[LANGUAGE_ENGLISH]["OPTION_LOCK"] = "Lock";
		loc[LANGUAGE_ENGLISH]["OPTION_DPS_HIDE"] = "Hide DPS";
		loc[LANGUAGE_ENGLISH]["OPTION_DPS_SHOW"] = "Show DPS";
		loc[LANGUAGE_ENGLISH]["OPTION_DOT_HIDE"] = "Hide DOT";
		loc[LANGUAGE_ENGLISH]["OPTION_DOT_SHOW"] = "Show DOT";
		loc[LANGUAGE_ENGLISH]["OPTION_USE_EXTERNAL_WINDOW"] = "Use external window";
		loc[LANGUAGE_ENGLISH]["OPTION_EXTERNAL_WINDOW_REFRESH_RATE"] = "Ext. refresh rate";
		loc[LANGUAGE_ENGLISH]["OPTION_OTHERS_HIDE_NAME"] = "Hide other names";
		loc[LANGUAGE_ENGLISH]["OPTION_OTHERS_SHOW"] = "Show everyone";
		loc[LANGUAGE_ENGLISH]["OPTION_SHOW_IFF_CHAT_WINDOW_OPEN"] = "Hide during cutscenes";
		loc[LANGUAGE_ENGLISH]["OPTION_DPS_NAME_LENGTH"] = "Max name width";
		loc[LANGUAGE_ENGLISH]["OPTION_DPS_SIMPLE_VIEW_THRESHOLD"] = "Simple view threshold";
		loc[LANGUAGE_ENGLISH]["OPTION_FONT_BOLD"] = "Bold text";
		loc[LANGUAGE_ENGLISH]["OPTION_FONT_SIZE"] = "Text size";
		loc[LANGUAGE_ENGLISH]["OPTION_FONT_NAME"] = "Font name";
		loc[LANGUAGE_ENGLISH]["OPTION_TEXT_BORDER"] = "Text border";
		loc[LANGUAGE_ENGLISH]["OPTION_TRANSPARENCY"] = "Transparency";
		loc[LANGUAGE_ENGLISH]["OPTION_METER_PADDING"] = "Padding";
		loc[LANGUAGE_ENGLISH]["OPTION_DPS_RESET_TIME"] = "DPS reset delay";
		loc[LANGUAGE_ENGLISH]["OPTION_SHOW_TIMES"] = "Show times";
		loc[LANGUAGE_ENGLISH]["OPTION_APPLY"] = "Apply";
		loc[LANGUAGE_ENGLISH]["OPTION_CAPTURE_PATH"] = "Capture path";
		loc[LANGUAGE_ENGLISH]["OPTION_DPS_RESET"] = "Reset DPS Meter";
		loc[LANGUAGE_ENGLISH]["OPTION_QUIT"] = "Quit";
		loc[LANGUAGE_ENGLISH]["OPTION_LOADING"] = "Loading...";
		loc[LANGUAGE_ENGLISH]["OPTION_HEADER_DPS"] = "DPS###HEADER_DPS";
		loc[LANGUAGE_ENGLISH]["OPTION_HEADER_APPEARANCE"] = "Appearance###HEADER_APPEARANCE";
		loc[LANGUAGE_ENGLISH]["OPTION_HEADER_CAPTURE"] = "Capture###HEADER_CAPTURE";
		loc[LANGUAGE_ENGLISH]["OPTION_HEADER_ABOUT"] = "About###HEADER_ABOUT";
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
		loc[LANGUAGE_KOREAN]["OPTION_HOWTO_OPEN"] = u8"채팅창에 /o:o라고 입력하시면 이 창을 다시 열 수 있습니다.\n/o라고 입력하시면 모든 오버레이를 숨기고 보일 수 있습니다.\n게임을 종료할 때는 반드시 아래 종료 버튼을 클릭해 주세요.";
		loc[LANGUAGE_KOREAN]["OPTION_LANGUAGE"] = u8"언어";
		loc[LANGUAGE_KOREAN]["OPTION_UNLOCK"] = u8"잠금 해제";
		loc[LANGUAGE_KOREAN]["OPTION_LOCK"] = u8"잠그기";
		loc[LANGUAGE_KOREAN]["OPTION_DPS_HIDE"] = u8"DPS 숨기기";
		loc[LANGUAGE_KOREAN]["OPTION_DPS_SHOW"] = u8"DPS 보이기";
		loc[LANGUAGE_KOREAN]["OPTION_DOT_HIDE"] = u8"DOT 숨기기";
		loc[LANGUAGE_KOREAN]["OPTION_DOT_SHOW"] = u8"DOT 보이기";
		loc[LANGUAGE_KOREAN]["OPTION_USE_EXTERNAL_WINDOW"] = u8"외부 창 사용";
		loc[LANGUAGE_KOREAN]["OPTION_EXTERNAL_WINDOW_REFRESH_RATE"] = u8"외부 창 갱신 빈도";
		loc[LANGUAGE_KOREAN]["OPTION_OTHERS_HIDE_NAME"] = u8"다른 이름 숨기기";
		loc[LANGUAGE_KOREAN]["OPTION_OTHERS_SHOW"] = u8"모두 보이기";
		loc[LANGUAGE_KOREAN]["OPTION_SHOW_IFF_CHAT_WINDOW_OPEN"] = u8"동영상 재생 시 숨기기";
		loc[LANGUAGE_KOREAN]["OPTION_FONT_BOLD"] = u8"굵은 글자";
		loc[LANGUAGE_KOREAN]["OPTION_DPS_NAME_LENGTH"] = u8"최대 이름 너비";
		loc[LANGUAGE_KOREAN]["OPTION_DPS_SIMPLE_VIEW_THRESHOLD"] = u8"간략 보기 최소 인원";
		loc[LANGUAGE_KOREAN]["OPTION_FONT_SIZE"] = u8"글자 크기";
		loc[LANGUAGE_KOREAN]["OPTION_FONT_NAME"] = u8"서체 이름";
		loc[LANGUAGE_KOREAN]["OPTION_TEXT_BORDER"] = u8"글자 테두리";
		loc[LANGUAGE_KOREAN]["OPTION_TRANSPARENCY"] = u8"투명도";
		loc[LANGUAGE_KOREAN]["OPTION_METER_PADDING"] = u8"간격";
		loc[LANGUAGE_KOREAN]["OPTION_DPS_RESET_TIME"] = u8"DPS 초기화 대기시간";
		loc[LANGUAGE_KOREAN]["OPTION_SHOW_TIMES"] = u8"시각 보이기";
		loc[LANGUAGE_KOREAN]["OPTION_APPLY"] = u8"적용";
		loc[LANGUAGE_KOREAN]["OPTION_CAPTURE_PATH"] = u8"캡쳐 경로";
		loc[LANGUAGE_KOREAN]["OPTION_DPS_RESET"] = u8"DPS 초기화";
		loc[LANGUAGE_KOREAN]["OPTION_QUIT"] = u8"종료";
		loc[LANGUAGE_KOREAN]["OPTION_LOADING"] = u8"불러오는 중...";
		loc[LANGUAGE_KOREAN]["OPTION_HEADER_DPS"] = "DPS###HEADER_DPS";
		loc[LANGUAGE_KOREAN]["OPTION_HEADER_APPEARANCE"] = u8"외관###HEADER_APPEARANCE";
		loc[LANGUAGE_KOREAN]["OPTION_HEADER_CAPTURE"] = u8"캡쳐###HEADER_CAPTURE";
		loc[LANGUAGE_KOREAN]["OPTION_HEADER_ABOUT"] = u8"정보###HEADER_ABOUT";
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

	std::wstring getDoTName(int skill) {
		switch (language) {
			case LANGUAGE_KOREAN:
				switch (skill) {
					case 0xf8: return L"파멸의 진";
					case 0xf4: return L"골절";
					case 0x77: return L"이단 찌르기";
					case 0x76: return L"꽃잎 폭풍";
					case 0x6a: return L"혈도 찌르기";
					case 0xf6: return L"파쇄권";
					case 0x7c: return L"독화살";
					case 0x81: return L"바람 화살";
					case 0x8f: return L"에어로";
					case 0x90: return L"에어로라";
					case 0xa1: return L"선더";
					case 0xa2: return L"선더라";
					case 0xa3: return L"선더가";
					case 0xb3: return L"바이오";
					case 0xb4: return L"미아즈마";
					case 0xbd: return L"바이오라";
					case 0xbc: return L"미아즈라";
					case 0x13a: return L"지옥의 화염";
					case 0x12: return L"독";
					case 0xec: return L"초코 찌르기";
					case 0x1ec: return L"무쌍베기";
					case 0x1fc: return L"그림자 송곳니";
					case 0x356: return L"산탄 사격";
					case 0x2e5: return L"재앙";
					case 0x346: return L"컴버스";
					case 0x34b: return L"컴버라";
					case 0x2d5: return L"꿰뚫는 검격";
					case 0x31e: return L"에어로가";
					case 0x4b5: return L"flamethrower";
					case 0x4ba: return L"선더쟈";
					case 0x527: return L"higanbana";
					case 0x529: return L"caustic bite";
					case 0x52a: return L"stormbite";
					case 0x52e: return L"바이오가";
					case 0x52f: return L"미아즈가";
				}
			default:
				switch (skill) {
					case 0x12: return L"Poison";
					case 0x6a: return L"Touch of Death";
					case 0x76: return L"Chaos Thrust";
					case 0x77: return L"Phlebotomize";
					case 0x7c: return L"Venomous Bite";
					case 0x81: return L"Windbite";
					case 0x8f: return L"Aero";
					case 0x90: return L"Aero II";
					case 0xa1: return L"Thunder";
					case 0xa2: return L"Thunder II";
					case 0xa3: return L"Thunder III";
					case 0xb3: return L"Bio";
					case 0xb4: return L"Miasma";
					case 0xbc: return L"Miasma II";
					case 0xbd: return L"Bio II";
					case 0xec: return L"Choco Beak";
					case 0xf4: return L"Fracture";
					case 0xf6: return L"Demolish";
					case 0xf8: return L"Circle of Scorn";
					case 0x13a: return L"Inferno";
					case 0x1ec: return L"Mutilation";
					case 0x1fc: return L"Shadow Fang";
					case 0x2d5: return L"Goring Blade";
					case 0x2e5: return L"Scourge";
					case 0x31e: return L"Aero III";
					case 0x346: return L"Combust";
					case 0x34b: return L"Combust II";
					case 0x356: return L"Lead Shot";
					case 0x4b5: return L"Flamethrower";
					case 0x4ba: return L"Thunder IV";
					case 0x527: return L"Higanbana";
					case 0x529: return L"Caustic Bite";
					case 0x52a: return L"Stormbite";
					case 0x52e: return L"Bio III";
					case 0x52f: return L"Miasma III";
				}
		}
		TCHAR def[256];
		swprintf(def, sizeof(def)/sizeof(TCHAR), L"%x", skill);
		return def;
	}
}
