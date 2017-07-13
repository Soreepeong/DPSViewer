#include "ChatWindowController.h"
#include "OverlayRenderer.h"
#include <Windows.h>
#include <windowsx.h>
#include "FFXIVDLL.h"
#include <WinInet.h>
#pragma comment (lib, "Wininet.lib")
#include <sstream>
#include <iomanip>
#include "json.hpp"

const int TRANSLATE_NONE = 0;
const int TRANSLATE_WORKING = 1;
const int TRANSLATE_DONE = 2;
const int TRANSLATE_DONE_NOTINUSE = 3;
const int TRANSLATE_ERROR = 4;

ChatWindowController::ChatWindowController(FFXIVDLL *dll) :
	WindowControllerBase(),
	mDragging (0),
	mScrollPos(0),
	mFadeOutAt(0),
	dll(dll) {
	width = 320;
	height = 240;
	/*
	{
		PCHATITEM_CUSTOM itm = new CHATITEM_CUSTOM;
		itm->by = L"A"; itm->text = L"This is a test text!";
		itm->translationStatus = TRANSLATE_NONE;
		itm->obj = this;
		mLines.push_back(itm);
	}
	{
		PCHATITEM_CUSTOM itm = new CHATITEM_CUSTOM;
		itm->by = L"B"; itm->text = L"Er, I don't get it.";
		itm->translationStatus = TRANSLATE_NONE;
		itm->obj = this;
		mLines.push_back(itm);
	}
	{
		PCHATITEM_CUSTOM itm = new CHATITEM_CUSTOM;
		itm->by = L"C"; itm->text = L"It might work after all!";
		itm->translationStatus = TRANSLATE_NONE;
		itm->obj = this;
		mLines.push_back(itm);
	}
	//*/
	mScrollPos = mLines.size() - 1;
}

bool ChatWindowController::isClickthrough() const {
	return false;
}

ChatWindowController::~ChatWindowController() {
	for (auto i = mLines.begin(); i != mLines.end(); i++)
		delete *i;
}

void ChatWindowController::draw(OverlayRenderer *target) {
	std::lock_guard<std::recursive_mutex> _lock(getLock());

	TCHAR buf[32768];
	int curY = calcHeight - margin - border - padding;
	Status current = statusMap[0];
	for (int i = 1; i < _CONTROL_STATUS_COUNT; i++)
		if (statusFlag[i]) {
			current = statusMap[i];
			break;
		}
	if (current.useDefault)
		current = statusMap[0];
	if (current.background != 0)
		target->DrawBox(calcX + border + margin,
			calcY + border + margin,
			calcWidth - 2 * (border + margin),
			calcHeight - 2 * (border + margin),
			current.background);
	if (statusFlag[CONTROL_STATUS_HOVER] || mDragging)
		mFadeOutAt = GetTickCount64() + FADE_OUT_DURATION * 2;
	if (mFadeOutAt > GetTickCount64()) {
		uint32_t tr = static_cast<uint32_t>(min(255, 255 * (mFadeOutAt - GetTickCount64()) / FADE_OUT_DURATION));
		tr <<= 24;
		target->DrawBox(calcX + calcWidth - BORDER_BOX_SIZE, calcY + calcHeight - BORDER_BOX_SIZE, BORDER_BOX_SIZE, BORDER_BOX_SIZE, 0x0000FF | tr);
		if (mLines.size() >= 2) {
			int barH = max(24, (calcHeight - BORDER_BOX_SIZE) / mLines.size());
			target->DrawBox(calcX + calcWidth - BORDER_BOX_SIZE,
				calcY + (calcHeight - BORDER_BOX_SIZE - barH) * mScrollPos / (mLines.size() - 1),
				BORDER_BOX_SIZE, barH, 0x00FF00 | tr);
		}
	}
	mClickMap.clear();
	for (int i = mScrollPos; i >= 0 && curY > 0; i--) {
		PCHATITEM_CUSTOM itm = mLines[i];
		RECT rc = { margin + border + padding,
			margin + border + padding,
			calcWidth - 2 * (margin + border + padding) - BORDER_BOX_SIZE - 2 * LINE_BUTTON_SIZE,
			calcHeight - 2 * (margin + border + padding) };
		TCHAR *type = L"";
		int pos;
		DWORD color = 0xFFFFFFFF;
		const wchar_t *text = itm->text.c_str();
		switch (itm->code2) {
			case 10: color = 0xFFf7f7f7; break; // say
			case 11: color = 0xFFffa666; break; // shout
			case 12: color = 0xFFffb8de; type = L"<<"; break;
			case 13: color = 0xFFffb8de; type = L">>"; break;
			case 14: color = 0xFF66e5ff; type = L"[P]"; break;
			case 16: color = 0xFFd4ff7d; type = L"[1]"; break;
			case 17: color = 0xFFd4ff7d; type = L"[2]"; break;
			case 18: color = 0xFFd4ff7d; type = L"[3]"; break;
			case 19: color = 0xFFd4ff7d; type = L"[4]"; break;
			case 20: color = 0xFFd4ff7d; type = L"[5]"; break;
			case 21: color = 0xFFd4ff7d; type = L"[6]"; break;
			case 22: color = 0xFFd4ff7d; type = L"[7]"; break;
			case 23: color = 0xFFd4ff7d; type = L"[8]"; break;
			case 24: color = 0xFFabdbe5;  type = L"[FC]"; break;
			case 29: color = 0xFFbafff0; break; // Expression
			case 30: color = 0xFFffff00; break; // Yell
			case 56: color = 0xFFcccccc; break; // Echo
			case 61: case 68: color = 0xFFabd647; break; // npc dialogue
		}
		if (itm->translationStatus == TRANSLATE_DONE || (itm->translationStatus == TRANSLATE_ERROR && !itm->translated.empty()))
			text = itm->translated.c_str();
		time_t time_raw = itm->timestamp;
		auto time = std::localtime(&time_raw);
		if(itm->by.empty())
			pos = swprintf(buf, sizeof(buf)/sizeof(TCHAR), L"[%02d:%02d] %s%s", time->tm_hour, time->tm_min, type, text);
		else
			pos = swprintf(buf, sizeof(buf) / sizeof(TCHAR), L"[%02d:%02d] %s%s: %s", time->tm_hour, time->tm_min, type, itm->by.c_str(), text);
		
		target->MeasureText(rc, buf, DT_WORDBREAK);
		curY -= rc.bottom - rc.top;
		if (curY < margin + border + padding)
			break;
		rc.left += calcX; rc.top += calcY + curY; rc.right += calcX; rc.bottom += calcY + curY;
		target->DrawText(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, buf, color, DT_LEFT | DT_WORDBREAK);

		// buttons
		rc.left = calcX + (margin + border + padding);
		rc.right = calcX + calcWidth - 2*(margin + border + padding) - BORDER_BOX_SIZE;
		mClickMap[rc] = i;
		if (PtInRect(&rc, mCursor) && statusFlag[CONTROL_STATUS_HOVER]) {
			rc.left = calcX + calcWidth - 2 * (margin + border + padding) - BORDER_BOX_SIZE - LINE_BUTTON_SIZE * 2;
			rc.right = rc.left + LINE_BUTTON_SIZE;
			if (PtInRect(&rc, mCursor) && statusFlag[CONTROL_STATUS_HOVER] && !mDragging) {
				target->DrawBox(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, 0x40FFFFFF);
			}
			target->DrawText(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, itm->translationStatus == TRANSLATE_WORKING ? L"..." : L"T", 0xFFFFFFFF, DT_CENTER | DT_WORDBREAK);
			rc.left += LINE_BUTTON_SIZE; rc.right += LINE_BUTTON_SIZE;
			if (PtInRect(&rc, mCursor) && statusFlag[CONTROL_STATUS_HOVER] && !mDragging) {
				target->DrawBox(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, 0x40FFFFFF);
			}
			target->DrawText(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, L"C", 0xFFFFFFFF, DT_CENTER | DT_WORDBREAK);
		}
	}
}

void ChatWindowController::addChat(void *p_, size_t len) {
	PCHATITEM p = (PCHATITEM) p_;
	if (p->code1 == 0) {
		PCHATITEM_CUSTOM n = new CHATITEM_CUSTOM;
		n->timestamp = p->timestamp;
		n->code = p->code;
		n->translationStatus = TRANSLATE_NONE;
		n->obj = this;

		char *clean = new char[len*8];
		int clen = 0;
		for (size_t i = 0; i < len - offsetof(CHATITEM, chat); i++) {
			switch (p->chat[i]) {
				case 2: {
					int len = p->chat[i + 2];
					if (len > 1) {
						i += len + 2;
					} else {
						i += 4;
						// clen += sprintf(clean + clen, "%02x", (int) p->chat[i]);
						clean[clen++] = p->chat[i];
					}
					break;
				}
				default:
					// clen += sprintf(clean + clen, "%02x", (int) p->chat[i]);
					clean[clen++] = p->chat[i];
			}
		}
		clean[clen++] = 0;

		TCHAR *conv;
		int len = MultiByteToWideChar(CP_UTF8, 0, clean, clen, 0, 0);
		conv = new TCHAR[len];
		MultiByteToWideChar(CP_UTF8, 0, clean, clen, conv, len);
		if (conv[1] == ':') {
			n->text = std::wstring(conv + 2);
		} else {
			TCHAR *pos = wcsstr(conv + 1, L":");
			if (pos) {
				n->by = std::wstring(conv + 1, pos - conv - 1);
				n->text = std::wstring(pos + 1);
			}else
				n->text = std::wstring(conv);
		}
		delete conv;
		delete clean;

		mLines.push_back(n);
		if (mLines.size() > 100)
			mLines.pop_front();
		if (!mDragging && (mScrollPos >= mLines.size() - 2 || mLines.size() == 1))
			mScrollPos = mLines.size() - 1;
	}
}


DWORD WINAPI translate(PVOID p_) {
	PCHATITEM_CUSTOM p = (PCHATITEM_CUSTOM) p_;
	HINTERNET Initialize, File;
	DWORD dwBytes;
	LPSTR accept1[2] = { "Accept: */*", NULL };
	std::wstringstream query;
	static const char *hdrs[] = { "Content-Type: application/x-www-form-urlencoded" };
	static const TCHAR *accept[] = { L"*/*", NULL };

	query << L"https://translation.googleapis.com/language/translate/v2";
	query << L"?format=text&key=";
	query << p->obj->mTranslateApiKey;
	query << L"&target=ko";
	query << L"&q=";

	{
		char *conv;
		int len = WideCharToMultiByte(CP_UTF8, 0, p->text.c_str(), -1, 0, 0, 0, 0);
		conv = new char[len];
		WideCharToMultiByte(CP_UTF8, 0, p->text.c_str(), -1, conv, len, 0, 0);
		for (char *c = conv; *c; c++) {
			if (isalnum(*c) || *c == '-' || *c == '_' || *c == '.' || *c == '~') {
				query << (wchar_t) *c;
				continue;
			}
			query << std::uppercase << std::setfill(L'0') << std::hex;
			query << '%' << std::setw(2) << int((unsigned char) *c);
			query << std::nouppercase;
		}
		delete conv;
	}

	if (Initialize = InternetOpen(L"Test", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0)) {
		if (File = InternetOpenUrl(Initialize, query.str().c_str(), NULL, NULL, INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_SECURE, NULL)) {
			std::stringstream webSource;
			char buf[8192];
			while (InternetReadFile(File, buf, sizeof(buf), &dwBytes)) {
				if (dwBytes == 0)break;
				webSource << std::string(buf, dwBytes);
			}

			nlohmann::json json = nlohmann::json::parse(webSource.str().c_str());
			try {
				// data/translations/[0]/translatedText
				auto d1 = json["data"];
				auto d2 = d1["translations"];
				auto d3 = d2[0];
				auto d4 = d3["translatedText"];
				std::string res = d4;
				TCHAR *conv;
				int len = MultiByteToWideChar(CP_UTF8, 0, res.c_str(), -1, 0, 0);
				conv = new TCHAR[len];
				MultiByteToWideChar(CP_UTF8, 0, res.c_str(), -1, conv, len);
				p->translated = conv;
				p->translationStatus = TRANSLATE_DONE;
				delete conv;
			} catch (nlohmann::json::type_error err) {
				try {
					// error/message
					TCHAR *conv;
					std::string res = json["error"]["message"];
					int len = MultiByteToWideChar(CP_UTF8, 0, res.c_str(), -1, 0, 0);
					conv = new TCHAR[len];
					MultiByteToWideChar(CP_UTF8, 0, res.c_str(), -1, conv, len);
					p->translated = conv;
				} catch (nlohmann::json::type_error err2) {
				}
				p->translationStatus = TRANSLATE_ERROR;
			}

			InternetCloseHandle(File);
		}else
			p->translationStatus = TRANSLATE_ERROR;
		InternetCloseHandle(Initialize);
	}else
		p->translationStatus = TRANSLATE_ERROR;
	return 0;
}

int ChatWindowController::callback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
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
		case WM_MOUSEWHEEL:
			if (mLines.size() >= 2) {
				int zDelta = GET_WHEEL_DELTA_WPARAM(wparam);
				mScrollPos = min(mLines.size() - 1, max(0, mScrollPos - zDelta / WHEEL_DELTA));
				mFadeOutAt = GetTickCount64() + FADE_OUT_DURATION * 2;
			}
			return 5; // eat
		case WM_MOUSEMOVE:
			if (mDragging == 1) {
				if (abs(x - mLastX) >= 5 || abs(y - mLastY) >= 5) {
					if (mLastX >= calcX + calcWidth - BORDER_BOX_SIZE) {
						if (mLastY >= calcY + calcHeight - BORDER_BOX_SIZE) {
							if (!mLocked)
								mDragging = 3;
						} else if (mLines.size() >= 2) {
							mDragging = 4;
							mLastScrollY = mLastY;
						}
					} else {
						if(!mLocked)
							mDragging = 2;
					}
				}
				return 3; // capture and eat
			} else if (mDragging == 2) {

				xF = min(1 - calcWidth / getParent()->width, max(0, xF + (float) (x - mLastX) / getParent()->width));
				yF = min(1 - calcHeight / getParent()->height, max(0, yF + (float) (y - mLastY) / getParent()->height));

				mLastX = x; mLastY = y;
				return 3; // capture and eat
			} else if (mDragging == 3) { // resize

				width = width + x - mLastX;
				height = height + y - mLastY;
				width = max(64, min(640, width));
				height = max(64, min(640, height));

				mLastX = x; mLastY = y;
				return 3; // capture and eat
			} else if (mDragging == 4) { // scroll

				int barH = max(24, (calcHeight - BORDER_BOX_SIZE) / mLines.size());
				mLastScrollY = mLastScrollY + y - mLastY;
				mScrollPos = (mLines.size() - 1) * min(calcHeight - BORDER_BOX_SIZE - barH, max(0, mLastScrollY - calcY)) / (calcHeight - BORDER_BOX_SIZE - barH);

				mLastX = x; mLastY = y;
				return 3; // capture and eat
			} else {
				mCursor.x = x;
				mCursor.y = y;
				return 4; // pass onto ffxiv
			}
			break;
		case WM_LBUTTONUP:
			statusFlag[CONTROL_STATUS_PRESS] = 0;
			if (mDragging) {
				if (mDragging == 1) { // Click
					std::lock_guard<std::recursive_mutex> _lock(getLock());
					mCursor.x = x;
					mCursor.y = y;
					for (auto i = mClickMap.cbegin(); i != mClickMap.cend(); ++i) {
						if (PtInRect(&(i->first), mCursor)) {
							PCHATITEM_CUSTOM line = mLines[i->second];
							if (x >= i->first.right - LINE_BUTTON_SIZE) {
								// copy
								int len = text.length() + 1;
								HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len * 2);
								memcpy(GlobalLock(hMem), line->text.c_str(), len * 2);
								GlobalUnlock(hMem);
								OpenClipboard(0);
								EmptyClipboard();
								SetClipboardData(CF_UNICODETEXT, hMem);
								CloseClipboard();
							} else if (x >= i->first.right - LINE_BUTTON_SIZE * 2) {
								// translate
								switch (line->translationStatus) {
									case TRANSLATE_NONE:
									case TRANSLATE_ERROR:
										line->translationStatus = TRANSLATE_WORKING;
										CloseHandle(CreateThread(0, 0, translate, line, 0, 0));
										break;
									case TRANSLATE_DONE_NOTINUSE:
										line->translationStatus = TRANSLATE_DONE;
										break;
									case TRANSLATE_DONE:
										line->translationStatus = TRANSLATE_DONE_NOTINUSE;
										break;
								}
							}
							break;
						}
					}
				}
				mDragging = false;
				return 3; // capture and eat
			}

			break;
	}

	return 4; // pass onto ffxiv
}