#pragma once
#include "OverlayRenderer.h"

class WindowControllerBase : public OverlayRenderer::Control{
protected:
	int mLocked;

public:
	WindowControllerBase(FILE *f);
	~WindowControllerBase();

	bool hasCallback() const override {
		return true;
	}

	void save(FILE *f);
	void lock();
	void toggleVisibility();
	int isLocked();
	virtual int callback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) = 0;
	void setTransparency(int transparency);
};

