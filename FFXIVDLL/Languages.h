#pragma once
#include<Windows.h>
#include<string>
#include<map>

namespace Languages {
	enum LANGUAGE : uint32_t {
		LANGUAGE_ENGLISH = 0,
		LANGUAGE_KOREAN = 1,
		_LANGUAGE_COUNT
	};

	extern int language;

	extern void initialize();
	extern const char* get(char* code);
	extern const wchar_t* get(wchar_t* code);
	extern TCHAR* getLanguageName(int lang);
	extern std::wstring getDoTName(int skill);

	extern std::map<std::string, std::string> loc[_LANGUAGE_COUNT];
	extern std::map<std::wstring, std::wstring> loc2[_LANGUAGE_COUNT];
}