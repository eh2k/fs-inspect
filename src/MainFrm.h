/*
* Copyright (c) 2005-2010 Eduard Heidt http://fs-inspect.sourceforge.net.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#pragma once

#include "simplesplitter.h"

#include "SQLiteDialog.h"
#include "filelist.h"
#include "dirsize.h"
#include "typestat.h"
#include "afxlinkctrl.h"

#define MAINFRAME_REFRESHDATA			123451
#define MAINFRAME_DOFILELIST			123454
#define MAINFRAME_WEBUPDATE				123456
#define MAINFRAME_PROGRESSBAR_INIT		123457
#define MAINFRAME_PROGRESSBAR_STEP		123458

#define ID_VIEW_SPLIT                   32935
#define ID_VIEW_DIRSIZE                 32937
#define ID_VIEW_TYPESTAT                32938
#define ID_VIEW_FILELIST                32939
#define ID_VIEW_DB                      32940

/************************   CMainFrame *************************/

class CMainFrame : public CFrameWndEx
{
public:
	CMainFrame();
protected: 
	DECLARE_DYNAMIC(CMainFrame)
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

private:  // Eingebundene Elemente der Steuerleiste

	CMFCRibbonBar  m_wndRibbonBar;
	CMFCStatusBar m_wndStatusBar;
	CMFCLinkCtrl m_hyperlink;
	void UpdateHyperlinkPos();

	CSQLiteDialog m_sqldlg;

	UINT m_view;
	CSimpleSplitter m_wndSplitterHorz, m_wndSplitterVert;
	CBrush m_bkgLeft, m_bkgRight, m_bkgRightTop;

	CRITICAL_SECTION m_cs;
	HANDLE m_hScanThread;

	HANDLE m_hUpdateCheckThread;
	void OnWebUpdateChecked();

	void OnProgressBarInit();
	void OnProgressBarStep();

	static long s_progressRange;
	static long s_progressStep;
public:

	static void ProgressBarInit(const TCHAR* txt, UINT range);
	static void ProgressBarStep();
	static void ProgressBarHide();

	void SetStatusText(const TCHAR* txt);

	BOOL InspectFolder(CString path);
	void UpdateNotExisting(bool background = false);

	void ExecuteFileOperation(SHFILEOPSTRUCT& fileOp)
	{
		this->EnableWindow(FALSE);
		int rc = SHFileOperation(&fileOp);
		this->EnableWindow(TRUE);
		this->UpdateNotExisting();
	}

	CFileList m_filelist;
	EHDirSize  m_dirsize;
	EHTypeStat m_typestat;

	void FillDB();
	void RefreshData();

	unsigned __int64 m_size;
	unsigned long m_count;

	// Generierte Funktionen für die Meldungstabellen
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnScanFolder();
	afx_msg void OnSettings();
	afx_msg void OnTwinfind();
	afx_msg void OnFileDb();
	afx_msg void OnDoFileList();

	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnUpdateIsActive(CCmdUI *pCmdUI);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSetFocus(CWnd* pOldWnd);

	afx_msg void OnOptionsPerc();
	afx_msg void OnUpdateOptionsPerc(CCmdUI *pCmdUI);
	afx_msg void OnOptionsMb();
	afx_msg void OnUpdateOptionsMb(CCmdUI *pCmdUI);
	afx_msg void OnOptionsShowFilename();
	afx_msg void OnUpdateOptionsShowFilename(CCmdUI *pCmdUI);

	afx_msg void OnView(UINT nID);
	afx_msg void OnClose();

	virtual BOOL PreTranslateMessage(MSG* pMsg);

	afx_msg void OnDatabase();
	afx_msg void OnUpdateDatabase(CCmdUI *pCmdUI);
	afx_msg void OnRescan();
	virtual BOOL DestroyWindow();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};