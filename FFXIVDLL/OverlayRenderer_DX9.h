#pragma once
#include "OverlayRenderer.h"
#include "dx9hook.h"
#include "dx9table.h"

class OverlayRendererDX9 : public OverlayRenderer {
private:

	IDirect3DDevice9* pDevice;

	LPD3DXFONT mFont;
	LPD3DXSPRITE mSprite;
	std::deque<IDirect3DSurface9*> mCaptureBuffers;

	void CaptureBackgroundSave(IDirect3DSurface9 *buf);
	void CaptureBackgroundSaverThread();
	static DWORD WINAPI CaptureBackgroundSaverExternal(PVOID p) { ((OverlayRendererDX9*) p)->CaptureBackgroundSaverThread(); return 0; }

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
	OverlayRendererDX9(FFXIVDLL *dll, IDirect3DDevice9* device);
	~OverlayRendererDX9();

	virtual void ReloadFromConfig();
	virtual void OnLostDevice();
	virtual void OnResetDevice();
};