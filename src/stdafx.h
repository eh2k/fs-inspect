/*
* Copyright (c) 2005-2010 Eduard Heidt http://fs-inspect.sourceforge.net.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#pragma once

#pragma warning ( disable : 4267 ) //initializing
#pragma warning ( disable : 4244 ) //conversion
#pragma warning ( disable : 4311 ) //typecast
#pragma warning ( disable : 4312 ) //typecast
#pragma warning ( disable : 4005 ) //macro redefinition
#pragma warning ( disable : 4996 ) // usafe functions

#define _APPNAME_ "FS-Inspect"
#define APPNAME CString(_T(_APPNAME_))

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN		// Selten verwendete Teile der Windows-Header nicht einbinden
#endif


#if(_MSC_VER == 1700)

	#ifndef WINVER
	#define WINVER 0x600
	#endif

	#ifndef _WIN32_WINNT
	#define _WIN32_WINNT 0x600
	#endif						

	#ifndef _WIN32_WINDOWS
	#define _WIN32_WINDOWS 0x600
	#endif

#else

	#ifndef WINVER
	#define WINVER 0x501
	#endif

	#ifndef _WIN32_WINNT
	#define _WIN32_WINNT 0x501
	#endif						

	#ifndef _WIN32_WINDOWS
	#define _WIN32_WINDOWS 0x501
	#endif

#endif


#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// einige CString-Konstruktoren sind explizit

// Deaktiviert das Ausblenden von einigen häufigen und oft ignorierten Warnungen
#define _AFX_ALL_WARNINGS

#define OEMRESOURCE

#include <afxwin.h>         // MFC-Kern- und -Standardkomponenten
#include <afxext.h>         // MFC-Erweiterungen

#include <afxdtctl.h>		// MFC-Unterstützung für allgemeine Steuerelemente von Internet Explorer 4
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC-Unterstützung für allgemeine Windows-Steuerelemente
#endif // _AFX_NO_AFXCMN_SUPPORT

#include "AtlBase.h"
//#include <afxdlgs.h>

#include <afxwinappex.h>
#include <afxframewndex.h>
#include <afxcontrolbars.h>

#include "app.h"
