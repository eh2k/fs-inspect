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
#include <map>
#include <sstream>
#include <string>
#include "typestat.h"
#include "ScanDlg.h"
#include <ppl.h>

BEGIN_MESSAGE_MAP(EHTypeStat, CTreeListCtrl)
	ON_MESSAGE(ID_ADD_FILE, OnAddType)
	ON_MESSAGE(ID_RESCAN, OnReScan)
END_MESSAGE_MAP()

LRESULT EHTypeStat::OnReScan(WPARAM wp, LPARAM lp)
{
	auto item = (HTREEITEM)wp;

	exttype *e = (exttype*)this->GetItemData(item);
	if(e != NULL)
	{
		bool update = false;

		for(auto it = e->_files.begin(); it != e->_files.end();)
		{
			FSFile* file = *it;
			if(auto fileSet = dynamic_cast<FSFileSet*>(file->GetParent()))
			{
				if(!PathFileExists(file->GetPath()))
				{
						if(file->item != NULL)
							theApp.GetMainFrame()->m_filelist.DeleteItem(file->item);

						theApp.GetMainFrame()->m_count--;
						theApp.GetMainFrame()->m_size -= file->GetSize();

						fileSet->RemoveVirtual(file);

						if(fileSet->item && fileSet->GetFiles().size() == 0)
						{
							theApp.GetMainFrame()->m_dirsize.DeleteItem(fileSet->item);
						}

						it = e->_files.erase(it);
						update = true;
				}
				else 
					++it;
			}
		}

		if(update)
		{
			if(e->_files.size() == 0)
				this->DeleteItem(item);
			else
				this->UpdateItem(item);
		}
	}

	return 0;
}

LRESULT EHTypeStat::OnAddType(WPARAM wp, LPARAM lp)
{
	FSFile* file = (FSFile*)wp;
	this->ScanFile(file);
	return 0;
}

EHTypeStat::EHTypeStat()
{
	this->m_hScanThread = NULL;
	this->m_item_dohash = NULL;

	this->m_sort_asc = true;
	this->m_sort_col = 2;
}

EHTypeStat::~EHTypeStat(void)
{
	if(m_hScanThread)
	{
		HANDLE handle = m_hScanThread;
		m_hScanThread = NULL;
		WaitForSingleObject(handle, 1000);
		CloseHandle(handle);
	}
}

int EHTypeStat::Create(CWnd *parent, CImageList *imagelist)
{
	CTreeListCtrl::Create(parent);

#define TSITEM_TYPE 0
#define TSITEM_SIZE 1
#define TSITEM_PERC 2
#define TSITEM_COUNT 3
#define TSITEM_DOUBL 4
#define TSITEM_DOUBLSIZE 5

	InsertColumn(TSITEM_TYPE, _T("File Type"),LVCFMT_LEFT, 115, TA_LEFT);
	InsertColumn(TSITEM_SIZE, _T("Size"),LVCFMT_LEFT, 80, TA_RIGHT);
	InsertColumn(TSITEM_PERC, _T("%"),LVCFMT_CENTER, 80, TA_RIGHT);
	InsertColumn(TSITEM_COUNT, _T("Count"),LVCFMT_LEFT, 50, TA_RIGHT);

	InsertColumn(TSITEM_DOUBL, _T("Duplicates"),LVCFMT_LEFT, 70, TA_RIGHT);
	InsertColumn(TSITEM_DOUBLSIZE, _T("Dup-Size"),LVCFMT_LEFT, 70, TA_RIGHT);

	this->m_procentcol = TSITEM_PERC;

	ASSERT(imagelist);
	this->SetImageList(imagelist, TVSIL_NORMAL);

	return TRUE;
}

static bool is_remote_path(const CString& path)
{
	if(path.Find('\\')==0 || GetDriveType(path.Left(3)) == DRIVE_REMOTE)
		return true;
	return false;
}

static void dup_find(size_t& progressPos, size_t& progressMax, exttype *e)
{
	e->_lDupSize = 0;
	e->_nDupCount = 0;
	e->_lDupFiles = 0;
	e->_finished = false;

	if(e->_files.size() < 2 )
		return;

	const std::vector<FSFile*>& _files = e->_files;
	size_t& lDupSize = e->_lDupSize; 
	size_t& nDupCount = e->_nDupCount; 
	size_t& nDupFiles = e->_lDupFiles;

	DWORD mode = 0;

	if( is_remote_path( theApp.m_scanpath ) )
	{
		if(theApp.m_settings.GetCmpNameN()) mode |= 1;
		if(theApp.m_settings.GetCmpSizeN()) mode |= 2;
		if(theApp.m_settings.GetCmpDateN()) mode |= 4;
		if(theApp.m_settings.GetCmpHashN()) mode |= 8;
	}
	else
	{
		if(theApp.m_settings.GetCmpNameL()) mode |= 1;
		if(theApp.m_settings.GetCmpSizeL()) mode |= 2;
		if(theApp.m_settings.GetCmpDateL()) mode |= 4;
		if(theApp.m_settings.GetCmpHashL()) mode |= 8;
	}

	struct key
	{
		FSFile* _file;
		DWORD _mode;

		key(FSFile* file, DWORD mode):
			_file(file), _mode(mode)
		{}

		inline bool operator < (const key& other) const
		{
			if((_mode & 2) && _file->GetSize() < other._file->GetSize())
				return true;

			if((_mode & 4) && memcmp(&_file->GetModified(), &other._file->GetModified(), sizeof(FILETIME)) < 0) 
				return true;

			if((_mode & 1) && _file->GetName() < other._file->GetName())
				return true;

			return false;
		}
		operator size_t() const
		{
			size_t seed = 0;

			if(_mode & 1)
			{
				std::hash<const TCHAR*> strhash;
				seed = strhash(_file->GetName());
			}

			if(_mode & 2)
				seed ^= _file->GetSize();

			if((_mode & 4))
				seed ^= _file->GetModified().dwHighDateTime ^ _file->GetModified().dwLowDateTime;

			return seed;
		}
	};

	typedef stdext::hash_map< key, std::list<FSFile*> > DUPS;
	typedef stdext::hash_map< HASHkey, std::list<FSFile*> > TWINS;

	DUPS dups;

	for(size_t i = 0; i < _files.size(); i++)
	{
		if(!CScanDlg::GetInstance().IsActive())
			ExitThread(-1);

		FSFile *file = _files[i];
		dups[ key(file, mode) ].push_back(file);	
	}

	for(auto it = dups.begin(); it != dups.end(); ++it)
	{
		if(!CScanDlg::GetInstance().IsActive())
			ExitThread(-1);

		TWINS twins;

		if(it->second.size() > 1)
		{
			for(auto it2 = it->second.begin(); it2 != it->second.end(); ++it2)
			{
				if(!CScanDlg::GetInstance().IsActive())
					ExitThread(-1);

				FSFile *file = *it2;

				CString tmp;
				tmp.Format(_T("Searching for Duplicates..(*.%s)"), file->GetExt());
				CString tmp2;

				tmp2.Format(_T("Computing MD5: %s (%s)"), file->GetName(), file->GetSizeString());
				CScanDlg::GetInstance().SetTexts( tmp, tmp2);
				CScanDlg::GetInstance().SetProgress(++progressPos, progressMax);
				CScanDlg::GetInstance().SetProgress2(0, 0);

				theApp.OnIdle(0); //Aktualisiert die Statusleiste...

				std::list<FSFile*>* pTwins = NULL;

				if(((mode & 8) != 0))
				{
					if(file->GetSize()>1024*1024*10)
					{
						int pos = 0;
						int max = file->GetSize()/4096;

						pTwins = &twins[ file->GetHash(true, true, 
							[&pos, max, &tmp, &tmp2](void)
						{
							CScanDlg::GetInstance().SetTexts( tmp, tmp2);
							CScanDlg::GetInstance().SetProgress2(++pos, max);
						})];
					}
					else
					{
						pTwins = &twins[ file->GetHash() ];
					}	
				}
				else
				{
					pTwins = &twins[ HASHkey("") ];
				}

				pTwins->push_back(*it2);

				if(pTwins->size() > 1)
				{
					if(pTwins->size() == 2)
					{	
						nDupCount++;
						theApp.GetMainFrame()->m_filelist.AddFile(pTwins->front());
					}

					theApp.GetMainFrame()->m_filelist.AddFile(pTwins->back());

					nDupFiles ++;
					nDupCount ++;
					lDupSize += pTwins->back()->GetSize();

					//theApp.GetMainFrame()->m_typestat.UpdateItem(e->ti);
				}
			}
		}
		else
		{
			progressMax--;
		}
	}

	e->_finished = true;
}

void EHTypeStat::DoTwinCheck()
{
	CScanDlg::GetInstance().SetTexts( _T("Searching for Duplicates.."), _T(".."));

	theApp.GetMainFrame()->SendMessage(WM_COMMAND, MAINFRAME_DOFILELIST);
	theApp.GetMainFrame()->m_filelist.DeleteAllItems();

	if(m_item_dohash != NULL)
	{
		exttype *e = (exttype*)this->GetItemData(m_item_dohash);

		size_t i = 0; 
		size_t n = e->_files.size();

		dup_find(i, n, e);
		this->UpdateItem(m_item_dohash);

		m_item_dohash = NULL;
		CScanDlg::GetInstance().Stop();
		return;
	}

	theApp.GetMainFrame()->m_filelist.DeleteAllItems();

	std::vector<exttype*> todo;
	size_t i = 0;
	size_t n = 0;

	for(HTREEITEM hItem = GetRootItem(); hItem!=NULL; hItem = GetNextItem( hItem, TVGN_NEXT ) )
	{
		exttype *e = (exttype*)this->GetItemData(hItem);

		bool ok = false;
		int curPos = 0;
		CString filter = theApp.m_settings.GetTwinTypes();
		CString resToken = filter.Tokenize(_T("|"), curPos);
		while(!resToken.IsEmpty() && ok == false)
		{
			if(wildcmp(resToken, e->get_type()))
				ok = true;

			resToken = filter.Tokenize(_T("|"), curPos);
		};

		if(ok && e && e->ti)
		{
			todo.push_back(e);
			n += e->_files.size();
		}

		if(!CScanDlg::GetInstance().IsActive())
			break;
	}

	for(size_t j = 0; j < todo.size(); j++)
	{
		exttype *e = todo[j];

		dup_find(i, n, e);
		this->UpdateItem(e->ti);

		if(!CScanDlg::GetInstance().IsActive())
			break;
	}

	//CString tmp; tmp.Format(L"Listing %d files..", dups.size());
	//CScanDlg::GetInstance().SetTexts( tmp, _T(""));
	CScanDlg::GetInstance().Stop();
}

static int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	EHTypeStat* me = (EHTypeStat*) lParamSort;

	exttype *val1 = (exttype*) (me->m_sort_asc?lParam1:lParam2);
	exttype *val2 = (exttype*) (me->m_sort_asc?lParam2:lParam1);

	switch(me->m_sort_col)
	{
	case 0:
		if(val1->get_type() > val2->get_type()) return -1;
		if(val1->get_type() < val2->get_type()) return 1;
		else return 0;
	case 1:
	case 2:
		if(val1->GetSize() < val2->GetSize()) return 1;
		if(val1->GetSize() > val2->GetSize()) return -1;
		else return 0;
	case 3:
		if(val1->GetCount() < val2->GetCount()) return 1;
		if(val1->GetCount() > val2->GetCount()) return -1;
		else return 0;
	case 4:
		if(val1->_nDupCount< val2->_nDupCount) return 1;
		if(val1->_nDupCount > val2->_nDupCount) return -1;
	case 5:
		if(val1->_lDupSize < val2->_lDupSize) return 1;
		if(val1->_lDupSize > val2->_lDupSize) return -1;
		else return 0;
	}

	return 0;

}

void EHTypeStat::RefreshData()
{
	this->Sort(1, TVI_ROOT, CompareFunc);

	HTREEITEM p = this->GetRootItem();
	while(p != NULL)
	{
		this->UpdateItem(p);
		p = this->GetNextItem(p,TVGN_NEXT);
	}
}

static std::vector<CString> s_typeNames;

void EHTypeStat::InsertType(exttype *e, bool with_info)
{
	if(!e->ti) //item einfügen
	{
		int icon = e->_files[0]->GetIcon();

		CString name = e->get_type();
		e->ti = this->InsertItem(name, icon, icon, TVI_ROOT);

		CString path = e->_files[0]->GetPath();

		if(path.Find(_T(":$"))!= -1)
			path = path.Left(path.Find(':'));

		SHFILEINFO sfi;
		ZeroMemory(&sfi, sizeof(SHFILEINFO));
		SHGetFileInfo(path, 0, &sfi, sizeof(SHFILEINFO), SHGFI_TYPENAME);

		s_typeNames.push_back(sfi.szTypeName);

		this->SetToolTipText(e->ti, s_typeNames.back());
		this->SetItemData(e->ti, (DWORD_PTR) e);
	}

	if(!with_info)
	{
		this->UpdateItem(e->ti);
		return;
	}

	if(e->GetCount() > 0 )
		this->UpdateItem(e->ti);
	else
		this->DeleteItem(e->ti);
}

void EHTypeStat::OnItemOperation(UINT cmd)
{

	HTREEITEM selected = this->GetSelectedItem();

	if(!selected) return;

	exttype *et = (exttype*) this->GetItemData(selected); 
	TRACE(_T("%s\n"), et->get_type());
	CString target;

	switch(cmd)
	{
	case ID_DOHASHES:

		m_item_dohash = selected;

		this->DoTwinCheckAsThread();

		break;


	case ID_ALL:
	case ID_OPEN:

		this->OnItemOpen();
		break;

	case ID_DELETE:
		{
			SHFILEOPSTRUCT fileOp;
			ZeroMemory(&fileOp, sizeof(fileOp));
			fileOp.hwnd = theApp.GetMainFrame()->GetSafeHwnd();
			fileOp.wFunc = FO_DELETE;
			fileOp.fFlags |= FOF_ALLOWUNDO;

			CString files;

			for(auto it = et->_files.begin(); it != et->_files.end(); ++it)
			{
				FSFile* file = *it;
				files.Append(file->GetPath() + _T("?"));
			}

			files.Append(_T("??"));
			files.Replace(_T('?'), _T('\0'));

			fileOp.pFrom = files;

			theApp.GetMainFrame()->ExecuteFileOperation(fileOp);
			break;
		}

	default: 
		break;
	}
}

void EHTypeStat::OnHeaderColumnClick(int item)
{
	this->Sort(item, TVI_ROOT, CompareFunc);//this->GetRootItem());
}

BOOL EHTypeStat::DeleteAllItems()
{
	this->m_types.clear(); 
	theApp.GetMainFrame()->m_size = 0; 
	theApp.GetMainFrame()->m_count = 0; 
	theApp.m_exceptions = 0;

	return CTreeListCtrl::DeleteAllItems(); 
}

void EHTypeStat::ScanFile(FSFile *file)
{
	if(IsOwnerThread() == false)
	{
		this->SendMessage(ID_ADD_FILE, (WPARAM)file);
		return;
	}

	theApp.GetMainFrame()->m_count++;
	theApp.GetMainFrame()->m_size += file->GetSize();

	exttype e(file);

	std::wstring h = e.get_type();

	exttype_map::iterator it = this->m_types.find(h);
	exttype *r = NULL;
	if(it == this->m_types.end())
	{
		this->m_types.insert(std::make_pair(h,e));
		r = &this->m_types.find(h)->second;
	}
	else
	{	
		r = &it->second;
		r->_files.push_back(file);
	}

	if(!r->ti)
		this->InsertType(r, false);
}

CString EHTypeStat::GetTwinInformation(ULONGLONG &lSumSize, int &count)
{
	for(exttype_map::iterator i = this->m_types.begin(); i != this->m_types.end(); ++i)
	{
		lSumSize += i->second._lDupSize;	
		count += i->second._nDupCount;
	}

	return FSbase::GetSizeString(lSumSize);
}

void EHTypeStat::OnItemOpen()
{
	HTREEITEM sel = GetSelectedItem();
	if(sel==NULL)
		return;

	exttype *et = (exttype*) this->GetItemData(sel); 

	theApp.GetMainFrame()->m_filelist.DeleteAllItems();
	theApp.GetMainFrame()->m_filelist.AddFiles(et->_files);

	theApp.GetMainFrame()->SendMessage(WM_COMMAND,MAINFRAME_DOFILELIST);
}

void EHTypeStat::FillMenu(CMenu &popup)
{
	popup.AppendMenu(MF_BYCOMMAND|MF_STRING, ID_ALL, _T("View Files\tReturn")); 
	popup.AppendMenu(MF_BYCOMMAND|MF_STRING, ID_DOHASHES, _T("Search for Duplicates")); 

	popup.AppendMenu(MF_SEPARATOR);
	popup.AppendMenu(MF_BYCOMMAND|MF_STRING, ID_DELETE, _T("Delete Files (to Recycle Bin)...\tDel")); 

	popup.SetDefaultItem(ID_ALL);

	CBitmap file; 
	file.LoadBitmap(IDB_FILE);
	popup.SetMenuItemBitmaps(ID_ALL, MF_BYCOMMAND, &file, &file);
}

void EHTypeStat::DoTwinCheckAsThread()
{
	struct thread
	{
		static DWORD WINAPI proc(LPVOID pThis)
		{
			WaitForSingleObject( CScanDlg::GetInstance().m_isModal, INFINITE);

			EHTypeStat* me = (EHTypeStat*)pThis;
			me->DoTwinCheck();

			return 0;
		}
	};


	DWORD dwThreadID = 0;
	m_hScanThread = ::CreateThread(NULL, 0, thread::proc, this, THREAD_PRIORITY_NORMAL, &dwThreadID);

	CScanDlg::GetInstance().SetTexts( _T("Searching for Duplicates.."), _T("."));
	if(::IsWindow(CScanDlg::GetInstance().GetSafeHwnd()) == FALSE)
	{
		CScanDlg::GetInstance().DoModal();
	}
}

void EHTypeStat::OnZoomButton()
{
	theApp.GetMainFrame()->SendMessage(WM_COMMAND, ID_VIEW_TYPESTAT);
}

const CString& EHTypeStat::GetItemText(HTREEITEM hItem, int col)
{
	std::vector<CString>& texts = m_texts[hItem];
	if(texts.size() == 0)
	{
		exttype* e = (exttype*)GetItemData(hItem);
		ASSERT(e);

		texts.resize(this->GetColumnCount());
		texts[0] = e->get_type();
		texts[TSITEM_COUNT] = L2A(e->GetCount());
		texts[TSITEM_SIZE] = FSbase::GetSizeString(e->GetSize());

		if(e->_finished)
		{
			if(e->_lDupFiles > 0)
				texts[TSITEM_DOUBL].Format(L"%d/%d", e->_lDupFiles, e->_nDupCount);
			else
				texts[TSITEM_DOUBL] = L"-";

			texts[TSITEM_DOUBLSIZE] = FSbase::GetSizeString(e->_lDupSize);
		}
		else
		{
			if(e->_lDupFiles > 0)
			{
				texts[TSITEM_DOUBL].Format(L"# %d/%d", e->_lDupFiles, e->_nDupCount);
				texts[TSITEM_DOUBLSIZE] = FSbase::GetSizeString(e->_lDupSize);
			}
		}
	}

	return texts[col];
}

float EHTypeStat::GetPercentValue(HTREEITEM hItem)
{
	exttype* e = (exttype*)GetItemData(hItem);
	ASSERT(e);
	return e->GetPercent();
}