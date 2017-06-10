#pragma once
#include <Windows.h>
#ifdef _WIN64
#include "D3D11.h"

//////////////////////////// TEMPORARY ////////////////////////////
typedef enum D3DXIMAGE_FILEFORMAT {
	D3DXIFF_BMP = 0,
	D3DXIFF_JPG = 1,
	D3DXIFF_TGA = 2,
	D3DXIFF_PNG = 3,
	D3DXIFF_DDS = 4,
	D3DXIFF_PPM = 5,
	D3DXIFF_DIB = 6,
	D3DXIFF_HDR = 7,
	D3DXIFF_PFM = 8,
	D3DXIFF_FORCE_DWORD = 0x7fffffff
} D3DXIMAGE_FILEFORMAT, *LPD3DXIMAGE_FILEFORMAT;
#define D3DCOLOR_ARGB(a,r,g,b) \
    ((DWORD)((((a)&0xff)<<24)|(((r)&0xff)<<16)|(((g)&0xff)<<8)|((b)&0xff)))
#define D3DCOLOR_RGBA(r,g,b,a) D3DCOLOR_ARGB(a,r,g,b)
#define D3DCOLOR_XRGB(r,g,b)   D3DCOLOR_ARGB(0xff,r,g,b)


#else
#include "dx9hook.h"
#endif
#include "Languages.h"

class FFXIVDLL;
class OverlayRenderer;

class ImGuiConfigWindow{
private:
	FFXIVDLL *dll;
	OverlayRenderer *mRenderer;
	bool mConfigVisibility;
	bool mConfigVisibilityPrev;
	TCHAR mSettingFilePath[1024];
	char mLanguageChoice[8192];
	std::string mAboutText;

	int readIni(TCHAR *k1, TCHAR *k2, int def = 0, int min = 0x80000000, int max = 0x7fffffff);
	float readIni(TCHAR *k1, TCHAR *k2, float def = 0, float min = -1, float max = 1);
	void readIni(TCHAR *k1, TCHAR *k2, char *def, char *target, int bufSize);

	void writeIni(TCHAR *k1, TCHAR *k2, int val);
	void writeIni(TCHAR *k1, TCHAR *k2, float val);
	void writeIni(TCHAR *k1, TCHAR *k2, const char* val);

public:

	bool ShowOnlyWhenChatWindowOpen = true;
	int UseDrawOverlay = true;
	bool ShowEveryDPS = true;
	int fontSize = 17;
	bool bold = true;
	int border = 2;
	int transparency = 100;
	int padding = 4;
	bool hideOtherUserName = 0;
	char capturePath[512] = "";
	int captureFormat = D3DXIFF_BMP;
	char fontName[512] = "Segoe UI";

	bool showTimes;

	int combatResetTime;

	ImGuiConfigWindow(FFXIVDLL *dll, OverlayRenderer *renderer);
	~ImGuiConfigWindow();

	void Show();
	void Render();
};

