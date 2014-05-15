/***********************************************************************
***********************************************************************/
#include "../StdAfx.h"

#include <windows.h>
#include <tchar.h>
#include <stdio.h>

#include "AltStreams.h"


void ListStreams(CString file, std::vector<CString> *streams, std::vector<ULONGLONG> *ssizes) 
{
	NTQUERYINFORMATIONFILE NtQueryInformationFile;
	int iRetCode = EXIT_FAILURE;

	LPBYTE pInfoBlock = NULL;
	ULONG uInfoBlockSize = 0;
	IO_STATUS_BLOCK ioStatus;
	NTSTATUS status;
	HANDLE hFile;

	// Load function pointer
	(FARPROC&)NtQueryInformationFile = ::GetProcAddress(::GetModuleHandle(_T("ntdll.dll")), "NtQueryInformationFile");
	
	if (NtQueryInformationFile == NULL) 
		throw ::GetLastError();

	// Obtain SE_BACKUP_NAME privilege (required for opening a directory)
	HANDLE hToken = NULL;
	TOKEN_PRIVILEGES tp;
	
	try 
	{
		if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken)) throw ::GetLastError();
		if (!::LookupPrivilegeValue(NULL, SE_BACKUP_NAME, &tp.Privileges[0].Luid))  throw ::GetLastError();
		tp.PrivilegeCount = 1;
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		if (!::AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL))  throw ::GetLastError();
	}
	catch (DWORD) { }   // Ignore errors
	
	if (hToken) ::CloseHandle(hToken);

	hFile = ::CreateFile(file, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	
	if (hFile == INVALID_HANDLE_VALUE) 
		throw ::GetLastError();

	// Get stream information block.
	// The amount of memory required for info block is unknown, so we
	// allocate 16, 32, 48 kb and so on until the block is sufficient.
	do 
	{
		uInfoBlockSize += 16 * 1024;
		delete [] pInfoBlock;
		pInfoBlock = new BYTE [uInfoBlockSize];
		((PFILE_STREAM_INFORMATION)pInfoBlock)->StreamNameLength = 0;
		status = NtQueryInformationFile(hFile, &ioStatus, (LPVOID)pInfoBlock, uInfoBlockSize, FileStreamInformation);
	} 
	while (status == STATUS_BUFFER_OVERFLOW);
	
	::CloseHandle(hFile);

	PFILE_STREAM_INFORMATION pStreamInfo = (PFILE_STREAM_INFORMATION)(LPVOID)pInfoBlock;
	LARGE_INTEGER fsize;
	WCHAR wszStreamName[MAX_PATH];

	// Loop for all streams
	for (;;) 
	{
		// Check if stream info block is empty (directory may have no stream)
		if (pStreamInfo->StreamNameLength == 0) 
			break; // No stream found

		// Get stream name
		memcpy(wszStreamName, pStreamInfo->StreamName, pStreamInfo->StreamNameLength);
		wszStreamName[pStreamInfo->StreamNameLength / sizeof(WCHAR)] = L'\0';

		// Get stream size
		hFile = ::CreateFile(file+wszStreamName, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
		
		if (hFile == INVALID_HANDLE_VALUE) 
			throw ::GetLastError();
		if (!::GetFileSizeEx(hFile, &fsize)) 
			throw ::GetLastError();
		
		::CloseHandle(hFile);

		
		//printf("  %s%I64u\n", szStreamName, fsize.QuadPart);
		if(streams)
			streams->push_back(file.Right(file.GetLength()-file.ReverseFind('\\')-1) + wszStreamName);
		if(ssizes)
			ssizes->push_back(fsize.QuadPart);

		if (pStreamInfo->NextEntryOffset == 0) 
			break;   // No more stream info records
			
		pStreamInfo = (PFILE_STREAM_INFORMATION)((LPBYTE)pStreamInfo + pStreamInfo->NextEntryOffset);   // Next stream info record
	}

	delete [] pInfoBlock;
}
