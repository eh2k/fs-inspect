/*
* Copyright (c) 2005-2010 Eduard Heidt http://fs-inspect.sourceforge.net.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#pragma once

#include <hash_map>
#include "TreeListCtrl.h"
#include "fs.h"

struct exttype
{
	HTREEITEM ti;
	size_t _nDupCount;
	size_t _lDupSize;
	size_t _lDupFiles;
	bool _finished;
	std::vector<FSFile*> _files;

	exttype(FSFile *file) // Konsturktor
	{
		this->_files.push_back(file);

		this->_finished = false;
		this->_lDupFiles = 0;
		this->_lDupSize = 0;
		this->_nDupCount = 0;
		this->ti = NULL;
	}

	unsigned __int64 GetSize()
	{
		unsigned __int64 _lSumSize = 0;

		for(size_t i = 0; i < _files.size(); i++)
			_lSumSize += _files[i]->GetSize();

		return _lSumSize;
	}

	size_t GetCount()
	{
		return  _files.size();
	}

	float GetPercent()
	{
		if(_files.size() > 0)
			return _files[0]->GetPercent(GetSize());
		else
			return 0;
	}

	const CString& get_type()
	{
		if(_files.size() > 0)
			return _files[0]->GetExt();
		else
		{
			static CString empty;
			return empty;
		}
	}
};

class EHTypeStat: public CTreeListCtrl
{
	void InsertType(exttype *e, bool with_info = true);
public:
	HTREEITEM   m_item_dohash;

	HANDLE m_hScanThread;

	typedef stdext::hash_map<std::wstring, exttype> exttype_map;
	exttype_map  m_types;

	EHTypeStat(void);
	virtual ~EHTypeStat(void);

	int Create(CWnd *parent, CImageList *imagelist);

	void RefreshData();
	void DoTwinCheck();

	void  DoTwinCheckAsThread();

	virtual BOOL DeleteAllItems();
	void ScanFile(FSFile *file);

	CString GetTwinInformation(ULONGLONG &lSumSize, int &count);

	virtual void OnItemOperation(UINT cmd);
	virtual void OnHeaderColumnClick(int item);

	void OnItemOpen();

	virtual void OnZoomButton();

	void FillMenu(CMenu &popup);

	virtual const CString& GetItemText(HTREEITEM hItem, int col);
	virtual float GetPercentValue(HTREEITEM hItem);

protected:
	DECLARE_MESSAGE_MAP()
	LRESULT OnAddType(WPARAM wp, LPARAM lp);
	LRESULT OnReScan(WPARAM wp, LPARAM lp);
};
