/*
* Copyright (c) 2005-2010 Eduard Heidt http://fs-inspect.sourceforge.net.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#pragma once

#include "resource.h"
// CSQLiteDialog dialog

class CSQLiteDialog : public CDialog
{
	DECLARE_DYNAMIC(CSQLiteDialog)

public:
	CSQLiteDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSQLiteDialog();

// Dialog Data
	enum { IDD = IDD_SQLITE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CString m_sQuery;
	CListCtrl m_lstTable;
	afx_msg void OnDoQuery();
	CEdit m_editQuery;
	afx_msg void OnSize(UINT nType, int cx, int cy);
	CButton m_btnDoQuery;
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	CButton m_btnExport;
	afx_msg void OnBnClickedExport();
};
