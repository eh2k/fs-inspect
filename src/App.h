/*
* Copyright (c) 2005-2010 Eduard Heidt http://fs-inspect.sourceforge.net.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#pragma once

#ifndef __AFXWIN_H__
	#error 'stdafx.h' muss vor dieser Datei in PCH eingeschlossen werden.
#endif

#include "resource.h"       // Hauptsymbole
#include "CppSQLite3U.h"
#include <vector>
#include <map>

static const char* CURRENT_VERSION = "1.7";
static const char* PROJECT_URL = "https://github.com/eh2k/fs-inspect";
static const char* UPDATECHECK_URL = "https://raw.githubusercontent.com/eh2k/fs-inspect/master/UpdateInfo.txt";

struct ScopedNamedMutex
{
	HANDLE _mutex;

	ScopedNamedMutex(const TCHAR* name)
	{
		_mutex = ::CreateMutex(NULL, FALSE, name);
		::WaitForSingleObject(_mutex, INFINITE);
	}
	~ScopedNamedMutex()
	{
		::ReleaseMutex(_mutex);
		::CloseHandle(_mutex);
	}
};

class Settings
{
	std::map<int,CString> m_values;
public:
	CString GetStringValue(int n);
	BOOL SetStringValue(int n, CString value);
	
	int GetIntValue(int n)
	{ 
		return _wtoi( GetStringValue(n) );
	}
	int SetIntValue(int n, int nValue)
	{
		CString i; i.Format(_T("%d"), nValue); return SetStringValue(n, i);}

	BOOL GetBoolValue(int n)
	{
		return GetIntValue(n);
	}
	BOOL SetBoolValue(int n, BOOL enabled)
	{
		return SetIntValue(n, enabled);
	}

	BOOL GetCmpNameL(){ return GetBoolValue(0);}
	BOOL SetCmpNameL(BOOL enabled){ return SetBoolValue(0, enabled);}
	BOOL GetCmpSizeL(){ return GetBoolValue(1);}
	BOOL SetCmpSizeL(BOOL enabled){ return SetBoolValue(1, enabled);}
	BOOL GetCmpDateL(){ return GetBoolValue(2);}
	BOOL SetCmpDateL(BOOL enabled){ return SetBoolValue(2, enabled);}
	BOOL GetCmpHashL(){ return GetBoolValue(3);}
	BOOL SetCmpHashL(BOOL enabled){ return SetBoolValue(3, enabled);}

	BOOL GetCmpNameN(){ return GetBoolValue(4);}
	BOOL SetCmpNameN(BOOL enabled){ return SetBoolValue(4, enabled);}
	BOOL GetCmpSizeN(){ return GetBoolValue(5);}
	BOOL SetCmpSizeN(BOOL enabled){ return SetBoolValue(5, enabled);}
	BOOL GetCmpDateN(){ return GetBoolValue(6);}
	BOOL SetCmpDateN(BOOL enabled){ return SetBoolValue(6, enabled);}
	BOOL GetCmpHashN(){ return GetBoolValue(7);}
	BOOL SetCmpHashN(BOOL enabled){ return SetBoolValue(7, enabled);}

	BOOL GetModeADS(){ return GetBoolValue(8);}
	BOOL SetModeADS(BOOL enabled){ return SetBoolValue(8, enabled);}

	BOOL GetViewPath(){ return !GetBoolValue(9);}
	BOOL SetViewPath(BOOL enabled){ return SetBoolValue(9, !enabled);}

	BOOL GetViewMB(){ return GetBoolValue(10);}
	BOOL SetViewMB(BOOL enabled){ return SetBoolValue(10, enabled);}

	BOOL GetViewPerc(){ return GetBoolValue(11);}
	BOOL SetViewPerc(BOOL enabled){ return SetBoolValue(11, enabled);}

	int GetUseCount(){ return GetIntValue(12);}
	int IncUseCount(){ return SetIntValue(12, GetUseCount()+1);}
	
	CString GetTwinTypes(){ return GetStringValue(13);}
	BOOL SetTwinTypes(CString types){ return SetStringValue(13, types);}

	BOOL GetShell(){ return GetBoolValue(14);}
	BOOL SetShell(BOOL enabled){ return SetBoolValue(14, enabled);}

	CString GetDateTimeFormat(){ return GetStringValue(14);}
	BOOL SetDateTimeFormat(CString format){ return SetStringValue(14, format);}

	BOOL GetDupSearchOnStartup(){ return GetBoolValue(15);}
	BOOL SetDupSearchOnStartup(BOOL enabled){ return SetBoolValue(15, enabled);}

	void Reset()
	{
		SetCmpNameL(FALSE);
		SetCmpSizeL(TRUE);
		SetCmpDateL(FALSE);
		SetCmpHashL(TRUE);

		SetCmpNameN(TRUE);
		SetCmpSizeN(TRUE);
		SetCmpDateN(FALSE);
		SetCmpHashN(FALSE);

		SetModeADS(FALSE);

		SetDupSearchOnStartup(TRUE);

		SetShell(FALSE);

		SetTwinTypes(_T("cpp|c|h|htm*|xml|css|js|vbs|php|java|cs|vb|pdf|doc|jpg|gif|bmp|png|zip|rar|mp3|"));
		SetDateTimeFormat(_T("%d.%m.%y  %H:%M "));
	}
};

class CMainFrame;
class CScanDlg;

class CApp : public CWinApp
{
public:
	CApp();
	CImageList *m_imagelist;
	HANDLE treesize_ready;

	Settings m_settings;

public:
	virtual BOOL InitInstance();

	CString m_tmp;
	DWORD m_time;

	CString m_scanpath;
	int m_exceptions;

	CMainFrame* GetMainFrame()
	{
		return (CMainFrame*)m_pMainWnd;
	}

	CppSQLite3DB m_db;

	void StartTimer(){m_time = ::GetTickCount();}
	void StopTimer(){m_time = ::GetTickCount()-m_time;}

	CString BrowseFolder(CString title = _T("Select a Directory.."),UINT ulFlags = BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_EDITBOX /*| BIF_NONEWFOLDERBUTTON*/);

	virtual BOOL OnIdle(LONG lCount);

	void OnAboutDlg();
	void OnSettingsDlg();
	
	CString GetAppPath() const;
	CString GetDBPath() const;

public:
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
};


extern CApp theApp;