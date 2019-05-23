; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CMidiFileProDlg
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "MidiFilePro.h"

ClassCount=4
Class1=CMidiFileProApp
Class2=CMidiFileProDlg
Class3=CAboutDlg

ResourceCount=4
Resource1=IDD_ABOUTBOX
Resource2=IDR_MAINFRAME
Resource3=IDD_MIDIFILEPRO_DIALOG
Class4=CPlayMuteDlg
Resource4=IDD_PLAY_MUTE_DIALOG

[CLS:CMidiFileProApp]
Type=0
HeaderFile=MidiFilePro.h
ImplementationFile=MidiFilePro.cpp
Filter=N

[CLS:CMidiFileProDlg]
Type=0
HeaderFile=MidiFileProDlg.h
ImplementationFile=MidiFileProDlg.cpp
Filter=D
BaseClass=CDialog
VirtualFilter=dWC
LastObject=IDC_MIDI_EVENT_LIST

[CLS:CAboutDlg]
Type=0
HeaderFile=MidiFileProDlg.h
ImplementationFile=MidiFileProDlg.cpp
Filter=D
LastObject=IDOK

[DLG:IDD_ABOUTBOX]
Type=1
Class=CAboutDlg
ControlCount=4
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC,static,1342308480
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889

[DLG:IDD_MIDIFILEPRO_DIALOG]
Type=1
Class=CMidiFileProDlg
ControlCount=21
Control1=IDC_CUSTOM_PIANO,PianoCtrl,1342242816
Control2=IDC_SLD_POS,msctls_trackbar32,1342242840
Control3=IDC_MIDI_EVENT_LIST,SysListView32,1350631425
Control4=IDC_BUTN_OPEN,button,1342275584
Control5=IDC_BUTN_PLAY,button,1342275584
Control6=IDC_BUTN_PAUSE,button,1342275584
Control7=IDC_COMBO_MIDI_OUT,combobox,1344339970
Control8=IDC_SLD_VOLUME,msctls_trackbar32,1342242842
Control9=IDC_SLD_PAN,msctls_trackbar32,1342242842
Control10=IDC_SLD_TEMPO,msctls_trackbar32,1342242842
Control11=IDC_STATIC_VOLUME,static,1342308352
Control12=IDC_STATIC_PAN,static,1342308352
Control13=IDC_STATIC_TEMPO,static,1342308352
Control14=IDC_CHECK_SHOW_KEYS,button,1342242819
Control15=IDC_EDIT_NAME,edit,1350633601
Control16=IDC_BUTN_REMOVE,button,1342275584
Control17=IDC_BUTN_MUTE,button,1342275584
Control18=IDC_BUTN_PRINT,button,1342275584
Control19=IDC_EDIT_BPM,edit,1350633601
Control20=IDC_EDIT_SIGNATURE,edit,1350633601
Control21=IDC_EDIT_DIVISION,edit,1350633601

[DLG:IDD_PLAY_MUTE_DIALOG]
Type=1
Class=CPlayMuteDlg
ControlCount=19
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_CHECK1,button,1342242819
Control4=IDC_CHECK2,button,1342242819
Control5=IDC_CHECK3,button,1342242819
Control6=IDC_CHECK4,button,1342242819
Control7=IDC_CHECK5,button,1342242819
Control8=IDC_CHECK6,button,1342242819
Control9=IDC_CHECK7,button,1342242819
Control10=IDC_CHECK8,button,1342242819
Control11=IDC_CHECK9,button,1342242819
Control12=IDC_CHECK10,button,1342242819
Control13=IDC_CHECK11,button,1342242819
Control14=IDC_CHECK12,button,1342242819
Control15=IDC_CHECK13,button,1342242819
Control16=IDC_CHECK14,button,1342242819
Control17=IDC_CHECK15,button,1342242819
Control18=IDC_CHECK16,button,1342242819
Control19=IDC_STATIC,button,1342178055

[CLS:CPlayMuteDlg]
Type=0
HeaderFile=PlayMuteDlg.h
ImplementationFile=PlayMuteDlg.cpp
BaseClass=CDialog
Filter=D
LastObject=CPlayMuteDlg
VirtualFilter=dWC

