
// MainFrm.h : interface of the CMainFrame class
//

#pragma once
#include "ChildView.h"
#include "Serial.h"

class CMainFrame : public CFrameWnd
{
	
public:
	CMainFrame();
protected: 
	DECLARE_DYNAMIC(CMainFrame)

// Attributes
public:

// Operations
public:

// Overrides
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	static CMainFrame& Instance();

	Serial& GetSerial() { return _serial; }

protected:  // control bar embedded members
	CToolBar          m_wndToolBar;
	CStatusBar        m_wndStatusBar;
	CChildView    m_wndView;

// Generated message map functions
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSetFocus(CWnd *pOldWnd);
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnTimer(UINT_PTR nIDEvent);

private:
	Serial _serial;
	static CMainFrame* _instance;
	double _frequency, _nextFrequency;
	DWORD _lastFrequencyUpdateTime;

public:
	afx_msg void OnDestroy();
	afx_msg void OnConnect();
	afx_msg void OnUpdateConnect(CCmdUI *pCmdUI);
	afx_msg void OnTrigger();
	afx_msg void OnUpdateTrigger(CCmdUI *pCmdUI);
	afx_msg void OnZoomIn();
	afx_msg void OnZoomOut();
};


