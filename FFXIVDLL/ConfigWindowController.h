#pragma once
#include "WindowController.h"

class ConfigWindowController : public WindowController{
private:
	typedef OverlayRenderer::Control ORC;
	int mLastX;
	int mLastY;
	int mDragging;

	FFXIVDLL *dll;

	ORC *title;
	ORC *main;
	
	ORC *btnClose;
	ORC *btnQuit;
	ORC *btnLock;
	ORC *btnShowDPS;
	ORC *btnShowDOT;
	ORC *btnResetMeter;
	ORC *btnHideOthers;

	void buttonify(ORC *btn);

public:
	ConfigWindowController(FFXIVDLL *dll, OverlayRenderer::Control &mControl, FILE *f);
	~ConfigWindowController();

	virtual int callback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
};

