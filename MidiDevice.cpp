// MidiDevice.cpp: implementation of the CMidiDevice class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MidiDevice.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

void CALLBACK MidiInFunc(HMIDIIN hMidiIn,WORD wMsg,DWORD dwInstance,DWORD dwParam1,DWORD dwParam2)
{
	LPVOID lpMidi;
	HGLOBAL hGMem;
	MSGPARAMS* msgParams;
	CMidiDevice* pMidiDev = (CMidiDevice*)dwInstance;
	
	switch(wMsg) 
	{
		case MM_MIM_LONGDATA:
		case MM_MIM_DATA:
		{
			hGMem = GlobalAlloc(GMEM_SHARE, sizeof(MSGPARAMS));
			if(hGMem != 0)
			{		
				lpMidi             = GlobalLock(hGMem);
				msgParams          = (MSGPARAMS *)lpMidi;
				msgParams->message = (BYTE)(dwParam1);
				msgParams->data1   = (BYTE)(dwParam1 >> 8);
				msgParams->data2   = (BYTE)(dwParam1 >> 16);
				msgParams->time    = dwParam2;

				BYTE type = dwParam1;
				if(type >= 0x80 && type <= 0xeF ) 
				{
					MIDIRECORDEVENT * pEvent = NULL;
					pEvent = new MIDIRECORDEVENT;
					
					if(pEvent != NULL)
					{				
						pEvent->u.dwEvent = dwParam1;
						pEvent->dwTime = dwParam2;
						pMidiDev->AddMidiRecordEvent(pEvent);

						if(pMidiDev->m_pWnd != NULL && pMidiDev->m_bNotifyWindow)
						{
							pMidiDev->m_pWnd->PostMessage(WM_USER_MIDI_IN, 
								(WPARAM)dwParam1, 0L);
						}
					}
				}
				GlobalUnlock(hGMem);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMidiDevice::CMidiDevice()
{
	m_pWnd = NULL;
	m_bNotifyWindow = FALSE;
}

CMidiDevice::~CMidiDevice()
{

}

void CMidiDevice::EnumMidiDev()
{
	UINT u; 
	MIDIINCAPS midiInDevCaps;
	MIDIOUTCAPS midiOutDevCaps;

	m_hMidiIn = NULL;
	m_hMidiOut = NULL;

	// get the number of midi device in and out
	m_uTotalNumOfDevIn = midiInGetNumDevs();
	m_uTotalNumOfDevOut = midiOutGetNumDevs();

	// init the device name and device id
	for(u = 0; u < MAX_MIDI_DEV_IN; u++)
	{
		m_midi_dev_in[u].szPnameIn = "";
		m_midi_dev_in[u].uMidiDev = 0;
	}

	for(u = 0; u < MAX_MIDI_DEV_OUT; u++)
	{
		m_midi_dev_out[u].szPnameOut = "";
		m_midi_dev_out[u].uMidiDev = 0;
	}
	
	// get the device name and device id of midi device in
	for(u = 0; u < m_uTotalNumOfDevIn; u++)
	{
		midiInGetDevCaps(u, &midiInDevCaps, sizeof(MIDIINCAPS));
		m_midi_dev_in[u].szPnameIn = midiInDevCaps.szPname;
		m_midi_dev_in[u].uMidiDev = u;
	}

	// get the device name and device id of midi device out
	for(u = 0; u < m_uTotalNumOfDevOut; u++)
	{
		midiOutGetDevCaps(u, &midiOutDevCaps, sizeof(MIDIOUTCAPS));
		m_midi_dev_out[u].szPnameOut = midiOutDevCaps.szPname;
		m_midi_dev_out[u].uMidiDev = u;
	}
}

UINT CMidiDevice::GetNumOfMidiDevIn()
{
	return m_uTotalNumOfDevIn;
}

UINT CMidiDevice::GetNumOfMidiDevOut()
{
	return m_uTotalNumOfDevOut;
}

INT CMidiDevice::GetDevInID()
{
	UINT uDeviceInID;
    MMRESULT Result = midiInGetID(m_hMidiIn, &uDeviceInID);

	// case of some error just return -1
    if(Result != MMSYSERR_NOERROR)
    {
		return -1;
	}

	return uDeviceInID;
}

INT CMidiDevice::GetDevOutID()
{
	UINT uDeviceOutID;
    MMRESULT Result = midiOutGetID(m_hMidiOut, &uDeviceOutID);

	// case of some error just return -1
    if(Result != MMSYSERR_NOERROR)
    {
		return -1;
	}

	return uDeviceOutID;
}

BOOL CMidiDevice::IsMidiInOpen()
{
	return (m_StateIn == OPENEDIN);
}

BOOL CMidiDevice::IsMidiOutOpen()
{
	return (m_StateOut == OPENEDOUT);
}

BOOL CMidiDevice::OpenDevIn(UINT uDeviceInID)
{
	// makes sure the previous device is closed before 
    // opening another one
    CloseDevIn();

    // open MIDI input device
    MMRESULT Result = midiInOpen(&m_hMidiIn, uDeviceInID, 
                                 (DWORD)MidiInFunc,
                                 (DWORD)this,
                                 CALLBACK_FUNCTION);

    // if we are able to open the device then change state
    if(Result == MMSYSERR_NOERROR)
    {
        m_StateIn = OPENEDIN;
    }
	else
	{
		return FALSE;
	}

	return TRUE;
}

BOOL CMidiDevice::OpenDevOut(UINT uDeviceOutID)
{
	// makes sure the previous device is closed before 
    // opening another one
    CloseDevOut();

    // open MIDI output device
    MMRESULT Result = midiOutOpen(&m_hMidiOut, uDeviceOutID, 
                                  0,
                                  0,
                                  CALLBACK_FUNCTION);

    // if we are able to open the device then change state
    if(Result == MMSYSERR_NOERROR)
    {
        m_StateOut = OPENEDOUT;
	}
	else
	{
		return FALSE;
	}

	return TRUE;
}

BOOL CMidiDevice::CloseDevIn()
{
	// if the device is recording
	// stop recording before closing the device
    if(m_StateIn == RECORDING)
    {
        StopRecording();
    }

    // if the device is opened...
    if(m_StateIn == OPENEDIN)
    {
        // close the device
        MMRESULT Result = midiInClose(m_hMidiIn);

        // if a failure occurred then return false
        if(Result != MMSYSERR_NOERROR)
        {
            return FALSE;
        }

        // change state
        m_StateIn = CLOSEDIN;
    }

	return TRUE;
}

BOOL CMidiDevice::CloseDevOut()
{
	// only close an already opened device
    if(m_StateOut == OPENEDOUT)
    {
        // change state
        m_StateOut = CLOSEDOUT;

        // close the MIDI output device
        MMRESULT Result = midiOutClose(m_hMidiOut);

		// if a failure occurred then return false
		if(Result != MMSYSERR_NOERROR)
        {
            return FALSE;
        }
    }

	return TRUE;
}

void CMidiDevice::StopRecording()
{
	// If the device is in fact recording...
    if(m_StateIn == RECORDING)
    {
        // Change state
        m_StateIn = OPENEDIN;

        // Reset the MIDI input device
        midiInReset(m_hMidiIn);

		// convert recorded time to relative minisecond time
		ConvertRecordTime();
    }
}

void CMidiDevice::StartRecording()
{
	// Only begin recording if the MIDI input device has been opened
    if(m_StateIn == OPENEDIN)
    { 
        // Change state
        m_StateIn = RECORDING;

		// release the recorded buffer
		ClearAllRecordEvent();

        // start recording
        MMRESULT Result = midiInStart(m_hMidiIn);

        // if recording attempt failed...
        if(Result != MMSYSERR_NOERROR)
        {
            // revert back to opened state
            m_StateIn = OPENEDIN;
        }
    }
}

BOOL CMidiDevice::IsRecording()
{
	return (m_StateIn == RECORDING);
}

void CMidiDevice::AddMidiRecordEvent(MIDIRECORDEVENT *pEvent)
{
	if(pEvent != NULL)
	{
		m_lstEventIn.AddTail(pEvent);
	}
}

BOOL CMidiDevice::SaveRecEventToFile(const LPCTSTR strFilePath)
{
	int i = 0, nTrkLen = 0;
	CFile file;

	union 
	{
		char  bData[4];
		DWORD dwData;
	}u;

	// if there is no event in the midi file
	if(m_lstEventIn.IsEmpty())
	{
		return FALSE;
	}

	// if creat or open the midi file failed
	if(!file.Open(strFilePath, CFile::modeWrite | CFile::modeCreate |CFile::typeBinary))
	{
		return FALSE;
	}
	
	BYTE chHead[] =
	{	
		0x4D,0x54,0x68,0x64,   // MThd
		0x00,0x00,0x00,0x06,   // head length
		0x00,0x01,             // file format
		0x00,0x01,             // track count
		0x00,0x78,             // time division
		0x4D, 0x54, 0x72, 0x6B // MTrk
	};
	
	// write head infomation
	file.Write(chHead, sizeof(chHead));

	// write the track length to zero
	// first and modify it at end
	BYTE b = 0;
	for(i = 0; i <= 3; i++)
	{
		file.Write(&b, 1);
	}

	// write tempo infomation, set the tick to 1ms
	BYTE chTempo[] = {0x00, 0xFF, 0x51, 0x03, 0x07, 0xa1, 0x20};
	file.Write(chTempo, sizeof(chTempo));

	// suppose that the max muber of bytes about every 
	// event is 7: 4 bytes delta time and 3 bytes message
	u.dwData = 7 * m_lstEventIn.GetCount();

	UCHAR * pData = NULL;
	pData = new UCHAR [u.dwData];

	if(pData == NULL)
	{
		return FALSE;
	}

	UCHAR * pCur = pData;

	POSITION pos = m_lstEventIn.GetHeadPosition();
	MIDIRECORDEVENT * pMsg = NULL;

	while(pos)
	{
		pMsg = (MIDIRECORDEVENT *)m_lstEventIn.GetNext(pos);
		
		// get delta time(ticks)
		for(i = 3; i >= 0; i--)
		{
			UCHAR uc = (UCHAR)(pMsg->dwTime >> (i * 8));
			if(uc != 0 || i == 0)
			{
				*pCur = uc;
				pCur++;
				nTrkLen++;
			}
		}
		
		int nByt = 3;
		BYTE bEventType = pMsg->u.bData[i];

		// Cn xx  and Dn xx only two bytes
		if(bEventType == 0xc0 || bEventType == 0xd0)
		{
			nByt = 2;	
		}

		// 9n, Bn, En are three bytes 
		for(i = 0; i < nByt; i++)
		{
			*pCur = pMsg->u.bData[i];
			pCur++;
			nTrkLen++;
		}
	}

	u.dwData = nTrkLen;
	
	// write all midi events
	file.Write(pData, u.dwData);
	
	// write the end mark of track
	UCHAR chEnd[] = {0x00, 0xFF, 0x2F, 0x00};
	file.Write(chEnd, sizeof(chEnd));

	// rewrite the trakc length
	file.Seek(18, file.begin);
	
	u.dwData = u.dwData + 7 + 4;
	for(i = 3; i >= 0; i--)
	{
		file.Write(&u.bData[i], 1);
	}

	file.Close();

	return TRUE;
}

void CMidiDevice::ClearAllRecordEvent()
{
	MIDIRECORDEVENT *pTemp = NULL;
	POSITION pos = m_lstEventIn.GetHeadPosition();
	
	while(pos)
	{
		pTemp = (MIDIRECORDEVENT *)m_lstEventIn.GetNext(pos);
		delete pTemp;
	}

	m_lstEventIn.RemoveAll();
}

DWORD CMidiDevice::ConvertDelta(DWORD dwValue)
{
	BYTE	ubTemp[4] = {0x00, 0x80, 0x80, 0x80};
	DWORD   dwReturn = 0;
	int		nCount = 0;
	int		i = 0;

	for(i = 0; i < 4; i++)
	{
		nCount++;	
		ubTemp[i] |= BYTE(dwValue & 0x7F);
		dwValue >>= 7;						
		if (dwValue == 0)
		{
			break;
		}
	}

	for (i = (nCount - 1); i >= 0; i--)
	{
		dwReturn |= ubTemp[i];
		dwReturn <<= 8;
	}
	
	dwReturn >>= 8;

	return dwReturn;
}

void CMidiDevice::ConvertRecordTime()
{
	if(!m_lstEventIn.IsEmpty())
	{
		int nMod = 0;
		POSITION pos = m_lstEventIn.GetHeadPosition();
		MIDIRECORDEVENT * pEvent = NULL;
		pEvent = (MIDIRECORDEVENT *)m_lstEventIn.GetHead();
		DWORD dwStartTime = pEvent->dwTime;
		DWORD dwNewTime = dwStartTime;
		
		while(pos)
		{
			pEvent = (MIDIRECORDEVENT *)m_lstEventIn.GetNext(pos);
			dwNewTime = pEvent->dwTime;

			// get the delta time(ms)
			pEvent->dwTime -= dwStartTime;

			// convert the delta time to ticks format
			// BMP = 6000000 / MicroTempo = 120  ->
			// Micotempo = 500000 us per quarter note ->
			// division = 120 ticks per quarter note ->
			// 1ms = 12/50 ticks
			pEvent->dwTime = (pEvent->dwTime * 12 / 50);

			// accumulate the errors
			nMod += ((pEvent->dwTime * 12) % 50);
			if(nMod >= 50)
			{
				nMod -= 50;
				pEvent->dwTime += 1;	
			}

			// convert the delta time to special format: alterable data length
			pEvent->dwTime = ConvertDelta(pEvent->dwTime);

			dwStartTime = dwNewTime;
		}
	}
}

BOOL CMidiDevice::GetDevInCaps(UINT uDeviceInId, MIDIINCAPS &Caps)
{
	MMRESULT Result = midiInGetDevCaps(uDeviceInId, &Caps, sizeof Caps);

    // if we are not able to retrieve device capabilities
    if(Result != MMSYSERR_NOERROR)
    {
        return FALSE;
    }

	return TRUE;
}

BOOL CMidiDevice::GetDevOutCaps(UINT uDeviceOutId, MIDIOUTCAPS &Caps)
{
	MMRESULT Result = midiOutGetDevCaps(uDeviceOutId, &Caps, sizeof Caps);

    // if we are not able to retrieve device capabilities
    if(Result != MMSYSERR_NOERROR)
    {
        return FALSE;
    }

	return TRUE;
}

BOOL CMidiDevice::SendShortMsg(DWORD dwMsg)
{
	if(m_StateOut == OPENEDOUT)
    {
        MMRESULT Result = ::midiOutShortMsg(m_hMidiOut, dwMsg);

        if(Result != MMSYSERR_NOERROR)
        {
            return FALSE;
        }

		return TRUE;
    }

	return FALSE;
}

BOOL CMidiDevice::SendLongMsg(LPBYTE pSys, DWORD dwMsgLen)
{
	UINT err;
	MIDIHDR hdr;
	
	if(m_StateOut == OPENEDOUT)
	{
		// lock buffer and store pointer in MIDIHDR
		hdr.lpData = (LPSTR)pSys;
		
		if(hdr.lpData)
		{
			// store its size in the MIDIHDR
			hdr.dwBufferLength = dwMsgLen;
			
			// flags must be set to 0
			hdr.dwFlags = 0;
			
			// prepare the buffer and MIDIHDR
			err = midiOutPrepareHeader(m_hMidiOut, &hdr, sizeof(MIDIHDR));
			
			if(!err)
			{
				// prepare the buffer and MIDIHDR
				err = midiOutLongMsg(m_hMidiOut, &hdr, sizeof(MIDIHDR));

				if(err)
				{
					return FALSE;
				}

				// unprepare the buffer and MIDIHDR
				while(MIDIERR_STILLPLAYING == midiOutUnprepareHeader(m_hMidiOut, &hdr, sizeof(MIDIHDR)))
				{
					// should put a delay in here rather than a busy-wait
					Sleep(5);
				}
				return TRUE;
			}
			else
			{
				return FALSE;
			}
		}
	}

	return FALSE;
}

void CMidiDevice::AllNotesOff(UINT uCh)
{
	// check the parameters
	if(uCh < 0 || uCh > 16)
	{
		return;
	}

	// CTRL 123
	DWORD dwEvent = 0;	
	dwEvent = 0xB0 | uCh | ((DWORD)0x7B << 8) | ((DWORD)(0x0) << 16);
	midiOutShortMsg(m_hMidiOut, dwEvent);
}

void CMidiDevice::AllSoundOff(UINT uCh)
{
	// check the parameters
	if(uCh < 0 || uCh > 16)
	{
		return;
	}
	
	// CTRL 120
	DWORD dwEvent = 0;	
	dwEvent = 0xB0 | uCh | ((DWORD)0x78 << 8);
	midiOutShortMsg(m_hMidiOut, dwEvent);
}

void CMidiDevice::ProgramChange(UINT uCh, BYTE bVal)
{
	// check the parameters
	if(uCh < 0 || uCh > 16)
	{
		return;
	}

	// change the patch 
	DWORD dwEvent = 0;	
	dwEvent = 0xC0 | uCh | ((DWORD)bVal << 8);
	midiOutShortMsg(m_hMidiOut, dwEvent);
}

void CMidiDevice::BankSelect(UINT uCh, BYTE bVal)
{
	// check the parameters
	if(uCh < 0 || uCh > 16)
	{
		return;
	}

	// select the bank
	DWORD dwEvent = 0;	
	dwEvent = 0xB0 | uCh | ((DWORD)0 << 8) | ((DWORD)bVal << 16);
	midiOutShortMsg(m_hMidiOut, dwEvent);
}
	
void CMidiDevice::SetVolume(UINT uCh, BYTE bVal)
{
	// check the parameters
	if(uCh < 0 || uCh > 16)
	{
		return;
	}

	// set the master volume
	DWORD dwEvent = 0;	
	dwEvent = 0xB0 | uCh | ((DWORD)0x07 << 8) | ((DWORD)bVal << 16);
	midiOutShortMsg(m_hMidiOut, dwEvent);
}

void CMidiDevice::SetPan(UINT uCh, BYTE bVal)
{
	// check the parameters
	if(uCh < 0 || uCh > 16)
	{
		return;
	}

	// set the master volume
	DWORD dwEvent = 0;	
	dwEvent = 0xB0 | uCh | ((DWORD)0x0a << 8) | ((DWORD)bVal << 16);
	midiOutShortMsg(m_hMidiOut, dwEvent);
}
	
void CMidiDevice::SetReverbType(UINT uCh, BYTE bVal)
{
	// check the parameters
	if(uCh < 0 || uCh > 16)
	{
		return;
	}

	if(bVal < 0 || bVal > 7)
	{
		return;
	}

	// set the reverb program
	DWORD dwEvent = 0;	
	dwEvent = 0xB0 | uCh | ((DWORD)0x50 << 8) | ((DWORD)bVal << 16);
	midiOutShortMsg(m_hMidiOut, dwEvent);
}

void CMidiDevice::SetChorusType(UINT uCh, BYTE bVal)
{
	// check the parameters
	if(uCh < 0 || uCh > 16)
	{
		return;
	}

	if(bVal < 0 || bVal > 7)
	{
		return;
	}

	// set the chorus program
	DWORD dwEvent = 0;	
	dwEvent = 0xB0 | uCh | ((DWORD)0x51 << 8) | ((DWORD)bVal << 16);
	midiOutShortMsg(m_hMidiOut, dwEvent);
}

void CMidiDevice::SetReverbLevel(UINT uCh, BYTE bVal)
{
	// check the parameters
	if(uCh < 0 || uCh > 16)
	{
		return;
	}

	// set the reverb send level
	DWORD dwEvent = 0;	
	dwEvent = 0xB0 | uCh | ((DWORD)0x5b << 8) | ((DWORD)bVal << 16);
	midiOutShortMsg(m_hMidiOut, dwEvent);
}

void CMidiDevice::SetChorusLevel(UINT uCh, BYTE bVal)
{
	// check the parameters
	if(uCh < 0 || uCh > 16)
	{
		return;
	}

	// set the chorus send level
	DWORD dwEvent = 0;	
	dwEvent = 0xB0 | uCh | ((DWORD)0x5d << 8) | ((DWORD)bVal << 16);
	midiOutShortMsg(m_hMidiOut, dwEvent);
}