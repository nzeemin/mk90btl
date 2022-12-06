/*  This file is part of MK90BTL.
    MK90BTL is free software: you can redistribute it and/or modify it under the terms
of the GNU Lesser General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.
    MK90BTL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Lesser General Public License for more details.
    You should have received a copy of the GNU Lesser General Public License along with
MK90BTL. If not, see <http://www.gnu.org/licenses/>. */

// MainWindow.cpp

#include "stdafx.h"
#include <commdlg.h>
#include <crtdbg.h>
#include <mmintrin.h>
#include <Vfw.h>
#include <CommCtrl.h>

#include "Main.h"
#include "emubase\Emubase.h"
#include "Emulator.h"
#include "Dialogs.h"
#include "Views.h"
#include "ToolWindow.h"


//////////////////////////////////////////////////////////////////////


TCHAR g_szTitle[MAX_LOADSTRING];            // The title bar text
TCHAR g_szWindowClass[MAX_LOADSTRING];      // Main window class name

HWND m_hwndToolbar = NULL;
HWND m_hwndStatusbar = NULL;
HWND m_hwndSplitter = (HWND)INVALID_HANDLE_VALUE;

int m_MainWindowMinCx = DEFAULT_SCREEN_WIDTH + 40;
int m_MainWindowMinCy = DEFAULT_SCREEN_HEIGHT + 40;


//////////////////////////////////////////////////////////////////////
// Forward declarations

void MainWindow_RestorePositionAndShow();
LRESULT CALLBACK MainWindow_WndProc(HWND, UINT, WPARAM, LPARAM);
void MainWindow_AdjustWindowLayout();
bool MainWindow_DoCommand(int commandId);
void MainWindow_DoViewDebug();
void MainWindow_DoDebugMemoryMap();
void MainWindow_DoViewToolbar();
void MainWindow_DoViewScreenMode(int newMode);
void MainWindow_DoViewScreenPalette(int newPalette);
void MainWindow_DoEmulatorRun();
void MainWindow_DoEmulatorAutostart();
void MainWindow_DoEmulatorReset();
void MainWindow_DoEmulatorSpeed(WORD speed);
void MainWindow_DoEmulatorSound();
void MainWindow_DoEmulatorSmp(int slot);
void MainWindow_DoFileSaveState();
void MainWindow_DoFileLoadState();
void MainWindow_DoFileScreenshot();
void MainWindow_DoFileScreenshotToClipboard();
void MainWindow_DoFileScreenshotSaveAs();
void MainWindow_DoFileSettings();
void MainWindow_DoFileSettingsColors();
void MainWindow_OnToolbarGetInfoTip(LPNMTBGETINFOTIP);


//////////////////////////////////////////////////////////////////////


void MainWindow_RegisterClass()
{
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = MainWindow_WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = g_hInst;
    wcex.hIcon          = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_APPICON));
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_BTNFACE + 1);
    wcex.lpszMenuName   = MAKEINTRESOURCE(IDC_APPLICATION);
    wcex.lpszClassName  = g_szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    RegisterClassEx(&wcex);

    ToolWindow_RegisterClass();
    OverlappedWindow_RegisterClass();
    SplitterWindow_RegisterClass();

    // Register view classes
    ScreenView_RegisterClass();
    KeyboardView_RegisterClass();
    MemoryView_RegisterClass();
    DebugView_RegisterClass();
    MemoryMapView_RegisterClass();
    DisasmView_RegisterClass();
    ConsoleView_RegisterClass();
}

BOOL CreateMainWindow()
{
    // Create the window
    g_hwnd = CreateWindow(
            g_szWindowClass, g_szTitle,
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
            0, 0, 0, 0,
            NULL, NULL, g_hInst, NULL);
    if (!g_hwnd)
        return FALSE;

    // Create and set up the toolbar and the statusbar
    if (!MainWindow_InitToolbar())
        return FALSE;
    if (!MainWindow_InitStatusbar())
        return FALSE;

    DebugView_Init();
    DisasmView_Init();
    ScreenView_Init();
    KeyboardView_Init();

    // Create screen window as a child of the main window
    ScreenView_Create(g_hwnd, 0, 0);

    MainWindow_RestoreSettings();

    MainWindow_ShowHideToolbar();
    MainWindow_ShowHideKeyboard();
    MainWindow_ShowHideDebug();
    //MainWindow_ShowHideMemoryMap();

    MainWindow_RestorePositionAndShow();

    UpdateWindow(g_hwnd);
    MainWindow_UpdateAllViews();
    MainWindow_UpdateMenu();

    // Autostart
    if (Settings_GetAutostart() || Option_AutoBoot >= 0)
        ::PostMessage(g_hwnd, WM_COMMAND, ID_EMULATOR_RUN, 0);

    return TRUE;
}

BOOL MainWindow_InitToolbar()
{
    m_hwndToolbar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL,
            WS_CHILD | TBSTYLE_FLAT | TBSTYLE_TRANSPARENT | TBSTYLE_LIST | TBSTYLE_TOOLTIPS | CCS_NOPARENTALIGN | CCS_NODIVIDER,
            4, 4, 0, 0, g_hwnd,
            (HMENU) 102,
            g_hInst, NULL);
    if (! m_hwndToolbar)
        return FALSE;

    SendMessage(m_hwndToolbar, TB_SETEXTENDEDSTYLE, 0, (LPARAM) (DWORD) TBSTYLE_EX_MIXEDBUTTONS);
    SendMessage(m_hwndToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0);
    SendMessage(m_hwndToolbar, TB_SETBUTTONSIZE, 0, (LPARAM) MAKELONG (26, 26));

    TBADDBITMAP addbitmap;
    addbitmap.hInst = g_hInst;
    addbitmap.nID = IDB_TOOLBAR;
    SendMessage(m_hwndToolbar, TB_ADDBITMAP, 2, (LPARAM) &addbitmap);

    TBBUTTON buttons[8];
    ZeroMemory(buttons, sizeof(buttons));
    for (int i = 0; i < sizeof(buttons) / sizeof(TBBUTTON); i++)
    {
        buttons[i].fsState = TBSTATE_ENABLED;
        buttons[i].iString = -1;
    }
    buttons[0].idCommand = ID_EMULATOR_RUN;
    buttons[0].iBitmap = ToolbarImageRun;
    buttons[0].fsStyle = BTNS_BUTTON | BTNS_SHOWTEXT;
    buttons[0].iString = (int)SendMessage(m_hwndToolbar, TB_ADDSTRING, (WPARAM)0, (LPARAM)_T("Run"));
    buttons[1].idCommand = ID_EMULATOR_RESET;
    buttons[1].iBitmap = ToolbarImageReset;
    buttons[1].fsStyle = BTNS_BUTTON | BTNS_SHOWTEXT;
    buttons[1].iString = (int)SendMessage(m_hwndToolbar, TB_ADDSTRING, (WPARAM)0, (LPARAM)_T("Reset"));
    buttons[2].fsStyle = BTNS_SEP;
    buttons[3].idCommand = ID_EMULATOR_SMP0;
    buttons[3].iBitmap = ToolbarImageCartSlot;
    buttons[3].fsStyle = BTNS_BUTTON | BTNS_SHOWTEXT;
    buttons[3].iString = (int)SendMessage(m_hwndToolbar, TB_ADDSTRING, (WPARAM)0, (LPARAM)_T("0"));
    buttons[4].idCommand = ID_EMULATOR_SMP1;
    buttons[4].iBitmap = ToolbarImageCartSlot;
    buttons[4].fsStyle = BTNS_BUTTON | BTNS_SHOWTEXT;
    buttons[4].iString = (int)SendMessage(m_hwndToolbar, TB_ADDSTRING, (WPARAM)0, (LPARAM)_T("1"));
    buttons[5].fsStyle = BTNS_SEP;
    buttons[6].idCommand = ID_EMULATOR_SOUND;
    buttons[6].iBitmap = ToolbarImageSoundOff;
    buttons[6].fsStyle = BTNS_BUTTON | BTNS_SHOWTEXT;
    buttons[6].iString = (int)SendMessage(m_hwndToolbar, TB_ADDSTRING, (WPARAM)0, (LPARAM)_T("Sound"));
    buttons[7].idCommand = ID_FILE_SCREENSHOT;
    buttons[7].iBitmap = ToolbarImageScreenshot;
    buttons[7].fsStyle = BTNS_BUTTON;
    SendMessage(m_hwndToolbar, TB_ADDBUTTONS, (WPARAM) sizeof(buttons) / sizeof(TBBUTTON), (LPARAM) &buttons);

    if (Settings_GetToolbar())
        ShowWindow(m_hwndToolbar, SW_SHOW);

    return TRUE;
}

BOOL MainWindow_InitStatusbar()
{
    TCHAR buffer[100];
    _sntprintf(buffer, sizeof(buffer) / sizeof(TCHAR) - 1, _T("%s version %s"), g_szTitle, _T(APP_VERSION_STRING));
    m_hwndStatusbar = CreateStatusWindow(
            WS_CHILD | WS_VISIBLE | SBT_TOOLTIPS | CCS_NOPARENTALIGN | CCS_NODIVIDER,
            buffer,
            g_hwnd, 101);
    if (! m_hwndStatusbar)
        return FALSE;

    int statusbarParts[4];
    statusbarParts[0] = 380;
    statusbarParts[1] = statusbarParts[0] + 50;  // Motor
    statusbarParts[2] = statusbarParts[1] + 50;  // FPS
    statusbarParts[3] = -1;
    SendMessage(m_hwndStatusbar, SB_SETPARTS, sizeof(statusbarParts) / sizeof(int), (LPARAM) statusbarParts);

    return TRUE;
}

void MainWindow_RestoreSettings()
{
    TCHAR buf[MAX_PATH];

    // Reattach SMP images
    for (int slot = 0; slot < 2; slot++)
    {
        buf[0] = _T('\0');
        Settings_GetSmpFilePath(slot, buf);
        if (buf[0] != _T('\0'))
        {
            if (! g_pBoard->AttachSmpImage(slot, buf))
                Settings_SetSmpFilePath(slot, NULL);
        }
    }

    // Restore ScreenViewMode
    int scrmode = Settings_GetScreenViewMode();
    ScreenView_SetScreenMode(scrmode);
    int scrpalette = Settings_GetScreenPalette();
    ScreenView_SetScreenPalette(scrpalette);
}

void MainWindow_SavePosition()
{
    WINDOWPLACEMENT placement;
    placement.length = sizeof(WINDOWPLACEMENT);
    ::GetWindowPlacement(g_hwnd, &placement);

    Settings_SetWindowRect(&(placement.rcNormalPosition));
    Settings_SetWindowMaximized(placement.showCmd == SW_SHOWMAXIMIZED);
}
void MainWindow_RestorePositionAndShow()
{
    RECT rc;
    if (Settings_GetWindowRect(&rc))
    {
        HMONITOR hmonitor = MonitorFromRect(&rc, MONITOR_DEFAULTTONULL);
        if (hmonitor != NULL)
        {
            ::SetWindowPos(g_hwnd, NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
                    SWP_NOACTIVATE | SWP_NOZORDER);
        }
    }

    ShowWindow(g_hwnd, Settings_GetWindowMaximized() ? SW_SHOWMAXIMIZED : SW_SHOW);
}

void MainWindow_UpdateWindowTitle()
{
    LPCTSTR emustate = g_okEmulatorRunning ? _T("run") : _T("stop");
    TCHAR buffer[100];
    _sntprintf(buffer, sizeof(buffer) / sizeof(TCHAR) - 1, _T("%s [%s]"), g_szTitle, emustate);
    SetWindowText(g_hwnd, buffer);
}

// Processes messages for the main window
LRESULT CALLBACK MainWindow_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_ACTIVATE:
        SetFocus(g_hwndScreen);
        break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            //int wmEvent = HIWORD(wParam);
            bool okProcessed = MainWindow_DoCommand(wmId);
            if (!okProcessed)
                return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    case WM_DESTROY:
        MainWindow_SavePosition();
        PostQuitMessage(0);
        break;
    case WM_SIZE:
        MainWindow_AdjustWindowLayout();
        break;
    case WM_GETMINMAXINFO:
        {
            DefWindowProc(hWnd, message, wParam, lParam);
            MINMAXINFO* mminfo = (MINMAXINFO*)lParam;
            mminfo->ptMinTrackSize.x = m_MainWindowMinCx;
            mminfo->ptMinTrackSize.y = m_MainWindowMinCy;
        }
        break;
    case WM_NOTIFY:
        {
            //int idCtrl = (int) wParam;
            HWND hwndFrom = ((LPNMHDR) lParam)->hwndFrom;
            UINT code = ((LPNMHDR) lParam)->code;
            if (code == TTN_SHOW)
            {
                return 0;
            }
            else if (hwndFrom == m_hwndToolbar && code == TBN_GETINFOTIP)
            {
                MainWindow_OnToolbarGetInfoTip((LPNMTBGETINFOTIP) lParam);
            }
            else
                return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    case WM_DRAWITEM:
        {
            //int idCtrl = (int) wParam;
            HWND hwndItem = ((LPDRAWITEMSTRUCT) lParam)->hwndItem;
            if (hwndItem == m_hwndStatusbar)
                ; //MainWindow_OnStatusbarDrawItem((LPDRAWITEMSTRUCT) lParam);
            else
                return DefWindowProc(hWnd, message, wParam, lParam);
        }
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void MainWindow_AdjustWindowSize()
{
    WINDOWPLACEMENT placement;
    placement.length = sizeof(WINDOWPLACEMENT);
    ::GetWindowPlacement(g_hwnd, &placement);
    if (placement.showCmd == SW_MAXIMIZE)
        return;

    // Get metrics
    int cxFrame   = ::GetSystemMetrics(SM_CXSIZEFRAME);
    int cyFrame   = ::GetSystemMetrics(SM_CYSIZEFRAME);
    int cyCaption = ::GetSystemMetrics(SM_CYCAPTION);
    int cyMenu    = ::GetSystemMetrics(SM_CYMENU);

    RECT rcToolbar;  GetWindowRect(m_hwndToolbar, &rcToolbar);
    int cyToolbar = rcToolbar.bottom - rcToolbar.top;
    RECT rcScreen;  GetWindowRect(g_hwndScreen, &rcScreen);
    int cxScreen = rcScreen.right - rcScreen.left;
    int cyScreen = rcScreen.bottom - rcScreen.top;
    RECT rcStatus;  GetWindowRect(m_hwndStatusbar, &rcStatus);
    int cyStatus = rcStatus.bottom - rcStatus.top;

    // Adjust main window size
    int xLeft, yTop;
    int cxWidth, cyHeight;
    if (Settings_GetDebug())
    {
        RECT rcWorkArea;  SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWorkArea, 0);
        xLeft = rcWorkArea.left;
        yTop = rcWorkArea.top;
        cxWidth = rcWorkArea.right - rcWorkArea.left;
        cyHeight = rcWorkArea.bottom - rcWorkArea.top;
    }
    else
    {
        RECT rcCurrent;  ::GetWindowRect(g_hwnd, &rcCurrent);
        xLeft = rcCurrent.left;
        yTop = rcCurrent.top;
        cxWidth = cxScreen + cxFrame * 2;
        cyHeight = cyCaption + cyMenu + 4 + cyScreen + 4 + cyStatus + cyFrame * 2;
        if (Settings_GetToolbar())
            cyHeight += cyToolbar + 4;
    }

    SetWindowPos(g_hwnd, NULL, xLeft, yTop, cxWidth, cyHeight, SWP_NOZORDER | SWP_NOMOVE);
}

void MainWindow_AdjustWindowLayout()
{
    RECT rcStatus;  GetWindowRect(m_hwndStatusbar, &rcStatus);
    int cyStatus = rcStatus.bottom - rcStatus.top;

    int xScreen = 0, yScreen = 0;
    int cxScreen = 0, cyScreen = 0;

    int cyToolbar = 0;
    if (Settings_GetToolbar())
    {
        RECT rcToolbar;  GetWindowRect(m_hwndToolbar, &rcToolbar);
        cyToolbar = rcToolbar.bottom - rcToolbar.top;
        yScreen += cyToolbar + 4;
    }

    RECT rc;  GetClientRect(g_hwnd, &rc);
    int cxClient = rc.right;
    int cyClient = rc.bottom - cyToolbar - cyStatus;
    int cxStatus = cxClient;

    if (!Settings_GetDebug())  // No debug views
    {
        int imageWidth, imageHeight;
        Emulator_GetImageSize(ScreenView_GetScreenMode(), &imageWidth, &imageHeight);

        xScreen = (imageWidth < cxClient) ? (cxClient - imageWidth) / 2 : 0;
        cxScreen = imageWidth * 3 / 5;
        int cxKeyboard = imageWidth * 2 / 5;

        cyScreen = cyClient;
        if (cyScreen > imageHeight)
        {
            cyScreen = imageHeight;
            yScreen = (cyClient - cyScreen) / 2 + cyToolbar + 4;
        }

        int xKeyboard = xScreen + cxScreen;
        int yKeyboard = yScreen;
        int cyKeyboard = cyScreen;
        SetWindowPos(g_hwndKeyboard, NULL, xKeyboard, yKeyboard, cxKeyboard, cyKeyboard, SWP_NOZORDER | SWP_NOCOPYBITS);
    }
    if (Settings_GetDebug())  // Debug views shown -- keyboard/tape snapped to top
    {
        cxScreen = 480/*MK90_SCREEN_WIDTH*/ + 16;
        int cxKeyboard = 314;
        cyScreen = 312;

        int yKeyboard = yScreen;
        int yConsole = yKeyboard;
        int cyKeyboard = cyScreen;
        int xKeyboard = cxScreen;
        SetWindowPos(g_hwndKeyboard, NULL, xKeyboard, yKeyboard, cxKeyboard, cyKeyboard, SWP_NOZORDER);
        yConsole += cyKeyboard + 4;

        int cxConsole = cxScreen + cxKeyboard;
        int cyConsole = rc.bottom - cyStatus - yConsole - 4;
        cxStatus = cxConsole;
        SetWindowPos(g_hwndConsole, NULL, 0, yConsole, cxConsole, cyConsole, SWP_NOZORDER);

        RECT rcDebug;  GetWindowRect(g_hwndDebug, &rcDebug);
        int cxDebug = rc.right - cxScreen - cxKeyboard - 4;
        int cyDebug = rcDebug.bottom - rcDebug.top;

        int yMemory = 528;
        int cxMemory = rc.right - cxScreen - 4;
        int cyMemory = rc.bottom - yMemory;

        int yDisasm = cyDebug + 4;
        int cyDisasm = yMemory - yDisasm - 4;
        int cxDisasm = cxDebug;

        if (Settings_GetMemoryMap())
        {
            RECT rcMemoryMap;  GetWindowRect(g_hwndMemoryMap, &rcMemoryMap);
            int cxMemoryMap = rcMemoryMap.right - rcMemoryMap.left;
            int cyMemoryMap = rcMemoryMap.bottom - rcMemoryMap.top;
            int xMemoryMap = rc.right - cxMemoryMap;
            SetWindowPos(g_hwndMemoryMap, NULL, xMemoryMap, 0, cxMemoryMap, cyMemoryMap, SWP_NOZORDER);

            cxDebug -= cxMemoryMap + 4;
            cxDisasm -= cxMemoryMap + 4;
            //yMemory = cyMemoryMap + 4;
        }

        int xDebug = cxConsole + 4;
        SetWindowPos(g_hwndDebug, NULL, xDebug, 0, cxDebug, cyDebug, SWP_NOZORDER);
        SetWindowPos(g_hwndDisasm, NULL, xDebug, yDisasm, cxDisasm, cyDisasm, SWP_NOZORDER);
        SetWindowPos(m_hwndSplitter, NULL, xDebug, yMemory - 4, cxDebug, 4, SWP_NOZORDER);
        SetWindowPos(g_hwndMemory, NULL, xDebug, yMemory, cxMemory, cyMemory, SWP_NOZORDER);
    }

    SetWindowPos(m_hwndToolbar, NULL, 4, 4, cxStatus, cyToolbar, SWP_NOZORDER);

    SetWindowPos(g_hwndScreen, NULL, xScreen, yScreen, cxScreen, cyScreen, SWP_NOZORDER);

    int cyStatusReal = rcStatus.bottom - rcStatus.top;
    SetWindowPos(m_hwndStatusbar, NULL, 0, rc.bottom - cyStatusReal, cxStatus, cyStatusReal,
            SWP_NOZORDER | SWP_SHOWWINDOW);
}

void MainWindow_ShowHideDebug()
{
    if (!Settings_GetDebug())
    {
        // Delete debug views
        if (m_hwndSplitter != INVALID_HANDLE_VALUE)
            DestroyWindow(m_hwndSplitter);
        if (g_hwndConsole != INVALID_HANDLE_VALUE)
            DestroyWindow(g_hwndConsole);
        if (g_hwndDebug != INVALID_HANDLE_VALUE)
            DestroyWindow(g_hwndDebug);
        if (g_hwndDisasm != INVALID_HANDLE_VALUE)
            DestroyWindow(g_hwndDisasm);
        if (g_hwndMemory != INVALID_HANDLE_VALUE)
            DestroyWindow(g_hwndMemory);
        if (g_hwndMemoryMap != INVALID_HANDLE_VALUE)
            DestroyWindow(g_hwndMemoryMap);

        MainWindow_AdjustWindowSize();
    }
    else  // Debug Views ON
    {
        MainWindow_AdjustWindowSize();

        // Calculate children positions
        RECT rc;  GetClientRect(g_hwnd, &rc);
        RECT rcScreen;  GetWindowRect(g_hwndScreen, &rcScreen);
        RECT rcStatus;  GetWindowRect(m_hwndStatusbar, &rcStatus);
        int cyStatus = rcStatus.bottom - rcStatus.top;
        int yConsoleTop = rcScreen.bottom - rcScreen.top + 8;
        int cxConsoleWidth = rcScreen.right - rcScreen.left;
        int cyConsoleHeight = rc.bottom - cyStatus - yConsoleTop - 4;
        int xDebugLeft = (rcScreen.right - rcScreen.left) + 8;
        int cxDebugWidth = rc.right - xDebugLeft - 4;
        int cyDebugHeight = 216;
        int yDisasmTop = 4 + cyDebugHeight + 4;
        int cyDisasmHeight = 328;
        int yMemoryTop = cyDebugHeight + 4 + cyDisasmHeight + 8;
        int cyMemoryHeight = rc.bottom - cyStatus - yMemoryTop - 4;

        // Create debug views
        if (g_hwndConsole == INVALID_HANDLE_VALUE)
            ConsoleView_Create(g_hwnd, 4, yConsoleTop, cxConsoleWidth, cyConsoleHeight);
        if (g_hwndDebug == INVALID_HANDLE_VALUE)
            DebugView_Create(g_hwnd, xDebugLeft, 4, cxDebugWidth, cyDebugHeight);
        if (g_hwndDisasm == INVALID_HANDLE_VALUE)
            DisasmView_Create(g_hwnd, xDebugLeft, yDisasmTop, cxDebugWidth, cyDisasmHeight);
        if (g_hwndMemory == INVALID_HANDLE_VALUE)
            MemoryView_Create(g_hwnd, xDebugLeft, yMemoryTop, cxDebugWidth, cyMemoryHeight);
        if (g_hwndMemoryMap == INVALID_HANDLE_VALUE && Settings_GetMemoryMap())
            MemoryMapView_Create(g_hwnd, xDebugLeft, yMemoryTop);
        m_hwndSplitter = SplitterWindow_Create(g_hwnd, g_hwndDisasm, g_hwndMemory);
    }

    MainWindow_AdjustWindowLayout();

    MainWindow_UpdateMenu();

    SetFocus(g_hwndScreen);
}

void MainWindow_ShowHideToolbar()
{
    ShowWindow(m_hwndToolbar, Settings_GetToolbar() ? SW_SHOW : SW_HIDE);

    MainWindow_AdjustWindowSize();
    MainWindow_AdjustWindowLayout();
    MainWindow_UpdateMenu();
}

void MainWindow_ShowHideKeyboard()
{
    // Calculate children positions
    RECT rc;  GetClientRect(g_hwnd, &rc);
    RECT rcScreen;  GetWindowRect(g_hwndScreen, &rcScreen);
    int yKeyboardTop = rcScreen.bottom - rcScreen.top + 8;
    int cxKeyboardWidth = rcScreen.right - rcScreen.left;
    const int cyKeyboardHeight = 400;

    if (g_hwndKeyboard == INVALID_HANDLE_VALUE)
        KeyboardView_Create(g_hwnd, 4, yKeyboardTop, cxKeyboardWidth, cyKeyboardHeight);

    MainWindow_AdjustWindowSize();
    MainWindow_AdjustWindowLayout();
    MainWindow_UpdateMenu();
}

void MainWindow_ShowHideMemoryMap()
{
    if (!Settings_GetMemoryMap())
    {
        if (g_hwndMemoryMap != INVALID_HANDLE_VALUE)
        {
            ::DestroyWindow(g_hwndMemoryMap);
            g_hwndMemoryMap = (HWND)INVALID_HANDLE_VALUE;
        }
    }
    else if (Settings_GetDebug())
    {
        if (g_hwndMemoryMap == INVALID_HANDLE_VALUE)
            MemoryMapView_Create(g_hwnd, 0, 0);
    }

    MainWindow_AdjustWindowLayout();
    MainWindow_UpdateMenu();
}

void MainWindow_UpdateMenu()
{
    // Get main menu
    HMENU hMenu = GetMenu(g_hwnd);

    // Emulator|Run check
    CheckMenuItem(hMenu, ID_EMULATOR_RUN, (g_okEmulatorRunning ? MF_CHECKED : MF_UNCHECKED));
    SendMessage(m_hwndToolbar, TB_CHECKBUTTON, ID_EMULATOR_RUN, (g_okEmulatorRunning ? 1 : 0));
    //MainWindow_SetToolbarImage(ID_EMULATOR_RUN, g_okEmulatorRunning ? ToolbarImageRun : ToolbarImagePause);

    // View menu
    CheckMenuItem(hMenu, ID_VIEW_TOOLBAR, (Settings_GetToolbar() ? MF_CHECKED : MF_UNCHECKED));
    // View|Screen Mode
    UINT scrmodecmd = 0;
    switch (ScreenView_GetScreenMode())
    {
    case 0: scrmodecmd = ID_VIEW_SCREENMODE0; break;
    case 1: scrmodecmd = ID_VIEW_SCREENMODE1; break;
    case 2: scrmodecmd = ID_VIEW_SCREENMODE2; break;
    case 3: scrmodecmd = ID_VIEW_SCREENMODE3; break;
    case 4: scrmodecmd = ID_VIEW_SCREENMODE4; break;
    }
    CheckMenuRadioItem(hMenu, ID_VIEW_SCREENMODE0, ID_VIEW_SCREENMODE4, scrmodecmd, MF_BYCOMMAND);

    UINT scrpalcmd = 0;
    switch (ScreenView_GetScreenPalette())
    {
    case 0: scrpalcmd = ID_VIEW_PALETTE0; break;
    case 1: scrpalcmd = ID_VIEW_PALETTE1; break;
    case 2: scrpalcmd = ID_VIEW_PALETTE2; break;
    case 3: scrpalcmd = ID_VIEW_PALETTE3; break;
    case 4: scrpalcmd = ID_VIEW_PALETTE4; break;
    case 5: scrpalcmd = ID_VIEW_PALETTE5; break;
    }
    CheckMenuRadioItem(hMenu, ID_VIEW_PALETTE0, ID_VIEW_PALETTE5, scrpalcmd, MF_BYCOMMAND);

    // Emulator menu options
    CheckMenuItem(hMenu, ID_EMULATOR_AUTOSTART, (Settings_GetAutostart() ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hMenu, ID_EMULATOR_SOUND, (Settings_GetSound() ? MF_CHECKED : MF_UNCHECKED));

    UINT speedcmd = 0;
    switch (Settings_GetRealSpeed())
    {
    case 0x7ffe: speedcmd = ID_EMULATOR_SPEED25;   break;
    case 0x7fff: speedcmd = ID_EMULATOR_SPEED50;   break;
    case 0:      speedcmd = ID_EMULATOR_SPEEDMAX;  break;
    case 1:      speedcmd = ID_EMULATOR_REALSPEED; break;
    case 2:      speedcmd = ID_EMULATOR_SPEED200;  break;
    }
    CheckMenuRadioItem(hMenu, ID_EMULATOR_SPEED25, ID_EMULATOR_SPEED200, speedcmd, MF_BYCOMMAND);

    MainWindow_SetToolbarImage(ID_EMULATOR_SOUND, (Settings_GetSound() ? ToolbarImageSoundOn : ToolbarImageSoundOff));
    EnableMenuItem(hMenu, ID_DEBUG_STEPINTO, (g_okEmulatorRunning ? MF_DISABLED : MF_ENABLED));

    UINT configcmd = 0;
    switch (g_nEmulatorConfiguration)
    {
    case EMU_CONF_BASIC10: configcmd = ID_CONF_BASIC10; break;
    case EMU_CONF_BASIC20: configcmd = ID_CONF_BASIC20; break;
    }
    CheckMenuRadioItem(hMenu, ID_CONF_BASIC10, ID_CONF_BASIC20, configcmd, MF_BYCOMMAND);

    CheckMenuItem(hMenu, ID_EMULATOR_SMP0, (g_pBoard->IsSmpImageAttached(0) ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(hMenu, ID_EMULATOR_SMP1, (g_pBoard->IsSmpImageAttached(1) ? MF_CHECKED : MF_UNCHECKED));
    MainWindow_SetToolbarImage(ID_EMULATOR_SMP0,
            g_pBoard->IsSmpImageAttached(0) ? ToolbarImageCartridge : ToolbarImageCartSlot);
    MainWindow_SetToolbarImage(ID_EMULATOR_SMP1,
            g_pBoard->IsSmpImageAttached(1) ? ToolbarImageCartridge : ToolbarImageCartSlot);

    // Debug menu
    BOOL okDebug = Settings_GetDebug();
    CheckMenuItem(hMenu, ID_VIEW_DEBUG, (okDebug ? MF_CHECKED : MF_UNCHECKED));
    EnableMenuItem(hMenu, ID_DEBUG_STEPINTO, (okDebug ? MF_ENABLED : MF_DISABLED));
    EnableMenuItem(hMenu, ID_DEBUG_STEPOVER, (okDebug ? MF_ENABLED : MF_DISABLED));
    EnableMenuItem(hMenu, ID_DEBUG_CLEARCONSOLE, (okDebug ? MF_ENABLED : MF_DISABLED));
    EnableMenuItem(hMenu, ID_DEBUG_DELETEALLBREAKPTS, (okDebug ? MF_ENABLED : MF_DISABLED));
}

// Process menu command
// Returns: true - command was processed, false - command not found
bool MainWindow_DoCommand(int commandId)
{
    switch (commandId)
    {
    case IDM_ABOUT:
        ShowAboutBox();
        break;
    case IDM_EXIT:
        DestroyWindow(g_hwnd);
        break;
    case ID_VIEW_DEBUG:
        MainWindow_DoViewDebug();
        break;
    case ID_VIEW_MEMORYMAP:
        MainWindow_DoDebugMemoryMap();
        break;
    case ID_VIEW_TOOLBAR:
        MainWindow_DoViewToolbar();
        break;
    case ID_VIEW_SCREENMODE0:
    case ID_VIEW_SCREENMODE1:
    case ID_VIEW_SCREENMODE2:
    case ID_VIEW_SCREENMODE3:
    case ID_VIEW_SCREENMODE4:
        MainWindow_DoViewScreenMode(commandId - ID_VIEW_SCREENMODE0);
        break;
    case ID_VIEW_PALETTE0:
    case ID_VIEW_PALETTE1:
    case ID_VIEW_PALETTE2:
    case ID_VIEW_PALETTE3:
    case ID_VIEW_PALETTE4:
    case ID_VIEW_PALETTE5:
        MainWindow_DoViewScreenPalette(commandId - ID_VIEW_PALETTE0);
        break;
    case ID_EMULATOR_RUN:
        MainWindow_DoEmulatorRun();
        break;
    case ID_EMULATOR_AUTOSTART:
        MainWindow_DoEmulatorAutostart();
        break;
    case ID_DEBUG_STEPINTO:
        if (!g_okEmulatorRunning && Settings_GetDebug())
            ConsoleView_StepInto();
        break;
    case ID_DEBUG_STEPOVER:
        if (!g_okEmulatorRunning && Settings_GetDebug())
            ConsoleView_StepOver();
        break;
    case ID_DEBUG_CLEARCONSOLE:
        if (Settings_GetDebug())
            ConsoleView_ClearConsole();
        break;
    case ID_DEBUG_DELETEALLBREAKPTS:
        if (Settings_GetDebug())
            ConsoleView_DeleteAllBreakpoints();
        break;
    case ID_DEBUG_MEMORY_WORDBYTE:
        MemoryView_SwitchWordByte();
        break;
    case ID_DEBUG_MEMORY_GOTO:
        MemoryView_SelectAddress();
        break;
    case ID_EMULATOR_RESET:
        MainWindow_DoEmulatorReset();
        break;
    case ID_EMULATOR_SPEED25:
        MainWindow_DoEmulatorSpeed(0x7ffe);
        break;
    case ID_EMULATOR_SPEED50:
        MainWindow_DoEmulatorSpeed(0x7fff);
        break;
    case ID_EMULATOR_SPEEDMAX:
        MainWindow_DoEmulatorSpeed(0);
        break;
    case ID_EMULATOR_REALSPEED:
        MainWindow_DoEmulatorSpeed(1);
        break;
    case ID_EMULATOR_SPEED200:
        MainWindow_DoEmulatorSpeed(2);
        break;
    case ID_EMULATOR_SOUND:
        MainWindow_DoEmulatorSound();
        break;
    case ID_EMULATOR_SMP0:
        MainWindow_DoEmulatorSmp(0);
        break;
    case ID_EMULATOR_SMP1:
        MainWindow_DoEmulatorSmp(1);
        break;
    case ID_FILE_LOADSTATE:
        MainWindow_DoFileLoadState();
        break;
    case ID_FILE_SAVESTATE:
        MainWindow_DoFileSaveState();
        break;
    case ID_FILE_SCREENSHOT:
        MainWindow_DoFileScreenshot();
        break;
    case ID_FILE_SCREENSHOTTOCLIPBOARD:
        MainWindow_DoFileScreenshotToClipboard();
        break;
    case ID_FILE_SAVESCREENSHOTAS:
        MainWindow_DoFileScreenshotSaveAs();
        break;
    case ID_FILE_SETTINGS:
        MainWindow_DoFileSettings();
        break;
    case ID_FILE_SETTINGS_COLORS:
        MainWindow_DoFileSettingsColors();
        break;
    default:
        return false;
    }
    return true;
}

void MainWindow_DoViewDebug()
{
    MainWindow_DoViewScreenMode(1);  // Switch to short mode

    Settings_SetDebug(!Settings_GetDebug());
    MainWindow_ShowHideDebug();
}
void MainWindow_DoDebugMemoryMap()
{
    Settings_SetMemoryMap(!Settings_GetMemoryMap());
    MainWindow_ShowHideMemoryMap();
}
void MainWindow_DoViewToolbar()
{
    Settings_SetToolbar(!Settings_GetToolbar());
    MainWindow_ShowHideToolbar();
}

void MainWindow_DoViewScreenMode(int newMode)
{
    if (Settings_GetDebug() && newMode != 1) return;  // Deny switching to other mode in Debug mode

    ScreenView_SetScreenMode(newMode);

    MainWindow_AdjustWindowSize();
    MainWindow_AdjustWindowLayout();
    MainWindow_UpdateMenu();

    Settings_SetScreenViewMode(newMode);

    KeyboardView_RedrawKeyboard();  // Scale change affects keyboard view
}

void MainWindow_DoViewScreenPalette(int newPalette)
{
    ScreenView_SetScreenPalette(newPalette);

    MainWindow_UpdateMenu();

    Settings_SetScreenPalette(newPalette);
}

void MainWindow_DoEmulatorRun()
{
    if (g_okEmulatorRunning)
    {
        Emulator_Stop();
    }
    else
    {
        Emulator_Start();
    }
}
void MainWindow_DoEmulatorAutostart()
{
    Settings_SetAutostart(!Settings_GetAutostart());

    MainWindow_UpdateMenu();
}
void MainWindow_DoEmulatorReset()
{
    Emulator_Reset();
}
void MainWindow_DoEmulatorSpeed(WORD speed)
{
    Settings_SetRealSpeed(speed);

    MainWindow_UpdateMenu();
}
void MainWindow_DoEmulatorSound()
{
    Settings_SetSound(!Settings_GetSound());

    Emulator_SetSound(Settings_GetSound() != 0);

    MainWindow_UpdateMenu();
}

void MainWindow_DoFileLoadState()
{
    TCHAR bufFileName[MAX_PATH];
    BOOL okResult = ShowOpenDialog(g_hwnd,
            _T("Open state image to load"),
            _T("MK90 state images (*.mk90st)\0*.nmst\0All Files (*.*)\0*.*\0\0"),
            bufFileName);
    if (!okResult) return;

    if (!Emulator_LoadImage(bufFileName))
    {
        AlertWarning(_T("Failed to load image file."));
    }

    MainWindow_UpdateAllViews();
}

void MainWindow_DoFileSaveState()
{
    TCHAR bufFileName[MAX_PATH];
    BOOL okResult = ShowSaveDialog(g_hwnd,
            _T("Save state image as"),
            _T("MK90 state images (*.mk90st)\0*.nmst\0All Files (*.*)\0*.*\0\0"),
            _T("mk90st"),
            bufFileName);
    if (! okResult) return;

    if (!Emulator_SaveImage(bufFileName))
    {
        AlertWarning(_T("Failed to save image file."));
    }
}

void MainWindow_DoFileScreenshot()
{
    TCHAR bufFileName[MAX_PATH];
    SYSTEMTIME st;
    ::GetSystemTime(&st);
    _sntprintf(bufFileName, sizeof(bufFileName) / sizeof(TCHAR) - 1,
            _T("%04d%02d%02d%02d%02d%02d%03d.png"),
            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

    if (!ScreenView_SaveScreenshot(bufFileName))
    {
        AlertWarning(_T("Failed to save screenshot bitmap."));
    }
}

void MainWindow_DoFileScreenshotToClipboard()
{
    HGLOBAL hDIB = ScreenView_GetScreenshotAsDIB();
    if (hDIB != NULL)
    {
        ::OpenClipboard(g_hwnd);
        ::EmptyClipboard();
        ::SetClipboardData(CF_DIB, hDIB);
        ::CloseClipboard();
    }
}

void MainWindow_DoFileScreenshotSaveAs()
{
    TCHAR bufFileName[MAX_PATH];
    BOOL okResult = ShowSaveDialog(g_hwnd,
            _T("Save screenshot as"),
            _T("PNG bitmaps (*.png)\0*.png\0BMP bitmaps (*.bmp)\0*.bmp\0TIFF bitmaps (*.tiff)\0*.tiff\0All Files (*.*)\0*.*\0\0"),
            _T("png"),
            bufFileName);
    if (! okResult) return;

    if (!ScreenView_SaveScreenshot(bufFileName))
    {
        AlertWarning(_T("Failed to save screenshot image."));
    }
}

void MainWindow_DoFileSettings()
{
    ShowSettingsDialog();
}

void MainWindow_DoFileSettingsColors()
{
    if (ShowSettingsColorsDialog())
    {
        RedrawWindow(g_hwnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN);
    }
}

void MainWindow_DoEmulatorSmp(int slot)
{
    BOOL okLoaded = g_pBoard->IsSmpImageAttached(slot);
    if (okLoaded)
    {
        g_pBoard->DetachSmpImage(slot);
        Settings_SetSmpFilePath(slot, NULL);
    }
    else
    {
        // File Open dialog
        TCHAR bufFileName[MAX_PATH];
        BOOL okResult = ShowOpenDialog(g_hwnd,
                _T("Open SMP image to load"),
                _T("MK-90 SMP images (*.bin)\0*.bin\0All Files (*.*)\0*.*\0\0"),
                bufFileName);
        if (!okResult) return;

        if (!g_pBoard->AttachSmpImage(slot, bufFileName))
        {
            AlertWarning(_T("Failed to attach the SMP image."));
            return;
        }

        Settings_SetSmpFilePath(slot, bufFileName);
    }
    MainWindow_UpdateMenu();
}

void MainWindow_OnToolbarGetInfoTip(LPNMTBGETINFOTIP /*lpnm*/)
{
    //int commandId = lpnm->iItem;
}

void MainWindow_UpdateAllViews()
{
    // Update cached values in views
    Emulator_OnUpdate();
    DebugView_OnUpdate();
    DisasmView_OnUpdate();

    // Update screen
    InvalidateRect(g_hwndScreen, NULL, TRUE);

    // Update debug windows
    if (g_hwndDebug != NULL)
        InvalidateRect(g_hwndDebug, NULL, TRUE);
    if (g_hwndDisasm != NULL)
        InvalidateRect(g_hwndDisasm, NULL, TRUE);
    if (g_hwndMemory != NULL)
        InvalidateRect(g_hwndMemory, NULL, TRUE);
    if (g_hwndMemoryMap != NULL)
        InvalidateRect(g_hwndMemoryMap, NULL, TRUE);
}

void MainWindow_SetToolbarImage(int commandId, int imageIndex)
{
    TBBUTTONINFO info;
    info.cbSize = sizeof(info);
    info.iImage = imageIndex;
    info.dwMask = TBIF_IMAGE;
    SendMessage(m_hwndToolbar, TB_SETBUTTONINFO, commandId, (LPARAM) &info);
}

void MainWindow_EnableToolbarItem(int commandId, BOOL enable)
{
    TBBUTTONINFO info;
    info.cbSize = sizeof(info);
    info.fsState = enable ? TBSTATE_ENABLED : 0;
    info.dwMask = TBIF_STATE;
    SendMessage(m_hwndToolbar, TB_SETBUTTONINFO, commandId, (LPARAM)&info);
}

void MainWindow_SetStatusbarText(int part, LPCTSTR message)
{
    SendMessage(m_hwndStatusbar, SB_SETTEXT, part, (LPARAM) message);
}
void MainWindow_SetStatusbarBitmap(int part, UINT resourceId)
{
    SendMessage(m_hwndStatusbar, SB_SETTEXT, part | SBT_OWNERDRAW, (LPARAM) resourceId);
}
void MainWindow_SetStatusbarIcon(int part, HICON hIcon)
{
    SendMessage(m_hwndStatusbar, SB_SETICON, part, (LPARAM) hIcon);
}


//////////////////////////////////////////////////////////////////////
