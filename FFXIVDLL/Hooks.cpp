#pragma comment(lib, "ws2_32.lib")
#include <Winsock2.h>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx9.h"
#include <Psapi.h>
#include <windowsx.h>
#include "Hooks.h"
#include "FFXIVDLL.h"
#include "GameDataProcess.h"
#include "ChatWindowController.h"
#include "../TsudaKageyu-minhook/include/MinHook.h"

std::atomic_int Hooks::mHookedFunctionDepth;
HMODULE Hooks::hGame;
OverlayRenderer *Hooks::pOverlayRenderer;
FFXIVDLL *Hooks::dll;
WNDPROC Hooks::ffxivWndProc;
int Hooks::ffxivWndPressed = 0;
WindowControllerBase *Hooks::ffxivHookCaptureControl = 0;
WindowControllerBase *Hooks::lastHover = 0;
bool Hooks::mLockCursor = false;
void *Hooks::chatObject = 0;
char *Hooks::chatPtrs[4] = { (char*) 0x06, 0, (char*) 0x06, 0 };
Hooks::HOOKS_ORIG_FN_SET Hooks::pfnOrig = { 0 };
Hooks::HOOKS_BRIDGE_FN_SET Hooks::pfnBridge = { 0 };
bool Hooks::isFFXIVChatWindowOpen = true;
std::map<std::string, PVOID> Hooks::WindowMap;

#ifdef _WIN64
PVOID *Hooks::pDX11SwapChainTable;
extern LRESULT ImGui_ImplDX11_WndProcHandler(HWND, UINT msg, WPARAM wParam, LPARAM lParam);
#else
PVOID *Hooks::pDX9Table;
extern LRESULT ImGui_ImplDX9_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	return DefWindowProc(hwnd, msg, wParam, lParam);
}
Hooks::Hooks(FFXIVDLL *dll) {
	if (MH_OK != MH_Initialize())
		return;

	Hooks::dll = dll;

	dll->memory()->AddCallback(std::bind(&Hooks::MemorySearchCompleteCallback, this));

	ffxivWndProc = (WNDPROC) SetWindowLongPtr(dll->ffxiv(), GWLP_WNDPROC, (LONG_PTR) hook_ffxivWndProc);

	MH_CreateHook(recv, hook_socket_recv, NULL);
	MH_CreateHook(send, hook_socket_send, NULL);
	MH_CreateHook(SetCursor, hook_WinApiSetCursor, (LPVOID*) &pfnBridge.WinApiSetCursor);
	MH_CreateHook(select, hook_socket_select, (LPVOID*) &pfnBridge.SocketSelect);

#ifdef _WIN64

	HWND tempHwnd;
	WNDCLASS wc = { 0 };
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hGame;
	wc.hbrBackground = (HBRUSH) (COLOR_BACKGROUND);
	wc.lpszClassName = L"TESTDX11DEV";

	RegisterClass(&wc);
	tempHwnd = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		L"TESTDX11DEV",
		L"The title of my window",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 640, 480,
		NULL, NULL, wc.hInstance, NULL);

	D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 1;
	sd.BufferDesc.Width = 640;
	sd.BufferDesc.Height = 480;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = tempHwnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

	IDXGISwapChain* m_pTempSwapChain = nullptr;
	ID3D11Device* m_pTempDevice = nullptr;
	ID3D11DeviceContext* m_pTempContext = nullptr;
	D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, D3D11_CREATE_DEVICE_DEBUG, &featureLevel, 1, D3D11_SDK_VERSION, &m_pTempDevice, NULL, &m_pTempContext);

	IDXGIDevice* dxgiDevice = nullptr;
	IDXGIFactory1* dxgiFactory = nullptr;
	IDXGIAdapter* adapter = nullptr;
	if (m_pTempDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice))) goto clear_dx_alloc;
	if (dxgiDevice->GetAdapter(&adapter)) goto clear_dx_alloc;
	if (adapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&dxgiFactory))) goto clear_dx_alloc;
	if (dxgiFactory->CreateSwapChain(m_pTempDevice, &sd, &m_pTempSwapChain)) goto clear_dx_alloc;
	if (m_pTempDevice != nullptr) {

		pDX11SwapChainTable = (PVOID*)*(PVOID*) m_pTempSwapChain;

		MH_CreateHook(pDX11SwapChainTable[8],
			hook_Dx11SwapChain_Present,
			(LPVOID*) &pfnBridge.Dx11SwapChainPresent);
	}

clear_dx_alloc:
	SAFE_RELEASE(m_pTempSwapChain);
	SAFE_RELEASE(dxgiFactory);
	SAFE_RELEASE(adapter);
	SAFE_RELEASE(dxgiDevice);
	SAFE_RELEASE(m_pTempContext);
	SAFE_RELEASE(m_pTempDevice);

	DestroyWindow(tempHwnd);
	UnregisterClass(L"TESTDX11DEV", hGame);

#else
	pDX9Table = (PVOID*)*((DWORD*) (2 + Tools::FindPattern((DWORD) GetModuleHandle(L"d3d9.dll"), 0x128000, (PBYTE)"\xC7\x06\x00\x00\x00\x00\x89\x86\x00\x00\x00\x00\x89\x86", "xx????xx????xx")));

	MH_CreateHook((LPVOID) pDX9Table[DX9_ENDSCENE], hook_Dx9EndScene, (LPVOID*) &pfnBridge.Dx9EndScene);
	MH_CreateHook((LPVOID) pDX9Table[DX9_RESET], hook_Dx9Reset, (LPVOID*) &pfnBridge.Dx9Reset);
	MH_CreateHook((LPVOID) pDX9Table[DX9_PRESENT], hook_Dx9Present, (LPVOID*) &pfnBridge.Dx9Present);
#endif
}

Hooks::~Hooks() {

	std::map<SOCKET, Tools::ByteQueue> *queues[] = { &dll->process()->mToRecv,
		&dll->process()->mRecv,
		&dll->process()->mToSend,
		&dll->process()->mSent };

	int stallTime = GetTickCount() + 6000;
	for (int i = 0; i < sizeof(queues) / sizeof(queues[0]); i++) {
		for (auto j = queues[i]->begin(); j != queues[i]->end(); ++j) {
			while (!j->second.isEmpty() && !j->second.isStall(stallTime - GetTickCount() + 100))
				Sleep(0);
		}
	}

	SetWindowLongPtr(dll->ffxiv(), GWLP_WNDPROC, (LONG_PTR) ffxivWndProc);

	int mainThread = Tools::GetMainThreadID(GetCurrentProcessId());
	HANDLE mainThreadHandle = OpenThread(THREAD_SUSPEND_RESUME, false, mainThread);
	SuspendThread(mainThreadHandle);
	Sleep(10);
	MH_DisableHook(MH_ALL_HOOKS);
	Sleep(10);
	ResumeThread(mainThreadHandle);
	CloseHandle(mainThreadHandle);

	while (mHookedFunctionDepth > 0) Sleep(1);
	Sleep(10);
	MH_Uninitialize();
	Sleep(10);

	if (pOverlayRenderer != nullptr)
		delete pOverlayRenderer;

}

void Hooks::MemorySearchCompleteCallback() {
	MH_CreateHook(pfnOrig.ProcessWindowMessage = (ProcessWindowMessage) dll->memory()->Result.Functions.ProcessWindowMessage, hook_ProcessWindowMessage, (PVOID*) &pfnBridge.ProcessWindowMessage);
	MH_CreateHook(pfnOrig.ProcessNewLine = (ProcessNewLine) dll->memory()->Result.Functions.ProcessNewLine, hook_ProcessNewLine, (PVOID*) &pfnBridge.ProcessNewLine);
	MH_CreateHook(pfnOrig.HideFFXIVWindow = (ShowHideFFXIVWindow) dll->memory()->Result.Functions.HideFFXIVWindow, hook_HideFFXIVWindow, (PVOID*) &pfnBridge.HideFFXIVWindow);
	MH_CreateHook(pfnOrig.ShowFFXIVWindow = (ShowHideFFXIVWindow) dll->memory()->Result.Functions.ShowFFXIVWindow, hook_ShowFFXIVWindow, (PVOID*) &pfnBridge.ShowFFXIVWindow);
	MH_CreateHook(pfnOrig.OnNewChatItem = (OnNewChatItem) dll->memory()->Result.Functions.OnNewChatItem, hook_OnNewChatItem, (PVOID*) &pfnBridge.OnNewChatItem);

	Activate();
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
int __fastcall Hooks::hook_OnNewChatItem(void *pthis, void* param, size_t n) {
#else
int __fastcall Hooks::hook_OnNewChatItem(void *pthis, void *unused, void* param, size_t n) {
#endif
	mHookedFunctionDepth++;
	dll->sendPipe("in__", (char*) param, n);
	dll->process()->wChat.addChat(param, n);
	int res = pfnBridge.OnNewChatItem(pthis, param, n);
	mHookedFunctionDepth--;
	return res;
}

#ifdef _WIN64
SIZE_T* __fastcall Hooks::hook_ProcessNewLine(void *pthis, size_t *dw1, char **dw2, int n) {
#else
SIZE_T* __fastcall Hooks::hook_ProcessNewLine(void *pthis, void *unused, size_t *dw1, char **dw2, int n) {
#endif
	mHookedFunctionDepth++;
	if (strncmp((char*) dw2[1], "/o:o", 3) == 0) {
		if (pOverlayRenderer != nullptr) {
			pOverlayRenderer->SetUseDrawOverlay(true);
			pOverlayRenderer->mConfig.Show();
		}
	} else if (strncmp((char*) dw2[1], "/o", 3) == 0) {
		if (pOverlayRenderer != nullptr)
			pOverlayRenderer->SetUseDrawOverlay(!pOverlayRenderer->GetUseDrawOverlay());
	}

	dll->sendPipe("out_", (char*) dw2[1], -1);

	chatObject = pthis;
	SIZE_T *res = pfnBridge.ProcessNewLine(pthis, dw1, dw2, n);
	mHookedFunctionDepth--;
	return res;
}

int WINAPI Hooks::hook_socket_send(SOCKET s, const char* buf, int len, int flags) {
	mHookedFunctionDepth++;
	{
		DWORD result;
		std::lock_guard<std::recursive_mutex> guard(dll->process()->mSocketMapLock);
		Tools::ByteQueue &snd = dll->process()->mToSend[s];
		Tools::ByteQueue &sndq = dll->process()->mSent[s];

		if ((dll->isUnloading() && snd.isEmpty() && sndq.isEmpty()) || WaitForSingleObject(dll->process()->hSendUpdateInfoThread, 0) != WAIT_TIMEOUT) {
			if (snd.getUsed() > 0) {
				DWORD result = 0, flags = 0;
				std::vector<char> buf2(snd.getUsed());
				snd.peek(buf2.data(), buf2.size());
				WSABUF buffer = { static_cast<ULONG>(buf2.size()), buf2.data() };
				Tools::DebugPrint(L"WSASend in send %d: %d\n", (int) s, buf2.size());
				if (WSASend(s, &buffer, 1, &result, flags, nullptr, nullptr) == SOCKET_ERROR) {
					if (WSAGetLastError() != WSAEWOULDBLOCK) {
						Tools::DebugPrint(L" -> error %d\n", WSAGetLastError());
						len = SOCKET_ERROR;
					}
				} else
					snd.waste(buf2.size());
			}
			if (len>0 && dll->process()->mSent[s].getUsed()) {
				DWORD result = 0, flags = 0;
				std::vector<char> buf2(sndq.getUsed());
				sndq.peek(buf2.data(), buf2.size());
				WSABUF buffer = { static_cast<ULONG>(buf2.size()), const_cast<CHAR *>(buf2.data()) };
				Tools::DebugPrint(L"WSASend in send %d: %d\n", (int) s, buf2.size());
				if (WSASend(s, &buffer, 1, &result, flags, nullptr, nullptr) == SOCKET_ERROR) {
					if (WSAGetLastError() != WSAEWOULDBLOCK) {
						Tools::DebugPrint(L" -> error %d\n", WSAGetLastError());
						len = SOCKET_ERROR;
					}
				} else
					sndq.waste(buf2.size());
			}
			if (len > 0) {
				WSABUF buffer = { static_cast<ULONG>(len), const_cast<CHAR *>(buf) };
				if (WSASend(s, &buffer, 1, &result, flags, nullptr, nullptr) != 0)
					len = SOCKET_ERROR;
				// Tools::DebugPrint(L"Unloading: send %d\n", (int) s);
			}
		} else {
			// Tools::DebugPrint(L"send buffer put %d: %d\n", (int) s, len);
			dll->process()->OnSend(s, buf, len);
		}
	}
	mHookedFunctionDepth--;
	return len;
}

int WINAPI Hooks::hook_socket_recv(SOCKET s, char* buf, int len, int flags) {
	mHookedFunctionDepth++;
	DWORD recvlen = 0, dflags = 0;
	{
		std::lock_guard<std::recursive_mutex> guard(dll->process()->mSocketMapLock);
		auto &rcv = dll->process()->mToRecv[s];

		if (dll->isUnloading() && rcv.isEmpty()) {
			WSABUF buffer = { static_cast<ULONG>(len), buf };
			if (WSARecv(s, &buffer, 1, &recvlen, &dflags, nullptr, nullptr) == SOCKET_ERROR)
				recvlen = -1;
			Tools::DebugPrint(L"Unloading: recv %d\n", (int) s);
		} else {
			if (ioctlsocket(s, FIONREAD, &recvlen) == SOCKET_ERROR)
				recvlen = SOCKET_ERROR;
			else if (recvlen > 0) {
				int i = 10;
				do {
					WSABUF buffer = { static_cast<ULONG>(len), buf };
					if (WSARecv(s, &buffer, 1, &recvlen, &dflags, nullptr, nullptr) == SOCKET_ERROR) {
						if (WSAGetLastError() == WSAEWOULDBLOCK)
							recvlen = 0;
						else {
							recvlen = SOCKET_ERROR;
							Tools::DebugPrint(L"WSARecv in recv %d: Error %d\n", (int) s, WSAGetLastError());
						}
					} else if (recvlen > 0) {
						dll->process()->OnRecv(s, buf, recvlen);
						Tools::DebugPrint(L"WSARecv in recv %d: %d\n", (int) s, recvlen);
					}
				} while (recvlen && recvlen != SOCKET_ERROR && --i > 0);
			}

			if (recvlen == 0) {
				recvlen = static_cast<int>(min(rcv.getUsed(), static_cast<size_t>(len)));
				Tools::DebugPrint(L"recv buffer pull %d: %d\n", (int) s, recvlen);
				if (recvlen) {
					rcv.read(buf, recvlen);
				} else
					WSASetLastError(WSAEWOULDBLOCK);
			}
		}
	}
	mHookedFunctionDepth--;
	return recvlen;
}

int WINAPI Hooks::hook_socket_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, const struct timeval *timeout) {
	mHookedFunctionDepth++;
	fd_set readfds1 = *readfds;
	int res = pfnBridge.SocketSelect(nfds, readfds, writefds, exceptfds, timeout);
	bool err = false;
	if (res != SOCKET_ERROR) {
		std::lock_guard<std::recursive_mutex> guard(dll->process()->mSocketMapLock);
		bool take = false;
		if (dll->isUnloading()) {
			for (size_t i = 0; i < readfds1.fd_count && !take; i++) {
				SOCKET &s = readfds1.fd_array[i];
				if (!dll->process()->mRecv[s].isEmpty())
					take = true;
				else if (!dll->process()->mToRecv[s].isEmpty())
					take = true;
			}
		} else
			take = true;

		if (take) {
			if (readfds->fd_count) {
				res -= readfds->fd_count;
				FD_ZERO(readfds);
				if (!dll->isUnloading()) {
					for (size_t i = 0; i < readfds1.fd_count; i++) {
						SOCKET &s = readfds1.fd_array[i];
						DWORD recvlen = 0, flags = 0;
						char buf[8192];
						WSABUF buffer = { sizeof(buf), buf };
						int j = 5;
						do {
							if (WSARecv(s, &buffer, 1, &recvlen, &flags, nullptr, nullptr) == SOCKET_ERROR) {
								if (WSAGetLastError() == WSAEWOULDBLOCK)
									recvlen = 0;
								else {
									res = SOCKET_ERROR;
									err = true;
									Tools::DebugPrint(L"WSARecv in select %d: Error %d\n", (int) s, WSAGetLastError());
								}
							} else if (recvlen > 0) {
								dll->process()->OnRecv(s, buf, recvlen);
								Tools::DebugPrint(L"WSARecv in select %d: %d\n", (int) s, recvlen);
							}
						} while (recvlen && res != SOCKET_ERROR && !err && --j > 0);
					}
				}
			}
			for (size_t i = 0; i < readfds1.fd_count; i++) {
				SOCKET &s = readfds1.fd_array[i];
				if (dll->process()->mToRecv[s].getUsed() && res != SOCKET_ERROR) {
					// Tools::DebugPrint(L"select read avail %d: %d\n", s, (int) dll->process()->mToRecv[s]->getUsed());
					FD_SET(s, readfds);
					res++;
				}
			}
		}
	}
	mHookedFunctionDepth--;
	return err ? SOCKET_ERROR : res;
}

#ifdef _WIN64
char __fastcall Hooks::hook_HideFFXIVWindow(void* pthis) {
#else
char __fastcall Hooks::hook_HideFFXIVWindow(void* pthis, void *_u) {
#endif
	char *wname = (char*) pthis + sizeof(char*);
	if (strncmp(wname, "_Status", 7) == 0)
		isFFXIVChatWindowOpen = false;
	dll->sendPipe("wndh", wname, -1);
	WindowMap[wname] = pthis;
	return pfnBridge.HideFFXIVWindow(pthis);
}

#ifdef _WIN64
char __fastcall Hooks::hook_ShowFFXIVWindow(void* pthis) {
#else
char __fastcall Hooks::hook_ShowFFXIVWindow(void* pthis, void *_u) {
#endif
	char *wname = (char*) pthis + sizeof(char*);
	if (strncmp(wname, "_Status", 7) == 0)
		isFFXIVChatWindowOpen = true;
	else if (strncmp(wname, "ContentsFinderConfirm", 21) == 0 && GetForegroundWindow() != dll->ffxiv())
		FlashWindow(dll->ffxiv(), true);
	dll->sendPipe("wnds", wname, -1);
	WindowMap[(char*) pthis + sizeof(char*)] = pthis;
	return pfnBridge.ShowFFXIVWindow(pthis);
}

char Hooks::hook_ProcessWindowMessage() {
	mHookedFunctionDepth++;
	std::string chatMessage;
	if (chatObject != 0) {
		if (dll->mChatInjectQueue.tryPop(&chatMessage)) {
			size_t res;
			chatPtrs[3] = chatPtrs[1] = (char*) chatMessage.c_str();
			pfnOrig.ProcessNewLine(chatObject, &res, chatPtrs, 20);
		}
	}

	if (dll->hooks()->GetOverlayRenderer())
		dll->hooks()->GetOverlayRenderer()->DoMainThreadOperation();

	char res = pfnBridge.ProcessWindowMessage();
	mHookedFunctionDepth--;
	return res;
}

#ifdef _WIN64

HRESULT APIENTRY Hooks::hook_Dx11SwapChain_Present(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
	mHookedFunctionDepth++;
	if (pOverlayRenderer == nullptr) {
		ID3D11Device *pDevice;
		ID3D11DeviceContext *pContext;
		pSwapChain->GetDevice(__uuidof(ID3D11Device), (LPVOID*) &pDevice);
		pDevice->GetImmediateContext(&pContext);
		pOverlayRenderer = new OverlayRendererDX11(dll, pDevice, pContext, pSwapChain);
	}
	if (pOverlayRenderer != nullptr) {
		pOverlayRenderer->CheckCapture();
		pOverlayRenderer->DrawOverlay();
	}

	HRESULT res = pfnBridge.Dx11SwapChainPresent(pSwapChain, SyncInterval, Flags);
	mHookedFunctionDepth--;
	return res;

}
#else
HRESULT APIENTRY Hooks::hook_Dx9Reset(IDirect3DDevice9 *pDevice, D3DPRESENT_PARAMETERS *pPresentationParameters) {
	mHookedFunctionDepth++;
	if (pOverlayRenderer != nullptr)
		pOverlayRenderer->OnLostDevice();
	HRESULT res = pfnBridge.Dx9Reset(pDevice, pPresentationParameters);
	if (pOverlayRenderer != nullptr)
		pOverlayRenderer->OnResetDevice();
	mHookedFunctionDepth--;
	return res;
}

HRESULT APIENTRY Hooks::hook_Dx9EndScene(IDirect3DDevice9 *pDevice) {
	mHookedFunctionDepth++;

	if (pfnBridge.Dx9SwapChainPresent == nullptr && pDevice->GetNumberOfSwapChains() > 0) {
		IDirect3DSwapChain9 *sc;
		pDevice->GetSwapChain(0, &sc);
		void **vtable = *(void ***) sc;
		MH_CreateHook((LPVOID) vtable[3], hook_Dx9SwapChain_Present, (LPVOID*) &pfnBridge.Dx9SwapChainPresent);
		MH_EnableHook((LPVOID) vtable[3]);
		pOverlayRenderer = new OverlayRendererDX9(dll, pDevice);
	}

	HRESULT res = pfnBridge.Dx9EndScene(pDevice);
	mHookedFunctionDepth--;
	return res;
}

HRESULT APIENTRY Hooks::hook_Dx9SwapChain_Present(IDirect3DSwapChain9 *pSwapChain, const RECT    *pSourceRect, const RECT    *pDestRect, HWND    hDestWindowOverride, const RGNDATA *pDirtyRegion, DWORD dwFlags) {
	mHookedFunctionDepth++;

	if (pOverlayRenderer != nullptr) {
		pOverlayRenderer->CheckCapture();
		pOverlayRenderer->DrawOverlay();
	}

	HRESULT res = pfnBridge.Dx9SwapChainPresent(pSwapChain, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, dwFlags);

	mHookedFunctionDepth--;
	return res;
}

HRESULT APIENTRY Hooks::hook_Dx9Present(IDirect3DDevice9 *pDevice, const RECT    *pSourceRect, const RECT    *pDestRect, HWND    hDestWindowOverride, const RGNDATA *pDirtyRegion) {
	mHookedFunctionDepth++;

	if (pOverlayRenderer != nullptr) {
		pOverlayRenderer->DrawOverlay();
	}

	HRESULT res = pfnBridge.Dx9Present(pDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);

	mHookedFunctionDepth--;
	return res;
}
#endif

HCURSOR WINAPI Hooks::hook_WinApiSetCursor(HCURSOR hCursor) {
	mHookedFunctionDepth++;
	HCURSOR res = hCursor;
	if (!mLockCursor)
		res = pfnBridge.WinApiSetCursor(hCursor);
	mHookedFunctionDepth--;
	return res;
}

void Hooks::updateLastFocus(WindowControllerBase *control) {
	if (ffxivHookCaptureControl == control)
		return;
	OverlayRenderer::Control *tmp;
	tmp = ffxivHookCaptureControl;
	while (tmp != nullptr) {
		tmp->statusFlag[CONTROL_STATUS_FOCUS] = 0;
		tmp = tmp->getParent();
	}
	tmp = ffxivHookCaptureControl = control;
	while (tmp != nullptr) {
		tmp->statusFlag[CONTROL_STATUS_FOCUS] = 1;
		tmp = tmp->getParent();
	}
}

LRESULT CALLBACK Hooks::hook_ffxivWndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam) {
	mHookedFunctionDepth++;
	bool callDef = false;

	ImGuiIO &io = ImGui::GetIO();
#ifdef _WIN64
	ImGui_ImplDX11_WndProcHandler(hWnd, iMessage, wParam, lParam);
#else
	ImGui_ImplDX9_WndProcHandler(hWnd, iMessage, wParam, lParam);
#endif
	if (io.WantCaptureMouse)
		mLockCursor = true;
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
			if (io.WantCaptureMouse) {
				mHookedFunctionDepth--;
				return 0;
			} else if (pOverlayRenderer != nullptr && !pOverlayRenderer->mConfig.UseExternalWindow) {
				int xPos = GET_X_LPARAM(lParam);
				int yPos = GET_Y_LPARAM(lParam);
				WindowControllerBase *pos = pOverlayRenderer->GetWindowAt(xPos, yPos);
				if (lastHover != nullptr)
					lastHover->statusFlag[CONTROL_STATUS_HOVER] = 0;
				lastHover = pos;
				if (lastHover != nullptr)
					lastHover->statusFlag[CONTROL_STATUS_HOVER] = 1;
				if (ffxivHookCaptureControl != nullptr && ffxivHookCaptureControl->isLocked())
					updateLastFocus(nullptr);
				WindowControllerBase *control = ffxivHookCaptureControl ? ffxivHookCaptureControl : pos;
				if (control && !ffxivWndPressed) {
					mLockCursor = true;
					int res = control->callback(hWnd, iMessage, wParam, lParam);
					switch (res) {
						case 0: updateLastFocus(nullptr); callDef = true; break;
						case 1: updateLastFocus(nullptr); callDef = false; break;
						case 2: updateLastFocus(control); callDef = true; break;
						case 3: updateLastFocus(control); callDef = false; break;
						case 4: callDef = true; break;
						case 5: callDef = false; break;
					}
					if (pos != control && pos != nullptr) {
						int res = pos->callback(hWnd, iMessage, wParam, lParam);
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
					mLockCursor = false;
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
		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_CHAR:
			if (wParam == VK_SNAPSHOT) {
				pOverlayRenderer->CaptureScreen();
			} else if (io.WantCaptureKeyboard || io.WantTextInput)
				break;
			else if (ffxivHookCaptureControl && !ffxivWndPressed) {
				int res = ffxivHookCaptureControl->callback(hWnd, iMessage, wParam, lParam);
				switch (res) {
					case 0: updateLastFocus(nullptr); callDef = true; break;
					case 1: updateLastFocus(nullptr); callDef = false; break;
					case 2: updateLastFocus(ffxivHookCaptureControl); callDef = true; break;
					case 3: updateLastFocus(ffxivHookCaptureControl); callDef = false; break;
					case 4: callDef = true; break;
					case 5: callDef = false; break;
				}
			} else
				callDef = true;
			break;
		default:
			callDef = true;
			if (ffxivHookCaptureControl && !ffxivWndPressed) {
				WindowControllerBase *control = ffxivHookCaptureControl;
				int res = control->callback(hWnd, iMessage, wParam, lParam);
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
	if (callDef) {
		LRESULT res = CallWindowProc(ffxivWndProc, hWnd, iMessage, wParam, lParam);
		mHookedFunctionDepth--;
		return res;
	}
	mHookedFunctionDepth--;
	return 0;
}