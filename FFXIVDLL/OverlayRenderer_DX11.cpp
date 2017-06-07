#include "OverlayRenderer_DX11.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx11.h"
#include "FFXIVDLL.h"

OverlayRendererDX11::OverlayRendererDX11(FFXIVDLL *dll, ID3D11Device* device, ID3D11DeviceContext *context, IDXGISwapChain *sc) :
	OverlayRenderer(dll),
	pDevice(device),
	pContext(context),
	pFW1Factory(nullptr),
	pFW(nullptr),
	pSwapChain(sc) {
	ImGui_ImplDX11_Init(dll->ffxiv(), device, context);
	FW1CreateFactory(FW1_VERSION, &pFW1Factory);
	OnResetDevice();
}

OverlayRendererDX11::~OverlayRendererDX11() {
	OnLostDevice();
	pFW1Factory->Release();
	ImGui_ImplDX11_Shutdown();
}

PDXTEXTURETYPE OverlayRendererDX11::GetTextureFromResource(TCHAR *resName) {
	PDXTEXTURETYPE tex = nullptr;
	return tex;
}

PDXTEXTURETYPE OverlayRendererDX11::GetTextureFromFile(TCHAR *resName) {
	PDXTEXTURETYPE tex = nullptr;
	return tex;
}

void OverlayRendererDX11::ReloadFromConfig() {
	std::lock_guard<std::recursive_mutex> lock(mWindows.layoutLock);
}

void OverlayRendererDX11::OnLostDevice() {
	std::lock_guard<std::recursive_mutex> lock(mWindows.layoutLock);
	ImGui_ImplDX11_InvalidateDeviceObjects();
	if (pFW != nullptr) {
		pFW->Release();
		pFW = nullptr;
	}
}

void OverlayRendererDX11::OnResetDevice() {
	std::lock_guard<std::recursive_mutex> lock(mWindows.layoutLock);
	ImGui_ImplDX11_CreateDeviceObjects();

	MultiByteToWideChar(CP_UTF8, 0, mConfig.fontName, -1, mFontName, 512);
	pFW1Factory->CreateFontWrapper(pDevice, mFontName, &pFW);
}

void OverlayRendererDX11::DrawBox(int x, int y, int w, int h, DWORD Color) {
	ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), Color);
}

std::vector<OverlayRendererDX11::RenderInfo> OverlayRendererDX11::textRenderList;

void OverlayRendererDX11::TextDrawCallbackExternal(const ImDrawList* parent_list, const ImDrawCmd* cmd) {
	OverlayRendererDX11::RenderInfo ri = OverlayRendererDX11::textRenderList[(int) cmd->UserCallbackData];
	ri.rend->TextDrawCallback(ri);
}
void OverlayRendererDX11::TextDrawCallback(RenderInfo& ri) {
	if(ri.w == -1)
		pFW->DrawString(pContext, ri.text, mConfig.fontSize, ri.x, ri.y, ri.color, ri.flags);
	else {
		FW1_RECTF rf = { ri.x, ri.y, ri.x + ri.w, ri.y + ri.h };
		pFW->DrawString(pContext, ri.text, mFontName, mConfig.fontSize, &rf, ri.color, NULL, NULL, ri.flags);
	}
}


void OverlayRendererDX11::DrawText(int x, int y, TCHAR *text, DWORD Color) {
	RenderInfo rnd = { this, text, x, y, -1, -1, 0, Color };
	ImGui::GetWindowDrawList()->AddCallback(TextDrawCallbackExternal, (PVOID) textRenderList.size());
	textRenderList.push_back(rnd);
	/*
	char *utf8;
	int len = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, 0, 0);
	utf8 = new char[len];
	WideCharToMultiByte(CP_UTF8, 0, text, -1, utf8, len, 0, 0);
	ImGui::GetWindowDrawList()->AddText(ImVec2(x, y), Color, utf8);
	delete[] utf8;
	//*/
}

void OverlayRendererDX11::DrawText(int x, int y, int width, int height, TCHAR *text, DWORD Color, int align) {
	RenderInfo rnd = { this, text, x, y, width, height, align, Color };
	ImGui::GetWindowDrawList()->AddCallback(TextDrawCallbackExternal, (PVOID) textRenderList.size());
	textRenderList.push_back(rnd);
	/*
	char *utf8;
	int len = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, 0, 0);
	utf8 = new char[len];
	WideCharToMultiByte(CP_UTF8, 0, text, -1, utf8, len, 0, 0);
	ImVec4 clip_rect(x, y, x + width, y + height);
	ImGui::GetWindowDrawList()->AddText(ImVec2(x, y), Color, utf8);
	ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(), ImGui::GetFontSize(), 
		ImVec2(x, y), Color, utf8, NULL, 0.0f, &clip_rect);
	delete[] utf8;
	//*/
}

void OverlayRendererDX11::DrawTexture(int x, int y, int w, int h, PDXTEXTURETYPE tex) {

}
void OverlayRendererDX11::MeasureText(RECT &rc, TCHAR *text, int flags) {
	FW1_RECTF rf = { rc.left, rc.top, rc.right, rc.bottom }, rf2;
	if (rf.Right - rf.Left <= 0)
		rf.Right = rf.Left + 240;
	if (rf.Bottom - rf.Top <= 0)
		rf.Bottom = rf.Top + 240;
	rf2 = pFW->MeasureString(text, mFontName, mConfig.fontSize, &rf, flags);
	rc = { rc.left, rc.top,
		(LONG) (rf.Right - rf.Left + rf2.Right - rf2.Left),
		(LONG) (rf.Bottom - rf.Top + rf2.Bottom - rf2.Top) };
	/*
	char *utf8;
	int len = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, 0, 0);
	utf8 = new char[len];
	WideCharToMultiByte(CP_UTF8, 0, text, -1, utf8, len, 0, 0);
	ImVec2 res = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, (flags & DT_WORDBREAK) ? rc.right - rc.left : 0, utf8);
	delete[] utf8;
	rc.right = rc.left + res.x;
	rc.bottom = rc.top + res.y;
	//*/
}

void OverlayRendererDX11::RenderOverlay() {
	if (mConfig.UseDrawOverlay) {
		std::lock_guard<std::recursive_mutex> guard(mWindows.getLock());

		RECT rect;
		GetClientRect(dll->ffxiv(), &rect);

		ID3D11RenderTargetView *backbuffer;
		ID3D11Texture2D *pBackBuffer;
		pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*) &pBackBuffer);
		pDevice->CreateRenderTargetView(pBackBuffer, NULL, &backbuffer);
		pBackBuffer->Release();
		pContext->OMSetRenderTargets(1, &backbuffer, NULL);

		D3D11_TEXTURE2D_DESC desc;
		pBackBuffer->GetDesc(&desc);
		D3D11_VIEWPORT viewportf = {0, 0, desc.Width, desc.Height, 0, 1};

		ImGui_ImplDX11_NewFrame();
		textRenderList.clear();

		mWindows.width = rect.right - rect.left;
		mWindows.height = rect.bottom - rect.top;
		mWindows.measure(this, rect, rect.right - rect.left, rect.bottom - rect.top, false);
		for (auto it = mWindows.children[0].begin(); it != mWindows.children[0].end(); ++it) {
			if ((*it)->relativeSize) {
				(*it)->xF = min(1 - (*it)->calcWidth / (*it)->getParent()->width, max(0, (*it)->xF));
				(*it)->yF = min(1 - (*it)->calcHeight / (*it)->getParent()->height, max(0, (*it)->yF));
			}
		}
		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSizeConstraints(ImVec2(rect.right - rect.left, rect.bottom - rect.top), ImVec2(rect.right - rect.left, rect.bottom - rect.top));
		bool open = true;
		ImGui::Begin("CustomWindowOverlay", &open, ImVec2(0, 0), 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs);
		mWindows.draw(this);
		ImGui::Text("%d", textRenderList.size());
		ImGui::End();
		mConfig.Render();
		ImGui::Render();
		pFW->Flush(pContext);

		backbuffer->Release();
	}
}

void OverlayRendererDX11::CheckCapture() {
	if (mDoCapture) {

	}
}

/*
void OverlayRendererDX11::CaptureBackgroundSave(IDirect3DSurface9 *buf) {
	std::lock_guard<std::recursive_mutex> lock(mCaptureMutex);
	mCaptureBuffers.push_back(buf);
	if (hSaverThread == INVALID_HANDLE_VALUE)
		hSaverThread = CreateThread(NULL, NULL, OverlayRendererDX11::CaptureBackgroundSaverExternal, this, NULL, NULL);

	dll->pipe()->AddChat("/e Saving screenshot...");
}
//*/

void OverlayRendererDX11::CaptureBackgroundSaverThread() {
}