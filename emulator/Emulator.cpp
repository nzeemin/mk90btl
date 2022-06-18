/*  This file is part of MK90BTL.
    MK90BTL is free software: you can redistribute it and/or modify it under the terms
of the GNU Lesser General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.
    MK90BTL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Lesser General Public License for more details.
    You should have received a copy of the GNU Lesser General Public License along with
MK90BTL. If not, see <http://www.gnu.org/licenses/>. */

// Emulator.cpp

#include "stdafx.h"
#include <stdio.h>
#include <Share.h>
#include "Main.h"
#include "Emulator.h"
#include "Views.h"
#include "emubase\Emubase.h"
#include "SoundGen.h"

//////////////////////////////////////////////////////////////////////


CMotherboard* g_pBoard = nullptr;
int g_nEmulatorConfiguration;  // Current configuration
bool g_okEmulatorRunning = false;

int m_wEmulatorCPUBpsCount = 0;
uint16_t m_EmulatorCPUBps[MAX_BREAKPOINTCOUNT + 1];
uint16_t m_wEmulatorTempCPUBreakpoint = 0177777;
int m_wEmulatorWatchesCount = 0;
uint16_t m_EmulatorWatches[MAX_BREAKPOINTCOUNT + 1];

bool m_okEmulatorSound = false;
uint16_t m_wEmulatorSoundSpeed = 100;
bool m_okEmulatorCovox = false;

long m_nFrameCount = 0;
uint32_t m_dwTickCount = 0;
uint32_t m_dwEmulatorUptime = 0;  // Machine uptime, seconds, from turn on or reset, increments every 25 frames
long m_nUptimeFrameCount = 0;

uint8_t* g_pEmulatorRam = nullptr;  // RAM values - for change tracking
uint8_t* g_pEmulatorChangedRam = nullptr;  // RAM change flags
uint16_t g_wEmulatorCpuPC = 0177777;      // Current PC value
uint16_t g_wEmulatorPrevCpuPC = 0177777;  // Previous PC value


void CALLBACK Emulator_SoundGenCallback(unsigned short L, unsigned short R);

//////////////////////////////////////////////////////////////////////
//Прототип функции преобразования экрана
// Input:
//   pVideoBuffer   Исходные данные, биты экрана
//   pPalette       Палитра
//   pImageBits     Результат, 32-битный цвет, размер для каждой функции свой
typedef void (CALLBACK* PREPARE_SCREEN_CALLBACK)(const uint8_t* pVideoBuffer, const uint32_t* pPalette, void* pImageBits);

void CALLBACK Emulator_PrepareScreen360x192(const uint8_t* pVideoBuffer, const uint32_t* palette, void* pImageBits);
void CALLBACK Emulator_PrepareScreen480x256(const uint8_t* pVideoBuffer, const uint32_t* palette, void* pImageBits);
void CALLBACK Emulator_PrepareScreen600x320(const uint8_t* pVideoBuffer, const uint32_t* palette, void* pImageBits);
void CALLBACK Emulator_PrepareScreen720x384(const uint8_t* pVideoBuffer, const uint32_t* palette, void* pImageBits);
void CALLBACK Emulator_PrepareScreen960x512(const uint8_t* pVideoBuffer, const uint32_t* palette, void* pImageBits);

struct ScreenModeStruct
{
    int width;
    int height;
    PREPARE_SCREEN_CALLBACK callback;
    int scale;
}
static ScreenModeReference[] =
{
    { 360, 192, Emulator_PrepareScreen360x192, 3 },
    { 480, 256, Emulator_PrepareScreen480x256, 4 },
    { 600, 320, Emulator_PrepareScreen600x320, 5 },
    { 720, 384, Emulator_PrepareScreen720x384, 6 },
    { 960, 512, Emulator_PrepareScreen960x512, 8 },
};

// Palette colors:
// #0 - inactive pixel
// #1 - active pixel
// #2 - active pixel edge, used to separate active pixels
// #3 - background are outside of the pixels, a little lighter than an inactive pixel
const uint32_t ScreenView_Palette[][4] =
{
    { 0xB0B0B0, 0x000000, 0xA0A0A0, 0xC0C0C0 },  // Gray with pixel edges
    { 0xB0B0B0, 0x000000, 0x000000, 0xC0C0C0 },  // Gray, no pixel edges
    { 0xFFFFFF, 0x000000, 0xA0A0A0, 0xFFFFFF },  // High contrast black & white with pixel edges
    { 0xFFFFFF, 0x000000, 0x000000, 0xFFFFFF },  // High contrast black & white, no pixel edges
    { 0xA9C5A0, 0x7D9484, 0xA0A0A0, 0xAAC59B },  // Green with pixel edges
    { 0xA9C5A0, 0x7D9484, 0x7D9484, 0xAAC59B },  // Green, no pixel edges
};


//////////////////////////////////////////////////////////////////////


const LPCTSTR FILENAME_ROM_BASIC10 = _T("basic10.rom");
const LPCTSTR FILENAME_ROM_BASIC20 = _T("basic20.rom");


//////////////////////////////////////////////////////////////////////

bool Emulator_LoadRomFile(LPCTSTR strFileName, uint8_t* buffer, uint32_t fileOffset, uint32_t bytesToRead)
{
    FILE* fpRomFile = ::_tfsopen(strFileName, _T("rb"), _SH_DENYWR);
    if (fpRomFile == nullptr)
        return false;

    ::memset(buffer, 0, bytesToRead);

    if (fileOffset > 0)
    {
        ::fseek(fpRomFile, fileOffset, SEEK_SET);
    }

    uint32_t dwBytesRead = ::fread(buffer, 1, bytesToRead, fpRomFile);
    if (dwBytesRead != bytesToRead)
    {
        ::fclose(fpRomFile);
        return false;
    }

    ::fclose(fpRomFile);

    return true;
}

bool Emulator_Init()
{
    ASSERT(g_pBoard == nullptr);

    CProcessor::Init();

    m_wEmulatorCPUBpsCount = 0;
    for (int i = 0; i <= MAX_BREAKPOINTCOUNT; i++)
    {
        m_EmulatorCPUBps[i] = 0177777;
    }
    m_wEmulatorWatchesCount = 0;
    for (int i = 0; i <= MAX_WATCHPOINTCOUNT; i++)
    {
        m_EmulatorWatches[i] = 0177777;
    }

    g_pBoard = new CMotherboard();

    // Allocate memory for old RAM values
    g_pEmulatorRam = (uint8_t*) ::calloc(65536, 1);
    g_pEmulatorChangedRam = (uint8_t*) ::calloc(65536, 1);

    g_pBoard->Reset();

    if (m_okEmulatorSound)
    {
        SoundGen_Initialize(Settings_GetSoundVolume());
        g_pBoard->SetSoundGenCallback(Emulator_SoundGenCallback);
    }

    return true;
}

void Emulator_Done()
{
    ASSERT(g_pBoard != nullptr);

    CProcessor::Done();

    g_pBoard->SetSoundGenCallback(nullptr);
    SoundGen_Finalize();

    delete g_pBoard;
    g_pBoard = nullptr;

    // Free memory used for old RAM values
    ::free(g_pEmulatorRam);
    ::free(g_pEmulatorChangedRam);
}

bool Emulator_InitConfiguration(uint16_t configuration)
{
    g_pBoard->SetConfiguration(configuration);

    LPCTSTR szRomFileName = nullptr;
    uint16_t nRomResourceId;
    switch (configuration)
    {
    case EMU_CONF_BASIC10:
        szRomFileName = FILENAME_ROM_BASIC10;
        nRomResourceId = IDR_MK90_ROM_BASIC10;
        break;
    case EMU_CONF_BASIC20:
        szRomFileName = FILENAME_ROM_BASIC20;
        nRomResourceId = IDR_MK90_ROM_BASIC20;
        break;
    default:
        szRomFileName = FILENAME_ROM_BASIC10;
        nRomResourceId = IDR_MK90_ROM_BASIC10;
        break;
    }

    uint8_t buffer[32768];//TODO: allocate on the heap

    // Load ROM file
    if (!Emulator_LoadRomFile(szRomFileName, buffer, 0, 32768))
    {
        // ROM file not found or failed to load, load the ROM from resource instead
        HRSRC hRes = NULL;
        uint32_t dwDataSize = 0;
        HGLOBAL hResLoaded = NULL;
        void * pResData = nullptr;
        if ((hRes = ::FindResource(NULL, MAKEINTRESOURCE(nRomResourceId), _T("BIN"))) == NULL ||
            (dwDataSize = ::SizeofResource(NULL, hRes)) < 32768 ||
            (hResLoaded = ::LoadResource(NULL, hRes)) == NULL ||
            (pResData = ::LockResource(hResLoaded)) == NULL)
        {
            AlertWarning(_T("Failed to load the ROM."));
            return false;
        }
        ::memcpy(buffer, pResData, 32768);
    }
    g_pBoard->LoadROM(buffer);

    g_nEmulatorConfiguration = configuration;

    g_pBoard->Reset();

    m_nUptimeFrameCount = 0;
    m_dwEmulatorUptime = 0;

    return true;
}

void Emulator_Start()
{
    g_okEmulatorRunning = true;

    // Set title bar text
    MainWindow_UpdateWindowTitle();
    MainWindow_UpdateMenu();

    m_nFrameCount = 0;
    m_dwTickCount = GetTickCount();

    // For proper breakpoint processing
    if (m_wEmulatorCPUBpsCount != 0)
    {
        g_pBoard->GetCPU()->ClearInternalTick();
    }
}
void Emulator_Stop()
{
    g_okEmulatorRunning = false;

    Emulator_SetTempCPUBreakpoint(0177777);

    // Reset title bar message
    MainWindow_UpdateWindowTitle();
    MainWindow_UpdateMenu();

    // Reset FPS indicator
    MainWindow_SetStatusbarText(StatusbarPartFPS, nullptr);

    MainWindow_UpdateAllViews();
}

void Emulator_Reset()
{
    ASSERT(g_pBoard != nullptr);

    g_pBoard->Reset();

    m_nUptimeFrameCount = 0;
    m_dwEmulatorUptime = 0;

    MainWindow_UpdateAllViews();
}

bool Emulator_AddCPUBreakpoint(uint16_t address)
{
    if (m_wEmulatorCPUBpsCount == MAX_BREAKPOINTCOUNT - 1 || address == 0177777)
        return false;
    for (int i = 0; i < m_wEmulatorCPUBpsCount; i++)  // Check if the BP exists
    {
        if (m_EmulatorCPUBps[i] == address)
            return false;  // Already in the list
    }
    for (int i = 0; i < MAX_BREAKPOINTCOUNT; i++)  // Put in the first empty cell
    {
        if (m_EmulatorCPUBps[i] == 0177777)
        {
            m_EmulatorCPUBps[i] = address;
            break;
        }
    }
    m_wEmulatorCPUBpsCount++;
    return true;
}
bool Emulator_RemoveCPUBreakpoint(uint16_t address)
{
    if (m_wEmulatorCPUBpsCount == 0 || address == 0177777)
        return false;
    for (int i = 0; i < MAX_BREAKPOINTCOUNT; i++)
    {
        if (m_EmulatorCPUBps[i] == address)
        {
            m_EmulatorCPUBps[i] = 0177777;
            m_wEmulatorCPUBpsCount--;
            if (m_wEmulatorCPUBpsCount > i)  // fill the hole
            {
                m_EmulatorCPUBps[i] = m_EmulatorCPUBps[m_wEmulatorCPUBpsCount];
                m_EmulatorCPUBps[m_wEmulatorCPUBpsCount] = 0177777;
            }
            return true;
        }
    }
    return false;
}
void Emulator_SetTempCPUBreakpoint(uint16_t address)
{
    if (m_wEmulatorTempCPUBreakpoint != 0177777)
        Emulator_RemoveCPUBreakpoint(m_wEmulatorTempCPUBreakpoint);
    if (address == 0177777)
    {
        m_wEmulatorTempCPUBreakpoint = 0177777;
        return;
    }
    for (int i = 0; i < MAX_BREAKPOINTCOUNT; i++)
    {
        if (m_EmulatorCPUBps[i] == address)
            return;  // We have regular breakpoint with the same address
    }
    m_wEmulatorTempCPUBreakpoint = address;
    m_EmulatorCPUBps[m_wEmulatorCPUBpsCount] = address;
    m_wEmulatorCPUBpsCount++;
}
const uint16_t* Emulator_GetCPUBreakpointList() { return m_EmulatorCPUBps; }
bool Emulator_IsBreakpoint()
{
    uint16_t address = g_pBoard->GetCPU()->GetPC();
    if (m_wEmulatorCPUBpsCount > 0)
    {
        for (int i = 0; i < m_wEmulatorCPUBpsCount; i++)
        {
            if (address == m_EmulatorCPUBps[i])
                return true;
        }
    }
    return false;
}
bool Emulator_IsBreakpoint(uint16_t address)
{
    if (m_wEmulatorCPUBpsCount == 0)
        return false;
    for (int i = 0; i < m_wEmulatorCPUBpsCount; i++)
    {
        if (address == m_EmulatorCPUBps[i])
            return true;
    }
    return false;
}
void Emulator_RemoveAllBreakpoints()
{
    for (int i = 0; i < MAX_BREAKPOINTCOUNT; i++)
        m_EmulatorCPUBps[i] = 0177777;
    m_wEmulatorCPUBpsCount = 0;
}

bool Emulator_AddWatchpoint(uint16_t address)
{
    if (m_wEmulatorWatchesCount == MAX_WATCHPOINTCOUNT - 1 || address == 0177777)
        return false;
    for (int i = 0; i < m_wEmulatorWatchesCount; i++)  // Check if the BP exists
    {
        if (m_EmulatorWatches[i] == address)
            return false;  // Already in the list
    }
    for (int i = 0; i < MAX_BREAKPOINTCOUNT; i++)  // Put in the first empty cell
    {
        if (m_EmulatorWatches[i] == 0177777)
        {
            m_EmulatorWatches[i] = address;
            break;
        }
    }
    m_wEmulatorWatchesCount++;
    return true;
}
const uint16_t* Emulator_GetWatchpointList() { return m_EmulatorWatches; }
bool Emulator_RemoveWatchpoint(uint16_t address)
{
    if (m_wEmulatorWatchesCount == 0 || address == 0177777)
        return false;
    for (int i = 0; i < MAX_WATCHPOINTCOUNT; i++)
    {
        if (m_EmulatorWatches[i] == address)
        {
            m_EmulatorWatches[i] = 0177777;
            m_wEmulatorWatchesCount--;
            if (m_wEmulatorWatchesCount > i)  // fill the hole
            {
                m_EmulatorWatches[i] = m_EmulatorWatches[m_wEmulatorWatchesCount];
                m_EmulatorWatches[m_wEmulatorWatchesCount] = 0177777;
            }
            return true;
        }
    }
    return false;
}
void Emulator_RemoveAllWatchpoints()
{
    for (int i = 0; i < MAX_WATCHPOINTCOUNT; i++)
        m_EmulatorWatches[i] = 0177777;
    m_wEmulatorWatchesCount = 0;
}

void Emulator_SetSpeed(uint16_t realspeed)
{
    uint16_t speedpercent = 100;
    switch (realspeed)
    {
    case 0: speedpercent = 200; break;
    case 1: speedpercent = 100; break;
    case 2: speedpercent = 200; break;
    case 0x7fff: speedpercent = 50; break;
    case 0x7ffe: speedpercent = 25; break;
    default: speedpercent = 100; break;
    }
    m_wEmulatorSoundSpeed = speedpercent;

    if (m_okEmulatorSound)
        SoundGen_SetSpeed(m_wEmulatorSoundSpeed);
}

void Emulator_SetSound(bool soundOnOff)
{
    if (m_okEmulatorSound != soundOnOff)
    {
        if (soundOnOff)
        {
            SoundGen_Initialize(Settings_GetSoundVolume());
            SoundGen_SetSpeed(m_wEmulatorSoundSpeed);
            g_pBoard->SetSoundGenCallback(Emulator_SoundGenCallback);
        }
        else
        {
            g_pBoard->SetSoundGenCallback(nullptr);
            SoundGen_Finalize();
        }
    }

    m_okEmulatorSound = soundOnOff;
}

bool Emulator_SystemFrame()
{
    g_pBoard->SetCPUBreakpoints(m_wEmulatorCPUBpsCount > 0 ? m_EmulatorCPUBps : nullptr);

    ScreenView_ScanKeyboard();
    ScreenView_ProcessKeyboard();

    if (!g_pBoard->SystemFrame())
        return false;

    // Calculate frames per second
    m_nFrameCount++;
    uint32_t dwCurrentTicks = GetTickCount();
    long nTicksElapsed = dwCurrentTicks - m_dwTickCount;
    if (nTicksElapsed >= 1200)
    {
        double dFramesPerSecond = m_nFrameCount * 1000.0 / nTicksElapsed;
        double dSpeed = dFramesPerSecond / 25.0 * 100;
        TCHAR buffer[16];
        _sntprintf(buffer, sizeof(buffer) / sizeof(TCHAR) - 1, _T("%03.f%%"), dSpeed);
        MainWindow_SetStatusbarText(StatusbarPartFPS, buffer);

        m_nFrameCount = 0;
        m_dwTickCount = dwCurrentTicks;
    }

    // Calculate emulator uptime (25 frames per second)
    m_nUptimeFrameCount++;
    if (m_nUptimeFrameCount >= 25)
    {
        m_dwEmulatorUptime++;
        m_nUptimeFrameCount = 0;

        int seconds = (int) (m_dwEmulatorUptime % 60);
        int minutes = (int) (m_dwEmulatorUptime / 60 % 60);
        int hours   = (int) (m_dwEmulatorUptime / 3600 % 60);

        TCHAR buffer[20];
        _sntprintf(buffer, sizeof(buffer) / sizeof(TCHAR) - 1, _T("Uptime: %02d:%02d:%02d"), hours, minutes, seconds);
        MainWindow_SetStatusbarText(StatusbarPartUptime, buffer);
    }

    // Auto-boot option processing
    if (Option_AutoBoot >= 0)
    {
        //TODO
    }

    return true;
}

void CALLBACK Emulator_SoundGenCallback(unsigned short L, unsigned short R)
{
    SoundGen_FeedDAC(L, R);
}

// Update cached values after Run or Step
void Emulator_OnUpdate()
{
    // Update stored PC value
    g_wEmulatorPrevCpuPC = g_wEmulatorCpuPC;
    g_wEmulatorCpuPC = g_pBoard->GetCPU()->GetPC();

    // Update memory change flags
    {
        uint8_t* pOld = g_pEmulatorRam;
        uint8_t* pChanged = g_pEmulatorChangedRam;
        uint16_t addr = 0;
        do
        {
            uint8_t newvalue = g_pBoard->GetRAMByte(addr);
            uint8_t oldvalue = *pOld;
            *pChanged = (newvalue != oldvalue) ? 255 : 0;
            *pOld = newvalue;
            addr++;
            pOld++;  pChanged++;
        }
        while (addr < 65535);
    }
}

// Get RAM change flag
//   addrtype - address mode - see ADDRTYPE_XXX constants
uint16_t Emulator_GetChangeRamStatus(uint16_t address)
{
    return *((uint16_t*)(g_pEmulatorChangedRam + address));
}

int Emulator_GetScreenScale(int scrmode)
{
    if (scrmode < 0 || scrmode >= sizeof(ScreenModeReference) / sizeof(ScreenModeStruct))
        return 3;
    ScreenModeStruct* pinfo = ScreenModeReference + scrmode;
    return pinfo->scale;
}

void Emulator_GetScreenSize(int scrmode, int* pwid, int* phei)
{
    if (scrmode < 0 || scrmode >= sizeof(ScreenModeReference) / sizeof(ScreenModeStruct))
        return;
    ScreenModeStruct* pinfo = ScreenModeReference + scrmode;
    *pwid = pinfo->width;
    *phei = pinfo->height;
}

void Emulator_PrepareScreenRGB32(void* pImageBits, int screenMode, int palette)
{
    if (pImageBits == nullptr) return;

    const uint8_t* pVideoBuffer = g_pBoard->GetVideoBuffer();
    ASSERT(pVideoBuffer != nullptr);

    const uint32_t * pPalette = Emulator_GetPalette(palette);

    // Render to bitmap
    PREPARE_SCREEN_CALLBACK callback = ScreenModeReference[screenMode].callback;
    callback(pVideoBuffer, pPalette, pImageBits);
}

const uint32_t * Emulator_GetPalette(int palette)
{
    return ScreenView_Palette[palette];
}


//////////////////////////////////////////////////////////////////////

//#define AVERAGERGB(a, b)  ( (((a) & 0xfefefeffUL) + ((b) & 0xfefefeffUL)) >> 1 )

#define PREPARE_SCREEN_BEGIN(SCALE) \
    const int scale = SCALE; \
    const int width = 120 * scale; \
    for (int y = 0; y < 64; y++) { \
        const uint8_t* pVideo = (pVideoBuffer + (y & 31) * 30) + (y >> 5); \
        uint32_t* pBits1 = (uint32_t*)pImageBits + (64 - 1 - y) * scale * width;
#define PREPARE_SCREEN_MIDDLE() \
        for (int col = 0; col < 15; col++) { \
            uint8_t src = *pVideo; \
            for (int bit = 0; bit < 8; bit++) { \
                int colorindex = (src & 0x80) >> 7; \
                uint32_t color = palette[colorindex];
#define PREPARE_SCREEN_END() \
                src = src << 1; \
            } \
            pVideo += 2; \
        } \
    }

void CALLBACK Emulator_PrepareScreen360x192(const uint8_t* pVideoBuffer, const uint32_t* palette, void* pImageBits)
{
    PREPARE_SCREEN_BEGIN(3)
    uint32_t* pBits2 = pBits1 + width;
    uint32_t* pBits3 = pBits2 + width;
    PREPARE_SCREEN_MIDDLE()
    *pBits1++ = *pBits2++ = *pBits3++ = color;
    *pBits1++ = *pBits2++ = *pBits3++ = color;
    *pBits1++ = *pBits2++ = *pBits3++ = color;
    PREPARE_SCREEN_END()
}

void CALLBACK Emulator_PrepareScreen480x256(const uint8_t* pVideoBuffer, const uint32_t* palette, void* pImageBits)
{
    PREPARE_SCREEN_BEGIN(4)
    uint32_t* pBits2 = pBits1 + width;
    uint32_t* pBits3 = pBits2 + width;
    uint32_t* pBits4 = pBits3 + width;
    PREPARE_SCREEN_MIDDLE()
    uint32_t color2 = palette[colorindex << 1];
    *pBits1++ = color2; *pBits2++ = *pBits3++ = *pBits4++ = color;
    *pBits1++ = *pBits2++ = *pBits3++ = *pBits4++ = color;
    *pBits1++ = *pBits2++ = *pBits3++ = *pBits4++ = color;
    *pBits1++ = *pBits2++ = *pBits3++ = *pBits4++ = color;
    PREPARE_SCREEN_END()
}

void CALLBACK Emulator_PrepareScreen600x320(const uint8_t* pVideoBuffer, const uint32_t* palette, void* pImageBits)
{
    PREPARE_SCREEN_BEGIN(5)
    uint32_t* pBits2 = pBits1 + width;
    uint32_t* pBits3 = pBits2 + width;
    uint32_t* pBits4 = pBits3 + width;
    uint32_t* pBits5 = pBits4 + width;
    PREPARE_SCREEN_MIDDLE()
    uint32_t color2 = palette[colorindex << 1];
    *pBits1++ = *pBits2++ = *pBits3++ = *pBits4++ = color; *pBits5++ = color2;
    *pBits1++ = *pBits2++ = *pBits3++ = *pBits4++ = color; *pBits5++ = color2;
    *pBits1++ = *pBits2++ = *pBits3++ = *pBits4++ = color; *pBits5++ = color2;
    *pBits1++ = *pBits2++ = *pBits3++ = *pBits4++ = color; *pBits5++ = color2;
    *pBits1++ = *pBits2++ = *pBits3++ = *pBits4++ = color2; *pBits5++ = color2;
    PREPARE_SCREEN_END()
}

void CALLBACK Emulator_PrepareScreen720x384(const uint8_t* pVideoBuffer, const uint32_t* palette, void* pImageBits)
{
    PREPARE_SCREEN_BEGIN(6)
    uint32_t* pBits2 = pBits1 + width;
    uint32_t* pBits3 = pBits2 + width;
    uint32_t* pBits4 = pBits3 + width;
    uint32_t* pBits5 = pBits4 + width;
    uint32_t* pBits6 = pBits5 + width;
    PREPARE_SCREEN_MIDDLE()
    uint32_t color2 = palette[colorindex << 1];
    *pBits1++ = *pBits2++ = *pBits3++ = *pBits4++ = *pBits5++ = color; *pBits6++ = color2;
    *pBits1++ = *pBits2++ = *pBits3++ = *pBits4++ = *pBits5++ = color; *pBits6++ = color2;
    *pBits1++ = *pBits2++ = *pBits3++ = *pBits4++ = *pBits5++ = color; *pBits6++ = color2;
    *pBits1++ = *pBits2++ = *pBits3++ = *pBits4++ = *pBits5++ = color; *pBits6++ = color2;
    *pBits1++ = *pBits2++ = *pBits3++ = *pBits4++ = *pBits5++ = color; *pBits6++ = color2;
    *pBits1++ = *pBits2++ = *pBits3++ = *pBits4++ = *pBits5++ = color2; *pBits6++ = color2;
    PREPARE_SCREEN_END()
}

void CALLBACK Emulator_PrepareScreen960x512(const uint8_t* pVideoBuffer, const uint32_t* palette, void* pImageBits)
{
    PREPARE_SCREEN_BEGIN(8)
    uint32_t* pBits2 = pBits1 + width;
    uint32_t* pBits3 = pBits2 + width;
    uint32_t* pBits4 = pBits3 + width;
    uint32_t* pBits5 = pBits4 + width;
    uint32_t* pBits6 = pBits5 + width;
    uint32_t* pBits7 = pBits6 + width;
    uint32_t* pBits8 = pBits7 + width;
    PREPARE_SCREEN_MIDDLE()
    uint32_t color2 = palette[colorindex << 1];
    *pBits1++ = *pBits2++ = *pBits3++ = *pBits4++ = *pBits5++ = *pBits6++ = *pBits7++ = color; *pBits8++ = color2;
    *pBits1++ = *pBits2++ = *pBits3++ = *pBits4++ = *pBits5++ = *pBits6++ = *pBits7++ = color; *pBits8++ = color2;
    *pBits1++ = *pBits2++ = *pBits3++ = *pBits4++ = *pBits5++ = *pBits6++ = *pBits7++ = color; *pBits8++ = color2;
    *pBits1++ = *pBits2++ = *pBits3++ = *pBits4++ = *pBits5++ = *pBits6++ = *pBits7++ = color; *pBits8++ = color2;
    *pBits1++ = *pBits2++ = *pBits3++ = *pBits4++ = *pBits5++ = *pBits6++ = *pBits7++ = color; *pBits8++ = color2;
    *pBits1++ = *pBits2++ = *pBits3++ = *pBits4++ = *pBits5++ = *pBits6++ = *pBits7++ = color; *pBits8++ = color2;
    *pBits1++ = *pBits2++ = *pBits3++ = *pBits4++ = *pBits5++ = *pBits6++ = *pBits7++ = color; *pBits8++ = color2;
    *pBits1++ = *pBits2++ = *pBits3++ = *pBits4++ = *pBits5++ = *pBits6++ = *pBits7++ = color2; *pBits8++ = color2;
    PREPARE_SCREEN_END()
}


//////////////////////////////////////////////////////////////////////
//
// Emulator image format - see CMotherboard::SaveToImage()
// Image header format (32 bytes):
//   4 bytes        MK90IMAGE_HEADER1
//   4 bytes        MK90IMAGE_HEADER2
//   4 bytes        MK90IMAGE_VERSION
//   4 bytes        MK90IMAGE_SIZE
//   4 bytes        MK90 uptime
//   12 bytes       Not used

bool Emulator_SaveImage(LPCTSTR sFilePath)
{
    // Create file
    FILE* fpFile = ::_tfsopen(sFilePath, _T("w+b"), _SH_DENYWR);
    if (fpFile == nullptr)
        return false;

    // Allocate memory
    uint8_t* pImage = (uint8_t*) ::calloc(MK90IMAGE_SIZE, 1);
    if (pImage == nullptr)
    {
        ::fclose(fpFile);
        return false;
    }
    memset(pImage, 0, MK90IMAGE_SIZE);
    // Prepare header
    uint32_t* pHeader = (uint32_t*) pImage;
    *pHeader++ = MK90IMAGE_HEADER1;
    *pHeader++ = MK90IMAGE_HEADER2;
    *pHeader++ = MK90IMAGE_VERSION;
    *pHeader++ = MK90IMAGE_SIZE;
    // Store emulator state to the image
    g_pBoard->SaveToImage(pImage);
    *(uint32_t*)(pImage + 16) = m_dwEmulatorUptime;

    // Save image to the file
    size_t dwBytesWritten = ::fwrite(pImage, 1, MK90IMAGE_SIZE, fpFile);
    ::free(pImage);
    ::fclose(fpFile);
    if (dwBytesWritten != MK90IMAGE_SIZE)
        return false;

    return true;
}

bool Emulator_LoadImage(LPCTSTR sFilePath)
{
    Emulator_Stop();
    Emulator_Reset();

    // Open file
    FILE* fpFile = ::_tfsopen(sFilePath, _T("rb"), _SH_DENYWR);
    if (fpFile == nullptr)
        return false;

    // Read header
    uint32_t bufHeader[MK90IMAGE_HEADER_SIZE / sizeof(uint32_t)];
    uint32_t dwBytesRead = ::fread(bufHeader, 1, MK90IMAGE_HEADER_SIZE, fpFile);
    if (dwBytesRead != MK90IMAGE_HEADER_SIZE)
    {
        ::fclose(fpFile);
        return false;
    }

    //TODO: Check version and size

    // Allocate memory
    uint8_t* pImage = (uint8_t*) ::calloc(MK90IMAGE_SIZE, 1);
    if (pImage == nullptr)
    {
        ::fclose(fpFile);
        return false;
    }

    // Read image
    ::fseek(fpFile, 0, SEEK_SET);
    dwBytesRead = ::fread(pImage, 1, MK90IMAGE_SIZE, fpFile);
    if (dwBytesRead != MK90IMAGE_SIZE)
    {
        ::free(pImage);
        ::fclose(fpFile);
        return false;
    }

    // Restore emulator state from the image
    g_pBoard->LoadFromImage(pImage);

    m_dwEmulatorUptime = *(uint32_t*)(pImage + 16);
    g_wEmulatorCpuPC = g_pBoard->GetCPU()->GetPC();

    // Free memory, close file
    ::free(pImage);
    ::fclose(fpFile);

    g_okEmulatorRunning = false;

    MainWindow_UpdateAllViews();

    return true;
}


//////////////////////////////////////////////////////////////////////
