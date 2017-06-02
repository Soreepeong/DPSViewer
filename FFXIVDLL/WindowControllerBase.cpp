#include "WindowControllerBase.h"
#include "c:\Users\Soreepeong\Documents\Visual Studio 2015\Projects\DPSViewer\FFXIVDLL\WindowControllerBase.h"



WindowControllerBase::WindowControllerBase(FILE *f) :
	OverlayRenderer::Control(),
	mLocked(0)
{
	xF = 0.1;
	yF = 0.1;
	visible = 1;
	int tr = 0x80;
	if (f != nullptr) {
		fscanf(f, "%f%f%d%d%d", &xF, &yF, &visible, &mLocked, &tr);
		xF = min(1, max(0, xF));
		yF = min(1, max(0, yF));
	}
	setTransparency(tr);
}


WindowControllerBase::WindowControllerBase(OverlayRenderer::Control & mControl, FILE * f)
{
}

WindowControllerBase::~WindowControllerBase()
{
}


void WindowControllerBase::save(FILE *f)
{
	fprintf(f, "%f %f %d %d %d\n", xF, yF, visible, mLocked, (statusMap[CONTROL_STATUS_DEFAULT].background & 0xFF000000) >> 24);
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