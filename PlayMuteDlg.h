#if !defined(AFX_PLAYMUTEDLG_H__8EFC905B_FF6F_440E_AC06_89D1DE4F6C29__INCLUDED_)
#define AFX_PLAYMUTEDLG_H__8EFC905B_FF6F_440E_AC06_89D1DE4F6C29__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PlayMuteDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPlayMuteDlg dialog

class CPlayMuteDlg : public CDialog
{
// Construction
public:
	CPlayMuteDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CPlayMuteDlg)
	enum { IDD = IDD_PLAY_MUTE_DIALOG };
	BOOL	m_checkCh1;
	BOOL	m_checkCh10;
	BOOL	m_checkCh11;
	BOOL	m_checkCh12;
	BOOL	m_checkCh13;
	BOOL	m_checkCh14;
	BOOL	m_checkCh15;
	BOOL	m_checkCh16;
	BOOL	m_checkCh2;
	BOOL	m_checkCh3;
	BOOL	m_checkCh4;
	BOOL	m_checkCh5;
	BOOL	m_checkCh6;
	BOOL	m_checkCh7;
	BOOL	m_checkCh8;
	BOOL	m_checkCh9;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPlayMuteDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

public:

	WORD wCh;

	// Generated message map functions
	//{{AFX_MSG(CPlayMuteDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PLAYMUTEDLG_H__8EFC905B_FF6F_440E_AC06_89D1DE4F6C29__INCLUDED_)
