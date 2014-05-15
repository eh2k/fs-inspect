/*
* Copyright (c) 2005-2010 Eduard Heidt http://fs-inspect.sourceforge.net.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#pragma once

#include "TreeListCtrl.h"

class FSbase;
class FSFolder;

class EHDirSize : public CTreeListCtrl
{
public:

	FSFolder *m_root;

	EHDirSize();
	virtual ~EHDirSize();

	int Create(CWnd *parent, CImageList *imagelist);

	void RefreshData();

	void InsertFolder(FSbase *tree);

	void OnItemOpen();

	void ScanFolder(CString path, HTREEITEM node = TVI_ROOT);

	virtual void OnZoomButton();
	virtual void OnHeaderColumnClick(int item);
	virtual void OnItemExpanding(LPNMTREEVIEW pNMTreeView);
	virtual void OnItemOperation(UINT cmd);
	virtual void FillMenu(CMenu &popup);

	virtual const CString& GetItemText(HTREEITEM hItem, int col);
	virtual float GetPercentValue(HTREEITEM hItem);

protected:
	DECLARE_MESSAGE_MAP()
	LRESULT OnInsertFolder(WPARAM wp, LPARAM lp);
	LRESULT OnReScan(WPARAM wp, LPARAM lp);
};
