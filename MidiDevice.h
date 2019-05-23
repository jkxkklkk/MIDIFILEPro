// MidiDevice.h: interface for the CMidiDevice class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MIDIDEVICE_H__22EE92EC_3299_4826_B39F_15DC2A92C6AF__INCLUDED_)
#define AFX_MIDIDEVICE_H__22EE92EC_3299_4826_B39F_15DC2A92C6AF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// midi in device
typedef struct
{
	UINT uMidiDev;  // the midi device number
	CString szPnameIn;  // the name of midi device in				
}MIDI_DEVICE_IN;

// midi out device
typedef struct
{
	UINT uMidiDev;  // the midi device number
	CString szPnameOut;  // the name of midi device out			
}MIDI_DEVICE_OUT;

#define MAX_MIDI_DEV_IN    ( 16 )
#define MAX_MIDI_DEV_OUT   ( 16 )
#define WM_USER_MIDI_IN    ( 2009 + 02 )

#include "windows.h"
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

typedef struct
{
	WORD message;
	char data1;
	char data2;
	long time;
}MSGPARAMS;

struct MIDIRECORDEVENT
{
	DWORD dwTime;

	union
	{
		DWORD dwEvent;
		UCHAR bData[4];
	}u;
};

class CMidiDevice  
{
public:
	CMidiDevice();
	virtual ~CMidiDevice();

	CWnd *m_pWnd;
	BOOL m_bNotifyWindow;  // notify(send message to) the window when receiving midi
	HMIDIIN m_hMidiIn;  // the handle of midi dev in
	HMIDIOUT m_hMidiOut;  // the handle of midi dev out
	UINT m_uTotalNumOfDevIn;  // the total number of midi dev in
	UINT m_uTotalNumOfDevOut;  // the total number of midi dev out
	MIDI_DEVICE_IN m_midi_dev_in[MAX_MIDI_DEV_IN];  // use to save the information of midi dev in
	MIDI_DEVICE_OUT m_midi_dev_out[MAX_MIDI_DEV_OUT];  // use to save the information of midi dev out
	CPtrList m_lstEventIn;  //a list for link the events of recording
 
	void EnumMidiDev();  // enumerate all midi device
	INT GetDevInID();  // get the ID for the midi in device
	UINT GetNumOfMidiDevIn();  // get the total number of midi dev in
	BOOL IsMidiInOpen();  // see if midi device in opened or not
	BOOL OpenDevIn(UINT uDeviceInID);  // open the midi device in
	BOOL CloseDevIn();  // close midi device in
	void StopRecording();  // stop to record midi
	void StartRecording();  // start to record midi
	void AddMidiRecordEvent(MIDIRECORDEVENT *pEvent);  // add a record midi event to list
	void ConvertRecordTime();  // convert the recorded time to relative minisecond time
	DWORD ConvertDelta(DWORD dwValue);  // convert the delta time format
	BOOL SaveRecEventToFile(const LPCTSTR strFilePath);  // save the recorded midi in event to a midi file
	void ClearAllRecordEvent();  // clear all recorded midi in event
	BOOL IsRecording();  // see if midi in device is recording or not
	BOOL GetDevInCaps(UINT uDeviceInId, MIDIINCAPS &Caps);  // get the capabilities of midi in device

	INT GetDevOutID();  // get the ID for the midi out device
	UINT GetNumOfMidiDevOut();  // get the total number of midi dev out
	BOOL IsMidiOutOpen();  // see if midi device out opened or not
	BOOL OpenDevOut(UINT uDeviceOutID);  // open the midi device out
	BOOL CloseDevOut();  // close midi device out
	BOOL GetDevOutCaps(UINT uDeviceOutId, MIDIOUTCAPS &Caps);  // get the capabilities of midi out device
	
	BOOL SendShortMsg(DWORD dwMsg);  // send short message
	BOOL SendLongMsg(LPBYTE pSys, DWORD dwMsgLen);  // send long message
	void AllNotesOff(UINT uCh);  // turn all notes off on channel n
	void AllSoundOff(UINT uCh);  // abrupt stop of sound on channel n
	void ProgramChange(UINT uCh, BYTE bVal);  // change the program(patch) on channel n
	void BankSelect(UINT uCh, BYTE bVal);  // bank select on channel n
	void SetVolume(UINT uCh, BYTE bVal);  // set volume on channel n
	void SetPan(UINT uCh, BYTE bVal);  // set pan on channel n
	void SetReverbType(UINT uCh, BYTE bVal);  // reverb program on channel n
	void SetChorusType(UINT uCh, BYTE bVal);  // chorus program on channel n
	void SetReverbLevel(UINT uCh, BYTE bVal);  // reverb send level on channel n
	void SetChorusLevel(UINT uCh, BYTE bVal);  // chorus send level on channel n

private:
	enum StateIn{ CLOSEDIN , OPENEDIN, RECORDING } m_StateIn;
	enum StateOut{ CLOSEDOUT, OPENEDOUT } m_StateOut;
	
};

#endif // !defined(AFX_MIDIDEVICE_H__22EE92EC_3299_4826_B39F_15DC2A92C6AF__INCLUDED_)
