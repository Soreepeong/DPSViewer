#pragma once
#include<windows.h>
#include<map>
#include<atomic>
#include "dx9hook.h"
#include "dx9table.h"
#include "OverlayRenderer.h"
#include "WindowControllerBase.h"

#define DEFLATE_CHUNK_SIZE (1 << 18)

class Hooks {
	friend class OverlayRenderer;
public:

	typedef struct _CHATITEM {
		int timestamp;
		short code;
		short _u1;
		char chat;
} CHATITEM, *PCHATITEM;

#ifdef _WIN64
	typedef SIZE_T* (__thiscall *ProcessNewLine)(void*, DWORD*, char**, int);
	typedef int(__thiscall *OnNewChatItem)(void*, PCHATITEM, size_t);
	SIZE_T* __fastcall hook_ProcessNewLine(void*, DWORD*, char**, int);
	int __fastcall hook_OnNewChatItem(void*, PCHATITEM, size_t);

#else
	typedef SIZE_T* (__thiscall *ProcessNewLine)(void*, DWORD*, char**, int);
	typedef int(__thiscall *OnNewChatItem)(void*, PCHATITEM, size_t);
	typedef int(__thiscall *OnRecv)(SOCKET, char*, int, int);
	typedef int(__thiscall *OnSend)(SOCKET, char*, int, int);

	static SIZE_T* __fastcall hook_ProcessNewLine(void*, void*, SIZE_T*, char**, int);
	static int __fastcall hook_OnNewChatItem(void*, void*, PCHATITEM, size_t);
	static int WINAPI hook_socket_recv(SOCKET s, char* buf, int len, int flags);
	static int WINAPI hook_socket_send(SOCKET s, const char* buf, int len, int flags);
#endif
	typedef char(*ProcessWindowMessage)();
	static char hook_ProcessWindowMessage();
	typedef HRESULT(APIENTRY *Dx9Reset)(IDirect3DDevice9 *pDevice, D3DPRESENT_PARAMETERS *pPresentationParameters);
	typedef HRESULT(APIENTRY *Dx9EndScene)(IDirect3DDevice9 *pDevice);
	typedef HRESULT(APIENTRY *Dx9Present)(IDirect3DDevice9 *pDevice, const RECT *pSourceRect, const RECT *pDestRect, HWND hDestWindowOverride, const RGNDATA *pDirtyRegion);
	typedef HRESULT(APIENTRY *Dx9SwapChainPresent)(IDirect3DSwapChain9 *pDevice, const RECT *pSourceRect, const RECT *pDestRect, HWND hDestWindowOverride, const RGNDATA *pDirtyRegion, DWORD dwFlags);
	typedef HCURSOR(WINAPI *WinApiSetCursor)(HCURSOR hCursor);

	static HRESULT APIENTRY hook_Reset(IDirect3DDevice9 *pDevice, D3DPRESENT_PARAMETERS *pPresentationParameters);
	static HRESULT APIENTRY hook_EndScene(IDirect3DDevice9 *pDevice);
	static HRESULT APIENTRY hook_SwapChain_Present(IDirect3DSwapChain9 *pSwapChain, const RECT    *pSourceRect, const RECT    *pDestRect, HWND    hDestWindowOverride, const RGNDATA *pDirtyRegion, DWORD dwFlags);
	static HRESULT APIENTRY hook_Present(IDirect3DDevice9 *pSwapChain, const RECT    *pSourceRect, const RECT    *pDestRect, HWND    hDestWindowOverride, const RGNDATA *pDirtyRegion);

	static HCURSOR WINAPI hook_SetCursor(HCURSOR hCursor);


	static struct HOOKS_ORIG_FN_SET {
		ProcessWindowMessage ProcessWindowMessage;
		ProcessNewLine ProcessNewLine;
		OnNewChatItem OnNewChatItem;
	}pfnOrig;

	static struct HOOKS_BRIDGE_FN_SET {
		ProcessWindowMessage ProcessWindowMessage;
		ProcessNewLine ProcessNewLine;
		OnNewChatItem OnNewChatItem;
		Dx9EndScene Dx9EndScene;
		Dx9Reset Dx9Reset;
		Dx9Present Dx9Present;
		Dx9SwapChainPresent Dx9SwapChainPresent;
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
	static bool mHookStarted;
	static std::atomic_int mHookedFunctionDepth;
	static HMODULE hGame;
	static PVOID *pDX9Table;
	static OverlayRenderer *pOverlayRenderer;
	static FFXIVDLL *dll;

	static void* chatObject;
	static char* chatPtrs[4];

	template<typename T>
	void applyRelativeHook(int delta, T *fn, PVOID fn_hook, T *fn_bridge) {
		*fn = (T)((char *)hGame + delta);

		MH_CreateHook(*fn, fn_hook, (LPVOID*)fn_bridge);
	}


public:
	Hooks(FFXIVDLL *dll, FILE *f);
	~Hooks();
	void Activate();

	OverlayRenderer* GetOverlayRenderer() {
		return pOverlayRenderer;
	}
};