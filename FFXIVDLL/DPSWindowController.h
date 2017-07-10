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

	union {
		struct {
			int jicon : 1;
			int uidx : 1;
			int uname : 1;
			int dps : 1;
			int total : 1;
			int crit : 1;
			int dh : 1;
			int cdmh : 1;
			int max : 1;
			int death : 1;
		}DpsColumns;
		int DpsColumnsInts[1];
	};

	int maxNameWidth;
	unsigned int simpleViewThreshold;

	virtual int callback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
};

