// MidiFile.cpp: implementation of the CMidiFile class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MidiFile.h"
#include "math.h"

// the marks in midi files
//
#define MThd  0x6468544D
#define MTrk  0x6B72544D

// Macros for swapping hi/lo-endian data
//
#define WORDSWAP(w)		(((w) >> 8) | \
						(((w) << 8) & 0xFF00))

#define DWORDSWAP(dw)	(((dw) >> 24) | \
						(((dw) >> 8) & 0x0000FF00) | \
						(((dw) << 8) & 0x00FF0000) | \
						(((dw) << 24) & 0xFF000000))

#define MAKEBYTE(l,h)	((l) | ((h) << 4))


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMidiFile::CMidiFile()
{
	InitVar();
	m_dwNewTimeDivision	= 0;
	m_bResetDivision    = false;
}

CMidiFile::~CMidiFile()
{
	ReleaseBuffer();
}

void CMidiFile::InitVar()
{
	m_dwFormat			= 0;
	m_dwTrackCount		= 0;
	m_dwTimeDivision	= 0;
	m_dwResidual        = 0;
	m_tkCurrentTime		= 0;
	m_dwCurrentTempo	= 0;
	m_uDenominator      = 0;
	m_uNumerator        = 0;
	m_BPM               = 0;
	m_dPlayTempo        = 1;
	m_dTimeCounter      = 0;
	m_dCurPlayTime      = 0;
	m_dMaxAbsMsec       = 0;
	m_dMaxAbsTicks      = 0;
	m_bOpen             = false;
	m_bPlaying          = false;
	m_bPaused           = false;
	m_bSendEventMsg     = true;
	m_wPlayCh           = 0xffff;
	m_PlayState			= PLAY_STATE_STOP;
	m_strFilePath       = _T("");
	m_strInfo           = _T("");
}

bool CMidiFile::Open(UINT uResID, CWnd * pParent /*= 0*/)
{
	HINSTANCE hApp = ::GetModuleHandle(0);
	ASSERT(hApp);

	HRSRC hResInfo = ::FindResource(hApp, MAKEINTRESOURCE(uResID), TEXT("MIDI"));
	if(hResInfo == 0)
	{
		return FALSE;
	}

	HGLOBAL hRes = ::LoadResource(hApp, hResInfo);
	if(hRes == 0)
	{
		return FALSE;
	}

	LPVOID pTheSound = ::LockResource(hRes);
	if(pTheSound == 0)
	{
		return FALSE;
	}

	DWORD dwTheSound = ::SizeofResource(hApp, hResInfo);

	return Open(pTheSound, dwTheSound, pParent);
}

bool CMidiFile::Open(LPVOID pSoundData, DWORD dwSize, CWnd * pParent /*= 0*/)
{
	if(pSoundData == NULL || dwSize == 0) 
	{
		ASSERT(FALSE);
		return FALSE;
	}

	register LPBYTE p = LPBYTE(pSoundData);
	TRACK track;

	// check header of MIDI
	if(*(DWORD*)p != MThd) 
	{
		ASSERT(FALSE);
		return FALSE;
	}
	p += sizeof(DWORD);

	// check header size
	DWORD dwHeaderSize = DWORDSWAP(*(DWORD*)p);
	if( dwHeaderSize != sizeof(MIDIFILEHDR) ) 
	{
		ASSERT(FALSE);
		return FALSE;
	}		
	p += sizeof(DWORD);

	// get header info
	MIDIFILEHDR hdr;
	::CopyMemory(&hdr, p, dwHeaderSize);
	m_dwFormat			= DWORD(WORDSWAP(hdr.wFormat));
	m_dwTrackCount		= DWORD(WORDSWAP(hdr.wTrackCount));
	m_dwTimeDivision	= DWORD(WORDSWAP(hdr.wTimeDivision));
	p += dwHeaderSize;

	// set MIDI information string
	m_strInfo.Format(_T("File format: %d  \nTrack count: %d  \nTime division: %d\n"), 
	m_dwFormat, m_dwTrackCount, m_dwTimeDivision);
	
	// if the division has changed
	if(m_bResetDivision)
	{
		m_strInfo.Format(_T("File format: %d  \nTrack count: %d  \nTime division: %d\n"), 
		m_dwFormat, m_dwTrackCount, m_dwNewTimeDivision);
	}

	for(register DWORD i = 0; i < m_dwTrackCount; ++i) 
	{
		// clear the track variable
		track.dwTrackFlag = 0;
		track.byRunningStatus = 0;

		// check header of track
		if(*(DWORD*)p != MTrk)
		{
			ASSERT(FALSE);
			return FALSE;
		}
		p += sizeof(DWORD);

		// track length,a DWORD value, not include "MTrk"
		track.dwTrackLength = DWORDSWAP(*(DWORD*)p);
		p += sizeof(DWORD);

		// track content begin
		track.pTrackStart = track.pTrackCurrent = p;
		p += track.dwTrackLength;
		
        // handle MIDI files which contain empty track chunks
		if(track.dwTrackLength == 0) 
		{
			continue;
		}

		// we always pre_read the time from each track so the mixer code can
		// determine which track has the next event with a minimum of work
		// get the track's first start time
		if(!GetTrackVDWord(&track, &track.dwAbsTimeOfNxtEvent)) 
		{
			TRACE0("Error in MIDI data\n");
			ASSERT(FALSE);
			return FALSE;
		}

		// if the division changed by user
		// then recalculate the absolute time of ticks
		if(m_bResetDivision)
		{
			track.dwAbsTimeOfNxtEvent = (track.dwAbsTimeOfNxtEvent * m_dwNewTimeDivision + m_dwResidual) / m_dwTimeDivision;

			// the residual accumulated for the next time
			m_dwResidual = (track.dwAbsTimeOfNxtEvent * m_dwNewTimeDivision + m_dwResidual) % m_dwTimeDivision;
		}

		RecalculateTime(track.dwAbsTimeOfNxtEvent);

		// load track event, everyone, every type
		GetTrackEvent(&track, i);
	}
	
	m_pWndParent = pParent;

	// convert the absolute time of ticks to absolute time of millisecond
	// and get the delta time of every event with ticks
	ConvertTime();	

	m_bOpen = true;

	return true;
}

bool CMidiFile::Open(LPCTSTR pszFileName, CWnd * pParent /*= 0*/)
{
	CFile file;

	if (!file.Open(pszFileName, CFile::modeRead | CFile::typeBinary))
	{
		return FALSE;
	}

	int length = file.GetLength();
	PBYTE pContent = NULL;

	if(length != 0) 
	{
		pContent = new BYTE [length/sizeof(BYTE)];
	}

	file.Read(pContent, length);
	file.Close();
	
	bool ret = FALSE;

	// make sure the content is not empty
	if(pContent != NULL)
	{
		// release the buffer and initialize variables
		ReleaseBuffer();
		
		InitVar();

		ret = Open(pContent, length, pParent);
		delete [] pContent;

		// save the midi file path
		m_strFilePath = pszFileName;		
	}

	return ret;
}

// Attempts to parse a variable length DWORD from the given track. A VDWord
// in a MIDI file
// (a) is in lo-hi format 
// (b) has the high bit set on every byte except the last
//
// Returns the DWORD in *lpdw and TRUE on success; else
// FALSE if we hit end of track first.
bool CMidiFile::GetTrackVDWord(TRACK * ptsTrack, LPDWORD lpdw) 
{
	ASSERT(ptsTrack != 0);
	ASSERT(lpdw != 0);

	if(ptsTrack->dwTrackFlag & ITS_F_ENDOFTRK)
	{
		return FALSE;
	}

	BYTE	byByte;
	DWORD	dw = 0;

	do 
	{
		if(!GetTrackByte(ptsTrack, &byByte))
		{
			return FALSE;
		}

		dw = ( dw << 7 ) | ( byByte & 0x7F );
	} while( byByte & 0x80 );

	*lpdw = dw;

	return true;
}

bool CMidiFile::GetTrackByte(TRACK * ptsTrack, LPBYTE lpbyByte)
{
	if(DWORD(ptsTrack->pTrackCurrent - ptsTrack->pTrackStart) == ptsTrack->dwTrackLength)
	{
		return FALSE;
	}
	
	*lpbyByte = *ptsTrack->pTrackCurrent++;

	return true;
}

// Fills in the event struct with the next event from the track
//
// pteTemp->tkEvent will contain the absolute tick time of the event
// pteTemp->byShortData[0] will contain
// MIDI_META if the event is a meta event;
// in this case pteTemp->byShortData[1] will contain the meta class
// MIDI_SYSEX or MIDI_SYSEXEND if the event is a SysEx event
// Otherwise, the event is a channel message and pteTemp->byShortData[1]
// and pteTemp->byShortData[2] will contain the rest of the event.
//
// pteTemp->dwEventLength will contain
// The total length of the channel message in pteTemp->byShortData if
// the event is a channel message
// The total length of the parameter data pointed to by
// pteTemp->pLongData otherwise
//
// pteTemp->pLongData will point at any additional parameters if the 
// event is a SysEx or meta event with non-zero length; else
// it will contain NULL
//
// Returns TRUE on success or FALSE on any kind of parse error
// Prints its own error message ONLY in the debug version
//
// Maintains the state of the input track (i.e. 
// ptsTrack->pTrackPointers, and ptsTrack->byRunningStatus).
//
bool CMidiFile::GetTrackEvent(TRACK * ptsTrack, const int nIndex) 
{
	MIDIMSG * pteTemp = NULL;
	POSITION pos = m_lstEvent.GetHeadPosition();

	DWORD   idx = 0;
	DWORD   dwEventLength = 0;
	UINT    uMeasure = 0;
	BYTE	byData[4];

	try
	{
		while(true)
		{
			// Get the first byte, which determines the type of event.
			BYTE byByte;
			if(!GetTrackByte(ptsTrack, &byByte))
			{
				return FALSE;
			}

			memset(byData, 0, sizeof(byData));

			// If the highest bit is not been set, then this is a channel message
			// which uses the status byte from the last channel message
			// we saw. NOTE: We do not clear running status across SysEx or
			// meta events even though the spec says to.because there are
			// actually files out there which contain that sequence of data.
			// MIDI channel event is range from 8[0-F] ~ F[0-F]
			if(!(byByte & 0x80)) 
			{
				// malloc memory for this event
				pteTemp = new MIDIMSG;

				// No previous status byte? We're hosed.
				if(!ptsTrack->byRunningStatus) 
				{
					return FALSE;
				}

				// set and save the MIDI value
				byData[0] = ptsTrack->byRunningStatus;
				byData[1] = byByte;

				pteTemp->strData.Format(_T("%.2X %.2X "), byData[0], byData[1]);
				byByte = byData[0] & 0xF0;

				// Only program change and channel pressure events are 2 bytes long;
				// the rest are 3 and need another byte
				if((byByte != MIDI_PRGMCHANGE) && 
					(byByte != MIDI_CHANPRESS)) 
				{
					if(!GetTrackByte(ptsTrack, &byData[2]))
					{
						return FALSE;
					}
					
					CString str;
					str.Format(_T("%.2X"), byData[2]);

					pteTemp->strData += str;
				}

				// calculate the event value
				pteTemp->dwEvent = ( byData[0] )
							| (((DWORD)byData[1] ) << 8 )
							| (((DWORD)byData[2] ) << 16 )
							| MEVT_F_SHORT;
				
				// set event type
				pteTemp->byEventType = byData[0];			
			}
			
			else if((byByte & 0xF0 ) != MIDI_SYSEX)
			{
				// malloc memory for this event
				pteTemp = new MIDIMSG;

				// Not running status, not in SysEx range - must be
				// normal channel message (0x80-0xEF)

				// event type
				byData[0] = byByte;

				ptsTrack->byRunningStatus = byByte;

				// Strip off channel and just keep message type
				byByte &= 0xF0;

				dwEventLength = (byByte == MIDI_PRGMCHANGE || byByte == MIDI_CHANPRESS) ? 1 : 2;

				if(!GetTrackByte( ptsTrack, &byData[1]))
				{
					return FALSE;
				}

				pteTemp->strData.Format(_T("%.2X %.2X "), byData[0], byData[1]);

				if(dwEventLength == 2)
				{
					if(!GetTrackByte(ptsTrack, &byData[2]))
					{
						return FALSE;
					}

					CString str;
					str.Format(_T("%.2X"), byData[2]);

					pteTemp->strData += str;
				}
				
				// calculate the event value
				pteTemp->dwEvent = ( byData[0] )
							| (((DWORD)byData[1] ) << 8 )
							| (((DWORD)byData[2] ) << 16 );

				pteTemp->byEventType = byData[0];
			}
			
			else if((byByte == MIDI_SYSEX) || (byByte == MIDI_SYSEXEND)) 
			{
				// malloc memory for this event
				pteTemp = new MIDIMSG;

				// One of the SysEx types. (They are the same as far as we're concerned;
				// there is only a semantic difference in how the data would actually
				// get sent when the file is played. We must take care to put the proper
				// event type back on the output track, however.)
				//
				// Parse the general format of:
				//  BYTE 	bEvent (MIDI_SYSEX or MIDI_SYSEXEND)
				//  VDWORD 	cbParms
				//  BYTE   	abParms[cbParms]

				// event type
				byData[0] = byByte;

				// get data length into "pteTemp->dwEventLength"
				if(!GetTrackVDWord( ptsTrack, &dwEventLength)) 
				{
					return FALSE;
				}
		
				BYTE data;
				// Copy from the input buffer to the parameter data buffer
				for(idx = 0; idx < dwEventLength; idx++)
				{				
					if(!GetTrackByte(ptsTrack, &data)) 
					{
						return FALSE;
					}
					
					//CString strTemp = d;
					//pteTemp->strData += _T("aa");/*(TCHAR)data;*/				
				}

				pteTemp->byEventType = byData[0];

				//Ignoring SysEx event
			}
			
			else if(byByte == MIDI_META) 
			{
				// malloc memory for this event
				pteTemp = new MIDIMSG;

				// It's a meta event. Parse the general form:
				//  BYTE	bEvent	(MIDI_META)
				//  BYTE	bClass
				//  VDWORD	cbParms
				//  BYTE	abParms[cbParms]

				// event type
				byData[0] = byByte;

				// META event type
				if( !GetTrackByte(ptsTrack, &byData[1]))
				{
					return FALSE;
				}

				if( byData[1] == MIDI_META_EOT )
				{
					delete pteTemp;
					return true;
				}

				// META event length
				if(!GetTrackVDWord(ptsTrack, &dwEventLength)) 
				{	
					return false;
				}
				
				pteTemp->byEventType = byData[1];
				
				// NOTE: It's perfectly valid to have a meta with no data
				// In this case, dwEventLength == 0 and pLongData == NULL
				if(dwEventLength) 
				{		
					BYTE data;	
					
					for(idx = 0; idx < dwEventLength; idx++)
					{
						if(!GetTrackByte(ptsTrack, &data))
						{
							return FALSE;
						}
	
						pteTemp->strData += (TCHAR)data;
					}
				}

				if(byData[1] == MIDI_META_TEMPO) 
				{
					// Note: this is backwards from above because we're converting a single
					//		 data value from hi-lo to lo-hi format...
					// note that, you must do shield the useless byte when do a force change
					pteTemp->dwEvent =  ((DWORD)pteTemp->strData[2]   & 0x000000FF)
							| ((((DWORD)pteTemp->strData[1] ) << 8 )  & 0x0000FF00)
							| ((((DWORD)pteTemp->strData[0] ) << 16 ) & 0x00FF0000);
												
					// This next step has absolutely nothing to do with the conversion of a
					// MIDI file to a stream, it's simply put here to add the functionality
					// of the tempo slider. If you don't need this, be sure to remove it
					m_dwCurrentTempo = pteTemp->dwEvent;

					// If the MicroTempo changed then recalculate the BPM and save it to list
					// m_uDenominator == 0 indicate that 
					// the ff 51 xx xx xx appeared before ff 58 04 xx xx xx xx
					if(m_uDenominator != 0)
					{
						float fA = 1000000 * 60;
						float fB = m_dwCurrentTempo;
						fA = fA / fB + 0.5;
						m_BPM = int(fA);

						POSITION pos = m_lstBPM.GetTailPosition();
						BPM *pBPMInsert = new BPM;
						pBPMInsert->dwAbsTicks = ptsTrack->dwAbsTimeOfNxtEvent;
						pBPMInsert->uBPM = m_BPM;
						m_lstBPM.InsertAfter(pos, pBPMInsert);
					}
				}		
				
				if(byData[1] == MIDI_META_BEATS)
				{
					// Time signature is expressed as 4 numbers. nn and dd represent the 
					// "numerator" and "denominator" of the signature as notated on sheet music
					// Note: The denominator is a negative power of 2: 2 = quarter note, 3 = eighth, etc.
					// Note: just get the first one

					// If the signature changed then recalculate it and save it to list
					m_uDenominator = pow((double)2,(int)pteTemp->strData[1]);
					m_uNumerator = pteTemp->strData[0];

					POSITION pos = m_lstSignature.GetTailPosition();
					TIME_SIGNATURE* pSigInsert = new TIME_SIGNATURE;
					pSigInsert->dwAbsTicks = ptsTrack->dwAbsTimeOfNxtEvent;
					pSigInsert->uDenominator = m_uDenominator;
					pSigInsert->uNumerator = m_uNumerator;
					m_lstSignature.InsertAfter(pos, pSigInsert);

					// m_dwCurrentTempo == 0 indicate that 
					// the ff 58 04 xx xx xx xx appeared before ff 51 xx xx xx
					if(m_dwCurrentTempo != 0)
					{
						float fA = 1000000 * 60;
						float fB = m_dwCurrentTempo;
						fA = fA / fB + 0.5;
						m_BPM = int(fA);

						POSITION pos = m_lstBPM.GetTailPosition();
						BPM *pBPMInsert = new BPM;
						pBPMInsert->dwAbsTicks = ptsTrack->dwAbsTimeOfNxtEvent;
						pBPMInsert->uBPM = m_BPM;
						m_lstBPM.InsertAfter(pos, pBPMInsert);
					}
				}

				// add meta information to "m_strInfo", such as,
				// track comment, copyright, instrument
				else
				{
					AddMetaString(byData[1], nIndex, pteTemp->strData);
				}
			}

			else 
			{
				// Messages in this range are system messages and aren't supposed to
				// be in a normal MIDI file. If they are, we've either misparsed or the
				// authoring software is stupid.
				return FALSE;
			}

			// Event time was already stored as the current track time
			pteTemp->dAbsTicks = ptsTrack->dwAbsTimeOfNxtEvent;
			pteTemp->byTrackIndex = nIndex;

			// calculate the absolute time(measure:beat:ticks)
			int x = 0, y = 0, b = 0, t = 0;
			CString strMBT = "0", strBeat = "00", strTick = "000";
			
			while(true)
			{
				if(m_uDenominator == 0)
				{
					break;
				}
				
				if(m_bResetDivision)
				{
					x = (m_dwNewTimeDivision * m_uNumerator * 4 / m_uDenominator);
				}
				else
				{
					x = (m_dwTimeDivision * m_uNumerator * 4 / m_uDenominator);
				}

				if(x == 0) break;
				
				if(pteTemp->dAbsTicks >= (uMeasure * x))
				{ 
					uMeasure++;
				}
				else break;
			}

			if(m_bResetDivision)
			{
				if(m_uDenominator != 0)
				{
					y = (pteTemp->dAbsTicks - (uMeasure - 1) * x);
					b = y / (m_dwNewTimeDivision * 4 / m_uDenominator) + 1;
					t = y % (m_dwNewTimeDivision * 4 / m_uDenominator);
				}
			}
			else
			{
				if(m_uDenominator != 0)
				{
					y = (pteTemp->dAbsTicks - (uMeasure - 1) * x);
					b = y / (m_dwTimeDivision * 4 / m_uDenominator) + 1;
					t = y % (m_dwTimeDivision * 4 / m_uDenominator);
				}
			}
			
			strBeat.Format(_T("%d"), b);
			if(strBeat.GetLength() != 2)
			{
				strBeat = "0" + strBeat;
			}

			strTick.Format(_T("%d"), t);
			while(strTick.GetLength() < 3)
			{
				strTick = "0" + strTick;
			}
			strMBT.Format(_T("%d"), uMeasure);
			strMBT += _T(":") + strBeat + _T(":") + strTick;
			pteTemp->strMBT = strMBT;

			uMeasure = 0;

			// channel event
			pteTemp->bMark = false;
			pos = InsertMsg(pteTemp, pos);

			// Now update to the next event time. The code above MUST properly
			// maintain the end of track flag in case the end of track meta is
			// missing.  NOTE: This code is a continuation of the track event
			// time pre_read which is done at the end of track initialization.
			DWORD	tkDelta = 0;
			if(!GetTrackVDWord(ptsTrack, &tkDelta))
			{
				return FALSE;
			}
			
			RecalculateTime(tkDelta);

			ptsTrack->dwAbsTimeOfNxtEvent += tkDelta;
		}
	}

	catch(CMemoryException * e)
	{
		ReleaseBuffer();
		AfxMessageBox(_T("Run out of memory!"));
		e->Delete();
		return false;
	}

	return true;
}

// Insert the midi message into event list
// Sort ascending by absolute ticks time
POSITION CMidiFile::InsertMsg(const MIDIMSG * pMsg, POSITION pos)
{
	if(pMsg == NULL)
	{
		return pos;
	}

	MIDIMSG * pTemp = NULL;
	POSITION ret = NULL;
	POSITION oldPos = NULL;
	bool bAdded = FALSE;
	
	// insert the midi message by sort ascending
	while(pos) 
	{
		oldPos = pos;

		pTemp = (MIDIMSG *)m_lstEvent.GetNext(pos);	
		
		if (pTemp->dAbsTicks > pMsg->dAbsTicks)
		{
			ret = m_lstEvent.InsertBefore(oldPos, (void *)pMsg);
			bAdded = true;
			break;
		}

		else if(pTemp->dAbsTicks == pMsg->dAbsTicks) 
		{
			ret = m_lstEvent.InsertAfter(oldPos, (void *)pMsg);
			bAdded = true;
			break;
		}
	}

	// if this one is the last one, add to tail
	if(pos == NULL && !bAdded)
	{
		ret = m_lstEvent.AddTail((void *)pMsg);
	}

	return ret;
}

// Convert the time format:
// Calculate the absolute millisecond time
// and delta time and the length of time(note on) by absolute ticks time
void CMidiFile::ConvertTime()
{
	MIDIMSG *pMsg = NULL;
	POSITION pos = m_lstEvent.GetHeadPosition();
		
	// the default BPM is 120 ticks per quarter note
	double dMsecPerTick = ((double)(60000000 / 120))/(double)(m_dwTimeDivision * 1000);

	// if the division has changed
	if(m_bResetDivision)
	{
		dMsecPerTick = ((double)(60000000 / 120))/(double)(m_dwNewTimeDivision * 1000);
	}

	for(int nTrack = 0; nTrack < GetTrackCount(); nTrack++)
	{
		double dPreAbsTicks = 0.0;

		pos = m_lstEvent.GetHeadPosition();

		while(pos)
		{
			pMsg = (MIDIMSG *)m_lstEvent.GetNext(pos);

			// when the event is a TEMPO change event, reset the 
			// tick time(mini-second)
			if(pMsg->byEventType == 0x51) 
			{
				// msg->dwEvent is a micro-second value
				dMsecPerTick = ((double)(pMsg->dwEvent)) / 
					(double)(m_dwTimeDivision * 1000);

				// if the division has changed
				if(m_bResetDivision)
				{
					dMsecPerTick = ((double)(pMsg->dwEvent)) / 
						(double)(m_dwNewTimeDivision * 1000);
				}
			}

			if(pMsg->byTrackIndex == nTrack)
			{
				// get the delta time of ticks
				pMsg->dwDeltaTime = pMsg->dAbsTicks - dPreAbsTicks;
				
				dPreAbsTicks = pMsg->dAbsTicks;
				
				// get the absolute time of millisecond
				pMsg->dAbsMsec = pMsg->dAbsTicks * dMsecPerTick;

				if(pMsg->dAbsMsec > m_dMaxAbsMsec)
				{
					m_dMaxAbsMsec = pMsg->dAbsMsec;
				}
				if(pMsg->dAbsTicks > m_dMaxAbsTicks)
				{
					m_dMaxAbsTicks = pMsg->dAbsTicks;
				}

				// get the time of midi(note on) goes on
				if((pMsg->byEventType & 0xf0) == 0x90 && 
					(pMsg->dwEvent & 0xff0000) != 0) 
				{
					MIDIMSG *pMsgTemp = NULL;
					POSITION posTemp = pos;

					while(posTemp)
					{
						pMsgTemp = (MIDIMSG *)m_lstEvent.GetNext(posTemp);

						// find the corresponding note off
						if((pMsg->dwEvent & 0xffff) == (pMsgTemp->dwEvent & 0xffff) &&
							(pMsgTemp->dwEvent & 0xff0000) == 0 && 
							!pMsgTemp->bMark)
						{
							pMsg->dTimeLen = pMsgTemp->dAbsTicks - pMsg->dAbsTicks;
							pMsgTemp->bMark = true;
							break;
						}
					} 
				}
				else
				{
					pMsg->dTimeLen = 0x00;
				}
			}
		}
	}
}

// If the time division has changed 
// then must re_calculate the time of ticks
void CMidiFile::RecalculateTime(DWORD &dwValue)
{
	if(m_bResetDivision)
	{
		DWORD value = dwValue;
		dwValue = (value * m_dwNewTimeDivision + m_dwResidual) / m_dwTimeDivision;

		// the arithmetical compliment is left for the next time
		m_dwResidual = (value * m_dwNewTimeDivision + m_dwResidual) % m_dwTimeDivision;
	}
}

// Convert the string to a convenient reading type
// two char one space
void CMidiFile::ConvertString(CString & strSource)
{
	CString strTemp = strSource;
	int nLength = strTemp.GetLength();

	// set the source to empty first
	strSource.Empty();
	
	// change them to hex format
	// such as: value FF change to text "FF"
	BYTE data;
	CString str, str2;

	for(int i = 0; i < nLength; i++)
	{
		data = strTemp[i];
		str.Format(_T("%.2X "), data);
		str2 += str;	
	}	
	
	strSource = str2;
}

// case of meta event of 01 02 03 04
// pickup the text information
// including track name,track comment,copyright and instrument
// you can use to write them to text file
void CMidiFile::AddMetaString(BYTE byEvent, const int nTrack,const CString strEvent)
{
	CString strTemp;

	switch(byEvent)
	{
	case 0x1:
		strTemp.Format(_T("Track %d comment:"), nTrack);
		break;

	case 0x2:
		strTemp.Format(_T("Copyright:"));
		break;

	case 0x3:
		strTemp.Format(_T("Track %d name:"), nTrack);
		break;

	case 0x4:
		strTemp.Format(_T("Instrument:"));
		break;
		
	default:
		break;
	}	

	m_strInfo += strTemp;
	m_strInfo += strEvent;
	m_strInfo += _T("\n");
}

void CMidiFile::ReleaseBuffer()
{
	MIDIMSG * pTemp1 = NULL;
	POSITION pos1 = m_lstEvent.GetHeadPosition();
	
	while(pos1)
	{
		pTemp1 = (MIDIMSG *)m_lstEvent.GetNext(pos1);
		delete pTemp1;
	}

	m_lstEvent.RemoveAll();

	TIME_SIGNATURE * pTemp2 = NULL;
	POSITION pos2 = m_lstSignature.GetHeadPosition();

	while(pos2)
	{
		pTemp2 = (TIME_SIGNATURE *)m_lstSignature.GetNext(pos2);
		delete pTemp2;
	}

	m_lstSignature.RemoveAll();

	BPM * pTemp3 = NULL;
	POSITION pos3 = m_lstBPM.GetHeadPosition();

	while(pos3)
	{
		pTemp3 = (BPM *)m_lstBPM.GetNext(pos3);
		delete pTemp3;
	}

	m_lstBPM.RemoveAll();
}

int CMidiFile::GetChannelCount(WORD &wUsedCh)
{
	int nReturn = 0, nCh = 0;
	WORD wTemp =0;

	wUsedCh = 0;
	
	for(int nTrack = 0; nTrack < m_dwTrackCount; nTrack++)
	{
		MIDIMSG *pMsg = NULL;
		POSITION pos = m_lstEvent.GetHeadPosition();
		
		while(pos)
		{
			pMsg = (MIDIMSG*)m_lstEvent.GetNext(pos);

			if(pMsg->byEventType >= 0x80 && pMsg->byEventType <= 0xef)
			{
				nCh = (pMsg->dwEvent & 0x0f);

				wUsedCh >>= nCh;
				wUsedCh |= 0x01;
				wUsedCh <<= nCh;
				wUsedCh |= wTemp;

				wTemp = wUsedCh;
			}
		}
	}
	
	for(int i = 0; i < 16; i++)
	{
		if(((wUsedCh >> i) & 0x01) != 0)
		{
			nReturn++;
		}
	}
	
	return nReturn;
}

// get the first BPM
UINT CMidiFile::GetFirstBPM()
{
	BPM *pBPM = NULL;
	POSITION pos = m_lstBPM.GetHeadPosition();
	pBPM = (BPM*)m_lstBPM.GetNext(pos);
	return (pBPM->uBPM);
}

// get the numerator of the first time signature
UINT CMidiFile::GetFirstBeat()
{
	TIME_SIGNATURE *pSignature = NULL;
	POSITION pos = m_lstSignature.GetHeadPosition();
	pSignature = (TIME_SIGNATURE*)m_lstSignature.GetNext(pos);
	return (pSignature->uNumerator);
}

// get the denominator of the first time signature
UINT CMidiFile::GetFirstBeatValue()
{
	TIME_SIGNATURE *pSignature = NULL;
	POSITION pos = m_lstSignature.GetHeadPosition();
	pSignature = (TIME_SIGNATURE*)m_lstSignature.GetNext(pos);
	return (pSignature->uDenominator);
}

// the total counts of beat
// consider that the time signature is fixed
int CMidiFile::GetBeatsCount()
{
	int A = m_dMaxAbsTicks;
	int B = m_dwTimeDivision;

	if(m_bResetDivision)
	{
		B = m_dwNewTimeDivision;
	}

	B = B * 4 / m_uDenominator;
	
	return ((A + B - 1) / B);
}

// the total counts of measure
// consider that the time signature is fixed
int CMidiFile::GetMeasureCount()
{
	int A = m_dMaxAbsTicks;
	int B = m_dwTimeDivision;

	if(m_bResetDivision)
	{
		B = m_dwNewTimeDivision;
	}

	B = B * 4 * m_uNumerator / m_uDenominator;

	return ((A + B - 1) / B);
}

// Convert the normal time to delta time format
// used in midi file
// If bAscii is true then strTo is the ASCII format
// you can write them to midi file expediently
// If bAscii is false then strTo is the string format
// you can read or write them to text file expediently
void CMidiFile::Convert2DeltaTime(CString& strTo, DWORD dwData, bool bAscii)
{
	CString strTemp;
	register unsigned long buffer;
	
	buffer = dwData & 0x7F;
	while((dwData >>= 7)) 
	{
		buffer <<= 8; 
		buffer |= ((dwData & 0x7F) | 0x80); 
	} 

	strTo.Empty();
	
	if(bAscii)
	{
		while(true) 
		{ 		
			// if unicode defined
			// TCHAR are 2 bytes
			strTemp = (TCHAR)buffer;
			strTo += strTemp.Left(2);
			
			if(buffer & 0x80) 
			{
				buffer >>= 8;
			}
			else break; 
		}
	}
	else
	{
		while(true)
		{
			strTemp.Format(_T("%.2x "), (BYTE)buffer);
			strTo += strTemp;

			if(buffer & 0x80) 
			{
				buffer >>= 8;
			}
			else break;
		}
	}
}

// Sometimes there are some notes that
// they are the same pith and same channel
// but overlaped together
// so we best remove the redundant midi event from midi file
// There are four instance we need to dispose
// 1st note1: ----------
//     note2: -----
//     in this way,remove the note2
// 2nd note1: ----------
//     note2:    ----
//     in this way,convert the note1 and note2 to
//     note1: ---
//     note2:    ----
// 3rd note1: ----------
//     note2:       ----
//     in this way,convert the note1 and note2 to
//     note1: ------
//     note2:       ----
// 4th note1: ----------
//     note2:       ----------
//     in this way,convert the note1 and note2 to
//     note1: ------
//     note2:       ----
bool CMidiFile::RemoveRedundantMidi(LPCTSTR lpszFileName)
{
	int nTrack = 0;
	CString strPath = lpszFileName;

	if(strPath.IsEmpty())
	{
		return false;
	}

	CMidiFile *pMidiFile = this;

	if(!pMidiFile->Open(lpszFileName))
	{
		return false;
	}

	// the first step
	// remove all note off first
	MIDIMSG *pMsg = NULL;
	POSITION pos = pMidiFile->m_lstEvent.GetHeadPosition();
	POSITION posOld = pos;
	
	while(pos)
	{
		posOld = pos;
		pMsg = (MIDIMSG*)pMidiFile->m_lstEvent.GetNext(pos);
		
		// case of note off
		if((pMsg->byEventType & 0xf0) == 0x90 && 
			(pMsg->dwEvent & 0xff0000) == 0)
		{
			pMidiFile->m_lstEvent.RemoveAt(posOld);
		}
	}

	// the second step
	// search all note on that be unwanted and modify it's real time
	pos = pMidiFile->m_lstEvent.GetHeadPosition();
	while(pos)
	{
		posOld = pos;
		pMsg = (MIDIMSG*)pMidiFile->m_lstEvent.GetNext(pos);
		
		// case of note on
		if((pMsg->byEventType & 0xf0) == 0x90)
		{
			MIDIMSG *pMsgTemp = NULL;
			POSITION posTemp = NULL;
			POSITION posTempOld = NULL;
			
			posTemp = pos;
			
			while(posTemp)
			{
				posTempOld = posTemp;
				pMsgTemp = (MIDIMSG*)pMidiFile->m_lstEvent.GetNext(posTemp);
				
				// case of midi on
				if((pMsgTemp->dwEvent & 0xf0) == 0x90)
				{
					// they are the same pitch and same channel
					if((pMsgTemp->dwEvent & 0xff0f) == (pMsg->dwEvent & 0xff0f))
					{
						// the same start time
						// for example:
						// note1: ------
						// note2: --
						// delete the short one
						if(pMsg->dAbsTicks == pMsgTemp->dAbsTicks)
						{
							if(pMsg->dTimeLen > pMsgTemp->dTimeLen)
							{
								pMidiFile->m_lstEvent.RemoveAt(posTempOld);
							}
							else
							{
								pMidiFile->m_lstEvent.RemoveAt(posOld);
							}
						}
						
						// not the same start time but not beyond the first note
						// for example:
						// note1: ---------- note1: ---------- note1: ----------
						// note2:   ----     note2:       ---- note2:       --------
						// cut off the first one or both of them
						else if((pMsgTemp->dAbsTicks > pMsg->dAbsTicks) &&
							(pMsgTemp->dAbsTicks < (pMsg->dAbsTicks + pMsg->dTimeLen)))
						{
							if((pMsgTemp->dAbsTicks + pMsgTemp->dTimeLen) >
								(pMsg->dAbsTicks + pMsg->dTimeLen))
							{
								pMsgTemp->dTimeLen = pMsg->dAbsTicks + pMsg->dTimeLen - pMsgTemp->dAbsTicks;
							}

							pMsg->dTimeLen = pMsgTemp->dAbsTicks - pMsg->dAbsTicks;
						}
						
						else if(pMsgTemp->dAbsTicks >= (pMsg->dAbsTicks + pMsg->dTimeLen))
						{
							break;
						}
					}
				}
			}
		}
	}

	// the third step
	// insert the removed note off
	pos = pMidiFile->m_lstEvent.GetHeadPosition();
	while(pos)
	{
		pMsg = (MIDIMSG*)pMidiFile->m_lstEvent.GetNext(pos);

		if((pMsg->byEventType & 0xf0) == 0x90 && 
			(pMsg->dwEvent & 0xff0000) != 0)
		{
			MIDIMSG* pInsertMsg = new MIDIMSG;
			pInsertMsg->byEventType = pMsg->byEventType;
			pInsertMsg->byTrackIndex = pMsg->byTrackIndex;
			pInsertMsg->dAbsMsec = 0;
			pInsertMsg->dAbsTicks = pMsg->dAbsTicks + pMsg->dTimeLen;
			pInsertMsg->dTimeLen = 0;
			pInsertMsg->dwDeltaTime = 0;
			pInsertMsg->dwEvent = (pMsg->dwEvent & 0x00ffff);
			pInsertMsg->strData = _T("");
			pInsertMsg->strMBT = _T("");
			
			pMidiFile->m_lstEvent.InsertBefore(pos, pInsertMsg);
		}
	}

	// the fourth step
	// sort ascending by absolute ticks time
	pos = pMidiFile->m_lstEvent.GetHeadPosition();
	while(pos)
	{
		POSITION posTemp = NULL;
		MIDIMSG *pMsgTemp = NULL;

		pMsg = (MIDIMSG*)pMidiFile->m_lstEvent.GetNext(pos);

		posTemp = pos;

		while(posTemp)
		{
			pMsgTemp = (MIDIMSG*)pMidiFile->m_lstEvent.GetNext(posTemp);
			
			if(pMsgTemp->dAbsTicks < pMsg->dAbsTicks)
			{
				// swap two midi event
				SwapEvent(pMsgTemp, pMsg);
			}
		}
	}

	// the last step
	// re_write the midi file
	return SaveMidiFile(lpszFileName, pMidiFile);
}

void CMidiFile::SwapEvent(MIDIMSG* pMsgA, MIDIMSG* pMsgB)
{
	MIDIMSG *pMsgTemp = new MIDIMSG;

	pMsgTemp->byEventType  = pMsgA->byEventType;
	pMsgTemp->byTrackIndex = pMsgA->byTrackIndex;
	pMsgTemp->dAbsMsec	   = pMsgA->dAbsMsec;
	pMsgTemp->dAbsTicks	   = pMsgA->dAbsTicks;
	pMsgTemp->dTimeLen	   = pMsgA->dTimeLen;
	pMsgTemp->dwDeltaTime  = pMsgA->dwDeltaTime;
	pMsgTemp->dwEvent	   = pMsgA->dwEvent;
	pMsgTemp->strData	   = pMsgA->strData;
	pMsgTemp->strMBT	   = pMsgA->strMBT;
	pMsgTemp->bMark        = pMsgA->bMark;
				
	pMsgA->byEventType	   = pMsgB->byEventType;
	pMsgA->byTrackIndex	   = pMsgB->byTrackIndex;
	pMsgA->dAbsMsec	       = pMsgB->dAbsMsec;
	pMsgA->dAbsTicks	   = pMsgB->dAbsTicks;
	pMsgA->dTimeLen		   = pMsgB->dTimeLen;
	pMsgA->dwDeltaTime	   = pMsgB->dwDeltaTime;
	pMsgA->dwEvent		   = pMsgB->dwEvent;
	pMsgA->strData		   = pMsgB->strData;
	pMsgA->strMBT		   = pMsgB->strMBT;
	pMsgA->bMark		   = pMsgB->bMark;
				
	pMsgB->byEventType	   = pMsgTemp->byEventType;
	pMsgB->byTrackIndex	   = pMsgTemp->byTrackIndex;
	pMsgB->dAbsMsec		   = pMsgTemp->dAbsMsec;
	pMsgB->dAbsTicks	   = pMsgTemp->dAbsTicks;
	pMsgB->dTimeLen		   = pMsgTemp->dTimeLen;
	pMsgB->dwDeltaTime	   = pMsgTemp->dwDeltaTime;
	pMsgB->dwEvent		   = pMsgTemp->dwEvent;
	pMsgB->strData		   = pMsgTemp->strData;
	pMsgB->strMBT		   = pMsgTemp->strMBT;
	pMsgB->bMark		   = pMsgTemp->bMark;
				
	delete pMsgTemp;
}

bool CMidiFile::SaveMidiFile(LPCTSTR lpszFileName, CMidiFile *pMidiFile)
{
	int nTrack = 0;
	MIDIMSG *pMsg = NULL;
	POSITION pos = NULL;
	CString strPath = lpszFileName;

	if(strPath.IsEmpty())
	{
		return false;
	}

	if(pMidiFile == NULL)
	{
		return false;
	}

	CFile file;
	file.Open(lpszFileName, CFile::modeCreate | CFile::modeWrite);

	// write header chunk
	DWORD dwMarkMThd = 0x6468544d;
	DWORD dwMarkMtrk = 0x6b72544d;
	DWORD dwLenMThd = 0x06000000;
	DWORD dwLenMtrk = 0x00000000;
	DWORD dwMarkEndTrack = 0x002fff00;
	
	file.Write(&dwMarkMThd, sizeof(DWORD));
	file.Write(&dwLenMThd, sizeof(DWORD));
	
	WORD wFormat = WORDSWAP(pMidiFile->m_dwFormat);
	WORD wTrackNum = WORDSWAP(pMidiFile->m_dwTrackCount);
	WORD wTimeDivision = WORDSWAP(pMidiFile->m_dwTimeDivision);
	file.Write(&wFormat, sizeof(WORD));
	file.Write(&wTrackNum, sizeof(WORD));
	file.Write(&wTimeDivision, sizeof(WORD));
	
	// write midi event from event list
	CString strDeltaTime;
	BYTE byStat = 0x00;
	BYTE byStatOld = 0x00;
	DWORD dwPreRealTime = 0;
	DWORD dwDeltaTime = 0;
	DWORD dwByteCnt = 0;

	for(nTrack = 0; nTrack < pMidiFile->GetTrackCount(); nTrack++)
	{
		file.Write(&dwMarkMtrk, sizeof(DWORD));
		file.Write(&dwLenMtrk, sizeof(DWORD));

		dwByteCnt = 0;
		dwPreRealTime = 0;
		byStatOld = 0;
		pos = pMidiFile->m_lstEvent.GetHeadPosition();
		
		while(pos)
		{
			pMsg = (MIDIMSG*)pMidiFile->m_lstEvent.GetNext(pos);

			if(pMsg->byTrackIndex == nTrack)
			{
				byStat = pMsg->dwEvent & 0xff;

				dwDeltaTime = pMsg->dAbsTicks - dwPreRealTime;
				dwPreRealTime = pMsg->dAbsTicks;

				// case of channel event
				if(pMsg->byEventType >= 0x80 && pMsg->byEventType <= 0xef)
				{
					Convert2DeltaTime(strDeltaTime, dwDeltaTime, true);
					file.Write(strDeltaTime, strDeltaTime.GetLength());

					dwByteCnt += strDeltaTime.GetLength();

					if(byStat != byStatOld)
					{
						if((byStat & 0xf0) == 0xc0 || (byStat & 0xf0) == 0xd0)
						{
							dwByteCnt += 2;
							file.Write(&pMsg->dwEvent, sizeof(BYTE) * 2);
						}
						else
						{
							dwByteCnt += 3;
							file.Write(&pMsg->dwEvent, sizeof(BYTE) * 3);
						}
					}
					else
					{
						DWORD dwTemp = pMsg->dwEvent >> 8;

						if((byStat & 0xf0) == 0xc0 || (byStat & 0xf0) == 0xd0)
						{
							dwByteCnt += 1;
							file.Write(&dwTemp, sizeof(BYTE) * 1);
						}
						else
						{
							dwByteCnt += 2;
							file.Write(&dwTemp, sizeof(BYTE) * 2);
						}
					}
				}

				// case of meta event
				else if(pMsg->byEventType < 0x80)
				{
					Convert2DeltaTime(strDeltaTime, dwDeltaTime, true);
					file.Write(strDeltaTime, strDeltaTime.GetLength());

					dwByteCnt += strDeltaTime.GetLength();

					BYTE byTemp = 0xff;
					file.Write(&byTemp, sizeof(BYTE));
					file.Write(&pMsg->byEventType, sizeof(BYTE));
					byTemp = pMsg->strData.GetLength();
					file.Write(&byTemp, sizeof(BYTE));

					dwByteCnt += 3;
					dwByteCnt += pMsg->strData.GetLength();
					
					for(int i = 0; i < pMsg->strData.GetLength(); i++)
					{
						file.Write(pMsg->strData.Mid(i,1), sizeof(BYTE));
					}
				}

				// case of SysEx event
				else if(pMsg->byEventType > 0xef)
				{
					// ignore SysEx event
				}
				
				byStatOld = byStat;
			}
		}
		
		dwByteCnt += 4;
		file.Write(&dwMarkEndTrack, sizeof(DWORD));

		// re_write the length of this track
		file.Seek(-(dwByteCnt + 4), CFile::end);
		dwByteCnt = DWORDSWAP(dwByteCnt);
		file.Write(&dwByteCnt, sizeof(DWORD));
		file.Seek(0, CFile::end);
	}

	file.Close();
	
	return true;
}

// print the midi event to a txt file
// including meta event,channel event,SysEx event
// and description informations about the midi file
// such as track name,track comment,copyright and instrument
bool CMidiFile::PrintMidiEvent(LPCTSTR lpszFileName)
{
	CStdioFile file;
	MIDIMSG * pMsg = NULL;
	POSITION pos = NULL;

	CString strTemp;
	CString strPath = lpszFileName;
	int nPoint = strPath.ReverseFind('.');

	if(strPath.IsEmpty())
	{
		return false;
	}

	if(m_dwTrackCount == 0)
	{
		return false;
	}

	if(nPoint == -1) 
	{
		strPath = strPath + _T(".txt");
	}
	else
	{
		strPath = m_strFilePath.Left(nPoint) + _T(".txt");
	}

	if(!file.Open(strPath, CFile::modeCreate | CFile::modeWrite)) 
	{
		return false;
	}	

	// write description information about midi file
	// such as track name,track comment,copyright,instrument
	file.WriteString(m_strInfo);

	// print the message one track by one
	for(DWORD nTrack = 0; nTrack < m_dwTrackCount; nTrack++)
	{
		// print the track index
		strTemp.Format(_T("\nTrack %d\n"), nTrack + 1);
		file.WriteString(strTemp);
		
		DWORD nPreAbsTicks = 0;
		pos = m_lstEvent.GetHeadPosition();
		
		while (pos)
		{
			pMsg = (MIDIMSG *)m_lstEvent.GetNext(pos);

			if(pMsg->byTrackIndex == nTrack)
			{
				// case of meta event
				if(pMsg->byEventType < 0X80)
				{
					// when the event is not a text event
					// convert it to a string format
					if(pMsg->byEventType < 0x01 || pMsg->byEventType > 0x09)
					{
						ConvertString(pMsg->strData);
					}
					
					strTemp.Format(_T("Meta event:\t%.0f\t%.2x "), pMsg->dAbsTicks - nPreAbsTicks, pMsg->byEventType);
					strTemp += pMsg->strData;
				}

				// case of SysEx event
				else if(pMsg->byEventType > 0xEF) 
				{
					strTemp.Format(_T("Sysex event:\t%d\t\t%.2X"), pMsg->dAbsTicks - nPreAbsTicks, pMsg->byEventType);
					strTemp += pMsg->strData;
				}

				// case of channel event
				else
				{
					strTemp.Format(_T("Channel event:\t%d\t"), pMsg->dAbsTicks - nPreAbsTicks);
					strTemp += pMsg->strData;
				}

				nPreAbsTicks = pMsg->dAbsTicks;
				strTemp += "\r\n";
				file.WriteString(strTemp);
			}
		}
	}

	file.Close();
	ShellExecute(NULL, _T("open"), _T("notepad.exe"), strPath, _T(""), SW_SHOW);
}

void CMidiFile::Play()
{
	TIMECAPS tc;

	// set up Multimedia timer for 1mS
	if(timeGetDevCaps(&tc, sizeof(TIMECAPS)) != TIMERR_NOERROR)
	{
		return;
	}

	m_uTimerRes = min(max(tc.wPeriodMin, 1), tc.wPeriodMax);

	if(timeBeginPeriod(m_uTimerRes) != TIMERR_NOERROR)
	{
		return;
	}

	timeKillEvent(m_uTimerId);
	timeEndPeriod(m_uTimerRes);
	
    // timer period: TIMER_PERIOD / 1000 = 5ms
	m_uTimerId = timeSetEvent(TIMER_PERIOD / 1000, 0,
							  (LPTIMECALLBACK)&CMidiFile::OnTimer,
							  (DWORD)this, TIME_PERIODIC);

	if(m_uTimerId != 0)
	{
		m_dPlayTempo     = 1;
		m_dCurPlayTime   = 0;
		m_bPlaying       = true;
		m_bPaused        = false;
		m_PlayState	     = PLAY_STATE_PLAY;
	}
}

void CMidiFile::Pause()
{
	if(m_PlayState == PLAY_STATE_PLAY)
	{
		m_bPlaying = false;
		m_bPaused  = true;
		m_PlayState	= PLAY_STATE_PAUSE;
		timeKillEvent(m_uTimerId);
	}
}

void CMidiFile::Stop()
{
	if(m_PlayState != PLAY_STATE_STOP)
	{
		m_bPlaying = false;
		m_bPaused  = false;
		m_dTimeCounter = 0;
		m_PlayState = PLAY_STATE_STOP;
		timeKillEvent(m_uTimerId);
	}
}

// 5ms timer callback
// Go through all tracks and looking for ready events
void CALLBACK CMidiFile::OnTimer(UINT uID, UINT uMsg, DWORD dwUser, DWORD dwPar1, DWORD dwPar2)
{
	CMidiFile *pMidiFile = (CMidiFile*)dwUser;

	pMidiFile->m_dTimeCounter += (TIMER_PERIOD / 1000);

	// the time counter > the time(ms) of the last event
	// means that the midi file has arrived the end
	if(pMidiFile->m_dTimeCounter > pMidiFile->m_dMaxAbsMsec * pMidiFile->m_dPlayTempo)
	{
		pMidiFile->Stop();
	}

	// check the events in all tracks are ready or not
	// the events that appeared the timer period(5ms) will be sent
	for(int nTrack = 0; nTrack < pMidiFile->GetTrackCount(); nTrack++)
	{
		MIDIMSG *pMsg = NULL;
		POSITION pos = pMidiFile->m_lstEvent.GetHeadPosition();
		
		while(pos)
		{
			pMsg = (MIDIMSG*)pMidiFile->m_lstEvent.GetNext(pos);

			pMidiFile->m_dCurPlayTime = pMsg->dAbsMsec * pMidiFile->m_dPlayTempo;

			if(pMsg->byTrackIndex == nTrack)
			{
				if(pMidiFile->m_dCurPlayTime > pMidiFile->m_dTimeCounter)
				{
					break;
				}

				// calculate the delta time between current event and the time counter
				int nDelta = pMidiFile->m_dTimeCounter - pMidiFile->m_dCurPlayTime;

				if(nDelta < (TIMER_PERIOD / 1000))
				{
					if(pMsg->byEventType >= 0x80 && pMsg->byEventType <= 0xef)
					{
						int nChannel = (pMsg->dwEvent & 0x0f);

						// see if the channel be muted or not
						if(((pMidiFile->m_wPlayCh >> nChannel) & 0x01) != 0)
						{
							// send short message to midi device out
							midiOutShortMsg(pMidiFile->m_hMidiOut, pMsg->dwEvent);
							
							// send event message to parent window
							if(pMidiFile->m_bSendEventMsg)
							{
								if(pMidiFile->m_pWndParent != NULL)
								{
									pMidiFile->m_pWndParent->PostMessage(WM_PRESS_KEYBOARD, (WPARAM)pMsg->dwEvent, 0L);
								}
							}
						}
					}
				}
			} // if(pMsg->byTrackIndex == nTrack)
		} // while(pos)
	} // for(int nTrack = 0; nTrack < pMidiFile->GetTrackCount(); nTrack++)
}
