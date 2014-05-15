/*
* Copyright (c) 2005-2010 Eduard Heidt http://fs-inspect.sourceforge.net.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#include "stdafx.h"
#include "MainFrm.h"
#include "ScanDlg.h"
#include "fs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CMainFrame Diagnose

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWndEx::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWndEx::Dump(dc);
}

#endif //_DEBUG

//----------------------------------------------------------------------------------------------------
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CFrameWndEx);

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWndEx)
	ON_WM_CREATE()

	ON_COMMAND_RANGE(ID_VIEW_SPLIT, ID_VIEW_FILELIST, OnView)

	ON_WM_SIZE()
	ON_WM_CLOSE()
	ON_WM_KEYDOWN()
	ON_WM_SETFOCUS()

	ON_COMMAND(ID_OPTIONS_PERC, OnOptionsPerc)
	ON_UPDATE_COMMAND_UI(ID_OPTIONS_PERC, OnUpdateOptionsPerc)
	ON_COMMAND(ID_OPTIONS_MB, OnOptionsMb)
	ON_UPDATE_COMMAND_UI(ID_OPTIONS_MB, OnUpdateOptionsMb)
	ON_COMMAND(ID_OPTIONS_FILENAME, OnOptionsShowFilename)
	ON_UPDATE_COMMAND_UI(ID_OPTIONS_FILENAME, OnUpdateOptionsShowFilename)

	ON_COMMAND(MAINFRAME_REFRESHDATA, RefreshData)
	ON_COMMAND(MAINFRAME_DOFILELIST, OnDoFileList)
	ON_COMMAND(MAINFRAME_WEBUPDATE, OnWebUpdateChecked)
	ON_COMMAND(MAINFRAME_PROGRESSBAR_INIT, OnProgressBarInit)
	ON_COMMAND(MAINFRAME_PROGRESSBAR_STEP, OnProgressBarStep)

	ON_COMMAND(ID_SCANFOLDER, OnScanFolder)
	ON_COMMAND(ID_TWINFIND, OnTwinfind)
	ON_COMMAND(ID_RESCAN, OnRescan)
	ON_COMMAND(ID_SETTINGS, OnSettings)
	ON_UPDATE_COMMAND_UI(ID_SETTINGS, OnUpdateIsActive)
	ON_UPDATE_COMMAND_UI(ID_RESCAN, OnUpdateIsActive)
	ON_UPDATE_COMMAND_UI(ID_SCANFOLDER, OnUpdateIsActive)
	ON_UPDATE_COMMAND_UI(ID_TWINFIND, OnUpdateIsActive)

	ON_COMMAND(ID_DATABASE, OnDatabase)
	ON_UPDATE_COMMAND_UI(ID_DATABASE, OnUpdateDatabase)
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // Statusleistenanzeige
	ID_INDICATOR_1,
};


// CMainFrame Erstellung/Zerstörung

CMainFrame::CMainFrame():
	m_bkgLeft(RGB(255, 255, 240)), 
	m_bkgRight(RGB(255, 248, 240)),
	m_bkgRightTop(::GetSysColor(COLOR_BTNFACE)),
	m_wndSplitterHorz(2, SSP_HORZ, 30, 3),
	m_wndSplitterVert(2, SSP_VERT, 50, 3),
	m_view(ID_VIEW_SPLIT),
	m_hUpdateCheckThread(NULL)
{
	this->m_size = 0;
	this->m_count = 0;

	InitializeCriticalSection(&m_cs);
}

CMainFrame::~CMainFrame()
{
	if(m_hScanThread)
	{
		HANDLE handle = m_hScanThread;
		m_hScanThread = NULL;
		WaitForSingleObject(handle, 1000);
		CloseHandle(handle);
	}

	DeleteCriticalSection(&m_cs);
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	RECT *rect;
	UINT nl;
	if(theApp.GetProfileBinary(_T("MainFrame"), _T("RECT"), (LPBYTE*)&rect, &nl))
	{
		cs.cx = rect->left - rect->right;
		cs.cy = rect->bottom - rect->top;
		delete [] rect;
	}
	else
	{
		cs.cx = 984;
		cs.cy = 688;
	}

	return __super::PreCreateWindow(cs);
}

static std::string s_updateCheck;
static DWORD WINAPI UpdateCheckThreadProc(LPVOID pThis)
{
	std::string CheckForUpdate(const char* httpUrl);
	s_updateCheck  = CheckForUpdate(UPDATECHECK_URL);
	theApp.GetMainFrame()->PostMessage(WM_COMMAND, MAINFRAME_WEBUPDATE, 0);
	return 0;
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows7));

	this->SetIcon(theApp.LoadIcon(IDR_MAINFRAME), true);  // Icon Laden...

	this->m_sqldlg.Create(IDD_SQLITE, this);

	if (CFrameWndEx::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_wndStatusBar.Create(this) || !m_wndStatusBar.SetIndicators(indicators,sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Statusleiste konnte nicht erstellt werden\n");
		return -1;      // Fehler bei Erstellung
	}

	m_wndStatusBar.SetPaneWidth(1, 0);

	if(m_hyperlink.Create(_T("FS-Inspect Website"), WS_CHILD|WS_VISIBLE, CRect(0,0,150,10), &m_wndStatusBar, -1 ))
	{
		m_hyperlink.SetURL(CString(PROJECT_URL));
		m_hyperlink.SizeToContent();
	}

	EnableDocking(CBRS_ALIGN_ANY );

	if (m_wndRibbonBar.Create(this))
	{
		m_wndRibbonBar.SetWindows7Look(TRUE);

		//auto pMainPanel = m_wndRibbonBar.AddMainCategory(_T("Start"), IDR_MAINFRAME, IDR_MAINFRAME);
		//pMainPanel->AddToolBar(IDR_MAINFRAME);

		if(auto home = m_wndRibbonBar.AddCategory(_T("Start"), IDR_MAINFRAME, IDR_MAINFRAME))
		{
			if(auto panel = home->AddPanel(_T("File")))
			{
				panel->Add(new CMFCRibbonButton( ID_SCANFOLDER, _T("Inspect..."), 0, 0));
				panel->Add(new CMFCRibbonButton( ID_TWINFIND, _T("DupFind..."), 1, 1));
				panel->Add(new CMFCRibbonButton( ID_RESCAN, _T("Refresh"), 2, 2));
				panel->Add(new CMFCRibbonButton( ID_DATABASE, _T("SQL"), 3, 3));
				panel->Add(new CMFCRibbonButton( ID_SETTINGS, _T("Settings"), 4, 4));
			}

			if(auto view = home->AddPanel(_T("View Options")))
			{

				view->Add(new CMFCRibbonCheckBox( ID_OPTIONS_MB, _T("Show Sizes in MB")));
				view->Add(new CMFCRibbonCheckBox( ID_OPTIONS_PERC, _T("Show %-bar relativ to DiskSpace")));
				view->Add(new CMFCRibbonCheckBox( ID_OPTIONS_FILENAME, _T("Show full paths in FileList")));
			}

			if(auto info = home->AddPanel(_T("Info")))
				info->Add(new CMFCRibbonButton( ID_APP_ABOUT, _T("About"), 5, 5));
		}
	}

	HCURSOR crsArrow = ::LoadCursor(0, IDC_ARROW);
	CString classLeft = AfxRegisterWndClass(CS_DBLCLKS, crsArrow, (HBRUSH)m_bkgLeft, 0);
	CString classRight = AfxRegisterWndClass(CS_DBLCLKS, crsArrow, (HBRUSH)m_bkgRight, 0);
	CString classRightTop =	AfxRegisterWndClass(CS_DBLCLKS, crsArrow, (HBRUSH)m_bkgRightTop, 0);

	m_wndSplitterVert.Create(this);
	m_wndSplitterHorz.Create(&m_wndSplitterVert);

	m_filelist.Create(&m_wndSplitterVert, theApp.m_imagelist);

	this->m_dirsize.Create(&m_wndSplitterHorz, theApp.m_imagelist);
	this->m_typestat.Create(&m_wndSplitterHorz, theApp.m_imagelist);

	int sizes[] = {60,40};
	m_wndSplitterVert.SetPaneSizes(sizes);
	m_wndSplitterVert.SetPane(0, &m_wndSplitterHorz);
	m_wndSplitterVert.SetPane(1, &this->m_filelist);

	m_wndSplitterHorz.SetPane(0, &this->m_dirsize);
	m_wndSplitterHorz.SetPane(1, &this->m_typestat);

	DWORD dwThreadID = 0;
	m_hUpdateCheckThread = ::CreateThread(NULL, 0, UpdateCheckThreadProc, this, THREAD_PRIORITY_HIGHEST, &dwThreadID);

	UpdateNotExisting(true);

	return 0;
}

void CMainFrame::OnScanFolder()
{
	if(m_view!=ID_VIEW_SPLIT)
		this->OnView(ID_VIEW_SPLIT);

	UINT ulFlags = /* BIF_RETURNFSANCESTORS | */ BIF_RETURNONLYFSDIRS /*|  BIF_NEWDIALOGSTYLE*/  | BIF_EDITBOX;
	CString path = theApp.BrowseFolder(_T("Select a directory..."), ulFlags);

	if(!path.IsEmpty())
	{
		InspectFolder(path);
		//OnTwinfind();
	}
}

BOOL CMainFrame::InspectFolder(CString path)
{
	if(path.Find(_T("\"")))
		path.Replace('\"','\\');


	this->SetWindowText( APPNAME + _T(" - ") + path);

	BOOL ret = TRUE;
	theApp.m_scanpath = path;
	DWORD dwThreadID = 0;

	struct Proc
	{
		static DWORD WINAPI ScanFolderThreadProc(LPVOID pThis)
		{
			WaitForSingleObject( CScanDlg::GetInstance().m_isModal, INFINITE);

			EHDirSize* me = (EHDirSize*)pThis;
			me->ScanFolder(theApp.m_scanpath, TVI_ROOT);

			theApp.GetMainFrame()->ShowWindow(SW_SHOW);

			if(theApp.m_settings.GetDupSearchOnStartup() && CScanDlg::GetInstance().IsActive())
			{
				theApp.GetMainFrame()->SendMessage(WM_COMMAND, ID_TWINFIND);
				CScanDlg::GetInstance().CenterWindow(theApp.GetMainFrame());
			}
			else
				CScanDlg::GetInstance().SendMessage(WM_COMMAND,IDOK);

			return 0;
		}
	};

	m_hScanThread = ::CreateThread(NULL, 0, Proc::ScanFolderThreadProc, &this->m_dirsize, THREAD_PRIORITY_NORMAL, &dwThreadID);

	if(CScanDlg::GetInstance().DoModal()==IDCANCEL)
	{
		if(this->IsWindowVisible() == FALSE)
			ret = FALSE;

		WaitForSingleObject(m_hScanThread, 1000);
		TerminateThread(m_hScanThread, 0);
	}

	theApp.m_settings.IncUseCount();
	this->m_dirsize.SetFocus();

	return ret;

}

void CMainFrame::OnUpdateIsActive(CCmdUI *pCmdUI)
{
	pCmdUI->Enable( this->m_view != ID_VIEW_DB);
}

void CMainFrame::OnSettings()
{
	theApp.OnSettingsDlg();
	this->RefreshData();
}

void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
	CFrameWndEx::OnSize(nType, cx, cy);

	if(!this->IsWindowVisible())
		return;

	CRect r;
	this->UpdateHyperlinkPos();

	this->m_wndSplitterVert.GetWindowRect(&r);
	ScreenToClient(&r);

	switch(this->m_view)
	{
	case ID_VIEW_SPLIT:

		this->m_dirsize.SetParent(&this->m_wndSplitterHorz);
		this->m_typestat.SetParent(&this->m_wndSplitterHorz);
		this->m_filelist.SetParent(&this->m_wndSplitterVert);

		this->m_wndSplitterHorz.ShowWindow(SW_SHOW);
		this->m_wndSplitterVert.ShowWindow(SW_SHOW);

		this->m_dirsize.Maximize(FALSE);
		this->m_typestat.Maximize(FALSE);
		this->m_filelist.Maximize(FALSE);

		this->m_wndSplitterVert.RecalcLayout();
		this->m_wndSplitterVert.ResizePanes();
		this->m_wndSplitterHorz.RecalcLayout();
		this->m_wndSplitterHorz.ResizePanes();

		this->m_sqldlg.ShowWindow(SW_HIDE);

		break;

	case ID_VIEW_DIRSIZE:

		this->m_dirsize.SetParent(this);

		this->m_dirsize.Maximize();
		this->m_typestat.Maximize(FALSE);
		this->m_filelist.Maximize(FALSE);

		this->m_wndSplitterHorz.ShowWindow(SW_HIDE);
		this->m_wndSplitterVert.ShowWindow(SW_HIDE);

		this->m_sqldlg.ShowWindow(SW_HIDE);

		this->m_dirsize.MoveWindow(r.left, r.top, r.Width(), r.Height());

		break;

	case ID_VIEW_TYPESTAT:

		this->m_typestat.SetParent(this);

		this->m_dirsize.Maximize(FALSE);
		this->m_typestat.Maximize();
		this->m_filelist.Maximize(FALSE);

		this->m_wndSplitterHorz.ShowWindow(SW_HIDE);
		this->m_wndSplitterVert.ShowWindow(SW_HIDE);

		this->m_sqldlg.ShowWindow(SW_HIDE);

		this->m_typestat.MoveWindow(r.left, r.top, r.Width(), r.Height());

		break;

	case ID_VIEW_FILELIST:

		this->m_filelist.SetParent(this);

		this->m_dirsize.Maximize(FALSE);
		this->m_typestat.Maximize(FALSE);
		this->m_filelist.Maximize();

		this->m_wndSplitterHorz.ShowWindow(SW_HIDE);
		this->m_wndSplitterVert.ShowWindow(SW_HIDE);

		this->m_sqldlg.ShowWindow(SW_HIDE);

		this->m_filelist.MoveWindow(r.left, r.top, r.Width(), r.Height());

		break;

	case ID_VIEW_DB:

		this->m_wndSplitterHorz.ShowWindow(SW_HIDE);
		this->m_wndSplitterVert.ShowWindow(SW_HIDE);

		this->m_sqldlg.ShowWindow(SW_SHOW);
		this->m_sqldlg.SetFocus();

		this->m_sqldlg.MoveWindow(r.left, r.top, r.Width(), r.Height());
		break;
	}
}

void CMainFrame::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar == VK_TAB)
	{
		this->m_dirsize.SetFocus();
		return;
	}

	CFrameWndEx::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CMainFrame::OnSetFocus(CWnd* pOldWnd)
{
	CFrameWndEx::OnSetFocus(pOldWnd);

	if(this->m_dirsize)
	{
		if(this->m_dirsize.IsWindowVisible())
			this->m_dirsize.SetFocus();
		else if(this->m_typestat.IsWindowVisible())
			this->m_typestat.SetFocus();
		else if(this->m_filelist.IsWindowVisible())
			this->m_filelist.SetFocus();
	}
}

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
	if(pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE)
		CScanDlg::GetInstance().Stop();

	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_TAB)
	{
		HWND focused = CWnd::GetFocus()->GetParent()->GetSafeHwnd();
		if(focused == this->m_filelist.GetSafeHwnd())
			m_dirsize.SetFocus();

		if (focused== this->m_dirsize.GetSafeHwnd())
			m_typestat.SetFocus();

		if (focused == this->m_typestat.GetSafeHwnd())
			m_filelist.SetFocus();
	}

	return CFrameWndEx::PreTranslateMessage(pMsg);
}

void CMainFrame::OnOptionsPerc()
{
	theApp.m_settings.SetViewPerc(!theApp.m_settings.GetViewPerc());
	this->RefreshData();
}

void CMainFrame::OnUpdateOptionsPerc(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck( theApp.m_settings.GetViewPerc() );
}

void CMainFrame::OnOptionsMb()
{
	theApp.m_settings.SetViewMB(!theApp.m_settings.GetViewMB());
	this->RefreshData();
}

void CMainFrame::OnUpdateOptionsMb(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(theApp.m_settings.GetViewMB());
}

void CMainFrame::OnUpdateOptionsShowFilename(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(theApp.m_settings.GetViewPath());
}

void CMainFrame::OnOptionsShowFilename()
{
	theApp.m_settings.SetViewPath(!theApp.m_settings.GetViewPath());
	this->m_filelist.RefreshData();
}

void CMainFrame::FillDB()
{
	if(this->m_dirsize.m_root)
		this->m_dirsize.m_root->DB_EXPORT(_T(""));
}

void CMainFrame::OnTwinfind()
{
	if(m_view!=ID_VIEW_SPLIT)
		this->OnView(ID_VIEW_SPLIT);

	this->m_typestat.DoTwinCheckAsThread();
}

void CMainFrame::OnDoFileList()
{
	if(this->m_view != ID_VIEW_SPLIT)
		this->OnView(ID_VIEW_FILELIST);
}

void CMainFrame::RefreshData()
{
	this->m_dirsize.RefreshData();
	this->m_typestat.RefreshData();
	this->m_filelist.RefreshData();
}

void CMainFrame::SetStatusText(const TCHAR* txt)
{
	m_wndStatusBar.SetWindowText(txt);
}

void CMainFrame::OnClose()
{
	CScanDlg::GetInstance().Stop();

	//WaitForSingleObject(this->m_dirsize.m_hScanThread, 1000);
	//WaitForSingleObject(this->m_typestat.m_hScanThread, 1000);

	//TerminateThread(this->m_dirsize.m_hScanThread, 0);
	//TerminateThread(this->m_typestat.m_hScanThread, 0);

	theApp.m_db.close();

	WINDOWPLACEMENT wp;
	RECT rect;
	this->GetWindowRect(&rect);
	theApp.WriteProfileBinary(_T("MainFrame"), _T("SIZE"), (LPBYTE)&rect, sizeof(rect));
	this->GetWindowPlacement(&wp);
	theApp.WriteProfileBinary(_T("MainFrame"), _T("WP"), (LPBYTE)&wp, sizeof(wp));
	CFrameWndEx::OnClose();
}

void CMainFrame::OnView(UINT nID)
{
	if(nID == ID_VIEW_DB  && m_view != ID_VIEW_DB)
		this->FillDB();

	if(m_view == nID)
		nID = ID_VIEW_SPLIT;

	m_view = nID;
	CRect r;
	this->OnSize(0,0,0);
}

void CMainFrame::OnDatabase()
{
	this->OnView(ID_VIEW_DB);
}

void CMainFrame::OnUpdateDatabase(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_view == ID_VIEW_DB);
	pCmdUI->Enable(TRUE);
}

void CMainFrame::OnRescan()
{
	m_filelist.DeleteAllItems();
	InspectFolder(theApp.m_scanpath);
}

BOOL CMainFrame::DestroyWindow()
{
	return CFrameWndEx::DestroyWindow();
}

long CMainFrame::s_progressRange = 0;
long CMainFrame::s_progressStep = 0;

void CMainFrame::ProgressBarInit(const TCHAR* txt, UINT range)
{
	s_progressStep = 0;
	s_progressRange = range;
	theApp.GetMainFrame()->SetStatusText(txt);
	theApp.GetMainFrame()->SendMessage(WM_COMMAND, MAINFRAME_PROGRESSBAR_INIT);

}
void CMainFrame::ProgressBarStep()
{
	theApp.GetMainFrame()->SendMessage(WM_COMMAND, MAINFRAME_PROGRESSBAR_STEP);
}
void CMainFrame::ProgressBarHide()
{
	s_progressRange = -1;
	theApp.GetMainFrame()->SendMessage(WM_COMMAND, MAINFRAME_PROGRESSBAR_INIT);
}

void CMainFrame::OnProgressBarInit()
{
	if(s_progressRange > 0)
	{
		m_wndStatusBar.SetPaneWidth(1, 200);
		m_wndStatusBar.EnablePaneProgressBar(1, s_progressRange, TRUE);
		m_hyperlink.ShowWindow(SW_HIDE);
	}
	else
	{
		m_wndStatusBar.EnablePaneProgressBar(1, -1, TRUE);
		m_wndStatusBar.SetPaneWidth(1, 0);
		m_hyperlink.ShowWindow(SW_SHOW);
	}
}

void CMainFrame::OnProgressBarStep()
{
	m_wndStatusBar.SetPaneProgress(1, s_progressStep++);
}

void CMainFrame::OnWebUpdateChecked()
{
	if(m_hUpdateCheckThread)
		CloseHandle(m_hUpdateCheckThread);

	size_t a = s_updateCheck.find('\t');
	size_t b = s_updateCheck.find('\t', a+1);

	if(a < b)
	{
		std::string version = s_updateCheck.substr(0, a );
		std::string text = s_updateCheck.substr(a+1, b-a );
		std::string url = s_updateCheck.substr(b+1, s_updateCheck.size()-b );

		if(version.compare(CURRENT_VERSION) != 0)
		{
			m_hyperlink.SetWindowText(CString(text.c_str()));
			m_hyperlink.SetURL(CString(url.c_str()));
			m_hyperlink.SizeToContent();
		}

		UpdateHyperlinkPos();
	}
}

void CMainFrame::UpdateHyperlinkPos()
{
	if(::IsWindow(m_wndStatusBar.GetSafeHwnd()))
	{
		CRect r, d;

		UINT dummy = 0;
		int cxWidth = 0;
		m_hyperlink.GetWindowRect(d);
		m_wndStatusBar.GetClientRect(&r);
		r.left  = r.right - r.Height() - d.Width();
		r.right = r.right - r.Height();
		r.OffsetRect( -10, 0 );
		m_hyperlink.MoveWindow( r );
	}
}

BOOL CMainFrame::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;	//return CFrameWndEx::OnEraseBkgnd(pDC);
}

void CMainFrame::UpdateNotExisting(bool background)
{
	struct thread
	{
		static DWORD WINAPI proc(LPVOID pThis)
		{
			auto _this = (CMainFrame*)pThis;

			if(::IsWindow(_this->GetSafeHwnd()))
			{
				HTREEITEM p = _this->m_typestat.GetRootItem();
				while(p != NULL)
				{
					auto before = p;
					p = _this->m_typestat.GetNextItem(p,TVGN_NEXT);
					_this->m_typestat.SendMessage(ID_RESCAN, (WPARAM)before);
				}

				p = _this->m_dirsize.GetRootItem();
				while(p != NULL)
				{
					if(_this->m_dirsize.SendMessage(ID_RESCAN, (WPARAM)p))
						p = _this->m_dirsize.GetRootItem();
					else
						p = _this->m_dirsize.GetNextItem(p, TVGN_NEXTVISIBLE);
				}
			}

			return 0;
		}
	};

	if(!background)
	{	
		thread::proc(this);
		//DWORD dwThreadID = 0;
		//m_hScanThread = ::CreateThread(NULL, 0, thread::proc, this, THREAD_PRIORITY_NORMAL, &dwThreadID);
	}
}