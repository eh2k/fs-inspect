/*
* Copyright (c) 2005-2010 Eduard Heidt http://fs-inspect.sourceforge.net.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#include "stdafx.h"
#include "SettingsDlg.h"

void Settings_get(CWnd *dlg)
{
	//((CButton*)dlg->GetDlgItem(IDC_CHECK2))->SetCheck(theApp.m_settings.GetShell());
	((CButton*)dlg->GetDlgItem(IDC_CHECK14))->SetCheck(theApp.m_settings.GetModeADS());

	((CButton*)dlg->GetDlgItem(IDC_CHECK5))->SetCheck(theApp.m_settings.GetCmpNameL());
	((CButton*)dlg->GetDlgItem(IDC_CHECK6))->SetCheck(theApp.m_settings.GetCmpSizeL());
	((CButton*)dlg->GetDlgItem(IDC_CHECK7))->SetCheck(theApp.m_settings.GetCmpDateL());
	((CButton*)dlg->GetDlgItem(IDC_CHECK8))->SetCheck(theApp.m_settings.GetCmpHashL());

	((CButton*)dlg->GetDlgItem( IDC_CHECK9))->SetCheck(theApp.m_settings.GetCmpNameN());
	((CButton*)dlg->GetDlgItem(IDC_CHECK10))->SetCheck(theApp.m_settings.GetCmpSizeN());
	((CButton*)dlg->GetDlgItem(IDC_CHECK11))->SetCheck(theApp.m_settings.GetCmpDateN());
	((CButton*)dlg->GetDlgItem(IDC_CHECK12))->SetCheck(theApp.m_settings.GetCmpHashN());

	((CButton*)dlg->GetDlgItem(IDC_TWINFIND_IN_BACKGROUND))->SetCheck(theApp.m_settings.GetDupSearchOnStartup());
	

	((CEdit*)dlg->GetDlgItem(IDC_EDIT1))->SetWindowText(theApp.m_settings.GetTwinTypes());
	((CEdit*)dlg->GetDlgItem(IDC_DATETIMEFORMAT))->SetWindowText(theApp.m_settings.GetDateTimeFormat());

	((CButton*)dlg->GetDlgItem(IDC_CHECK1))->SetCheck(TRUE);
	((CButton*)dlg->GetDlgItem(IDC_CHECK3))->SetCheck(TRUE);
}


void Settings_set(CWnd *dlg)
{
	//theApp.m_settings.SetShell(((CButton*)dlg->GetDlgItem(IDC_CHECK2))->GetCheck());

	theApp.m_settings.SetCmpNameL(((CButton*)dlg->GetDlgItem(IDC_CHECK5))->GetCheck());
	theApp.m_settings.SetCmpSizeL(((CButton*)dlg->GetDlgItem(IDC_CHECK6))->GetCheck());
	theApp.m_settings.SetCmpDateL(((CButton*)dlg->GetDlgItem(IDC_CHECK7))->GetCheck());
	theApp.m_settings.SetCmpHashL(((CButton*)dlg->GetDlgItem(IDC_CHECK8))->GetCheck());

	theApp.m_settings.SetCmpNameN(((CButton*)dlg->GetDlgItem( IDC_CHECK9))->GetCheck());
	theApp.m_settings.SetCmpSizeN(((CButton*)dlg->GetDlgItem(IDC_CHECK10))->GetCheck());
	theApp.m_settings.SetCmpDateN(((CButton*)dlg->GetDlgItem(IDC_CHECK11))->GetCheck());
	theApp.m_settings.SetCmpHashN(((CButton*)dlg->GetDlgItem(IDC_CHECK12))->GetCheck());

	theApp.m_settings.SetModeADS(((CButton*)dlg->GetDlgItem(IDC_CHECK14))->GetCheck());

	theApp.m_settings.SetDupSearchOnStartup(((CButton*)dlg->GetDlgItem(IDC_TWINFIND_IN_BACKGROUND))->GetCheck());

	CString types;
	((CEdit*)dlg->GetDlgItem(IDC_EDIT1))->GetWindowText(types);
	theApp.m_settings.SetTwinTypes(types);

	((CEdit*)dlg->GetDlgItem(IDC_DATETIMEFORMAT))->GetWindowText(types);
	theApp.m_settings.SetDateTimeFormat(types);
}

// SettingsDlg dialog

IMPLEMENT_DYNAMIC(SettingsDlg, CDialog)
	SettingsDlg::SettingsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(SettingsDlg::IDD, pParent)
{
}

SettingsDlg::~SettingsDlg()
{
}


BEGIN_MESSAGE_MAP(SettingsDlg, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDC_ADDSHELL, &SettingsDlg::OnBnClickedAddshell)
	ON_BN_CLICKED(IDC_REMOVESHELL, &SettingsDlg::OnBnClickedRemoveshell)
	ON_BN_CLICKED(IDC_RESET, &SettingsDlg::OnBnClickedReset)
END_MESSAGE_MAP()

// SettingsDlg message handlers

BOOL SettingsDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	Settings_get(this);

	return TRUE;  // return TRUE unless you set the focus to a control
}

void SettingsDlg::OnBnClickedOk()
{
	Settings_set(this);

	OnOK();
}


// SettingsPage dialog

IMPLEMENT_DYNAMIC(SettingsPage, CPropertyPage)
	SettingsPage::SettingsPage()
	: CPropertyPage(SettingsPage::IDD)
{
}

SettingsPage::~SettingsPage()
{
}

BEGIN_MESSAGE_MAP(SettingsPage, CPropertyPage)
END_MESSAGE_MAP()


BOOL SettingsPage::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	Settings_get(this);

	this->GetDlgItem(IDOK)->ShowWindow(SW_HIDE);
	this->GetDlgItem(IDCANCEL)->ShowWindow(SW_HIDE);


	return TRUE;  // return TRUE unless you set the focus to a control
}

void SettingsPage::OnOK()
{
	CPropertyPage::OnOK();
}

BOOL SettingsPage::OnWizardFinish()
{
	Settings_set(this);
	return CPropertyPage::OnWizardFinish();
}

BOOL SettingsPage::OnSetActive()
{
	((CPropertySheet*)this->GetParent())->SetWizardButtons(PSWIZB_BACK|PSWIZB_FINISH);
	//	((CPropertySheet*)this->GetParent())->SetFinishText(_T("Start"));
	return CPropertyPage::OnSetActive();
}


void SettingsDlg::OnBnClickedAddshell()
{
	CString val = theApp.GetAppPath() + _T(" \"%1\"");
	CString sKeyPath = _T("Software\\Classes\\Folder\\shell\\->") + APPNAME;
	if(::RegSetValue(HKEY_CURRENT_USER, sKeyPath + _T("\\command"), REG_SZ, val, val.GetLength()) == ERROR_SUCCESS)
	{
		CString msg; msg.Format(L"Explorer context menu extention \"%s\" added.", APPNAME);
		AfxMessageBox(msg, MB_ICONINFORMATION);
	}
}


void SettingsDlg::OnBnClickedRemoveshell()
{
	CString sKeyPath = _T("Software\\Classes\\Folder\\shell\\->")+APPNAME;
	if(::RegDeleteKey(HKEY_CURRENT_USER,sKeyPath + _T("\\command")) == ERROR_SUCCESS &&
		::RegDeleteKey(HKEY_CURRENT_USER,sKeyPath) == ERROR_SUCCESS)
	{
		CString msg; msg.Format(L"Explorer context menu extention \"%s\" removed.", APPNAME);
		AfxMessageBox(msg, MB_ICONINFORMATION);
	}
}


void SettingsDlg::OnBnClickedReset()
{
	theApp.m_settings.Reset();
	Settings_get(this);
}
