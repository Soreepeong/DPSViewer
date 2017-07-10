#pragma once
#include <Windows.h>
#include "OverlayRenderer.h"

class OverlayRenderer::Control;

class WindowControllerBase : public OverlayRenderer::Control{
protected:
	int mLocked;

public:
	WindowControllerBase();
	~WindowControllerBase();

	bool hasCallback() const override {
		return true;
	}
	
	virtual bool isClickthrough() const;

	void lock();
	void toggleVisibility();
	int isLocked();
	virtual int callback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) = 0;
	void setTransparency(int transparency);
};

