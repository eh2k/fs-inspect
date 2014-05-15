/* TreeListCtrl.h	

Copyright (C) 2005 Eduard Heidt

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

*/

#pragma once
#include <hash_map>
#include <typeinfo.h>

#define VerifyOwnerThread()  if(m_ownerThread != GetCurrentThreadId()) \
	TRACE("VERIFY_OWNER_THREAD %s :: %s\n", typeid(*this).name(), __FUNCTION__);

class CTreeListCtrl : public CStatic
{
	friend class TreeControl;
	friend class TreeHeader;

	CTreeCtrl* m_pTree;
	CHeaderCtrl* m_pHeader;
	CButton* m_pButton;

	CFont m_font;

protected:

	BOOL maximized;
	
	int m_procentcol;
	UINT align[10];
	int colwidth[10];

	int m_ownerThread;

	bool IsOwnerThread()
	{
		return m_ownerThread == GetCurrentThreadId();
	}

	stdext::hash_map<HTREEITEM, std::vector<CString>> m_texts;

public:

	int m_sort_col;
	bool m_sort_asc;

public:

	CTreeListCtrl();
	virtual ~CTreeListCtrl();

	BOOL Create(CWnd *parent)
	{
		m_ownerThread = GetCurrentThreadId();
		return __super::Create(NULL, WS_CHILD | /*WS_BORDER|*/ WS_VISIBLE, CRect(0,0,0,0), parent);
	}

	virtual void OnItemExpanding(LPNMTREEVIEW pNMHDR) { };
	virtual void OnHeaderColumnClick(int item) { ASSERT(0); }
	virtual void OnZoomButton() = 0;

	virtual void FillMenu(CMenu &popup) = 0;
	virtual void OnItemOperation(UINT cmd) = 0;

	virtual const CString& GetItemText(HTREEITEM hItem, int col) = 0;
	virtual float GetPercentValue(HTREEITEM hItem) = 0;

	void UpdateItem(HTREEITEM hItem)
	{ 
		VerifyOwnerThread();

		std::vector<CString>& texts = m_texts[hItem];
		texts.clear();

		m_pTree->SetItemText(hItem, NULL);
	}

	virtual COLORREF GetItemColor(HTREEITEM hItem)
	{
		return RGB(0xff,0xff,0xff);
	}

	BOOL SetItemData(HTREEITEM hItem, DWORD_PTR dwData)
	{
		BOOL ret = m_pTree->SetItemData(hItem, dwData);
		ASSERT(ret);
		return ret;
	}

	DWORD_PTR GetItemData(HTREEITEM hItem)
	{
		return m_pTree->GetItemData(hItem);
	}

	void Maximize(BOOL max = TRUE)
	{
		if(maximized != max)
		{
			maximized = max;
			if(maximized == TRUE)
				this->m_pButton->SetWindowText(L"2");
			else
				this->m_pButton->SetWindowText(L"1");
		}
	}

	BOOL DeleteItem(HTREEITEM hItem)
	{
		VerifyOwnerThread();

		m_texts.erase(hItem);
		return m_pTree->DeleteItem(hItem);
	}

	virtual BOOL DeleteAllItems()
	{
		VerifyOwnerThread();

		m_texts.clear();
		m_pTree->LockWindowUpdate();
		BOOL ret = m_pTree->DeleteAllItems();
		m_pTree->UnlockWindowUpdate();
		return ret;
	}

	int InsertColumn(int nCol, LPTSTR lpszColumnHeading, DWORD nFormat, int nWidth, DWORD nAlign = DT_LEFT)
	{
		VerifyOwnerThread();

		HD_ITEM hdi;
		hdi.mask = HDI_TEXT | HDI_FORMAT;
		if(nWidth!=-1)
		{
			hdi.mask |= HDI_WIDTH;
			hdi.cxy = nWidth;
		}

		hdi.pszText = (LPTSTR)lpszColumnHeading;
		hdi.fmt = HDF_STRING; // | HDF_OWNERDRAW;

		if(nFormat == LVCFMT_RIGHT)		hdi.fmt |= HDF_RIGHT;
		else if(nFormat == LVCFMT_CENTER)	hdi.fmt |= HDF_CENTER;
		else					hdi.fmt |= HDF_LEFT;

		this->colwidth[nCol] = nWidth;
		this->align[nCol] = nAlign;

		return this->m_pHeader->InsertItem(nCol, &hdi);
	}

	int GetColumnCount()
	{ 
		VerifyOwnerThread();

		return this->m_pHeader->GetItemCount();
	}

	int GetColumnWidth(int i)
	{ 
		VerifyOwnerThread();

		return colwidth[i];
	}

	void SetColumnWidth(int i, int width)
	{ 
		VerifyOwnerThread();

		if(::IsWindow(this->m_pHeader->GetSafeHwnd()) && this->m_pHeader->GetItemCount() > i)
		{
			HDITEM col;
			ZeroMemory(&col, sizeof(HDITEM));
			col.mask = HDI_WIDTH;
			col.cxy = width;
			this->colwidth[i] = width;
			this->m_pHeader->SetItem(i, &col);
		}
	}

	COLORREF GetTextBkColor(HTREEITEM hItem)
	{
		VerifyOwnerThread();

		if(TVIS_SELECTED & m_pTree->GetItemState(hItem,TVIS_SELECTED))
			return (this->m_pTree == this->GetFocus())?GetSysColor(COLOR_HIGHLIGHT):GetSysColor(COLOR_INACTIVEBORDER);
		else
			return this->GetItemColor(hItem);
	}

	COLORREF GetTextColor(HTREEITEM hItem)
	{
		VerifyOwnerThread();

		if(TVIS_SELECTED == m_pTree->GetItemState(hItem,TVIS_SELECTED))
			return (this->m_pTree == this->GetFocus())?m_pTree->GetBkColor():RGB(0x0,0x0,0x0);
		else
			return RGB(0x00,0x00,0x00);
	}

	BOOL Sort(int col, HTREEITEM item, PFNTVCOMPARE CompareFunc)
	{
		TVSORTCB tvs; 
		tvs.hParent = item;
		tvs.lpfnCompare = CompareFunc;
		tvs.lParam = (LPARAM) this;

		m_sort_col = col;

		return m_pTree->SortChildrenCB(&tvs);
	}

	BOOL SetToolTipText(HTREEITEM hItem, const CString& text)
	{
		VerifyOwnerThread();

		CRect r;
		m_pTree->GetItemRect(hItem, &r, false);
		m_pTree->GetToolTips()->DelTool(this, (UINT)hItem);
		return m_pTree->GetToolTips()->AddTool(this, text, r, (UINT)hItem);
	}

	BOOL Expand(HTREEITEM hItem, UINT nCode)
	{
		return m_pTree->Expand(hItem, nCode);
	}

	HTREEITEM GetRootItem() const
	{
		return m_pTree->GetRootItem();
	}

	HTREEITEM GetFirstVisibleItem() const
	{
		return m_pTree->GetFirstVisibleItem();
	}

	HTREEITEM GetSelectedItem() const
	{
		return m_pTree->GetSelectedItem();
	}

	HTREEITEM GetNextItem(HTREEITEM nItem, int nFlags) const
	{
		return m_pTree->GetNextItem(nItem, nFlags);
	}

	HTREEITEM InsertItem(LPTVINSERTSTRUCT lpInsertStruct)
	{
		return m_pTree->InsertItem(lpInsertStruct);
	}

	HTREEITEM InsertItem(LPCTSTR lpszItem, int nImage, int nSelectedImage, HTREEITEM hParent = TVI_ROOT, HTREEITEM hInsertAfter = TVI_LAST)
	{
		return m_pTree->InsertItem(lpszItem, nImage, nSelectedImage, hParent, hInsertAfter);
	}

	CImageList* SetImageList(CImageList* pImageList, int nImageList)
	{
		return m_pTree->SetImageList(pImageList, nImageList);
	}

	BOOL GetItemImage(HTREEITEM hItem, int& nImage, int& nSelectedImage) const
	{
		return m_pTree->GetItemImage(hItem, nImage, nSelectedImage);
	}

	BOOL SetItem(TVITEM* pItem)
	{
		return m_pTree->SetItem(pItem);
	}

	HTREEITEM HitTest(CPoint pt, UINT* pFlags = NULL) const
	{
		return m_pTree->HitTest(pt, pFlags);
	}
public:
	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};