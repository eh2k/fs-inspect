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
#include <Wincrypt.h>

#define HASHBUFF 4096 //32768 16384 8192 4096 2048

class MD5Hash
{
	HCRYPTPROV hProv;
	HCRYPTHASH hHash;
public:
	MD5Hash()
	{
		CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
		CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash);
	}
	~MD5Hash()
	{
		CryptDestroyHash(hHash);
		CryptReleaseContext(hProv, 0);
	}

	void Update(BYTE* buffer, int len)
	{
		CryptHashData(hHash, buffer, len, 0);
	}

	HASH GetHash()
	{
		const int MD5LEN = 16;
		BYTE rgbHash[MD5LEN];
		CHAR rgbDigits[] = "0123456789abcdef";
		DWORD cbHash = MD5LEN;

		CHAR hash[33];
		hash[0] = 0;
		hash[32] = 0;

		if (CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0))
		{
			for (DWORD i = 0; i < cbHash; i++)
			{
				hash[i*2] = rgbDigits[rgbHash[i] >> 4];
				hash[(i*2)+1] = rgbDigits[rgbHash[i] & 0xf];
			}
		}

		return HASH(hash);
	}
};


static HASH GetMD5(const std::string& in)
{
	MD5Hash hash;
	hash.Update((BYTE*)in.c_str(), in.size());
	return hash.GetHash();
}

static HASH FileGetMD5(const std::wstring& file, std::function<void(void)> step)
{
	MD5Hash md5;

	try
	{
		unsigned long size = 0;
		FILE *f = _wfopen ( file.c_str() , _T("rb") ) ;
		if ( f != NULL )
		{
			byte buf[HASHBUFF] ;
			size_t trb = 0 ;
			for (;;) {
				size_t rb = fread ( buf , 1 , HASHBUFF , f ) ;
				if ( size > 0 && rb + trb > size ) rb = size - trb ;
				trb += rb ;
				md5.Update(buf , rb);
				if(step!=NULL)step();
				if ( rb < HASHBUFF || ( size > 0 && trb >= size ) ) break ;
			}
			fclose ( f ) ;
		}
	}
	catch(...)
	{
		AfxMessageBox(CString("MD5 failed: ") + file.c_str());
	}

	return md5.GetHash();
}

///////////

static CString DBTimeString(const FILETIME *time)
{
	if(time)
		return CTime(*time).Format("%y%m%d%H%M%S");
	return _T("");
}

CString FSbase::GetSizeString(unsigned __int64 size)
{
	if(size == 0)
		return _T("0");

	CString tmp;
	if(theApp.m_settings.GetViewMB())
	{
		tmp.Format(_T("%.1f MB"), (((double)size)/(1024*1024)));
		return tmp;
	}

	if(size >= (1024*1024*1024))
		tmp.Format(_T("%.1f GB"), (((double)size)/(1024*1024*1024)));
	else if(size >= (1024*1024))
		tmp.Format(_T("%.1f MB"), (((double)size)/(1024*1024)));
	else if(size >= 1024)
		tmp.Format(_T("%.1f KB"), (((double)size)/(1024)));
	else 
		tmp.Format(_T("%d B"),  size);

	return tmp;
}

static FSbase * GetRoot(FSbase* node)
{
	if(node->GetParent() == NULL)
		return node;
	else
		return GetRoot(node->GetParent());
}

float FSbase::GetPercent(unsigned __int64 size)
{
	if( theApp.m_settings.GetViewPerc() )
	{
		unsigned __int64 total, free, free2;
		GetDiskFreeSpaceEx(theApp.m_scanpath.GetBuffer(3), (PULARGE_INTEGER)&free, (PULARGE_INTEGER)&total, (PULARGE_INTEGER)&free2);
		theApp.m_scanpath.ReleaseBuffer();
		float ret = ((float)(size)/((float)total)*100.0);//+(float)0.5 ;
		return (ret<=100.5&&ret>=0.0)?ret:0;		
	}
	else
	{
		float ret = ((float)size/(float)GetRoot(this)->GetSize()*100.0);//+(float)0.5 ;
		return (ret<=100.5&&ret>=0.0)?ret:0;
	}
}


int FSbase::GetIcon()
{
	if(this->m_icon == -1)
	{	
		CString path = this->GetPath();

		if(path.Find(_T(":$"))!= -1)
			path = path.Left(path.Find(':'));

		SHFILEINFO sfi;
		ZeroMemory(&sfi, sizeof(SHFILEINFO));
		SHGetFileInfo(path, 0, &sfi, sizeof(SHFILEINFO), SHGFI_ICON);
		this->m_icon = sfi.iIcon;
		DestroyIcon(sfi.hIcon);
	}
	return this->m_icon;
}


CString FSbase::GetSizeString()
{
	return GetSizeString(this->GetSize());
}

float FSbase::GetPercent()
{
	return GetPercent(this->GetSize());
}

const HASH& FSFile::GetHash(bool compute, bool fromdb, std::function<void(void)> step)	//steps GetSize()/HASHBUFF
{
	if(!_hash.empty())
		return _hash;

	try
	{
		CString file_id;
		if(fromdb)
		{
			file_id.Format( L"%s%I64u%I32u%I32u",
				this->GetName(), 
				this->GetSize(),
				this->GetModified().dwHighDateTime,
				this->GetModified().dwLowDateTime
				);

			MD5Hash hash;
			hash.Update((BYTE*)file_id.GetBuffer(), file_id.GetLength()*sizeof(TCHAR));
			file_id.ReleaseBuffer();
			file_id = CString(hash.GetHash().c_str());

			ScopedNamedMutex(_T("DB"));
			CString sql; sql.Format(_T("SELECT md5 FROM hashing WHERE id='%s'"), file_id);
			CppSQLite3Query q = theApp.m_db.execQuery(sql);
			if(!q.eof())
			{
				_hash = CStringA(q.fieldValue(0));
				fromdb = false;
				compute = false;
			}
		}

		if(compute)
		{
			_hash = FileGetMD5(std::wstring(GetPath()), step);
		}

		if(fromdb && !_hash.empty())
		{
			ScopedNamedMutex(_T("DB"));
			CString sql; sql.Format(_T("INSERT INTO hashing (id, md5) VALUES('%s','%s');"), file_id, CString(_hash.c_str()) );
			theApp.m_db.execDML(sql);
		}

	}
	catch (CppSQLite3Exception e)
	{
		AfxMessageBox(e.errorMessage());
	}

	return _hash;
}

FSFileSet::FSFileSet(FSFolder* folder)
	:FSbase()
{
	this->m_icon = 0;
	m_parent = folder;
	m_filesize = 0;
}

void FSFileSet::DB_EXPORT(void step())
{
	for(UINT i = 0; i < this->m_files.size(); i++)
	{
		CString sql;
		const HASH& hash = this->m_files[i]->GetHash(false);
		CString modi = DBTimeString(&this->m_files[i]->GetModified());
		sql.Format(_T("insert into file values (%d,\"%s\",\"%s\",\"%s\",%I64u,"),
			&this->m_files[i],
			this->m_files[i]->GetName(),
			this->m_files[i]->GetExt(),
			this->m_files[i]->GetParent()->GetPath(),
			this->m_files[i]->GetSize()
			);

		sql += "\"";
		sql += modi;
		sql += "\",";
		sql += "\"";
		sql += CString(hash.c_str());
		sql += "\");";

		theApp.m_db.execDML(sql);

		if(step != NULL)
			step();
	}
}

void FSFolder::DB_EXPORT_RECURSIVE(void step(void))
{
	//	theApp.m_db.execDML("begin transaction;");

	this->m_files.DB_EXPORT(step);

	//	theApp.m_db.execDML("commit transaction;");


	for(UINT i = 0; i < this->m_folders.size(); i++)
		this->m_folders[i]->DB_EXPORT_RECURSIVE(step);
}

void FSFolder::DB_EXPORT(CString dbpath)
{
	ScopedNamedMutex(_T("DB"));

	try
	{
		CString sql;
		sql = "create table file(";
		sql += "id int, ";
		sql += "name char(260), ";
		sql += "ext char(8), ";
		sql += "path char(260), ";
		sql += "size BLOB, ";
		sql += "modified char(12), ";
		sql += "hash char(32));";

		if(theApp.m_db.tableExists(_T("file")))
		{
			theApp.m_db.execDML(_T("delete from file;"));
		}
		else 
			theApp.m_db.execDML(sql);

		theApp.m_db.execDML(_T("begin transaction;"));

		CMainFrame::ProgressBarInit(_T("db-export"), 0);

		this->DB_EXPORT_RECURSIVE(CMainFrame::ProgressBarStep);

		CMainFrame::ProgressBarHide();

		theApp.m_db.execDML(_T("commit transaction;"));
	}
	catch (CppSQLite3Exception e)
	{
		AfxMessageBox(e.errorMessage());
	}
}