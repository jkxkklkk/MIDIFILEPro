// PlayMuteDlg.cpp : implementation file
//

#include "stdafx.h"
#include "MidiFilePro.h"
#include "PlayMuteDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPlayMuteDlg dialog


CPlayMuteDlg::CPlayMuteDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPlayMuteDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPlayMuteDlg)
	m_checkCh1 = FALSE;
	m_checkCh10 = FALSE;
	m_checkCh11 = FALSE;
	m_checkCh12 = FALSE;
	m_checkCh13 = FALSE;
	m_checkCh14 = FALSE;
	m_checkCh15 = FALSE;
	m_checkCh16 = FALSE;
	m_checkCh2 = FALSE;
	m_checkCh3 = FALSE;
	m_checkCh4 = FALSE;
	m_checkCh5 = FALSE;
	m_checkCh6 = FALSE;
	m_checkCh7 = FALSE;
	m_checkCh8 = FALSE;
	m_checkCh9 = FALSE;
	//}}AFX_DATA_INIT
}


void CPlayMuteDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPlayMuteDlg)
	DDX_Check(pDX, IDC_CHECK1, m_checkCh1);
	DDX_Check(pDX, IDC_CHECK10, m_checkCh10);
	DDX_Check(pDX, IDC_CHECK11, m_checkCh11);
	DDX_Check(pDX, IDC_CHECK12, m_checkCh12);
	DDX_Check(pDX, IDC_CHECK13, m_checkCh13);
	DDX_Check(pDX, IDC_CHECK14, m_checkCh14);
	DDX_Check(pDX, IDC_CHECK15, m_checkCh15);
	DDX_Check(pDX, IDC_CHECK16, m_checkCh16);
	DDX_Check(pDX, IDC_CHECK2, m_checkCh2);
	DDX_Check(pDX, IDC_CHECK3, m_checkCh3);
	DDX_Check(pDX, IDC_CHECK4, m_checkCh4);
	DDX_Check(pDX, IDC_CHECK5, m_checkCh5);
	DDX_Check(pDX, IDC_CHECK6, m_checkCh6);
	DDX_Check(pDX, IDC_CHECK7, m_checkCh7);
	DDX_Check(pDX, IDC_CHECK8, m_checkCh8);
	DDX_Check(pDX, IDC_CHECK9, m_checkCh9);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPlayMuteDlg, CDialog)
	//{{AFX_MSG_MAP(CPlayMuteDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPlayMuteDlg message handlers

BOOL CPlayMuteDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here

	if((wCh & 0x0001) != 0) m_checkCh1 = true; else m_checkCh1 = false;
	if((wCh & 0x0002) != 0) m_checkCh2 = true; else m_checkCh2 = false;
	if((wCh & 0x0004) != 0) m_checkCh3 = true; else m_checkCh3 = false;
	if((wCh & 0x0008) != 0) m_checkCh4 = true; else m_checkCh4 = false;
	if((wCh & 0x0010) != 0) m_checkCh5 = true; else m_checkCh5 = false;
	if((wCh & 0x0020) != 0) m_checkCh6 = true; else m_checkCh6 = false;
	if((wCh & 0x0040) != 0) m_checkCh7 = true; else m_checkCh7 = false;
	if((wCh & 0x0080) != 0) m_checkCh8 = true; else m_checkCh8 = false;
	if((wCh & 0x0100) != 0) m_checkCh9 = true; else m_checkCh9 = false;
	if((wCh & 0x0200) != 0) m_checkCh10 = true; else m_checkCh10 = false;
	if((wCh & 0x0400) != 0) m_checkCh11 = true; else m_checkCh11 = false;
	if((wCh & 0x0800) != 0) m_checkCh12 = true; else m_checkCh12 = false;
	if((wCh & 0x1000) != 0) m_checkCh13 = true; else m_checkCh13 = false;
	if((wCh & 0x2000) != 0) m_checkCh14 = true; else m_checkCh14 = false;
	if((wCh & 0x4000) != 0) m_checkCh15 = true; else m_checkCh15 = false;
	if((wCh & 0x8000) != 0) m_checkCh16 = true; else m_checkCh16 = false;

	UpdateData(FALSE);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
