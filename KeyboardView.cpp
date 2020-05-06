/*  This file is part of MK90BTL.
    MK90BTL is free software: you can redistribute it and/or modify it under the terms
of the GNU Lesser General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.
    MK90BTL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Lesser General Public License for more details.
    You should have received a copy of the GNU Lesser General Public License along with
MK90BTL. If not, see <http://www.gnu.org/licenses/>. */

// KeyboardView.cpp

#include "stdafx.h"
#include "Main.h"
#include "Views.h"
#include "Emulator.h"


//////////////////////////////////////////////////////////////////////

#define COLOR_KEYBOARD_BACKGROUND   RGB(190,188,190)
#define COLOR_KEYBOARD_LITE         RGB(228,228,228)
#define COLOR_KEYBOARD_GRAY         RGB(160,160,160)
#define COLOR_KEYBOARD_DARK         RGB(80,80,80)
#define COLOR_KEYBOARD_RED          RGB(200,80,80)

#define KEYCLASSLITE 0
#define KEYCLASSGRAY 1
#define KEYCLASSDARK 2

#define KEYSCAN_NONE 255

HWND g_hwndKeyboard = (HWND) INVALID_HANDLE_VALUE;  // Keyboard View window handle

int m_nKeyboardBitmapLeft = 0;
int m_nKeyboardBitmapTop = 0;
BYTE * m_pKeyboardRom = NULL;
BYTE m_nKeyboardKeyPressed = KEYSCAN_NONE;  // Scan-code for the key pressed, or KEYSCAN_NONE
BYTE m_nKeyboardCharPressed = 0;

void KeyboardView_OnDraw(HDC hdc);
int KeyboardView_GetKeyByPoint(int x, int y);
void Keyboard_DrawKey(HDC hdc, BYTE keyscan);


//////////////////////////////////////////////////////////////////////

#define KEYEXTRA_CIF    0201
#define KEYEXTRA_DOP    0202
#define KEYEXTRA_SHIFT  0203
#define KEYEXTRA_RUS    0204
#define KEYEXTRA_LAT    0205
#define KEYEXTRA_RUSLAT 0206
#define KEYEXTRA_UPR    0207
#define KEYEXTRA_FPB    0210

#define KEYEXTRA_START  0220
#define KEYEXTRA_STOP   0221
#define KEYEXTRA_TIMER  0222

struct KeyboardKeys
{
    int x, y, w, h;
    int keyclass;
    LPCTSTR text;
    BYTE scan;
}
m_arrKeyboardKeys[] =
{
    {  18,  23, 22, 18, KEYCLASSLITE, _T("?"),      0007 },
    {  52,  22, 24, 23, KEYCLASSGRAY, _T("1"),      0124 }, // 1 !
    {  86,  22, 24, 23, KEYCLASSGRAY, _T("2"),      0011 }, // 2 "
    { 120,  22, 24, 23, KEYCLASSGRAY, _T("3"),      0121 }, // 3 #
    { 154,  22, 24, 23, KEYCLASSGRAY, _T("4"),      0012 }, // 4
    { 188,  22, 24, 23, KEYCLASSGRAY, _T("5"),      0122 }, // 5 %
    { 222,  22, 24, 23, KEYCLASSGRAY, _T(":"),      0122 }, // :
    { 256,  22, 24, 23, KEYCLASSGRAY, _T(";"),      0122 }, // ;

    {  18,  61, 22, 18, KEYCLASSLITE, _T("?"),      0007 },
    {  52,  60, 24, 23, KEYCLASSGRAY, _T("6"),      0013 }, // 6 &
    {  86,  60, 24, 23, KEYCLASSGRAY, _T("7"),      0123 }, // 7 '
    { 120,  60, 24, 23, KEYCLASSGRAY, _T("8"),      0113 }, // 8 (
    { 154,  60, 24, 23, KEYCLASSGRAY, _T("9"),      0110 }, // 9 )
    { 188,  60, 24, 23, KEYCLASSGRAY, _T("0"),      0010 }, // 0
    { 222,  60, 24, 23, KEYCLASSGRAY, _T("/"),      0060 }, // /
    { 256,  60, 24, 23, KEYCLASSGRAY, _T("-"),      0016 }, // ~

    {  18,  98, 24, 20, KEYCLASSGRAY, _T("A"),      0052 }, // À A
    {  52,  98, 24, 20, KEYCLASSGRAY, _T("B"),      0132 }, // Á B
    {  86,  98, 24, 20, KEYCLASSGRAY, _T("W"),      0021 }, // Â W
    { 120,  98, 24, 20, KEYCLASSGRAY, _T("G"),      0112 }, // Ã G
    { 154,  98, 24, 20, KEYCLASSGRAY, _T("D"),      0073 }, // Ä D
    { 188,  98, 24, 20, KEYCLASSGRAY, _T("E"),      0002 }, // Å E
    { 222,  98, 24, 20, KEYCLASSGRAY, _T("V"),      0040 }, // Æ V
    { 256,  98, 24, 20, KEYCLASSGRAY, _T("Z"),      0033 }, // Ç Z

    {  18, 136, 24, 20, KEYCLASSGRAY, _T("I"),      0061 }, // È I
    {  52, 136, 24, 20, KEYCLASSGRAY, _T("J"),      0034 }, // É J
    {  86, 136, 24, 20, KEYCLASSGRAY, _T("K"),      0111 }, // Ê K
    { 120, 136, 24, 20, KEYCLASSGRAY, _T("L"),      0043 }, // Ë L
    { 154, 136, 24, 20, KEYCLASSGRAY, _T("M"),      0072 }, // Ì M
    { 188, 136, 24, 20, KEYCLASSGRAY, _T("N"),      0032 }, // Í N
    { 222, 136, 24, 20, KEYCLASSGRAY, _T("O"),      0053 }, // Î O
    { 256, 136, 24, 20, KEYCLASSGRAY, _T("P"),      0022 }, // Ï P

    {  18, 174, 24, 20, KEYCLASSGRAY, _T("R"),      0042 }, // Ð R
    {  52, 174, 24, 20, KEYCLASSGRAY, _T("S"),      0041 }, // Ñ S
    {  86, 174, 24, 20, KEYCLASSGRAY, _T("T"),      0131 }, // Ò T
    { 120, 174, 24, 20, KEYCLASSGRAY, _T("U"),      0031 }, // Ó U
    { 154, 174, 24, 20, KEYCLASSGRAY, _T("F"),      0024 }, // Ô F
    { 188, 174, 24, 20, KEYCLASSGRAY, _T("H"),      0000 }, // Õ H
    { 222, 174, 24, 20, KEYCLASSGRAY, _T("C"),      0001 }, // Ö C
    { 256, 174, 24, 20, KEYCLASSGRAY, _T("\u00ac"), 0071 }, // × ^

    {  18, 212, 24, 20, KEYCLASSGRAY, _T("["),      0003 }, // Ø [
    {  52, 212, 24, 20, KEYCLASSGRAY, _T("]"),      0023 }, // Ù ]
    {  86, 212, 24, 20, KEYCLASSGRAY, _T("X"),      0062 }, // Ü X
    { 120, 212, 24, 20, KEYCLASSGRAY, _T("Y"),      0051 }, // Û Y
    { 154, 212, 24, 20, KEYCLASSGRAY, _T("-"),      0020 }, // Ú }
    { 188, 212, 24, 20, KEYCLASSGRAY, _T("\\"),     0050 }, // Ý backslash
    { 222, 212, 24, 20, KEYCLASSGRAY, _T("@"),      0063 }, // Þ @
    { 256, 212, 24, 20, KEYCLASSGRAY, _T("Q"),      0044 }, // ß Q

    {  18, 250, 24, 20, KEYCLASSLITE, _T("?"),      0007 },
    {  52, 250, 24, 20, KEYCLASSLITE, _T("\u2191"), 0000 }, // Up
    {  86, 250, 24, 20, KEYCLASSLITE, _T("<-"),     0007 }, // Left
    { 120, 250, 24, 20, KEYCLASSLITE, _T(","),      0133 }, // , <
    { 154, 250, 24, 20, KEYCLASSLITE, _T("."),      0070 }, // . >
    { 188, 250, 24, 20, KEYCLASSLITE, _T("->"),     0007 }, // Right
    { 222, 250, 24, 20, KEYCLASSLITE, _T("ÇÂ"),     0007 },
    { 256, 250, 24, 20, KEYCLASSLITE, _T("ÂÊ"),     0046 }, // ÂÂÎÄ

    {  18, 288, 24, 20, KEYCLASSLITE, _T("Ð/Ë"),    0007 },
    {  52, 288, 24, 20, KEYCLASSLITE, _T("?"),      0007 }, // Down
    {  86, 288, 24, 20, KEYCLASSLITE, _T("?"),      0007 },
    { 120, 288, 58, 20, KEYCLASSLITE, NULL,         0102 }, // Space
    { 188, 288, 24, 20, KEYCLASSLITE, _T("?"),      0007 },
    { 222, 288, 24, 20, KEYCLASSLITE, _T("ÔÊ"),     0007 },
    { 256, 288, 24, 20, KEYCLASSLITE, _T("Â/Í"),    0000 }, // Shift
};

const int m_nKeyboardKeysCount = sizeof(m_arrKeyboardKeys) / sizeof(KeyboardKeys);


//////////////////////////////////////////////////////////////////////


void KeyboardView_RegisterClass()
{
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style			= CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc	= KeyboardViewWndProc;
    wcex.cbClsExtra		= 0;
    wcex.cbWndExtra		= 0;
    wcex.hInstance		= g_hInst;
    wcex.hIcon			= NULL;
    wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground	= (HBRUSH)(COLOR_BTNFACE + 1);
    wcex.lpszMenuName	= NULL;
    wcex.lpszClassName	= CLASSNAME_KEYBOARDVIEW;
    wcex.hIconSm		= NULL;

    RegisterClassEx(&wcex);
}

void KeyboardView_Init()
{
    HRSRC hres = ::FindResource(NULL, MAKEINTRESOURCE(IDR_MK90_KEYB), RT_RCDATA);
    HGLOBAL hgData = ::LoadResource(NULL, hres);
    m_pKeyboardRom = (BYTE*) ::LockResource(hgData);
}

void KeyboardView_Done()
{
}

void KeyboardView_Create(HWND hwndParent, int x, int y, int width, int height)
{
    ASSERT(hwndParent != NULL);

    g_hwndKeyboard = CreateWindow(
            CLASSNAME_KEYBOARDVIEW, NULL,
            WS_CHILD | WS_VISIBLE,
            x, y, width, height,
            hwndParent, NULL, g_hInst, NULL);
}

LRESULT CALLBACK KeyboardViewWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            KeyboardView_OnDraw(hdc);

            EndPaint(hWnd, &ps);
        }
        break;
    case WM_SETCURSOR:
        {
            POINT ptCursor;  ::GetCursorPos(&ptCursor);
            ::ScreenToClient(g_hwndKeyboard, &ptCursor);
            int keyindex = KeyboardView_GetKeyByPoint(ptCursor.x, ptCursor.y);
            LPCTSTR cursor = (keyindex == -1) ? IDC_ARROW : IDC_HAND;
            ::SetCursor(::LoadCursor(NULL, cursor));
        }
        return (LRESULT)TRUE;
    case WM_LBUTTONDOWN:
        {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            WORD fwkeys = (WORD) wParam;

            int keyindex = KeyboardView_GetKeyByPoint(x, y);
            if (keyindex == -1) break;
            BYTE keyscan = (BYTE) m_arrKeyboardKeys[keyindex].scan;
            if (keyscan == KEYSCAN_NONE) break;

            if (keyscan < 128)
            {
                // Convert keyscan to key char
                BOOL ctrl = (fwkeys & MK_CONTROL) != 0;
                BOOL shift = (fwkeys & MK_SHIFT) != 0;
                bool islat = true;//TODO
                int tableindex = 1 | (shift ? 4 : 0) | (ctrl ? 0 : 2) | (islat ? 0 : 8);  //TODO
                int index = tableindex * 128 + keyscan;
                m_nKeyboardCharPressed = m_pKeyboardRom[index];
//#if !defined(PRODUCT)
//                DebugPrintFormat(_T("Keyboard key: 0x%0x\r\n"), m_nKeyboardCharPressed);
//#endif

                // Fire keydown event
                ScreenView_KeyEvent(m_nKeyboardCharPressed, TRUE);
            }
            else
            {
                switch (keyscan)
                {
                case KEYEXTRA_LAT:
                    break;
                case KEYEXTRA_RUS:
                    break;
                case KEYEXTRA_RUSLAT:
                    break;
                case KEYEXTRA_CIF:
                    break;
                case KEYEXTRA_DOP:
                    break;
                case KEYEXTRA_TIMER:
                    break;
                }
            }

            ::SetCapture(g_hwndKeyboard);

            // Draw focus frame for the key pressed
            HDC hdc = ::GetDC(g_hwndKeyboard);
            Keyboard_DrawKey(hdc, keyscan);
            ::ReleaseDC(g_hwndKeyboard, hdc);

            // Remember key pressed
            m_nKeyboardKeyPressed = keyscan;
        }
        break;
    case WM_LBUTTONUP:
        if (m_nKeyboardKeyPressed != KEYSCAN_NONE)
        {
            // Fire keyup event and release mouse
            if (m_nKeyboardKeyPressed < 128)
                ScreenView_KeyEvent(m_nKeyboardCharPressed, FALSE);
            ::ReleaseCapture();

            // Draw focus frame for the released key
            HDC hdc = ::GetDC(g_hwndKeyboard);
            Keyboard_DrawKey(hdc, m_nKeyboardKeyPressed);
            ::ReleaseDC(g_hwndKeyboard, hdc);

            m_nKeyboardKeyPressed = KEYSCAN_NONE;
        }
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return (LRESULT)FALSE;
}

void KeyboardView_OnDraw(HDC hdc)
{
    RECT rc;  ::GetClientRect(g_hwndKeyboard, &rc);

    // Keyboard background
    HBRUSH hBkBrush = ::CreateSolidBrush(COLOR_KEYBOARD_BACKGROUND);
    HGDIOBJ hOldBrush = ::SelectObject(hdc, hBkBrush);
    ::PatBlt(hdc, 0, 0, rc.right, rc.bottom, PATCOPY);
    ::SelectObject(hdc, hOldBrush);

    if (m_nKeyboardKeyPressed != KEYSCAN_NONE)
        Keyboard_DrawKey(hdc, m_nKeyboardKeyPressed);

    HBRUSH hbrLite = ::CreateSolidBrush(COLOR_KEYBOARD_LITE);
    HBRUSH hbrGray = ::CreateSolidBrush(COLOR_KEYBOARD_GRAY);
    HBRUSH hbrDark = ::CreateSolidBrush(COLOR_KEYBOARD_DARK);
    HBRUSH hbrRed = ::CreateSolidBrush(COLOR_KEYBOARD_RED);
    m_nKeyboardBitmapLeft = (rc.right - 300) / 2;
    m_nKeyboardBitmapTop = (rc.bottom - 330) / 2;
    if (m_nKeyboardBitmapTop < 0) m_nKeyboardBitmapTop = 0;
    if (m_nKeyboardBitmapTop > 16) m_nKeyboardBitmapTop = 16;

    HFONT hfont = CreateDialogFont();
    HGDIOBJ hOldFont = ::SelectObject(hdc, hfont);
    ::SetBkMode(hdc, TRANSPARENT);

    // Draw keys
    for (int i = 0; i < m_nKeyboardKeysCount; i++)
    {
        RECT rcKey;
        rcKey.left = m_nKeyboardBitmapLeft + m_arrKeyboardKeys[i].x;
        rcKey.top = m_nKeyboardBitmapTop + m_arrKeyboardKeys[i].y;
        rcKey.right = rcKey.left + m_arrKeyboardKeys[i].w;
        rcKey.bottom = rcKey.top + m_arrKeyboardKeys[i].h;

        HBRUSH hbr = hBkBrush;
        COLORREF textcolor = COLOR_KEYBOARD_DARK;
        switch (m_arrKeyboardKeys[i].keyclass)
        {
        case KEYCLASSLITE: hbr = hbrLite; break;
        case KEYCLASSGRAY: hbr = hbrGray; break;
        case KEYCLASSDARK: hbr = hbrDark;  textcolor = COLOR_KEYBOARD_LITE; break;
        }
        HGDIOBJ hOldBrush = ::SelectObject(hdc, hbr);
        //rcKey.left++; rcKey.top++; rcKey.right--; rc.bottom--;
        ::PatBlt(hdc, rcKey.left, rcKey.top, rcKey.right - rcKey.left, rcKey.bottom - rcKey.top, PATCOPY);
        ::SelectObject(hdc, hOldBrush);

        //TCHAR text[10];
        //wsprintf(text, _T("%02x"), (int)m_arrKeyboardKeys[i].scan);
        LPCTSTR text = m_arrKeyboardKeys[i].text;
        if (text != NULL)
        {
            ::SetTextColor(hdc, textcolor);
            ::DrawText(hdc, text, wcslen(text), &rcKey, DT_NOPREFIX | DT_SINGLELINE | DT_CENTER | DT_VCENTER);
        }

        ::DrawEdge(hdc, &rcKey, BDR_RAISEDOUTER, BF_RECT);
    }

    ::SelectObject(hdc, hOldFont);
    ::DeleteObject(hfont);

    ::DeleteObject(hbrLite);
    ::DeleteObject(hbrGray);
    ::DeleteObject(hbrDark);
    ::DeleteObject(hbrRed);
    ::DeleteObject(hBkBrush);
}

// Returns: index of key under the cursor position, or -1 if not found
int KeyboardView_GetKeyByPoint(int x, int y)
{
    for (int i = 0; i < m_nKeyboardKeysCount; i++)
    {
        RECT rcKey;
        rcKey.left = m_nKeyboardBitmapLeft + m_arrKeyboardKeys[i].x;
        rcKey.top = m_nKeyboardBitmapTop + m_arrKeyboardKeys[i].y;
        rcKey.right = rcKey.left + m_arrKeyboardKeys[i].w;
        rcKey.bottom = rcKey.top + m_arrKeyboardKeys[i].h;

        if (x >= rcKey.left && x < rcKey.right && y >= rcKey.top && y < rcKey.bottom)
        {
            return i;
        }
    }
    return -1;
}

void Keyboard_DrawKey(HDC hdc, BYTE keyscan)
{
    for (int i = 0; i < m_nKeyboardKeysCount; i++)
        if (keyscan == m_arrKeyboardKeys[i].scan)
        {
            RECT rcKey;
            rcKey.left = m_nKeyboardBitmapLeft + m_arrKeyboardKeys[i].x;
            rcKey.top = m_nKeyboardBitmapTop + m_arrKeyboardKeys[i].y;
            rcKey.right = rcKey.left + m_arrKeyboardKeys[i].w;
            rcKey.bottom = rcKey.top + m_arrKeyboardKeys[i].h;
            ::DrawFocusRect(hdc, &rcKey);
        }
}


//////////////////////////////////////////////////////////////////////
