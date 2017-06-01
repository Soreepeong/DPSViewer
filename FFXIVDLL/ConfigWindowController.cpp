#include "ConfigWindowController.h"
#include "OverlayRenderer.h"
#include <Windows.h>
#include <windowsx.h>
#include "FFXIVDLL.h"

ConfigWindowController::ConfigWindowController(FFXIVDLL *dll, OverlayRenderer::Control &c, FILE *f) :
	WindowController(c, f),
	dll(dll), 
	mDragging (0)
{
	c.layoutDirection = LAYOUT_DIRECTION_VERTICAL;
	c.statusMap[CONTROL_STATUS_DEFAULT] = { 0, 0, 0x80000000, 0 };
	c.statusMap[CONTROL_STATUS_HOVER] = { 0, 0, 0xC0333333, 0 };
	c.statusMap[CONTROL_STATUS_FOCUS] = { 0, 0, 0xC0333333, 0 };
	c.relativePosition = 1;
	title = new ORC(LAYOUT_DIRECTION_HORIZONTAL);
	main = new ORC(LAYOUT_DIRECTION_VERTICAL);
	title->statusMap[CONTROL_STATUS_DEFAULT] = { 0, 0xFFFFFFFF, 0x88888888, 0 };
	title->addChild(btnClose = new ORC(L"[X]", CONTROL_TEXT_TYPE::CONTROL_TEXT_STRING, DT_CENTER));
	title->addChild(new ORC(L"Config", CONTROL_TEXT_TYPE::CONTROL_TEXT_STRING, DT_LEFT));
	c.addChild(title);
	c.addChild(main);
	
	main->addChild(new ORC(LAYOUT_DIRECTION_HORIZONTAL));
	main->getChild(0)->addChild(btnLock = new ORC(L"Lock/Unlock", CONTROL_TEXT_TYPE::CONTROL_TEXT_STRING, DT_CENTER));
	main->getChild(0)->addChild(btnShowDPS = new ORC(L"Toggle DPS", CONTROL_TEXT_TYPE::CONTROL_TEXT_STRING, DT_CENTER));
	main->getChild(0)->addChild(btnShowDOT = new ORC(L"Toggle DOT", CONTROL_TEXT_TYPE::CONTROL_TEXT_STRING, DT_CENTER));
	main->getChild(0)->addChild(btnHideOthers = new ORC(L"Hide others", CONTROL_TEXT_TYPE::CONTROL_TEXT_STRING, DT_CENTER));
	main->getChild(0)->addChild(btnResetMeter = new ORC(L"Reset meter", CONTROL_TEXT_TYPE::CONTROL_TEXT_STRING, DT_CENTER));
	main->getChild(0)->addChild(btnQuit = new ORC(L"Quit", CONTROL_TEXT_TYPE::CONTROL_TEXT_STRING, DT_CENTER));

	buttonify(btnLock);
	buttonify(btnShowDPS);
	buttonify(btnShowDOT);
	buttonify(btnHideOthers);
	buttonify(btnResetMeter);
	buttonify(btnQuit);
}


ConfigWindowController::~ConfigWindowController()
{
}

void ConfigWindowController::buttonify(ORC *btn) {
	btn->statusMap[CONTROL_STATUS_DEFAULT] = { 0, 0xFFFFFFFF, 0xFF000000, 0xFF888888 };
	btn->statusMap[CONTROL_STATUS_HOVER] = { 0, 0xFFFFFFFF, 0xFF555555, 0xFFCCCCCC };
	btn->statusMap[CONTROL_STATUS_FOCUS] = { 0, 0xFFFFFFFF, 0xFF333333, 0xFFAAAAAA };
	btn->statusMap[CONTROL_STATUS_PRESS] = { 0, 0xFFFFFFFF, 0xFF000000, 0xFF444444 };
	btn->padding = 8;
	btn->border = 2;
}

int ConfigWindowController::callback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
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
			if (mControl.getChild(0)->hittest(mLastX,mLastY) && (abs(x - mLastX) >= 5 || abs(y - mLastY) >= 5))
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
			if (mDragging == 1) {
				// click
				if (btnLock->hittest(x, y)) {
					dll->process()->mDPSController->lock();
					dll->process()->mDOTController->lock();
				} else if (btnClose->hittest(x, y))
					mControl.visible = false;
				else if (btnHideOthers->hittest(x, y) && dll->hooks()->GetOverlayRenderer())
					dll->hooks()->GetOverlayRenderer()->GetHideOtherUser();
				else if (btnResetMeter->hittest(x, y))
					dll->process()->ResetMeter();
				else if (btnShowDPS->hittest(x, y))
					dll->process()->mDPSController->toggleVisibility();
				else if (btnShowDOT->hittest(x, y))
					dll->process()->mDOTController->toggleVisibility();
				else if (btnQuit->hittest(x, y))
					dll->spawnSelfUnloader();
			}
			mDragging = false;
			return 3; // capture and eat
		} 
			
		break;
	}

	return 4; // pass onto ffxiv
}