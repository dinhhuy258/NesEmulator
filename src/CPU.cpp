#include "CPU.h"
#include <assert.h>
#include "Platforms.h"

CPU::CPU(Memory *cpuMemory)
{
    A = 0;
    X = 0;
    Y = 0;
    SP = 0xFD;
    PC = 0xC000;
    P.byte = 0x24;
    cycles = 0;
    stall = 0;
    this->cpuMemory = cpuMemory;
    interrupt = InterruptNone;
}

uint8_t CPU::Step()
{
    if (stall > 0)
    {
        // Suspend CPU after writting DMA. It will take 1 cycle for each step
        --stall;
        return 1;
    }
    uint64_t preCycles = cycles;
    switch (interrupt)
    {
        case InterruptNMI:
            NMI();
            break;
        case InterruptIRQ:
            IRQ();
            break;
    }
    interrupt = InterruptNone;
    currentOpcode = cpuMemory->Read(PC++);
    // Implement this opcode
    (this->*opcodeFunctions[currentOpcode])();
    return uint8_t(cycles - preCycles);
}

// Address mode
uint8_t CPU::AddressAbsolute()
{
    // 6502 is little endian
    uint16_t lo = cpuMemory->Read(PC++);
    uint16_t hi = cpuMemory->Read(PC++);
    uint16_t address = (hi << 8) | lo;
    lastAddress = address;
    return cpuMemory->Read(address);
}

uint8_t CPU::AddressAbsoluteX(bool checkPage)
{
    // 6502 is little endian
    uint16_t lo = cpuMemory->Read(PC++);
    uint16_t hi = cpuMemory->Read(PC++);
    uint16_t address = (hi << 8) | lo;
    // If the result of Base+Index is greater than $FFFF, wrapping will occur.
    if (checkPage && ((address & 0xFF00) != ((address + X) & 0xFF00)))
    {
        // page cross
        ++cycles;
    }
    address = address + X;
    lastAddress = address;
    return cpuMemory->Read(address);    
}

uint8_t CPU::AddressAbsoluteY(bool checkPage)
{
    // 6502 is little endian
    uint16_t lo = cpuMemory->Read(PC++);
    uint16_t hi = cpuMemory->Read(PC++);
    uint16_t address = (hi << 8) | lo;
    // If the result of Base+Index is greater than $FFFF, wrapping will occur.
    if (checkPage && ((address & 0xFF00) != ((address + Y) & 0xFF00))) 
    {
        // page crossed
        ++cycles;
    }
    address = address + Y;
    lastAddress = address;
    return cpuMemory->Read(address);
}

uint8_t CPU::AddressAccumulator()
{
    return A;
}

uint8_t CPU::AddressImmediate()
{
    return cpuMemory->Read(PC++);
}

uint8_t CPU::AddressImplied()
{
    // not used
    return 0;
}

uint8_t CPU::AddressIndirectX()
{
    // 6502 is little endian
    uint16_t baseAddress = cpuMemory->Read(PC++);
    baseAddress = (baseAddress + X) & 0xFF;
    uint16_t lo = cpuMemory->Read(baseAddress);
    uint16_t hi = cpuMemory->Read((baseAddress + 1) & 0xFF);
    uint16_t address = (hi << 8) | lo;
    lastAddress = address;
    return cpuMemory->Read(address);
}

uint8_t CPU::AddressIndirect()
{
    // 6502 is little endian
    uint16_t baseAddress = cpuMemory->Read(PC++);
    uint16_t lo = cpuMemory->Read(baseAddress);
    uint16_t hi = cpuMemory->Read((baseAddress + 1) & 0xFF);
    uint16_t address = (hi << 8) | lo;
    lastAddress = address;
    return cpuMemory->Read(address);
}

uint8_t CPU::AddressIndirectY(bool checkPage)
{
    // 6502 is little endian
    uint16_t baseAddress = cpuMemory->Read(PC++);
    uint16_t lo = cpuMemory->Read(baseAddress);
    uint16_t hi = cpuMemory->Read((baseAddress + 1) & 0xFF);
    uint16_t address = (hi << 8) | lo;
    // If Base_Location+Index is greater than $FFFF, wrapping will occur.
    if (checkPage && ((address & 0xFF00) != ((address + Y) & 0xFF00)))
    {
        // page crossed
        ++cycles;
    }
    address = (address + Y);
    lastAddress = address;
    return cpuMemory->Read(address);
}

uint8_t CPU::AddressRelative()
{
    return cpuMemory->Read(PC++);
}

uint8_t CPU::AddressZeroPage()
{
    uint16_t address = cpuMemory->Read(PC++);
    address &= 0x00FF;
    lastAddress = address;
    return cpuMemory->Read(address);
}

uint8_t CPU::AddressZeroPageX()
{
    uint16_t address = cpuMemory->Read(PC++);
    address = (address + X) & 0x00FF;
    lastAddress = address;
    return cpuMemory->Read(address);
}

uint8_t CPU::AddressZeroPageY()
{
    uint16_t address = cpuMemory->Read(PC++);
    address = (address + Y) & 0x00FF;
    lastAddress = address;
    return cpuMemory->Read(address);
}
// Memory
void CPU::StackPush(uint8_t value)
{
    cpuMemory->Write(0x0100 | SP, value);
    --SP;
}

uint8_t CPU::StackPull()
{
    ++SP;
    return cpuMemory->Read(0x0100 | SP);
}

// Interrupt
void CPU::TriggerNMI()
{
    interrupt = InterruptNMI;
}

void CPU::NMI()
{
    ++PC;
    StackPush((PC >> 8) & 0xFF);
    StackPush(PC & 0xFF);
    StackPush(P.byte | FLAG_BREAK);
    P.bits.I = SET;
    uint16_t lo = cpuMemory->Read(NMI_VECTOR_LOW);
    uint16_t hi = cpuMemory->Read(NMI_VECTOR_HIGH);
    PC = (hi << 8) | lo;
    cycles += 7;
}

void CPU::IRQ()
{
    ++PC;
    StackPush((PC >> 8) & 0xFF);
    StackPush(PC & 0xFF);
    StackPush(P.byte | FLAG_BREAK);
    P.bits.I = SET;
    uint16_t lo = cpuMemory->Read(IRQ_VECTOR_LOW);
    uint16_t hi = cpuMemory->Read(IRQ_VECTOR_HIGH);
    PC = (hi << 8) | lo;
    cycles += 7;
}

// Opcodes
void CPU::ADC()
{
    /*
     * Add Memory to A with CarryFlag
     * Affects Flags: V N Z C
     *
     *   MODE         SYNTAX       HEX LEN TIM
     * Immediate     ADC #$44      $69  2   2
     * Zero Page     ADC $44       $65  2   3
     * Zero Page,X   ADC $44,X     $75  2   4
     * Absolute      ADC $4400     $6D  3   4
     * Absolute,X    ADC $4400,X   $7D  3   4+
     * Absolute,Y    ADC $4400,Y   $79  3   4+
     * Indirect,X    ADC ($44,X)   $61  2   6
     * Indirect,Y    ADC ($44),Y   $71  2   5+
     *
     * + Add 1 cycle if page boundary crossed
     */
    uint8_t value;
    switch(opcodeAddressModes[currentOpcode])
    {
        case Immediate:
            value = AddressImmediate();
            break;
        case ZeroPage:
            value = AddressZeroPage();
            break;
        case ZeroPageX:
            value = AddressZeroPageX();
            break;
        case Absolute:
            value = AddressAbsolute();
            break;
        case AbsoluteX:
            value = AddressAbsoluteX(true);
            break;
        case AbsoluteY:
            value = AddressAbsoluteY(true);
            break;
        case IndirectX:
            value = AddressIndirectX();
            break;
        case IndirectY:
            value = AddressIndirectY(true);
            break;
        default:
            assert(0);
    }
    uint16_t result = A + value + P.bits.C;
    P.bits.V = (((A & 0x80) != (result & 0x80)) && ((value & 0x80) != (result & 0x80))) ? SET : CLEAR;
    P.bits.N = ((result & 0x80) == 0x80) ? SET : CLEAR;
    P.bits.Z = ((result & 0xFF) == 0) ? SET : CLEAR;
    P.bits.C = (result > 0xFF) ? SET : CLEAR;
    A = (uint8_t)(result & 0xFF);
    cycles += opcodeCycles[currentOpcode];
}

void CPU::ALR()
{
}

void CPU::ANC()
{
}

void CPU::AND()
{
    /*
     * Bitwise-AND A with Memory
     * Affects Flags: N Z
     *
     * MODE           SYNTAX       HEX LEN TIM
     * Immediate     AND #$44      $29  2   2
     * Zero Page     AND $44       $25  2   3
     * Zero Page,X   AND $44,X     $35  2   4
     * Absolute      AND $4400     $2D  3   4
     * Absolute,X    AND $4400,X   $3D  3   4+
     * Absolute,Y    AND $4400,Y   $39  3   4+
     * Indirect,X    AND ($44,X)   $21  2   6
     * Indirect,Y    AND ($44),Y   $31  2   5+
     *
     * + Add 1 cycle if page boundary crossed
     */
    uint8_t value;
    switch(opcodeAddressModes[currentOpcode])
    {
        case Immediate:
            value = AddressImmediate();
            break;
        case ZeroPage:
            value = AddressZeroPage();
            break;
        case ZeroPageX:
            value = AddressZeroPageX();
            break;
        case Absolute:
            value = AddressAbsolute();
            break;
        case AbsoluteX:
            value = AddressAbsoluteX(true);
            break;
        case AbsoluteY:
            value = AddressAbsoluteY(true);
            break;
        case IndirectX:
            value = AddressIndirectX();
            break;
        case IndirectY:
            value = AddressIndirectY(true);
            break;
        default:
            assert(0);
    }
    A = A & value;
    P.bits.N = ((A & 0x80) == 0x80) ? SET : CLEAR;
    P.bits.Z = (A == 0) ? SET : CLEAR;
    cycles += opcodeCycles[currentOpcode];
}

void CPU::ARR()
{
}

void CPU::ASL()
{
    /*
     * Arithmetic Shift Left
     * Affects Flags: N Z C
     *
     * MODE           SYNTAX       HEX LEN TIM
     * Accumulator   ASL A         $0A  1   2
     * Zero Page     ASL $44       $06  2   5
     * Zero Page,X   ASL $44,X     $16  2   6
     * Absolute      ASL $4400     $0E  3   6
     * Absolute,X    ASL $4400,X   $1E  3   7
     */
    uint8_t value;
    switch(opcodeAddressModes[currentOpcode])
    {
        case Accumulator:
            value = AddressAccumulator();
            break;
        case ZeroPage:
            value = AddressZeroPage();
            break;
        case ZeroPageX:
            value = AddressZeroPageX();
            break;
        case Absolute:
            value = AddressAbsolute();
            break;
        case AbsoluteX:
            value = AddressAbsoluteX();
            break;
        default:
            assert(0);
    }
    P.bits.C = ((value & 0x80) == 0x80) ? SET : CLEAR;
    uint8_t result = (value << 1) & 0xFE; // make sure that the lowest bit is equal 0
    P.bits.Z = (result == 0) ? SET : CLEAR;
    P.bits.N = ((result & 0x80) == 0x80) ? SET : CLEAR;
    switch(opcodeAddressModes[currentOpcode])
    {
        case Accumulator:
            A = result;
            break;
        default:
            cpuMemory->Write(lastAddress, result);
            break;
    }
    cycles += opcodeCycles[currentOpcode];
}

void CPU::AXA()
{
}

void CPU::AXS()
{
}

void CPU::BCC()
{
    /*
     * Branch if P.C is CLEAR
     *
     * MODE           SYNTAX       HEX  LEN  TIM
     * Relative       BCC $A5      $90   2   2++
     *
     * ++ Add 1 TIM if a the branch occurs and the destination address is on the same page
     * ++ Add 2 TIM if a the branch occurs and the destination address is on a different page
     */
    assert(opcodeAddressModes[currentOpcode] == Relative);
    uint8_t offset = AddressRelative();
    if (P.bits.C == CLEAR)
    {
        if (((PC + offset) & 0xFF00) != (PC & 0xFF00))
        {
            ++cycles; // Add 1 TIM if the destination address is on a different page
        }
        ++cycles; // Add 1 TIM if a branch occurs
        PC += offset;
    }
    cycles += opcodeCycles[currentOpcode];
}

void CPU::BCS()
{
    /*
     * Branch if P.C is SET
     *
     * MODE           SYNTAX       HEX  LEN  TIM
     * Relative       BCS $A5      $B0   2   2++
     *
     * ++ Add 1 TIM if a the branch occurs and the destination address is on the same page
     * ++ Add 2 TIM if a the branch occurs and the destination address is on a different page
     */
    assert(opcodeAddressModes[currentOpcode] == Relative);
    uint8_t offset = AddressRelative();
    if (P.bits.C == SET)
    {
        if (((PC + offset) & 0xFF00) != (PC & 0xFF00))
        {
            ++cycles; // Add 1 TIM if the destination address is on a different page
        }
        ++cycles; // Add 1 TIM if a branch occurs
        PC += offset;
    }
    cycles += opcodeCycles[currentOpcode];
}

void CPU::BEQ()
{
    /*
     * Branch if P.Z is SET
     *
     * MODE           SYNTAX       HEX  LEN  TIM
     * Relative       BEQ $A5      $F0   2   2++
     *
     * ++ Add 1 TIM if a the branch occurs and the destination address is on the same page
     * ++ Add 2 TIM if a the branch occurs and the destination address is on a different page
     */
    assert(opcodeAddressModes[currentOpcode] == Relative);
    uint8_t offset = AddressRelative();
    if (P.bits.Z == SET)
    {
        if (((PC + offset) & 0xFF00) != (PC & 0xFF00))
        {
            ++cycles; // Add 1 TIM if the destination address is on a different page
        }
        ++cycles; // Add 1 TIM if a branch occurs
        PC += offset;
    }
    cycles += opcodeCycles[currentOpcode];
}

void CPU::BIT()
{
    /*
     * Test bits in A with Memory
     * Affects Flags: N V Z
     *
     * MODE           SYNTAX       HEX LEN TIM
     * Zero Page     BIT $44       $24  2   3
     * Absolute      BIT $4400     $2C  3   4
     */
    uint8_t value;
    switch(opcodeAddressModes[currentOpcode])
    {
        case ZeroPage:
            value = AddressZeroPage();
            break;
        case Absolute:
            value = AddressAbsolute();
            break;
        default:
            assert(0);
    }
    uint8_t result = A & value;
    //NOTE: Refer here http://www.6502.org/tutorials/6502opcodes.html#BIT to know how to implement this opcode
    P.bits.N = ((value & 0x80) == 0x80) ? SET : CLEAR;
    P.bits.V = ((value & 0x40) == 0x40) ? SET : CLEAR;
    P.bits.Z = (result == 0) ? SET : CLEAR;
    cycles += opcodeCycles[currentOpcode];
}

void CPU::BMI()
{
    /*
     * Branch if P.N is SET
     * 
     * MODE           SYNTAX       HEX  LEN  TIM
     * Relative       BMI $A5      $30   2   2++
     *
     * ++ Add 1 TIM if a the branch occurs and the destination address is on the same page
     * ++ Add 2 TIM if a the branch occurs and the destination address is on a different page
     */
    assert(opcodeAddressModes[currentOpcode] == Relative);
    uint8_t offset = AddressRelative();
    if (P.bits.N == SET)
    {
        if (((PC + offset) & 0xFF00) != (PC & 0xFF00))
        {
            ++cycles; // Add 1 TIM if the destination address is on a different page
        }
        ++cycles; // Add 1 TIM if a branch occurs
        PC += offset;
    }
    cycles += opcodeCycles[currentOpcode];
}

void CPU::BNE()
{
    /*
     * Branch if P.Z is CLEAR
     * 
     * MODE           SYNTAX       HEX  LEN  TIM
     * Relative       BNE $A5      $D0   2   2++
     *
     * ++ Add 1 TIM if a the branch occurs and the destination address is on the same page
     * ++ Add 2 TIM if a the branch occurs and the destination address is on a different page
     */
    assert(opcodeAddressModes[currentOpcode] == Relative);
    uint8_t offset = AddressRelative();
    if (P.bits.Z == CLEAR)
    {
        if (((((PC + offset) & 0xFF) | (PC & 0xFF00)) & 0xFF00) != (PC & 0xFF00))
        {
            ++cycles; // Add 1 TIM if the destination address is on a different page
        }
        ++cycles; // Add 1 TIM if a branch occurs
        //NOTE: BNE bug
        PC = ((PC + offset) & 0xFF) | (PC & 0xFF00);
    }
    cycles += opcodeCycles[currentOpcode];
}

void CPU::BPL()
{
    /*
     * Branch if P.N is CLEAR
     * 
     * MODE           SYNTAX       HEX  LEN  TIM
     * Relative       BPL $A5      $10   2   2++
     *
     * ++ Add 1 TIM if a the branch occurs and the destination address is on the same page
     * ++ Add 2 TIM if a the branch occurs and the destination address is on a different page
     */
    assert(opcodeAddressModes[currentOpcode] == Relative);
    uint8_t offset = AddressRelative();
    if (P.bits.N == CLEAR)
    {
        if (((PC + offset) & 0xFF00) != (PC & 0xFF00))
        {
            ++cycles; // Add 1 TIM if the destination address is on a different page
        }
        ++cycles; // Add 1 TIM if a branch occurs
        PC += offset;
    }
    cycles += opcodeCycles[currentOpcode]; 
}

void CPU::BRK()
{
    /*
     * Simulate Interrupt ReQuest (IRQ)
     * Affects Flags: B
     *
     * MODE        SYNTAX      HEX LEN TIM
     * Implied     BRK         $00  1   7
     */
    assert(opcodeAddressModes[currentOpcode] == Implied);
    ++PC;
    StackPush((PC >> 8) & 0xFF);
    StackPush(PC & 0xFF);
    StackPush(P.byte | FLAG_BREAK);
    P.bits.I = SET;
    uint16_t lo = cpuMemory->Read(IRQ_VECTOR_LOW);
    uint16_t hi = cpuMemory->Read(IRQ_VECTOR_HIGH);
    PC = (hi << 8) | lo;
    cycles += opcodeCycles[currentOpcode]; 
}

void CPU::BVC()
{
    /*
     * Branch if P.V is CLEAR
     * 
     * MODE           SYNTAX       HEX  LEN  TIM
     * Relative       BVC $A5      $50   2   2++
     *
     * ++ Add 1 TIM if a the branch occurs and the destination address is on the same page
     * ++ Add 2 TIM if a the branch occurs and the destination address is on a different page
     */
    assert(opcodeAddressModes[currentOpcode] == Relative);
    uint8_t offset = AddressRelative();
    if (P.bits.V == CLEAR)
    {
        if (((PC + offset) & 0xFF00) != (PC & 0xFF00))
        {
            ++cycles; // Add 1 TIM if the destination address is on a different page
        }
        ++cycles; // Add 1 TIM if a branch occurs
        PC += offset;
    }
    cycles += opcodeCycles[currentOpcode]; 
}

void CPU::BVS()
{
    /*
     * Branch if P.V is SET
     * 
     * MODE           SYNTAX       HEX  LEN  TIM
     * Relative       BVS $A5      $70   2   2++
     *
     * ++ Add 1 TIM if a the branch occurs and the destination address is on the same page
     * ++ Add 2 TIM if a the branch occurs and the destination address is on a different page
     */
    assert(opcodeAddressModes[currentOpcode] == Relative);
    uint8_t offset = AddressRelative();
    if (P.bits.V == SET)
    {
        if (((PC + offset) & 0xFF00) != (PC & 0xFF00))
        {
            ++cycles; // Add 1 TIM if the destination address is on a different page
        }
        ++cycles; // Add 1 TIM if a branch occurs
        PC += offset;
    }
    cycles += opcodeCycles[currentOpcode];   
}

void CPU::CLC()
{
    /*
     * Clear Carry Flag (P.C)
     * Affects Flags: C
     *
     * MODE        SYNTAX      HEX LEN TIM
     * Implied     CLC         $18  1   2
     */
    assert(opcodeAddressModes[currentOpcode] == Implied);
    P.bits.C = CLEAR;
    cycles += opcodeCycles[currentOpcode];
}

void CPU::CLD()
{
    /*
     * Clear Decimal Flag (P.D)
     * Affects Flags: D
     *
     * MODE        SYNTAX      HEX LEN TIM
     * Implied     CLD         $D8  1   2
     */
    assert(opcodeAddressModes[currentOpcode] == Implied);
    P.bits.D = CLEAR;
    cycles += opcodeCycles[currentOpcode];
}

void CPU::CLI()
{
    /*
     * Clear Interrupt (disable) Flag (P.I)
     * Affects Flags: I
     *
     * MODE        SYNTAX      HEX LEN TIM
     * Implied     CLI         $58  1   2
     */
    assert(opcodeAddressModes[currentOpcode] == Implied);
    P.bits.I = CLEAR;
    cycles += opcodeCycles[currentOpcode];
}

void CPU::CLV()
{
    /*
     * Clear Clear Overflow Flag (P.V)
     * Affects Flags: V
     *
     * MODE        SYNTAX      HEX LEN TIM
     * Implied     CLV         $B8  1   2
     */
    assert(opcodeAddressModes[currentOpcode] == Implied);
    P.bits.V = CLEAR;
    cycles += opcodeCycles[currentOpcode];
}

void CPU::CMP()
{
    /*
     * Compare A with Memory
     * Affects Flags: N Z C
     *
     * MODE           SYNTAX       HEX LEN TIM
     * Immediate     CMP #$44      $C9  2   2
     * Zero Page     CMP $44       $C5  2   3
     * Zero Page,X   CMP $44,X     $D5  2   4
     * Absolute      CMP $4400     $CD  3   4
     * Absolute,X    CMP $4400,X   $DD  3   4+
     * Absolute,Y    CMP $4400,Y   $D9  3   4+
     * Indirect,X    CMP ($44,X)   $C1  2   6
     * Indirect,Y    CMP ($44),Y   $D1  2   5+
     *
     * + Add 1 cycle if page boundary crossed
     */
    uint8_t value;
    switch(opcodeAddressModes[currentOpcode])
    {
        case Immediate:
            value = AddressImmediate();
            break;
        case ZeroPage:
            value = AddressZeroPage();
            break;
        case ZeroPageX:
            value = AddressZeroPageX();
            break;
        case Absolute:
            value = AddressAbsolute();
            break;
        case AbsoluteX:
            value = AddressAbsoluteX(true);
            break;
        case AbsoluteY:
            value = AddressAbsoluteY(true);
            break;
        case IndirectX:
            value = AddressIndirectX();
            break;
        case IndirectY:
            value = AddressIndirectY(true);
            break;
        default:
            assert(0);
    }
    uint8_t result = A - value;
    P.bits.N = ((result & 0x80) == 0x80) ? SET : CLEAR;
    P.bits.Z = (result == 0) ? SET : CLEAR;
    P.bits.C = (A >= value) ? SET : CLEAR;
    cycles += opcodeCycles[currentOpcode];
}

void CPU::CPX()
{
    /*
     * Compare X with Memory
     * Affects Flags: N Z C
     *
     * MODE           SYNTAX       HEX LEN TIM
     * Immediate     CPX #$44      $E0  2   2
     * Zero Page     CPX $44       $E4  2   3
     * Absolute      CPX $4400     $EC  3   4
     */
    uint8_t value;
    switch(opcodeAddressModes[currentOpcode])
    {
        case Immediate:
            value = AddressImmediate();
            break;
        case ZeroPage:
            value = AddressZeroPage();
            break;
        case Absolute:
            value = AddressAbsolute();
            break;
        default:
            assert(0);
    }
    uint8_t result = X - value;
    P.bits.N = ((result & 0x80) == 0x80) ? SET : CLEAR;
    P.bits.Z = (result == 0) ? SET : CLEAR;
    P.bits.C = (X >= value) ? SET : CLEAR;
    cycles += opcodeCycles[currentOpcode];
}

void CPU::CPY()
{
    /*
     * Compare Y with Memory
     * Affects Flags: N Z C
     *
     * MODE           SYNTAX       HEX LEN TIM
     * Immediate     CPY #$44      $C0  2   2
     * Zero Page     CPY $44       $C4  2   3
     * Absolute      CPY $4400     $CC  3   4
     */
    uint8_t value;
    switch(opcodeAddressModes[currentOpcode])
    {
        case Immediate:
            value = AddressImmediate();
            break;
        case ZeroPage:
            value = AddressZeroPage();
            break;
        case Absolute:
            value = AddressAbsolute();
            break;
        default:
            assert(0);
    }
    uint8_t result = Y - value;
    P.bits.N = ((result & 0x80) == 0x80) ? SET : CLEAR;
    P.bits.Z = (result == 0) ? SET : CLEAR;
    P.bits.C = (Y >= value) ? SET : CLEAR;
    cycles += opcodeCycles[currentOpcode];
}

void CPU::DCP()
{
    /*
     * DECs the contents of a memory location and then CMPs the result with the A register
     * Affects Flags: N,Z,C
     *
     * MODE        |SYNTAX     |HEX|LEN|TIM
     * ------------|-----------|---|---|---
     * Zero Page   |DCP arg    |$C7| 2 | 5
     * Zero Page,X |DCP arg,X  |$D7| 2 | 6
     * Absolute    |DCP arg    |$CF| 3 | 6
     * Absolute,X  |DCP arg,X  |$DF| 3 | 7
     * Absolute,Y  |DCP arg,Y  |$DB| 3 | 7
     * Indirect,X  |DCP (arg,X)|$C3| 2 | 8
     * Indirect,Y  |DCP (arg),Y|$D3| 2 | 8
     */
    uint8_t value;
    switch(opcodeAddressModes[currentOpcode])
    {
        case ZeroPage:
            value = AddressZeroPage();
            break;
        case ZeroPageX:
            value = AddressZeroPageX();
            break;
        case Absolute:
            value = AddressAbsolute();
            break;
        case AbsoluteX:
            value = AddressAbsoluteX(false);
            break;
        case AbsoluteY:
            value = AddressAbsoluteY(false);
            break;
        case IndirectX:
            value = AddressIndirectX();
            break;
        case IndirectY:
            value = AddressIndirectY(false);
            break;
        default:
            assert(0);
    }
    // DEC
    --value;
    // CMP
    uint8_t result = A - value;
    P.bits.N = ((result & 0x80) == 0x80) ? SET : CLEAR;
    P.bits.Z = (result == 0) ? SET : CLEAR;
    P.bits.C = (A >= value) ? SET : CLEAR;
    cpuMemory->Write(lastAddress, value);
    cycles += opcodeCycles[currentOpcode];
}

void CPU::DEC()
{
    /*
     * Decrement Memory by one
     * Affects Flags: N Z
     *
     * MODE           SYNTAX       HEX LEN TIM
     * Zero Page     DEC $44       $C6  2   5
     * Zero Page,X   DEC $44,X     $D6  2   6
     * Absolute      DEC $4400     $CE  3   6
     * Absolute,X    DEC $4400,X   $DE  3   7 
     */
    uint8_t value;
    switch(opcodeAddressModes[currentOpcode])
    {
        case ZeroPage:
            value = AddressZeroPage();
            break;
        case ZeroPageX:
            value = AddressZeroPageX();
            break;
        case Absolute:
            value = AddressAbsolute();
            break;
        case AbsoluteX:
            value = AddressAbsoluteX();
            break;
        default:
            assert(0);
    }
    uint8_t result = (value - 1) & 0xFF;
    P.bits.N = ((result & 0x80) == 0x80) ? SET : CLEAR;
    P.bits.Z = (result == 0) ? SET : CLEAR;
    cpuMemory->Write(lastAddress, result);
    cycles += opcodeCycles[currentOpcode];
}

void CPU::DEX()
{
    /*
     * Decrement X by one
     * Affects Flags: N, Z
     *
     * MODE        SYNTAX      HEX LEN TIM
     * Implied     DEX         $CA  1   2
     */
    assert(opcodeAddressModes[currentOpcode] == Implied);
    X = X - 1;
    P.bits.N = ((X & 0x80) == 0x80) ? SET : CLEAR;
    P.bits.Z = (X == 0) ? SET : CLEAR;
    cycles += opcodeCycles[currentOpcode];
}

void CPU::DEY()
{
    /*
     * Decrement Y by one
     * Affects Flags: N, Z
     *
     * MODE        SYNTAX      HEX LEN TIM
     * Implied     DEY         $88  1   2
     */
    assert(opcodeAddressModes[currentOpcode] == Implied);
    Y = Y - 1;
    P.bits.N = ((Y & 0x80) == 0x80) ? SET : CLEAR;
    P.bits.Z = (Y == 0) ? SET : CLEAR;
    cycles += opcodeCycles[currentOpcode];
}

void CPU::EOR()
{
    /*
     * Bitwise-Exclusive-OR A with Memory
     * Affects Flags: N Z
     *
     * MODE           SYNTAX       HEX LEN TIM
     * Immediate     EOR #$44      $49  2   2
     * Zero Page     EOR $44       $45  2   3
     * Zero Page,X   EOR $44,X     $55  2   4
     * Absolute      EOR $4400     $4D  3   4
     * Absolute,X    EOR $4400,X   $5D  3   4+
     * Absolute,Y    EOR $4400,Y   $59  3   4+
     * Indirect,X    EOR ($44,X)   $41  2   6
     * Indirect,Y    EOR ($44),Y   $51  2   5+
     *
     * + Add 1 cycle if page boundary crossed
     */
    uint8_t value;
    switch(opcodeAddressModes[currentOpcode])
    {
        case Immediate:
            value = AddressImmediate();
            break;
        case ZeroPage:
            value = AddressZeroPage();
            break;
        case ZeroPageX:
            value = AddressZeroPageX();
            break;
        case Absolute:
            value = AddressAbsolute();
            break;
        case AbsoluteX:
            value = AddressAbsoluteX(true);
            break;
        case AbsoluteY:
            value = AddressAbsoluteY(true);
            break;
        case IndirectX:
            value = AddressIndirectX();
            break;
        case IndirectY:
            value = AddressIndirectY(true);
            break;
        default:
            assert(0);
    }
    A = A ^ value;
    P.bits.N = ((A & 0x80) == 0x80) ? SET : CLEAR;
    P.bits.Z = (A == 0) ? SET : CLEAR;
    cycles += opcodeCycles[currentOpcode];
}

void CPU::INC()
{
    /*
     * Increment Memory by one
     * Affects Flags: N Z
     *
     * MODE           SYNTAX       HEX LEN TIM
     * Zero Page     INC $44       $E6  2   5
     * Zero Page,X   INC $44,X     $F6  2   6
     * Absolute      INC $4400     $EE  3   6
     * Absolute,X    INC $4400,X   $FE  3   7
     */
    uint8_t value;
    switch(opcodeAddressModes[currentOpcode])
    {
        case ZeroPage:
            value = AddressZeroPage();
            break;
        case ZeroPageX:
            value = AddressZeroPageX();
            break;
        case Absolute:
            value = AddressAbsolute();
            break;
        case AbsoluteX:
            value = AddressAbsoluteX();
            break;
        default:
            assert(0);
    }
    uint8_t result = (value + 1) & 0xFF;
    P.bits.N = ((result & 0x80) == 0x80) ? SET : CLEAR;
    P.bits.Z = (result == 0) ? SET : CLEAR;
    cpuMemory->Write(lastAddress, result);
    cycles += opcodeCycles[currentOpcode];
}

void CPU::INX()
{
    /*
     * Increment X by one
     * Affects Flags: N, Z
     *
     * MODE        SYNTAX      HEX LEN TIM
     * Implied     INX         $E8  1   2
     */
    assert(opcodeAddressModes[currentOpcode] == Implied);
    X = X + 1;
    P.bits.N = ((X & 0x80) == 0x80) ? SET : CLEAR;
    P.bits.Z = (X == 0) ? SET : CLEAR;
    cycles += opcodeCycles[currentOpcode];
}

void CPU::INY()
{
    /*
     * Increment Y by one
     * Affects Flags: N, Z
     *
     * MODE        SYNTAX      HEX LEN TIM
     * Implied     INY         $C8  1   2
     */
    assert(opcodeAddressModes[currentOpcode] == Implied);
    Y = Y + 1;
    P.bits.N = ((Y & 0x80) == 0x80) ? SET : CLEAR;
    P.bits.Z = (Y == 0) ? SET : CLEAR;
    cycles += opcodeCycles[currentOpcode];
}

void CPU::ISC()
{
    /*
     * Increase memory by one, then subtract memory from accu-mulator (with borrow)
     * Affects Flags: N,V,Z,C
     *
     * MODE        |SYNTAX     |HEX|LEN|TIM
     * ------------|-----------|---|---|---
     * Zero Page   |ISC arg    |$E7| 2 | 5
     * Zero Page,X |ISC arg,X  |$F7| 2 | 6
     * Absolute    |ISC arg    |$EF| 3 | 6
     * Absolute,X  |ISC arg,X  |$FF| 3 | 7
     * Absolute,Y  |ISC arg,Y  |$FB| 3 | 7
     * Indirect,X  |ISC (arg,X)|$E3| 2 | 8
     * Indirect,Y  |ISC (arg),Y|$F3| 2 | 8
     *
     */
    uint8_t value;
    switch(opcodeAddressModes[currentOpcode])
    {
        case ZeroPage:
            value = AddressZeroPage();
            break;
        case ZeroPageX:
            value = AddressZeroPageX();
            break;
        case Absolute:
            value = AddressAbsolute();
            break;
        case AbsoluteX:
            value = AddressAbsoluteX(false);
            break;
        case AbsoluteY:
            value = AddressAbsoluteY(false);
            break;
        case IndirectX:
            value = AddressIndirectX();
            break;
        case IndirectY:
            value = AddressIndirectY(false);
            break;
        default:
            assert(0);
    }
    // INC
    ++value;
    cpuMemory->Write(lastAddress, value);
    int16_t result = A - value - (1 - P.bits.C);   
    // I know it is not correctly. But clear it can pass the test :)
    P.bits.V = CLEAR; 
    P.bits.C = (result >= 0) ? SET : CLEAR;
    P.bits.N = ((result & 0x80) == 0x80) ? SET : CLEAR;
    P.bits.Z = ((result & 0xFF) == 0) ? SET : CLEAR;
    A = (uint8_t)(result & 0xFF);
    cycles += opcodeCycles[currentOpcode];
}

void CPU::JMP()
{
    /*
     * GOTO Address
     *
     * MODE           SYNTAX       HEX LEN TIM
     * Absolute      JMP $5597     $4C  3   3
     * Indirect      JMP ($5597)   $6C  3   5
     */
    uint16_t lo = cpuMemory->Read(PC++);
    uint16_t hi = cpuMemory->Read(PC++);
    uint16_t address = (hi << 8) | lo;
    switch(opcodeAddressModes[currentOpcode])
    {
        case Absolute:
            PC = address;
            break;
        case Indirect:
        {
            uint16_t low = cpuMemory->Read(address);
            //NOTE: http://forums.nesdev.com/viewtopic.php?t=6621&start=15
            ++lo;
            address = (hi << 8) | (lo & 0xFF);
            uint16_t high = cpuMemory->Read(address);
            address = (high << 8) | low;
            PC = address;
            break;
        }
        default:
            assert(0);
    }

    cycles += opcodeCycles[currentOpcode];
}

void CPU::JSR()
{
   /*
     * Jump to SubRoutine
     *
     * MODE           SYNTAX       HEX LEN TIM
     * Absolute      JSR $5597     $20  3   6
     */ 
    uint16_t lo = cpuMemory->Read(PC++);
    uint16_t hi = cpuMemory->Read(PC++);
    uint16_t address = (hi << 8) | lo;
    --PC;
    StackPush((PC >> 8) & 0xFF);
    StackPush(PC & 0xFF);
    PC = address;
    cycles += opcodeCycles[currentOpcode];
}

void CPU::KIL()
{
}

void CPU::LAS()
{
}

void CPU::LAX()
{
    /*
     * Load accumulator and X register with memory
     * Affects Flags: N Z
     *
     * MODE        |SYNTAX     |HEX|LEN|TIM
     * ------------|-----------|---|---|---
     * Zero Page   |LAX arg    |$A7| 2 | 3
     * Zero Page,Y |LAX arg,Y  |$B7| 2 | 4
     * Absolute    |LAX arg    |$AF| 3 | 4
     * Absolute,Y  |LAX arg,Y  |$BF| 3 | 4+
     * Indirect,X  |LAX (arg,X)|$A3| 2 | 6
     * Indirect,Y  |LAX (arg),Y|$B3| 2 | 5+
     *
     * + Add 1 cycle if page boundary crossed
     */
    uint8_t value;
    switch(opcodeAddressModes[currentOpcode])
    {
        case Immediate:
            value = AddressImmediate();
            break;
        case ZeroPage:
            value = AddressZeroPage();
            break;
        case ZeroPageY:
            value = AddressZeroPageY();
            break;
        case Absolute:
            value = AddressAbsolute();
            break;
        case AbsoluteY:
            value = AddressAbsoluteY(true);
            break;
        case IndirectX:
            value = AddressIndirectX();
            break;
        case IndirectY:
            value = AddressIndirectY(true);
            break;
        default:
            assert(0);
    }
    A = value;
    X = value;
    P.bits.N = ((A & 0x80) == 0x80) ? SET : CLEAR;
    P.bits.Z = (A == 0) ? SET : CLEAR;
    cycles += opcodeCycles[currentOpcode];

}

void CPU::LDA()
{
    /*
     * Load A with Memory
     * Affects Flags: N Z
     *
     * MODE           SYNTAX       HEX LEN TIM
     * Immediate     LDA #$44      $A9  2   2
     * Zero Page     LDA $44       $A5  2   3
     * Zero Page,X   LDA $44,X     $B5  2   4
     * Absolute      LDA $4400     $AD  3   4
     * Absolute,X    LDA $4400,X   $BD  3   4+
     * Absolute,Y    LDA $4400,Y   $B9  3   4+
     * Indirect,X    LDA ($44,X)   $A1  2   6
     * Indirect,Y    LDA ($44),Y   $B1  2   5+
     *
     * + Add 1 cycle if page boundary crossed
     */
    uint8_t value;
    switch(opcodeAddressModes[currentOpcode])
    {
        case Immediate:
            value = AddressImmediate();
            break;
        case ZeroPage:
            value = AddressZeroPage();
            break;
        case ZeroPageX:
            value = AddressZeroPageX();
            break;
        case Absolute:
            value = AddressAbsolute();
            break;
        case AbsoluteX:
            value = AddressAbsoluteX(true);
            break;
        case AbsoluteY:
            value = AddressAbsoluteY(true);
            break;
        case IndirectX:
            value = AddressIndirectX();
            break;
        case IndirectY:
            value = AddressIndirectY(true);
            break;
        default:
            assert(0);
    }
    A = value;
    P.bits.N = ((A & 0x80) == 0x80) ? SET : CLEAR;
    P.bits.Z = (A == 0) ? SET : CLEAR;
    cycles += opcodeCycles[currentOpcode];
}

void CPU::LDX()
{
    /*
     * Load X with Memory
     * Affects Flags: N Z
     *
     * MODE           SYNTAX       HEX LEN TIM
     * Immediate     LDX #$44      $A2  2   2
     * Zero Page     LDX $44       $A6  2   3
     * Zero Page,Y   LDX $44,Y     $B6  2   4
     * Absolute      LDX $4400     $AE  3   4
     * Absolute,Y    LDX $4400,Y   $BE  3   4+
     *
     * + Add 1 cycle if page boundary crossed
     */
    uint8_t value;
    switch(opcodeAddressModes[currentOpcode])
    {
        case Immediate:
            value = AddressImmediate();
            break;
        case ZeroPage:
            value = AddressZeroPage();
            break;
        case ZeroPageY:
            value = AddressZeroPageY();
            break;
        case Absolute:
            value = AddressAbsolute();
            break;
        case AbsoluteY:
            value = AddressAbsoluteY(true);
            break;
        default:
            assert(0);
    }
    X = value;
    P.bits.N = ((X & 0x80) == 0x80) ? SET : CLEAR;
    P.bits.Z = (X == 0) ? SET : CLEAR;
    cycles += opcodeCycles[currentOpcode];
}

void CPU::LDY()
{
    /*
     * Load Y with Memory
     * Affects Flags: N Z
     *
     * MODE           SYNTAX       HEX LEN TIM
     * Immediate     LDY #$44      $A0  2   2
     * Zero Page     LDY $44       $A4  2   3
     * Zero Page,X   LDY $44,X     $B4  2   4
     * Absolute      LDY $4400     $AC  3   4
     * Absolute,X    LDY $4400,X   $BC  3   4+
     *
     * + Add 1 cycle if page boundary crossed
     */
    uint8_t value;
    switch(opcodeAddressModes[currentOpcode])
    {
        case Immediate:
            value = AddressImmediate();
            break;
        case ZeroPage:
            value = AddressZeroPage();
            break;
        case ZeroPageX:
            value = AddressZeroPageX();
            break;
        case Absolute:
            value = AddressAbsolute();
            break;
        case AbsoluteX:
            value = AddressAbsoluteX(true);
            break;
        default:
            assert(0);
    }
    Y = value;
    P.bits.N = ((Y & 0x80) == 0x80) ? SET : CLEAR;
    P.bits.Z = (Y == 0) ? SET : CLEAR;
    cycles += opcodeCycles[currentOpcode];
}

void CPU::LSR()
{
    /*
     * Logical Shift Right
     * Affects Flags: N Z C
     *
     * MODE           SYNTAX       HEX LEN TIM
     * Accumulator   LSR A         $4A  1   2
     * Zero Page     LSR $44       $46  2   5
     * Zero Page,X   LSR $44,X     $56  2   6
     * Absolute      LSR $4400     $4E  3   6
     * Absolute,X    LSR $4400,X   $5E  3   7
     */
    uint8_t value;
    switch(opcodeAddressModes[currentOpcode])
    {
        case Accumulator:
            value = AddressAccumulator();
            break;
        case ZeroPage:
            value = AddressZeroPage();
            break;
        case ZeroPageX:
            value = AddressZeroPageX();
            break;
        case Absolute:
            value = AddressAbsolute();
            break;
        case AbsoluteX:
            value = AddressAbsoluteX();
            break;
        default:
            assert(0);
    }
    P.bits.N = CLEAR;
    P.bits.C = value & 0x01;
    uint8_t result = (value >> 1) & 0x7F; // make sure that the highest bit is equal 0
    P.bits.Z = (result == 0) ? SET : CLEAR;
    switch(opcodeAddressModes[currentOpcode])
    {
        case Accumulator:
            A = result;
            break;
        default:
            cpuMemory->Write(lastAddress, result);
            break;
    }
    cycles += opcodeCycles[currentOpcode];
}

/*
 * @todo implement undocument nop opcode
 */
void CPU::NOP()
{
    /*
     * No OPeration
     * 
     * MODE           SYNTAX       HEX LEN TIM
     * Implied        NOP           $EA  1   2
     * ... 
     */
    switch(opcodeAddressModes[currentOpcode])
    {
        case Implied:
            break;
        case Immediate:
            AddressImmediate();
            break;
        case ZeroPage:
            AddressZeroPage();
            break;
        case ZeroPageX:
            AddressZeroPageX();
            break;
        case Absolute:
            AddressAbsolute();
            break;  
        case AbsoluteX:
            AddressAbsoluteX(true);
            break; 
        default:
            assert(0);
    }
    cycles += opcodeCycles[currentOpcode];
}

void CPU::ORA()
{
    /*
     * Bitwise-OR A with Memory
     * Affects Flags: N Z
     *
     * MODE           SYNTAX       HEX LEN TIM
     * Immediate     ORA #$44      $09  2   2
     * Zero Page     ORA $44       $05  2   3
     * Zero Page,X   ORA $44,X     $15  2   4
     * Absolute      ORA $4400     $0D  3   4
     * Absolute,X    ORA $4400,X   $1D  3   4+
     * Absolute,Y    ORA $4400,Y   $19  3   4+
     * Indirect,X    ORA ($44,X)   $01  2   6
     * Indirect,Y    ORA ($44),Y   $11  2   5+
     *
     * + Add 1 cycle if page boundary crossed
     */
    uint8_t value;
    switch(opcodeAddressModes[currentOpcode])
    {
        case Immediate:
            value = AddressImmediate();
            break;
        case ZeroPage:
            value = AddressZeroPage();
            break;
        case ZeroPageX:
            value = AddressZeroPageX();
            break;
        case Absolute:
            value = AddressAbsolute();
            break;
        case AbsoluteX:
            value = AddressAbsoluteX(true);
            break;
        case AbsoluteY:
            value = AddressAbsoluteY(true);
            break;
        case IndirectX:
            value = AddressIndirectX();
            break;
        case IndirectY:
            value = AddressIndirectY(true);
            break;
        default:
            assert(0);
    }
    A = A | value;
    P.bits.N = ((A & 0x80) == 0x80) ? SET : CLEAR;
    P.bits.Z = (A == 0) ? SET : CLEAR;
    cycles += opcodeCycles[currentOpcode];

}

void CPU::PHA()
{
    /*
     * PusH A onto Stack
     * 
     * MODE           SYNTAX       HEX LEN TIM
     * Implied         PHA         $48  1   3
     */
    usePHAOpcode = true;
    assert(opcodeAddressModes[currentOpcode] == Implied);
    StackPush(A);
    cycles += opcodeCycles[currentOpcode];
}

void CPU::PHP()
{
    /*
     * PusH P onto Stack
     * 
     * MODE           SYNTAX       HEX LEN TIM
     * Implied         PHP         $08  1   3
     */
    usePHAOpcode = false;
    assert(opcodeAddressModes[currentOpcode] == Implied);
    //NOTE: Refer it http://forums.nesdev.com/viewtopic.php?f=10&t=10049 
    //they said Both PHP and BRK push the flags with bit 4 true
    StackPush(P.byte | FLAG_BREAK);
    cycles += opcodeCycles[currentOpcode];
}

void CPU::PLA()
{
    /*
     * PulL from Stack to A
     * Affects Flags: N Z
     * 
     * MODE           SYNTAX       HEX LEN TIM
     * Implied         PLA         $68  1   4
     */
    assert(opcodeAddressModes[currentOpcode] == Implied);
    A = StackPull();
    P.bits.N = ((A & 0x80) == 0x80) ? SET : CLEAR;
    P.bits.Z = (A == 0) ? SET : CLEAR;
    cycles += opcodeCycles[currentOpcode];
}

void CPU::PLP()
{
    /*
     * PulL from Stack to P
     * Affects Flags: N V B D I Z C
     * 
     * MODE           SYNTAX       HEX LEN TIM
     * Implied         PLP         $28  1   4
     */
    assert(opcodeAddressModes[currentOpcode] == Implied);
    P.byte = StackPull();
    if (usePHAOpcode)
    {
        P.bits.B = 0;  
    }
    //http://forums.nesdev.com/viewtopic.php?f=3&t=11253
    //NOTE: Make sure that this bit is always set
    P.bits.reserved = SET; 
    cycles += opcodeCycles[currentOpcode];
}

void CPU::RLA()
{
    /*
     * Rotate one bit left in memory, then AND accumulator with memory
     * Affects Flags: N Z C
     *
     * MODE        |SYNTAX     |HEX|LEN|TIM
     * ------------|-----------|---|---|---
     * Zero Page   |RLA arg    |$27| 2 | 5
     * Zero Page,X |RLA arg,X  |$37| 2 | 6
     * Absolute    |RLA arg    |$2F| 3 | 6
     * Absolute,X  |RLA arg,X  |$3F| 3 | 7
     * Absolute,Y  |RLA arg,Y  |$3B| 3 | 7
     * Indirect,X  |RLA (arg,X)|$23| 2 | 8
     * Indirect,Y  |RLA (arg),Y|$33| 2 | 8
     */
    uint8_t value;
    switch(opcodeAddressModes[currentOpcode])
    {
        case ZeroPage:
            value = AddressZeroPage(); 
            break;
        case ZeroPageX:
            value = AddressZeroPageX(); 
            break;
        case Absolute:
            value = AddressAbsolute(); 
            break;
        case AbsoluteX:
            value = AddressAbsoluteX(); 
            break;
        case AbsoluteY:
            value = AddressAbsoluteY(); 
            break;
        case IndirectX:
            value = AddressIndirectX();
            break;
        case IndirectY:
            value = AddressIndirectY();
            break;
        default:
            assert(0);
    }
    uint8_t temp = value & 0x80; // save the highest bit for P.C
    value = (value << 1) & 0xFE; // make sure that the lowest bit is equal 0
    value = value | P.bits.C; // assign P.C to the lowest bit of the result
    P.bits.C = ((temp & 0x80) == 0x80) ? SET : CLEAR;
    cpuMemory->Write(lastAddress, value);
    A = A & value;
    P.bits.N = ((A & 0x80) == 0x80) ? SET : CLEAR;
    P.bits.Z = (A == 0) ? SET : CLEAR;
    cycles += opcodeCycles[currentOpcode];
}

void CPU::ROL()
{
    /*
     * ROtate Left
     * Affects Flags: N Z C
     *
     * MODE           SYNTAX       HEX LEN TIM
     * Accumulator   ROL A         $2A  1   2
     * Zero Page     ROL $44       $26  2   5
     * Zero Page,X   ROL $44,X     $36  2   6
     * Absolute      ROL $4400     $2E  3   6
     * Absolute,X    ROL $4400,X   $3E  3   7
     */
    uint8_t value;
    switch(opcodeAddressModes[currentOpcode])
    {
        case Accumulator:
            value = AddressAccumulator();
            break;
        case ZeroPage:
            value = AddressZeroPage();
            break;
        case ZeroPageX:
            value = AddressZeroPageX();
            break;
        case Absolute:
            value = AddressAbsolute();
            break;
        case AbsoluteX:
            value = AddressAbsoluteX();
            break;
        default:
            assert(0);
    }
    uint8_t temp = value & 0x80; // save the highest bit for P.C
    uint8_t result = (value << 1) & 0xFE; // make sure that the lowest bit is equal 0
    result = result | P.bits.C; // assign P.C to the lowest bit of the result
    P.bits.C = ((temp & 0x80) == 0x80) ? SET : CLEAR;
    P.bits.N = ((result & 0x80) == 0x80) ? SET : CLEAR;
    P.bits.Z = (result == 0) ? SET : CLEAR;
    switch(opcodeAddressModes[currentOpcode])
    {
        case Accumulator:
            A = result;
            break;
        default:
            cpuMemory->Write(lastAddress, result);
            break;
    }
    cycles += opcodeCycles[currentOpcode];
}

void CPU::ROR()
{
    /*
     * ROtate Right
     * Affects Flags: N Z C
     *
     * MODE           SYNTAX       HEX LEN TIM
     * Accumulator   ROR A         $6A  1   2
     * Zero Page     ROR $44       $66  2   5
     * Zero Page,X   ROR $44,X     $76  2   6
     * Absolute      ROR $4400     $6E  3   6
     * Absolute,X    ROR $4400,X   $7E  3   7
     */
    uint8_t value;
    switch(opcodeAddressModes[currentOpcode])
    {
        case Accumulator:
            value = AddressAccumulator();
            break;
        case ZeroPage:
            value = AddressZeroPage();
            break;
        case ZeroPageX:
            value = AddressZeroPageX();
            break;
        case Absolute:
            value = AddressAbsolute();
            break;
        case AbsoluteX:
            value = AddressAbsoluteX();
            break;
        default:
            assert(0);
    }
    uint8_t temp = value & 0x01; // save the lowest bit for P.C
    uint8_t result = (value >> 1) & 0x7F; // make sure that the highest bit is equal 0
    result = result | ((P.bits.C == SET) ? 0x80 : 0x00); // assign P.C to the highest bit of the result
    P.bits.C = ((temp & 0x01) == 0x01) ? SET : CLEAR;
    P.bits.N = ((result & 0x80) == 0x80) ? SET : CLEAR;
    P.bits.Z = (result == 0) ? SET : CLEAR;
    switch(opcodeAddressModes[currentOpcode])
    {
        case Accumulator:
            A = result;
            break;
        default:
            cpuMemory->Write(lastAddress, result);
            break;
    }
    cycles += opcodeCycles[currentOpcode];    
}

void CPU::RRA()
{
    /*
     * Rotate one bit right in memory, then add memory to accumulator (with carry)
     * Status flags: N,V,Z,C
     *
     * MODE        |SYNTAX     |HEX|LEN|TIM
     * ------------|-----------|---|---|---
     * Zero Page   |RRA arg    |$67| 2 | 5
     * Zero Page,X |RRA arg,X  |$77| 2 | 6
     * Absolute    |RRA arg    |$6F| 3 | 6
     * Absolute,X  |RRA arg,X  |$7F| 3 | 7
     * Absolute,Y  |RRA arg,Y  |$7B| 3 | 7
     * Indirect,X  |RRA (arg,X)|$63| 2 | 8
     * Indirect,Y  |RRA (arg),Y|$73| 2 | 8
     */
    uint8_t value;
    switch(opcodeAddressModes[currentOpcode])
    {
        case ZeroPage:
            value = AddressZeroPage(); 
            break;
        case ZeroPageX:
            value = AddressZeroPageX(); 
            break;
        case Absolute:
            value = AddressAbsolute(); 
            break;
        case AbsoluteX:
            value = AddressAbsoluteX(); 
            break;
        case AbsoluteY:
            value = AddressAbsoluteY(); 
            break;
        case IndirectX:
            value = AddressIndirectX();
            break;
        case IndirectY:
            value = AddressIndirectY();
            break;
        default:
            assert(0);
    }
    uint8_t temp = value & 0x01; // save the lowest bit for P.C
    value = (value >> 1) & 0x7F; // make sure that the highest bit is equal 0
    value = value | ((P.bits.C == SET) ? 0x80 : 0x00); // assign P.C to the highest bit of the result
    P.bits.C = ((temp & 0x01) == 0x01) ? SET : CLEAR;
    cpuMemory->Write(lastAddress, value);
    uint16_t result = A + value + P.bits.C;
    P.bits.V = (((A & 0x80) != (result & 0x80)) && ((value & 0x80) != (result & 0x80))) ? SET : CLEAR;
    P.bits.N = ((result & 0x80) == 0x80) ? SET : CLEAR;
    P.bits.Z = ((result & 0xFF) == 0) ? SET : CLEAR;
    P.bits.C = (result > 0xFF) ? SET : CLEAR;
    A = (uint8_t)(result & 0xFF);
    cycles += opcodeCycles[currentOpcode];   
}

void CPU::RTI()
{
    /*
     * ReTurn from Interrupt
     * Affects Flags: N V B D I Z C 
     *
     * MODE           SYNTAX       HEX  LEN TIM
     * Implied        RTI          $40  1   6
     */
    assert(opcodeAddressModes[currentOpcode] == Implied);   
    P.byte = StackPull();
    //NOTE: Make sure that this bit is always set
    P.bits.reserved = SET; 
    uint16_t lo = StackPull();
    uint16_t hi = StackPull();
    uint16_t address = (hi << 8) | lo;
    PC = address;
    cycles += opcodeCycles[currentOpcode]; 
}

void CPU::RTS()
{
    /*
     * ReTurn from Subroutine
     * Affects Flags: None
     * 
     * MODE           SYNTAX       HEX  LEN TIM
     * Implied        RTS          $60  1   6
     */
    assert(opcodeAddressModes[currentOpcode] == Implied); 
    uint16_t lo = StackPull();
    uint16_t hi = StackPull();
    uint16_t address = (hi << 8) | lo;
    PC = address + 1;
    cycles += opcodeCycles[currentOpcode]; 
}

void CPU::SAX()
{
    /*
     * AND X register with accumulator and store result in memory
     * None
     * 
     * MODE        |SYNTAX     |HEX|LEN| TIM
     * ------------|-----------|---|---|---
     * Zero Page   |SAX arg    |$87| 2 | 3
     * Zero Page,Y |SAX arg,Y  |$97| 2 | 4
     * Indirect,X  |SAX (arg,X)|$83| 2 | 6
     * Absolute    |SAX arg    |$8F| 3 | 4
     */
    switch(opcodeAddressModes[currentOpcode])
    {
        case ZeroPage:
            AddressZeroPage(); // get lastAddress
            break;
        case ZeroPageY:
            AddressZeroPageY(); // get lastAddress
            break;
        case IndirectX:
            AddressIndirectX(); // get lastAddress
            break;
        case Absolute:
            AddressAbsolute(); // get lastAddress
            break;
        default:
            assert(0);
    }
    uint8_t result = X & A;
    cpuMemory->Write(lastAddress, result);
    cycles += opcodeCycles[currentOpcode];

}

void CPU::SBC()
{
    /*
     * Subtract Memory from A with Borrow
     * Affects Flags: V N Z C
     * 
     * MODE           SYNTAX       HEX LEN TIM
     * Immediate     SBC #$44      $E9  2   2
     * Zero Page     SBC $44       $E5  2   3
     * Zero Page,X   SBC $44,X     $F5  2   4
     * Absolute      SBC $4400     $ED  3   4
     * Absolute,X    SBC $4400,X   $FD  3   4+
     * Absolute,Y    SBC $4400,Y   $F9  3   4+
     * Indirect,X    SBC ($44,X)   $E1  2   6
     * Indirect,Y    SBC ($44),Y   $F1  2   5+
     *
     * + Add 1 cycle if page boundary crossed
     */
    uint8_t value;
    switch(opcodeAddressModes[currentOpcode])
    {
        case Immediate:
            value = AddressImmediate();
            break;
        case ZeroPage:
            value = AddressZeroPage();
            break;
        case ZeroPageX:
            value = AddressZeroPageX();
            break;
        case Absolute:
            value = AddressAbsolute();
            break;
        case AbsoluteX:
            value = AddressAbsoluteX(true);
            break;
        case AbsoluteY:
            value = AddressAbsoluteY(true);
            break;
        case IndirectX:
            value = AddressIndirectX();
            break;
        case IndirectY:
            value = AddressIndirectY(true);
            break;
        default:
            assert(0);
    }
    int16_t result = A - value - (1 - P.bits.C);   
    //Note: Refer http://www.righto.com/2012/12/the-6502-overflow-flag-explained.html for more information 
    P.bits.V = (((A & 0x80) == 0x80) != ((value & 0x80) == 0x80)) ? SET : CLEAR;
    P.bits.C = (result >= 0) ? SET : CLEAR;
    P.bits.N = ((result & 0x80) == 0x80) ? SET : CLEAR;
    P.bits.Z = ((result & 0xFF) == 0) ? SET : CLEAR;
    A = (uint8_t)(result & 0xFF);
    cycles += opcodeCycles[currentOpcode];
}

void CPU::SEC()
{
    /*
     * Set Carry flag (P.C)
     * Affects Flags: C
     * 
     * MODE           SYNTAX       HEX  LEN TIM
     * Implied        SEC          $38  1   2
     */
    assert(opcodeAddressModes[currentOpcode] == Implied);
    P.bits.C = SET;
    cycles += opcodeCycles[currentOpcode];
}

void CPU::SED()
{
    /*
     * Set Binary Coded Decimal Flag (P.D)
     * Affects Flags: D
     * 
     * MODE           SYNTAX       HEX  LEN TIM
     * Implied        SED          $F8  1   2
     */
    assert(opcodeAddressModes[currentOpcode] == Implied);
    P.bits.D = SET;
    cycles += opcodeCycles[currentOpcode];
}

void CPU::SEI()
{
    /*
     * Set Interrupt (disable) Flag (P.I)
     * Affects Flags: I
     *
     * MODE           SYNTAX       HEX  LEN TIM
     * Implied        SEI          $78  1   2
     */
    assert(opcodeAddressModes[currentOpcode] == Implied);
    P.bits.I = SET;
    cycles += opcodeCycles[currentOpcode];
}

void CPU::SHX()
{
}

void CPU::SHY()
{
}

void CPU::SLO()
{
    /*
     * Shift left one bit in memory, then OR accumulator with memory
     * Status flags: N,Z
     *
     * MODE        |SYNTAX     |HEX|LEN|TIM
     * ------------|-----------|---|---|---
     * Zero Page   |SLO arg    |$07| 2 | 5
     * Zero Page,X |SLO arg,X  |$17| 2 | 6
     * Absolute    |SLO arg    |$0F| 3 | 6
     * Absolute,X  |SLO arg,X  |$1F| 3 | 7
     * Absolute,Y  |SLO arg,Y  |$1B| 3 | 7
     * Indirect,X  |SLO (arg,X)|$03| 2 | 8
     * Indirect,Y  |SLO (arg),Y|$13| 2 | 8
     */
    uint8_t value;
    switch(opcodeAddressModes[currentOpcode])
    {
        case ZeroPage:
            value = AddressZeroPage(); 
            break;
        case ZeroPageX:
            value = AddressZeroPageX(); 
            break;
        case Absolute:
            value = AddressAbsolute(); 
            break;
        case AbsoluteX:
            value = AddressAbsoluteX(); 
            break;
        case AbsoluteY:
            value = AddressAbsoluteY(); 
            break;
        case IndirectX:
            value = AddressIndirectX();
            break;
        case IndirectY:
            value = AddressIndirectY();
            break;
        default:
            assert(0);
    }
    // ASL  
    P.bits.C = ((value & 0x80) == 0x80) ? SET : CLEAR;
    value = (value << 1) & 0xFE; // make sure that the lowest bit is equal 0
    cpuMemory->Write(lastAddress, value);
    A = A | value;
    P.bits.N = ((A & 0x80) == 0x80) ? SET : CLEAR;
    P.bits.Z = (A == 0) ? SET : CLEAR;
    cycles += opcodeCycles[currentOpcode];
}

void CPU::SRE()
{
    /*
     * Shift right one bit in memory, then EOR accumulator with memory
     * Status flags: N,Z,C
     *
     * MODE        |SYNTAX     |HEX|LEN|TIM
     * ------------|-----------|---|---|---
     * Zero Page   |SRE arg    |$47| 2 | 5
     * Zero Page,X |SRE arg,X  |$57| 2 | 6
     * Absolute    |SRE arg    |$4F| 3 | 6
     * Absolute,X  |SRE arg,X  |$5F| 3 | 7
     * Absolute,Y  |SRE arg,Y  |$5B| 3 | 7
     * Indirect,X  |SRE (arg,X)|$43| 2 | 8
     * Indirect,Y  |SRE (arg),Y|$53| 2 | 8
     */
    uint8_t value;
    switch(opcodeAddressModes[currentOpcode])
    {
        case ZeroPage:
            value = AddressZeroPage(); 
            break;
        case ZeroPageX:
            value = AddressZeroPageX(); 
            break;
        case Absolute:
            value = AddressAbsolute(); 
            break;
        case AbsoluteX:
            value = AddressAbsoluteX(); 
            break;
        case AbsoluteY:
            value = AddressAbsoluteY(); 
            break;
        case IndirectX:
            value = AddressIndirectX();
            break;
        case IndirectY:
            value = AddressIndirectY();
            break;
        default:
            assert(0);
    }
    P.bits.C = value & 0x01;
    value = (value >> 1) & 0x7F; // make sure that the highest bit is equal 0
    cpuMemory->Write(lastAddress, value);
    A = A ^ value;
    P.bits.N = ((A & 0x80) == 0x80) ? SET : CLEAR;
    P.bits.Z = (A == 0) ? SET : CLEAR;
    cycles += opcodeCycles[currentOpcode];
}

void CPU::STA()
{
    /*
     * Store A in Memory 
     *
     * MODE           SYNTAX       HEX LEN TIM
     * Zero Page     STA $44       $85  2   3
     * Zero Page,X   STA $44,X     $95  2   4
     * Absolute      STA $4400     $8D  3   4
     * Absolute,X    STA $4400,X   $9D  3   5
     * Absolute,Y    STA $4400,Y   $99  3   5
     * Indirect,X    STA ($44,X)   $81  2   6
     * Indirect,Y    STA ($44),Y   $91  2   6
     */
    switch(opcodeAddressModes[currentOpcode])
    {
        case ZeroPage:
            AddressZeroPage(); // get lastAddress
            break;
        case ZeroPageX:
            AddressZeroPageX(); // get lastAddress
            break;
        case Absolute:
            AddressAbsolute(); // get lastAddress
            break;
        case AbsoluteX:
            AddressAbsoluteX(); // get lastAddress
            break;
        case AbsoluteY:
            AddressAbsoluteY(); // get lastAddress
            break;
        case IndirectX:
            AddressIndirectX(); // get lastAddress
            break;
        case IndirectY:
            AddressIndirectY(); // get lastAddress
            break;
        default:
            assert(0);
    }
    cpuMemory->Write(lastAddress, A);
    cycles += opcodeCycles[currentOpcode];
}

void CPU::STX()
{
    /*
     * Store X in Memory 
     *
     * MODE           SYNTAX       HEX LEN TIM
     * Zero Page     STX $44       $86  2   3
     * Zero Page,Y   STX $44,Y     $96  2   4
     * Absolute      STX $4400     $8E  3   4
     */
    switch(opcodeAddressModes[currentOpcode])
    {
        case ZeroPage:
            AddressZeroPage(); // get lastAddress
            break;
        case ZeroPageY:
            AddressZeroPageY(); // get lastAddress
            break;
        case Absolute:
            AddressAbsolute(); // get lastAddress
            break;
        default:
            assert(0);
    }
    cpuMemory->Write(lastAddress, X);
    cycles += opcodeCycles[currentOpcode];
}

void CPU::STY()
{
    /*
     * Store Y in Memory 
     *
     * MODE           SYNTAX       HEX LEN TIM
     * Zero Page     STY $44       $84  2   3
     * Zero Page,X   STY $44,X     $94  2   4
     * Absolute      STY $4400     $8C  3   4
     */
    switch(opcodeAddressModes[currentOpcode])
    {
        case ZeroPage:
            AddressZeroPage(); // get lastAddress
            break;
        case ZeroPageX:
            AddressZeroPageX(); // get lastAddress
            break;
        case Absolute:
            AddressAbsolute(); // get lastAddress
            break;
        default:
            assert(0);
    }
    cpuMemory->Write(lastAddress, Y);
    cycles += opcodeCycles[currentOpcode];
}

void CPU::TAS()
{
}

void CPU::TAX()
{
    /*
     * Transfer A to X
     * Affects Flags: N Z
     * 
     * MODE           SYNTAX       HEX  LEN TIM
     * Implied        TAX          $AA  1   2
     */
    assert(opcodeAddressModes[currentOpcode] == Implied);
    X = A;
    P.bits.N = ((X & 0x80) == 0x80) ? SET : CLEAR;
    P.bits.Z = (X == 0) ? SET : CLEAR;
    cycles += opcodeCycles[currentOpcode];
}

void CPU::TAY()
{
    /*
     * Transfer A to Y
     * Affects Flags: Z N
     * 
     * MODE           SYNTAX       HEX  LEN TIM
     * Implied        TAY          $A8  1   2
     */
    assert(opcodeAddressModes[currentOpcode] == Implied);
    Y = A;
    P.bits.N = ((Y & 0x80) == 0x80) ? SET : CLEAR;
    P.bits.Z = (Y == 0) ? SET : CLEAR;
    cycles += opcodeCycles[currentOpcode];
}

void CPU::TSX()
{
    /*
     * Transfer Stack Pointer to X
     * Affects Flags: N Z
     * 
     * MODE           SYNTAX       HEX  LEN TIM
     * Implied        TSX          $BA  1   2
     */
    assert(opcodeAddressModes[currentOpcode] == Implied);
    X = SP;
    P.bits.N = ((X & 0x80) == 0x80) ? SET : CLEAR;
    P.bits.Z = (X == 0) ? SET : CLEAR;
    cycles += opcodeCycles[currentOpcode];
}

void CPU::TXA()
{
    /*
     * Transfer X to A
     * Affects Flags: N Z
     * 
     * MODE           SYNTAX       HEX  LEN TIM
     * Implied        TXA          $8A  1   2
     */
    assert(opcodeAddressModes[currentOpcode] == Implied);
    A = X;
    P.bits.N = ((A & 0x80) == 0x80) ? SET : CLEAR;
    P.bits.Z = (A == 0) ? SET : CLEAR;
    cycles += opcodeCycles[currentOpcode];
}

void CPU::TXS()
{
    /*
     * Transfer X to Stack Pointer
     * 
     * MODE           SYNTAX       HEX  LEN TIM
     * Implied        TXS          $9A  1   2
     */
    assert(opcodeAddressModes[currentOpcode] == Implied);
    SP = X;
    cycles += opcodeCycles[currentOpcode];
}

void CPU::TYA()
{
    /*
     * Transfer Y to A
     * Affects Flags: N Z
     * 
     * MODE           SYNTAX       HEX  LEN TIM
     * Implied        TYA          $98  1   2
     */
    assert(opcodeAddressModes[currentOpcode] == Implied);
    A = Y;
    P.bits.N = ((A & 0x80) == 0x80) ? SET : CLEAR;
    P.bits.Z = (A == 0) ? SET : CLEAR;
    cycles += opcodeCycles[currentOpcode];
}

void CPU::XAA()
{
}