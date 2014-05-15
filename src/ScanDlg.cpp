/*
* Copyright (c) 2005-2010 Eduard Heidt http://fs-inspect.sourceforge.net.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#include "stdafx.h"
#include "ScanDlg.h"

BEGIN_MESSAGE_MAP(CScanDlg, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
	ON_WM_CLOSE()
	ON_MESSAGE(ID_SET_FILES, OnSetFiles)
	ON_MESSAGE(ID_SET_PROGRESS, OnSetProgress)
	ON_MESSAGE(ID_SET_PROGRESS2, OnSetProgress2)
END_MESSAGE_MAP()

CScanDlg::CScanDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CScanDlg::IDD, pParent)
{
	m_isModal = ::CreateEvent( NULL, TRUE, FALSE, NULL );
	InitializeCriticalSection(&m_cs);
}

CScanDlg::~CScanDlg()
{
	::CloseHandle( m_isModal );
	DeleteCriticalSection(&m_cs);
}

void CScanDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROGRESS1, m_progressCtrl);
	DDX_Control(pDX, IDC_PROGRESS2, m_progressCtrl2);
}

// CScanDlg message handlers

LRESULT CScanDlg::OnSetFiles(WPARAM wp, LPARAM lp)
{
	if(TryEnterCriticalSection(&m_cs))   // |> KRITISCHER BEREICH
	{
		this->GetDlgItem(IDC_FILES)->SetWindowText(m_files);
		this->GetDlgItem(IDC_PATH)->SetWindowText(m_dir);

		LeaveCriticalSection(&m_cs);   // <| KRITISCHER BEREICH
	}
	return 0;
}

inline DWORD GetTickCountEx()
{
	return ::GetTickCount()/10;
}

inline void PostMessageEx(DWORD& tickCount, HWND wnd, UINT msgID, WPARAM wParam, LPARAM lParam)
{
	if(tickCount != GetTickCountEx() || wParam == 0 || wParam == lParam)
	{
		tickCount = GetTickCountEx();

		if(CScanDlg::GetInstance().IsActive())
		{
			MSG msg;
			while(::PeekMessage(&msg, wnd, msgID, msgID, PM_REMOVE));
			::PostMessage(wnd, msgID, wParam, lParam);
		}
	}
}

void CScanDlg::SetProgress(int value, int max)
{
	static DWORD s_tickCount = 0;
	PostMessageEx(s_tickCount, this->GetSafeHwnd(), ID_SET_PROGRESS, value, max);
}

void CScanDlg::SetProgress2(int value, int max)
{
	static DWORD s_tickCount = 0;
	PostMessageEx(s_tickCount, this->GetSafeHwnd(), ID_SET_PROGRESS2, value, max);
}

void CScanDlg::SetTexts(const CString& files, const CString& dir)
{
	if(dir.IsEmpty()) //Force Text Update
	{
		EnterCriticalSection(&m_cs);   // |> KRITISCHER BEREICH

		m_files = files;
		m_dir = dir;

		LeaveCriticalSection(&m_cs);   // <| KRITISCHER BEREICH

		static DWORD s_tickCount = 0;
		s_tickCount = -1;
		PostMessageEx(s_tickCount, this->GetSafeHwnd(), ID_SET_FILES, 1, 2);
	}
	else if(TryEnterCriticalSection(&m_cs))   // |> KRITISCHER BEREICH
	{
		m_files = files;
		m_dir = dir;

		LeaveCriticalSection(&m_cs);   // <| KRITISCHER BEREICH

		static DWORD s_tickCount = 0;
		PostMessageEx(s_tickCount, this->GetSafeHwnd(), ID_SET_FILES, 1, 2);
	}
}

LRESULT CScanDlg::OnSetProgress(WPARAM wp, LPARAM lp)
{
	if((int)lp == 0 || (int)wp == (int)lp)
	{
		m_progressCtrl.ShowWindow(SW_HIDE);
	}
	else
	{
		m_progressCtrl.ShowWindow(SW_SHOW);
		m_progressCtrl.SetRange32(0, (int)lp);
		m_progressCtrl.SetPos((int)wp);
	}

	return 0;
}

LRESULT CScanDlg::OnSetProgress2(WPARAM wp, LPARAM lp)
{
	if((int)lp == 0 || (int)wp == (int)lp)
	{
		m_progressCtrl2.ShowWindow(SW_HIDE);
	}
	else
	{
		m_progressCtrl2.ShowWindow(SW_SHOW);
		m_progressCtrl2.SetRange32(0, (int)lp);
		m_progressCtrl2.SetPos((int)wp);
	}

	return 0;
}

void CScanDlg::OnBnClickedOk()
{
	OnOK();
}

void CScanDlg::OnBnClickedCancel()
{
	OnCancel();
}

void CScanDlg::OnClose()
{
	::ResetEvent(this->m_isModal);
	__super::OnClose();
}

void CScanDlg::EndModalLoop(int nResult)
{
	::ResetEvent(this->m_isModal);
	__super::EndModalLoop(nResult);
}

BOOL CScanDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	GetDlgItem(IDC_FILES)->SetWindowText(m_text);

	m_progressCtrl.ShowWindow(SW_HIDE);
	m_progressCtrl2.ShowWindow(SW_HIDE);

	::SetEvent(this->m_isModal);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

CScanDlg& CScanDlg::GetInstance()
{
	static CScanDlg s_instance;
	return s_instance;
}
