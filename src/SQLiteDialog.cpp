/*
* Copyright (c) 2005-2010 Eduard Heidt http://fs-inspect.sourceforge.net.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#include "stdafx.h"
#include "SQLiteDialog.h"

// CSQLiteDialog dialog

IMPLEMENT_DYNAMIC(CSQLiteDialog, CDialog)
CSQLiteDialog::CSQLiteDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CSQLiteDialog::IDD, pParent)
	, m_sQuery(_T("SELECT * FROM file WHERE name LIKE '%';"))
{}

CSQLiteDialog::~CSQLiteDialog()
{
}

void CSQLiteDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_QUERY, m_sQuery);
	DDX_Control(pDX, IDC_TABLE, m_lstTable);
	DDX_Control(pDX, IDC_QUERY, m_editQuery);
	DDX_Control(pDX, IDC_DOQUERY, m_btnDoQuery);
	DDX_Control(pDX, IDC_EXPORT, m_btnExport);
}


BEGIN_MESSAGE_MAP(CSQLiteDialog, CDialog)
	ON_BN_CLICKED(IDC_DOQUERY, OnDoQuery)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_EXPORT, OnBnClickedExport)
END_MESSAGE_MAP()


// CSQLiteDialog message handlers

void CSQLiteDialog::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	CRect r;
	this->GetClientRect(&r);

	if(this->m_editQuery)
	{
		this->m_editQuery.MoveWindow(r.left + 10, r.top + 10, r.Width()-20, 160);
		this->m_btnDoQuery.MoveWindow(r.left + 10, r.top + 180, 120, 30);
		this->m_btnExport.MoveWindow(r.left + 140, r.top + 180, 140, 30);
		this->m_lstTable.MoveWindow(r.left + 10, r.top + 220, r.Width() -20, r.Height() - 230 );
	}
}

BOOL CSQLiteDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CSQLiteDialog::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class

	if(pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_F5)
		this->OnDoQuery();

	if(pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_F12)
		this->OnBnClickedExport();

	return CDialog::PreTranslateMessage(pMsg);
}

void CSQLiteDialog::OnDoQuery()
{
	UpdateData();

	ScopedNamedMutex(_T("DB"));

	try
	{
		CppSQLite3Query q = theApp.m_db.execQuery( this->m_sQuery);

		this->m_lstTable.DeleteAllItems();
		while(this->m_lstTable.DeleteColumn(0));

		// 8 Pixel pro Buchstabe
		CRect r;
		this->m_lstTable.GetClientRect(&r);
		r.right -= 20;

		int sumwidth = 0;
		for(int i = 0; i < q.numFields(); i++)
		{
			int width = wcslen(q.fieldName(i)) * 8 + 15;
			this->m_lstTable.InsertColumn(i, q.fieldName(i), LVCFMT_LEFT,width);
			sumwidth += width;
		}
		while(!q.eof() && this->m_lstTable.GetItemCount() < 500)
		{

			int item = 0;
			//this->m_lstTable.SetRedraw(FALSE);
			for(int i = 0; i < q.numFields(); i++)
			{
				CString field = q.fieldValue(i);

				if(field && i < 20)
				{

					if(i==0)
						item = this->m_lstTable.InsertItem(i, field);
					else
						this->m_lstTable.SetItemText(item, i, field); 

					int width = field.GetLength() * 8 + 15;
					int div = width - this->m_lstTable.GetColumnWidth(i);

					if(div > 0 && (sumwidth + div) < r.Width())
					{
						this->m_lstTable.SetColumnWidth(i, width);
						sumwidth += div;
					}
					/*else if( div > 0 )
					{
					width = r.Width() - sumwidth;
					div = width - this->m_lstTable.GetColumnWidth(i);
					this->m_lstTable.SetColumnWidth(i, width);
					sumwidth += div;
					}*/
				}
				else
					item = this->m_lstTable.InsertItem(0, _T("null"));

			}
			//this->m_lstTable.SetRedraw(TRUE);
			q.nextRow();
		}

		CString tmp; tmp.Format(_T("%s SQL Query: %d Records"), APPNAME, this->m_lstTable.GetItemCount());
		this->SetWindowText(tmp);

	}
	catch (CppSQLite3Exception e)
	{
		AfxMessageBox(e.errorMessage());
	}
	//catch(...)
	//{
	//	AfxMessageBox(_T("BUG"));
	//}

}


void CSQLiteDialog::OnBnClickedExport()
{
	CFileDialog dlg(false, _T("csv"), _T("export"), OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT, _T("CSV Files (*.csv)|*.csv||"));
	//---------------------------------------------------------

	if(dlg.DoModal()==IDCANCEL)
		return;

	CString file_name = dlg.GetFileName();

	if (file_name.IsEmpty()) return;

	int records = 0;

	//---------------
	FILE  *file = NULL;
	if( (file = _wfopen(file_name, _T("w+"))) != NULL)
	{
		ScopedNamedMutex(_T("DB"));

		try
		{
			CppSQLite3Query q = theApp.m_db.execQuery( this->m_sQuery);

			for(int i = 0; i < q.numFields(); i++)
			{
				fwprintf(file, _T("\"%s\""), q.fieldName(i));
				fwprintf(file, _T("%c"), (i<q.numFields()-1)?',':'\n');
			}
			while(!q.eof())
			{
				for(int i = 0; i < q.numFields(); i++)
				{
					fwprintf(file, _T("\"%s\""), q.fieldValue(i));
					fwprintf(file, _T("%c"), (i<q.numFields()-1)?',':'\n');
	
				}
				records++;
				q.nextRow();
			}
		}
		catch (CppSQLite3Exception e)
		{
			AfxMessageBox(e.errorMessage());
		}
	}
	if(file) fclose(file);

	CString msg; msg.Format(_T("%d Records exported."), records);
	AfxMessageBox(msg);

}
