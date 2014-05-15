/* TreeListCtrl.cpp	

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
#include "stdafx.h"
#include "TreeListCtrl.h"
#include <windowsx.h>

class TreeButton: public CButton
{
	CTreeListCtrl* Parent() 
	{ 
		return (CTreeListCtrl*)this->GetParent();
	}

public:

	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClicked();
};

class TreeHeader: public CHeaderCtrl
{
public:

	TreeHeader():
		m_sortdown(-1)
	{
	}

	int m_sortdown;

	CTreeListCtrl* Parent() 
	{ 
		return (CTreeListCtrl*)this->GetParent();
	}

	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
public:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
};

//////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(TreeHeader, CHeaderCtrl)
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()

void TreeHeader::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	CDC dc;

	dc.Attach( lpDrawItemStruct->hDC );

	// Save DC
	int nSavedDC = dc.SaveDC();

	// Get the column rect
	CRect rcLabel( lpDrawItemStruct->rcItem );

	// Set clipping region to limit drawing within column
	CRgn rgn;
	rgn.CreateRectRgnIndirect( &rcLabel );
	dc.SelectObject( &rgn );
	rgn.DeleteObject();

	// Labels are offset by a certain amount  
	// This offset is related to the width of a space character
	int offset = dc.GetTextExtent(_T(" "), 1 ).cx*2;

	// Draw image from image list

	// Get the column text and format
	TCHAR buf[256];
	HD_ITEM hditem;

	hditem.mask = HDI_TEXT | HDI_FORMAT;
	hditem.pszText = buf;
	hditem.cchTextMax = 255;

	GetItem( lpDrawItemStruct->itemID, &hditem );

	// Determine format for drawing column label
	UINT uFormat = DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP 
		| DT_VCENTER | DT_END_ELLIPSIS ;

	if( hditem.fmt & HDF_CENTER)
		uFormat |= DT_CENTER;
	else if( hditem.fmt & HDF_RIGHT)
		uFormat |= DT_RIGHT;
	else
		uFormat |= DT_LEFT;

	if(!(uFormat & DT_RIGHT))
	{
		// Adjust the rect if the mouse button is pressed on it
		if( lpDrawItemStruct->itemState == ODS_SELECTED )
		{
			rcLabel.left++;
			rcLabel.top += 2;
			rcLabel.right++;
		}

		rcLabel.left += offset;
		rcLabel.right -= offset;

		// Draw column label
		if( rcLabel.left < rcLabel.right )
			dc.DrawText(buf,-1,rcLabel, uFormat);
	}


	//-----------------------------------------------------------------------------------------

	if(uFormat & 0x0200 || lpDrawItemStruct->itemID == m_sortdown) //...HDF_SORTDOWN
	{
		CPoint points[4];
		if(this->Parent()->m_sort_asc)
		{
			points[0].x = rcLabel.right - 6;
			points[0].y = rcLabel.top + rcLabel.Height()/2 - 2;
			points[1].x = points[0].x + 4;
			points[1].y = points[0].y;
			points[2].x = points[0].x + 2;
			points[2].y = points[0].y + 2;
			points[3] = points[0];
		}
		else
		{
			points[0].x = rcLabel.right - 6;
			points[0].y = rcLabel.top + rcLabel.Height()/2 ;
			points[1].x = points[0].x + 4;
			points[1].y = points[0].y;
			points[2].x = points[0].x + 2;
			points[2].y = points[0].y - 2;
			points[3] = points[0];
		}
		dc.Polyline(points, 4);
	}

	// Restore dc
	dc.RestoreDC( nSavedDC );

	// Detach the dc before returning
	dc.Detach();

	//if(m_TreeCtrl)
	//		m_TreeCtrl->RedrawWindow();

	return __super::DrawItem(lpDrawItemStruct);
}


void TreeHeader::OnLButtonDown(UINT nFlags, CPoint point)
{
	CHeaderCtrl::OnLButtonDown(nFlags, point);
}


void TreeHeader::OnLButtonUp(UINT nFlags, CPoint point)
{
	CHeaderCtrl::OnLButtonUp(nFlags, point);

	for(int i = 0; i < this->GetItemCount(); i++)
	{
		CRect r;
		this->GetItemRect(i, &r);

		bool sortit = this->Parent()->colwidth[i] == r.Width();
		this->Parent()->colwidth[i] = r.Width();

		r.left +=8;
		r.right-=8;

		if(sortit && r.PtInRect(point))
		{
			if(this->m_sortdown == i)
				this->Parent()->m_sort_asc = !this->Parent()->m_sort_asc;

			this->Parent()->OnHeaderColumnClick(i);
			TRACE("col %d asc %d\n", i, this->Parent()->m_sort_asc);
			this->m_sortdown = i;
			this->RedrawWindow();
		}
		else
			this->Parent()->m_pTree->RedrawWindow();
	}
}

///////////////////////////////////
// TreeControl

class TreeControl : public CTreeCtrl
{
public:
	HTREEITEM m_FirstSelItem;
	HTREEITEM GetFirstSel()
	{
		return m_FirstSelItem;
	}
	void SetFirstSel(HTREEITEM item)
	{
		this->m_FirstSelItem = item;
	}

	void ClearSelection();
	BOOL SelectItems(HTREEITEM hItemFrom, HTREEITEM hItemTo);
	HTREEITEM GetFirstSelectedItem();
	HTREEITEM GetNextSelectedItem( HTREEITEM hItem );
	HTREEITEM GetPrevSelectedItem( HTREEITEM hItem );

	CDC m_dc;
	CBitmap m_backBuffer;
	int m_paintCounter;

public:

	CTreeListCtrl* Parent() 
	{ 
		return (CTreeListCtrl*)this->GetParent();
	}

public:
	TreeControl();
	virtual ~TreeControl();

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);

	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);

	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);

	afx_msg void OnNMClick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult);

	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnTvnItemexpanding(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnTvnSelchanged(NMHDR *pNMHDR, LRESULT *pResult);
};

BEGIN_MESSAGE_MAP(TreeControl, CTreeCtrl)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_CONTEXTMENU()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_KEYDOWN()
	ON_WM_KILLFOCUS()
	ON_WM_SETFOCUS()
	ON_WM_LBUTTONDOWN()
	ON_NOTIFY_REFLECT(NM_CLICK, OnNMClick)
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDING, &TreeControl::OnTvnItemexpanding)
	ON_WM_SIZE()
	ON_WM_LBUTTONDBLCLK()
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, &TreeControl::OnTvnSelchanged)
END_MESSAGE_MAP()


TreeControl::TreeControl()
{
	m_paintCounter = 0;
	this->SetFirstSel(NULL);
	m_dc.CreateCompatibleDC(NULL);
}

TreeControl::~TreeControl()
{

}

void TreeControl::OnSize(UINT nType, int cx, int cy)
{
	CTreeCtrl::OnSize(nType, cx, cy);

	if(this->Parent()->GetColumnCount() > 0)
	{
		CRect w;
		this->Parent()->GetClientRect(&w);

		CRect buttonRect;
		this->Parent()->m_pButton->GetWindowRect(&buttonRect);

		int newWidth = w.Width() - buttonRect.Width();

		for(int i = 1; i < this->Parent()->GetColumnCount(); i++)
			newWidth -= this->Parent()->GetColumnWidth(i);

		if(newWidth > 0)
			this->Parent()->SetColumnWidth(0, newWidth);
	}

	this->Invalidate();
}

#define DDBUFF

BOOL TreeControl::OnEraseBkgnd(CDC* pDC)
{
#ifdef DDBUFF

	UNUSED_ALWAYS(pDC);
	return TRUE; //return __super::OnEraseBkgnd(pDC);
#else
	return CTreeCtrl::OnEraseBkgnd(pDC);
#endif

}

static std::map<COLORREF, HBRUSH> s_brushes;
static std::map<COLORREF, HPEN> s_pens;

void TreeControl::OnPaint()
{
	CPaintDC paintDc(this);
	CRect clientRect;
	GetClientRect(&clientRect);

	//TRACE("OnPaint %s :: %s - PC %d\n", typeid(*this->GetParent()).name(), __FUNCTION__, m_paintCounter++);

#ifdef DDBUFF
	CDC& dc = m_dc;
	//dc.LPtoDP(clientRect);

	BITMAP bmp;

	if(m_backBuffer.m_hObject != NULL)
		m_backBuffer.GetBitmap(&bmp);

	if(m_backBuffer.m_hObject == NULL || bmp.bmWidth != clientRect.Width() || bmp.bmHeight != clientRect.Height())
	{
		if(m_backBuffer.m_hObject != NULL)
		{
			dc.SelectObject(m_backBuffer);
			paintDc.BitBlt(clientRect.left, clientRect.top, clientRect.Width(), clientRect.Height(),  &dc, 0, 0, SRCCOPY); 
			::DeleteObject(m_backBuffer.m_hObject);
			m_backBuffer.m_hObject = NULL;
		}

		m_backBuffer.CreateCompatibleBitmap(&paintDc, clientRect.Width(), clientRect.Height());
	}

	dc.SelectObject(m_backBuffer);

#else
	CDC &dc = paintDc;
#endif

	CWnd::DefWindowProc(WM_PAINT, (WPARAM)dc.m_hDC, 0);

	dc.SetBkMode(OPAQUE );

	HTREEITEM hItem = GetFirstVisibleItem();
	CRect r;
	CFont *pFont, *pFont2;

	while(hItem != NULL)
	{
		this->GetItemRect(hItem, &r, true);

		if(!clientRect.PtInRect(r.TopLeft()))
		{
			hItem = GetNextVisibleItem(hItem);
			break;
		}

		this->GetToolTips()->SetToolRect(this, (UINT)hItem, &r);

		//do here what you want
		//-----------------------------------------------------------------------------------------
		pFont = this->GetFont();
		pFont2 = dc.SelectObject(pFont);

		//------------
		{
			COLORREF rgb = this->Parent()->GetTextBkColor(hItem);
			auto brush = s_brushes.find(this->Parent()->GetTextBkColor(hItem));
			if(brush == s_brushes.end())
			{
				dc.SelectObject( s_brushes[rgb] = ::CreateSolidBrush(rgb));//Inhalt
			}
			else
				dc.SelectObject(brush->second);

			auto pen = s_pens.find(rgb);
			if(pen == s_pens.end())
			{
				dc.SelectObject( s_pens[rgb] = ::CreatePen(PS_SOLID, 1, rgb));
			}
			else
				dc.SelectObject(pen->second);

			r.right = clientRect.right;
			dc.Rectangle(&r);

			dc.SetBkColor(this->Parent()->GetTextBkColor(hItem));
			dc.SetTextColor(this->Parent()->GetTextColor(hItem));

			r.top+=1;
			r.right = this->Parent()->GetColumnWidth(0);
			dc.DrawText(this->Parent()->GetItemText(hItem, 0), r, DT_SINGLELINE | DT_WORD_ELLIPSIS | this->Parent()->align[0]);
		}
		//-----------------------------------------------------------------------------------------

		r.left = 0;
		dc.SetBkColor(this->Parent()->GetTextBkColor(hItem));
		dc.SetTextColor(this->Parent()->GetTextColor(hItem));

		for (int i = 0; i < this->Parent()->m_pHeader->GetItemCount()-1; i++)
		{
			r.left += this->Parent()->GetColumnWidth(i);
			r.right = r.left + this->Parent()->GetColumnWidth(i+1);

			//dc.SetTextAlign(align[i+1]);
			dc.DrawText(this->Parent()->GetItemText(hItem, i+1), &r, DT_SINGLELINE | DT_WORD_ELLIPSIS | this->Parent()->align[i+1]);
		}

#define PROCENT
#ifdef PROCENT

		static CBrush s_brush(RGB(0xff,0xff,0xff));//Inhalt
		static CPen s_pen(PS_SOLID, 1, RGB(0xbb,0xbb,0xbb));

		dc.SelectObject(s_brush.GetSafeHandle());
		dc.SelectObject(s_pen.GetSafeHandle());
		dc.SetTextColor(RGB(0x0,0x0,0x0));

		float val = this->Parent()->GetPercentValue(hItem);

		if(this->Parent()->m_procentcol != -1 && this->Parent()->m_procentcol < this->Parent()->m_pHeader->GetItemCount())
			if( val > 0)
			{
				if(val>100)
					val=100;

				r.top -=1;
				r.left = 0;
				for (int i = 0; i < this->Parent()->m_procentcol; i++)
				{
					r.left += this->Parent()->GetColumnWidth(i);
					r.right = r.left + this->Parent()->GetColumnWidth(i+1);
				}

				int width = this->Parent()->GetColumnWidth(this->Parent()->m_procentcol) -20;

				r.top += 1;	r.bottom -= 1;
				r.left += 10;    r.right = r.left + width;
				dc.Rectangle(&r);							// der schwarze Rahmen


				CRect bak(r);
				r.left +=1;	r.top +=1;
				r.bottom -= 1; r.right = r.left + (val*width/100)-2; //der rote Balken im Rahmen
				if(r.right < r.left) r.right = r.left;
				dc.FillSolidRect(&r,RGB(255,255 - (val)*2,0));	

				dc.SetBkMode(TRANSPARENT);
				dc.SetBkColor(RGB(0xff,0xff,0xff));

				CString tmp = _T("0.0%");
				if(val>0.1)
					tmp.Format(_T("%.1f%%"), val);

				dc.DrawText(tmp, &bak, DT_SINGLELINE | DT_VCENTER | DT_CENTER);

				dc.SetBkMode(OPAQUE );
			}
#endif

			hItem = GetNextVisibleItem(hItem);
	}

	paintDc.BitBlt(clientRect.left, clientRect.top, clientRect.Width(), clientRect.Height(),  &dc, 0, 0, SRCCOPY);  
}

#define BIGSEL 20
void TreeControl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	return CTreeCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}




void TreeControl::OnTvnSelchanged(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);

	Parent()->OnItemOperation(ID_SELECT);

	*pResult = 0;
}



BOOL TreeControl::PreTranslateMessage(MSG* pMsg)
{
	//Context menu via the keyboard
	///////////////

	if ((((pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSKEYDOWN) && // If we hit a key and
		(pMsg->wParam == VK_F10) && (GetKeyState(VK_SHIFT) & ~1)) != 0) ||   // it's Shift+F10 OR
		(pMsg->message == WM_CONTEXTMENU))						                   	   // Natural keyboard key
	{
		CRect rect;
		GetItemRect(GetSelectedItem(), rect, TRUE);
		ClientToScreen(rect);
		OnContextMenu(NULL, rect.CenterPoint());
		return TRUE;
	}
	//Hitting the Alt-Enter key combination, show the properties sheet 
	else if (pMsg->message == WM_SYSKEYDOWN && pMsg->wParam == VK_RETURN)
	{
		Parent()->OnItemOperation(ID_PROPERTIES);
		return TRUE;
	}
	//Hitting the Enter key, open the item
	else if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
	{
		Parent()->OnItemOperation(ID_OPEN);
		return TRUE;
	}
	//Hitting the delete key, delete the item
	else if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_DELETE)
	{
		ASSERT(!"Todo");
		Parent()->OnItemOperation(ID_DELETE);
		return TRUE;
	}
	//hitting the backspace key, go to the parent folder
	else if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_BACK)
	{
		//UpOneLevel();
		return TRUE;
	}
	//hitting the F2 key, being in-place editing of an item
	else if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_F2)
	{
		//OnFileRename();
		return TRUE;
	}
	//hitting the F5 key, force a refresh of the whole tree
	else if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_F5)
	{
		//OnViewRefresh();
		return TRUE;
	}
	else if ( pMsg->wParam == 'a' || pMsg->wParam == 'A' )
	{
		//Select ALL!!!
		return TRUE;
	}


	return __super::PreTranslateMessage(pMsg);
}

void TreeControl::OnKillFocus(CWnd* pNewWnd)
{
	__super::OnKillFocus(pNewWnd);

}

void TreeControl::OnSetFocus(CWnd* pOldWnd)
{
	__super::OnSetFocus(pOldWnd);
}


void TreeControl::OnLButtonDown(UINT nFlags, CPoint point)
{
	CTreeCtrl::OnLButtonDown(nFlags, point);
}

void TreeControl::ClearSelection()
{
	for ( HTREEITEM hItem=GetRootItem(); hItem!=NULL; hItem=GetNextItem( hItem , TVGN_NEXT ) )
		if ( GetItemState( hItem, TVIS_SELECTED ) & TVIS_SELECTED )
			SetItemState( hItem, 0, TVIS_SELECTED );
}
BOOL TreeControl::SelectItems(HTREEITEM hItemFrom, HTREEITEM hItemTo)
{
	HTREEITEM hItem = GetRootItem();

	// Clear selection upto the first item
	while ( hItem && hItem!=hItemFrom && hItem!=hItemTo )
	{
		hItem = GetNextVisibleItem( hItem );
		SetItemState( hItem, 0, TVIS_SELECTED );
	}

	if ( !hItem )
		return FALSE;	// Item is not visible

	SelectItem( hItemTo );

	// Rearrange hItemFrom and hItemTo so that hItemFirst is at top
	if( hItem == hItemTo )
	{
		hItemTo = hItemFrom;
		hItemFrom = hItem;
	}


	// Go through remaining visible items
	BOOL bSelect = TRUE;
	while ( hItem )
	{
		// Select or remove selection depending on whether item
		// is still within the range.
		SetItemState( hItem, bSelect ? TVIS_SELECTED : 0, TVIS_SELECTED );

		// Do we need to start removing items from selection
		if( hItem == hItemTo )
			bSelect = FALSE;

		hItem = GetNextVisibleItem( hItem );
	}

	return TRUE;
}
HTREEITEM TreeControl::GetFirstSelectedItem()
{
	for ( HTREEITEM hItem = GetRootItem(); hItem!=NULL; hItem = GetNextItem( hItem, TVGN_NEXT ) )
		if ( GetItemState( hItem, TVIS_SELECTED ) & TVIS_SELECTED )
			return hItem;

	return NULL;
}

HTREEITEM TreeControl::GetNextSelectedItem( HTREEITEM hItem )
{
	for ( hItem = GetNextItem( hItem,TVGN_NEXT ); hItem!=NULL; hItem = GetNextItem( hItem , TVGN_NEXT) )
		if ( GetItemState( hItem, TVIS_SELECTED ) & TVIS_SELECTED )
			return hItem;

	return NULL;
}

HTREEITEM TreeControl::GetPrevSelectedItem( HTREEITEM hItem )
{
	for ( hItem = GetNextItem( hItem, TVGN_PREVIOUS ); hItem!=NULL; hItem = GetNextItem( hItem , TVGN_PREVIOUS) )
		if ( GetItemState( hItem, TVIS_SELECTED ) & TVIS_SELECTED )
			return hItem;

	return NULL;
}

void TreeControl::OnNMClick(NMHDR *pNMHDR, LRESULT *pResult)
{
	DWORD dwPos = GetMessagePos();

	CPoint p( GET_X_LPARAM( dwPos ), GET_Y_LPARAM( dwPos ) ), spt;

	ScreenToClient(&p);
	HTREEITEM selected = this->HitTest(p);
	this->Select(selected,TVGN_CARET);
	*pResult = 0;
}

void TreeControl::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	this->Parent()->OnItemOperation(ID_OPEN);
	CTreeCtrl::OnLButtonDblClk(nFlags, point);
}

void TreeControl::OnRButtonDown(UINT nFlags, CPoint point)
{
	HTREEITEM selected = this->HitTest(point);
	this->Select(selected,TVGN_CARET);
}


void TreeControl::OnRButtonUp(UINT nFlags, CPoint point)
{
	HTREEITEM selected = this->GetSelectedItem();

	if(!selected) 
		return;

	CMenu popup;
	popup.CreatePopupMenu();

	Parent()->FillMenu(popup);

	ClientToScreen(&point);
	Parent()->OnItemOperation(popup.TrackPopupMenu(TPM_RIGHTBUTTON |TPM_RETURNCMD, point.x, point.y, this));
}

void TreeControl::OnTvnItemexpanding(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);

	Parent()->OnItemExpanding(pNMTreeView);
	*pResult = 0;
}



BEGIN_MESSAGE_MAP(TreeButton, CButton)
	ON_CONTROL_REFLECT(BN_CLICKED, OnBnClicked)
END_MESSAGE_MAP()

void TreeButton::OnBnClicked()
{
	this->Parent()->OnZoomButton();
}

BEGIN_MESSAGE_MAP(CTreeListCtrl, CStatic)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

CTreeListCtrl::CTreeListCtrl():
	m_procentcol(-1),
	m_pTree(new TreeControl()),
	m_pHeader(new TreeHeader()),
	m_pButton(new TreeButton())
{
}

CTreeListCtrl::~CTreeListCtrl()
{
	delete m_pTree;
	delete m_pHeader;
	delete m_pButton;
	m_procentcol = -1;
}

int CTreeListCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CStatic::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect rectDummy(0,0,0, 20);

	m_pHeader->Create(
		WS_CHILD | 
		WS_VISIBLE | 
		HDS_BUTTONS |  //HDS_DRAGDROP | HDS_FULLDRAG |
		HDS_HORZ,
		rectDummy, this, -1);

	m_pTree->Create(  
		WS_TABSTOP |
		WS_CHILD |
		WS_VISIBLE |
		TVS_HASBUTTONS |
		TVS_HASLINES |
		//TVS_LINESATROOT |
		TVS_SHOWSELALWAYS |
		WS_CLIPSIBLINGS|
		TVS_DISABLEDRAGDROP |
		//TVS_CHECKBOXES |		// These style constant is needed !
		//TVS_TRACKSELECT |
		TVS_FULLROWSELECT |
		0,
		rectDummy, 
		this, 
		-1);

	m_pTree->SetBkColor(RGB(0xff,0xff,0xff));
	//m_pTree->SetItemHeight(16);

	m_pHeader->SetFont(m_pTree->GetFont());

	m_font.CreateFont(0,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE, DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH|FF_DONTCARE,L"Webdings");

	m_pButton->Create(L"1", WS_CHILD|WS_VISIBLE|BS_FLAT|BS_BOTTOM, CRect(0,0,0,0), this, -1);
	m_pButton->SetFont(&m_font);

	return TRUE;

	return 0;
}


void CTreeListCtrl::OnSize(UINT nType, int cx, int cy)
{
	CStatic::OnSize(nType, cx, cy);

	CRect rect; 
	this->m_pTree->GetClientRect(&rect);
	int x = rect.left;
	int y = rect.top;
	int nWidth = cx;
	int nHeight = cy;
	BOOL bRepaint = FALSE;

	CRect r;
	this->m_pHeader->GetWindowRect(&r);
	this->m_pButton->MoveWindow(x+nWidth-r.Height(),y,r.Height(),r.Height(),bRepaint);
	this->m_pHeader->MoveWindow(x,y,nWidth-r.Height(),r.Height(),bRepaint);
	this->m_pTree->MoveWindow(x,y+r.Height(),nWidth,nHeight-r.Height(),bRepaint);
}


void CTreeListCtrl::OnSetFocus(CWnd* pOldWnd)
{
	CStatic::OnSetFocus(pOldWnd);

	this->m_pTree->SetFocus();
}

BOOL CTreeListCtrl::OnEraseBkgnd(CDC* pDC)
{
	return TRUE; //return CStatic::OnEraseBkgnd(pDC);
}
