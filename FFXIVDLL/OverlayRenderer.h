#pragma once
#include <Windows.h>
#include<deque>
#include<mutex>
#include "dx9hook.h"
#include "dx9table.h"

struct OVERLAY_RENDER_TABLE_ROW {
	std::wstring icon;
	std::wstring cols[12];
	int align[12] = { 0 };
	float barSize = 0;
	int count = 0;
};

class OverlayRenderer {
	friend class Hooks;
private:
	LPD3DXFONT mFont;
	std::deque<uint64_t> mFpsMeter;
	std::mutex mDrawLock;
	TCHAR mDebugText[65536] = { 0 };
	TCHAR mDotText[65536] = { 0 };
	std::vector<OVERLAY_RENDER_TABLE_ROW> mTableRows;
	IDirect3DDevice9* pDevice;
	LPD3DXSPRITE mSprite;

	std::map<std::wstring, LPDIRECT3DTEXTURE9> mClassIcons;
	std::map<std::wstring, D3DCOLOR> mClassColors;

	struct {
		int UseDrawOverlay = true;
		int UseDrawOverlayEveryone = true;
		float x = 0;
		float y = 0;
		float xdot = 0;
		float ydot = 0;
		int fontSize = 17;
		int bold = true;
		int border = 2;
		int hideOtherUser = 0;
		int backgroundTransparency = 0x80;
		TCHAR capturePath[512] = L"";
		int captureFormat = D3DXIFF_BMP;
		TCHAR fontName[512] = L"Segoe UI";
	}mConfig;

	static BOOL CALLBACK EnumResNameProcExternal(HMODULE  hModule, LPCTSTR  lpszType, LPTSTR   lpszName, LONG_PTR lParam) {
		return ((OverlayRenderer*)lParam)->EnumResNameProc(hModule, lpszType, lpszName);
	}
	BOOL CALLBACK EnumResNameProc(HMODULE hModule, LPCTSTR lpszType, LPTSTR lpszName);

	void DrawTexture(int x, int y, int w, int h, LPDIRECT3DTEXTURE9 tex);
	void DrawBox(int x, int y, int w, int h, D3DCOLOR Color);
	void DrawText(int x, int y, TCHAR *text, D3DCOLOR Color);
	void DrawText(int x, int y, int width, TCHAR *text, D3DCOLOR Color, int align);
	void RenderOverlay();
	void RenderDpsBox(D3DVIEWPORT9 &prt);
	void RenderDotBox(D3DVIEWPORT9 &prt);

	std::deque<IDirect3DSurface9*> mCaptureBuffers;
	std::mutex mCaptureMutex;
	bool mDoCapture = false;
	void CaptureBackgroundSave(IDirect3DSurface9 *buf);
	void CaptureBackgroundSaverThread();
	HANDLE hSaverThread;
	static DWORD WINAPI CaptureBackgroundSaverExternal(PVOID p) { ((OverlayRenderer*)p)->CaptureBackgroundSaverThread(); return 0; }

public:
	OverlayRenderer(IDirect3DDevice9* device);
	~OverlayRenderer();

	int GetFPS();
	void SetText(TCHAR *t);
	void SetDoTText(TCHAR *t);
	void SetTable(std::vector<OVERLAY_RENDER_TABLE_ROW> &tbl);

	void CaptureScreen();
	void ReloadResources();

	void DrawOverlay();
	void CheckCapture();

	void SetUseDrawOverlay(bool use);
	int GetUseDrawOverlay();
	void SetUseDrawOverlayEveryone(bool use);
	int GetUseDrawOverlayEveryone();
	void SetBackgroundTransparency(int t);
	void SetCapturePath(TCHAR *c);
	void GetCaptureFormat(int format);
	int GetHideOtherUser();
	void SetHideOtherUser();
	void SetDpsBoxLocation(float x, float y);
	void SetDotBoxLocation(float x, float y);
	void SetFontSize(int size);
	void SetFontWeight();
	void SetFontName(TCHAR *c);
	void SetBorder(int border);
};

