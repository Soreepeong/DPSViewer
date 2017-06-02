#include"stdafx.h"
#include "OverlayRenderer.h"
#include "WindowControllerBase.h"
#include <psapi.h>
#include <shlobj.h>
#include <Shlwapi.h>
#include "Tools.h"
#include "FFXIVDLL.h"
#pragma comment(lib, "Shlwapi.lib")

OverlayRenderer::OverlayRenderer(FFXIVDLL *dll, IDirect3DDevice9* device) :
	pDevice(device),
	mFont(nullptr),
	mSprite(nullptr),
	dll (dll),
	hSaverThread(INVALID_HANDLE_VALUE) {

	TCHAR path[256];
	int i = 0, h = 0;
	GetModuleFileNameEx(GetCurrentProcess(), NULL, path, MAX_PATH);
	for (i = 0; i < 128 && path[i * 2] && path[i * 2 + 1]; i++)
		h ^= ((int*)path)[i];
	ExpandEnvironmentStrings(L"%APPDATA%", path, MAX_PATH);
	wsprintf(path + wcslen(path), L"\\ffxiv_overlay_config_%d.txt", h);

	FILE *f = _wfopen(path, L"r");
	if (f != nullptr) {
		fwscanf(f, L"%d%d%d%d%d%d%d",
			&mConfig.UseDrawOverlay,
			&mConfig.UseDrawOverlayEveryone,
			&mConfig.fontSize, &mConfig.bold,
			&mConfig.border, &mConfig.hideOtherUser,
			&mConfig.captureFormat);
		fwscanf(f, L"\n");
		fgetws(mConfig.fontName, sizeof(mConfig.fontName) / sizeof(mConfig.fontName[0]), f);
		fgetws(mConfig.capturePath, sizeof(mConfig.capturePath) / sizeof(mConfig.capturePath[0]), f);
		while (wcslen(mConfig.fontName) > 0 && mConfig.fontName[wcslen(mConfig.fontName) - 1] == '\n')
			mConfig.fontName[wcslen(mConfig.fontName) - 1] = 0;
		while (wcslen(mConfig.capturePath) > 0 && mConfig.capturePath[wcslen(mConfig.capturePath) - 1] == '\n')
			mConfig.capturePath[wcslen(mConfig.capturePath) - 1] = 0;
		mConfig.fontSize = max(9, min(128, mConfig.fontSize));
		mConfig.border = max(0, min(3, mConfig.border));
		switch (mConfig.captureFormat) {
		case D3DXIFF_BMP:
		case D3DXIFF_PNG:
		case D3DXIFF_JPG:
			break;
		default:
			mConfig.captureFormat = D3DXIFF_BMP;
		}
		fclose(f);
	}

	memset(mWindows.statusMap, 0, sizeof(mWindows.statusMap));
	mWindows.layoutDirection = LAYOUT_ABSOLUTE;

	ReloadResources();
}

OverlayRenderer::~OverlayRenderer() {
	if (mFont != nullptr) {
		mFont->Release();
		mFont = nullptr;
	}
	if (mSprite != nullptr) {
		mSprite->Release();
		mSprite = nullptr;
	}
	for (auto it = mResourceTextures.begin(); it != mResourceTextures.end(); it = mResourceTextures.erase(it))
		if(it->second != nullptr)
			it->second->Release();

	if (hSaverThread != INVALID_HANDLE_VALUE)
		WaitForSingleObject(hSaverThread, -1);

	TCHAR path[256];
	int i = 0, h = 0;
	GetModuleFileNameEx(GetCurrentProcess(), NULL, path, MAX_PATH);
	for (i = 0; i < 128 && path[i * 2] && path[i * 2 + 1]; i++)
		h ^= ((int*)path)[i];
	ExpandEnvironmentStrings(L"%APPDATA%", path, MAX_PATH);
	wsprintf(path + wcslen(path), L"\\ffxiv_overlay_config_%d.txt", h);

	FILE *f = _wfopen(path, L"w");
	if (f != nullptr) {
		fwprintf(f, L"%d %d %d %d %d %d %d\n",
			mConfig.UseDrawOverlay,
			mConfig.UseDrawOverlayEveryone,
			mConfig.fontSize, mConfig.bold,
			mConfig.border, mConfig.hideOtherUser,
			mConfig.captureFormat);
		fputws(mConfig.fontName, f);
		fwprintf(f, L"\n");
		fputws(mConfig.capturePath, f);
		fwprintf(f, L"\n");
		fclose(f);
	}
}

LPDIRECT3DTEXTURE9 OverlayRenderer::GetTextureFromResource(TCHAR *resName) {
	LPDIRECT3DTEXTURE9 tex = nullptr;
	if (mResourceTextures.find(resName) != mResourceTextures.end())
		return mResourceTextures[resName];
	D3DXCreateTextureFromResource(pDevice, dll->instance(), resName, &tex);
	mResourceTextures[resName] = tex;
	return tex;
}

LPDIRECT3DTEXTURE9 OverlayRenderer::GetTextureFromFile(TCHAR *resName) {
	LPDIRECT3DTEXTURE9 tex = nullptr;
	if (mFileTextures.find(resName) != mResourceTextures.end())
		return mFileTextures[resName];
	D3DXCreateTextureFromFile(pDevice, resName, &tex);
	mFileTextures[resName] = tex;
	return tex;
}

void OverlayRenderer::SetCapturePath(TCHAR *chr) {
	wcsncpy(mConfig.capturePath, chr, sizeof(mConfig.capturePath) / sizeof(mConfig.capturePath[0]));
	if (mConfig.capturePath[0] == 0) {
		TCHAR path[MAX_PATH];
		char path2[MAX_PATH * 4];
		SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, path);
		WideCharToMultiByte(CP_UTF8, 0, path, -1, path2, sizeof(path2), 0, 0);
		std::string s("/e Saving screenshots in ");
		s += path2;
		dll->pipe()->AddChat(s);
	} else {
		char path2[MAX_PATH * 4];
		WideCharToMultiByte(CP_UTF8, 0, chr, -1, path2, sizeof(path2), 0, 0);
		std::string s("/e Saving screenshots in ");
		s += path2;
		dll->pipe()->AddChat(s);
	}
}

void OverlayRenderer::GetCaptureFormat(int f) {
	mConfig.captureFormat = f;
}

void OverlayRenderer::ReloadResources() {
	std::lock_guard<std::recursive_mutex> lock(mWindows.layoutLock);
	if (mFont != nullptr) {
		mFont->Release();
		mFont = nullptr;
	}
	if (mSprite != nullptr) {
		mSprite->Release();
		mSprite = nullptr;
	}
	for (auto it = mResourceTextures.begin(); it != mResourceTextures.end(); it = mResourceTextures.erase(it))
		it->second->Release();

	D3DXCreateSprite(pDevice, &mSprite);
	D3DXCreateFont(pDevice, mConfig.fontSize, 0, (mConfig.bold ? FW_BOLD : 0), 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, mConfig.fontName, &mFont);
}

void OverlayRenderer::SetHideOtherUser() {
	mConfig.hideOtherUser = !mConfig.hideOtherUser;
}
int OverlayRenderer::GetHideOtherUser() {
	return mConfig.hideOtherUser;
}

void OverlayRenderer::SetFontSize(int size) {
	mConfig.fontSize = size;
	ReloadResources();
}
void OverlayRenderer::SetFontWeight() {
	mConfig.bold = !mConfig.bold;
	ReloadResources();
}
void OverlayRenderer::SetFontName(TCHAR *c) {
	wcsncpy(mConfig.fontName, c, sizeof(mConfig.fontName) / sizeof(mConfig.fontName[0]));
	ReloadResources();
}

void OverlayRenderer::SetBorder(int n) {
	mConfig.border = n;
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
	mConfig.UseDrawOverlayEveryone = b;
}
int OverlayRenderer::GetUseDrawOverlayEveryone() {
	return mConfig.UseDrawOverlayEveryone;
}

OverlayRenderer::Control* OverlayRenderer::GetRoot() {
	return &mWindows;
}


void OverlayRenderer::DrawBox(int x, int y, int w, int h, D3DCOLOR Color) {
	struct Vertex {
		float x, y, z, ht;
		DWORD Color;
	}
	V[4] = { { x, y + h, 0.0f, 0.0f, Color },{ x, y, 0.0f, 0.0f, Color },{ x + w, y + h, 0.0f, 0.0f, Color },{ x + w, y, 0.0f, 0.0f, Color } };
	pDevice->SetTexture(0, NULL);
	pDevice->SetPixelShader(0);
	pDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
	pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
	pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
	pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, V, sizeof(Vertex));
	return;
}

void OverlayRenderer::DrawText(int x, int y, TCHAR *text, D3DCOLOR Color) {
	if(mConfig.border)
		for (int i = -mConfig.border; i <= mConfig.border; i++)
			for (int j = -mConfig.border; j <= mConfig.border; j++)
				if (i != 0 && j != 0) {
					RECT rc4 = { x + i, y + j, 10000, 10000 };
					mFont->DrawTextW(NULL, text, -1, &rc4, DT_NOCLIP, (~Color & 0xFFFFFF) | 0xFF000000);
				}
	RECT rc = { x, y , 10000, 10000 };
	mFont->DrawTextW(NULL, text, -1, &rc, DT_NOCLIP, D3DCOLOR_ARGB(255, 255, 255, 255));
}

void OverlayRenderer::DrawText(int x, int y, int width, int height, TCHAR *text, D3DCOLOR Color, int align) {
	if(mConfig.border)
		for (int i = -mConfig.border; i <= mConfig.border; i++)
			for (int j = -mConfig.border; j <= mConfig.border; j++)
				if (i != 0 && j != 0) {
					RECT rc4 = { x + i, y + j, x + i + width, y + j + height };
					mFont->DrawTextW(NULL, text, -1, &rc4, DT_NOCLIP | align, (~Color & 0xFFFFFF) | 0xFF000000);
				}
	RECT rc = { x, y , x + width, y + height };
	mFont->DrawTextW(NULL, text, -1, &rc, DT_NOCLIP | align, D3DCOLOR_ARGB(255, 255, 255, 255));
}

void OverlayRenderer::DrawTexture(int x, int y, int w, int h, LPDIRECT3DTEXTURE9 tex) {
	D3DSURFACE_DESC desc;
	tex->GetLevelDesc(0, &desc);
	D3DXMATRIX matrix;
	D3DXVECTOR3 scale((float)w / desc.Width, (float)h / desc.Height, 1);
	D3DXVECTOR3 translate(x, y, 0);
	D3DXMatrixTransformation(&matrix, NULL, NULL, &scale, NULL, NULL, &translate);
	mSprite->Begin(D3DXSPRITE_ALPHABLEND);
	mSprite->SetTransform(&matrix);
	mSprite->Draw(tex, NULL, NULL, NULL, 0xFFFFFFFF);
	mSprite->End();
}

void OverlayRenderer::AddWindow(Control* windows) {
	mWindows.addChild(windows);
}

WindowControllerBase* OverlayRenderer::GetWindowAt(Control *in, int x, int y) {
	std::lock_guard<std::recursive_mutex> lock(in->layoutLock);
	for(int i = 0; i < _CHILD_TYPE_COUNT; i++)
		for (auto it = in->children[i].rbegin(); it != in->children[i].rend(); ++it)
			if ((*it)->hittest(x, y)) {
				if ((*it)->hasCallback()){
					WindowControllerBase *w = (WindowControllerBase*)*it;
					if(!w->isLocked())
						return w;
			}
	return nullptr;
}

WindowControllerBase* OverlayRenderer::GetWindowAt(int x, int y) {
	return GetWindowAt(&mWindows, x, y);
}

void OverlayRenderer::RenderOverlay() {

	if (mConfig.UseDrawOverlay && mFont != nullptr) {
		std::lock_guard<std::recursive_mutex> guard(mWindows.getLock());
		if (mFont == nullptr)
			D3DXCreateFont(pDevice, mConfig.fontSize, 0, (mConfig.bold ? FW_BOLD : 0), 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, mConfig.fontName, &mFont);

		D3DVIEWPORT9 prt;
		pDevice->GetViewport(&prt);
		RECT rect = { prt.X, prt.Y, prt.X+prt.Width, prt.Y+prt.Height };

		pDevice->BeginScene();

		mWindows.width = prt.Width;
		mWindows.height = prt.Height;
		mWindows.measure(this, rect, rect.right - rect.left, rect.bottom - rect.top, false);
		for (auto it = mWindows.children[0].begin(); it != mWindows.children[0].end(); ++it) {
			if ((*it)->relativeSize) {
				(*it)->xF = min(1 - (*it)->calcWidth / (*it)->getParent()->width, max(0, (*it)->xF));
				(*it)->yF = min(1 - (*it)->calcHeight / (*it)->getParent()->height, max(0, (*it)->yF));
			}
		}
		mWindows.draw(this);

		pDevice->EndScene();
	}
}

int a = 0;

void OverlayRenderer::DrawOverlay() {
	mFpsMeter.push_back(GetTickCount64());
	uint64_t cur = GetTickCount64();
	while (!mFpsMeter.empty() && mFpsMeter.front() + 1000 < cur)
		mFpsMeter.pop_front();

	CheckCapture();

	// __try {
	RenderOverlay();
	// } __except (EXCEPTION_EXECUTE_HANDLER) {}


}
void OverlayRenderer::CheckCapture() {
	if (mDoCapture) {
		HRESULT hr;
		IDirect3DSurface9 *renderTarget, *old = nullptr;
		hr = pDevice->GetRenderTarget(0, &renderTarget);
		if (!renderTarget || FAILED(hr))
			return;

		D3DSURFACE_DESC rtDesc;
		renderTarget->GetDesc(&rtDesc);

		IDirect3DSurface9* resolvedSurface = nullptr;
		if (rtDesc.MultiSampleType != D3DMULTISAMPLE_NONE) {
			hr = pDevice->CreateRenderTarget(rtDesc.Width, rtDesc.Height, rtDesc.Format, D3DMULTISAMPLE_NONE, 0, FALSE, &resolvedSurface, NULL);
			if (FAILED(hr))
				return;
			hr = pDevice->StretchRect(renderTarget, NULL, resolvedSurface, NULL, D3DTEXF_NONE);
			if (FAILED(hr))
				return;
			old = renderTarget;
			renderTarget = resolvedSurface;
		}

		IDirect3DSurface9* offscreenSurface;
		hr = pDevice->CreateOffscreenPlainSurface(rtDesc.Width, rtDesc.Height, rtDesc.Format, D3DPOOL_SYSTEMMEM, &offscreenSurface, NULL);
		if (FAILED(hr))
			return;
		hr = pDevice->GetRenderTargetData(renderTarget, offscreenSurface);

		CaptureBackgroundSave(offscreenSurface);

		renderTarget->Release();
		if (old != nullptr)
			old->Release();
		mDoCapture = false;
	}
}

void OverlayRenderer::CaptureBackgroundSave(IDirect3DSurface9 *buf) {
	std::lock_guard<std::recursive_mutex> lock(mCaptureMutex);
	mCaptureBuffers.push_back(buf);
	if (hSaverThread == INVALID_HANDLE_VALUE)
		hSaverThread = CreateThread(NULL, NULL, OverlayRenderer::CaptureBackgroundSaverExternal, this, NULL, NULL);

	dll->pipe()->AddChat("/e Saving screenshot...");
}

void OverlayRenderer::CaptureScreen() {
	mDoCapture = true;
}

void OverlayRenderer::CaptureBackgroundSaverThread() {
	IDirect3DSurface9 *surface;
	ID3DXBuffer *buf;
	while (mCaptureBuffers.size() > 0) {
		{
			std::lock_guard<std::recursive_mutex> lock(mCaptureMutex);
			surface = mCaptureBuffers.front();
			mCaptureBuffers.pop_front();
		}
		buf = nullptr;
		D3DXSaveSurfaceToFileInMemory(&buf, (D3DXIMAGE_FILEFORMAT)mConfig.captureFormat, surface, 0, 0);
		surface->Release();
		if (buf == nullptr)
			continue;
		TCHAR path[MAX_PATH];
		int i = 0;
		SYSTEMTIME lt;
		GetLocalTime(&lt);
		do {
			if (mConfig.capturePath[0] == 0 || !Tools::DirExists(mConfig.capturePath))
				SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, path);
			else
				wcscpy(path, mConfig.capturePath);
			wsprintf(path + wcslen(path), L"\\ffxiv_%04d%02d%02d_%02d%02d%02d_%d.%s",
				lt.wYear, lt.wMonth, lt.wDay, lt.wHour, lt.wMinute, lt.wSecond,
				i,
				mConfig.captureFormat == D3DXIFF_BMP ? L"bmp" :
				mConfig.captureFormat == D3DXIFF_PNG ? L"png" :
				mConfig.captureFormat == D3DXIFF_JPG ? L"jpg" : L".unk");
		} while (PathFileExists(path) && i++ < 16);
		if (i < 16) {
			HANDLE h = CreateFile(path, GENERIC_WRITE, NULL, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
			if (h == INVALID_HANDLE_VALUE) {
				LPSTR messageBuffer = nullptr;
				int err = GetLastError();
				size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

				std::string message(messageBuffer, size);

				//Free the buffer.
				LocalFree(messageBuffer);
				message = "/e Capture error: " + message + " (";
				message += err;
				message += ")";
				dll->pipe()->AddChat(message);
			} else {
				DWORD wr;
				WriteFile(h, buf->GetBufferPointer(), buf->GetBufferSize(), &wr, NULL);
				CloseHandle(h);
				std::string message("/e Screenshot saved in ");
				char path2[MAX_PATH * 4];
				WideCharToMultiByte(CP_UTF8, 0, path, -1, path2, sizeof(path2), 0, 0);
				message += path2;
				dll->pipe()->AddChat(message);
			}
		}
		buf->Release();
	}
	{
		std::lock_guard<std::recursive_mutex> lock(mCaptureMutex);
		CloseHandle(hSaverThread);
		hSaverThread = INVALID_HANDLE_VALUE;
	}
	if (dll->isUnloading())
		TerminateThread(GetCurrentThread(), 0);
}