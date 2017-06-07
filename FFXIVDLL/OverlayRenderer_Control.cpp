#include "OverlayRenderer.h"
#include<assert.h>

OverlayRenderer::Control::Control() :
	x(0),
	y(0),
	width(SIZE_WRAP),
	height(SIZE_WRAP),
	margin(0),
	padding(4),
	border(0),
	relativePosition(0),
	relativeSize(0),
	parent(0),
	visible(1),
	text(L""),
	textAlign(DT_LEFT),
	layoutDirection(CONTROL_LAYOUT_DIRECTION::LAYOUT_DIRECTION_VERTICAL),
	statusFlag{ 0, 0, 0, 0 },
	useIcon(CONTROL_TEXT_STRING) {
	memset(statusMap, 0, sizeof(statusMap));
}

OverlayRenderer::Control::Control(CONTROL_LAYOUT_DIRECTION dir) :
	x(0),
	y(0),
	width(SIZE_WRAP),
	height(SIZE_WRAP),
	margin(0),
	padding(4),
	border(0),
	relativePosition(0),
	relativeSize(0),
	parent(0),
	visible(1),
	text(L""),
	textAlign(DT_LEFT),
	layoutDirection(dir),
	statusFlag{ 0, 0, 0, 0 },
	useIcon(CONTROL_TEXT_STRING) {
	memset(statusMap, 0, sizeof(statusMap));
}

OverlayRenderer::Control::Control(float width, float height, DWORD bgcolor) :
	x(0),
	y(0),
	widthF(width),
	heightF(height),
	margin(0),
	padding(0),
	border(0),
	relativePosition(0),
	relativeSize(1),
	parent(0),
	visible(1),
	textAlign(DT_LEFT),
	text(L""),
	layoutDirection(CONTROL_LAYOUT_DIRECTION::LAYOUT_DIRECTION_VERTICAL),
	statusFlag{ 0, 0, 0, 0 },
	useIcon(CONTROL_TEXT_STRING) {
	memset(statusMap, 0, sizeof(statusMap));
	statusMap[0] = { 0, 0xFFFFFFFF, bgcolor, 0 };
}

OverlayRenderer::Control::Control(std::wstring txt, CONTROL_TEXT_TYPE useIcon, int align) :
	x(0),
	y(0),
	width(SIZE_WRAP),
	height(SIZE_WRAP),
	margin(0),
	padding(4),
	border(0),
	relativePosition(0),
	relativeSize(0),
	parent(0),
	visible(1),
	layoutDirection(CONTROL_LAYOUT_DIRECTION::LAYOUT_DIRECTION_VERTICAL),
	statusFlag{ 0, 0, 0, 0 },
	useIcon(useIcon),
	text(txt),
	textAlign(align) {
	memset(statusMap, 0, sizeof(statusMap));
	statusMap[0] = { 0, 0xFFFFFFFF, 0, 0 };
}

OverlayRenderer::Control::~Control() {
	removeAllChildren();
}

void OverlayRenderer::Control::setPaddingRecursive(int p) {
	padding = p;
	for (auto it = children[CHILD_TYPE_NORMAL].begin(); it != children[CHILD_TYPE_NORMAL].end(); ++it)
		(*it)->setPaddingRecursive(p);
}

int OverlayRenderer::Control::hittest(int x, int y) const {
	return calcX <= x && x <= calcX + calcWidth && calcY <= y && y <= calcY + calcHeight;
}

void OverlayRenderer::Control::requestFront() {
	if (parent != nullptr) {
		std::lock_guard<std::recursive_mutex> _lock(parent->layoutLock);
		for(auto it = parent->children[CHILD_TYPE_NORMAL].begin(); it != parent->children[CHILD_TYPE_NORMAL].end(); ++it)
			if ((*it) == this) {
				parent->children[CHILD_TYPE_NORMAL].erase(it);
				parent->children[CHILD_TYPE_NORMAL].insert(parent->children[CHILD_TYPE_NORMAL].end(), this);
				break;
			}
	}
}

void OverlayRenderer::Control::removeAllChildren() {
	std::lock_guard<std::recursive_mutex> _lock(layoutLock);
	for (int i = 0; i < CONTROL_CHILD_TYPE::_CHILD_TYPE_COUNT; i++) {
		for (auto it = children[i].begin(); it != children[i].end(); ++it)
			delete *it;
		children[i].clear();
	}
}

void OverlayRenderer::Control::addChild(OverlayRenderer::Control *c, CONTROL_CHILD_TYPE type) {
	std::lock_guard<std::recursive_mutex> _lock(layoutLock);
	assert(c != nullptr && c->parent == nullptr);
	for (auto it = children[type].begin(); it != children[type].end(); ++it)
		if (*it == c)
			return;
	children[type].push_back(c);
	c->parent = this;
}

void OverlayRenderer::Control::addChild(OverlayRenderer::Control *c) {
	addChild(c, CHILD_TYPE_NORMAL);
}

OverlayRenderer::Control* OverlayRenderer::Control::getChild(int i, CONTROL_CHILD_TYPE type) {
	std::lock_guard<std::recursive_mutex> _lock(layoutLock);
	return children[type][i];
}
OverlayRenderer::Control* OverlayRenderer::Control::getChild(int i) {
	std::lock_guard<std::recursive_mutex> _lock(layoutLock);
	return children[CHILD_TYPE_NORMAL][i];
}
int OverlayRenderer::Control::getChildCount(CONTROL_CHILD_TYPE type) const {
	return children[type].size();
}
int OverlayRenderer::Control::getChildCount() const {
	return children[CHILD_TYPE_NORMAL].size();
}
OverlayRenderer::Control* OverlayRenderer::Control::getParent() const {
	return parent;
}
OverlayRenderer::Control* OverlayRenderer::Control::removeChild(int i, CONTROL_CHILD_TYPE type) {
	std::lock_guard<std::recursive_mutex> _lock(layoutLock);
	OverlayRenderer::Control* c = *(children[type].begin() + i);
	children[type].erase(children[type].begin() + i);
	return c;
}
OverlayRenderer::Control*  OverlayRenderer::Control::removeChild(int i) {
	std::lock_guard<std::recursive_mutex> _lock(layoutLock);
	OverlayRenderer::Control* c = *(children[CHILD_TYPE_NORMAL].begin() + i);
	children[CHILD_TYPE_NORMAL].erase(children[CHILD_TYPE_NORMAL].begin() + i);
	return c;
}

std::recursive_mutex& OverlayRenderer::Control::getLock() {
	return layoutLock;
}

void OverlayRenderer::Control::measure(OverlayRenderer *target, RECT &area, int widthFixed, int heightFixed, int skipChildren) {
	std::lock_guard<std::recursive_mutex> _lock(layoutLock);
	if (!visible) {
		calcX = calcY = calcWidth = calcHeight = 0;
		return;
	}
	RECT rc = { 0, 0,
		relativeSize ? (int)((area.right - area.left) * widthF) : widthFixed != -1 ? widthFixed : (width == SIZE_FILL ? area.right - area.left : width) - (border + padding + margin) * 2,
		relativeSize ? (int)((area.bottom - area.top) * heightF) : heightFixed != -1 ? heightFixed : (height == SIZE_FILL ? area.bottom - area.top : height) - (border + padding + margin) * 2
	};
	if (relativePosition) {
		calcX = area.left + (int)(xF * (area.right - area.left));
		calcY = area.top + (int)(yF * (area.bottom - area.top));
	} else {
		calcX = area.left + x;
		calcY = area.top + y;
	}
	if (!children[CHILD_TYPE_NORMAL].empty()) {
		if (!skipChildren) {
			RECT child = area;
			child.left = calcX + border + margin + padding;
			child.top = calcY + border + margin + padding;
			if (widthFixed == -1) {
				if (!relativeSize) {
					if (width != SIZE_WRAP && width != SIZE_FILL)
						child.right = child.left + width - 2 * (border + margin + padding);
					else if (width == SIZE_WRAP)
						child.right = area.right - border - margin - padding;
				} else {
					child.right = child.left + rc.right - 2 * (border + margin + padding);
				}
			} else
				child.right = child.left + widthFixed - 2 * (border + margin + padding);
			if (heightFixed == -1) {
				if (!relativeSize) {
					if (height != SIZE_WRAP && height != SIZE_FILL)
						child.bottom = child.top + height - 2 * (border + margin + padding);
					else if (height == SIZE_WRAP)
						child.bottom = area.bottom - border - margin - padding;
				} else {
					child.bottom = child.top + rc.bottom - 2 * (border + margin + padding);
				}
			} else
				child.bottom = child.top + heightFixed - 2 * (border + margin + padding);

			if (layoutDirection == LAYOUT_ABSOLUTE) {
				for (auto it = children[CHILD_TYPE_NORMAL].begin(); it != children[CHILD_TYPE_NORMAL].end(); ++it) {
					(*it)->measure(target, child, -1, -1, 0);
				}
			} else if (layoutDirection == LAYOUT_DIRECTION_HORIZONTAL) {
				int base = child.left;
				int maxHeight = 0;
				for (auto it = children[CHILD_TYPE_NORMAL].begin(); it != children[CHILD_TYPE_NORMAL].end(); ++it) {
					(*it)->measure(target, child, -1, -1, 0);
					child.left += (*it)->calcWidth;
					if (heightFixed == -1 && height == SIZE_WRAP && !relativeSize && maxHeight < (*it)->calcHeight)
						maxHeight = (*it)->calcHeight;
				}
				child.bottom = min(child.top + maxHeight, area.bottom);
				rc.right = child.left + border + padding + margin - calcX;
				rc.bottom = child.bottom + border + padding + margin - calcY;
				if (height == SIZE_WRAP && heightFixed == -1) {
					child.left = base;
					for (auto it = children[CHILD_TYPE_NORMAL].begin(); it != children[CHILD_TYPE_NORMAL].end(); ++it) {
						(*it)->measure(target, child, -1, rc.bottom - rc.top - 2 * (border + padding + margin), 0);
						child.left += (*it)->calcWidth;
					}
				}
			} else {
				int base = child.top;
				int maxWidth = 0;
				for (auto it = children[CHILD_TYPE_NORMAL].begin(); it != children[CHILD_TYPE_NORMAL].end(); ++it) {
					(*it)->measure(target, child, -1, -1, 0);
					child.top += (*it)->calcHeight;
					if (widthFixed == -1 && width == SIZE_WRAP && !relativeSize && maxWidth < (*it)->calcWidth)
						maxWidth = (*it)->calcWidth;
				}
				child.right = min(child.left + maxWidth, area.right);
				rc.right = child.right + border + padding + margin - calcX;
				rc.bottom = child.top + border + padding + margin - calcY;
				if (width == SIZE_WRAP && widthFixed == -1) {
					child.top = base;
					for (auto it = children[CHILD_TYPE_NORMAL].begin(); it != children[CHILD_TYPE_NORMAL].end(); ++it) {
						(*it)->measure(target, child, rc.right - rc.left - 2 * (border + padding + margin), -1, 0);
						child.top += (*it)->calcHeight;
					}
				}

				if (layoutDirection == LAYOUT_DIRECTION_VERTICAL_TABLE) {
					std::vector<int> colWidth;
					int baseTop = children[CHILD_TYPE_NORMAL].front()->calcY;
					for (auto it = children[CHILD_TYPE_NORMAL].begin(); it != children[CHILD_TYPE_NORMAL].end(); ++it) {
						uint32_t colIndex = 0;
						int colHeight = 0;
						for (auto it2 = (*it)->children[CHILD_TYPE_NORMAL].begin(); it2 != (*it)->children[CHILD_TYPE_NORMAL].end(); ++it2, ++colIndex) {
							if (colWidth.size() <= colIndex)
								colWidth.push_back(0);
							colWidth[colIndex] = max(colWidth[colIndex], (*it2)->calcWidth);
							colHeight = max(colHeight, (*it2)->calcHeight);
						}
						for (auto it2 = (*it)->children[CHILD_TYPE_NORMAL].begin(); it2 != (*it)->children[CHILD_TYPE_NORMAL].end(); ++it2, ++colIndex) {
							(*it2)->calcHeight = colHeight;
							(*it2)->calcY = baseTop;
						}
						(*it)->calcY = baseTop;
						(*it)->calcHeight = colHeight;
						baseTop += colHeight;
					}
					int widthSum = 0;
					for (auto it = colWidth.begin(); it != colWidth.end(); ++it)
						widthSum += *it;
					for (auto it = children[CHILD_TYPE_NORMAL].begin(); it != children[CHILD_TYPE_NORMAL].end(); ++it) {
						int colIndex = 0, widthSumT = 0;
						RECT rt = { (*it)->calcX, (*it)->calcY, (*it)->calcX + widthSum, (*it)->calcY + (*it)->calcHeight };
						(*it)->measure(target, rt, rt.right - rt.left, rt.bottom - rt.top, 1);
						for (auto it2 = (*it)->children[CHILD_TYPE_NORMAL].begin(); it2 != (*it)->children[CHILD_TYPE_NORMAL].end(); ++it2, ++colIndex) {
							rt.left = calcX + margin + border + padding + widthSumT;
							rt.right = rt.left + colWidth[colIndex];
							(*it2)->measure(target, rt, rt.right - rt.left, rt.bottom - rt.top, 0);
							widthSumT += (*it2)->calcWidth;
						}
					}
					rc.bottom = rc.top + (border + padding + margin)*2 + baseTop - children[CHILD_TYPE_NORMAL].front()->calcY;
					rc.right = rc.left + (border + padding + margin) * 2 + widthSum;
				}
			}
		}
	} else if (!relativeSize && width == SIZE_WRAP) {
		if (text.empty() || useIcon != CONTROL_TEXT_STRING) {
			target->MeasureText(rc, L"DUMMY", DT_NOCLIP);
			rc.right = rc.left + rc.bottom - rc.top;
		} else
			target->MeasureText(rc, (TCHAR*) text.c_str(), DT_NOCLIP);
		rc.right += 2 * (border + padding + margin);
		rc.bottom += 2 * (border + padding + margin);
	} else if (!relativeSize && height == SIZE_WRAP) {
		if (text.empty() || useIcon != CONTROL_TEXT_STRING) {
			target->MeasureText(rc, L"DUMMY", DT_NOCLIP);
		} else
			target->MeasureText(rc, (TCHAR*) text.c_str(), DT_WORDBREAK);
		rc.right += 2 * (border + padding + margin);
		rc.bottom += 2 * (border + padding + margin);
	}
	calcWidth = widthFixed != -1 ? widthFixed : min(rc.right, area.right - area.left);
	calcHeight = heightFixed != -1 ? heightFixed : min(rc.bottom, area.bottom - area.top);
	rc = { calcX + margin + border, calcY + margin + border,
		calcWidth - 2 * (margin + border), calcHeight - 2 * (margin + border) };
	rc.right += rc.left; rc.bottom += rc.top;
	for (auto it = children[CHILD_TYPE_FOREGROUND].begin(); it != children[CHILD_TYPE_FOREGROUND].end(); ++it)
		(*it)->measure(target, rc, rc.right - rc.left, rc.bottom - rc.top, skipChildren);
	for (auto it = children[CHILD_TYPE_BACKGROUND].begin(); it != children[CHILD_TYPE_BACKGROUND].end(); ++it)
		(*it)->measure(target, rc, rc.right - rc.left, rc.bottom - rc.top, skipChildren);
}

void OverlayRenderer::Control::draw(OverlayRenderer *target) {
	std::lock_guard<std::recursive_mutex> _lock(layoutLock);
	if (!visible)
		return;
	Status current = statusMap[0];
	for(int i = 1; i < _CONTROL_STATUS_COUNT; i++)
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
	if (border > 0) {
		target->DrawBox(calcX + margin, calcY + margin, calcWidth - margin * 2, border, current.border);
		target->DrawBox(calcX + margin, calcY + margin + border, border, calcHeight - 2 * (margin + border), current.border);
		target->DrawBox(calcX + calcWidth - margin - border, calcY + margin + border, border, calcHeight - 2 * (margin + border), current.border);
		target->DrawBox(calcX + margin, calcY + calcHeight - margin - border, calcWidth - margin * 2, border, current.border);
	}
	for (auto it = children[CHILD_TYPE_BACKGROUND].begin(); it != children[CHILD_TYPE_BACKGROUND].end(); ++it)
		(*it)->draw(target);
	if (children[CHILD_TYPE_NORMAL].empty()) {
		if (!text.empty()) {
			switch (useIcon) {
			case CONTROL_TEXT_STRING:
				target->DrawText(calcX + margin + border + padding,
					calcY + margin + border + padding,
					calcWidth - 2 * (margin + border + padding),
					calcHeight - 2 * (margin + border + padding),
					(TCHAR*)text.c_str(), current.foreground, textAlign);
				break;
			case CONTROL_TEXT_RESNAME: {
				PDXTEXTURETYPE tex = target->GetTextureFromResource((TCHAR*)text.c_str());
				if (tex != nullptr)
					target->DrawTexture(calcX + margin + border + padding,
						calcY + margin + border + padding,
						calcWidth - 2 * (margin + border + padding),
						calcHeight - 2 * (margin + border + padding),
						tex);
				break;
			}
			case CONTROL_TEXT_FILENAME: {
				PDXTEXTURETYPE tex = target->GetTextureFromFile((TCHAR*)text.c_str());
				if (tex != nullptr)
					target->DrawTexture(calcX + margin + border + padding,
						calcY + margin + border + padding,
						calcWidth - 2 * (margin + border + padding),
						calcHeight - 2 * (margin + border + padding),
						tex);
				break;
			}
			}
		}
	} else {
		for (auto it = children[CHILD_TYPE_NORMAL].begin(); it != children[CHILD_TYPE_NORMAL].end(); ++it)
			(*it)->draw(target);
	}
	for (auto it = children[CHILD_TYPE_FOREGROUND].begin(); it != children[CHILD_TYPE_FOREGROUND].end(); ++it)
		(*it)->draw(target);
}