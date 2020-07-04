/*  This file is part of MK90BTL.
    MK90BTL is free software: you can redistribute it and/or modify it under the terms
of the GNU Lesser General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.
    MK90BTL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Lesser General Public License for more details.
    You should have received a copy of the GNU Lesser General Public License along with
MK90BTL. If not, see <http://www.gnu.org/licenses/>. */

// Dialogs.cpp

#include "stdafx.h"
#include <commdlg.h>
#include <commctrl.h>
#include "Dialogs.h"
#include "Emulator.h"
#include "Main.h"
#include "emubase\Emubase.h"

//////////////////////////////////////////////////////////////////////


INT_PTR CALLBACK AboutBoxProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK InputBoxProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK SettingsProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DcbEditorProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

BOOL InputBoxValidate(HWND hDlg);

LPCTSTR m_strInputBoxTitle = NULL;
WORD* m_pInputBoxValueOctal = NULL;

DCB m_DialogSettings_SerialConfig;
DCB m_DialogSettings_NetComConfig;
DCB* m_pDcbEditorData = NULL;


//////////////////////////////////////////////////////////////////////
// About Box

void ShowAboutBox()
{
    DialogBox(g_hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), g_hwnd, AboutBoxProc);
}

INT_PTR CALLBACK AboutBoxProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        {
            TCHAR buf[64];
            wsprintf(buf, _T("%S %S"), __DATE__, __TIME__);
            ::SetWindowText(::GetDlgItem(hDlg, IDC_BUILDDATE), buf);
            return (INT_PTR)TRUE;
        }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}


//////////////////////////////////////////////////////////////////////


BOOL InputBoxOctal(HWND hwndOwner, LPCTSTR strTitle, WORD* pValue)
{
    m_strInputBoxTitle = strTitle;
    m_pInputBoxValueOctal = pValue;
    INT_PTR result = DialogBox(g_hInst, MAKEINTRESOURCE(IDD_INPUTBOX), hwndOwner, InputBoxProc);
    if (result != IDOK)
        return FALSE;

    return TRUE;
}


INT_PTR CALLBACK InputBoxProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        {
            SetWindowText(hDlg, m_strInputBoxTitle);
            HWND hEdit = GetDlgItem(hDlg, IDC_EDIT1);

            TCHAR buffer[8];
            _sntprintf_s(buffer, 8, _T("%06ho"), *m_pInputBoxValueOctal);
            SetWindowText(hEdit, buffer);
            SendMessage(hEdit, EM_SETSEL, 0, -1);

            SetFocus(hEdit);
            return (INT_PTR)FALSE;
        }
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_EDIT1:
            {
                TCHAR buffer[8];
                GetDlgItemText(hDlg, IDC_EDIT1, buffer, 8);
                if (_sntscanf_s(buffer, 8, _T("%ho"), m_pInputBoxValueOctal) > 0)
                {
                    GetDlgItemText(hDlg, IDC_EDIT2, buffer, 8);
                    WORD otherValue;
                    if (_sntscanf_s(buffer, 8, _T("%hx"), &otherValue) <= 0 || *m_pInputBoxValueOctal != otherValue)
                    {
                        _sntprintf_s(buffer, 8, _T("%04hx"), *m_pInputBoxValueOctal);
                        SetDlgItemText(hDlg, IDC_EDIT2, buffer);
                    }
                }
            }
            return (INT_PTR)TRUE;
        case IDC_EDIT2:
            {
                TCHAR buffer[8];
                GetDlgItemText(hDlg, IDC_EDIT2, buffer, 8);
                if (_sntscanf_s(buffer, 8, _T("%hx"), m_pInputBoxValueOctal) > 0)
                {
                    GetDlgItemText(hDlg, IDC_EDIT1, buffer, 8);
                    WORD otherValue;
                    if (_sntscanf_s(buffer, 8, _T("%ho"), &otherValue) <= 0 || *m_pInputBoxValueOctal != otherValue)
                    {
                        _sntprintf_s(buffer, 8, _T("%06ho"), *m_pInputBoxValueOctal);
                        SetDlgItemText(hDlg, IDC_EDIT1, buffer);
                    }
                }
            }
            return (INT_PTR)TRUE;
        case IDOK:
            if (! InputBoxValidate(hDlg))
                return (INT_PTR) FALSE;
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        default:
            return (INT_PTR) FALSE;
        }
        break;
    }
    return (INT_PTR) FALSE;
}

BOOL InputBoxValidate(HWND hDlg)
{
    HWND hEdit = GetDlgItem(hDlg, IDC_EDIT1);
    TCHAR buffer[8];
    GetWindowText(hEdit, buffer, 8);

    WORD value;
    if (! ParseOctalValue(buffer, &value))
    {
        MessageBox(NULL, _T("Please enter correct octal value."), _T("Input Box Validation"),
                MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
        return FALSE;
    }

    *m_pInputBoxValueOctal = value;

    return TRUE;
}


//////////////////////////////////////////////////////////////////////


BOOL ShowSaveDialog(HWND hwndOwner, LPCTSTR strTitle, LPCTSTR strFilter, LPCTSTR strDefExt, TCHAR* bufFileName)
{
    *bufFileName = 0;
    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwndOwner;
    ofn.hInstance = g_hInst;
    ofn.lpstrTitle = strTitle;
    ofn.lpstrFilter = strFilter;
    ofn.lpstrDefExt = strDefExt;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
    ofn.lpstrFile = bufFileName;
    ofn.nMaxFile = MAX_PATH;

    BOOL okResult = GetSaveFileName(&ofn);
    return okResult;
}

BOOL ShowOpenDialog(HWND hwndOwner, LPCTSTR strTitle, LPCTSTR strFilter, TCHAR* bufFileName)
{
    *bufFileName = 0;
    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwndOwner;
    ofn.hInstance = g_hInst;
    ofn.lpstrTitle = strTitle;
    ofn.lpstrFilter = strFilter;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    ofn.lpstrFile = bufFileName;
    ofn.nMaxFile = MAX_PATH;

    BOOL okResult = GetOpenFileName(&ofn);
    return okResult;
}


//////////////////////////////////////////////////////////////////////
// Settings Dialog

void ShowSettingsDialog()
{
    DialogBox(g_hInst, MAKEINTRESOURCE(IDD_SETTINGS), g_hwnd, SettingsProc);
}

int CALLBACK SettingsDialog_EnumFontProc(const LOGFONT* lpelfe, const TEXTMETRIC* /*lpntme*/, DWORD /*FontType*/, LPARAM lParam)
{
    if ((lpelfe->lfPitchAndFamily & FIXED_PITCH) == 0)
        return TRUE;
    if (lpelfe->lfFaceName[0] == _T('@'))  // Skip vertical fonts
        return TRUE;

    HWND hCombo = (HWND)lParam;

    int item = ::SendMessage(hCombo, CB_FINDSTRING, 0, (LPARAM)lpelfe->lfFaceName);
    if (item < 0)
        ::SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)lpelfe->lfFaceName);

    return TRUE;
}

void FillDebugFontCombo(HWND hCombo)
{
    LOGFONT logfont;  ZeroMemory(&logfont, sizeof logfont);
    logfont.lfCharSet = DEFAULT_CHARSET;
    logfont.lfWeight = FW_NORMAL;
    logfont.lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE;

    HDC hdc = GetDC(NULL);
    EnumFontFamiliesEx(hdc, &logfont, (FONTENUMPROC)SettingsDialog_EnumFontProc, (LPARAM)hCombo, 0);
    ReleaseDC(NULL, hdc);

    Settings_GetDebugFontName(logfont.lfFaceName);
    ::SendMessage(hCombo, CB_SELECTSTRING, 0, (LPARAM)logfont.lfFaceName);
}

INT_PTR CALLBACK SettingsProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        {
            HWND hVolume = GetDlgItem(hDlg, IDC_VOLUME);
            SendMessage(hVolume, TBM_SETRANGEMIN, 0, (LPARAM)0);
            SendMessage(hVolume, TBM_SETRANGEMAX, 0, (LPARAM)0xffff);
            SendMessage(hVolume, TBM_SETTICFREQ, 0x1000, 0);
            SendMessage(hVolume, TBM_SETPOS, TRUE, (LPARAM)Settings_GetSoundVolume());

            HWND hDebugFont = GetDlgItem(hDlg, IDC_DEBUGFONT);
            FillDebugFontCombo(hDebugFont);

            return (INT_PTR)FALSE;
        }
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            {
                HWND hVolume = GetDlgItem(hDlg, IDC_VOLUME);
                DWORD volume = SendMessage(hVolume, TBM_GETPOS, 0, 0);
                Settings_SetSoundVolume((WORD)volume);

                TCHAR buffer[32];

                GetDlgItemText(hDlg, IDC_DEBUGFONT, buffer, 32);
                Settings_SetDebugFontName(buffer);
            }

            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        case IDC_BUTTON1:
            {
                ShowSerialPortSettings(&m_DialogSettings_SerialConfig);
                SetFocus(GetDlgItem(hDlg, IDC_BUTTON1));
            }
            break;
        case IDC_BUTTON2:
            {
                ShowSerialPortSettings(&m_DialogSettings_NetComConfig);
                SetFocus(GetDlgItem(hDlg, IDC_BUTTON2));
            }
            break;
        default:
            return (INT_PTR)FALSE;
        }
        break;
    }
    return (INT_PTR) FALSE;
}


//////////////////////////////////////////////////////////////////////


void ShowSerialPortSettings(DCB * pDCB)
{
    m_pDcbEditorData = pDCB;
    DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DCB_EDITOR), g_hwnd, DcbEditorProc);
}

const DWORD BaudrateValues[] =
{
    300, 600, 1200, 2400, 4800, 9600, 14400, 19200, 38400, 57600, 115200, 128000, 256000
};

INT_PTR CALLBACK DcbEditorProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM /*lParam*/)
{
    switch (message)
    {
    case WM_INITDIALOG:
        {
            const DCB * pDCB = m_pDcbEditorData;

            HWND hBaudrate = GetDlgItem(hDlg, IDC_BAUDRATE);
            TCHAR buffer[10];
            int selindex = 5;  // 9600 by default
            for (int i = 0; i < sizeof(BaudrateValues) / sizeof(DWORD); i++)
            {
                wsprintf(buffer, _T("%lu"), BaudrateValues[i]);
                SendMessage(hBaudrate, LB_ADDSTRING, 0, (LPARAM)buffer);
                if (pDCB->BaudRate == BaudrateValues[i])
                    selindex = i;
            }
            SendMessage(hBaudrate, LB_SETCURSEL, (WPARAM)selindex, 0);

            HWND hParity = GetDlgItem(hDlg, IDC_PARITY);
            SendMessage(hParity, LB_ADDSTRING, 0, (LPARAM)_T("None"));
            SendMessage(hParity, LB_ADDSTRING, 0, (LPARAM)_T("Odd"));
            SendMessage(hParity, LB_ADDSTRING, 0, (LPARAM)_T("Even"));
            SendMessage(hParity, LB_ADDSTRING, 0, (LPARAM)_T("Mark"));
            SendMessage(hParity, LB_ADDSTRING, 0, (LPARAM)_T("Space"));

            HWND hStopBits = GetDlgItem(hDlg, IDC_STOPBITS);
            SendMessage(hStopBits, LB_ADDSTRING, 0, (LPARAM)_T("1"));
            SendMessage(hStopBits, LB_ADDSTRING, 0, (LPARAM)_T("1.5"));
            SendMessage(hStopBits, LB_ADDSTRING, 0, (LPARAM)_T("2"));

            HWND hDtrControl = GetDlgItem(hDlg, IDC_DTRCONTROL);
            SendMessage(hDtrControl, LB_ADDSTRING, 0, (LPARAM)_T("Disable"));
            SendMessage(hDtrControl, LB_ADDSTRING, 0, (LPARAM)_T("Enable"));
            SendMessage(hDtrControl, LB_ADDSTRING, 0, (LPARAM)_T("Handshake"));

            HWND hRtsControl = GetDlgItem(hDlg, IDC_RTSCONTROL);
            SendMessage(hRtsControl, LB_ADDSTRING, 0, (LPARAM)_T("Disable"));
            SendMessage(hRtsControl, LB_ADDSTRING, 0, (LPARAM)_T("Enable"));
            SendMessage(hRtsControl, LB_ADDSTRING, 0, (LPARAM)_T("Handshake"));
            SendMessage(hRtsControl, LB_ADDSTRING, 0, (LPARAM)_T("Toggle"));

            SendMessage(hParity, LB_SETCURSEL, (WPARAM)pDCB->Parity, 0);
            SendMessage(hStopBits, LB_SETCURSEL, (WPARAM)pDCB->StopBits, 0);
            SendMessage(hDtrControl, LB_SETCURSEL, (WPARAM)pDCB->fDtrControl, 0);
            SendMessage(hRtsControl, LB_SETCURSEL, (WPARAM)pDCB->fRtsControl, 0);
            CheckDlgButton(hDlg, IDC_OUTXCTSFLOW, pDCB->fOutxCtsFlow);
            CheckDlgButton(hDlg, IDC_OUTXDSRFLOW, pDCB->fOutxDsrFlow);
            CheckDlgButton(hDlg, IDC_DSRSENSITIVITY, pDCB->fDsrSensitivity);

            return (INT_PTR)FALSE;
        }
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            {
                DCB * pDCB = m_pDcbEditorData;

                HWND hBaudrate = GetDlgItem(hDlg, IDC_BAUDRATE);
                int baudrateIndex = SendMessage(hBaudrate, LB_GETCURSEL, 0, 0);
                if (baudrateIndex >= sizeof(BaudrateValues) / sizeof(DWORD))
                    baudrateIndex = 5;  // 9600 by default
                pDCB->BaudRate = BaudrateValues[baudrateIndex];
                HWND hParity = GetDlgItem(hDlg, IDC_PARITY);
                pDCB->Parity = (BYTE)SendMessage(hParity, LB_GETCURSEL, 0, 0);
                HWND hStopBits = GetDlgItem(hDlg, IDC_STOPBITS);
                pDCB->StopBits = (BYTE)SendMessage(hStopBits, LB_GETCURSEL, 0, 0);
                HWND hDtrControl = GetDlgItem(hDlg, IDC_DTRCONTROL);
                pDCB->fDtrControl = (BYTE)SendMessage(hDtrControl, LB_GETCURSEL, 0, 0);
                HWND hRtsControl = GetDlgItem(hDlg, IDC_RTSCONTROL);
                pDCB->fRtsControl = (BYTE)SendMessage(hRtsControl, LB_GETCURSEL, 0, 0);
                pDCB->fOutxCtsFlow = IsDlgButtonChecked(hDlg, IDC_OUTXCTSFLOW);
                pDCB->fOutxDsrFlow = IsDlgButtonChecked(hDlg, IDC_OUTXDSRFLOW);
                pDCB->fDsrSensitivity = IsDlgButtonChecked(hDlg, IDC_DSRSENSITIVITY);
            }

            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        default:
            return (INT_PTR)FALSE;
        }
        break;
    }
    return (INT_PTR) FALSE;
}


//////////////////////////////////////////////////////////////////////

