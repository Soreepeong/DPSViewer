#include "OverlayRenderer.h"
#include "WindowControllerBase.h"
#include <psapi.h>
#include "Tools.h"
#include "FFXIVDLL.h"
#include "resource.h"
#include "imgui/imgui.h"

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
					ImGui::GetIO().Fonts->AddFontFromMemoryTTF(res, dwResourceSize, 17, 0, ImGui::GetIO().Fonts->GetGlyphRangesKorean());
				}
			}
		}
	}
}

OverlayRenderer::~OverlayRenderer() {
	if (hSaverThread != INVALID_HANDLE_VALUE)
		WaitForSingleObject(hSaverThread, -1);
}

void OverlayRenderer::GetCaptureFormat(int f) {
	mConfig.captureFormat = f;
}

void OverlayRenderer::SetHideOtherUserName() {
	mConfig.hideOtherUserName = !mConfig.hideOtherUserName;
}
int OverlayRenderer::GetHideOtherUserName() {
	return mConfig.hideOtherUserName;
}

int OverlayRenderer::GetFPS() {
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
					if (!w->isLocked())
						return w;
				}
			}
	return nullptr;
}

WindowControllerBase* OverlayRenderer::GetWindowAt(int x, int y) {
	return GetWindowAt(&mWindows, x, y);
}

int a = 0;

void OverlayRenderer::DrawOverlay() {
	mFpsMeter.push_back(GetTickCount64());
	uint64_t cur = GetTickCount64();
	while (!mFpsMeter.empty() && mFpsMeter.front() + 1000 < cur)
		mFpsMeter.pop_front();

	CheckCapture();

	__try {
		RenderOverlay();
	} __except (EXCEPTION_EXECUTE_HANDLER) {}


}

void OverlayRenderer::CaptureScreen() {
	mDoCapture = true;
}