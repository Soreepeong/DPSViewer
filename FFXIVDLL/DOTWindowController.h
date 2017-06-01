#pragma once
#include "WindowController.h"
class DOTWindowController : public WindowController{
private:
	int mLastX;
	int mLastY;
	int mDragging;

public:
	DOTWindowController(OverlayRenderer::Control &mControl, FILE *f);
	~DOTWindowController();

	virtual int callback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
};

