#include "WindowController.h"



WindowController::WindowController(OverlayRenderer::Control &c, FILE *f) :
	mControl(c),
	mLocked(0)
{
	mControl.xF = 0.1;
	mControl.yF = 0.1;
	mControl.visible = 1;
	int tr = 0x80;
	if (f != nullptr) {
		fscanf(f, "%f%f%d%d%d", &mControl.xF, &mControl.yF, &mControl.visible, &mLocked, &tr);
		mControl.xF = min(1, max(0, mControl.xF));
		mControl.yF = min(1, max(0, mControl.yF));
	}
	setTransparency(tr);
}


WindowController::~WindowController()
{
}


void WindowController::save(FILE *f)
{
	fprintf(f, "%f %f %d %d %d\n", mControl.xF, mControl.yF, mControl.visible, mLocked, (mControl.statusMap[CONTROL_STATUS_DEFAULT].background & 0xFF000000) >> 24);
}

int WindowController::isLocked() {
	return mLocked;
}

void WindowController::lock() {
	mLocked = !mLocked;
}
void WindowController::toggleVisibility() {
	mControl.visible = !mControl.visible;
}

void WindowController::setTransparency(int transparency) {
	mControl.statusMap[CONTROL_STATUS_DEFAULT].background = (mControl.statusMap[CONTROL_STATUS_DEFAULT].background & 0xFFFFFF) | (min(0xFF, max(0, transparency)) << 24);
	mControl.statusMap[CONTROL_STATUS_HOVER].background = (mControl.statusMap[CONTROL_STATUS_HOVER].background & 0xFFFFFF) | (min(0xFF, max(0, transparency + 30)) << 24);
	mControl.statusMap[CONTROL_STATUS_FOCUS].background = (mControl.statusMap[CONTROL_STATUS_FOCUS].background & 0xFFFFFF) | (min(0xFF, max(0, transparency + 30)) << 24);
	mControl.statusMap[CONTROL_STATUS_PRESS].background = (mControl.statusMap[CONTROL_STATUS_PRESS].background & 0xFFFFFF) | (min(0xFF, max(0, transparency + 30)) << 24);
}