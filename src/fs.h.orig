/*
* Copyright (c) 2005-2010 Eduard Heidt http://fs-inspect.sourceforge.net.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#pragma once

#include <vector>
#include <hash_map>
#include <functional>

typedef std::string HASH;

//typedef HASH HASHkey;

struct HASHkey
{
	const HASH& _hash;

	HASHkey(const HASH& hash):
		_hash(hash)
	{}

	inline bool operator < (const HASHkey& other) const
	{
		return strcmp(_hash.c_str(), other._hash.c_str()) < 0;
	}

	operator size_t() const
	{
		const char* s = _hash.c_str();
		size_t hash = 0;
		while (*s)
		{
			hash = hash * 101  +  *s++;
		}
		return hash;
	}
};

inline CString L2A(long l)
{
	TCHAR ca[16];
	_ltow(l, ca, 10);
	return ca;
}

class FSbase
{
protected:
	int m_icon;
	FSbase *m_parent;
public:

	HTREEITEM item;

	FSbase()
	{
		this->m_parent = NULL;
		this->item = NULL;
		this->m_icon = -1;
	}

	virtual ~FSbase()
	{
	}

	FSbase* GetParent() 
	{ 
		return m_parent; 
	}

	virtual int GetIcon();

	virtual unsigned __int64 GetSize() = 0;
	virtual CString GetPath() const = 0;

	CString GetSizeString();
	float GetPercent();

	static CString GetSizeString(unsigned __int64 size);
	float GetPercent(unsigned __int64 size);
};

class FSFile: public FSbase
{
private:
	CString _name;
	CString _ext;
	unsigned __int64 _size;
	FILETIME _modified;

	HASH _hash;

public:

	FSFile(const CString& stream_name, const ULONGLONG& stream_size)
		:FSbase()
	{
		this->_name = stream_name;
		this->_size = stream_size;
		this->_ext = _T(":$Data");

		ZeroMemory(&_modified, sizeof(FILETIME));
	}

	FSFile(const WIN32_FIND_DATA& fdata)
		:FSbase()
	{
		this->_name = fdata.cFileName;
		this->_size = (fdata.nFileSizeHigh * (0x100000000)) + fdata.nFileSizeLow;

		this->_modified = fdata.ftLastWriteTime;

		if(_name.Find('.') != -1)
			_ext = _name.Right(_name.GetLength()-_name.ReverseFind('.')-1).MakeLower();
		else
			_ext = _T("   ");
	}

	virtual ~FSFile()
	{
	}

	void SetParent(FSbase* parent)
	{
		this->m_parent = parent;
	}

	const HASH& GetHash(bool compute = true, bool fromdb = true, std::function<void(void)> step = []{});		//steps = GetSize()/HASHBUFF

	const FILETIME& GetModified()
	{
		return _modified;
	}

	const CString& GetName() const
	{
		return this->_name;
	}

	const CString& GetExt() const
	{
		return _ext;
	}

	CString GetPath() const
	{
		return this->m_parent->GetPath() + GetName();
	}

	unsigned __int64 GetSize()
	{
		return _size;
	}

	CString GetModifiedDateString()
	{
		if(_modified.dwHighDateTime == 0)
			return _T("");
		else
		{
			return CTime(this->GetModified()).Format(theApp.m_settings.GetDateTimeFormat()); //_T("%d.%m.%y  %H:%M"));
		}
	}
};

class FSFolder;

class FSFileSet: public FSbase
{
private:
	std::vector<FSFile*> m_files;
	unsigned __int64 m_filesize;
public:

	FSFileSet(FSFolder* folder);

	virtual ~FSFileSet()
	{
		for(UINT i = 0; i < this->m_files.size(); i++)
			delete (this->m_files[i]);
	}

	FSFile * AddFile(FSFile *file)
	{
		m_files.push_back(file);
		file->SetParent(this);
		m_filesize+= file->GetSize();
		return file;
	}

	CString GetPath() const
	{ 
		return m_parent->GetPath();			
	};

	void RemoveVirtual(FSFile* file)
	{
		for(auto it = m_files.begin(); it != m_files.end(); ++it)
		{
			if(*it == file)
			{
				m_filesize -= file->GetSize();
				m_files.erase(it);
				delete file;
				return;
			}
		}
	}

	unsigned __int64 GetSize()
	{
		return this->m_filesize;
	}

	const std::vector<FSFile*>& GetFiles()
	{
		return m_files;
	}

	void DB_EXPORT(void step(void));
};

class FSFolder: public FSbase
{
private:

	CString m_name;
	std::vector<FSFolder*> m_folders;
	FSFileSet m_files;
public:

	FSFolder::FSFolder():
		FSbase(),
		m_files(this)
	{
		this->m_name = _T("");
	}

	FSFolder(const CString& name, FSFolder *parent = NULL):
		FSbase(), 
		m_files(this)
	{
		this->m_name = name;
		this->m_name.TrimRight(_T("\\"));
		this->m_parent = parent;
	}

	virtual ~FSFolder()
	{
		for(UINT i = 0; i < this->m_folders.size(); i++)
			delete (this->m_folders[i]);
	}

	FSFile* AddFile(FSFile *file)
	{
		return m_files.AddFile(file);
	}

	FSFile* AddFile(const WIN32_FIND_DATA& fdata)
	{
		return AddFile(new FSFile(fdata));
	}

	const CString& GetName()
	{
		return this->m_name;
	}

	CString FSFolder::GetPath() const
	{ 
		if(m_parent)
			return m_parent->GetPath() + m_name + _T("\\");
		else
			return m_name + _T("\\");
	};

	const std::vector<FSFile*>& GetFiles()
	{
		return m_files.GetFiles();
	}

	const std::vector<FSFolder*>& GetSubFolders()
	{
		return this->m_folders;
	}

	FSFileSet& GetFileSet()
	{
		return m_files;
	}

	unsigned __int64 GetSize()
	{
		unsigned __int64 size = 0;
		for(UINT i = 0; i < this->m_folders.size(); i++)
			size += this->m_folders[i]->GetSize();

		size += m_files.GetSize();

		return size;
	}
	unsigned __int64 GetFileSize()
	{
		return m_files.GetSize();
	}

	FSFolder* AddChildFolder(const CString&  folder)
	{
		FSFolder *f = new FSFolder(folder, this);
		this->m_folders.push_back(f);
		return f;
	}

	void RemoveVirtual(FSFolder* folder)
	{
		for(auto it = m_folders.begin(); it != m_folders.end(); ++it)
		{
			if(*it == folder)
			{
				m_folders.erase(it);
				delete folder;
				return;
			}
		}
	}

	void RemoveVirtual(FSFile* file)
	{
		m_files.RemoveVirtual(file);
	}

	size_t GetObjectCount()
	{
		return GetSubFolders().size() + GetFiles().size();
	}

	void DB_EXPORT_RECURSIVE(void step(void));
	void DB_EXPORT(CString dbpath);
};

static int wildcmp(const TCHAR *wild, const TCHAR *string) 
{
	// Written by Jack Handy - jakkhandy@hotmail.com
	const TCHAR *cp = NULL, *mp = NULL;

	while ((*string) && (*wild != '*')) {
		if ((*wild != *string) && (*wild != '?')) {
			return 0;
		}
		wild++;
		string++;
	}

	while (*string) {
		if (*wild == '*') {
			if (!*++wild) {
				return 1;
			}
			mp = wild;
			cp = string+1;
		} else if ((*wild == *string) || (*wild == '?')) {
			wild++;
			string++;
		} else {
			wild = mp;
			string = cp++;
		}
	}

	while (*wild == '*') {
		wild++;
	}
	return !*wild;
}
