/*
* Copyright (c) 2005-2010 Eduard Heidt http://fs-inspect.sourceforge.net.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#include "stdafx.h"
#include "app.h"
#include "MainFrm.h"
#include "version.h"

#include "fs.h"
#include "AboutDlg.h"
#include "SettingsDlg.h"
#include "LicenseDlg.h"

#include <vector>
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CApp

BEGIN_MESSAGE_MAP(CApp, CWinApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
END_MESSAGE_MAP()


// CApp-Erstellung

CApp::CApp():
	m_time(0),
	m_tmp(""),
	m_scanpath(""),
	m_exceptions(0),
	m_db()
{
	// TODO: Hier Code zur Konstruktion einfügen
	// Alle wichtigen Initialisierungen in InitInstance positionieren
}


// Das einzige CApp-Objekt

CApp theApp;



// CApp Initialisierung

static BOOL InitSettings()
{
	if( ::PathFileExists( theApp.GetDBPath() ) )
	{
		theApp.m_db.open( theApp.GetDBPath() );
	}
	else
	{
		CLicenseDialog d;

		if(d.DoModal()== IDCANCEL)
			return FALSE;

		theApp.m_db.open( theApp.GetDBPath() );

		if(!theApp.m_db.tableExists(_T("settings")))
			theApp.m_db.execDML(_T("create table settings(name char(12), value char(32));"));

		theApp.m_settings.Reset();
	}

	if(!theApp.m_db.tableExists(_T("hashing")))
		theApp.m_db.execDML(_T("create table hashing(id char(32) UNIQUE, md5 char(32));"));

	return TRUE;
}

BOOL CApp::InitInstance()
{
	CWinApp::InitInstance();

	HIMAGELIST il_small, il_large;
	Shell_GetImageLists(&il_large, &il_small);
	this->m_imagelist = CImageList::FromHandle(il_small);

	//SystemParametersInfo(SPI_SETDRAGFULLWINDOWS,FALSE,NULL,0);


	if(!InitSettings())
		return FALSE;

	CMainFrame* pFrame = new CMainFrame;
	if (!pFrame)
		return FALSE;
	m_pMainWnd = pFrame;

	// Rahmen mit Ressourcen erstellen und laden
	if(pFrame->LoadFrame(IDR_MAINFRAME)) //, WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, NULL,NULL);
	{
		//pFrame->ShowWindow(SW_SHOW);

		CCommandLineInfo CmdInfo;
		theApp.ParseCommandLine(CmdInfo);
		if(!CmdInfo.m_strFileName.IsEmpty())
		{
			if(!pFrame->InspectFolder(CmdInfo.m_strFileName))
				exit(-1); //Force Exit!!!
		}
		else
		{
			pFrame->ShowWindow(SW_SHOW);
			pFrame->PostMessage(WM_COMMAND, ID_SCANFOLDER);
		}

		WINDOWPLACEMENT *lwp;
		UINT nl;
		if(GetProfileBinary(_T("MainFrame"), _T("WP"), (LPBYTE*)&lwp, &nl))
		{
			pFrame->SetWindowPlacement(lwp);
			delete [] lwp;
		}

		return TRUE;
	}
	return FALSE;
}

// Anwendungsbefehl zum Ausführen des Dialogfelds
void CApp::OnAppAbout()
{
	theApp.OnAboutDlg();
}

BOOL CApp::OnIdle(LONG lCount)
{
	ULONGLONG twinsize = 0;
	int twincount = 0;
	GetMainFrame()->m_typestat.GetTwinInformation(twinsize, twincount);

	CString tmp; tmp.Format(_T("Path: %s, Size: %s, Files: %d, File-Types: %d, Duplicates: %d ( %s)"),
		theApp.m_scanpath, // theApp.m_scanpath, 
		FSbase::GetSizeString(GetMainFrame()->m_size),
		GetMainFrame()->m_count,
		GetMainFrame()->m_typestat.m_types.size(),
		twincount,
		FSbase::GetSizeString(twinsize)
		);

	theApp.GetMainFrame()->SetStatusText(tmp);

	return __super::OnIdle(lCount);
}

void CApp::OnAboutDlg()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

void CApp::OnSettingsDlg()
{
	SettingsDlg dlg;
	dlg.DoModal();
}

CString CApp::BrowseFolder(CString title, UINT ulFlags)
{
	CString		sFolder;
	LPMALLOC	pMalloc;

	// Gets the Shell's default allocator
	if (::SHGetMalloc(&pMalloc) == NOERROR)
	{
		BROWSEINFO bi;
		TCHAR pszBuffer[MAX_PATH];
		LPITEMIDLIST pidl;

		bi.hwndOwner = AfxGetMainWnd()->GetSafeHwnd();
		bi.pidlRoot = NULL;
		bi.pszDisplayName = pszBuffer;
		bi.lpszTitle = title;
		bi.ulFlags = ulFlags;
		bi.lpfn = NULL;
		bi.lParam = 0;

		// This next call issues the dialog box.
		if ((pidl = ::SHBrowseForFolder(&bi)) != NULL)
		{
			if (::SHGetPathFromIDList(pidl, pszBuffer))
			{ 
				// At this point pszBuffer contains the selected path
				sFolder = pszBuffer;
			}

			// Free the PIDL allocated by SHBrowseForFolder.
			pMalloc->Free(pidl);
		}
		// Release the shell's allocator.
		pMalloc->Release();
	}

	return sFolder;
}

static bool hasLocatationADS()
{
	TCHAR szVolName[MAX_PATH], szFSName[MAX_PATH];
	DWORD dwSN, dwMaxLen, dwVolFlags;
	if(::GetVolumeInformation( theApp.GetAppPath().Left(3), szVolName, MAX_PATH, &dwSN,&dwMaxLen, &dwVolFlags, szFSName, MAX_PATH))
		return (dwVolFlags & FILE_NAMED_STREAMS)!=0;
	//return (_wcsicmp(szFSName, _T("NTFS")) == 0);

	return false;
}

CString CApp::GetAppPath() const
{
	CString path = GetCommandLine();
	path.MakeLower();
	path = path.Left(path.Find(_T("exe"))+4);
	path.Remove('\"');
	return path;
}

CString CApp::GetDBPath() const
{
	if(0 && hasLocatationADS())
	{
		return GetAppPath() + _T(":db");
	}
	else
	{
		CString ret = GetAppPath();
		return ret.Left(ret.ReverseFind('.')) + _T(".db");
	}
}

/////////////////////////////////////
CString Settings::GetStringValue(int n)
{
	if(m_values.find(n) != m_values.end())
		return m_values[n];
	else
	{
		ScopedNamedMutex(_T("DB"));
		CString sql; sql.Format(_T("SELECT value FROM settings WHERE name = '%d';"), n);
		CppSQLite3Query q = theApp.m_db.execQuery(sql);
		return m_values[n] = q.fieldValue(0);
	}
}

BOOL Settings::SetStringValue(int n, CString value) 
{
	m_values[n] = value;
	ScopedNamedMutex(_T("DB"));
	CString sql; 
	//sql.Format(_T("UPDATE settings SET value = '%s' WHERE name = '%d';"), value, n);
	sql.Format(_T("DELETE FROM settings WHERE name = '%d';"), n);
	theApp.m_db.execDML( sql );
	sql.Format(_T("INSERT INTO settings (name, value) VALUES('%d','%s');"), n, value );
	theApp.m_db.execDML( sql );
	return TRUE;
}
