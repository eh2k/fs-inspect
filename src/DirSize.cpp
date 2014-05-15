/*
* Copyright (c) 2005-2010 Eduard Heidt http://fs-inspect.sourceforge.net.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#include "stdafx.h"

#include "fs.h"
#include "MainFrm.h"
#include "dirsize.h"
#include "ScanDlg.h"
#include "AltStreams.h"

BEGIN_MESSAGE_MAP(EHDirSize, CTreeListCtrl)
	ON_MESSAGE(ID_ADD_FILE, OnInsertFolder)
	ON_MESSAGE(ID_RESCAN, OnReScan)
END_MESSAGE_MAP()

LRESULT EHDirSize::OnReScan(WPARAM wp, LPARAM lp)
{
	auto item = (HTREEITEM)wp;

	if(auto folder = dynamic_cast<FSFolder*>((FSbase*)this->GetItemData(item)))
	{
		if(folder->item && !PathFileExists(folder->GetPath()))
		{
			ASSERT(folder->item == item);
			this->DeleteItem(folder->item);
			folder->item = NULL;
			//Todo ((FSFolder*)folder->GetParent())->RemoveVirtual(folder);
			return 1;
		}
		else
			this->UpdateItem(item);
	}

	return 0;
}

LRESULT EHDirSize::OnInsertFolder(WPARAM wp, LPARAM lp)
{
	FSFolder* f = (FSFolder*)wp;
	this->InsertFolder(f);
	return 0;
}

static void FeedBack(const CString& sDir, __int64 size)
{
	static CString files; 
	files.Format(_T("%d Files"), theApp.GetMainFrame()->m_count);

	CScanDlg::GetInstance().SetTexts(files, sDir);

	static int pos = 0;
	int npos = size / 1024 / 1024;
	if( npos != pos)
	{
		unsigned __int64 total, free, free2;
		GetDiskFreeSpaceEx(theApp.m_scanpath.GetBuffer(3), (PULARGE_INTEGER)&free, (PULARGE_INTEGER)&total, (PULARGE_INTEGER)&free2);

		CScanDlg::GetInstance().SetProgress(npos, (total - free2) / 1024 / 1024);
		pos = npos;
	}
}

static int ScanADS(CString path, FSFolder *parent, bool file)
{
	std::vector<CString> stream_names;
	std::vector<ULONGLONG> stream_sizes;

	try
	{
		if(theApp.m_settings.GetModeADS())
		{
			ListStreams(path, &stream_names, &stream_sizes);

			UINT j = (file?1:0);
			if(stream_names.size()>j)
				for(UINT i = j; i< stream_names.size(); i++)
				{
					CString pre = file?_T(""):_T("..\\");
					FSFile *ads = new FSFile(pre+stream_names[i], stream_sizes[i]);
					parent->AddFile(ads);

					theApp.GetMainFrame()->m_typestat.ScanFile(ads);
				}
		}
	}
	catch (DWORD e)
	{
		e = e;
		theApp.m_exceptions++;
	}

	return stream_names.size();
}

static unsigned __int64 Dir(CString sDir, FSFolder *parent) 
{
	if(CScanDlg::GetInstance().IsActive() == false)
		return 0;

	if(sDir.Find('\\')!=0)
		sDir.Replace(_T("\\\\"),_T("\\"));	

	if(parent == theApp.GetMainFrame()->m_dirsize.m_root)
		theApp.GetMainFrame()->m_dirsize.InsertFolder(parent);

	WIN32_FIND_DATA findData;
	HANDLE hFind=FindFirstFile((sDir+_T("\\*.*")), &findData);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		// access denied ...
	}
	else 
	{
		do 
		{
			CString sFileName(findData.cFileName);

			if ((findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) &&
				IsReparseTagNameSurrogate(findData.dwReserved0))
				continue;

			if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
			{
				if (sFileName != _T(".") && sFileName != _T("..")) 
				{
					Dir(sDir+_T("\\")+sFileName, parent->AddChildFolder(sFileName));
				}

				FeedBack(sDir, theApp.GetMainFrame()->m_size);
			}
			else 
			{	

				FSFile *file = parent->AddFile(findData);

				if(parent->GetFiles().size() % 100 == 0)
				{
					FeedBack(sDir, theApp.GetMainFrame()->m_size);
				}

				theApp.GetMainFrame()->m_typestat.ScanFile(file);

				ScanADS(file->GetPath(), parent, true);
			}
		} 
		while (FindNextFile(hFind, &findData));
	}

	FindClose(hFind); 

	if(parent->GetParent() == theApp.GetMainFrame()->m_dirsize.m_root)
		theApp.GetMainFrame()->m_dirsize.InsertFolder(parent);

	return parent->GetSize();

}

EHDirSize::EHDirSize()
{
	this->m_root = NULL;
	this->m_sort_asc = true;
	this->m_sort_col = 2;
}
EHDirSize::~EHDirSize()
{
	if(this->m_root)
		delete this->m_root;
}

int EHDirSize::Create(CWnd *parent, CImageList *imagelist)
{
	CTreeListCtrl::Create(parent);

	this->SetImageList(imagelist, TVSIL_NORMAL);

	//---------------------------------------------------------------
#define DSITEM_NAME 0
#define DSITEM_PERC 2
#define DSITEM_SIZE 1
#define DSITEM_COUNT1 3
#define DSITEM_COUNT2 4

	InsertColumn(DSITEM_NAME, _T("Name"),LVCFMT_LEFT, 235 , TA_LEFT);
	InsertColumn(DSITEM_SIZE, _T("Size"),LVCFMT_LEFT, 70 , TA_RIGHT);
	this->m_procentcol = InsertColumn(DSITEM_PERC, _T("%"),LVCFMT_CENTER, 80 , TA_RIGHT);
	InsertColumn(DSITEM_COUNT1, _T("Dirs"),LVCFMT_LEFT, 40, TA_RIGHT);
	InsertColumn(DSITEM_COUNT2, _T("Files"),LVCFMT_LEFT, 40, TA_RIGHT);

	return 1;

}

//-------------------------------------- Sort ------------------------------------------------------
static int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	EHDirSize* me = (EHDirSize*) lParamSort;

	FSFolder *val1 = (FSFolder*) lParam1; //(me->m_sort_asc?lParam1:lParam2);
	FSFolder *val2 = (FSFolder*) lParam2; //(me->m_sort_asc?lParam2:lParam1);

	if(val1 == NULL)
		return 1;
	if(val2 == NULL)
		return -1;

	switch(me->m_sort_col)
	{
	case 0:
		if(val1->GetName() <  val2->GetName()) return -1;
		if(val1->GetName() > val2->GetName()) return 1;
		else return 0;
	case 1:
	case 2:
		if(val1->GetSize() < val2->GetSize()) return 1;
		if(val1->GetSize() == val2->GetSize()) return 0;
		else return -1;
	case 3:
		if(val1->GetSubFolders().size() < val2->GetSubFolders().size()) return 1;
		if(val1->GetSubFolders().size() == val2->GetSubFolders().size()) return 0;
		else return -1;
	case 4:
		if(val1->GetFiles().size() < val2->GetFiles().size()) return 1;
		if(val1->GetFiles().size() == val2->GetFiles().size()) return 0;
		else return -1;
	}

	return 0;
}

void EHDirSize::ScanFolder(CString path, HTREEITEM node)
{
	theApp.StartTimer();

	DeleteAllItems(); 
	theApp.GetMainFrame()->m_typestat.DeleteAllItems();

	if(this->m_root)
		delete (this->m_root);

	this->m_root = new FSFolder(path);

	Dir(path, this->m_root);

	if(this->m_root->GetFiles().size() > 0)
		this->InsertFolder(&this->m_root->GetFileSet());

	this->RefreshData();
	theApp.GetMainFrame()->m_typestat.RefreshData();

	Sort(this->m_sort_col, this->GetRootItem(), CompareFunc);

	theApp.StopTimer();
}

void EHDirSize::OnItemOperation(UINT cmd)
{
	HTREEITEM selected = this->GetSelectedItem();

	if(!selected) 
		return;

	switch(cmd)
	{
	case ID_OPEN:
		this->OnItemOpen();
		break;
	case ID_EXPLORE:

		if(auto *folder = dynamic_cast<FSFolder*>((FSbase*) this->GetItemData(selected)))
			ShellExecute(AfxGetMainWnd()->GetSafeHwnd(), TEXT("open"), folder->GetPath(), NULL, NULL, SW_SHOW);

		break;
	case ID_DELETE:
		{
			SHFILEOPSTRUCT fileOp;
			ZeroMemory(&fileOp, sizeof(fileOp));
			fileOp.hwnd = theApp.GetMainFrame()->GetSafeHwnd();
			fileOp.wFunc = FO_DELETE;
			fileOp.fFlags |= FOF_ALLOWUNDO;

			CString files;
			if(auto *folder = dynamic_cast<FSFolder*>((FSbase*) this->GetItemData(selected)))
			{
				files = folder->GetPath() + _T("??");
			}
			else if(auto *fileSet = dynamic_cast<FSFileSet*>((FSbase*) this->GetItemData(selected)))
			{
				for(auto it = fileSet->GetFiles().begin(); it != fileSet->GetFiles().end(); ++it)
				{
					FSFile* file = *it;
					files.Append(file->GetPath() + _T("?"));
				}

				files.Append(_T("??"));
			}
			files.Replace(_T('?'), _T('\0'));
			fileOp.pFrom = files;
			theApp.GetMainFrame()->ExecuteFileOperation(fileOp);
			break;
		}
	case ID_PROPERTIES:

		if(auto *folder = dynamic_cast<FSFolder*>((FSbase*) this->GetItemData(selected)))
		{
			SHELLEXECUTEINFO sei;
			ZeroMemory((PVOID)&sei, sizeof(sei));

			CString path = folder->GetPath();

			sei.cbSize     = sizeof(sei);
			sei.fMask      = SEE_MASK_INVOKEIDLIST;
			sei.hwnd       = theApp.GetMainFrame()->GetSafeHwnd();
			sei.nShow      = SW_SHOW;
			sei.lpVerb = _T("properties");
			sei.lpFile = path;
			ShellExecuteEx(&sei); 
		}
		break;
	}
}

void EHDirSize::RefreshData()
{
	HTREEITEM p = this->GetRootItem();
	while(p != NULL)
	{
		this->UpdateItem(p);
		p = this->GetNextItem(p, TVGN_NEXTVISIBLE);
	}
}

void EHDirSize::InsertFolder(FSbase *tree)
{
	if(IsOwnerThread() == false)
	{
		this->SendMessage(ID_ADD_FILE, (WPARAM)tree);
		return;
	}

	if(tree->item == NULL)
	{
		TV_INSERTSTRUCT tvis;
		ZeroMemory(&tvis, sizeof(TV_INSERTSTRUCT));
		tvis.hInsertAfter = TVI_LAST;
		tvis.item.mask = TVIF_CHILDREN | TVIF_IMAGE | TVIF_SELECTEDIMAGE;

		if(FSFolder* folder = dynamic_cast<FSFolder*>(tree))
			tvis.item.cChildren = folder->GetSubFolders().size() > 0 || tree->GetParent() == NULL;

		tvis.item.iImage = tvis.item.iSelectedImage = tree->GetIcon();

		if(tree->GetParent() == NULL)
			tvis.hParent = TVI_ROOT;
		else if(tree->GetParent()->item != NULL)
			tvis.hParent = tree->GetParent()->item;

		tree->item = this->InsertItem(&tvis);
		this->SetItemData(tree->item, (DWORD_PTR)tree);

		if(!tree->item)
			ASSERT(FALSE);

		this->Expand(GetRootItem(), TVE_EXPAND);
	}
	else
	{
		TVITEM tvi;
		ZeroMemory(&tvi, sizeof(TVITEM));
		tvi.mask = TVIF_CHILDREN | TVIF_IMAGE | TVIF_SELECTEDIMAGE;

		if(FSFolder* folder = dynamic_cast<FSFolder*>(tree))
			tvi.cChildren = folder->GetSubFolders().size() > 0;// || tree->m_fds.size()>0; //HasGotSubEntries(sPath);

		this->GetItemImage(tree->item, tvi.iImage, tvi.iSelectedImage);
		tvi.hItem = tree->item;

		this->SetItem(&tvi);
	}

	this->UpdateItem(tree->item);
}

void EHDirSize::OnItemExpanding(LPNMTREEVIEW pNMTreeView)
{
	FSFolder *tmp = (FSFolder*) this->GetItemData(pNMTreeView->itemNew.hItem); 

	if(tmp && pNMTreeView->action == 2)
	{
		size_t c = tmp->GetSubFolders().size();

		this->LockWindowUpdate();

		CWaitCursor wait;

		for(UINT i = 0; i < c; i++)
		{
			this->InsertFolder(tmp->GetSubFolders().at(i));
		}

		if(tmp->GetFileSize() > 0)
			this->InsertFolder(&tmp->GetFileSet());

		Sort(this->m_sort_col, tmp->item, CompareFunc);

		this->UnlockWindowUpdate();
	}
}

void EHDirSize::OnHeaderColumnClick(int item)
{
	HTREEITEM p = this->GetFirstVisibleItem();
	while(p != NULL)
	{
		this->Sort(item, p, CompareFunc);
		p = this->GetNextItem(p,TVGN_NEXTVISIBLE);
	}
}

void EHDirSize::FillMenu(CMenu &popup)
{
	popup.AppendMenu(MF_BYCOMMAND|MF_STRING, ID_OPEN, _T("View Files\tReturn")); 
	popup.AppendMenu(MF_SEPARATOR);
	popup.AppendMenu(MF_BYCOMMAND|MF_STRING, ID_EXPLORE, _T("Explore...")); 
	popup.AppendMenu(MF_SEPARATOR);
	popup.AppendMenu(MF_BYCOMMAND|MF_STRING, ID_DELETE, _T("Delete (to Recycle Bin)...\tDel")); 
	popup.AppendMenu(MF_SEPARATOR);
	popup.AppendMenu(MF_BYCOMMAND|MF_STRING, ID_PROPERTIES, _T("Properties")); 

	popup.SetDefaultItem(ID_OPEN);

	CBitmap file; file.LoadBitmap(IDB_FILE);
	popup.SetMenuItemBitmaps(ID_OPEN, MF_BYCOMMAND, &file, &file);
}

void EHDirSize::OnItemOpen()
{
	HTREEITEM sel = GetSelectedItem();
	if(sel==NULL)
		return;

	FSbase *tmp = (FSbase*) this->GetItemData(sel); 

	std::vector<FSFile*> files;

	if( FSFolder* folder = dynamic_cast<FSFolder*>(tmp))
	{
		for(auto it = folder->GetFiles().begin(); it != folder->GetFiles().end(); ++it)
			files.push_back(*it);
	}
	else if(FSFileSet* fileset = dynamic_cast<FSFileSet*>(tmp))
	{
		for(auto it = fileset->GetFiles().begin(); it != fileset->GetFiles().end(); ++it)
			files.push_back(*it);
	}

	theApp.GetMainFrame()->m_filelist.DeleteAllItems();
	theApp.GetMainFrame()->m_filelist.AddFiles(files);
	theApp.GetMainFrame()->SendMessage(WM_COMMAND,MAINFRAME_DOFILELIST);
}

void EHDirSize::OnZoomButton()
{
	theApp.GetMainFrame()->SendMessage(WM_COMMAND, ID_VIEW_DIRSIZE);
}

const CString& EHDirSize::GetItemText(HTREEITEM hItem, int col)
{
	std::vector<CString>& texts = m_texts[hItem];
	if(texts.size() == 0)
	{
		if(FSFolder* tree = dynamic_cast<FSFolder*>((FSbase*)GetItemData(hItem)))
		{
			ASSERT(tree);

			texts.resize(this->GetColumnCount());
			texts[0] = tree->GetName();
			texts[DSITEM_SIZE] = tree->GetSizeString();
			texts[DSITEM_COUNT1] = L2A(tree->GetSubFolders().size());
			texts[DSITEM_COUNT2] = L2A(tree->GetFiles().size());
		}
		else if(FSFileSet* tree = dynamic_cast<FSFileSet*>((FSbase*)GetItemData(hItem)))
		{
			texts.resize(this->GetColumnCount());
			texts[0].Format(_T("[%d Files]"), tree->GetFiles().size());
			texts[DSITEM_SIZE] = tree->GetSizeString();
			texts[DSITEM_COUNT1] = _T("");
			texts[DSITEM_COUNT2] = L2A(tree->GetFiles().size());
		}
		else
			ASSERT(NULL);

	}

	return texts[col];
}

float EHDirSize::GetPercentValue(HTREEITEM hItem)
{
	FSFolder* tree = (FSFolder*)GetItemData(hItem);
	ASSERT(tree);
	return tree->GetPercent();
}