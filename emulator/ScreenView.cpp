/*  This file is part of MK90BTL.
    MK90BTL is free software: you can redistribute it and/or modify it under the terms
of the GNU Lesser General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.
    MK90BTL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Lesser General Public License for more details.
    You should have received a copy of the GNU Lesser General Public License along with
MK90BTL. If not, see <http://www.gnu.org/licenses/>. */

// ScreenView.cpp

#include "stdafx.h"
#include <mmintrin.h>
#include <vfw.h>
#include "Main.h"
#include "Views.h"
#include "Emulator.h"
#include "util/BitmapFile.h"

//////////////////////////////////////////////////////////////////////


#define COLOR_BK_BACKGROUND RGB(190,188,190)


HWND g_hwndScreen = NULL;  // Screen View window handle

HDRAWDIB m_hdd = NULL;
BITMAPINFO m_bmpinfo;
HBITMAP m_hbmp = NULL;
DWORD * m_bits = NULL;
int m_cxScreenWidth = DEFAULT_SCREEN_WIDTH;
int m_cyScreenHeight = DEFAULT_SCREEN_HEIGHT;
int m_xScreenOffset = 0;
int m_yScreenOffset = 0;
BYTE m_ScreenKeyState[256];
int m_ScreenMode = 0;
int m_ScreenPalette = 0;

void ScreenView_CreateDisplay();
void ScreenView_OnDraw(HDC hdc);
//BOOL ScreenView_OnKeyEvent(WPARAM vkey, BOOL okExtKey, BOOL okPressed);

const int KEYEVENT_QUEUE_SIZE = 32;
WORD m_ScreenKeyQueue[KEYEVENT_QUEUE_SIZE];
int m_ScreenKeyQueueTop = 0;
int m_ScreenKeyQueueBottom = 0;
int m_ScreenKeyQueueCount = 0;
void ScreenView_PutKeyEventToQueue(WORD keyevent);
WORD ScreenView_GetKeyEventFromQueue();


//////////////////////////////////////////////////////////////////////

void ScreenView_RegisterClass()
{
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = ScreenViewWndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = g_hInst;
    wcex.hIcon          = NULL;
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = NULL; //(HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = NULL;
    wcex.lpszClassName  = CLASSNAME_SCREENVIEW;
    wcex.hIconSm        = NULL;

    RegisterClassEx(&wcex);
}

void ScreenView_Init()
{
    m_hdd = DrawDibOpen();
    ScreenView_CreateDisplay();
}

void ScreenView_Done()
{
    if (m_hbmp != NULL)
    {
        VERIFY(::DeleteObject(m_hbmp));
        m_hbmp = NULL;
    }

    DrawDibClose( m_hdd );
}

void ScreenView_CreateDisplay()
{
    ASSERT(g_hwnd != NULL);

    if (m_hbmp != NULL)
    {
        VERIFY(::DeleteObject(m_hbmp));
        m_hbmp = NULL;
    }

    HDC hdc = ::GetDC(g_hwnd);

    m_bmpinfo.bmiHeader.biSize = sizeof( BITMAPINFOHEADER );
    m_bmpinfo.bmiHeader.biWidth = m_cxScreenWidth;
    m_bmpinfo.bmiHeader.biHeight = m_cyScreenHeight;
    m_bmpinfo.bmiHeader.biPlanes = 1;
    m_bmpinfo.bmiHeader.biBitCount = 32;
    m_bmpinfo.bmiHeader.biCompression = BI_RGB;
    m_bmpinfo.bmiHeader.biSizeImage = 0;
    m_bmpinfo.bmiHeader.biXPelsPerMeter = 0;
    m_bmpinfo.bmiHeader.biYPelsPerMeter = 0;
    m_bmpinfo.bmiHeader.biClrUsed = 0;
    m_bmpinfo.bmiHeader.biClrImportant = 0;

    m_hbmp = CreateDIBSection( hdc, &m_bmpinfo, DIB_RGB_COLORS, (void **) &m_bits, NULL, 0 );

    VERIFY(::ReleaseDC(g_hwnd, hdc));
}

// Create Screen View as child of Main Window
void ScreenView_Create(HWND hwndParent, int x, int y)
{
    ASSERT(hwndParent != NULL);

    int xLeft = x;
    int yTop = y;
    int cyHeight = 8 + DEFAULT_SCREEN_HEIGHT + 8;
    int cxWidth = 8 + m_cxScreenWidth + 8;

    g_hwndScreen = CreateWindow(
            CLASSNAME_SCREENVIEW, NULL,
            WS_CHILD | WS_VISIBLE,
            xLeft, yTop, cxWidth, cyHeight,
            hwndParent, NULL, g_hInst, NULL);

    // Initialize m_ScreenKeyState
    VERIFY(::GetKeyboardState(m_ScreenKeyState));
}

void ScreenView_GetDesiredSize(SIZE& size)
{
    size.cx = m_cxScreenWidth / 120 * 6 + m_cxScreenWidth;
    size.cy = m_cyScreenHeight / 64 * 6 + m_cyScreenHeight;
}

LRESULT CALLBACK ScreenViewWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            ScreenView_PrepareScreen();  //DEBUG
            ScreenView_OnDraw(hdc);

            EndPaint(hWnd, &ps);
        }
        break;
    case WM_LBUTTONDOWN:
        SetFocus(hWnd);
        break;
        //case WM_KEYDOWN:
        //    //if ((lParam & (1 << 30)) != 0)  // Auto-repeats should be ignored
        //    //    return (LRESULT) TRUE;
        //    //return (LRESULT) ScreenView_OnKeyEvent(wParam, (lParam & (1 << 24)) != 0, TRUE);
        //    return (LRESULT) TRUE;
        //case WM_KEYUP:
        //    //return (LRESULT) ScreenView_OnKeyEvent(wParam, (lParam & (1 << 24)) != 0, FALSE);
        //    return (LRESULT) TRUE;
    case WM_SETCURSOR:
        if (::GetFocus() == g_hwndScreen)
        {
            SetCursor(::LoadCursor(NULL, MAKEINTRESOURCE(IDC_IBEAM)));
            return (LRESULT) TRUE;
        }
        else
            return DefWindowProc(hWnd, message, wParam, lParam);
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return (LRESULT)FALSE;
}

int ScreenView_GetScreenMode()
{
    return m_ScreenMode;
}
void ScreenView_SetScreenMode(int newMode)
{
    if (m_ScreenMode == newMode) return;

    m_ScreenMode = newMode;

    // Ask Emulator module for screen width and height
    int cxWidth, cyHeight;
    Emulator_GetScreenSize(newMode, &cxWidth, &cyHeight);
    m_cxScreenWidth = cxWidth;
    m_cyScreenHeight = cyHeight;
    ScreenView_CreateDisplay();

    ScreenView_RedrawScreen();
}

int ScreenView_GetScreenPalette()
{
    return m_ScreenPalette;
}
void ScreenView_SetScreenPalette(int newPalette)
{
    if (m_ScreenPalette == newPalette) return;

    m_ScreenPalette = newPalette;

    ScreenView_RedrawScreen();
}

void ScreenView_OnDraw(HDC hdc)
{
    if (m_bits == NULL) return;

    HBRUSH hBrush = ::CreateSolidBrush(COLOR_BK_BACKGROUND);
    HGDIOBJ hOldBrush = ::SelectObject(hdc, hBrush);

    RECT rc;  ::GetClientRect(g_hwndScreen, &rc);

    m_xScreenOffset = 0;
    m_yScreenOffset = 0;
    if (rc.right > m_cxScreenWidth)
    {
        m_xScreenOffset = (rc.right - m_cxScreenWidth) / 2;
        ::PatBlt(hdc, 0, 0, m_xScreenOffset, rc.bottom, PATCOPY);
        ::PatBlt(hdc, rc.right, 0, m_cxScreenWidth + m_xScreenOffset - rc.right, rc.bottom, PATCOPY);
    }
    if (rc.bottom > m_cyScreenHeight)
    {
        m_yScreenOffset = (rc.bottom - m_cyScreenHeight) / 2;
        ::PatBlt(hdc, m_xScreenOffset, 0, m_cxScreenWidth, m_yScreenOffset, PATCOPY);
        int frombottom = rc.bottom - m_yScreenOffset - m_cyScreenHeight;
        ::PatBlt(hdc, m_xScreenOffset, rc.bottom, m_cxScreenWidth, -frombottom, PATCOPY);
    }

    ::SelectObject(hdc, hOldBrush);
    VERIFY(::DeleteObject(hBrush));

    DrawDibDraw(m_hdd, hdc,
            m_xScreenOffset, m_yScreenOffset, -1, -1,
            &m_bmpinfo.bmiHeader, m_bits, 0, 0,
            m_cxScreenWidth, m_cyScreenHeight,
            0);
}

void ScreenView_RedrawScreen()
{
    ScreenView_PrepareScreen();

    HDC hdc = ::GetDC(g_hwndScreen);
    ScreenView_OnDraw(hdc);
    VERIFY(::ReleaseDC(g_hwndScreen, hdc));
}

void ScreenView_PrepareScreen()
{
    if (m_bits == NULL) return;

    Emulator_PrepareScreenRGB32(m_bits, m_ScreenMode, m_ScreenPalette);
}

void ScreenView_PutKeyEventToQueue(WORD keyevent)
{
    if (m_ScreenKeyQueueCount == KEYEVENT_QUEUE_SIZE) return;  // Full queue

    m_ScreenKeyQueue[m_ScreenKeyQueueTop] = keyevent;
    m_ScreenKeyQueueTop++;
    if (m_ScreenKeyQueueTop >= KEYEVENT_QUEUE_SIZE)
        m_ScreenKeyQueueTop = 0;
    m_ScreenKeyQueueCount++;
}
WORD ScreenView_GetKeyEventFromQueue()
{
    if (m_ScreenKeyQueueCount == 0) return 0;  // Empty queue

    WORD keyevent = m_ScreenKeyQueue[m_ScreenKeyQueueBottom];
    m_ScreenKeyQueueBottom++;
    if (m_ScreenKeyQueueBottom >= KEYEVENT_QUEUE_SIZE)
        m_ScreenKeyQueueBottom = 0;
    m_ScreenKeyQueueCount--;

    return keyevent;
}

// MK-90 char from PC scan key
const BYTE arrPcScan2Mk90CharLat[256] =
{
    /*       0     1     2     3     4     5     6     7     8     9     a     b     c     d     e     f  */
    /*0*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0373, 0000, 0000,
    /*1*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
    /*2*/    0000, 0000, 0000, 0000, 0000, 0133, 0073, 0273, 0077, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
    /*3*/    0247, 0043, 0103, 0143, 0203, 0243, 0047, 0107, 0147, 0207, 0000, 0000, 0000, 0000, 0000, 0000,
    /*4*/    0000, 0013, 0053, 0323, 0213, 0253, 0223, 0153, 0263, 0017, 0057, 0117, 0157, 0217, 0257, 0317,
    /*5*/    0357, 0367, 0023, 0063, 0123, 0163, 0313, 0113, 0127, 0167, 0353, 0000, 0000, 0000, 0000, 0000,
    /*6*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
    /*7*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
    /*8*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
    /*9*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
    /*a*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
    /*b*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0173, 0000, 0233, 0000,
    /*c*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
    /*d*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0027, 0000, 0067, 0000, 0000,
    /*e*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
    /*f*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
};
const BYTE arrPcScan2Mk90CharRus[256] =
{
    /*       0     1     2     3     4     5     6     7     8     9     a     b     c     d     e     f  */
    /*0*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0373, 0000, 0000,
    /*1*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
    /*2*/    0000, 0000, 0000, 0000, 0000, 0133, 0073, 0273, 0077, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
    /*3*/    0247, 0043, 0103, 0143, 0203, 0243, 0047, 0107, 0147, 0207, 0000, 0000, 0000, 0000, 0000, 0000,
    /*4*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
    /*5*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
    /*6*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
    /*7*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
    /*8*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
    /*9*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
    /*a*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
    /*b*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
    /*c*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
    /*d*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
    /*e*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
    /*f*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
};

void ScreenView_ScanKeyboard()
{
    if (! g_okEmulatorRunning) return;
    if (::GetFocus() == g_hwndScreen)
    {
        // Read current keyboard state
        BYTE keys[256];
        VERIFY(::GetKeyboardState(keys));

        //BOOL okShift = ((keys[VK_SHIFT] & 128) != 0);
        BOOL okCtrl = ((keys[VK_CONTROL] & 128) != 0);

        // Check every key for state change
        for (int scan = 0; scan < 256; scan++)
        {
            BYTE newstate = keys[scan];
            BYTE oldstate = m_ScreenKeyState[scan];
            if ((newstate & 128) == (oldstate & 128))
                continue; // Key state not changed

            bool islat = true;//TODO
            const BYTE * arrPcScan2Mk90Char = islat ? arrPcScan2Mk90CharLat : arrPcScan2Mk90CharRus;
            BYTE key = arrPcScan2Mk90Char[scan];
            if (okCtrl && key >= 'A' && key <= 'X')
                key -= 0x40;

            if ((newstate & 128) != 0 && scan > 4)
                DebugPrintFormat(_T("Screen key: 0x%02x %d %03o\r\n"), scan, okCtrl, (uint16_t)key);

            if (key == 0)
                continue;

            BYTE pressed = (newstate & 128) | (okCtrl ? 64 : 0);
            WORD keyevent = MAKEWORD(key, pressed);
            ScreenView_PutKeyEventToQueue(keyevent);
        }

        // Save keyboard state
        ::memcpy(m_ScreenKeyState, keys, 256);
    }
}

void ScreenView_ProcessKeyboard()
{
    // Process next event in the keyboard queue
    WORD keyevent = ScreenView_GetKeyEventFromQueue();
    if (keyevent != 0)
    {
        bool pressed = ((keyevent & 0x8000) != 0);
        //bool ctrl = ((keyevent & 0x4000) != 0);
        BYTE bkscan = LOBYTE(keyevent);

//        DebugPrintFormat(_T("KeyEvent: 0x%0x %d %d\r\n"), bkscan, pressed, ctrl);

        g_pBoard->KeyboardEvent(bkscan, pressed);
    }
}

// External key event - e.g. from KeyboardView
void ScreenView_KeyEvent(BYTE keyscan, BOOL pressed)
{
    ScreenView_PutKeyEventToQueue(MAKEWORD(keyscan, pressed ? 128 : 0));
}

BOOL ScreenView_SaveScreenshot(LPCTSTR sFileName)
{
    ASSERT(sFileName != NULL);
    ASSERT(m_bits != NULL);

    DWORD* pBits = (DWORD*) ::calloc(m_cxScreenWidth * m_cyScreenHeight, 4);
    const uint32_t* colors = Emulator_GetPalette(m_ScreenPalette);
    Emulator_PrepareScreenRGB32(pBits, m_ScreenMode, m_ScreenPalette);

    LPCTSTR sFileNameExt = _tcsrchr(sFileName, _T('.'));
    BOOL result = FALSE;
    if (sFileNameExt != NULL && _tcsicmp(sFileNameExt, _T(".png")) == 0)
        result = PngFile_SaveScreenshot((uint32_t*)pBits, colors, sFileName, m_cxScreenWidth, m_cyScreenHeight);
    else
        result = BmpFile_SaveScreenshot((uint32_t*)pBits, colors, sFileName, m_cxScreenWidth, m_cyScreenHeight);

    ::free(pBits);

    return result;
}


//////////////////////////////////////////////////////////////////////
