#include "WindowControllerBase.h"

WindowControllerBase::WindowControllerBase() :
	OverlayRenderer::Control(),
	mLocked(0)
{
	xF = 0.1f;
	yF = 0.1f;
	visible = 1;
	setTransparency(0x80);
}

WindowControllerBase::~WindowControllerBase()
{
}

int WindowControllerBase::isLocked() {
	return mLocked;
}

void WindowControllerBase::lock() {
	mLocked = !mLocked;
}
void WindowControllerBase::toggleVisibility() {
	visible = !visible;
}

void WindowControllerBase::setTransparency(int transparency) {
	statusMap[CONTROL_STATUS_DEFAULT].background = (statusMap[CONTROL_STATUS_DEFAULT].background & 0xFFFFFF) | (min(0xFF, max(0, transparency)) << 24);
	statusMap[CONTROL_STATUS_HOVER].background = (statusMap[CONTROL_STATUS_HOVER].background & 0xFFFFFF) | (min(0xFF, max(0, transparency + 30)) << 24);
	statusMap[CONTROL_STATUS_FOCUS].background = (statusMap[CONTROL_STATUS_FOCUS].background & 0xFFFFFF) | (min(0xFF, max(0, transparency + 30)) << 24);
	statusMap[CONTROL_STATUS_PRESS].background = (statusMap[CONTROL_STATUS_PRESS].background & 0xFFFFFF) | (min(0xFF, max(0, transparency + 30)) << 24);
}