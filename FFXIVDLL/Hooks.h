#pragma once
#include<windows.h>
#include<map>
#include<atomic>
#include "WindowControllerBase.h"

#ifdef _WIN64
#include <d3d11.h>
#else
#include "dx9hook.h"
#include "dx9table.h"
#endif

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x) \
   if(x != NULL)        \
   {                    \
      x->Release();     \
      x = NULL;         \
   }
#endif

class FFXIVDLL;

class Hooks {
	friend class OverlayRenderer;
public:

	static bool isFFXIVChatWindowOpen;

	typedef struct _CHATITEM {
		int timestamp;
		short code;
		short _u1;
		char chat;
	} CHATITEM, *PCHATITEM;

#ifdef _WIN64
	static SIZE_T* __fastcall hook_ProcessNewLine(void*, DWORD*, char**, int);
	static char __fastcall hook_ShowFFXIVWindow(void*);
	static char __fastcall hook_HideFFXIVWindow(void*);
	static int __fastcall hook_OnNewChatItem(void*, PCHATITEM, size_t);

	typedef HRESULT(APIENTRY *Dx11SwapChain_Present)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);

	static HRESULT APIENTRY hook_Dx11SwapChain_Present(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);

#else
	static SIZE_T* __fastcall hook_ProcessNewLine(void*, void*, SIZE_T*, char**, int);
	static int __fastcall hook_OnNewChatItem(void*, void*, PCHATITEM, size_t);

	static char __fastcall hook_ShowFFXIVWindow(void*, void*);
	static char __fastcall hook_HideFFXIVWindow(void*, void*);

	typedef HRESULT(APIENTRY *Dx9Reset)(IDirect3DDevice9 *pDevice, D3DPRESENT_PARAMETERS *pPresentationParameters);
	typedef HRESULT(APIENTRY *Dx9EndScene)(IDirect3DDevice9 *pDevice);
	typedef HRESULT(APIENTRY *Dx9Present)(IDirect3DDevice9 *pDevice, const RECT *pSourceRect, const RECT *pDestRect, HWND hDestWindowOverride, const RGNDATA *pDirtyRegion);
	typedef HRESULT(APIENTRY *Dx9SwapChainPresent)(IDirect3DSwapChain9 *pDevice, const RECT *pSourceRect, const RECT *pDestRect, HWND hDestWindowOverride, const RGNDATA *pDirtyRegion, DWORD dwFlags);

	static HRESULT APIENTRY hook_Dx9Reset(IDirect3DDevice9 *pDevice, D3DPRESENT_PARAMETERS *pPresentationParameters);
	static HRESULT APIENTRY hook_Dx9EndScene(IDirect3DDevice9 *pDevice);
	static HRESULT APIENTRY hook_Dx9SwapChain_Present(IDirect3DSwapChain9 *pSwapChain, const RECT    *pSourceRect, const RECT    *pDestRect, HWND    hDestWindowOverride, const RGNDATA *pDirtyRegion, DWORD dwFlags);
	static HRESULT APIENTRY hook_Dx9Present(IDirect3DDevice9 *pSwapChain, const RECT    *pSourceRect, const RECT    *pDestRect, HWND    hDestWindowOverride, const RGNDATA *pDirtyRegion);

#endif
	typedef int(__thiscall *OnNewChatItem)(void*, PCHATITEM, size_t);
	typedef SIZE_T* (__thiscall *ProcessNewLine)(void*, DWORD*, char**, int);
	typedef char (__thiscall *ShowHideFFXIVWindow)(void*);
	typedef char(*ProcessWindowMessage)();
	typedef HCURSOR(WINAPI *WinApiSetCursor)(HCURSOR hCursor);
	typedef int(__thiscall *OnRecv)(SOCKET, char*, int, int);
	typedef int(__thiscall *OnSend)(SOCKET, char*, int, int);

	static char hook_ProcessWindowMessage();
	static int WINAPI hook_socket_recv(SOCKET s, char* buf, int len, int flags);
	static int WINAPI hook_socket_send(SOCKET s, const char* buf, int len, int flags);
	static HCURSOR WINAPI hook_WinApiSetCursor(HCURSOR hCursor);

	void MemorySearchCompleteCallback();

	static struct HOOKS_ORIG_FN_SET {
		ProcessWindowMessage ProcessWindowMessage;
		ProcessNewLine ProcessNewLine;
		ShowHideFFXIVWindow ShowFFXIVWindow;
		ShowHideFFXIVWindow HideFFXIVWindow;
		OnNewChatItem OnNewChatItem;
	}pfnOrig;

	static struct HOOKS_BRIDGE_FN_SET {
		ProcessWindowMessage ProcessWindowMessage;
		ProcessNewLine ProcessNewLine;
		ShowHideFFXIVWindow ShowFFXIVWindow;
		ShowHideFFXIVWindow HideFFXIVWindow;
		OnNewChatItem OnNewChatItem;
#ifdef _WIN64
		Dx11SwapChain_Present Dx11SwapChainPresent;
#else
		Dx9EndScene Dx9EndScene;
		Dx9Reset Dx9Reset;
		Dx9Present Dx9Present;
		Dx9SwapChainPresent Dx9SwapChainPresent;
#endif
		WinApiSetCursor WinApiSetCursor;
	}pfnBridge;
private:

	static WNDPROC ffxivWndProc;
	static int ffxivWndPressed;
	static WindowControllerBase *lastHover;
	static WindowControllerBase *ffxivHookCaptureControl;
	static void updateLastFocus(WindowControllerBase *control);
	static LRESULT CALLBACK hook_ffxivWndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);

	static bool mLockCursor;
	static std::atomic_int mHookedFunctionDepth;
	static HMODULE hGame;
#ifdef _WIN64
	static PVOID *pDX11SwapChainTable;
#else
	static PVOID *pDX9Table;
#endif
	static OverlayRenderer *pOverlayRenderer;
	static FFXIVDLL *dll;

	static void* chatObject;
	static char* chatPtrs[4];

	template<typename T>
	void applyRelativeHook(int delta, T *fn, PVOID fn_hook, T *fn_bridge) {
		*fn = (T) ((char *) hGame + delta);

		MH_CreateHook(*fn, fn_hook, (LPVOID*) fn_bridge);
	}


public:
	Hooks(FFXIVDLL *dll);
	~Hooks();
	void Activate();

	static std::map<std::string, PVOID> WindowMap;

	OverlayRenderer* GetOverlayRenderer() {
		return pOverlayRenderer;
	}
};