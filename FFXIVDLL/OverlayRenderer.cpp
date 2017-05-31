#include"stdafx.h"
#include "OverlayRenderer.h"
#include <psapi.h>
#include <shlobj.h>
#include <Shlwapi.h>
#include "Tools.h"
#include "resource.h"
#pragma comment(lib, "Shlwapi.lib")

BOOL CALLBACK OverlayRenderer::EnumResNameProc(HMODULE hModule, LPCTSTR lpszType, LPTSTR lpszName) {
	LPDIRECT3DTEXTURE9 tex;
	D3DXCreateTextureFromResource(pDevice, ffxivDll->instance(), lpszName, &tex);
	if (wcsncmp(lpszName, L"CLASS_", 6) == 0)
		mClassIcons[lpszName + 6] = tex;
	else
		tex->Release();
	return true;
}


OverlayRenderer::OverlayRenderer(IDirect3DDevice9* device) :
	pDevice(device),
	mFont(nullptr),
	mSprite(nullptr),
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
		fwscanf(f, L"%d%d%f%f%f%f%d%d%d%d%d%d",
			&mConfig.UseDrawOverlay,
			&mConfig.UseDrawOverlayEveryone,
			&mConfig.x, &mConfig.y,
			&mConfig.xdot, &mConfig.ydot,
			&mConfig.fontSize, &mConfig.bold,
			&mConfig.border, &mConfig.hideOtherUser,
			&mConfig.captureFormat,
			&mConfig.backgroundTransparency);
		fwscanf(f, L"\n");
		fgetws(mConfig.fontName, sizeof(mConfig.fontName) / sizeof(mConfig.fontName[0]), f);
		fgetws(mConfig.capturePath, sizeof(mConfig.capturePath) / sizeof(mConfig.capturePath[0]), f);
		while (wcslen(mConfig.fontName) > 0 && mConfig.fontName[wcslen(mConfig.fontName) - 1] == '\n')
			mConfig.fontName[wcslen(mConfig.fontName) - 1] = 0;
		while (wcslen(mConfig.capturePath) > 0 && mConfig.capturePath[wcslen(mConfig.capturePath) - 1] == '\n')
			mConfig.capturePath[wcslen(mConfig.capturePath) - 1] = 0;
		mConfig.x = max(0, min(mConfig.x, 1));
		mConfig.y = max(0, min(mConfig.y, 1));
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

	EnumResourceNames(ffxivDll->instance(), MAKEINTRESOURCE(RT_RCDATA), OverlayRenderer::EnumResNameProcExternal, (LONG_PTR) this);

	HRSRC hResource = FindResource(ffxivDll->instance(), MAKEINTRESOURCE(IDR_CLASS_COLORDEF), L"TEXT");
	if (hResource) {
		HGLOBAL hLoadedResource = LoadResource(ffxivDll->instance(), hResource);
		if (hLoadedResource) {
			LPVOID pLockedResource = LockResource(hLoadedResource);
			if (pLockedResource) {
				DWORD dwResourceSize = SizeofResource(ffxivDll->instance(), hResource);
				if (0 != dwResourceSize) {
					struct membuf : std::streambuf {
						membuf(char* base, std::ptrdiff_t n) {
							this->setg(base, base, base + n);
						}
					} sbuf((char*)pLockedResource, dwResourceSize);
					std::istream in(&sbuf);
					std::string job;
					TCHAR buf[64];
					int r, g, b;
					while (in >> job >> r >> g >> b) {
						std::transform(job.begin(), job.end(), job.begin(), ::toupper);
						MultiByteToWideChar(CP_UTF8, 0, job.c_str(), -1, buf, sizeof(buf) / sizeof(TCHAR));
						mClassColors[buf] = D3DCOLOR_XRGB(r, g, b);
					}
				}
			}
		}
	}

	ReloadResources();
}

OverlayRenderer::~OverlayRenderer() {
	if (mFont != nullptr) {
		mFont->Release();
		mFont = nullptr;
	}
	for (auto it = mClassIcons.begin(); it != mClassIcons.end(); it = mClassIcons.erase(it))
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
		fwprintf(f, L"%d %d %f %f %f %f %d %d %d %d %d %d\n",
			mConfig.UseDrawOverlay,
			mConfig.UseDrawOverlayEveryone,
			mConfig.x, mConfig.y,
			mConfig.xdot, mConfig.ydot,
			mConfig.fontSize, mConfig.bold,
			mConfig.border, mConfig.hideOtherUser,
			mConfig.captureFormat,
			mConfig.backgroundTransparency);
		fputws(mConfig.fontName, f);
		fwprintf(f, L"\n");
		fputws(mConfig.capturePath, f);
		fwprintf(f, L"\n");
		fclose(f);
	}
}

void OverlayRenderer::SetTable(std::vector<OVERLAY_RENDER_TABLE_ROW> &tbl) {
	std::lock_guard<std::mutex> lock(mDrawLock);
	mTableRows = tbl;
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
		ffxivDll->pipe()->AddChat(s);
	} else {
		char path2[MAX_PATH * 4];
		WideCharToMultiByte(CP_UTF8, 0, chr, -1, path2, sizeof(path2), 0, 0);
		std::string s("/e Saving screenshots in ");
		s += path2;
		ffxivDll->pipe()->AddChat(s);
	}
}

void OverlayRenderer::SetBackgroundTransparency(int t) {
	mConfig.backgroundTransparency = t;
}

void OverlayRenderer::GetCaptureFormat(int f) {
	mConfig.captureFormat = f;
}

void OverlayRenderer::ReloadResources() {
	std::lock_guard<std::mutex> lock(mDrawLock);
	if (mFont != nullptr) {
		mFont->Release();
		mFont = nullptr;
	}
	if (mSprite != nullptr) {
		mSprite->Release();
		mSprite = nullptr;
	}
	D3DXCreateSprite(pDevice, &mSprite);
	D3DXCreateFont(pDevice, mConfig.fontSize, 0, (mConfig.bold ? FW_BOLD : 0), 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, mConfig.fontName, &mFont);
}

void OverlayRenderer::SetHideOtherUser() {
	mConfig.hideOtherUser = !mConfig.hideOtherUser;
}
int OverlayRenderer::GetHideOtherUser() {
	return mConfig.hideOtherUser;
}

void OverlayRenderer::SetDpsBoxLocation(float x, float y) {
	mConfig.x = x;
	mConfig.y = y;
}
void OverlayRenderer::SetDotBoxLocation(float x, float y) {
	mConfig.xdot = x;
	mConfig.ydot = y;
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

void OverlayRenderer::SetText(TCHAR *t) {
	std::lock_guard<std::mutex> lock(mDrawLock);
	wcscpy(mDebugText, t);
}

void OverlayRenderer::SetDoTText(TCHAR *t) {
	std::lock_guard<std::mutex> lock(mDrawLock);
	wcscpy(mDotText, t);
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

void OverlayRenderer::DrawTexture(int x, int y, int w, int h, LPDIRECT3DTEXTURE9 tex) {
	D3DSURFACE_DESC desc;
	tex->GetLevelDesc(0, &desc);
	D3DXMATRIX matrix;
	D3DXVECTOR3 scale((float)w / desc.Width, (float)h / desc.Height, 1);
	D3DXVECTOR3 translate(x, y, 0);
	D3DXMatrixTransformation(&matrix, NULL, NULL, &scale, NULL, NULL, &translate);
	mSprite->SetTransform(&matrix);
	mSprite->Draw(tex, NULL, NULL, NULL, 0xFFFFFFFF);
}

void OverlayRenderer::RenderDpsBox(D3DVIEWPORT9 &prt) {
	RECT rc2 = { (int)(mConfig.x * prt.Width) , (int)(mConfig.y * prt.Height),prt.Width, prt.Height };
	RECT lineHeightTset;
	int lineHeight;
	mFont->DrawTextW(NULL, L"Dummy", -1, &lineHeightTset, DT_CALCRECT, D3DCOLOR_ARGB(255, 255, 255, 255));
	lineHeight = lineHeightTset.bottom - lineHeightTset.top;
	mFont->DrawTextW(NULL, mDebugText, -1, &rc2, DT_CALCRECT, D3DCOLOR_ARGB(255, 255, 255, 255));
	if (mConfig.border == 0) {
		DrawBox(rc2.left - 4, rc2.top - 4, rc2.right - rc2.left + 8, rc2.bottom - rc2.top + 8, mConfig.backgroundTransparency << 24);
	}
	DrawText(rc2.left, rc2.top, mDebugText, D3DCOLOR_ARGB(255, 255, 255, 255));

	int boxBaseX = rc2.left - 4;
	int boxBaseY = rc2.bottom + 4;
	int cols[13] = { 0 };
	int row = 0;
	for (auto it = mTableRows.begin(); it != mTableRows.end(); ++it) {
		for (int i = 0; i < it->count; i++) {
			mFont->DrawTextW(NULL, (TCHAR*)it->cols[i].c_str(), -1, &lineHeightTset, DT_CALCRECT, D3DCOLOR_ARGB(255, 255, 255, 255));
			cols[i + 1] = max(cols[i + 1], lineHeightTset.right - lineHeightTset.left);
		}
	}
	for (int i = 1; i < sizeof(cols) / sizeof(int); i++)
		cols[i] += cols[i - 1] + 8;
	mSprite->Begin(16);
	lineHeight += 8;
	int tableWidth = lineHeight + cols[8];
	DrawBox(boxBaseX, boxBaseY, tableWidth, lineHeight * mTableRows.size(), mConfig.backgroundTransparency << 24);
	for (auto it = mTableRows.begin(); it != mTableRows.end(); ++it, ++row) {
		int bw = (int)(tableWidth * it->barSize);
		if (bw > 0)
			DrawBox(boxBaseX, boxBaseY + row * lineHeight, bw, lineHeight, (mClassColors[it->icon] & 0xFFFFFF) | 0x80000000);
		if (mClassIcons.find(it->icon) != mClassIcons.end())
			DrawTexture(boxBaseX + 4, boxBaseY + 4 + row * lineHeight, lineHeight - 8, lineHeight - 8, mClassIcons[it->icon]);
		for (int i = 0; i < it->count; i++) {
			DrawText(boxBaseX + lineHeight + cols[i], boxBaseY + row * lineHeight + 4, (TCHAR*)it->cols[i].c_str(), D3DCOLOR_ARGB(255, 255, 255, 255));
		}
	}
	mSprite->End();
}

void OverlayRenderer::RenderDotBox(D3DVIEWPORT9 &prt) {
	RECT rc2 = { (int)(mConfig.xdot * prt.Width) , (int)(mConfig.ydot * prt.Height),prt.Width, prt.Height };
	mFont->DrawTextW(NULL, mDotText, -1, &rc2, DT_CALCRECT, D3DCOLOR_ARGB(255, 255, 255, 255));
	if (mConfig.border == 0) {
		DrawBox(rc2.left - 4, rc2.top - 4, rc2.right - rc2.left + 8, rc2.bottom - rc2.top + 8, mConfig.backgroundTransparency << 24);
	}
	DrawText(rc2.left, rc2.top, mDotText, D3DCOLOR_ARGB(255, 255, 255, 255));
}

void OverlayRenderer::RenderOverlay() {

	if (mConfig.UseDrawOverlay && mFont != nullptr) {
		std::lock_guard<std::mutex> lock(mDrawLock);

		if (mFont == nullptr)
			D3DXCreateFont(pDevice, mConfig.fontSize, 0, (mConfig.bold ? FW_BOLD : 0), 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, mConfig.fontName, &mFont);

		D3DVIEWPORT9 prt;
		pDevice->GetViewport(&prt);

		pDevice->BeginScene();

		RenderDpsBox(prt);
		RenderDotBox(prt);

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

	__try {
		RenderOverlay();
	} __except (EXCEPTION_EXECUTE_HANDLER) {}


}
void OverlayRenderer::CheckCapture() {
	if (mDoCapture) {
		D3DVIEWPORT9 prt;
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
	std::lock_guard<std::mutex> lock(mCaptureMutex);
	mCaptureBuffers.push_back(buf);
	if (hSaverThread == INVALID_HANDLE_VALUE)
		hSaverThread = CreateThread(NULL, NULL, OverlayRenderer::CaptureBackgroundSaverExternal, this, NULL, NULL);

	ffxivDll->pipe()->AddChat("/e Saving screenshot...");
}

void OverlayRenderer::CaptureScreen() {
	mDoCapture = true;
}

void OverlayRenderer::CaptureBackgroundSaverThread() {
	IDirect3DSurface9 *surface;
	ID3DXBuffer *buf;
	while (mCaptureBuffers.size() > 0) {
		{
			std::lock_guard<std::mutex> lock(mCaptureMutex);
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
				ffxivDll->pipe()->AddChat(message);
			} else {
				DWORD wr;
				WriteFile(h, buf->GetBufferPointer(), buf->GetBufferSize(), &wr, NULL);
				CloseHandle(h);
				std::string message("/e Screenshot saved in ");
				char path2[MAX_PATH * 4];
				WideCharToMultiByte(CP_UTF8, 0, path, -1, path2, sizeof(path2), 0, 0);
				message += path2;
				ffxivDll->pipe()->AddChat(message);
			}
		}
		buf->Release();
	}
	{
		std::lock_guard<std::mutex> lock(mCaptureMutex);
		CloseHandle(hSaverThread);
		hSaverThread = INVALID_HANDLE_VALUE;
	}
	if (ffxivDll->isUnloading())
		TerminateThread(GetCurrentThread(), 0);
}