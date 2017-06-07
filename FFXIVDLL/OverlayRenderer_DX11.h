#pragma once
#include "OverlayRenderer.h"
#include <d3d11.h>
#include <vector>
#include "../FW1FontWrapper/Source/FW1FontWrapper.h"
#include "imgui/imgui.h"

class OverlayRendererDX11 : public OverlayRenderer {
private:

	struct RenderInfo {
		OverlayRendererDX11 *rend;
		TCHAR *text;
		int x;
		int y;
		int w;
		int h;
		int flags;
		DWORD color;
	};
	static void OverlayRendererDX11::TextDrawCallbackExternal(const ImDrawList* parent_list, const ImDrawCmd* cmd);
	void TextDrawCallback(RenderInfo& ri);
	static std::vector<RenderInfo> textRenderList;

	TCHAR mFontName[512];

	ID3D11Device* pDevice;
	IDXGISwapChain* pSwapChain;
	ID3D11DeviceContext *pContext;

	IFW1Factory *pFW1Factory;
	IFW1FontWrapper *pFW;

	// std::deque<IDirect3DSurface9*> mCaptureBuffers;

	// void CaptureBackgroundSave(IDirect3DSurface9 *buf);
	void CaptureBackgroundSaverThread();
	static DWORD WINAPI CaptureBackgroundSaverExternal(PVOID p) { ((OverlayRendererDX11*) p)->CaptureBackgroundSaverThread(); return 0; }

	virtual PDXTEXTURETYPE GetTextureFromResource(TCHAR *resName);
	virtual PDXTEXTURETYPE GetTextureFromFile(TCHAR *resName);

	virtual void DrawTexture(int x, int y, int w, int h, PDXTEXTURETYPE tex);
	virtual void MeasureText(RECT &rc, TCHAR *text, int flags);
	virtual void DrawBox(int x, int y, int w, int h, DWORD Color);
	virtual void DrawText(int x, int y, TCHAR *text, DWORD Color);
	virtual void DrawText(int x, int y, int width, int height, TCHAR *text, DWORD Color, int align);
	virtual void RenderOverlay();

	virtual void CheckCapture();

public:
	OverlayRendererDX11(FFXIVDLL *dll, ID3D11Device* device, ID3D11DeviceContext *context, IDXGISwapChain* sc);
	~OverlayRendererDX11();

	virtual void ReloadFromConfig();
	virtual void OnLostDevice();
	virtual void OnResetDevice();
};