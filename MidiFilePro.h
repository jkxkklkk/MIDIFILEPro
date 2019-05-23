// MidiFilePro.h : main header file for the MIDIFILEPRO application
//

#if !defined(AFX_MIDIFILEPRO_H__4EE9577D_9911_435B_B510_6EA4527D5E79__INCLUDED_)
#define AFX_MIDIFILEPRO_H__4EE9577D_9911_435B_B510_6EA4527D5E79__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CMidiFileProApp:
// See MidiFilePro.cpp for the implementation of this class
//

class CMidiFileProApp : public CWinApp
{
public:
	CMidiFileProApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMidiFileProApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CMidiFileProApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MIDIFILEPRO_H__4EE9577D_9911_435B_B510_6EA4527D5E79__INCLUDED_)
