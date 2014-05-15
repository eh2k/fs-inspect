/*
* Copyright (c) 2005-2010 Eduard Heidt http://fs-inspect.sourceforge.net.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#include "stdafx.h"
#include <string>
#include <algorithm>
#include <functional>      // For greater<int>( )

#include "fs.h"
#include "MainFrm.h"
#include "FileList.h"
#include "filelist.h"
#include "typestat.h"

BEGIN_MESSAGE_MAP(CFileList, CTreeListCtrl)
	ON_MESSAGE(ID_ADD_FILE, OnAddFile)
END_MESSAGE_MAP()

CFileList::CFileList()
{
	this->m_sort_asc = true;
	this->m_sort_col = 3;
}

CFileList::~CFileList()
{

}

LRESULT CFileList::OnAddFile(WPARAM wp, LPARAM lp)
{
	FSFile* file = (FSFile*)wp;
	this->AddFile(file);
	return 0;
}

BOOL CFileList::Create(CWnd *parent, CImageList *imagelist)
{
	CTreeListCtrl::Create(parent);

	ASSERT(imagelist);
	this->SetImageList(imagelist, TVSIL_NORMAL);

#define FLITEM_NAME 0
#define FLITEM_PATH 0
#define FLITEM_SIZE FLITEM_PATH+1
#define FLITEM_PERC FLITEM_SIZE+1
#define FLITEM_MODI FLITEM_PERC+1
#define FLITEM_HASH FLITEM_MODI+1

	InsertColumn(FLITEM_NAME, _T("File"),LVCFMT_LEFT, 160, TA_LEFT);

	if(FLITEM_PATH) InsertColumn(FLITEM_PATH, _T("Path"),LVCFMT_LEFT, 310, TA_LEFT);
	if(FLITEM_SIZE) InsertColumn(FLITEM_SIZE, _T("Size"),LVCFMT_LEFT, 75,  TA_RIGHT);
	if(FLITEM_PERC) this->m_procentcol = InsertColumn(FLITEM_PERC, _T("%"),LVCFMT_CENTER, 80);
	if(FLITEM_MODI) InsertColumn(FLITEM_MODI, _T("Modified"),LVCFMT_LEFT, 100, TA_RIGHT);
	if(FLITEM_HASH) InsertColumn(FLITEM_HASH, _T("Checksum"),LVCFMT_LEFT, 70, TA_RIGHT);

	return TRUE;
}

void CFileList::AddFile(FSFile *f)
{
	if(IsOwnerThread() == false)
	{
		static int i = 0;
		this->SendMessage(ID_ADD_FILE, (WPARAM)f);
		return;
	}

	f->item = InsertItem(NULL, f->GetIcon(), f->GetIcon(), TVI_ROOT);	
	this->SetItemData(f->item, (DWORD_PTR)f);

	const HASH& h = f->GetHash(false);
	HASH_TREE::iterator it = this->twins.find(h);
	if(it == this->twins.end())
	{
		this->twins[h] = f->item;
	}
	else
	{
		HTREEITEM r = it->second;
		if(h != "")
		{
			UINT c1 = 0xBB+(this->twins.size()*0x10)%0x33;
			UINT c2 = 0xBB+(this->twins.size()*0x5)%0x33;
			UINT c3 = 0xBB+(this->twins.size()*0x15)%0x33;

			if(this->GetItemColor(r) == RGB(0xff,0xff,0xff))
			{
				this->SetItemColor(r, RGB(c1, c2, c3));
				this->UpdateItem(r);
			}
			this->SetItemColor(f->item, RGB(c1, c2, c3));
		}
	}

	UpdateItem(f->item);
}

void CFileList::RefreshData()
{
	HTREEITEM p = this->GetRootItem();
	while(p != NULL)
	{
		this->UpdateItem(p);
		p = this->GetNextItem(p,TVGN_NEXT);
	}
}

void CFileList::OnItemOperation(UINT cmd)
{
	CString target;

	HTREEITEM item = this->GetSelectedItem();

	if(item == NULL)
		return;

	FSFile *f = (FSFile*)this->GetItemData(item);
	CString sFile = f->GetPath();

	switch(cmd)
	{
	case ID_DOHASHES:

		this->UpdateItem(item);
		MessageBoxA( this->GetSafeHwnd(), f->GetHash().c_str(), "MD5", MB_ICONINFORMATION);
		break;

	case ID_OPEN:

		ShellExecute(AfxGetMainWnd()->GetSafeHwnd(), TEXT("open"), sFile, NULL, NULL, SW_SHOW);

		return;
		break;

	case ID_OPENFOLDER:

		//ShellExecute(AfxGetMainWnd()->GetSafeHwnd(), TEXT("open"), sPath, NULL, NULL, SW_SHOW);

		ShellExecute(AfxGetMainWnd()->GetSafeHwnd(),
			TEXT("open"),
			TEXT("explorer.exe"),
			CString(L"/select, \"") + sFile + L"\"",
			NULL,
			SW_NORMAL);

		return;
		break;

	case ID_DELETE:
	{
		SHFILEOPSTRUCT fileOp;
		ZeroMemory(&fileOp, sizeof(fileOp));
		fileOp.hwnd = theApp.GetMainFrame()->GetSafeHwnd();
		fileOp.wFunc = FO_DELETE;
		fileOp.fFlags |= FOF_ALLOWUNDO;

		CString files = sFile;

		files.Append(_T("??"));
		files.Replace(_T('?'), _T('\0'));

		fileOp.pFrom = files;

		theApp.GetMainFrame()->ExecuteFileOperation(fileOp);
		break;
	}

	case ID_PROPERTIES:

		{
			SHELLEXECUTEINFO sei;
			ZeroMemory((PVOID)&sei, sizeof(sei));

			sei.cbSize     = sizeof(sei);
			sei.fMask      = SEE_MASK_INVOKEIDLIST;
			sei.hwnd       = this->GetSafeHwnd();
			sei.nShow      = SW_SHOW;
			sei.lpVerb = _T("properties");
			sei.lpFile = sFile;
			ShellExecuteEx(&sei); 
		}

		return;
		break;

	case ID_SELECT:

		{

		}
	}
}

void CFileList::AddFiles(const std::vector<FSFile*>& _files)
{
	CWaitCursor wait;

	std::vector<FSFile*>& files = const_cast<std::vector<FSFile*>&>(_files);

	std::sort( files.begin( ), files.end(), 
		[]( FSFile* elem1,  FSFile* elem2)
	{
		return elem1->GetHash(false) > elem2->GetHash(false);
	} );

	this->LockWindowUpdate();

	for(size_t i = 0; i < files.size(); i++)
	{
		this->AddFile(files[i]);
	}

	this->UnlockWindowUpdate();
}

BOOL CFileList::DeleteAllItems()
{
	this->m_itemColors.clear();
	this->twins.clear();

	return CTreeListCtrl::DeleteAllItems();
}

void CFileList::OnHeaderColumnClick(int item)
{
	struct sort
	{
		static int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
		{
			CFileList* me = (CFileList*) lParamSort;

			FSFile *a = (FSFile *)(me->m_sort_asc?lParam1:lParam2);
			FSFile *b = (FSFile *)(me->m_sort_asc?lParam2:lParam1);

			switch(me->m_sort_col)
			{
				//case 0:
				//if(a->GetName() > b->GetName()) return -1;
				//if(a->GetName() < b->GetName()) return 1;
				//else return 0;
			case 0:
				if(a->GetPath() < b->GetPath()) return 1;
				if(a->GetPath() > b->GetPath()) return -1;
				else return 0;
			case 1:
			case 2:
				if(a->GetSize() < b->GetSize()) return 1;
				if(a->GetSize() > b->GetSize()) return -1;
				else return 0;
			case 3:
				return CompareFileTime(&a->GetModified(), &b->GetModified());
			case 4:
				if(a->GetHash(false) < b->GetHash(false)) return 1;
				if(a->GetHash(false) > b->GetHash(false)) return -1;
				else return 0;
			}
			return 0;
		}
	};

	this->Sort(item, TVI_ROOT, sort::CompareFunc);//this->GetRootItem());
}

void CFileList::FillMenu(CMenu &popup)
{		
	popup.AppendMenu(MF_BYCOMMAND|MF_STRING, ID_OPEN, _T("Open...\tReturn")); 
	popup.AppendMenu(MF_BYCOMMAND|MF_STRING, ID_OPENFOLDER, _T("Open Folder...")); 
	popup.AppendMenu(MF_SEPARATOR);

	popup.AppendMenu(MF_BYCOMMAND|MF_STRING, ID_DOHASHES, _T("Compute MD5-Hash"));
	popup.AppendMenu(MF_SEPARATOR);
	popup.AppendMenu(MF_BYCOMMAND|MF_STRING, ID_DELETE, _T("Delete (to Recycle Bin)...\tDel")); 
	popup.AppendMenu(MF_SEPARATOR);
	popup.AppendMenu(MF_BYCOMMAND|MF_STRING, ID_PROPERTIES, _T("Properties...")); 

	popup.SetDefaultItem(ID_OPEN);
}

void CFileList::OnZoomButton()
{
	theApp.GetMainFrame()->SendMessage(WM_COMMAND, ID_VIEW_FILELIST);
}

const CString& CFileList::GetItemText(HTREEITEM hItem, int col)
{
	std::vector<CString>& texts = m_texts[hItem];
	if(texts.size() == 0)
	{
		FSFile* f = (FSFile*) this->GetItemData(hItem);
		ASSERT(f);

		texts.resize(this->GetColumnCount());

		texts[0] = theApp.m_settings.GetViewPath() ? f->GetPath() : f->GetName();
		texts[FLITEM_SIZE] = f->GetSizeString();
		texts[FLITEM_MODI] = f->GetModifiedDateString();
		texts[FLITEM_HASH] = CString( f->GetHash(false).c_str() );

		this->SetToolTipText(f->item, theApp.m_settings.GetViewPath() ? f->GetName() : f->GetPath());
	}

	static CString s_empty;
	return col < (int)texts.size() ? texts[col] : s_empty;
}

float CFileList::GetPercentValue(HTREEITEM hItem)
{
	FSFile* f = (FSFile*) this->GetItemData(hItem);
	ASSERT(f);

	return f->GetPercent();
}