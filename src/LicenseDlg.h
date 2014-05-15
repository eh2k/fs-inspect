/*
* Copyright (c) 2005-2010 Eduard Heidt http://fs-inspect.sourceforge.net.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#pragma once

#include "resource.h"

//WelcomePage

class CLicenseDialog : public CDialog
{
	DECLARE_DYNAMIC(CLicenseDialog)

public:
	CLicenseDialog();
	virtual ~CLicenseDialog();

	// Dialog Data
	enum { IDD = IDD_LICENSE };

protected:
	//	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
};