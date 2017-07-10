#include "OverlayRenderer.h"
#include "WindowControllerBase.h"
#include <psapi.h>
#include "Tools.h"
#include "Hooks.h"
#include "FFXIVDLL.h"
#include "resource.h"
#include "imgui/imgui.h"

OverlayRenderer *OverlayRenderer::ExternalRendererRef;

OverlayRenderer::OverlayRenderer(FFXIVDLL *dll) :
	dll (dll),
	mConfig(dll, this),
	hSaverThread(INVALID_HANDLE_VALUE) {

	memset(mWindows.statusMap, 0, sizeof(mWindows.statusMap));
	mWindows.layoutDirection = LAYOUT_ABSOLUTE;

	HRSRC hResource = FindResource(dll->instance(), MAKEINTRESOURCE(IDR_FONT_NANUMGOTHIC), L"TTFFONT");
	if (hResource) {
		HGLOBAL hLoadedResource = LoadResource(dll->instance(), hResource);
		if (hLoadedResource) {
			LPVOID pLockedResource = LockResource(hLoadedResource);
			if (pLockedResource) {
				DWORD dwResourceSize = SizeofResource(dll->instance(), hResource);
				if (0 != dwResourceSize) {
					void * res = ImGui::GetIO().MemAllocFn(dwResourceSize);
					memcpy(res, pLockedResource, dwResourceSize);
					ImGui::GetIO().Fonts->AddFontFromMemoryTTF(res, dwResourceSize, 14, 0, ImGui::GetIO().Fonts->GetGlyphRangesKorean());
				}
			}
		}
	}
	MSG msg = { 0 };
	WNDCLASS wc = { 0 };
	ExternalRendererRef = this;
	wc.lpfnWndProc = ExternalWindowWndProcExternal;
	wc.hInstance = dll->instance();
	wc.hbrBackground = (HBRUSH) (COLOR_BACKGROUND);
	wc.lpszClassName = EXTERNAL_WINDOW_CLASS;
	RegisterClass(&wc);
}

OverlayRenderer::~OverlayRenderer() {
	if (hSaverThread != INVALID_HANDLE_VALUE)
		WaitForSingleObject(hSaverThread, -1);

	UnregisterClass(EXTERNAL_WINDOW_CLASS, dll->instance());
}

bool OverlayRenderer::IsUnloadable() {
	return mUnloadable;
}

void OverlayRenderer::DoMainThreadOperation() {
	std::unique_lock<std::recursive_mutex> guard(mWindows.getLock());
	if (dll->isUnloading()) {
		std::unique_lock<std::recursive_mutex> guard(mWindows.getLock());
		for (auto i = mExternalWindows.begin(); i != mExternalWindows.end(); ++i) {
			ReleaseDC(0, mExternalDC[i->first]);
			DestroyWindow(i->second);
		}
		mExternalWindows.clear();
		mExternalDC.clear();
		mUnloadable = true;
	} else {
		for (size_t i = mWindows.getChildCount() - 1; i >= 0 && ~i != 0; --i) {
			Control* c = mWindows.getChild(i);
			if (mExternalWindows.count(c) == 0) {
				int sx = GetSystemMetrics(SM_CXVIRTUALSCREEN);
				int sy = GetSystemMetrics(SM_CYVIRTUALSCREEN);
				HWND hwnd = CreateWindowEx(WS_EX_LAYERED, EXTERNAL_WINDOW_CLASS, c->text.c_str(), WS_POPUP, (int) (c->xF * sx), (int) (c->yF * sy), 0, 0, dll->ffxiv(), NULL, dll->instance(), 0);
				if (mConfig.UseExternalWindow)
					ShowWindow(hwnd, SW_SHOW);
				else
					ShowWindow(hwnd, SW_HIDE);
				mExternalWindows[c] = hwnd;
				mExternalDC[c] = GetDC(hwnd);
			}
		}
		for (auto i = mExternalWindows.begin(); i != mExternalWindows.end(); ) {
			if (std::find(mWindows.children[1].begin(), mWindows.children[1].end(), i->first) == mWindows.children[1].end()) {
				mExternalDC.erase(i->first);
				i = mExternalWindows.erase(i);
			} else {
				++i;
			}
		}
	}
}
LRESULT CALLBACK OverlayRenderer::ExternalWindowWndProcExternal(HWND hWnd, UINT message, WPARAM w, LPARAM l) {
	return ExternalRendererRef->ExternalWindowWndProc(hWnd, message, w, l);
}
LRESULT OverlayRenderer::ExternalWindowWndProc(HWND hWnd, UINT message, WPARAM w, LPARAM l) {
	switch (message) {
		case WM_SETFOCUS:
		case WM_ACTIVATE:
			SetForegroundWindow(dll->ffxiv());
			return 0;
		case WM_CLOSE:
			return 0;
		case WM_NCHITTEST:
		{
			LRESULT hit = DefWindowProc(hWnd, message, w, l);
			if (hit == HTCLIENT) hit = HTCAPTION;
			return hit;
		}
		case WM_ERASEBKGND:
		{
			mExternalLastUpdate = 0;
			return 0;
		}
	}
	return DefWindowProc(hWnd, message, w, l);
}

size_t OverlayRenderer::GetFPS() {
	return mFpsMeter.size();
}
void OverlayRenderer::SetUseDrawOverlay(bool b) {
	mConfig.UseDrawOverlay = b;
}
int OverlayRenderer::GetUseDrawOverlay() {
	return mConfig.UseDrawOverlay;
}

void OverlayRenderer::SetUseDrawOverlayEveryone(bool b) {
	mConfig.ShowEveryDPS = b;
}
int OverlayRenderer::GetUseDrawOverlayEveryone() {
	return mConfig.ShowEveryDPS;
}

OverlayRenderer::Control* OverlayRenderer::GetRoot() {
	return &mWindows;
}

void OverlayRenderer::AddWindow(Control* windows) {
	mWindows.addChild(windows);
}

WindowControllerBase* OverlayRenderer::GetWindowAt(Control *in, int x, int y) {
	std::lock_guard<std::recursive_mutex> lock(in->layoutLock);
	for (int i = 0; i < _CHILD_TYPE_COUNT; i++)
		for (auto it = in->children[i].rbegin(); it != in->children[i].rend(); ++it)
			if ((*it)->hittest(x, y)) {
				if ((*it)->hasCallback()) {
					WindowControllerBase *w = (WindowControllerBase*) *it;
					if (!w->isClickthrough())
						return w;
				}
			}
	return nullptr;
}

WindowControllerBase* OverlayRenderer::GetWindowAt(int x, int y) {
	return GetWindowAt(&mWindows, x, y);
}

void OverlayRenderer::RenderOverlayMisc(int w, int h){
	/*
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSizeConstraints(ImVec2(w, h), ImVec2(w, h));
	// maybe add something?
	ImGui::End();
	//*/
}

void OverlayRenderer::DrawOverlay() {
	mFpsMeter.push_back(GetTickCount64());
	uint64_t cur = GetTickCount64();
	while (!mFpsMeter.empty() && mFpsMeter.front() + 1000 < cur)
		mFpsMeter.pop_front();

	CheckCapture();

	RenderExternalOverlay();
	RenderOverlay();
}

void OverlayRenderer::RenderExternalOverlay() {
	if (mConfig.UseExternalWindow && (Hooks::isFFXIVChatWindowOpen || !mConfig.ShowOnlyWhenChatWindowOpen)) {
		if (mExternalLastUpdate + 1000 / mConfig.ExternalWindowRefreshRate < GetTickCount64()) {
			mExternalLastUpdate = GetTickCount64();
			if (mExternalFont == nullptr) {
				TCHAR fontName[512];
				MultiByteToWideChar(CP_UTF8, 0, mConfig.fontName, -1, fontName, 512);
				mExternalFont = CreateFont(mConfig.fontSize, 0, 0, 0, mConfig.bold ? FW_BOLD : FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, VARIABLE_PITCH | FF_ROMAN, fontName);
			}
			mUseDefaultRenderer = true;
			mWindows.width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
			mWindows.height = GetSystemMetrics(SM_CYVIRTUALSCREEN);
			RECT rect = { 0, 0, mWindows.width, mWindows.height };
			mWindows.measure(this, rect, rect.right - rect.left, rect.bottom - rect.top, false);
			for (auto it = mWindows.children[1].begin(); it != mWindows.children[1].end(); ++it) {
				if ((*it)->relativeSize) {
					(*it)->xF = min(1 - (*it)->calcWidth / (*it)->getParent()->width, max(0, (*it)->xF));
					(*it)->yF = min(1 - (*it)->calcHeight / (*it)->getParent()->height, max(0, (*it)->yF));
				}
			}
			for (auto i = mWindows.children[1].begin(); i != mWindows.children[1].end(); ++i) {
				if (((WindowControllerBase*) *i)->isLocked())
					SetWindowLong(mExternalWindows[mExternalCurrent], GWL_EXSTYLE, GetWindowLong(mExternalWindows[mExternalCurrent], GWL_EXSTYLE) | WS_EX_TRANSPARENT);
				else
					SetWindowLong(mExternalWindows[mExternalCurrent], GWL_EXSTYLE, GetWindowLong(mExternalWindows[mExternalCurrent], GWL_EXSTYLE) & ~WS_EX_TRANSPARENT);
				mExternalCurrent = *i;
				mExternalTemporaryDC = CreateCompatibleDC(mExternalDC[*i]);
				HBITMAP hbmMem = CreateCompatibleBitmap(mExternalDC[*i], (*i)->calcWidth, (*i)->calcHeight);
				HGDIOBJ hbmOld = SelectObject(mExternalTemporaryDC, hbmMem);

				SetLayeredWindowAttributes(mExternalWindows[mExternalCurrent], -1, (BYTE) mConfig.transparency, LWA_ALPHA);
				RECT rt;
				POINT pt = { 0 };
				SetWindowPos(mExternalWindows[mExternalCurrent], 0, 0, 0, (*i)->calcWidth, (*i)->calcHeight, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOREDRAW);
				GetWindowRect(mExternalWindows[mExternalCurrent], &rt);

				mExternalCurrent->xF = (float) (rt.left) / mWindows.width;
				mExternalCurrent->yF = (float) (rt.top) / mWindows.height;
				SelectObject(mExternalTemporaryDC, mExternalFont);
				SelectObject(mExternalDC[mExternalCurrent], mExternalFont);
				mWindows.draw(this);
				BitBlt(mExternalDC[mExternalCurrent], 0, 0, (*i)->calcWidth, (*i)->calcHeight, mExternalTemporaryDC, 0, 0, SRCCOPY);

				SelectObject(mExternalTemporaryDC, hbmOld);
				DeleteObject(hbmMem);
				DeleteDC(mExternalTemporaryDC);
			}
			mUseDefaultRenderer = false;
		}
		for (auto i = mExternalWindows.begin(); i != mExternalWindows.end(); ++i)
			ShowWindow(i->second, SW_SHOWNOACTIVATE);
	} else {
		for (auto i = mExternalWindows.begin(); i != mExternalWindows.end(); ++i)
			ShowWindow(i->second, SW_HIDE);
	}
}

void OverlayRenderer::ReloadFromConfig() {
	for (auto i = mExternalDC.begin(); i != mExternalDC.end(); ++i)
		SelectObject(i->second, GetStockObject(ANSI_VAR_FONT));
	DeleteObject(mExternalFont);
	mExternalFont = nullptr;
}

void OverlayRenderer::DrawTexture(int x, int y, int w, int h, PDXTEXTURETYPE tex) {

}
void OverlayRenderer::DrawBox(int x, int y, int w, int h, DWORD Color) {
	HGDIOBJ old = SelectObject(mExternalTemporaryDC, CreateSolidBrush(Color & 0xFFFFFF));
	Rectangle(mExternalTemporaryDC, x - mExternalCurrent->calcX, y - mExternalCurrent->calcY, x - mExternalCurrent->calcX + w, y - mExternalCurrent->calcY + h);
	DeleteObject(SelectObject(mExternalTemporaryDC, old));
}
void OverlayRenderer::MeasureText(RECT &rc, TCHAR *text, int flags) {
	::DrawText(mExternalDC[mExternalCurrent], text, -1, &rc, flags | DT_CALCRECT);
}
void OverlayRenderer::DrawText(int x, int y, TCHAR *text, DWORD Color) {
	RECT rc = { x - mExternalCurrent->calcX, y - mExternalCurrent->calcY, -1, -1 };
	SetTextColor(mExternalTemporaryDC, Color & 0xFFFFFF);
	SetBkMode(mExternalTemporaryDC, TRANSPARENT);
	::DrawText(mExternalTemporaryDC, text, -1, &rc, DT_NOCLIP);
}
void OverlayRenderer::DrawText(int x, int y, int width, int height, TCHAR *text, DWORD Color, int align) {
	RECT rc = { x - mExternalCurrent->calcX, y - mExternalCurrent->calcY, x + width - mExternalCurrent->calcX, y + height - mExternalCurrent->calcY };
	SetTextColor(mExternalTemporaryDC, Color & 0xFFFFFF);
	SetBkMode(mExternalTemporaryDC, TRANSPARENT);
	::DrawText(mExternalTemporaryDC, text, -1, &rc, align);
}


void OverlayRenderer::CaptureScreen() {
	mDoCapture = true;
}