/*
* Copyright (c) 2005-2010 Eduard Heidt http://fs-inspect.sourceforge.net.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#pragma once

// SettingsDlg dialog

class SettingsDlg : public CDialog
{
	DECLARE_DYNAMIC(SettingsDlg)

public:
	SettingsDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~SettingsDlg();

// Dialog Data
	enum { IDD = IDD_SETTINGS };

protected:
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();

	afx_msg void OnBnClickedAddshell();
	afx_msg void OnBnClickedRemoveshell();
	afx_msg void OnBnClickedReset();
};


class SettingsPage : public CPropertyPage
{
	DECLARE_DYNAMIC(SettingsPage)

public:
	SettingsPage();
	virtual ~SettingsPage();

	// Dialog Data
	enum { IDD = IDD_SETTINGS };

protected:
//	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual BOOL OnWizardFinish();
	virtual BOOL OnSetActive();
};