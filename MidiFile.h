// MidiFile.h: interface for the CMidiFile class.
//
//////////////////////////////////////////////////////////////////////
#pragma once

#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

#define WORDSWAP(w)		(((w) >> 8) | \
						(((w) << 8) & 0xFF00))

#define DWORDSWAP(dw)	(((dw) >> 24) | \
						(((dw) >> 8) & 0x0000FF00) | \
						(((dw) << 8) & 0x00FF0000) | \
						(((dw) << 24) & 0xFF000000))

#define MAKEBYTE(l,h)	((l) | ((h) << 4))

struct MIDIMSG
{
	BYTE byTrackIndex; // which track this event in
	BYTE byEventType;  // the event type: < 80 is meta event, 
					   // [80, F0) is channel event,
					   // > F0 is sysex event
	DWORD dwEvent;	   // event type
	DWORD dwDeltaTime; // delta time(ticks)
	double dAbsTicks;  // absolute time of ticks
	double dAbsMsec;   // absolute time of millisecond(ms)
	double dTimeLen;   // the time of midi goes on(ticks)
	CString	strData;   // event string data if applicable
	CString strMBT;    // absolute time(mesure:Beat:Ticks)
	bool bMark;        // the mark used to find the correct note off

	MIDIMSG operator = (const MIDIMSG &other)
	{
		memcpy(this, &other, sizeof(MIDIMSG));
		return *this;
	}
};

struct TRACK
{
	DWORD dwTrackFlag;    // the track's flags: the track is empty or not
	DWORD dwTrackLength;  // the length of the track
	LPBYTE pTrackStart;	  // pointer that point to start of track data buffer
	LPBYTE pTrackCurrent; // pointer that point to next byte to read in buffer
	DWORD dwAbsTimeOfNxtEvent; // absolute time of next event in track
	BYTE byRunningStatus;      // Running status from last channel msg

	TRACK()
		: dwTrackFlag(0)
		, dwTrackLength(0)
		, pTrackStart(0)
		, pTrackCurrent(0)
		, dwAbsTimeOfNxtEvent(0)
		, byRunningStatus(0)
	{
	}
};

struct MIDIFILEHDR
{
    WORD wFormat;	    // Format(hi-lo)
    WORD wTrackCount;	// tracks(hi-lo)
    WORD wTimeDivision; // time division(hi-lo)
};

struct TIME_SIGNATURE
{
	UINT uDenominator; // denominator of signature
	UINT uNumerator;   // numerator of signature
	double dwAbsTicks; // absolute time(ticks) of the signature changed 
};

struct BPM
{
	UINT uBPM;         // beats per minute(here quarter note)
	double dwAbsTicks; // absolute time(ticks) of the BPM changed
};


#define ITS_F_ENDOFTRK		(0x00000001)
#define MIDI_SYSEX			((BYTE)0xF0)  // SysEx begin
#define MIDI_SYSEXEND		((BYTE)0xF7)  // SysEx end
#define MIDI_META			((BYTE)0xFF)  // Meta event begin
#define MIDI_META_TEMPO		((BYTE)0x51)  // ff 51 Tempo change
#define MIDI_META_BEATS		((BYTE)0x58)  // ff 58 Beat
#define MIDI_META_EOT		((BYTE)0x2F)  // end-of-track
#define MIDI_CTRLCHANGE		((BYTE)0xB0)  // CTRL
#define MIDI_PRGMCHANGE		((BYTE)0xC0)  // program change
#define MIDI_CHANPRESS		((BYTE)0xD0)  // channel aftertouch
#define MIDICTRL_VOLUME		((BYTE)0x07)  // Bn 07 xx

#define TIMER_PERIOD        (5000)        // the period of timer

#define WM_PRESS_KEYBOARD   (WM_USER + 2008)

#define PLAY_STATE_STOP     (0)
#define PLAY_STATE_PLAY     (1)
#define PLAY_STATE_PAUSE    (2)

class CMidiFile  
{
public:
	CMidiFile();
	virtual ~CMidiFile();

	CPtrList m_lstEvent;		// a list for link the midi events of midi file
	CPtrList m_lstSignature;	// a list for link the info of signature
	CPtrList m_lstBPM;			// a list for link the info of BPM 
	HMIDIOUT m_hMidiOut;		// for play midis
	DWORD m_dwNewTimeDivision;  // the new time division changed by user
	double m_dPlayTempo;        // the tempo when playing
	bool m_bResetDivision;		// if the time division is changed by user

	// open a midi file with the file path
	bool Open(LPVOID pszFileName, DWORD dwSize, CWnd * pParent = 0);
	
	// open a midi file from resource
	bool Open(UINT uResID, CWnd * pParent = 0);
	bool Open(LPCTSTR pszResID, CWnd * pParent = 0);

	void Play();
	void Pause();
	void Stop();

	// mute channels when playing
	void MuteChannels(WORD wCh){ m_wPlayCh = wCh;}

	// get the number of used channels in midi file
	int GetChannelCount(WORD &wUsedCh);

	// get the first BPM
	UINT GetFirstBPM();

	// the numerator of the first time signature
	UINT GetFirstBeat();

	// the denominator of the first time signature
	UINT GetFirstBeatValue();

	// the total count of beats
	int GetBeatsCount();

	// the total count of measure
	int GetMeasureCount();

	// convert the time to delta time format
	void Convert2DeltaTime(CString &strTo, DWORD dwData, bool bAscii);

	// remove all redundant midi (note on)
	bool RemoveRedundantMidi(LPCTSTR lpszFileName);

	// swap two midi event by sort ascending
	void SwapEvent(MIDIMSG* pMsgA, MIDIMSG* pMsgB);

	// save a midi file
	bool SaveMidiFile(LPCTSTR lpszFileName, CMidiFile *pMidiFile);

	// print the midi event to a txt file
	// including meta event,channel event,SysEx event
	// and description informations about the midi file
	// such as track name,track comment,copyright and instrument
	bool PrintMidiEvent(LPCTSTR lpszFileName);

	inline DWORD GetFormat(){ return m_dwFormat; }
	inline DWORD GetTrackCount(){ return m_dwTrackCount; }
	inline int GetTimeDivision(){ return m_dwTimeDivision; }
	inline int GetNewTimeDivision(){ return m_dwNewTimeDivision; }
	inline double GetCurPlayTempo(){ return m_dPlayTempo; }
	inline UINT GetPlayState(){ return m_PlayState; }
	inline double GetMaxAbsMsec(){ return m_dMaxAbsMsec; }
	inline double GetMaxAbsTicks(){ return m_dMaxAbsTicks; }
	inline double GetCurrPlayTime(){ return m_dCurPlayTime; }
	inline double GetTimeCounter(){ return m_dTimeCounter; }
	inline WORD GetPlayChannel(){ return m_wPlayCh; }
	inline CWnd * GetParWnd(){ return m_pWndParent; }
	inline void SetCurPlayTempo(double dPlayTempo){ m_dPlayTempo = dPlayTempo; }
	inline void SetMidiOutHandle(HMIDIOUT hMidiOut){ m_hMidiOut = hMidiOut; }
	inline void SetSendEventMsg(bool bSendEventMsg){ m_bSendEventMsg = bSendEventMsg; }
	inline bool IsOpen(){ return m_bOpen; }
	inline bool IsPlaying(){ return m_bPlaying; }
	inline bool IsPaused(){ return m_bPaused; }

protected:
	WORD  m_wPlayCh;          // 16 bit 16 channel(right-to-left),1 play,0 mute
	DWORD m_dwFormat;	      // file format
	DWORD m_dwTrackCount;	  // track count
	DWORD m_tkCurrentTime;	  // current time of ticks
	DWORD m_dwCurrentTempo;	  // Micro temp(microsecond per quarter note)
	DWORD m_dwResidual;		  // the arithmetical residual
	DWORD m_dwTimeDivision;	  // the origin time division in midi file
	UINT m_BPM;				  // the BPM(beats per minute)
	UINT m_uNumerator;		  // the numerator of signature
	UINT m_uDenominator;	  // the denominator of signature

	MMRESULT m_uTimerId;      // timer ID
	UINT m_PlayState;		  // the state of player
	UINT m_uTimerRes;		  // timer resolution
	double m_dTimeCounter;    // counter for timer(add five every 5ms)
	double m_dCurPlayTime;    // the current time(ms) when playing

	double m_dMaxAbsMsec;     // the time(ms) of the last event
	double m_dMaxAbsTicks;    // the time(millisecond) of the last event
	
	CString m_strInfo;		  // use to save some info,such as comment,copyright 
	CString m_strFilePath;	  // the path of midi file

	bool m_bOpen;             // is opened midi file or not
	bool m_bPlaying;          // is playing now
	bool m_bPaused;           // is paused or not
	bool m_bSendEventMsg;     // send event message to parent window

	CWnd *m_pWndParent;	      // the window pointer for message sending

	// Initialize variables
	void InitVar();

	// convert the time format:
	// calculate the absolute millisecond time
	// and delta time and the length of time(note on) by absolute ticks time
	void ConvertTime();

	// recalculate the delta time or absolute time
	// if time division has changed
	void RecalculateTime(DWORD &dwValue);

	// convert the string to a convenient reading type
	// two char one space
	void ConvertString(CString & strSource);

	// case of meta event of 01 02 03 04
	// pickup the text information
	// including track name,track comment,copyright and instrument
	// you can use to write them to text file
	void AddMetaString(BYTE byEvent, const int nTrack,const CString strEvent);

	// we always pre_read the time from each track so the mixer code can
	// determine which track has the next event with a minimum of work
	// get the track's first start time
	bool GetTrackVDWord(TRACK * ptsTrack, LPDWORD lpdw); 

	// get the track event
	// every one, every type
	bool GetTrackEvent(TRACK * ptsTrack, const int nIndex);

	// obtain the next byte from the track buffer
	bool GetTrackByte(TRACK * ptsTrack, LPBYTE lpbyByte);

	// insert a midi message to event list
	POSITION InsertMsg(const MIDIMSG * pMsg, POSITION pos);

	// remove all elements from event list
	void ReleaseBuffer();
	
	static void CALLBACK OnTimer(UINT uID, UINT uMsg, DWORD dwUser, DWORD dwPar1, DWORD dwPar2);
};
