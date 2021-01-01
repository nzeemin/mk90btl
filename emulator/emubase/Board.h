/*  This file is part of MK90BTL.
    MK90BTL is free software: you can redistribute it and/or modify it under the terms
of the GNU Lesser General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.
    MK90BTL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Lesser General Public License for more details.
    You should have received a copy of the GNU Lesser General Public License along with
MK90BTL. If not, see <http://www.gnu.org/licenses/>. */

// Board.h
//

#pragma once

#include "Defines.h"

class CProcessor;


//////////////////////////////////////////////////////////////////////


// TranslateAddress result code
#define ADDRTYPE_RAM     0  // RAM
#define ADDRTYPE_ROM     8  // ROM
#define ADDRTYPE_IO     16  // I/O port
#define ADDRTYPE_DENY  128  // Access denied
#define ADDRTYPE_MASK  255  // RAM type mask

// Trace flags
#define TRACE_NONE         0  // Turn off all tracing
#define TRACE_CPUROM       1  // Trace CPU instructions from ROM
#define TRACE_CPURAM       2  // Trace CPU instructions from RAM
#define TRACE_CPU          3  // Trace CPU instructions (mask)
#define TRACE_CPUINT       7  // Trace CPU interrupts
#define TRACE_TIMER      010  // Trace timer events
#define TRACE_KEYBOARD 01000  // Trace keyboard events
#define TRACE_ALL    0177777  // Trace all

// Emulator image constants
#define MK90IMAGE_HEADER_SIZE 32
#define MK90IMAGE_SIZE 147456
#define MK90IMAGE_HEADER1 0x494D454E  // "MK90"
#define MK90IMAGE_HEADER2 0x21214147  // "BTL!"
#define MK90IMAGE_VERSION 0x00010000  // 1.0


//////////////////////////////////////////////////////////////////////

struct CSmp
{
public:
    FILE* fpFile;
    size_t size;            // File size in bytes
    uint8_t * pData;
    uint32_t dataptr;       // Data offset
    uint32_t mask;
    uint8_t cmd;
public:
    CSmp() { fpFile = nullptr; pData = nullptr; size = 0; dataptr = mask = 0; cmd = 0; }
    //void Reset();
};


//////////////////////////////////////////////////////////////////////

// Sound generator callback function type
typedef void (CALLBACK* SOUNDGENCALLBACK)(unsigned short L, unsigned short R);


//////////////////////////////////////////////////////////////////////

class CMotherboard  // MK90 computer
{
private:  // Devices
    CProcessor* m_pCPU;  // CPU device
    bool        m_okTimer50OnOff;
private:  // Memory
    uint16_t    m_Configuration;  // See BK_COPT_Xxx flag constants
    uint8_t*    m_pRAM;  // RAM, 64 KB
    uint8_t*    m_pROM;  // ROM, 32 KB
public:  // Construct / destruct
    CMotherboard();
    ~CMotherboard();
public:  // Getting devices
    CProcessor*     GetCPU() { return m_pCPU; }
public:  // Memory access  //TODO: Make it private
    uint16_t    GetRAMWord(uint16_t offset);
    uint8_t     GetRAMByte(uint16_t offset);
    void        SetRAMWord(uint16_t offset, uint16_t word);
    void        SetRAMByte(uint16_t offset, uint8_t byte);
    uint16_t    GetROMWord(uint16_t offset);
    uint8_t     GetROMByte(uint16_t offset);
public:  // Debug
    void        DebugTicks();  // One Debug CPU tick -- use for debug step or debug breakpoint
    void        SetCPUBreakpoints(const uint16_t* bps) { m_CPUbps = bps; } // Set CPU breakpoint list
    uint32_t    GetTrace() const { return m_dwTrace; }
    void        SetTrace(uint32_t dwTrace);
public:  // System control
    void        SetConfiguration(uint16_t conf);
    void        Reset();  // Reset computer
    void        LoadROM(const uint8_t* pBuffer);  // Load 32K ROM image from the buffer
    void        LoadRAM(int startbank, const uint8_t* pBuffer, int length);  // Load data into the RAM
    void        SetTimer50OnOff(bool okOnOff) { m_okTimer50OnOff = okOnOff; }
    bool        IsTimer50OnOff() const { return m_okTimer50OnOff; }
    void        Tick50();           // Tick 50 Hz - goes to CPU EVNT line
    void        TimerTick();        // Timer Tick, 31250 Hz, 32uS -- dividers are within timer routine
    void        ResetDevices();     // INIT signal
    void        ResetHALT();//DEBUG
public:
    void        ExecuteCPU();  // Execute one CPU instruction
    bool        SystemFrame();  // Do one frame -- use for normal run
    void        KeyboardEvent(uint8_t scancode, bool okPressed);  // Key pressed or released
public:  // SMP
    bool        AttachSmpImage(int slot, LPCTSTR sFileName);
    void        DetachSmpImage(int slot);
    bool        IsSmpImageAttached(int slot) const;
public:  // Callbacks
    void        SetSoundGenCallback(SOUNDGENCALLBACK callback);
public:  // Memory
    // Read command for execution
    uint16_t GetWordExec(uint16_t address, bool okHaltMode) { return GetWord(address, okHaltMode, TRUE); }
    // Read word from memory
    uint16_t GetWord(uint16_t address, bool okHaltMode) { return GetWord(address, okHaltMode, FALSE); }
    // Read word
    uint16_t GetWord(uint16_t address, bool okHaltMode, bool okExec);
    // Write word
    void SetWord(uint16_t address, bool okHaltMode, uint16_t word);
    // Read byte
    uint8_t GetByte(uint16_t address, bool okHaltMode);
    // Write byte
    void SetByte(uint16_t address, bool okHaltMode, uint8_t byte);
    // Read word from memory for debugger
    uint16_t GetWordView(uint16_t address, bool okHaltMode, bool okExec, int* pValid);
    // Read word from port for debugger
    uint16_t GetPortView(uint16_t address);
    // Get video buffer address
    const uint8_t* GetVideoBuffer();
private:
    // Determite memory type for given address - see ADDRTYPE_Xxx constants
    //   okHaltMode - processor mode (USER/HALT)
    //   okExec - TRUE: read instruction for execution; FALSE: read memory
    //   pOffset - result - offset in memory plane
    int TranslateAddress(uint16_t address, bool okHaltMode, bool okExec, uint16_t* pOffset);
private:  // Access to I/O ports
    uint16_t    GetPortWord(uint16_t address);
    void        SetPortWord(uint16_t address, uint16_t word);
    uint8_t     GetPortByte(uint16_t address);
    void        SetPortByte(uint16_t address, uint8_t byte);
public:  // Saving/loading emulator status
    void        SaveToImage(uint8_t* pImage);
    void        LoadFromImage(const uint8_t* pImage);
private:  // Ports: implementation
    uint16_t    m_LcdAddr;
    uint16_t    m_LcdConf;
    uint16_t    m_LcdIndex;
private:  // Implementation: external devices controller
    uint8_t     m_ExtDeviceKeyboardScan;
    uint8_t     m_ExtDeviceControl;
    uint8_t     m_ExtDeviceShift;
    bool        m_ExtDeviceSelect;
    uint16_t    m_ExtDeviceIntStatus;
    uint8_t     ExtDeviceReadData();
    uint16_t    ExtDeviceReadIntStatus() { return m_ExtDeviceIntStatus; }
    uint16_t    ExtDeviceReadStatus();
    uint8_t     ExtDeviceReadCommand();
    void        ExtDeviceWriteData(uint8_t byte);
    void        ExtDeviceWriteClockRate(uint16_t word);
    void        ExtDeviceWriteControl(uint8_t byte);
    void        ExtDeviceWriteCommand(uint8_t byte);
private:  // Implementation: SMPs
    CSmp        m_Smp[2];
    uint8_t     SmpReadCommand(int slot);
    void        SmpWriteCommand(int slot, uint8_t byte);
    uint8_t     SmpReadData(int slot);
    void        SmpWriteData(int slot, uint8_t byte);
private:
    const uint16_t* m_CPUbps;  // CPU breakpoint list, ends with 177777 value
    uint32_t    m_dwTrace;  // Trace flags
    bool        m_okSoundOnOff;
private:
    SOUNDGENCALLBACK m_SoundGenCallback;
    void        DoSound();
};


//////////////////////////////////////////////////////////////////////
