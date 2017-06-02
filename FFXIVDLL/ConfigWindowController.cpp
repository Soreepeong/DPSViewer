#include "ConfigWindowController.h"
#include "OverlayRenderer.h"
#include <Windows.h>
#include <windowsx.h>
#include "FFXIVDLL.h"

ConfigWindowController::ConfigWindowController(FFXIVDLL *dll, FILE *f) :
	WindowControllerBase(f),
	dll(dll), 
	mDragging (0)
{
	layoutDirection = LAYOUT_DIRECTION_VERTICAL;
	statusMap[CONTROL_STATUS_DEFAULT] = { 0, 0, 0x80000000, 0 };
	statusMap[CONTROL_STATUS_HOVER] = { 0, 0, 0xC0333333, 0 };
	statusMap[CONTROL_STATUS_FOCUS] = { 0, 0, 0xC0333333, 0 };
	relativePosition = 1;
	title = new ORC(LAYOUT_DIRECTION_HORIZONTAL);
	main = new ORC(LAYOUT_DIRECTION_VERTICAL);
	title->statusMap[CONTROL_STATUS_DEFAULT] = { 0, 0xFFFFFFFF, 0x88888888, 0 };
	title->addChild(btnClose = new ORC(L"[X]", CONTROL_TEXT_TYPE::CONTROL_TEXT_STRING, DT_CENTER));
	title->addChild(new ORC(L"Config", CONTROL_TEXT_TYPE::CONTROL_TEXT_STRING, DT_LEFT));
	addChild(title);
	addChild(main);
	
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
		if (!hittest(x, y)) {
			return 0; // release capture
		}
		mDragging = 1;
		requestFront();
		statusFlag[CONTROL_STATUS_PRESS] = 1;
		return 3; // capture and eat
	case WM_MOUSEMOVE:
		if (mDragging == 1) {
			if (getChild(0)->hittest(mLastX,mLastY) && (abs(x - mLastX) >= 5 || abs(y - mLastY) >= 5))
				mDragging = 2;
			return 3; // capture and eat
		} else if (mDragging == 2) {

			xF = min(1 - calcWidth / getParent()->width, max(0, xF + (float)(x - mLastX) / getParent()->width));
			yF = min(1 - calcHeight / getParent()->height, max(0, yF + (float)(y - mLastY) / getParent()->height));

			mLastX = x; mLastY = y;
			return 3; // capture and eat
		} else {
			if (hittest(x, y))
				return 2;
			else
				return 4; // pass onto ffxiv
		}
		break;
	case WM_LBUTTONUP:
		statusFlag[CONTROL_STATUS_PRESS] = 0;
		if (mDragging) {
			if (mDragging == 1) {
				// click
				if (btnLock->hittest(x, y)) {
					dll->process()->wDPS->lock();
					dll->process()->wDOT->lock();
				} else if (btnClose->hittest(x, y))
					visible = false;
				else if (btnHideOthers->hittest(x, y) && dll->hooks()->GetOverlayRenderer())
					dll->hooks()->GetOverlayRenderer()->GetHideOtherUser();
				else if (btnResetMeter->hittest(x, y))
					dll->process()->ResetMeter();
				else if (btnShowDPS->hittest(x, y))
					dll->process()->wDPS->toggleVisibility();
				else if (btnShowDOT->hittest(x, y))
					dll->process()->wDOT->toggleVisibility();
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