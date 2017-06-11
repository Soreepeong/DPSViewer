#pragma once
#include "WindowControllerBase.h"
class DPSWindowController : public WindowControllerBase{
private:
	int mLastX;
	int mLastY;
	int mDragging;

public:
	DPSWindowController();
	~DPSWindowController();

	int maxNameWidth;
	int simpleViewThreshold;

	virtual int callback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
};

