#pragma once
#include "OverlayRenderer.h"

class WindowController {
protected:
	OverlayRenderer::Control& mControl;
	int mLocked;

public:
	WindowController(OverlayRenderer::Control &mControl, FILE *f);
	~WindowController();

	void save(FILE *f);
	void lock();
	void toggleVisibility();
	int isLocked();
	virtual int callback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) = 0;
	void setTransparency(int transparency);
};

