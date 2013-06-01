#pragma once

#include "General.h"

class Window
{
protected:
	HWND		m_hWnd;
	HWND		m_hParent;

public:
	Window(HWND hParent) : m_hParent(hParent), m_hWnd(nullptr) {}
	virtual ~Window() {/*TODO: what do we do with hWnd?*/}
};