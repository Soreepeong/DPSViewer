#include "stdafx.h"
#include "Hooks.h"
#include "Tools.h"
#include <Psapi.h>
#include<windowsx.h>

bool Hooks::mHookStarted = false;
std::atomic_int Hooks::mHookedFunctionDepth;
HMODULE Hooks::hGame;
PVOID *Hooks::pDX9Table;
OverlayRenderer *Hooks::pOverlayRenderer;
FFXIVDLL *Hooks::dll;
HWND Hooks::ffxivhWnd;
WNDPROC Hooks::ffxivWndProc;
int Hooks::ffxivWndPressed = 0;
OverlayRenderer::Control *Hooks::ffxivHookCaptureControl = 0;
OverlayRenderer::Control *Hooks::lastHover = 0;

void *Hooks::chatObject = 0;
char *Hooks::chatPtrs[4] = { (char*)0x06, 0, (char*)0x06, 0 };

Hooks::HOOKS_ORIG_FN_SET Hooks::pfnOrig = { 0 };
Hooks::HOOKS_BRIDGE_FN_SET Hooks::pfnBridge = { 0 };

Hooks::Hooks(FFXIVDLL *dll, FILE *f) {
	if (MH_OK != MH_Initialize())
		return;

	Hooks::dll = dll;

	DWORD cbNeeded;
	EnumProcessModulesEx(GetCurrentProcess(), &hGame, sizeof(hGame), &cbNeeded, LIST_MODULES_32BIT | LIST_MODULES_64BIT);

	int n1, n2, n3;
	fwscanf(f, L"%d%d%d", &n1, &n2, &n3);
	fwscanf(f, L"%d", &ffxivhWnd);

	ffxivWndProc = (WNDPROC)SetWindowLong(ffxivhWnd, GWL_WNDPROC, (LONG)hook_ffxivWndProc);

	applyRelativeHook(n1, &pfnOrig.ProcessWindowMessage, Hooks::hook_ProcessWindowMessage, &pfnBridge.ProcessWindowMessage);
	applyRelativeHook(n2, &pfnOrig.ProcessNewLine, Hooks::hook_ProcessNewLine, &pfnBridge.ProcessNewLine);
	applyRelativeHook(n3, &pfnOrig.OnNewChatItem, Hooks::hook_OnNewChatItem, &pfnBridge.OnNewChatItem);
	MH_CreateHook(recv, hook_socket_recv, NULL);
	MH_CreateHook(send, hook_socket_send, NULL);

	pDX9Table = (PVOID*)*((DWORD*)(2 + Tools::FindPattern((DWORD)GetModuleHandle(L"d3d9.dll"), 0x128000, (PBYTE)"\xC7\x06\x00\x00\x00\x00\x89\x86\x00\x00\x00\x00\x89\x86", "xx????xx????xx")));

	MH_CreateHook((LPVOID)pDX9Table[DX9_ENDSCENE], hook_EndScene, (LPVOID*)&pfnBridge.Dx9EndScene);
	MH_CreateHook((LPVOID)pDX9Table[DX9_RESET], hook_Reset, (LPVOID*)&pfnBridge.Dx9Reset);
	MH_CreateHook((LPVOID)pDX9Table[DX9_PRESENT], hook_Present, (LPVOID*)&pfnBridge.Dx9Present);
}

Hooks::~Hooks() {

	SetWindowLong(ffxivhWnd, GWL_WNDPROC, (LONG)ffxivWndProc);

	int mainThread = Tools::GetMainThreadID(GetCurrentProcessId());
	HANDLE mainThreadHandle = OpenThread(THREAD_SUSPEND_RESUME, false, mainThread);
	SuspendThread(mainThreadHandle);
	MH_DisableHook(MH_ALL_HOOKS);
	ResumeThread(mainThreadHandle);
	CloseHandle(mainThreadHandle);

	while (mHookedFunctionDepth > 0) Sleep(1);
	MH_Uninitialize();

	if (pOverlayRenderer != nullptr)
		delete pOverlayRenderer;
}

HRESULT APIENTRY Hooks::hook_Reset(IDirect3DDevice9 *pDevice, D3DPRESENT_PARAMETERS *pPresentationParameters) {
	mHookedFunctionDepth++;
	if (pOverlayRenderer != nullptr)
		pOverlayRenderer->ReloadResources();
	HRESULT res = pfnBridge.Dx9Reset(pDevice, pPresentationParameters);
	mHookedFunctionDepth--;
	return res;
}

void Hooks::Activate() {
	int mainThread = Tools::GetMainThreadID(GetCurrentProcessId());
	HANDLE mainThreadHandle = OpenThread(THREAD_SUSPEND_RESUME, false, mainThread);
	SuspendThread(mainThreadHandle);

	MH_EnableHook(MH_ALL_HOOKS);

	ResumeThread(mainThreadHandle);
	CloseHandle(mainThreadHandle);
}

#ifdef _WIN64
SIZE_T* __fastcall Hooks::hook_ProcessNewLine(void *pthis, DWORD *dw1, char **dw2, int n) {
#else
SIZE_T* __fastcall Hooks::hook_ProcessNewLine(void *pthis, void *unused, DWORD *dw1, char **dw2, int n) {
#endif
	mHookedFunctionDepth++;
	if (strncmp((char*)dw2[1], "/o:s", 4) == 0) {
		int size;
		sscanf((char*)dw2[1], "/o:s %d", &size);
		if (size < 9) size = 9;
		if (size > 128) size = 128;
		pOverlayRenderer->SetFontSize(size);
	} else if (strncmp((char*)dw2[1], "/o:t", 4) == 0) {
		int t;
		sscanf((char*)dw2[1], "/o:t %d", &t);
		t = max(0, min(255, t));
		dll->process()->mDPSController->setTransparency(t);
		dll->process()->mDOTController->setTransparency(t);
	} else if (strncmp((char*)dw2[1], "/o:f", 4) == 0) {
		TCHAR tmp[512];
		MultiByteToWideChar(CP_UTF8, 0, (char*)dw2[1] + 5, -1, tmp, sizeof(tmp) / sizeof(TCHAR));
		pOverlayRenderer->SetFontName(tmp);
	} else if (strncmp((char*)dw2[1], "/o:b", 4) == 0) {
		pOverlayRenderer->SetFontWeight();
	} else if (strncmp((char*)dw2[1], "/o:cp", 5) == 0) {
		if (strlen((char*)dw2[1]) < 6)
			pOverlayRenderer->SetCapturePath(L"");
		else {
			TCHAR tmp[512];
			MultiByteToWideChar(CP_UTF8, 0, (char*)dw2[1] + 6, -1, tmp, sizeof(tmp) / sizeof(TCHAR));
			pOverlayRenderer->SetCapturePath(tmp);
		}
	} else if (strncmp((char*)dw2[1], "/o:cf", 5) == 0) {
		if (strlen((char*)dw2[1]) < 6)
			pOverlayRenderer->GetCaptureFormat(D3DXIFF_BMP);
		else {
			if (strncmp((char*)dw2[1] + 6, "b", 1) == 0)
				pOverlayRenderer->GetCaptureFormat(D3DXIFF_BMP);
			else if (strncmp((char*)dw2[1] + 6, "j", 1) == 0)
				pOverlayRenderer->GetCaptureFormat(D3DXIFF_JPG);
			else if (strncmp((char*)dw2[1] + 6, "p", 1) == 0)
				pOverlayRenderer->GetCaptureFormat(D3DXIFF_PNG);
			else
				dll->pipe()->AddChat("/e supported types: bmp, jpg, png");
		}
	} else if (strncmp((char*)dw2[1], "/o:w", 4) == 0) {
		int size;
		sscanf((char*)dw2[1], "/o:w %d", &size);
		if (size < 0) size = 0;
		if (size > 3) size = 3;
		pOverlayRenderer->SetBorder(size);
	} else if (strncmp((char*)dw2[1], "/o:a", 4) == 0) {
		if (pOverlayRenderer != nullptr)
			pOverlayRenderer->SetUseDrawOverlayEveryone(!pOverlayRenderer->GetUseDrawOverlayEveryone());
	} else if (strncmp((char*)dw2[1], "/o:o", 3) == 0) {
		dll->process()->mConfigController->toggleVisibility();
	} else if (strncmp((char*)dw2[1], "/o", 3) == 0) {
		if (pOverlayRenderer != nullptr)
			pOverlayRenderer->SetUseDrawOverlay(!pOverlayRenderer->GetUseDrawOverlay());
	}
	std::string k("cmsg");
	chatObject = pthis;
	k.append((char*)dw2[1]);
	dll->pipe()->SendInfo(k);
	SIZE_T *res = pfnBridge.ProcessNewLine(pthis, dw1, dw2, n);
	mHookedFunctionDepth--;
	return res;
}

#ifdef _WIN64
int __fastcall Hooks::hook_OnNewChatItem(void *pthis, PCHATITEM param, size_t n) {
#else
int __fastcall Hooks::hook_OnNewChatItem(void *pthis, void *unused, PCHATITEM param, size_t n) {
#endif
	mHookedFunctionDepth++;
	std::string k("chat");
	k.append((char*)param, n);
	dll->pipe()->SendInfo(k);
	int res = pfnBridge.OnNewChatItem(pthis, param, n);
	mHookedFunctionDepth--;
	return res;
}

int WINAPI Hooks::hook_socket_recv(SOCKET s, char* buf, int len, int flags) {
	mHookedFunctionDepth++;
	DWORD result = 0, flags2 = flags;
	WSABUF buffer = { static_cast<ULONG>(len), buf };

	int alen = WSARecv(s, &buffer, 1, &result, &flags2, nullptr, nullptr) == 0 ? static_cast<int>(result) : SOCKET_ERROR;
	__try {
		if (alen > 0)
			dll->process()->OnRecv(buf, alen);
	} __except (EXCEPTION_EXECUTE_HANDLER) {}
	mHookedFunctionDepth--;
	return alen;
}

int WINAPI Hooks::hook_socket_send(SOCKET s, const char* buf, int len, int flags) {
	mHookedFunctionDepth++;
	DWORD result = 0;
	WSABUF buffer = { static_cast<ULONG>(len), const_cast<CHAR *>(buf) };

	int alen = WSASend(s, &buffer, 1, &result, flags, nullptr, nullptr) == 0 ? static_cast<int>(result) : SOCKET_ERROR;
	__try {
		if (alen > 0)
			dll->process()->OnSent(buf, alen);
	} __except (EXCEPTION_EXECUTE_HANDLER) {}
	mHookedFunctionDepth--;
	return alen;
}

char Hooks::hook_ProcessWindowMessage() {
	mHookedFunctionDepth++;
	std::string chatMessage;
	if (chatObject != 0) {
		if (dll->pipe()->injectQueue.tryPop(&chatMessage)) {
			DWORD res;
			chatPtrs[3] = chatPtrs[1] = (char*)chatMessage.c_str();
			pfnOrig.ProcessNewLine(chatObject, &res, chatPtrs, 20);
		}
	}

	if (pDX9Table != 0 && !mHookStarted) {
		mHookStarted = true;
		MH_EnableHook(MH_ALL_HOOKS);
	}

	if (GetAsyncKeyState(VK_SNAPSHOT) == (SHORT)0x8001)
		pOverlayRenderer->CaptureScreen();

	char res = pfnBridge.ProcessWindowMessage();
	mHookedFunctionDepth--;
	return res;
}

HRESULT APIENTRY Hooks::hook_EndScene(IDirect3DDevice9 *pDevice)
{
	mHookedFunctionDepth++;

	if (pfnBridge.Dx9SwapChainPresent == nullptr && pDevice->GetNumberOfSwapChains() > 0) {
		IDirect3DSwapChain9 *sc;
		pDevice->GetSwapChain(0, &sc);
		void **vtable = *(void ***)sc;
		MH_CreateHook((LPVOID)vtable[3], hook_SwapChain_Present, (LPVOID*)&pfnBridge.Dx9SwapChainPresent);
		MH_EnableHook(MH_ALL_HOOKS);
		pOverlayRenderer = new OverlayRenderer(dll, pDevice);
	}

	HRESULT res = pfnBridge.Dx9EndScene(pDevice);
	mHookedFunctionDepth--;
	return res;
}

HRESULT APIENTRY Hooks::hook_SwapChain_Present(IDirect3DSwapChain9 *pSwapChain, const RECT    *pSourceRect, const RECT    *pDestRect, HWND    hDestWindowOverride, const RGNDATA *pDirtyRegion, DWORD dwFlags) {
	mHookedFunctionDepth++;

	if (pOverlayRenderer != nullptr) {
		pOverlayRenderer->CheckCapture();
		pOverlayRenderer->DrawOverlay();
	}

	HRESULT res = pfnBridge.Dx9SwapChainPresent(pSwapChain, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, dwFlags);

	mHookedFunctionDepth--;
	return res;
}

HRESULT APIENTRY Hooks::hook_Present(IDirect3DDevice9 *pDevice, const RECT    *pSourceRect, const RECT    *pDestRect, HWND    hDestWindowOverride, const RGNDATA *pDirtyRegion) {
	mHookedFunctionDepth++;

	if (pOverlayRenderer != nullptr) {
		pOverlayRenderer->DrawOverlay();
	}

	HRESULT res = pfnBridge.Dx9Present(pDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);

	mHookedFunctionDepth--;
	return res;
}

void Hooks::updateLastFocus(OverlayRenderer::Control *control) {
	if (ffxivHookCaptureControl == control)
		return;
	while(ffxivHookCaptureControl != nullptr) {
		ffxivHookCaptureControl->statusFlag[CONTROL_STATUS_FOCUS] = 0;
		ffxivHookCaptureControl = ffxivHookCaptureControl->getParent();
	}
	ffxivHookCaptureControl = control;
	while (control != nullptr) {
		control->statusFlag[CONTROL_STATUS_FOCUS] = 0;
		control = control->getParent();
	}
}

LRESULT CALLBACK Hooks::hook_ffxivWndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam) {
	bool callDef = false;
	switch (iMessage) {
	case WM_MOUSEMOVE:
		callDef = true;
	case WM_RBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_LBUTTONDBLCLK:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_MBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MOUSEHOVER:
	case WM_MOUSEHWHEEL:
	case WM_MOUSELEAVE:
	case WM_MOUSEWHEEL:
		if (pOverlayRenderer != nullptr) {
			int xPos = GET_X_LPARAM(lParam);
			int yPos = GET_Y_LPARAM(lParam);
			OverlayRenderer::Control *pos = pOverlayRenderer->GetWindowAt(xPos, yPos);
			if (lastHover != nullptr && lastHover->callback)
				lastHover->statusFlag[CONTROL_STATUS_HOVER] = 0;
			lastHover = pos;
			if (lastHover != nullptr && lastHover->callback)
				lastHover->statusFlag[CONTROL_STATUS_HOVER] = 1;
			if (ffxivHookCaptureControl != nullptr && ffxivHookCaptureControl->callback && ffxivHookCaptureControl->callback->isLocked())
				updateLastFocus(nullptr);
			OverlayRenderer::Control *control = ffxivHookCaptureControl ? ffxivHookCaptureControl : pos;
			if (control && control->callback && !ffxivWndPressed) {
				int res = control->callback->callback(hWnd, iMessage, wParam, lParam);
				switch (res) {
				case 0: updateLastFocus(nullptr); callDef = true; break;
				case 1: updateLastFocus(nullptr); callDef = false; break;
				case 2: updateLastFocus(control); callDef = true; break;
				case 3: updateLastFocus(control); callDef = false; break;
				case 4: callDef = true; break;
				case 5: callDef = false; break;
				}
				if (pos != control && pos != nullptr) {
					int res = pos->callback->callback(hWnd, iMessage, wParam, lParam);
					switch (res) {
					case 0: updateLastFocus(nullptr); callDef = true; break;
					case 1: updateLastFocus(nullptr); callDef = false; break;
					case 2: updateLastFocus(pos); callDef = true; break;
					case 3: updateLastFocus(pos); callDef = false; break;
					case 4: callDef = true; break;
					case 5: callDef = false; break;
					}
				}
			} else {
				callDef = true;
				switch (iMessage) {
				case WM_LBUTTONDOWN:
				case WM_RBUTTONDOWN:
				case WM_MBUTTONDOWN:
					ffxivWndPressed = true;
					break;
				case WM_LBUTTONUP:
				case WM_RBUTTONUP:
				case WM_MBUTTONUP:
					ffxivWndPressed = false;
					break;
				}
			}
		} else
			callDef = true;
		break;
	default:
		callDef = true;
		if (ffxivHookCaptureControl) {
			OverlayRenderer::Control *control = ffxivHookCaptureControl;
			int res = control->callback->callback(hWnd, iMessage, wParam, lParam);
			switch (res) {
			case 0: updateLastFocus(nullptr); callDef = true; break;
			case 1: updateLastFocus(nullptr); callDef = false; break;
			case 2: updateLastFocus(control); callDef = true; break;
			case 3: updateLastFocus(control); callDef = false; break;
			case 4: callDef = true; break;
			case 5: callDef = false; break;
			}
		}
	}
	if (callDef)
		return CallWindowProc(ffxivWndProc, hWnd, iMessage, wParam, lParam);
	return 0;
}