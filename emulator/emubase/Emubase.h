/*  This file is part of MK90BTL.
    MK90BTL is free software: you can redistribute it and/or modify it under the terms
of the GNU Lesser General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.
    MK90BTL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Lesser General Public License for more details.
    You should have received a copy of the GNU Lesser General Public License along with
MK90BTL. If not, see <http://www.gnu.org/licenses/>. */

// Emubase.h  Header for use of all emubase classes
//

#pragma once

#include "Board.h"
#include "Processor.h"


//////////////////////////////////////////////////////////////////////


#define SOUNDSAMPLERATE  22050


//////////////////////////////////////////////////////////////////////
// Disassembler

// Disassemble one instruction of KM1801VM2 processor
//   pMemory - memory image (we read only words of the instruction)
//   sInstr  - instruction mnemonics buffer - at least 8 characters
//   sArg    - instruction arguments buffer - at least 32 characters
//   Return value: number of words in the instruction
uint16_t DisassembleInstruction(const uint16_t* pMemory, uint16_t addr, TCHAR* sInstr, TCHAR* sArg);

bool Disasm_CheckForJump(const uint16_t* memory, int* pDelta);

// Prepare "Jump Hint" string, and also calculate condition for conditional jump
// Returns: jump prediction flag: true = will jump, false = will not jump
bool Disasm_GetJumpConditionHint(
    const uint16_t* memory, const CProcessor * pProc, const CMotherboard * pBoard, LPTSTR buffer);

// Prepare "Instruction Hint" for a regular instruction (not a branch/jump one)
// buffer, buffer2 - buffers for 1st and 2nd lines of the instruction hint, min size 42
// Returns: number of hint lines; 0 = no hints
int Disasm_GetInstructionHint(
    const uint16_t* memory, const CProcessor * pProc, const CMotherboard * pBoard,
    LPTSTR buffer, LPTSTR buffer2);


//////////////////////////////////////////////////////////////////////
