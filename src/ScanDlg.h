/*
* Copyright (c) 2005-2010 Eduard Heidt http://fs-inspect.sourceforge.net.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#pragma once
#include "afxcmn.h"

class CScanDlg : public CDialog
{
	CRITICAL_SECTION m_cs;

	CString m_text;
	CString m_files;
	CString m_dir;
	CProgressCtrl m_progressCtrl;
	CProgressCtrl m_progressCtrl2;

	CScanDlg(CWnd* pParent = NULL);

public:
	virtual ~CScanDlg();

// Dialog Data
	enum { IDD = IDD_SCAN2 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void EndModalLoop(int nResult);

	afx_msg LRESULT OnSetFiles(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnSetProgress(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnSetProgress2(WPARAM wp, LPARAM lp);
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnClose();

	HANDLE m_isModal;

	bool IsActive()
	{
		return WaitForSingleObject( m_isModal, 0) == WAIT_OBJECT_0;
	}

	void Stop()
	{
		if(::IsWindow(this->GetSafeHwnd()))
			SendMessage(WM_COMMAND,IDOK);
	}

	static CScanDlg& GetInstance();

	void SetTexts(const CString& top, const CString& bottom);
	void SetProgress(int value, int max);
	void SetProgress2(int value, int max);
	virtual BOOL OnInitDialog();
};
