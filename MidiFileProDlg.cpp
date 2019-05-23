// MidiFileProDlg.cpp : implementation file
//

#include "stdafx.h"
#include "MidiFilePro.h"
#include "MidiFileProDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMidiFileProDlg dialog

CMidiFileProDlg::CMidiFileProDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMidiFileProDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMidiFileProDlg)
	m_checkShowKeys = FALSE;
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_font.CreateFont(15, 5, 0, 0, 0, FALSE, FALSE, 0, 0, 0, 0, 0, 0, _T("Arial"));
}

void CMidiFileProDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMidiFileProDlg)
	DDX_Control(pDX, IDC_COMBO_MIDI_OUT, m_comMidiOut);
	DDX_Control(pDX, IDC_SLD_PAN, m_sldPan);
	DDX_Control(pDX, IDC_SLD_TEMPO, m_sldTempo);
	DDX_Control(pDX, IDC_SLD_POS, m_sldPos);
	DDX_Control(pDX, IDC_MIDI_EVENT_LIST, m_EventList);
	DDX_Control(pDX, IDC_SLD_VOLUME, m_sldVolume);
	DDX_Control(pDX, IDC_CUSTOM_PIANO, m_ctlPiano);
	DDX_Check(pDX, IDC_CHECK_SHOW_KEYS, m_checkShowKeys);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CMidiFileProDlg, CDialog)
	//{{AFX_MSG_MAP(CMidiFileProDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTN_OPEN, OnButnOpen)
	ON_BN_CLICKED(IDC_BUTN_PLAY, OnButnPlay)
	ON_WM_VSCROLL()
	ON_BN_CLICKED(IDC_BUTN_PAUSE, OnButnPause)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_CHECK_SHOW_KEYS, OnCheckShowKeys)
	ON_BN_CLICKED(IDC_BUTN_REMOVE, OnButnRemove)
	ON_BN_CLICKED(IDC_BUTN_MUTE, OnButnMute)
	ON_BN_CLICKED(IDC_BUTN_PRINT, OnButnPrint)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_PRESS_KEYBOARD,OnPressKeyboard)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMidiFileProDlg message handlers

BOOL CMidiFileProDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here

	// init piano control
	m_ctlPiano.Initialize(36, 95);
	m_ctlPiano.SetNoteOnColor(RGB(31, 213, 61));

	// init slider controls
	m_sldVolume.SetRange(0, 127);
	m_sldPan.SetRange(0, 127);
	m_sldTempo.SetRange(0, 7);

	m_sldVolume.SetPos(127 - 100);
	m_sldPan.SetPos(127 - 64);
	m_sldTempo.SetPos(7 - 3);

	// init list control
	m_EventList.InsertColumn(0, _T("轨道"), LVCFMT_LEFT, 50);
	m_EventList.InsertColumn(1, _T("Delta时间"), LVCFMT_LEFT, 75);
	m_EventList.InsertColumn(2, _T("时长"), LVCFMT_LEFT, 75);
	m_EventList.InsertColumn(3, _T("小节:拍:TICKS"), LVCFMT_LEFT, 100);
	m_EventList.InsertColumn(4, _T("通道"), LVCFMT_LEFT, 50);
	m_EventList.InsertColumn(5, _T("类型"), LVCFMT_LEFT, 50);
	m_EventList.InsertColumn(6, _T("数据"), LVCFMT_LEFT, 65);

	// enum all midi device in and midi device out
	m_MidiDevice.EnumMidiDev();

	// get the name of all midi device out and insert combobox
	for(UINT u = 0; u < m_MidiDevice.GetNumOfMidiDevOut(); u++)
	{
		m_comMidiOut.AddString(m_MidiDevice.m_midi_dev_out[u].szPnameOut);	
	}
	m_comMidiOut.SetCurSel(0);

	m_checkShowKeys = TRUE;

	UpdateData(FALSE);
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CMidiFileProDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CMidiFileProDlg::OnPaint() 
{
	CPaintDC dc(this); // device context for painting

	if (IsIconic())
	{
		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CMidiFileProDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CMidiFileProDlg::OnButnOpen() 
{
	CFileDialogEx dlg(TRUE, NULL, NULL, OFN_HIDEREADONLY 
		| OFN_OVERWRITEPROMPT, _T("*.mid|*.mid||"));

	if(m_MidiFile.GetPlayState() == PLAY_STATE_PLAY)
	{
		return;
	}

	if(dlg.DoModal() == IDOK)
	{
		m_strFileName = dlg.GetFileName();
		m_strFilePath = dlg.GetPathName();
		GetDlgItem(IDC_EDIT_NAME)->SetWindowText(m_strFileName);
		
		bool bResult = m_MidiFile.Open(m_strFilePath, this);

		if(bResult)
		{
			// set the range of pos sloder
			nProgressPos = 0;			
			m_sldPos.SetRange(0, m_MidiFile.GetMaxAbsMsec());

			CString strDivision;
			strDivision.Format(_T("1/4音符: %d ticks"), m_MidiFile.GetTimeDivision());
			GetDlgItem(IDC_EDIT_DIVISION)->SetWindowText(strDivision);

			// display events in list
			DisplayEvents();

			BPM *pBPM = NULL;
			TIME_SIGNATURE *pSignature = NULL;

			// get the start BPM
			POSITION pos = m_MidiFile.m_lstBPM.GetHeadPosition();

			while(pos)
			{
				CString strTemp;
				pBPM = (BPM*)m_MidiFile.m_lstBPM.GetNext(pos);

				strTemp.Format(_T("BPM: %d"), pBPM->uBPM);
				GetDlgItem(IDC_EDIT_BPM)->SetWindowText(strTemp);
				break;
			}

			// get the start signature
			pos = m_MidiFile.m_lstSignature.GetHeadPosition();

			while(pos)
			{
				CString strTemp;
				pSignature = (TIME_SIGNATURE*)m_MidiFile.m_lstSignature.GetNext(pos);

				UINT uNumerator = pSignature->uNumerator;
				UINT uDenominator = pSignature->uDenominator;

				strTemp.Format(_T("拍号: %d/%d"),uNumerator,uDenominator);
				GetDlgItem(IDC_EDIT_SIGNATURE)->SetWindowText(strTemp);
				break;
			}
		}
	}
}

void CMidiFileProDlg::OnButnPlay() 
{
	// open midi device out
	int nSel = m_comMidiOut.GetCurSel();
	m_MidiDevice.OpenDevOut(m_MidiDevice.m_midi_dev_out[nSel].uMidiDev);
	
	// set the handle of midi device out 
	if(m_MidiDevice.m_hMidiOut != NULL)
	{
		m_MidiFile.SetMidiOutHandle(m_MidiDevice.m_hMidiOut);
	}
	else
	{
		return;
	}

	if(!m_MidiFile.IsOpen())
	{
		return;
	}
	
	if(m_MidiFile.GetPlayState() == PLAY_STATE_STOP)
	{
		m_sldTempo.SetPos(3);
		m_sldPos.SetRange(0, m_MidiFile.GetMaxAbsMsec());
		m_MidiFile.Play();	
		SetTimer(1, 5, NULL);
		GetDlgItem(IDC_BUTN_PLAY)->SetWindowText(_T("停止"));
		return;
	}

	if(m_MidiFile.GetPlayState() == PLAY_STATE_PLAY)
	{
		KillTimer(1);
		m_MidiFile.Stop();
		m_sldPos.SetPos(0);	
		GetDlgItem(IDC_BUTN_PLAY)->SetWindowText(_T("播放"));
		ResetPianoCtrl();
		return;
	}

	if(m_MidiFile.GetPlayState() == PLAY_STATE_PAUSE)
	{
		m_sldTempo.SetPos(3);
		m_sldPos.SetRange(0, m_MidiFile.GetMaxAbsMsec());
		m_MidiFile.Play();
		SetTimer(1, 5, NULL);
		GetDlgItem(IDC_BUTN_PLAY)->SetWindowText(_T("停止"));
		return;
	}
}

void CMidiFileProDlg::OnButnPause() 
{
	if(!m_MidiFile.IsOpen())
	{
		return;
	}
	
	if(m_MidiFile.GetPlayState() == PLAY_STATE_PLAY)
	{
		KillTimer(1);
		m_MidiFile.Pause();
		GetDlgItem(IDC_BUTN_PLAY)->SetWindowText(_T("播放"));
	}
}

void CMidiFileProDlg::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	if(pScrollBar->GetDlgCtrlID() == IDC_SLD_VOLUME)
	{
		BYTE byVar = m_sldVolume.GetPos();

		for(int i = 0; i < 16; i++)
		{
			m_MidiDevice.SetVolume(i, 127 - byVar);
		}
	}

	if(pScrollBar->GetDlgCtrlID() == IDC_SLD_PAN)
	{
		BYTE byVar = m_sldPan.GetPos();

		for(int i = 0; i < 16; i++)
		{
			m_MidiDevice.SetPan(i, 127 - byVar);
		}
	}

	if(pScrollBar->GetDlgCtrlID() == IDC_SLD_TEMPO)
	{
		BYTE byVar = 7 - m_sldTempo.GetPos();

		if(byVar >= 0 && byVar <= 3)
		{
			double dTemp = (double)(4 - byVar);

			m_MidiFile.SetCurPlayTempo(dTemp);
			m_sldPos.SetRange(0, m_MidiFile.GetMaxAbsMsec() * dTemp);
		}
		
		if(byVar >= 4 && byVar <= 6)
		{
			double dTemp = (double)(double(7 - byVar) / 4);
			
			m_MidiFile.SetCurPlayTempo(dTemp);
			m_sldPos.SetRange(0, m_MidiFile.GetMaxAbsMsec() * dTemp);
		}
	}

	CDialog::OnVScroll(nSBCode, nPos, pScrollBar);
}

void CMidiFileProDlg::DisplayEvents()
{
	int nCount = 0;
	MIDIMSG *pMsg = NULL;
	CString strTrack, strDeltaTime, strTimeLen;
	CString strMBT, strChannel, strType, strData;

	m_EventList.DeleteAllItems();

	for(int nTrack = 0; nTrack < m_MidiFile.GetTrackCount(); nTrack++)
	{
		POSITION pos = m_MidiFile.m_lstEvent.GetHeadPosition();

		strTrack.Format(_T("%d"), nTrack);
		
		while(pos)
		{
			pMsg = (MIDIMSG *)m_MidiFile.m_lstEvent.GetNext(pos);

			if(pMsg->byTrackIndex == nTrack)
			{
				// case of channel event
				if(pMsg->byEventType >= 0X80 && pMsg->byEventType <= 0xef)
				{
					nCount++;

					strMBT = pMsg->strMBT;
					strDeltaTime.Format(_T("%d"), pMsg->dwDeltaTime);
					strTimeLen.Format(_T("%.0f"), pMsg->dTimeLen);
					strChannel.Format(_T("%d"), (pMsg->dwEvent & 0x0f));
					strType.Format(_T("%x"), pMsg->byEventType);

					// Cn xx and Dn xx only two bytes
					// and others are three bytes
					if((pMsg->byEventType & 0xf0) == 0xc0 || (pMsg->byEventType & 0xf0) == 0xd0)
					{
						strData.Format(_T("%.2x"), (pMsg->dwEvent >> 8) & 0xff);
					}
					else
					{
						strData.Format(_T("%.2x %.2x"), (pMsg->dwEvent >> 8) & 0xff, (pMsg->dwEvent >> 16) & 0xff);
					}

					m_EventList.InsertItem(nCount, strTrack);
					m_EventList.SetItemText(nCount - 1, 1, strDeltaTime);
					m_EventList.SetItemText(nCount - 1, 2, strTimeLen);
					m_EventList.SetItemText(nCount - 1, 3, strMBT);
					m_EventList.SetItemText(nCount - 1, 4, strChannel);
					m_EventList.SetItemText(nCount - 1, 5, strType);
					m_EventList.SetItemText(nCount - 1, 6, strData);
				}
			}
		}
	}
}

LRESULT CMidiFileProDlg::OnPressKeyboard(WPARAM wParam, LPARAM lParam)
{
	DWORD dwEvent = (DWORD)wParam;

	BYTE byType		= LOBYTE(dwEvent & 0x000000F0);
	BYTE byNote		= HIBYTE(LOWORD(dwEvent & 0x0000FF00));
	BYTE byVelocity = LOBYTE(HIWORD(dwEvent & 0x00FF0000));	

	// case of note off
	if(byType == 0x80 || (byType == 0x90 && byVelocity == 0x00))
	{
		m_ctlPiano.NoteOff(byNote);
	}

	// case of note on
	else if(byType == 0x90 && byVelocity != 0x00 ) 
	{
		m_ctlPiano.NoteOn(byNote);
	}

	return 1;
}

void CMidiFileProDlg::ResetPianoCtrl()
{
	BYTE byNote = 0;

	for(byNote = 0; byNote < 128; byNote++)
	{
		m_ctlPiano.NoteOff(byNote);
	}

	for(UINT i = 0; i < 16; i++)
	{
		m_MidiDevice.AllNotesOff(i);
	}
}

void CMidiFileProDlg::OnTimer(UINT nIDEvent) 
{
	if(nIDEvent == 1)
	{
		if(m_MidiFile.GetPlayState() != PLAY_STATE_PLAY)
		{
			KillTimer(1);
			m_sldPos.SetPos(0);
			ResetPianoCtrl();
			GetDlgItem(IDC_BUTN_PLAY)->SetWindowText(_T("&Play"));
		}
		else
		{
			m_sldPos.SetPos(m_MidiFile.GetTimeCounter());
		}
	}
	
	CDialog::OnTimer(nIDEvent);
}

void CMidiFileProDlg::OnCheckShowKeys() 
{
	if(m_checkShowKeys)
	{
		m_checkShowKeys = FALSE;
		m_MidiFile.SetSendEventMsg(false);
		ResetPianoCtrl();
	}
	else
	{
		m_checkShowKeys = TRUE;
		m_MidiFile.SetSendEventMsg(true);
	}
}

void CMidiFileProDlg::OnButnRemove() 
{
	CFileDialogEx dlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("*.mid|*.mid||"));

	if(dlg.DoModal() == IDOK)
	{
		m_MidiFile.RemoveRedundantMidi(dlg.GetPathName());
	}
}

void CMidiFileProDlg::OnButnMute() 
{
	if(!m_MidiFile.IsOpen())
	{
		return;
	}

	CPlayMuteDlg dlg;
	WORD wCh = 0;
	dlg.wCh = m_MidiFile.GetPlayChannel();

	if(dlg.DoModal() == IDOK)
	{
		if(dlg.m_checkCh1) wCh |= 0x0001;
		if(dlg.m_checkCh2) wCh |= 0x0002;
		if(dlg.m_checkCh3) wCh |= 0x0004;
		if(dlg.m_checkCh4) wCh |= 0x0008;
		if(dlg.m_checkCh5) wCh |= 0x0010;
		if(dlg.m_checkCh6) wCh |= 0x0020;
		if(dlg.m_checkCh7) wCh |= 0x0040;
		if(dlg.m_checkCh8) wCh |= 0x0080;
		if(dlg.m_checkCh9) wCh |= 0x0100;
		if(dlg.m_checkCh10) wCh |= 0x0200;
		if(dlg.m_checkCh11) wCh |= 0x0400;
		if(dlg.m_checkCh12) wCh |= 0x0800;
		if(dlg.m_checkCh13) wCh |= 0x1000;
		if(dlg.m_checkCh14) wCh |= 0x2000;
		if(dlg.m_checkCh15) wCh |= 0x4000;
		if(dlg.m_checkCh16) wCh |= 0x8000;

		m_MidiFile.MuteChannels(wCh);
	}
}

void CMidiFileProDlg::OnButnPrint() 
{
	m_MidiFile.PrintMidiEvent(m_strFilePath);
}
