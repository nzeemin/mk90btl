/*  This file is part of MK90BTL.
    MK90BTL is free software: you can redistribute it and/or modify it under the terms
of the GNU Lesser General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.
    MK90BTL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Lesser General Public License for more details.
    You should have received a copy of the GNU Lesser General Public License along with
MK90BTL. If not, see <http://www.gnu.org/licenses/>. */

// Board.cpp
//

#include "stdafx.h"
#include "Emubase.h"

void TraceInstruction(CProcessor* pProc, CMotherboard* pBoard, uint16_t address, DWORD dwTrace);


//////////////////////////////////////////////////////////////////////

CMotherboard::CMotherboard ()
{
    // Create devices
    m_pCPU = new CProcessor(this);

    m_dwTrace = TRACE_NONE;
    m_SoundGenCallback = NULL;
    m_okTimer50OnOff = false;
    m_okSoundOnOff = false;
    m_CPUbps = nullptr;

    // Allocate memory for RAM and ROM
    m_pRAM = (uint8_t*) ::calloc(64 * 1024, 1);
    m_pROM = (uint8_t*) ::calloc(32 * 1024, 1);

    SetConfiguration(0);  // Default configuration

    Reset();
}

CMotherboard::~CMotherboard ()
{
    // Delete devices
    delete m_pCPU;

    // Free memory
    ::free(m_pRAM);
    ::free(m_pROM);

    // Free memory allocated for SMPs and close the files
    for (int slot = 0; slot < 2; slot++)
    {
        if (m_Smp[slot].pData != nullptr)
        {
            ::free(m_Smp[slot].pData);
            m_Smp[slot].pData = nullptr;
        }
        if (m_Smp[slot].fpFile != nullptr)
        {
            ::fclose(m_Smp[slot].fpFile);
            m_Smp[slot].fpFile = nullptr;
        }
    }
}

void CMotherboard::SetConfiguration(uint16_t conf)
{
    m_Configuration = conf;

    // Clean RAM/ROM
    ::memset(m_pRAM, 0, 64 * 1024);
    ::memset(m_pROM, 0, 32 * 1024);

    //// Pre-fill RAM with "uninitialized" values
    //uint16_t * pMemory = (uint16_t *) m_pRAM;
    //uint16_t val = 0;
    //uint8_t flag = 0;
    //for (uint32_t i = 0; i < 128 * 1024; i += 2, flag--)
    //{
    //    *pMemory = val;  pMemory++;
    //    if (flag == 192)
    //        flag = 0;
    //    else
    //        val = ~val;
    //}
}

void CMotherboard::SetTrace(uint32_t dwTrace)
{
    m_dwTrace = dwTrace;
}

void CMotherboard::Reset()
{
    m_pCPU->Stop();

    // Reset ports
    m_LcdAddr = 0;  m_LcdConf = 0x88c6;  m_LcdIndex = 0xffff;
    m_okSoundOnOff = false;

    // External devices controller
    m_ExtDeviceKeyboardScan = 0;
    m_ExtDeviceControl = 0;
    m_ExtDeviceShift = 0xff;
    m_ExtDeviceSelect = false;
    m_ExtDeviceIntStatus = 0xffff;

    ResetDevices();

    m_pCPU->Start();
}

// Load 4 KB ROM image from the buffer
void CMotherboard::LoadROM(const uint8_t* pBuffer)
{
    ::memcpy(m_pROM, pBuffer, 32768);
}

void CMotherboard::LoadRAM(int startbank, const uint8_t* pBuffer, int length)
{
    ASSERT(pBuffer != NULL);
    ASSERT(startbank >= 0 && startbank < 15);
    int address = 8192 * startbank;
    ASSERT(address + length <= 128 * 1024);
    ::memcpy(m_pRAM + address, pBuffer, length);
}


//////////////////////////////////////////////////////////////////////

bool CMotherboard::IsSmpImageAttached(int slot) const
{
    ASSERT(slot >= 0 && slot < 2);
    return m_Smp[slot].fpFile != nullptr;
}
bool CMotherboard::AttachSmpImage(int slot, LPCTSTR sFileName)
{
    ASSERT(slot >= 0 && slot < 2);

    // if image attached - detach one first
    if (m_Smp[slot].fpFile != nullptr)
        DetachSmpImage(slot);

    // open the file
    m_Smp[slot].fpFile = ::_tfopen(sFileName, _T("r+b"));
    if (m_Smp[slot].fpFile == nullptr)
        return false;

    // get file size
    ::fseek(m_Smp[slot].fpFile, 0, SEEK_END);
    size_t size = ::ftell(m_Smp[slot].fpFile);
    if (size > 10240)
    {
        DetachSmpImage(slot);
        return false;
    }

    m_Smp[slot].size = size;
    m_Smp[slot].mask = size < 65536 ? 0xffff : 0xffffff;

    // allocate the memory
    m_Smp[slot].pData = (uint8_t*)::calloc(1, m_Smp[slot].size);
    //TODO: if null

    // load the file data
    ::fseek(m_Smp[slot].fpFile, 0, SEEK_SET);
    size_t count = ::fread(m_Smp[slot].pData, 1, m_Smp[slot].size, m_Smp[slot].fpFile);
    //TODO: if count != size

    return true;
}

void CMotherboard::DetachSmpImage(int slot)
{
    ASSERT(slot >= 0 && slot < 2);

    if (m_Smp[slot].fpFile == nullptr)
        return;

    // free the memory
    if (m_Smp[slot].pData != nullptr)
    {
        ::free(m_Smp[slot].pData);
        m_Smp[slot].pData = nullptr;
    }

    // close the file
    ::fclose(m_Smp[slot].fpFile);
    m_Smp[slot].fpFile = nullptr;
    m_Smp[slot].size = 0;
}


//////////////////////////////////////////////////////////////////////

uint16_t CMotherboard::GetRAMWord(uint16_t offset)
{
    return *((uint16_t*)(m_pRAM + offset));
}
uint8_t CMotherboard::GetRAMByte(uint16_t offset)
{
    return m_pRAM[offset];
}
void CMotherboard::SetRAMWord(uint16_t offset, uint16_t word)
{
    *((uint16_t*)(m_pRAM + offset)) = word;
}
void CMotherboard::SetRAMByte(uint16_t offset, uint8_t byte)
{
    m_pRAM[offset] = byte;
}

uint16_t CMotherboard::GetROMWord(uint16_t offset)
{
    return *((uint16_t*)(m_pROM + offset));
}
uint8_t CMotherboard::GetROMByte(uint16_t offset)
{
    return m_pROM[offset];
}


//////////////////////////////////////////////////////////////////////


void CMotherboard::ResetDevices()
{
    //m_pCPU->DeassertHALT();//DEBUG

    // Reset ports
    //TODO
}

void CMotherboard::ResetHALT()
{
}

void CMotherboard::Tick50()  // 50 Hz timer
{
    if (m_okTimer50OnOff)
    {
        m_pCPU->TickEVNT();
    }

    //TODO
}

void CMotherboard::TimerTick() // Timer Tick, 8 MHz
{
    //TODO
}

void CMotherboard::DebugTicks()
{
    m_pCPU->ClearInternalTick();
    m_pCPU->Execute();
}

void CMotherboard::ExecuteCPU()
{
    m_pCPU->Execute();
}

/*
Каждый фрейм равен 1/25 секунды = 40 мс
Фрейм делим на 20000 тиков, 1 тик = 2 мкс
В каждый фрейм происходит:
* 320000 тиков таймер 1 -- 16 раз за тик -- 8 МГц
* 320000 тиков ЦП       -- 16 раз за тик
*      2 тика IRQ2 и таймер 2 -- 50 Гц, в 0-й и 10000-й тик фрейма
*/
bool CMotherboard::SystemFrame()
{
    const int frameProcTicks = 16;
    const int audioticks = 20286 / (SOUNDSAMPLERATE / 25);

    for (int frameticks = 0; frameticks < 20000; frameticks++)
    {
        for (int procticks = 0; procticks < frameProcTicks; procticks++)  // CPU ticks
        {
#if !defined(PRODUCT)
            if ((m_dwTrace & TRACE_CPU) && m_pCPU->GetInternalTick() == 0)
                TraceInstruction(m_pCPU, this, m_pCPU->GetPC(), m_dwTrace);
#endif
            m_pCPU->Execute();
            if (m_CPUbps != nullptr)  // Check for breakpoints
            {
                const uint16_t* pbps = m_CPUbps;
                while (*pbps != 0177777) { if (m_pCPU->GetPC() == *pbps++) return false; }
            }

            // Timer 1 ticks
            TimerTick();
        }

        if (frameticks == 0 || frameticks == 10000)
        {
            Tick50();  // 1/50 timer event
        }

        if (frameticks % audioticks == 0)  // AUDIO tick
            DoSound();
    }

    return true;
}

// Key pressed or released
void CMotherboard::KeyboardEvent(uint8_t scancode, bool okPressed)
{
    if (okPressed)  // Key released
    {
        //if (m_dwTrace & TRACE_KEYBOARD)
        {
            DebugLogFormat(_T("Keyboard %03o %d\r\n"), (uint16_t)scancode, (int)okPressed);
        }

        m_ExtDeviceKeyboardScan = scancode;
        //TODO: Interrupt??
        m_pCPU->InterruptVIRQ(8, 0000310);
        return;
    }
}

uint8_t CMotherboard::ExtDeviceReadData()
{
    uint8_t result = m_ExtDeviceShift;
    m_ExtDeviceShift = 0xff;
    m_ExtDeviceIntStatus |= (1 << (m_ExtDeviceControl & 7));
    if (m_ExtDeviceSelect)
    {
        switch (m_ExtDeviceControl & 0x0f)
        {
        case 0:  // read SMP 0
        case 1:  // read SMP 1
            m_ExtDeviceShift = SmpReadData(m_ExtDeviceControl & 1);
            if ((m_ExtDeviceControl & 0x20) == 0)
                m_pCPU->InterruptVIRQ(7, 0000304);
            break;
        case 2:  // read keyboard
            if (m_ExtDeviceKeyboardScan != 0)
            {
                //DebugLogFormat(_T("ExtDeviceReadData Keyboard %03o\r\n"), (uint16_t)m_ExtDeviceKeyboardScan);
                m_ExtDeviceShift = m_ExtDeviceKeyboardScan;
                m_ExtDeviceKeyboardScan = 0;
                if ((m_ExtDeviceControl & 0x20) == 0)
                    m_pCPU->InterruptVIRQ(7, 0000304);
            }
            break;
        }
    }
    return result;
}
uint16_t CMotherboard::ExtDeviceReadStatus()
{
    return (m_ExtDeviceControl & 0x70) | 0xff84 | (m_ExtDeviceSelect ? 0 : 8);
}
uint8_t CMotherboard::ExtDeviceReadCommand()
{
    if (m_ExtDeviceSelect)
    {
        m_ExtDeviceSelect = false;
        if ((m_ExtDeviceControl & 0x20) == 0)
            m_pCPU->InterruptVIRQ(7, 0000304);
        return m_ExtDeviceShift;
    }
    return 0;
}
void CMotherboard::ExtDeviceWriteData(uint8_t byte)
{
    m_ExtDeviceShift = byte;
    m_ExtDeviceIntStatus |= (1 << (m_ExtDeviceControl & 7));
    if (m_ExtDeviceSelect)
    {
        switch (m_ExtDeviceControl & 0x0f)
        {
        case 0:  // read SMP 0
        case 1:  // read SMP 1
            m_ExtDeviceShift = SmpReadData(m_ExtDeviceControl & 1);
            if ((m_ExtDeviceControl & 0x20) == 0)
                m_pCPU->InterruptVIRQ(7, 0000304);
            break;
        case 2:  // read keyboard
            if (m_ExtDeviceKeyboardScan != 0)
            {
                //DebugLogFormat(_T("ExtDeviceReadData Keyboard %03o\r\n"), (uint16_t)m_ExtDeviceKeyboardScan);
                m_ExtDeviceShift = m_ExtDeviceKeyboardScan;
                m_ExtDeviceKeyboardScan = 0;
                if ((m_ExtDeviceControl & 0x20) == 0)
                    m_pCPU->InterruptVIRQ(7, 0000304);
            }
            break;
        case 8:  // write SMP 0
        case 9:  // write SMP 1
            SmpWriteData(m_ExtDeviceControl & 1, m_ExtDeviceShift);
            if ((m_ExtDeviceControl & 0x20) == 0)
                m_pCPU->InterruptVIRQ(7, 0000304);
            break;
        }
    }
}
void CMotherboard::ExtDeviceWriteClockRate(uint16_t /*word*/)
{
    //TODO: Ignored for now
}
void CMotherboard::ExtDeviceWriteControl(uint8_t byte)
{
    m_ExtDeviceControl = byte;
    if (m_ExtDeviceSelect)
    {
        switch (m_ExtDeviceControl & 0x0f)
        {
        case 0:  // read SMP 0
        case 1:  // read SMP 1
            m_ExtDeviceShift = SmpReadData(m_ExtDeviceControl & 1);
            if ((m_ExtDeviceControl & 0x20) == 0)
                m_pCPU->InterruptVIRQ(7, 0000304);
            break;
        case 2:  // read keyboard
            if (m_ExtDeviceKeyboardScan != 0)
            {
                //DebugLogFormat(_T("ExtDeviceReadData Keyboard %03o\r\n"), (uint16_t)m_ExtDeviceKeyboardScan);
                m_ExtDeviceShift = m_ExtDeviceKeyboardScan;
                m_ExtDeviceKeyboardScan = 0;
                if ((m_ExtDeviceControl & 0x20) == 0)
                    m_pCPU->InterruptVIRQ(7, 0000304);
            }
            break;
        }
    }
}
void CMotherboard::ExtDeviceWriteCommand(uint8_t byte)
{
    m_ExtDeviceSelect = true;
    m_ExtDeviceShift = byte;
    m_ExtDeviceIntStatus |= (1 << (m_ExtDeviceControl & 7));

    switch (m_ExtDeviceControl & 0x0f)
    {
    case 0:  // read SMP 0
    case 1:  // read SMP 1
        m_ExtDeviceShift = SmpReadCommand(m_ExtDeviceControl & 1);
        if ((m_ExtDeviceControl & 0x20) == 0)
            m_pCPU->InterruptVIRQ(7, 0000304);
        break;
    case 2:  // read keyboard
        if (m_ExtDeviceKeyboardScan != 0)
        {
            //DebugLogFormat(_T("ExtDeviceReadData Keyboard %03o\r\n"), (uint16_t)m_ExtDeviceKeyboardScan);
            m_ExtDeviceShift = m_ExtDeviceKeyboardScan;
            m_ExtDeviceKeyboardScan = 0;
            if ((m_ExtDeviceControl & 0x20) == 0)
                m_pCPU->InterruptVIRQ(7, 0000304);
        }
        break;
    case 8:  // write SMP 0
    case 9:  // write SMP 1
        SmpWriteCommand(m_ExtDeviceControl & 1, m_ExtDeviceShift);
        if ((m_ExtDeviceControl & 0x20) == 0)
            m_pCPU->InterruptVIRQ(7, 0000304);
        break;
    }
}

uint8_t CMotherboard::SmpReadCommand(int slot)
{
    ASSERT(slot >= 0 && slot < 2);

    return 0; //STUB
}
void CMotherboard::SmpWriteCommand(int slot, uint8_t byte)
{
    ASSERT(slot >= 0 && slot < 2);

    //DebugLogFormat(_T("SmpWriteCommand pos %04x cmd %02x\r\n"), m_Smp[slot].dataptr, (uint16_t)byte);
    m_Smp[slot].cmd = byte;
}
uint8_t CMotherboard::SmpReadData(int slot)
{
    ASSERT(slot >= 0 && slot < 2);

    switch ((m_Smp[slot].cmd & 0xf0) >> 4)
    {
    case 0:
        return 0;
    case 1:  // read data
    case 13:
        {
            uint8_t result = 0xff;
            if (m_Smp[slot].dataptr < m_Smp[slot].size)
            {
                result = *(m_Smp[slot].pData + m_Smp[slot].dataptr);
            }
            m_Smp[slot].dataptr = (m_Smp[slot].dataptr + ((m_Smp[slot].cmd & 0x80) ? 1 : -1)) & m_Smp[slot].mask;
            //DebugLogFormat(_T("SmpReadData data %02x nextpos %06x\r\n"), (uint16_t)result, m_Smp[slot].dataptr);
            return result;
        }
    default:
        return 0xff;
    }
}
void CMotherboard::SmpWriteData(int slot, uint8_t byte)
{
    ASSERT(slot >= 0 && slot < 2);

    switch ((m_Smp[slot].cmd & 0xf0) >> 4)
    {
    case 10:  // write address
        m_Smp[slot].dataptr = ((m_Smp[slot].dataptr << 8) | byte) & m_Smp[slot].mask;
        break;
    case 2:  // write data
    case 12:
    case 14:
        //TODO
        break;
    }
}



//////////////////////////////////////////////////////////////////////
// Motherboard: memory management

// Read word from memory for debugger
uint16_t CMotherboard::GetWordView(uint16_t address, bool okHaltMode, bool okExec, int* pAddrType)
{
    uint16_t offset;
    int addrtype = TranslateAddress(address, okHaltMode, okExec, &offset);

    *pAddrType = addrtype;

    switch (addrtype & ADDRTYPE_MASK)
    {
    case ADDRTYPE_RAM:
        return GetRAMWord(offset & 0177776);
    case ADDRTYPE_ROM:
        return GetROMWord(offset);
    case ADDRTYPE_IO:
        return 0;  // I/O port, not memory
    case ADDRTYPE_DENY:
        return 0;  // This memory is inaccessible for reading
    }

    ASSERT(false);  // If we are here - then addrtype has invalid value
    return 0;
}

uint16_t CMotherboard::GetWord(uint16_t address, bool okHaltMode, bool okExec)
{
    uint16_t offset;
    int addrtype = TranslateAddress(address, okHaltMode, okExec, &offset);

    switch (addrtype & ADDRTYPE_MASK)
    {
    case ADDRTYPE_RAM:
        return GetRAMWord(offset & 0177776);
    case ADDRTYPE_ROM:
        return GetROMWord(offset);
    case ADDRTYPE_IO:
        //TODO: What to do if okExec == true ?
        return GetPortWord(address);
    case ADDRTYPE_DENY:
        m_pCPU->MemoryError();
        return 0;
    }

    ASSERT(false);  // If we are here - then addrtype has invalid value
    return 0;
}

uint8_t CMotherboard::GetByte(uint16_t address, bool okHaltMode)
{
    uint16_t offset;
    int addrtype = TranslateAddress(address, okHaltMode, false, &offset);

    switch (addrtype & ADDRTYPE_MASK)
    {
    case ADDRTYPE_RAM:
        if (address == 0177562)
            DebugLogFormat(_T("GetByte %06o %03o\r\n"), address, (uint16_t)GetRAMByte(offset));
        return GetRAMByte(offset);
    case ADDRTYPE_ROM:
        return GetROMByte(offset);
    case ADDRTYPE_IO:
        //TODO: What to do if okExec == true ?
        return GetPortByte(address);
    case ADDRTYPE_DENY:
        m_pCPU->MemoryError();
        return 0;
    }

    ASSERT(false);  // If we are here - then addrtype has invalid value
    return 0;
}

void CMotherboard::SetWord(uint16_t address, bool okHaltMode, uint16_t word)
{
    uint16_t offset;

    int addrtype = TranslateAddress(address, okHaltMode, false, &offset);

    switch (addrtype & ADDRTYPE_MASK)
    {
    case ADDRTYPE_RAM:
        SetRAMWord(offset & 0177776, word);
        return;
    case ADDRTYPE_ROM:  // Writing to ROM: exception
        m_pCPU->MemoryError();
        return;
    case ADDRTYPE_IO:
        SetPortWord(address, word);
        return;
    case ADDRTYPE_DENY:
        m_pCPU->MemoryError();
        return;
    }

    ASSERT(false);  // If we are here - then addrtype has invalid value
}

void CMotherboard::SetByte(uint16_t address, bool okHaltMode, uint8_t byte)
{
    uint16_t offset;
    int addrtype = TranslateAddress(address, okHaltMode, false, &offset);

    switch (addrtype & ADDRTYPE_MASK)
    {
    case ADDRTYPE_RAM:
        SetRAMByte(offset, byte);
        return;
    case ADDRTYPE_ROM:  // Writing to ROM: exception
        m_pCPU->MemoryError();
        return;
    case ADDRTYPE_IO:
        SetPortByte(address, byte);
        return;
    case ADDRTYPE_DENY:
        m_pCPU->MemoryError();
        return;
    }

    ASSERT(false);  // If we are here - then addrtype has invalid value
}

// Calculates video buffer start address, for screen drawing procedure
const uint8_t* CMotherboard::GetVideoBuffer()
{
    return (m_pRAM + m_LcdAddr);
}

int CMotherboard::TranslateAddress(uint16_t address, bool /*okHaltMode*/, bool /*okExec*/, uint16_t* pOffset)
{
    if (address < 0040000)  // 000000-037777 -- RAM, 16K
    {
        *pOffset = address;
        return ADDRTYPE_RAM;
    }

    if (address < 0100000)  // 040000-077777 -- ??, 16K
    {
        *pOffset = address;
        return ADDRTYPE_DENY;
    }

    if (address >= 0164000 && address < 0166000)  // 164000-165777
    {
        if ((address >= 0164000 && address <= 0164007) ||
            (address >= 0164020 && address <= 0164027) ||
            (address >= 0164032 && address <= 0164035) ||
            (address >= 0165000 && address <= 0165177))  // Ports
        {
            *pOffset = address;
            return ADDRTYPE_IO;
        }

        *pOffset = address;
        return ADDRTYPE_RAM;
    }

    if (address < 0174666)  // 100000-174666?? -- ROM
    {
        *pOffset = address - 0100000;
        return ADDRTYPE_ROM;
    }

    //if (okHaltMode && address >= 0177600 && address <= 0177777)
    {
        *pOffset = address;
        return ADDRTYPE_RAM;
    }
}

uint8_t CMotherboard::GetPortByte(uint16_t address)
{
    if (address & 1)
        return GetPortWord(address & 0xfffe) >> 8;

    return (uint8_t) GetPortWord(address);
}

uint16_t CMotherboard::GetPortWord(uint16_t address)
{
    switch (address)
    {
    case 0164020:  // External devices controller, data register
        return ExtDeviceReadData();
    case 0164022:  // External devices controller, interrupt status
        return ExtDeviceReadIntStatus();
    case 0164024:  // External devices controller, status register
        return ExtDeviceReadStatus();
    case 0164026:  // External devices controller, command
        return ExtDeviceReadCommand();

        //TODO

    default:
        if (address >= 0165000 && address <= 0165177)  // Real time clock
        {
            DebugLogFormat(_T("READ PORT %06o PC=%06o\r\n"), address, m_pCPU->GetInstructionPC());
            //TODO
        }
        else
        {
            DebugLogFormat(_T("READ UNKNOWN PORT %06o PC=%06o\r\n"), address, m_pCPU->GetInstructionPC());

            m_pCPU->MemoryError();
        }
        return 0;
    }

    //return 0;
}

// Read word from port for debugger
uint16_t CMotherboard::GetPortView(uint16_t address)
{
    switch (address)
    {
        //TODO

    default:
        return 0;
    }
}

void CMotherboard::SetPortByte(uint16_t address, uint8_t byte)
{
    uint16_t word;
    if (address & 1)
    {
        word = GetPortWord(address & 0xfffe);
        word &= 0xff;
        word |= byte << 8;
        SetPortWord(address & 0xfffe, word);
    }
    else
    {
        word = GetPortWord(address);
        word &= 0xff00;
        SetPortWord(address, word | byte);
    }
}

void CMotherboard::SetPortWord(uint16_t address, uint16_t word)
{
    switch (address)
    {
    case 0164000:
    case 0164004:
        m_LcdAddr = word;
        break;
    case 0164002:
    case 0164006:
        m_LcdConf = word;
        break;

    case 0164020:
        ExtDeviceWriteData(word & 0xff);
        break;
    case 0164022:
        ExtDeviceWriteClockRate(word);
        break;
    case 0164024:
        ExtDeviceWriteControl(word & 0xff);
        break;
    case 0164026:
        ExtDeviceWriteCommand(word & 0xff);
        break;

    case 0164030:
    case 0164032:  // RG1
    case 0164034:  // RG2
    case 0164036:
        DebugLogFormat(_T("WRITE PORT %06o word=%06o PC=%06o\r\n"), address, word, m_pCPU->GetInstructionPC());
        break;  //STUB

    default:
        if (address >= 0165000 && address <= 0165177)
        {
            //TODO
        }
        else
        {
            DebugLogFormat(_T("WRITE UNKNOWN PORT %06o word=%06o PC=%06o\r\n"), address, word, m_pCPU->GetInstructionPC());

            m_pCPU->MemoryError();
        }
        break;
    }
}


//////////////////////////////////////////////////////////////////////
// Emulator image
//  Offset Length
//       0     32 bytes  - Header
//      32    128 bytes  - Board status
//     160     32 bytes  - CPU status
//     192   3904 bytes  - RESERVED
//    4096  32768 bytes  - Main ROM image 32K
//   36864 131072 bytes  - RAM image 64K
//  196608     --        - END

void CMotherboard::SaveToImage(uint8_t* pImage)
{
    // Board data
    uint16_t* pwImage = (uint16_t*) (pImage + 32);
    *pwImage++ = m_Configuration;
    pwImage += 6;  // RESERVED
    *pwImage++ = m_LcdAddr;
    *pwImage++ = m_LcdConf;
    *pwImage++ = m_LcdIndex;
    *pwImage++ = (uint16_t)m_okSoundOnOff;

    // CPU status
    uint8_t* pImageCPU = pImage + 160;
    m_pCPU->SaveToImage(pImageCPU);
    // ROM
    uint8_t* pImageRom = pImage + 4096;
    memcpy(pImageRom, m_pROM, 32 * 1024);
    // RAM
    uint8_t* pImageRam = pImage + 36864;
    memcpy(pImageRam, m_pRAM, 64 * 1024);
}
void CMotherboard::LoadFromImage(const uint8_t* pImage)
{
    // Board data
    uint16_t* pwImage = (uint16_t*)(pImage + 32);
    m_Configuration = *pwImage++;
    pwImage += 6;  // RESERVED
    m_LcdAddr = *pwImage++;
    m_LcdConf = *pwImage++;
    m_LcdIndex = *pwImage++;
    m_okSoundOnOff = ((*pwImage++) != 0);

    // CPU status
    const uint8_t* pImageCPU = pImage + 160;
    m_pCPU->LoadFromImage(pImageCPU);

    // ROM
    const uint8_t* pImageRom = pImage + 4096;
    memcpy(m_pROM, pImageRom, 32 * 1024);
    // RAM
    const uint8_t* pImageRam = pImage + 36864;
    memcpy(m_pRAM, pImageRam, 64 * 1024);
}


//////////////////////////////////////////////////////////////////////

void CMotherboard::DoSound(void)
{
    if (m_SoundGenCallback == NULL)
        return;

    uint16_t volume = 0;//TODO

    uint16_t sound = 0x1fff >> (3 - volume);
    (*m_SoundGenCallback)(sound, sound);
}

void CMotherboard::SetSoundGenCallback(SOUNDGENCALLBACK callback)
{
    if (callback == NULL)  // Reset callback
    {
        m_SoundGenCallback = NULL;
    }
    else
    {
        m_SoundGenCallback = callback;
    }
}


//////////////////////////////////////////////////////////////////////

#if !defined(PRODUCT)

void TraceInstruction(CProcessor* pProc, CMotherboard* pBoard, uint16_t address, DWORD dwTrace)
{
    bool okHaltMode = pProc->IsHaltMode();

    uint16_t memory[4];
    int addrtype = ADDRTYPE_RAM;
    memory[0] = pBoard->GetWordView(address + 0 * 2, okHaltMode, true, &addrtype);
    if (!(addrtype == ADDRTYPE_RAM && (dwTrace & TRACE_CPURAM)) &&
        !(addrtype == ADDRTYPE_ROM && (dwTrace & TRACE_CPUROM)))
        return;
    memory[1] = pBoard->GetWordView(address + 1 * 2, okHaltMode, true, &addrtype);
    memory[2] = pBoard->GetWordView(address + 2 * 2, okHaltMode, true, &addrtype);
    memory[3] = pBoard->GetWordView(address + 3 * 2, okHaltMode, true, &addrtype);

    TCHAR bufaddr[7];
    PrintOctalValue(bufaddr, address);

    TCHAR instr[8];
    TCHAR args[32];
    DisassembleInstruction(memory, address, instr, args);
    TCHAR buffer[64];
    _sntprintf(buffer, sizeof(buffer) / sizeof(TCHAR) - 1, _T("%s: %s\t%s\r\n"), bufaddr, instr, args);
    //_sntprintf(buffer, sizeof(buffer) / sizeof(TCHAR) - 1, _T("%s %s: %s\t%s\r\n"), pProc->IsHaltMode() ? _T("HALT") : _T("USER"), bufaddr, instr, args);

    DebugLog(buffer);
}

#endif

//////////////////////////////////////////////////////////////////////
