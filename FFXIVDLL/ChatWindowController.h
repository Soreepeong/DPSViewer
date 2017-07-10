#pragma once
#include "WindowControllerBase.h"
#include <deque>

class ChatWindowController;

typedef struct _CHATITEM {
	int timestamp;
	union {
		struct {
			char code2;
			char code1;
		};
		short code;
	};
	short _u1;
	char chat[1];
} CHATITEM, *PCHATITEM;

typedef struct _CHATITEM_CUSTOM {
	int timestamp;
	union {
		struct {
			char code2;
			char code1;
		};
		short code;
	};
	std::wstring by;
	std::wstring text;
	int translationStatus;
	std::wstring translated;
	ChatWindowController *obj;
}CHATITEM_CUSTOM, *PCHATITEM_CUSTOM;

struct RectComparator {
	bool operator()(const RECT& a, const RECT& b) const {
		return memcmp(&a, &b, sizeof(RECT)) > 0;
	}
};

class ChatWindowController : public WindowControllerBase{
private:
	const int FADE_OUT_DURATION = 500;
	const int BORDER_BOX_SIZE = 8;
	const int LINE_BUTTON_SIZE = 16;

	int mLastX;
	int mLastY;
	int mLastScrollY;
	int mDragging;
	POINT mCursor;
	std::deque<PCHATITEM_CUSTOM> mLines;
	std::map<RECT, int, RectComparator> mClickMap;
	int mScrollPos;
	uint64_t mFadeOutAt;
	FFXIVDLL *dll;

public:
	ChatWindowController(FFXIVDLL *dll);
	~ChatWindowController();

	bool isClickthrough() const override;

	char mTranslateApiKey[512];

	void addChat(void *p, size_t n);

	virtual void draw(OverlayRenderer *target) override;

	virtual int callback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
};

