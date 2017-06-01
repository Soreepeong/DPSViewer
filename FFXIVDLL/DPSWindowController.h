#pragma once
#include "WindowController.h"
class DPSWindowController : public WindowController{
private:
	int mLastX;
	int mLastY;
	int mDragging;

public:
	DPSWindowController(OverlayRenderer::Control &mControl, FILE *f);
	~DPSWindowController();

	virtual int callback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
};

