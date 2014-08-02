
// Oscilloscope.h : main header file for the Oscilloscope application
//
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols


class COscilloscopeApp : public CWinApp
{
public:
	COscilloscopeApp();


// Overrides
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

public:
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()

private:
};

COscilloscopeApp& GetApp();
