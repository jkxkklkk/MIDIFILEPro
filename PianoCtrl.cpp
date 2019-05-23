/*

  CPianoCtrl.cpp

  This file represents the implementation for the CPianoCtrl class

  Copyright (C) 2002 Leslie Sanford

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 
  USA

  Contact: Leslie Sanford (jabberdabber@hotmail.com)

  Last modified: 12/18/2002

*/


//---------------------------------------------------------------------
// Dependencies
//---------------------------------------------------------------------

#include "stdafx.h"
#include <afxwin.h>  
#include "PianoCtrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPianoCtrl


//---------------------------------------------------------------------
// Constants
//---------------------------------------------------------------------


// Class name for this control
const TCHAR CPianoCtrl::CLASS_NAME[] = _T("PianoCtrl");

// Default note-on color
const COLORREF CPianoCtrl::DEF_NOTE_ON_COLOR = RGB(0, 150, 250);

// Maximum number of notes
const int CPianoCtrl::MAX_NOTE_COUNT = 128;

// Note table 
const int CPianoCtrl::NOTE_TABLE[] = 
{ 
    C, C_SHARP, D, D_SHARP, E, F, F_SHARP, G, G_SHARP, A, A_SHARP, B,
    C, C_SHARP, D, D_SHARP, E, F, F_SHARP, G, G_SHARP, A, A_SHARP, B,
    C, C_SHARP, D, D_SHARP, E, F, F_SHARP, G, G_SHARP, A, A_SHARP, B,
    C, C_SHARP, D, D_SHARP, E, F, F_SHARP, G, G_SHARP, A, A_SHARP, B,
    C, C_SHARP, D, D_SHARP, E, F, F_SHARP, G, G_SHARP, A, A_SHARP, B,
    C, C_SHARP, D, D_SHARP, E, F, F_SHARP, G, G_SHARP, A, A_SHARP, B,
    C, C_SHARP, D, D_SHARP, E, F, F_SHARP, G, G_SHARP, A, A_SHARP, B,
    C, C_SHARP, D, D_SHARP, E, F, F_SHARP, G, G_SHARP, A, A_SHARP, B,
    C, C_SHARP, D, D_SHARP, E, F, F_SHARP, G, G_SHARP, A, A_SHARP, B,
    C, C_SHARP, D, D_SHARP, E, F, F_SHARP, G, G_SHARP, A, A_SHARP, B,
    C, C_SHARP, D, D_SHARP, E, F, F_SHARP, G
};

// Range error messages
const CString CPianoCtrl::NOTE_RANGE_ERR = _T("Note out of range.");
const CString CPianoCtrl::LOW_NOTE_RANGE_ERR = _T("Low note out of range.");
const CString CPianoCtrl::HIGH_NOTE_RANGE_ERR = _T("High note out of range.");
const CString CPianoCtrl::INVALID_RANGE_ERR = _T("Note range is invalid.");
const CString CPianoCtrl::NATURAL_NOTE_ERR = _T("Notes must be natural.");

// Flags whether or not this custom control class has been registered.
bool CPianoCtrl::REGISTER_FLAG = false;
  

//---------------------------------------------------------------------
// CPianoCtrl class implementation
//---------------------------------------------------------------------


// Constructor
CPianoCtrl::CPianoCtrl() : 
m_Keys(0),
m_LowNote(0),
m_HighNote(0),
m_CurrKey(-1),
m_IsInitialized(false),
m_HasCapture(false)
{
    // If this control has not been registered, do it here.
    if(!REGISTER_FLAG)
    {
        RegisterWindowClass();
    }

    // Initialize critical section
    ::InitializeCriticalSection(&m_CriticalSection);
}


// Destructor
CPianoCtrl::~CPianoCtrl()
{
    DestroyPianoKeys();

    ::DeleteCriticalSection(&m_CriticalSection);
}


// Create function for creating the control dynamically
bool CPianoCtrl::Create(CWnd *pParentWnd, const RECT &rect, UINT nId, 
                        DWORD dwStyle)
{
    return CWnd::Create(CLASS_NAME, _T(""), dwStyle, rect, pParentWnd, nId);
}


// Initializes the control
bool CPianoCtrl::Initialize(unsigned char LowNote, 
                            unsigned char HighNote,
                            COLORREF NoteOnColor)
{
    CRect ClientRect;
    
    // Get the rectangular coordinates of this control and get its 
    // width and length
    GetClientRect(&ClientRect);
    m_Width = ClientRect.bottom;
    m_Length = ClientRect.right - 1;

    // Initialize note-on color
    m_NoteOnColor = NoteOnColor;

    // If we successfully set the note range, we are finished 
    // initializing this control. Indicate that it has been initialized.
    if(SetNoteRange(LowNote, HighNote))
    {
        m_IsInitialized = true;
    }

    return true ? m_IsInitialized : false;
}


// Turn on a note 
void CPianoCtrl::NoteOn(unsigned char NoteId, COLORREF NoteOnColor)
{
    // Make sure this control has been initialized
    if(m_IsInitialized)
    {
        // Range checking
        if(NoteId >= m_LowNote && NoteId <= m_HighNote)
        {
            CRect InvalidRect;

            m_Keys[NoteId - m_LowNote]->NoteOn(NoteOnColor, InvalidRect);
            NotifyNoteOn(NoteId);

            InvalidateRect(InvalidRect);
        }
    }
}


// Turn on note
void CPianoCtrl::NoteOn(unsigned char NoteId)
{
    NoteOn(NoteId, m_NoteOnColor);
}


// Turn off note
void CPianoCtrl::NoteOff(unsigned char NoteId)
{
    // Make sure this control has been initialized
    if(m_IsInitialized)
    {
        // Range checking
        if(NoteId >= m_LowNote && NoteId <= m_HighNote)
        {
            CRect InvalidRect;

            m_Keys[NoteId - m_LowNote]->NoteOff(InvalidRect);
            NotifyNoteOff(NoteId);

            InvalidateRect(InvalidRect);
        }
        // Turn note off and invalidate its area so that it can be 
        // repainted
    }
}


// Attach listener for event notification
void CPianoCtrl::AttachListener(CPianoCtrlListener &Listener)
{
    // Protect list with critical section
    ::EnterCriticalSection(&m_CriticalSection);

    m_Listeners.push_front(&Listener);

    ::LeaveCriticalSection(&m_CriticalSection);
}


// Detach listener
void CPianoCtrl::DetachListener(CPianoCtrlListener &Listener)
{
    ::EnterCriticalSection(&m_CriticalSection);

    m_Listeners.remove(&Listener);

    ::LeaveCriticalSection(&m_CriticalSection);
}


// Set note range
bool CPianoCtrl::SetNoteRange(unsigned char LowNote, 
                              unsigned char HighNote)
{
    bool Result = false;

    // Range checking
    if(LowNote < 0)
    {
        MessageBox(LOW_NOTE_RANGE_ERR, _T("Error"), MB_OK | MB_ICONSTOP);
    }
    // High note beyond maximum number allowed
    else if(HighNote >= MAX_NOTE_COUNT)
    {
        MessageBox(HIGH_NOTE_RANGE_ERR, _T("Error"), MB_OK | MB_ICONSTOP);
    }
    // Low note is higher than high note
    else if(LowNote >= HighNote)
    {
        MessageBox(INVALID_RANGE_ERR, _T("Error"), MB_OK | MB_ICONSTOP);
    }
    // Either the low note or the high note is not natural
    else if(!IsNoteNatural(LowNote) || !IsNoteNatural(HighNote))
    {
        MessageBox(NATURAL_NOTE_ERR, _T("Error"), MB_OK | MB_ICONSTOP);
    }
    // Initialize range and create keys
    else
    {
        m_LowNote = LowNote;
        m_HighNote = HighNote;

        CreatePianoKeys();

        // Indicate that the operation was successful
        Result = true;
    }

    return Result;
}
    

// Determines if a note is natural
bool CPianoCtrl::IsNoteNatural(unsigned char Note)
{
    bool Result = true;

    if(NOTE_TABLE[Note] == C_SHARP || NOTE_TABLE[Note] == D_SHARP ||
        NOTE_TABLE[Note] == F_SHARP || NOTE_TABLE[Note] == G_SHARP ||
        NOTE_TABLE[Note] == A_SHARP)
    {
        Result = false;
    }

    return Result;
}


// Get the number of natural notes in this control's range
int CPianoCtrl::GetNaturalNoteCount()
{
    int NatNoteCount = 0;

    for(unsigned char i = m_LowNote; i <= m_HighNote; i++)
    {
        if(IsNoteNatural(i))
        {
            NatNoteCount++;
        }
    }

    return NatNoteCount;
}


// Calculate the unit length for this control
double CPianoCtrl::GetUnitLength()
{
    // Determine the number of natural notes within this note range
    int NatNoteCount = GetNaturalNoteCount();

    double Result = static_cast<double>(m_Length) / NatNoteCount;

    // Return the length of each unit
    return Result / CPianoKey::UNIT_PER_NAT_KEY;
}


// Create the piano keys
void CPianoCtrl::CreatePianoKeys()
{
    double Position = 0;

    DestroyPianoKeys();

    m_Keys.resize(m_HighNote - m_LowNote + 1);

    m_UnitLength = GetUnitLength();

    //
    // The leftmost key is a special case. Take care of it first.
    //

    if(NOTE_TABLE[m_LowNote] == B || NOTE_TABLE[m_LowNote] == E)
    {
        m_Keys[0] = new CWhiteKeyFull(m_UnitLength, m_Width, 0);

        // Move position to the right
        Position = m_UnitLength * CPianoKey::UNIT_PER_NAT_KEY;
    }
    else
    {
        m_Keys[0] = new CWhiteKeyLeft(m_UnitLength, m_Width, 0);

        // Move position to the right
        Position = m_UnitLength * (CPianoKey::UNIT_PER_NAT_KEY - 1);
    }

    //
    // Create all but the rightmost key. 
    //

    for(int i = m_LowNote + 1; i < m_HighNote; i++)
    {
        // CWhiteKeyLeft cases
        if(NOTE_TABLE[i] == C || NOTE_TABLE[i] == F)
        {          
            m_Keys[i - m_LowNote] = new CWhiteKeyLeft(m_UnitLength, 
                m_Width, Position);

            // Move position to the right
            Position += m_UnitLength * (CPianoKey::UNIT_PER_NAT_KEY - 1);
        }
        // CWhiteKeyMiddle cases
        else if(NOTE_TABLE[i] == G || NOTE_TABLE[i] == A ||
            NOTE_TABLE[i] == D)
        {
            m_Keys[i - m_LowNote] = new CWhiteKeyMiddle(m_UnitLength, 
                m_Width, Position);

            // Move position to the right
            Position += m_UnitLength * (CPianoKey::UNIT_PER_NAT_KEY - 1);
        }
        // CWhiteKeyRight cases
        else if(NOTE_TABLE[i] == B || NOTE_TABLE[i] == E)
        {
            m_Keys[i - m_LowNote] = new CWhiteKeyRight(m_UnitLength, 
                m_Width, Position);

            // Move position to the right
            Position += m_UnitLength * (CPianoKey::UNIT_PER_NAT_KEY);
        }
        // CBlackKey cases
        else
        {
            m_Keys[i - m_LowNote] = new CBlackKey(m_UnitLength, 
                m_Width, Position);

            // Move position to the right
            Position += m_UnitLength;
        }
    }

    //
    // The rightmost key is a special case. Take care of it last.
    //

    if(NOTE_TABLE[m_HighNote] == C || 
        NOTE_TABLE[m_HighNote] == F)
    {
        m_Keys[m_HighNote - m_LowNote] = new 
            CWhiteKeyFull(m_UnitLength, m_Width, Position);
    }
    else
    {
        m_Keys[m_HighNote - m_LowNote] = new 
            CWhiteKeyRight(m_UnitLength, m_Width, Position);
    }

    // Paint new keyboard
    Invalidate();
}


// Destroy the piano keys
void CPianoCtrl::DestroyPianoKeys()
{
    // Make sure there are keys to destroy
    if(m_Keys.size() > 0)
    {
        for(int i = 0; i < m_Keys.size(); i++)
        {
            delete m_Keys[i];
        }
    }

    // Resise the container to 0 indicating that there are no keys in 
    // the control at this time
    m_Keys.resize(0);
}


// Find a key in this control
int CPianoCtrl::FindKey(CPoint &point)
{
    bool FoundFlag = false;
    int Key = 0;

    // This algorithm does a simple linear search for the key that was
    // pressed. This is not an efficient way to search. However, it's
    // the simplest and works well enough in practise for this situation.
    while(!FoundFlag && Key < m_Keys.size() - 1)
    {
        if(m_Keys[Key]->IsPointInKey(point))
        {
            FoundFlag = true;
        }
        else
        {
            Key++;
        }
    }

    return Key;
}


// Notify all listeners that a note-on event has occurred
void CPianoCtrl::NotifyNoteOn(unsigned char NoteId)
{
    // Protect list with critical section
    ::EnterCriticalSection(&m_CriticalSection);

    std::list<CPianoCtrlListener *>::iterator i;

    for(i = m_Listeners.begin(); i != m_Listeners.end(); i++)
    {
        (*i)->OnNoteOn(*this, NoteId);
    }

    ::LeaveCriticalSection(&m_CriticalSection);
}


// Notify listeners that a note-off event has occurred
void CPianoCtrl::NotifyNoteOff(unsigned char NoteId)
{
    // Protect list with critical section
    ::EnterCriticalSection(&m_CriticalSection);

    std::list<CPianoCtrlListener *>::iterator i;

    for(i = m_Listeners.begin(); i != m_Listeners.end(); i++)
    {
        (*i)->OnNoteOff(*this, NoteId);
    }

    ::LeaveCriticalSection(&m_CriticalSection);
}


// Register this controls Window class
void CPianoCtrl::RegisterWindowClass()
{
    WNDCLASS WndClass;
    HINSTANCE hInst = ::AfxGetInstanceHandle();
    HCURSOR Cursor = ::AfxGetApp()->LoadStandardCursor(IDC_ARROW);

    WndClass.style         = CS_HREDRAW | CS_VREDRAW;
    WndClass.lpfnWndProc   = ::DefWindowProc;
    WndClass.cbClsExtra    = WndClass.cbWndExtra = 0;
    WndClass.hInstance     = hInst;
    WndClass.hIcon         = NULL;
    WndClass.hCursor       = Cursor;
    WndClass.hbrBackground = NULL;
    WndClass.lpszMenuName  = NULL;
    WndClass.lpszClassName = CLASS_NAME;

    if (::AfxRegisterClass(&WndClass))
    {
        // Indicate that this Window class has now been registered
        REGISTER_FLAG = true;
    }
    else
    {
        ::AfxThrowResourceException();
    }
}


BEGIN_MESSAGE_MAP(CPianoCtrl, CWnd)
	//{{AFX_MSG_MAP(CPianoCtrl)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CPianoCtrl message handlers


// Paints the control
void CPianoCtrl::OnPaint() 
{
	PAINTSTRUCT ps;
    CDC *dc = BeginPaint(&ps);

    for(int i = 0; i < m_Keys.size(); i++)
    {
        m_Keys[i]->Paint(dc);
    }

    EndPaint(&ps);
}


// On left mouse button down
void CPianoCtrl::OnLButtonDown(UINT nFlags, CPoint point) 
{
    // Capture the mouse if we haven't already
    if(!m_HasCapture)
    {
        SetCapture();
        m_HasCapture = true;
    }

    // Make sure this control has been initialized
    if(m_IsInitialized)
    {
        // Find the key to turn on. This will become the current key. 
        // We keep track of it so that we can turn it off later
        m_CurrKey = FindKey(point);

        // Turn on note, get its area, and then invalidate so that the
        // control is repainted to indicate that this note is being 
        // played
        CRect InvalidRect;
        m_Keys[m_CurrKey]->NoteOn(m_NoteOnColor, InvalidRect);
        NotifyNoteOn(m_CurrKey + m_LowNote);    

        InvalidateRect(&InvalidRect);
    }
	
	CWnd::OnLButtonDown(nFlags, point);
}


// On left mouse button up
void CPianoCtrl::OnLButtonUp(UINT nFlags, CPoint point) 
{
    // If there is a key already being played by the mouse, turn it off
    if(m_CurrKey >= 0)
    {
        CRect InvalidRect;
        m_Keys[m_CurrKey]->NoteOff(InvalidRect);
        NotifyNoteOff(m_CurrKey + m_LowNote);

        // Indicate that there is no key active
        m_CurrKey = -1;

        InvalidateRect(&InvalidRect);
    }
	
	CWnd::OnLButtonUp(nFlags, point);
}


// On mouse move
void CPianoCtrl::OnMouseMove(UINT nFlags, CPoint point) 
{
    // If the mouse has moved outside of this window, turn off any 
    // currently active key and release the mouse.
    if(point.x <= 0 || point.x >= m_Length || point.y <= 0 || 
        point.y >= m_Width)
    {   
        // If there is a key currently active key
        if(m_CurrKey >= 0)
        {
            CRect InvalidRect;

            m_Keys[m_CurrKey]->NoteOff(InvalidRect);
            NotifyNoteOff(m_CurrKey + m_LowNote);
            InvalidateRect(&InvalidRect);
        }

        ReleaseCapture();
        m_HasCapture = false;
    }

    // If this piano control has been intialized, and it has captured the
    // mouse, and the left mouse button is down...
    if(m_IsInitialized && m_HasCapture && (nFlags & MK_LBUTTON))
    {
        // Find the key beneath the mouse cursor
        int Key = FindKey(point); 

        // If the mouse has moved to a new key
        if(Key != m_CurrKey)
        {
            CRect InvalidRect;

            // If the current key is valid, turn note off
            if(m_CurrKey >= 0)
            {               
                m_Keys[m_CurrKey]->NoteOff(InvalidRect);
                NotifyNoteOff(m_CurrKey + m_LowNote);
                InvalidateRect(&InvalidRect);
            }

            // Turn new note on
            m_CurrKey = Key;
            m_Keys[m_CurrKey]->NoteOn(m_NoteOnColor, InvalidRect);
            NotifyNoteOn(m_CurrKey + m_LowNote);

            InvalidateRect(&InvalidRect);
        }
    }
            
	CWnd::OnMouseMove(nFlags, point);
}


//---------------------------------------------------------------------
// Constants
//---------------------------------------------------------------------


// The width of the black keys as a percentage of the width of the 
// white keys
const double CPianoCtrl::CPianoKey::BLACK_KEY_OFFSET = 0.7;

// The number of units natural keys use in their widest part.
const int CPianoCtrl::CPianoKey::UNIT_PER_NAT_KEY = 3;


//---------------------------------------------------------------------
// CWhiteKeyLeft class implementation
//---------------------------------------------------------------------


// Constructor
CPianoCtrl::CWhiteKeyLeft::CWhiteKeyLeft(double UnitLength, int Width, 
                                         double Position)
{
    // Initialize upper portion of this key
    m_UpperRect.top    = 0;
    m_UpperRect.bottom = static_cast<long>(Width * BLACK_KEY_OFFSET);
    m_UpperRect.left   = static_cast<long>(Position);
    m_UpperRect.right  = static_cast<long>
        (Position + UnitLength * (UNIT_PER_NAT_KEY - 1));

    // Initialize lower portion of this key
    m_LowerRect.top    = static_cast<long>(Width * BLACK_KEY_OFFSET);
    m_LowerRect.bottom = Width - 1;
    m_LowerRect.left   = static_cast<long>(Position);
    m_LowerRect.right  = static_cast<long>
        (Position + UnitLength * UNIT_PER_NAT_KEY);
}


// Hit detection
bool CPianoCtrl::CWhiteKeyLeft::IsPointInKey(const CPoint &pt) const
{
    return (IsPointInRect(pt, m_UpperRect) || 
            IsPointInRect(pt, m_LowerRect));
}


// Turns note on
void CPianoCtrl::CWhiteKeyLeft::NoteOn(COLORREF NoteOnColor, 
                                       CRect &Rect)
{
    m_NoteOnFlag = true;
    m_NoteOnColor = NoteOnColor;

    CalcInvalidRect(Rect);
}


// Turns note off
void CPianoCtrl::CWhiteKeyLeft::NoteOff(CRect &Rect)
{
    m_NoteOnFlag = false;

    CalcInvalidRect(Rect);
}


// Paints the key
void CPianoCtrl::CWhiteKeyLeft::Paint(CDC *dc)
{
    //
    // Fill key
    //

    // Note is on
    if(m_NoteOnFlag)
    {
        dc->FillSolidRect(&m_UpperRect, m_NoteOnColor);
        dc->FillSolidRect(&m_LowerRect, m_NoteOnColor);
    }
    // Note is off
    else
    {
        dc->FillSolidRect(&m_UpperRect, RGB(250, 250, 250));
        dc->FillSolidRect(&m_LowerRect, RGB(250, 250, 250));
    }

    dc->SelectStockObject(BLACK_PEN);

    // 
    // Draw border
    //

    // Left side
    dc->MoveTo(m_UpperRect.left, m_UpperRect.top);
    dc->LineTo(m_UpperRect.left, m_LowerRect.bottom);

    // Bottom
    dc->LineTo(m_LowerRect.right, m_LowerRect.bottom);

    // Right side
    dc->LineTo(m_LowerRect.right, m_LowerRect.top);
    dc->LineTo(m_UpperRect.right, m_UpperRect.bottom);
    dc->LineTo(m_UpperRect.right, m_UpperRect.top);

    // Top
    dc->LineTo(m_UpperRect.left, m_UpperRect.top);
}


// Calculate the rectangular area to invalidate for this key
void CPianoCtrl::CWhiteKeyLeft::CalcInvalidRect(CRect &Rect)
{
    Rect.top    = m_UpperRect.top;
    Rect.bottom = m_LowerRect.bottom;
    Rect.left   = m_UpperRect.left;
    Rect.right  = m_LowerRect.right;
}


//---------------------------------------------------------------------
// CWhiteKeyMiddle class implementation
//---------------------------------------------------------------------


// Constructor
CPianoCtrl::CWhiteKeyMiddle::CWhiteKeyMiddle(double UnitLength, 
                                             int Width, double Position)
{
    // Initialize upper portion of this key
    m_UpperRect.top    = 0;
    m_UpperRect.bottom = static_cast<long>(Width * BLACK_KEY_OFFSET);
    m_UpperRect.left   = static_cast<long>(Position + UnitLength);
    m_UpperRect.right  = static_cast<long>
        (Position + UnitLength * (UNIT_PER_NAT_KEY - 1));

    // Initialize lower portion of this key
    m_LowerRect.top    = static_cast<long>(Width * BLACK_KEY_OFFSET);
    m_LowerRect.bottom = Width - 1;
    m_LowerRect.left   = static_cast<long>(Position);
    m_LowerRect.right  = static_cast<long>
        (Position + UnitLength * UNIT_PER_NAT_KEY);
}


// Hit detection
bool CPianoCtrl::CWhiteKeyMiddle::IsPointInKey(const CPoint &pt) const
{
    return (IsPointInRect(pt, m_UpperRect) || 
            IsPointInRect(pt, m_LowerRect));
}


// Turns note on
void CPianoCtrl::CWhiteKeyMiddle::NoteOn(COLORREF NoteOnColor, 
                                         CRect &Rect)
{
    m_NoteOnFlag = true;
    m_NoteOnColor = NoteOnColor;

    CalcInvalidRect(Rect);
}


// Turns note off
void CPianoCtrl::CWhiteKeyMiddle::NoteOff(CRect &Rect)
{
    m_NoteOnFlag = false;

    CalcInvalidRect(Rect);
}


// Paints key
void CPianoCtrl::CWhiteKeyMiddle::Paint(CDC *dc)
{
    //
    // Fill key
    //

    // Note is on
    if(m_NoteOnFlag)
    {
        dc->FillSolidRect(&m_UpperRect, m_NoteOnColor);
        dc->FillSolidRect(&m_LowerRect, m_NoteOnColor);
    }
    // Note is off
    else
    {
        dc->FillSolidRect(&m_UpperRect, RGB(250, 250, 250));
        dc->FillSolidRect(&m_LowerRect, RGB(250, 250, 250));
    }

    dc->SelectStockObject(BLACK_PEN);

    // 
    // Draw border
    //

    // Left side
    dc->MoveTo(m_UpperRect.left, m_UpperRect.top);
    dc->LineTo(m_UpperRect.left, m_UpperRect.bottom);
    dc->LineTo(m_LowerRect.left, m_LowerRect.top);
    dc->LineTo(m_LowerRect.left, m_LowerRect.bottom);

    // Bottom
    dc->LineTo(m_LowerRect.right, m_LowerRect.bottom);

    // Right side
    dc->LineTo(m_LowerRect.right, m_LowerRect.top);
    dc->LineTo(m_UpperRect.right, m_UpperRect.bottom);
    dc->LineTo(m_UpperRect.right, m_UpperRect.top);

    // Top
    dc->LineTo(m_UpperRect.left, m_UpperRect.top);
}


// Calculate rectangular area to invalidate for this key
void CPianoCtrl::CWhiteKeyMiddle::CalcInvalidRect(CRect &Rect)
{
    Rect.top    = m_UpperRect.top;
    Rect.bottom = m_LowerRect.bottom;
    Rect.left   = m_LowerRect.left;
    Rect.right  = m_LowerRect.right;
}


//---------------------------------------------------------------------
// CWhiteKeyRight class implementation
//---------------------------------------------------------------------


// Constructor
CPianoCtrl::CWhiteKeyRight::CWhiteKeyRight(double UnitLength, int Width, 
                                           double Position)
{
    // Initialize upper portion of this key
    m_UpperRect.top    = 0;
    m_UpperRect.bottom = static_cast<long>(Width * BLACK_KEY_OFFSET);
    m_UpperRect.left   = static_cast<long>(Position + UnitLength);
    m_UpperRect.right  = static_cast<long>
        (Position + UnitLength * UNIT_PER_NAT_KEY);

    // Initialize lower portion of this key
    m_LowerRect.top    = static_cast<long>(Width * BLACK_KEY_OFFSET);
    m_LowerRect.bottom = Width - 1;
    m_LowerRect.left   = static_cast<long>(Position);
    m_LowerRect.right  = static_cast<long>
        (Position + UnitLength * UNIT_PER_NAT_KEY);
}


// Hit detection
bool CPianoCtrl::CWhiteKeyRight::IsPointInKey(const CPoint &pt) const
{
    return (IsPointInRect(pt, m_UpperRect) || 
            IsPointInRect(pt, m_LowerRect));
}


// Turns note on
void CPianoCtrl::CWhiteKeyRight::NoteOn(COLORREF NoteOnColor, 
                                        CRect &Rect)
{
    m_NoteOnFlag = true;
    m_NoteOnColor = NoteOnColor;

    CalcInvalidRect(Rect);
}


// Turns note off
void CPianoCtrl::CWhiteKeyRight::NoteOff(CRect &Rect)
{
    m_NoteOnFlag = false;

    CalcInvalidRect(Rect);
}


// Paints key
void CPianoCtrl::CWhiteKeyRight::Paint(CDC *dc)
{
    //
    // Fill key
    //

    // Note is on
    if(m_NoteOnFlag)
    {
        dc->FillSolidRect(&m_UpperRect, m_NoteOnColor);
        dc->FillSolidRect(&m_LowerRect, m_NoteOnColor);
    }
    // Note is off
    else
    {
        dc->FillSolidRect(&m_UpperRect, RGB(250, 250, 250));
        dc->FillSolidRect(&m_LowerRect, RGB(250, 250, 250));
    }

    dc->SelectStockObject(BLACK_PEN);

    // 
    // Draw border
    //

    // Left side
    dc->MoveTo(m_UpperRect.left, m_UpperRect.top);
    dc->LineTo(m_UpperRect.left, m_UpperRect.bottom);
    dc->LineTo(m_LowerRect.left, m_LowerRect.top);
    dc->LineTo(m_LowerRect.left, m_LowerRect.bottom);

    // Bottom
    dc->LineTo(m_LowerRect.right, m_LowerRect.bottom);

    // Right side
    dc->LineTo(m_LowerRect.right, m_UpperRect.top);

    // Top
    dc->LineTo(m_UpperRect.left, m_UpperRect.top);
}


// Calculates rectangular area to invalidate for this key
void CPianoCtrl::CWhiteKeyRight::CalcInvalidRect(CRect &Rect)
{
    Rect.top    = m_UpperRect.top;
    Rect.bottom = m_LowerRect.bottom;
    Rect.left   = m_LowerRect.left;
    Rect.right  = m_LowerRect.right;
}


//---------------------------------------------------------------------
// CWhiteKeyFull class implementation
//---------------------------------------------------------------------


// Constructor
CPianoCtrl::CWhiteKeyFull::CWhiteKeyFull(double UnitLength, int Width, 
                                         double Position)
{
    // Initialize this key's rectangular area
    m_Rect.top    = 0;
    m_Rect.bottom = Width - 1;
    m_Rect.left   = static_cast<long>(Position);
    m_Rect.right  = 
        static_cast<long>(Position + UnitLength * UNIT_PER_NAT_KEY);
}


// Hit detection
bool CPianoCtrl::CWhiteKeyFull::IsPointInKey(const CPoint &pt) const
{
    return (IsPointInRect(pt, m_Rect));
}


// Turns note on
void CPianoCtrl::CWhiteKeyFull::NoteOn(COLORREF NoteOnColor, CRect &Rect)
{
    m_NoteOnFlag = true;
    m_NoteOnColor = NoteOnColor;

    Rect = m_Rect;
}


// Turns note off
void CPianoCtrl::CWhiteKeyFull::NoteOff(CRect &Rect)
{
    m_NoteOnFlag = false;

    Rect = m_Rect;
}


// Paints this key
void CPianoCtrl::CWhiteKeyFull::Paint(CDC *dc)
{
    //
    // Fill key
    //

    // Note is on
    if(m_NoteOnFlag)
    {
        dc->FillSolidRect(&m_Rect, m_NoteOnColor);
    }
    // Note is off
    else
    {
        dc->FillSolidRect(&m_Rect, RGB(250, 250, 250));
    }

    dc->SelectStockObject(BLACK_PEN);

    // 
    // Draw border
    //

    // Left side
    dc->MoveTo(m_Rect.left, m_Rect.top);
    dc->LineTo(m_Rect.left, m_Rect.bottom);

    // Bottom
    dc->LineTo(m_Rect.right, m_Rect.bottom);

    // Right side
    dc->LineTo(m_Rect.right, m_Rect.top);

    // Top
    dc->LineTo(m_Rect.left, m_Rect.top);
}


//---------------------------------------------------------------------
// CBlackKey class implementation
//---------------------------------------------------------------------


// Constructor
CPianoCtrl::CBlackKey::CBlackKey(double UnitLength, int Width, 
                                 double Position)
{
    // Initialize this key's rectangular area
    m_Rect.top    = 0;
    m_Rect.bottom = static_cast<long>(Width * BLACK_KEY_OFFSET);
    m_Rect.left   = static_cast<long>(Position);
    m_Rect.right  = static_cast<long>
        (Position + UnitLength * (UNIT_PER_NAT_KEY - 1));
}


// Hit detection
bool CPianoCtrl::CBlackKey::IsPointInKey(const CPoint &pt) const
{
    return (IsPointInRect(pt, m_Rect));
}


// Turns note on
void CPianoCtrl::CBlackKey::NoteOn(COLORREF NoteOnColor, CRect &Rect)
{
    m_NoteOnFlag = true;
    m_NoteOnColor = NoteOnColor;

    Rect = m_Rect;
}


// Turns note off
void CPianoCtrl::CBlackKey::NoteOff(CRect &Rect)
{
    m_NoteOnFlag = false;

    Rect = m_Rect;
}


// Paints this key
void CPianoCtrl::CBlackKey::Paint(CDC *dc)
{
    //
    // Fill key
    //

    // Note is on
    if(m_NoteOnFlag)
    {
        dc->FillSolidRect(&m_Rect, m_NoteOnColor);
    }
    // Note is off
    else
    {
        dc->FillSolidRect(&m_Rect, RGB(0, 0, 0));
    }

    dc->SelectStockObject(BLACK_PEN);

    // 
    // Draw border
    //

    // Left side
    dc->MoveTo(m_Rect.left, m_Rect.top);
    dc->LineTo(m_Rect.left, m_Rect.bottom);

    // Bottom
    dc->LineTo(m_Rect.right, m_Rect.bottom);

    // Right side
    dc->LineTo(m_Rect.right, m_Rect.top);

    // Top
    dc->LineTo(m_Rect.left, m_Rect.top);
}


