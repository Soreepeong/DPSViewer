#pragma once
#include <Windows.h>
#include <deque>
#include <mutex>
#include <map>
#include "dx9hook.h"
#include "dx9table.h"

enum CONTROL_STATUS_ID : uint32_t{
	CONTROL_STATUS_DEFAULT,
	CONTROL_STATUS_PRESS,
	CONTROL_STATUS_HOVER,
	CONTROL_STATUS_FOCUS,
	_CONTROL_STATUS_COUNT
};
enum CONTROL_LAYOUT_DIRECTION {
	LAYOUT_DIRECTION_HORIZONTAL,
	LAYOUT_DIRECTION_HORIZONTAL_REVERSE,
	LAYOUT_DIRECTION_VERTICAL,
	LAYOUT_DIRECTION_VERTICAL_REVERSE,
	LAYOUT_DIRECTION_VERTICAL_TABLE,
	LAYOUT_ABSOLUTE
};
enum CONTROL_TEXT_TYPE : uint32_t {
	CONTROL_TEXT_STRING,
	CONTROL_TEXT_RESNAME,
	CONTROL_TEXT_FILENAME
};
enum CONTROL_CHILD_TYPE : uint32_t {
	CHILD_TYPE_FOREGROUND,
	CHILD_TYPE_NORMAL,
	CHILD_TYPE_BACKGROUND,
	_CHILD_TYPE_COUNT
};

class FFXIVDLL;
class WindowController;

class OverlayRenderer {
	friend class Hooks;
public:

	class Control {
		friend class OverlayRenderer;

		std::recursive_mutex layoutLock;
		std::vector<Control*> children[_CHILD_TYPE_COUNT];
		Control* parent;

	public:
		typedef bool(*EventCallback)(Control &c, HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

		Control();
		Control(CONTROL_LAYOUT_DIRECTION direction);
		Control(std::wstring txt, CONTROL_TEXT_TYPE useIcon, int align);
		Control(float width, float height, D3DCOLOR background);
		~Control();

		class Status {
		public:
			int useDefault;
			D3DCOLOR foreground;
			D3DCOLOR background;
			D3DCOLOR border;
		};
		const int SIZE_WRAP = -1;
		const int SIZE_FILL = -2;

		CONTROL_LAYOUT_DIRECTION layoutDirection;

		WindowController *callback;

		int visible;

		int relativePosition;
		int relativeSize;
		union {
			int x;
			float xF;
		};
		union {
			int y;
			float yF;
		};
		union {
			int width;
			float widthF;
		};
		union {
			int height;
			float heightF;
		};

		int calcX;
		int calcY;
		int calcWidth;
		int calcHeight;

		int border;
		int margin;
		int padding;
		int statusFlag[_CONTROL_STATUS_COUNT];
		Status statusMap[_CONTROL_STATUS_COUNT];

		CONTROL_TEXT_TYPE useIcon;
		std::wstring text;
		int textAlign;

		void measure(OverlayRenderer *target, RECT &area, int widthFixed, int heightFixed, int skipChildren);
		void draw(OverlayRenderer *target);
		int hittest(int x, int y);
		void removeAllChildren();
		void addChild(OverlayRenderer::Control *c, CONTROL_CHILD_TYPE type);
		void addChild(OverlayRenderer::Control *c);
		OverlayRenderer::Control* getChild(int i, CONTROL_CHILD_TYPE type);
		OverlayRenderer::Control* getChild(int i);
		OverlayRenderer::Control* removeChild(int i, CONTROL_CHILD_TYPE type);
		OverlayRenderer::Control* removeChild(int i);
		int getChildCount(CONTROL_CHILD_TYPE type);
		int getChildCount();
		Control* getParent();
		void requestFront();
		std::recursive_mutex& getLock();
	};

private:

	FFXIVDLL *dll;

	LPD3DXFONT mFont;
	std::deque<uint64_t> mFpsMeter;
	IDirect3DDevice9* pDevice;
	LPD3DXSPRITE mSprite;

	std::map<std::wstring, LPDIRECT3DTEXTURE9> mResourceTextures;
	std::map<std::wstring, LPDIRECT3DTEXTURE9> mFileTextures;

	LPDIRECT3DTEXTURE9 GetTextureFromResource(TCHAR *resName);
	LPDIRECT3DTEXTURE9 GetTextureFromFile(TCHAR *resName);

	struct {
		int UseDrawOverlay = true;
		int UseDrawOverlayEveryone = true;
		int fontSize = 17;
		int bold = true;
		int border = 2;
		int hideOtherUser = 0;
		TCHAR capturePath[512] = L"";
		int captureFormat = D3DXIFF_BMP;
		TCHAR fontName[512] = L"Segoe UI";
	}mConfig;

	Control mWindows;

	void DrawTexture(int x, int y, int w, int h, LPDIRECT3DTEXTURE9 tex);
	void DrawBox(int x, int y, int w, int h, D3DCOLOR Color);
	void DrawText(int x, int y, TCHAR *text, D3DCOLOR Color);
	void DrawText(int x, int y, int width, int height, TCHAR *text, D3DCOLOR Color, int align);
	void RenderOverlay();

	std::deque<IDirect3DSurface9*> mCaptureBuffers;
	std::recursive_mutex mCaptureMutex;
	bool mDoCapture = false;
	void CaptureBackgroundSave(IDirect3DSurface9 *buf);
	void CaptureBackgroundSaverThread();
	HANDLE hSaverThread;
	static DWORD WINAPI CaptureBackgroundSaverExternal(PVOID p) { ((OverlayRenderer*)p)->CaptureBackgroundSaverThread(); return 0; }

public:
	OverlayRenderer(FFXIVDLL *dll, IDirect3DDevice9* device);
	~OverlayRenderer();

	int GetFPS();

	void CaptureScreen();
	void ReloadResources();

	void DrawOverlay();
	void CheckCapture();

	void SetUseDrawOverlay(bool use);
	int GetUseDrawOverlay();
	void SetUseDrawOverlayEveryone(bool use);
	int GetUseDrawOverlayEveryone();
	void SetCapturePath(TCHAR *c);
	void GetCaptureFormat(int format);
	int GetHideOtherUser();
	void SetHideOtherUser();
	void SetFontSize(int size);
	void SetFontWeight();
	void SetFontName(TCHAR *c);
	void SetBorder(int border);

	void AddWindow(Control *windows);
	Control* GetRoot();
	Control* GetWindowAt(int x, int y);
	Control* GetWindowAt(Control *in, int x, int y);

	class Control;
};

