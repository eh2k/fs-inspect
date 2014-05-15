/*
* Copyright (c) 2005-2010 Eduard Heidt http://fs-inspect.sourceforge.net.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#include "stdafx.h"
#include "LicenseDlg.h"

// SettingsPage dialog

IMPLEMENT_DYNAMIC(CLicenseDialog, CPropertyPage)
CLicenseDialog::CLicenseDialog()
: CDialog(CLicenseDialog::IDD)
{
}

CLicenseDialog::~CLicenseDialog()
{
}

BEGIN_MESSAGE_MAP(CLicenseDialog, CDialog)
END_MESSAGE_MAP()


BOOL CLicenseDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	HMODULE module = ::GetModuleHandle(NULL);
	HRSRC res = FindResource( module, (LPCTSTR)IDR_LICENSE, RT_HTML);
	HGLOBAL res_handle = LoadResource(module, res);

	char *res_data = (char*)LockResource(res_handle);
	DWORD res_size = SizeofResource(module, res);

	CString license(res_data);
	license.SetAt(res_size-1, 0);
	license.Replace(10, _T('ß'));
	license.Replace(_T("ß"), _T("\r\n"));
	

	UnlockResource( res );
	FreeResource( res );

	this->GetDlgItem(IDC_LICENSE)->SetWindowText(license);

	return TRUE;
}
