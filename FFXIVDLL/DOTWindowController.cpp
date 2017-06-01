#include "DOTWindowController.h"
#include "OverlayRenderer.h"
#include <Windows.h>
#include <windowsx.h>



DOTWindowController::DOTWindowController(OverlayRenderer::Control &c, FILE *f) :
	WindowController(c, f),
	mDragging (0)
{
}


DOTWindowController::~DOTWindowController()
{
}


int DOTWindowController::callback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	int x = GET_X_LPARAM(lparam);
	int y = GET_Y_LPARAM(lparam);

	switch (msg) {
	case WM_LBUTTONDOWN:
		mLastX = x;
		mLastY = y;
		if (!mControl.hittest(x, y)) {
			return 0; // release capture
		}
		mDragging = 1;
		mControl.requestFront();
		mControl.statusFlag[CONTROL_STATUS_PRESS] = 1;
		return 3; // capture and eat
	case WM_MOUSEMOVE:
		if (mDragging == 1) {
			if (abs(x - mLastX) >= 5 || abs(y - mLastY) >= 5)
				mDragging = 2;
			return 3; // capture and eat
		} else if (mDragging == 2) {

			mControl.xF = min(1 - mControl.calcWidth / mControl.getParent()->width, max(0, mControl.xF + (float)(x - mLastX) / mControl.getParent()->width));
			mControl.yF = min(1 - mControl.calcHeight / mControl.getParent()->height, max(0, mControl.yF + (float)(y - mLastY) / mControl.getParent()->height));

			mLastX = x; mLastY = y;
			return 3; // capture and eat
		} else {
			if (mControl.hittest(x, y))
				return 2;
			else
				return 4; // pass onto ffxiv
		}
		break;
	case WM_LBUTTONUP:
		mControl.statusFlag[CONTROL_STATUS_PRESS] = 0;
		if (mDragging) {
			mDragging = false;
			return 3; // capture and eat
		}
		break;
	}

	return 4; // pass onto ffxiv
}