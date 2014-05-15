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

class FSFile;
struct HASHkey;

class CFileList: public CTreeListCtrl
{
	typedef stdext::hash_map<HASHkey, HTREEITEM> HASH_TREE;
	HASH_TREE twins;

	stdext::hash_map<HTREEITEM, COLORREF> m_itemColors;
public:

	CFileList();
	virtual ~CFileList();

	virtual void OnHeaderColumnClick(int item);
	virtual void OnZoomButton();
	virtual void OnItemOperation(UINT cmd);
	virtual void FillMenu(CMenu &popup);

	BOOL Create(CWnd *parent, CImageList *imagelist);
	
	virtual BOOL DeleteAllItems();

	void AddFiles(const std::vector<FSFile*>& files);
	void AddFile(FSFile* f);
	void RefreshData();

	virtual const CString& GetItemText(HTREEITEM hItem, int col);
	virtual float GetPercentValue(HTREEITEM hItem);

	virtual COLORREF GetItemColor(HTREEITEM hItem)
	{
		VerifyOwnerThread();

		COLORREF& color = m_itemColors[hItem];
		if(color == 0)
			color = RGB(0xff,0xff,0xff);

		return color;
	}
	void SetItemColor(HTREEITEM hItem, COLORREF color)
	{
		VerifyOwnerThread();

		m_itemColors[hItem] = color;
	}

protected:
	DECLARE_MESSAGE_MAP()
	LRESULT OnAddFile(WPARAM wp, LPARAM lp);
};
