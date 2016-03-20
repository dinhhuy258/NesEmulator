#include "CPU.h"
#include <assert.h>
#include "Platforms.h"

CPU::CPU()
{
    A = 0;
    X = 0;
    Y = 0;
    SP = 0xFF;
    PC = 0;
    P.byte = 0x34;
    cycles = 0;
    (this->*opcodeFunctions[0x61])();
}

// Address mode
uint8_t CPU::AddressAbsolute()
{
    // 6502 is little endian
    uint16_t lo = memory.Read(PC++);
    uint16_t hi = memory.Read(PC++);
    uint16_t address = (hi << 8) | lo;
    lastAddress = address;
    return memory.Read(address);
}

uint8_t CPU::AddressAbsoluteX(bool checkPage)
{
    // 6502 is little endian
    uint16_t lo = memory.Read(PC++);
    uint16_t hi = memory.Read(PC++);
    uint16_t address = (hi << 8) | lo;
    // If the result of Base+Index is greater than $FFFF, wrapping will occur.
    if (checkPage && ((address & 0xFF00) != ((address + X) & 0xFF00)))
    {
        // page cross
        ++cycles;
    }
    address = address + X;
    lastAddress = address;
    return memory.Read(address);    
}

uint8_t CPU::AddressAbsoluteY(bool checkPage)
{
    // 6502 is little endian
    uint16_t lo = memory.Read(PC++);
    uint16_t hi = memory.Read(PC++);
    uint16_t address = (hi << 8) | lo;
    // If the result of Base+Index is greater than $FFFF, wrapping will occur.
    if (checkPage && ((address & 0xFF00) != ((address + Y) & 0xFF00))) 
    {
        // page cross\
        ++cycles;
    }
    address = address + Y;
    lastAddress = address;
    return memory.Read(address);
}

uint8_t CPU::AddressAccumulator()
{
    return A;
}

uint8_t CPU::AddressImmediate()
{
    return PC++;
}

uint8_t CPU::AddressImplied()
{
    // not used
    return 0;
}

uint8_t CPU::AddressIndirectX()
{
    // 6502 is little endian
    uint16_t baseAddress = memory.Read(PC++);
    baseAddress = (baseAddress + X) & 0xFF;
    uint16_t lo = memory.Read(baseAddress);
    uint16_t hi = memory.Read((baseAddress + 1) & 0xFF);
    uint16_t address = (hi << 8) | lo;
    lastAddress = address;
    return memory.Read(address);
}

uint8_t CPU::AddressIndirect()
{
    // 6502 is little endian
    uint16_t baseAddress = memory.Read(PC++);
    uint16_t lo = memory.Read(baseAddress);
    uint16_t hi = memory.Read((baseAddress + 1) & 0xFF);
    uint16_t address = (hi << 8) | lo;
    lastAddress = address;
    return memory.Read(address);
}

uint8_t CPU::AddressIndirectY(bool checkPage)
{
    // 6502 is little endian
    uint16_t baseAddress = memory.Read(PC++);
    uint16_t lo = memory.Read(baseAddress);
    uint16_t hi = memory.Read((baseAddress + 1) & 0xFF);
    uint16_t address = (hi << 8) | lo;
    // If Base_Location+Index is greater than $FFFF, wrapping will occur.
    if (checkPage && ((address & 0xFF00) != ((address + Y) & 0xFF00)))
    {
        // page crossed
        ++cycles;
    }
    address = (address + Y);
    lastAddress = address;
    return memory.Read(address);
}

uint8_t CPU::AddressRelative()
{
    return memory.Read(PC++);
}

uint8_t CPU::AddressZeroPage()
{
    uint16_t address = memory.Read(PC++);
    address &= 0x00FF;
    lastAddress = address;
    return memory.Read(address);
}

uint8_t CPU::AddressZeroPageX()
{
    uint16_t address = memory.Read(PC++);
    address = (address + X) & 0x00FF;
    lastAddress = address;
    return memory.Read(address);
}

uint8_t CPU::AddressZeroPageY()
{
    uint16_t address = memory.Read(PC++);
    address = (address + Y) & 0x00FF;
    lastAddress = address;
    return memory.Read(address);
}
// Memory
void CPU::StackPush(uint8_t value)
{
    memory.Write(0x0100 | SP, value);
    --SP;
}

uint8_t CPU::StackPop()
{
    ++SP;
    return memory.Read(0x0100 | SP);
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
    P.bits.V = ((A & 0x80) != (result & 0x80)) ? SET : CLEAR;
    P.bits.N = ((A & 0x80) == 0x80) ? SET : CLEAR;
    P.bits.Z = (result == 0) ? SET : CLEAR;
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
    P.bits.C = (value & 0x80 == 0x80) ? SET : CLEAR;
    uint8_t result = (value << 1) & 0xFE; // make sure that the last bit is equal 0
    P.bits.Z = (result == 0) ? SET : CLEAR;
    P.bits.N = ((result & 0x80) == 0x80) ? SET : CLEAR;
    switch(opcodeAddressModes[currentOpcode])
    {
        case Accumulator:
            A = result;
            break;
        default:
            memory.Write(lastAddress, result);
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
        if (((PC + offset) & 0xFF00) != PC & 0xFF00)
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
        if (((PC + offset) & 0xFF00) != PC & 0xFF00)
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
        if (((PC + offset) & 0xFF00) != PC & 0xFF00)
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
    P.bits.N = ((result & 0x80) == 0x80) ? SET : CLEAR;
    P.bits.V = ((result & 0x40) == 0x40) ? SET : CLEAR;
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
        if (((PC + offset) & 0xFF00) != PC & 0xFF00)
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
        if (((PC + offset) & 0xFF00) != PC & 0xFF00)
        {
            ++cycles; // Add 1 TIM if the destination address is on a different page
        }
        ++cycles; // Add 1 TIM if a branch occurs
        PC += offset;
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
        if (((PC + offset) & 0xFF00) != PC & 0xFF00)
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
    uint16_t lo = memory.Read(IRQ_VECTOR_LOW);
    uint16_t hi = memory.Read(IRQ_VECTOR_HIGH);
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
        if (((PC + offset) & 0xFF00) != PC & 0xFF00)
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
        if (((PC + offset) & 0xFF00) != PC & 0xFF00)
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
    memory.Write(lastAddress, result);
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
    memory.Write(lastAddress, result);
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
}

void CPU::JMP()
{
}

void CPU::JSR()
{
}

void CPU::KIL()
{
}


void CPU::LAS()
{
}

void CPU::LAX()
{
}

void CPU::LDA()
{
}

void CPU::LDX()
{
}

void CPU::LDY()
{
}

void CPU::LSR()
{
}

void CPU::NOP()
{
}

void CPU::ORA()
{
}

void CPU::PHA()
{
}

void CPU::PHP()
{
}

void CPU::PLA()
{
}

void CPU::PLP()
{
}

void CPU::RLA()
{
}

void CPU::ROL()
{
}

void CPU::ROR()
{
}

void CPU::RRA()
{
}

void CPU::RTI()
{
}

void CPU::RTS()
{
}

void CPU::SAX()
{
}

void CPU::SBC()
{
}

void CPU::SEC()
{
}

void CPU::SED()
{
}

void CPU::SEI()
{
}

void CPU::SHX()
{
}

void CPU::SHY()
{
}

void CPU::SLO()
{
}

void CPU::SRE()
{
}

void CPU::STA()
{
}

void CPU::STX()
{
}

void CPU::STY()
{
}

void CPU::TAS()
{
}

void CPU::TAX()
{
}

void CPU::TAY()
{
}

void CPU::TSX()
{
}

void CPU::TXA()
{
}

void CPU::TXS()
{
}

void CPU::TYA()
{
}

void CPU::XAA()
{
}