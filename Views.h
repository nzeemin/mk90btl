/*  This file is part of MK90BTL.
    MK90BTL is free software: you can redistribute it and/or modify it under the terms
of the GNU Lesser General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.
    MK90BTL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Lesser General Public License for more details.
    You should have received a copy of the GNU Lesser General Public License along with
MK90BTL. If not, see <http://www.gnu.org/licenses/>. */

// Views.h
// Defines for all views of the application

#pragma once

//////////////////////////////////////////////////////////////////////
// Window class names

const LPCTSTR CLASSNAME_SCREENVIEW      = _T("MK90BTLSCREEN");
const LPCTSTR CLASSNAME_KEYBOARDVIEW    = _T("MK90BTLKEYBOARD");
const LPCTSTR CLASSNAME_DEBUGVIEW       = _T("MK90BTLDEBUG");
const LPCTSTR CLASSNAME_DISASMVIEW      = _T("MK90BTLDISASM");
const LPCTSTR CLASSNAME_MEMORYVIEW      = _T("MK90BTLMEMORY");
const LPCTSTR CLASSNAME_MEMORYMAPVIEW   = _T("MK90BTLMEMORYMAP");
const LPCTSTR CLASSNAME_CONSOLEVIEW     = _T("MK90BTLCONSOLE");


//////////////////////////////////////////////////////////////////////
// ScreenView

extern HWND g_hwndScreen;  // Screen View window handle

void ScreenView_RegisterClass();
void ScreenView_Init();
void ScreenView_Done();
int ScreenView_GetScreenMode();
void ScreenView_SetScreenMode(int);
int ScreenView_GetScreenPalette();
void ScreenView_SetScreenPalette(int);
void ScreenView_PrepareScreen();
void ScreenView_ScanKeyboard();
void ScreenView_ProcessKeyboard();
void ScreenView_RedrawScreen();  // Force to call PrepareScreen and to draw the image
void ScreenView_Create(HWND hwndParent, int x, int y);
void ScreenView_GetDesiredSize(SIZE& size);
LRESULT CALLBACK ScreenViewWndProc(HWND, UINT, WPARAM, LPARAM);
BOOL ScreenView_SaveScreenshot(LPCTSTR sFileName);
void ScreenView_KeyEvent(BYTE keyscan, BOOL pressed);


//////////////////////////////////////////////////////////////////////
// KeyboardView

extern HWND g_hwndKeyboard;  // Keyboard View window handle

void KeyboardView_RegisterClass();
void KeyboardView_Init();
void KeyboardView_Done();
void KeyboardView_Create(HWND hwndParent, int x, int y, int width, int height);
LRESULT CALLBACK KeyboardViewWndProc(HWND, UINT, WPARAM, LPARAM);


//////////////////////////////////////////////////////////////////////
// DebugView

extern HWND g_hwndDebug;  // Debug View window handle

void DebugView_RegisterClass();
void DebugView_Init();
void DebugView_Create(HWND hwndParent, int x, int y, int width, int height);
LRESULT CALLBACK DebugViewWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK DebugViewViewerWndProc(HWND, UINT, WPARAM, LPARAM);
void DebugView_OnUpdate();
BOOL DebugView_IsRegisterChanged(int regno);


//////////////////////////////////////////////////////////////////////
// DisasmView

extern HWND g_hwndDisasm;  // Disasm View window handle

void DisasmView_RegisterClass();
void DisasmView_Create(HWND hwndParent, int x, int y, int width, int height);
LRESULT CALLBACK DisasmViewWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK DisasmViewViewerWndProc(HWND, UINT, WPARAM, LPARAM);
void DisasmView_OnUpdate();
void DisasmView_SetCurrentProc(BOOL okCPU);


//////////////////////////////////////////////////////////////////////
// MemoryView

extern HWND g_hwndMemory;  // Memory view window handler

void MemoryView_RegisterClass();
void MemoryView_Create(HWND hwndParent, int x, int y, int width, int height);
LRESULT CALLBACK MemoryViewWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK MemoryViewViewerWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void MemoryView_SwitchWordByte();
void MemoryView_SelectAddress();


//////////////////////////////////////////////////////////////////////
// MemoryMapView

extern HWND g_hwndMemoryMap;  // MemoryMap view window handler

void MemoryMapView_RegisterClass();
void MemoryMapView_Create(HWND hwndParent, int x, int y);
LRESULT CALLBACK MemoryMapViewWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK MemoryMapViewViewerWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void MemoryMapView_RedrawMap();


//////////////////////////////////////////////////////////////////////
// ConsoleView

extern HWND g_hwndConsole;  // Console View window handle

void ConsoleView_RegisterClass();
void ConsoleView_Create(HWND hwndParent, int x, int y, int width, int height);
LRESULT CALLBACK ConsoleViewWndProc(HWND, UINT, WPARAM, LPARAM);
void ConsoleView_PrintFormat(LPCTSTR pszFormat, ...);
void ConsoleView_Print(LPCTSTR message);
void ConsoleView_Activate();
void ConsoleView_StepInto();
void ConsoleView_StepOver();


//////////////////////////////////////////////////////////////////////