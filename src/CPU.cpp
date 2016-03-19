#include "CPU.h"

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

// Adress mode
uint8_t CPU::AddressAbsolute()
{
    // 6502 is little endian
    uint16_t lo = memory.Read(PC++);
    uint16_t hi = memory.Read(PC++);
    uint16_t address = (hi << 8) | lo;
    return memory.Read(address);
}

uint8_t CPU::AddressAbsoluteX()
{
    // 6502 is little endian
    uint16_t lo = memory.Read(PC++);
    uint16_t hi = memory.Read(PC++);
    uint16_t address = (hi << 8) | lo;
    // If the result of Base+Index is greater than $FFFF, wrapping will occur.
    if ((address & 0xFF00) != ((address + X) & 0xFF00)) 
    {
        // page cross
    }
    address = address + X;
    return memory.Read(address);    
}

uint8_t CPU::AddressAbsoluteY()
{
    // 6502 is little endian
    uint16_t lo = memory.Read(PC++);
    uint16_t hi = memory.Read(PC++);
    uint16_t address = (hi << 8) | lo;
    // If the result of Base+Index is greater than $FFFF, wrapping will occur.
    if ((address & 0xFF00) != ((address + Y) & 0xFF00)) 
    {
        // page cross
    }
    address = address + Y;
    return memory.Read(address);
}

uint8_t CPU::AddressAccumulator()
{
    // not used
    return 0;
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
    // If Base_Location+Index is greater than $FF, wrapping will occur
    if ((baseAddress + X) > 0xFF) 
    {
        // page cross
    }
    baseAddress = (baseAddress + X) & 0xFF;
    uint16_t lo = memory.Read(baseAddress);
    uint16_t hi = memory.Read((baseAddress + 1) & 0xFF);
    uint16_t address = (hi << 8) | lo;
    return memory.Read(address);
}

uint8_t CPU::AddressIndirect()
{
    // 6502 is little endian
    uint16_t baseAddress = memory.Read(PC++);
    uint16_t lo = memory.Read(baseAddress);
    uint16_t hi = memory.Read((baseAddress + 1) & 0xFF);
    uint16_t address = (hi << 8) | lo;
    return memory.Read(address);
}

uint8_t CPU::AddressIndirectY()
{
    // 6502 is little endian
    uint16_t baseAddress = memory.Read(PC++);
    uint16_t lo = memory.Read(baseAddress);
    uint16_t hi = memory.Read((baseAddress + 1) & 0xFF);
    uint16_t address = (hi << 8) | lo;
    // If Base_Location+Index is greater than $FFFF, wrapping will occur.
    if ((address & 0xFF00) != ((address + Y) & 0xFF00)) 
    {
        // page cross
    }
    address = (address + Y);
    return memory.Read(address);
}

uint8_t CPU::AddressRelative()
{

}

uint8_t CPU::AddressZeroPage()
{
    uint16_t address = memory.Read(PC++);
    return memory.Read(address & 0x00FF);
}

uint8_t CPU::AddressZeroPageX()
{
    uint16_t address = memory.Read(PC++);
    if ((address + X) > 0xFF) 
    {
        // page cross
    }
    address = (address + X) & 0xFF;
    return memory.Read(address & 0x00FF);
}

uint8_t CPU::AddressZeroPageY()
{
    uint16_t address = memory.Read(PC++);
    if ((address + Y) > 0xFF) 
    {
        // page cross
    }
    address = (address + Y) & 0xFF;
    return memory.Read(address & 0x00FF);
}

// Opcodes
void CPU::ADC()
{
}

void CPU::ALR()
{
}

void CPU::ANC()
{
}

void CPU::AND()
{
}

void CPU::ARR()
{
}

void CPU::ASL()
{
}

void CPU::AXA()
{
}

void CPU::AXS()
{
}

void CPU::BCC()
{
}

void CPU::BCS()
{
}

void CPU::BEQ()
{
}

void CPU::BIT()
{
}

void CPU::BMI()
{
}

void CPU::BNE()
{
}

void CPU::BPL()
{
}

void CPU::BRK()
{
}

void CPU::BVC()
{
}

void CPU::BVS()
{
}

void CPU::CLD()
{
}

void CPU::CLC()
{
}

void CPU::CLI()
{
}

void CPU::CLV()
{
}

void CPU::CMP()
{
}

void CPU::CPX()
{
}

void CPU::CPY()
{
}

void CPU::DCP()
{
}

void CPU::DEC()
{
}

void CPU::DEX()
{
}

void CPU::DEY()
{
}

void CPU::EOR()
{
}

void CPU::INC()
{
}

void CPU::INX()
{
}

void CPU::INY()
{
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