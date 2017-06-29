#include "OverlayRenderer_DX11.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx11.h"
#include "FFXIVDLL.h"
#include "Hooks.h"
#include "DX11_TextureFromImage.h"
#include <shlobj.h>
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
#include "DX11_ScreenGrab.h"
#pragma warning(push)
#pragma warning(disable : 4005)
#include <wincodec.h>
#pragma warning(pop)

OverlayRendererDX11::OverlayRendererDX11(FFXIVDLL *dll, ID3D11Device* device, ID3D11DeviceContext *context, IDXGISwapChain *sc) :
	OverlayRenderer(dll),
	pDevice(device),
	pContext(context),
	pFW1Factory(nullptr),
	pFW(nullptr),
	pSwapChain(sc) {
	ImGui_ImplDX11_Init(dll->ffxiv(), device, context);
	FW1CreateFactory(FW1_VERSION, &pFW1Factory);
	ReloadFromConfig();
}

OverlayRendererDX11::~OverlayRendererDX11() {
	OnLostDevice();
	pFW1Factory->Release();
	ImGui_ImplDX11_Shutdown();
}

PDXTEXTURETYPE OverlayRendererDX11::GetTextureFromResource(TCHAR *resName) {
	PDXTEXTURETYPE tex = nullptr;

	if (mResourceTextures.find(resName) != mResourceTextures.end())
		return mResourceTextures[resName];

	HRSRC hResource = FindResource(dll->instance(), resName, RT_RCDATA);
	if (hResource) {
		HGLOBAL hLoadedResource = LoadResource(dll->instance(), hResource);
		if (hLoadedResource) {
			LPVOID pLockedResource = LockResource(hLoadedResource);
			if (pLockedResource) {
				DWORD dwResourceSize = SizeofResource(dll->instance(), hResource);
				if (0 != dwResourceSize) {
					CreateWICTextureFromMemory(pDevice, pContext, (uint8_t*)pLockedResource, dwResourceSize, nullptr, &tex);
				}
			}
		}
	}

	mResourceTextures[resName] = tex;

	return tex;
}

PDXTEXTURETYPE OverlayRendererDX11::GetTextureFromFile(TCHAR *resName) {
	PDXTEXTURETYPE tex = nullptr;

	if (mFileTextures.find(resName) != mResourceTextures.end())
		return mFileTextures[resName];
	
	CreateWICTextureFromFile(pDevice, pContext, resName, nullptr, &tex);

	mFileTextures[resName] = tex;

	return tex;
}

void OverlayRendererDX11::ReloadFromConfig() {
	OverlayRenderer::ReloadFromConfig();
	std::lock_guard<std::recursive_mutex> lock(mWindows.layoutLock);
	MultiByteToWideChar(CP_UTF8, 0, mConfig.fontName, -1, mFontName, 512);
	FW1_FONTWRAPPERCREATEPARAMS createParams;
	ZeroMemory(&createParams, sizeof(createParams));

	createParams.GlyphSheetWidth = 512;
	createParams.GlyphSheetHeight = 512;
	createParams.MaxGlyphCountPerSheet = 2048;
	createParams.SheetMipLevels = 1;
	createParams.AnisotropicFiltering = FALSE;
	createParams.MaxGlyphWidth = 384;
	createParams.MaxGlyphHeight = 384;
	createParams.DisableGeometryShader = FALSE;
	createParams.VertexBufferSize = 0;
	createParams.DefaultFontParams.pszFontFamily = mFontName;
	createParams.DefaultFontParams.FontWeight = mConfig.bold ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL;
	createParams.DefaultFontParams.FontStyle = DWRITE_FONT_STYLE_NORMAL;
	createParams.DefaultFontParams.FontStretch = DWRITE_FONT_STRETCH_NORMAL;
	createParams.DefaultFontParams.pszLocale = L"";
	pFW1Factory->CreateFontWrapper(pDevice, nullptr, &createParams, &pFW);
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


	FW1_FONTWRAPPERCREATEPARAMS createParams;
	ZeroMemory(&createParams, sizeof(createParams));

	createParams.GlyphSheetWidth = 512;
	createParams.GlyphSheetHeight = 512;
	createParams.MaxGlyphCountPerSheet = 2048;
	createParams.SheetMipLevels = 1;
	createParams.AnisotropicFiltering = FALSE;
	createParams.MaxGlyphWidth = 384;
	createParams.MaxGlyphHeight = 384;
	createParams.DisableGeometryShader = FALSE;
	createParams.VertexBufferSize = 0;
	createParams.DefaultFontParams.pszFontFamily = mFontName;
	createParams.DefaultFontParams.FontWeight = mConfig.bold ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL;
	createParams.DefaultFontParams.FontStyle = DWRITE_FONT_STYLE_NORMAL;
	createParams.DefaultFontParams.FontStretch = DWRITE_FONT_STRETCH_NORMAL;
	createParams.DefaultFontParams.pszLocale = L"";
	pFW1Factory->CreateFontWrapper(pDevice, nullptr, &createParams, &pFW);
}

void OverlayRendererDX11::DrawBox(int x, int y, int w, int h, DWORD Color) {
	if (mUseDefaultRenderer) {
		OverlayRenderer::DrawBox(x, y, w, h, Color);
		return;
	}
#pragma warning (push)
#pragma warning (disable: 4244)
	ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), Color);
#pragma warning (pop)
}

std::vector<OverlayRendererDX11::RenderInfo> OverlayRendererDX11::textRenderList;

void OverlayRendererDX11::TextDrawCallbackExternal(const ImDrawList* parent_list, const ImDrawCmd* cmd) {
	OverlayRendererDX11::RenderInfo ri = OverlayRendererDX11::textRenderList[reinterpret_cast<size_t>(cmd->UserCallbackData)];
	ri.rend->TextDrawCallback(ri);
}
void OverlayRendererDX11::TextDrawCallback(RenderInfo& ri) {
#pragma warning (push)
#pragma warning (disable: 4244 4838)
	if(ri.w == -1)
		pFW->DrawString(pContext, ri.text, mConfig.fontSize, ri.x, ri.y, ri.color, ri.flags | FW1_RESTORESTATE);
	else {
		FW1_RECTF rf = { ri.x, ri.y, ri.x + ri.w, ri.y + ri.h };
		pFW->DrawString(pContext, ri.text, mFontName, mConfig.fontSize, &rf, ri.color, &rf, NULL, ri.flags | FW1_RESTORESTATE | FW1_CLIPRECT | FW1_NOWORDWRAP);
	}
#pragma warning (pop)
}


void OverlayRendererDX11::DrawText(int x, int y, TCHAR *text, DWORD Color) {
	if (mUseDefaultRenderer) {
		OverlayRenderer::DrawText(x, y, text, Color);
		return;
	}
	RenderInfo rnd = { this, text, x, y, -1, -1, 0, Color };
	ImGui::GetWindowDrawList()->AddCallback(TextDrawCallbackExternal, (PVOID) textRenderList.size());
	textRenderList.push_back(rnd);
}

void OverlayRendererDX11::DrawText(int x, int y, int width, int height, TCHAR *text, DWORD Color, int align) {
	if (mUseDefaultRenderer) {
		OverlayRenderer::DrawText(x, y, width, height, text, Color, align);
		return;
	}
	RenderInfo rnd = { this, text, x, y, width, height, align, Color };
	ImGui::GetWindowDrawList()->AddCallback(TextDrawCallbackExternal, (PVOID) textRenderList.size());
	textRenderList.push_back(rnd);
}

void OverlayRendererDX11::DrawTexture(int x, int y, int w, int h, PDXTEXTURETYPE tex) {
	if (mUseDefaultRenderer) {
		OverlayRenderer::DrawTexture(x, y, w, h, tex);
		return;
	}
#pragma warning (push)
#pragma warning (disable: 4244)
	ImGui::GetWindowDrawList()->AddImage((ImTextureID) tex, ImVec2(x, y), ImVec2(x + w, y + h));
#pragma warning (pop)
}
void OverlayRendererDX11::MeasureText(RECT &rc, TCHAR *text, int flags) {
	if (mUseDefaultRenderer) {
		OverlayRenderer::MeasureText(rc, text, flags);
		return;
	}
#pragma warning (push)
#pragma warning (disable: 4244 4838)
	FW1_RECTF rf = { rc.left, rc.top, rc.right, rc.bottom }, rf2;
	if (rf.Right - rf.Left <= 0)
		rf.Right = rf.Left + 2000;
	if (rf.Bottom - rf.Top <= 0)
		rf.Bottom = rf.Top + 2000;
	// Modified function - it uses GetMetrics instead of GetOverhangMetrics
	rf2 = pFW->MeasureString(text, mFontName, mConfig.fontSize, &rf, flags);
#pragma warning (pop)
	rc = { rc.left, rc.top, (int) rf2.Right + 2, (int) rf2.Bottom };
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
#pragma warning (push)
#pragma warning (disable: 4244 4838)
		D3D11_VIEWPORT viewportf = {0, 0, desc.Width, desc.Height, 0, 1};
#pragma warning (pop)

		ImGui_ImplDX11_NewFrame();
		textRenderList.clear();
		RenderOverlayMisc(rect.right - rect.left, rect.bottom - rect.top);

		if ((Hooks::isFFXIVChatWindowOpen || !mConfig.ShowOnlyWhenChatWindowOpen) && !mConfig.UseExternalWindow) {
			mWindows.width = rect.right - rect.left;
			mWindows.height = rect.bottom - rect.top;
			mWindows.measure(this, rect, rect.right - rect.left, rect.bottom - rect.top, false);
			for (auto it = mWindows.children[1].begin(); it != mWindows.children[1].end(); ++it) {
				if ((*it)->relativeSize) {
					(*it)->xF = min(1 - (*it)->calcWidth / (*it)->getParent()->width, max(0, (*it)->xF));
					(*it)->yF = min(1 - (*it)->calcHeight / (*it)->getParent()->height, max(0, (*it)->yF));
				}
			}
#pragma warning (push)
#pragma warning (disable: 4244 4838)
			ImGui::SetNextWindowPos(ImVec2(0, 0));
			ImGui::SetNextWindowSizeConstraints(ImVec2(rect.right - rect.left, rect.bottom - rect.top), ImVec2(rect.right - rect.left, rect.bottom - rect.top));
#pragma warning (pop)
			bool open = true;
			ImGui::Begin("CustomWindowOverlay", &open, ImVec2(0, 0), 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs);
			mWindows.draw(this);
			ImGui::End();
		}
		mConfig.Render();
		ImGui::Render();
		pFW->Flush(pContext);

		backbuffer->Release();
	}
}

void OverlayRendererDX11::CheckCapture() {
	if (mDoCapture) {
		ID3D11Texture2D *pSurface;
		if (S_OK == pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pSurface))) {
			std::lock_guard<std::recursive_mutex> lock(mCaptureMutex);
			mCaptureBuffers.push_back(pSurface);
			dll->addChat("/e Saving screenshot...");
			if (hSaverThread == INVALID_HANDLE_VALUE)
				hSaverThread = CreateThread(NULL, NULL, OverlayRendererDX11::CaptureBackgroundSaverExternal, this, NULL, NULL);
		}
		mDoCapture = false;
	}
}

void OverlayRendererDX11::CaptureBackgroundSaverThread() {
	ID3D11Texture2D *surface;
	while (mCaptureBuffers.size() > 0) {
		{
			std::lock_guard<std::recursive_mutex> lock(mCaptureMutex);
			surface = mCaptureBuffers.front();
			mCaptureBuffers.pop_front();
		}
		TCHAR path[MAX_PATH];
		int i = 0;
		SYSTEMTIME lt;
		GetLocalTime(&lt);

		TCHAR capturePathW[512];
		MultiByteToWideChar(CP_UTF8, NULL, mConfig.capturePath, -1, capturePathW, sizeof(capturePathW) / sizeof(capturePathW[0]));
		do {
			if (mConfig.capturePath[0] == 0 || !Tools::DirExists(capturePathW))
				SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, path);
			else
				wcscpy(path, capturePathW);
			wsprintf(path + wcslen(path), L"\\ffxiv_%04d%02d%02d_%02d%02d%02d_%d.%s",
				lt.wYear, lt.wMonth, lt.wDay, lt.wHour, lt.wMinute, lt.wSecond,
				i,
				mConfig.captureFormat == D3DXIFF_BMP ? L"bmp" :
				mConfig.captureFormat == D3DXIFF_PNG ? L"png" :
				mConfig.captureFormat == D3DXIFF_JPG ? L"jpg" : L".unk");
		} while (PathFileExists(path) && i++ < 16);
		if (i < 16) {

			HRESULT hr;
			if (S_OK == (hr=DirectX::SaveWICTextureToFile(pContext, surface, mConfig.captureFormat == D3DXIFF_BMP ? GUID_ContainerFormatBmp :
				mConfig.captureFormat == D3DXIFF_PNG ? GUID_ContainerFormatPng : GUID_ContainerFormatJpeg, path))) {
				std::string message("/e Screenshot saved in ");
				char path2[MAX_PATH * 4];
				WideCharToMultiByte(CP_UTF8, 0, path, -1, path2, sizeof(path2), 0, 0);
				message += path2;
				dll->addChat(message);
			} else {
				char err[100];
				sprintf(err, "/e Capture error: %08X", (int) hr);
				dll->addChat(err);
			}
		}
		surface->Release();
	}
	{
		std::lock_guard<std::recursive_mutex> lock(mCaptureMutex);
		CloseHandle(hSaverThread);
		hSaverThread = INVALID_HANDLE_VALUE;
	}
	if (dll->isUnloading())
		TerminateThread(GetCurrentThread(), 0);
}



