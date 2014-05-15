/*
* Copyright (c) 2005-2010 Eduard Heidt http://fs-inspect.sourceforge.net.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#include "stdafx.h"
#include <string>
#include <afxinet.h>
#include <time.h>

std::string CheckForUpdate(const char* httpUrl)
{
	const int capacity = 1024;
	char buffer[capacity];
	buffer[0] = 0;

	try
	{
		time_t t = time(NULL);
		CString url;
		url.Format(_T("%s?%I64u"), CString(httpUrl), t);

		CInternetSession connection;
		if(CStdioFile* stream = connection.OpenURL(url, 1, INTERNET_FLAG_TRANSFER_ASCII|INTERNET_FLAG_DONT_CACHE))
		{
			int bytes_read = stream->Read(buffer, capacity);
			delete stream;
		}
	}
	catch(...)
	{
	}

	return std::string(buffer);
}
