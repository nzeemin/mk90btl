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

#define KEYSCAN_NONE 0

HWND g_hwndKeyboard = (HWND) INVALID_HANDLE_VALUE;  // Keyboard View window handle

int m_nKeyboardBitmapLeft = 0;
int m_nKeyboardBitmapTop = 0;
int m_nKeyboardViewScale = 3;
BYTE m_nKeyboardKeyPressed = KEYSCAN_NONE;  // Scan-code for the key pressed, or KEYSCAN_NONE

void KeyboardView_OnDraw(HDC hdc);
int KeyboardView_GetKeyByPoint(int x, int y);
void Keyboard_DrawKey(HDC hdc, BYTE keyscan);


//////////////////////////////////////////////////////////////////////

struct KeyboardKeys
{
    int x, y, w, h;
    BYTE scan;
}
m_arrKeyboardKeys[] =
{
    {  28,  45, 38, 26, 0000 }, // Power
    {  75,  43, 39, 30, 0043 }, // 1 !
    { 123,  43, 39, 30, 0103 }, // 2 "
    { 170,  43, 39, 30, 0143 }, // 3 #
    { 218,  43, 39, 30, 0203 }, // 4
    { 266,  43, 39, 30, 0243 }, // 5 %
    { 313,  43, 39, 30, 0303 }, // :
    { 361,  43, 39, 30, 0343 }, // ;

    {  28,  90, 38, 26, 0000 }, // Reset
    {  75,  88, 39, 30, 0047 }, // 6 &
    { 123,  88, 39, 30, 0107 }, // 7 '
    { 170,  88, 39, 30, 0147 }, // 8 (
    { 218,  88, 39, 30, 0207 }, // 9 )
    { 266,  88, 39, 30, 0247 }, // 0
    { 313,  88, 39, 30, 0307 }, // /
    { 361,  88, 39, 30, 0347 }, // ~

    {  28, 136, 38, 27, 0013 }, // À A
    {  75, 136, 38, 27, 0053 }, // Á B
    { 123, 136, 38, 27, 0113 }, // Â W
    { 170, 136, 38, 27, 0153 }, // Ã G
    { 218, 136, 38, 27, 0213 }, // Ä D
    { 266, 136, 38, 27, 0253 }, // Å E
    { 313, 136, 38, 27, 0313 }, // Æ V
    { 361, 136, 38, 27, 0353 }, // Ç Z

    {  28, 178, 38, 27, 0017 }, // È I
    {  75, 178, 38, 27, 0057 }, // É J
    { 123, 178, 38, 27, 0117 }, // Ê K
    { 170, 178, 38, 27, 0157 }, // Ë L
    { 218, 178, 38, 27, 0217 }, // Ì M
    { 266, 178, 38, 27, 0257 }, // Í N
    { 313, 178, 38, 27, 0317 }, // Î O
    { 361, 178, 38, 27, 0357 }, // Ï P

    {  28, 220, 38, 27, 0023 }, // Ð R
    {  75, 220, 38, 27, 0063 }, // Ñ S
    { 123, 220, 38, 27, 0123 }, // Ò T
    { 170, 220, 38, 27, 0163 }, // Ó U
    { 218, 220, 38, 27, 0223 }, // Ô F
    { 266, 220, 38, 27, 0263 }, // Õ H
    { 313, 220, 38, 27, 0323 }, // Ö C
    { 361, 220, 38, 27, 0363 }, // × ^

    {  28, 262, 38, 27, 0027 }, // Ø [
    {  75, 262, 38, 27, 0067 }, // Ù ]
    { 123, 262, 38, 27, 0127 }, // Ü X
    { 170, 262, 38, 27, 0167 }, // Û Y
    { 218, 262, 38, 27, 0227 }, // Ú }
    { 266, 262, 38, 27, 0267 }, // Ý backslash
    { 313, 262, 38, 27, 0327 }, // Þ @
    { 361, 262, 38, 27, 0367 }, // ß Q

    {  28, 304, 38, 27, 0033 },
    {  75, 304, 38, 27, 0073 }, // Up
    { 123, 304, 38, 27, 0133 }, // Left
    { 170, 304, 38, 27, 0173 }, // , <
    { 218, 304, 38, 27, 0233 }, // . >
    { 266, 304, 38, 27, 0273 }, // Right
    { 313, 304, 38, 27, 0333 },
    { 361, 304, 38, 27, 0373 }, // ÂÊ Enter

    {  28, 346, 38, 27, 0037 },
    {  75, 346, 38, 27, 0077 }, // Down
    { 123, 346, 38, 27, 0137 }, // Down from bar
    { 170, 346, 86, 27, 0177 }, // Space
    { 266, 346, 38, 27, 0277 },
    { 313, 346, 38, 27, 0337 },
    { 361, 346, 38, 27, 0377 }, // Shift
};

const int m_nKeyboardKeysCount = sizeof(m_arrKeyboardKeys) / sizeof(KeyboardKeys);


//////////////////////////////////////////////////////////////////////


void KeyboardView_RegisterClass()
{
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = KeyboardViewWndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = g_hInst;
    wcex.hIcon          = NULL;
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_BTNFACE + 1);
    wcex.lpszMenuName   = NULL;
    wcex.lpszClassName  = CLASSNAME_KEYBOARDVIEW;
    wcex.hIconSm        = NULL;

    RegisterClassEx(&wcex);
}

void KeyboardView_Init()
{
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
            //WORD fwkeys = (WORD) wParam;

            int keyindex = KeyboardView_GetKeyByPoint(x, y);
            if (keyindex == -1) break;
            BYTE keyscan = m_arrKeyboardKeys[keyindex].scan;
            if (keyscan == KEYSCAN_NONE)
                break;

            // Fire keydown event and capture mouse
            ScreenView_KeyEvent(keyscan, TRUE);
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
            ScreenView_KeyEvent(m_nKeyboardKeyPressed, FALSE);
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

void KeyboardView_RedrawKeyboard()
{
    InvalidateRect(g_hwndKeyboard, NULL, TRUE);
}

void KeyboardView_OnDraw(HDC hdc)
{
    if (Settings_GetDebug())
        m_nKeyboardViewScale = 3;  // In Debug mode, we're using keyboard for scale 3, but screen scale 4
    else
        m_nKeyboardViewScale = Emulator_GetScreenScale(ScreenView_GetScreenMode());
    UINT nKeyboardResource = IDB_KEYBOARD3;
    switch (m_nKeyboardViewScale)
    {
    case 3: nKeyboardResource = IDB_KEYBOARD3; break;
    case 4: nKeyboardResource = IDB_KEYBOARD4; break;
    case 5: nKeyboardResource = IDB_KEYBOARD5; break;
    case 6: nKeyboardResource = IDB_KEYBOARD6; break;
    case 8: nKeyboardResource = IDB_KEYBOARD8; break;
    }

    HBITMAP hBmp = ::LoadBitmap(g_hInst, MAKEINTRESOURCE(nKeyboardResource));

    HDC hdcMem = ::CreateCompatibleDC(hdc);
    HGDIOBJ hOldBitmap = ::SelectObject(hdcMem, hBmp);

    RECT rc;  ::GetClientRect(g_hwndKeyboard, &rc);

    BITMAP bitmap;
    VERIFY(::GetObject(hBmp, sizeof(BITMAP), &bitmap));
    int cxBitmap = (int)bitmap.bmWidth;
    int cyBitmap = (int)bitmap.bmHeight;
    m_nKeyboardBitmapLeft = -m_nKeyboardViewScale * 160; //(rc.right - 300) / 2;
    m_nKeyboardBitmapTop = (rc.bottom - cyBitmap) / 2;
    if (m_nKeyboardBitmapTop < 0) m_nKeyboardBitmapTop = 0;
    //if (m_nKeyboardBitmapTop > 16) m_nKeyboardBitmapTop = 16;

    ::BitBlt(hdc, m_nKeyboardBitmapLeft, m_nKeyboardBitmapTop, cxBitmap, cyBitmap, hdcMem, 0, 0, SRCCOPY);

    ::SelectObject(hdcMem, hOldBitmap);
    ::DeleteDC(hdcMem);
    ::DeleteObject(hBmp);

    // Keyboard background
    //HBRUSH hBkBrush = ::CreateSolidBrush(COLOR_KEYBOARD_BACKGROUND);
    //HGDIOBJ hOldBrush = ::SelectObject(hdc, hBkBrush);
    //::PatBlt(hdc, 0, 0, rc.right, rc.bottom, PATCOPY);
    //::SelectObject(hdc, hOldBrush);
    //::DeleteObject(hBkBrush);

    if (m_nKeyboardKeyPressed != KEYSCAN_NONE)
        Keyboard_DrawKey(hdc, m_nKeyboardKeyPressed);

    //// Draw keys
    //HPEN hpenRed = ::CreatePen(PS_SOLID, 1, COLOR_KEYBOARD_RED);
    //HGDIOBJ hOldPen = ::SelectObject(hdc, hpenRed);
    //HBRUSH hbrushOld = static_cast<HBRUSH>(::SelectObject(hdc, ::GetStockObject(NULL_BRUSH)));
    //for (int i = 0; i < m_nKeyboardKeysCount; i++)
    //{
    //    RECT rcKey;
    //    rcKey.left = /*m_nKeyboardBitmapLeft +*/ m_arrKeyboardKeys[i].x * m_nKeyboardViewScale / 4;
    //    rcKey.top = m_nKeyboardBitmapTop + m_arrKeyboardKeys[i].y * m_nKeyboardViewScale / 4;
    //    rcKey.right = rcKey.left + m_arrKeyboardKeys[i].w * m_nKeyboardViewScale / 4;
    //    rcKey.bottom = rcKey.top + m_arrKeyboardKeys[i].h * m_nKeyboardViewScale / 4;
    //    ::Rectangle(hdc, rcKey.left, rcKey.top, rcKey.right, rcKey.bottom);
    //}
    //::SelectObject(hdc, hbrushOld);
    //::SelectObject(hdc, hOldPen);
    //::DeleteObject(hpenRed);
}

// Returns: index of key under the cursor position, or -1 if not found
int KeyboardView_GetKeyByPoint(int x, int y)
{
    for (int i = 0; i < m_nKeyboardKeysCount; i++)
    {
        RECT rcKey;
        rcKey.left = /*m_nKeyboardBitmapLeft +*/ m_arrKeyboardKeys[i].x * m_nKeyboardViewScale / 4;
        rcKey.top = m_nKeyboardBitmapTop + m_arrKeyboardKeys[i].y * m_nKeyboardViewScale / 4;
        rcKey.right = rcKey.left + m_arrKeyboardKeys[i].w * m_nKeyboardViewScale / 4;
        rcKey.bottom = rcKey.top + m_arrKeyboardKeys[i].h * m_nKeyboardViewScale / 4;

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
            rcKey.left = /*m_nKeyboardBitmapLeft +*/ m_arrKeyboardKeys[i].x * m_nKeyboardViewScale / 4;
            rcKey.top = m_nKeyboardBitmapTop + m_arrKeyboardKeys[i].y * m_nKeyboardViewScale / 4;
            rcKey.right = rcKey.left + m_arrKeyboardKeys[i].w * m_nKeyboardViewScale / 4;
            rcKey.bottom = rcKey.top + m_arrKeyboardKeys[i].h * m_nKeyboardViewScale / 4;
            ::DrawFocusRect(hdc, &rcKey);
        }
}


//////////////////////////////////////////////////////////////////////
