/*
* Copyright (c) 2005-2010 Eduard Heidt http://fs-inspect.sourceforge.net.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#include "stdafx.h"
#include "aboutdlg.h"
#include "version.h"

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	ON_WM_CREATE()
END_MESSAGE_MAP()

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD), m_version(_T("")), m_strWebpage(_T(""))
{
	this->m_strWebpage = CString(PROJECT_URL);
	this->m_version.Format(_T("%s %s - Build %s"), APPNAME, CString(CURRENT_VERSION), _T(VERSION_TIMESTAMP));
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_WEBPAGE, m_webpage);
	DDX_Text(pDX, IDC_VERSION, m_version);
	DDX_Text(pDX, IDC_WEBPAGE, m_strWebpage);
}

int CAboutDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDialog::OnCreate(lpCreateStruct) == -1)
		return -1;

	this->m_webpage.SetURL(m_strWebpage);
	//	this->m_webpage.SizeToContent();

	return 0;
}

void CAboutDlg::OnBnClickedOk()
{
	OnOK();
}