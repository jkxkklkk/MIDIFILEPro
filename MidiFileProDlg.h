// MidiFileProDlg.h : header file
//

#if !defined(AFX_MIDIFILEPRODLG_H__B487B816_3312_46D3_806B_59DBE65642D3__INCLUDED_)
#define AFX_MIDIFILEPRODLG_H__B487B816_3312_46D3_806B_59DBE65642D3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "PianoCtrl.h"
#include "MidiFile.h"
#include "FileDialogEx.h"
#include "MidiDevice.h"
#include "PlayMuteDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CMidiFileProDlg dialog

class CMidiFileProDlg : public CDialog
{
// Construction
public:
	CMidiFileProDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CMidiFileProDlg)
	enum { IDD = IDD_MIDIFILEPRO_DIALOG };
	CComboBox	m_comMidiOut;
	CSliderCtrl	m_sldPan;
	CSliderCtrl	m_sldTempo;
	CSliderCtrl	m_sldPos;
	CListCtrl	m_EventList;
	CSliderCtrl	m_sldVolume;
	BOOL	m_checkShowKeys;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMidiFileProDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;
	CFont m_font;
	CPianoCtrl m_ctlPiano;
	CMidiDevice m_MidiDevice;
	CMidiFile m_MidiFile;

	CString m_strFileName;
	CString m_strFilePath;
	int nProgressPos;

	// Generated message map functions
	//{{AFX_MSG(CMidiFileProDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnButnOpen();
	afx_msg void OnButnPlay();
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnButnPause();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnCheckShowKeys();
	afx_msg void OnButnRemove();
	afx_msg void OnButnMute();
	afx_msg void OnButnPrint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	LRESULT OnPressKeyboard(WPARAM wParam, LPARAM lParam);
private:
	void ResetPianoCtrl();
	void DisplayEvents();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MIDIFILEPRODLG_H__B487B816_3312_46D3_806B_59DBE65642D3__INCLUDED_)
