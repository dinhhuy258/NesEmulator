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

// Opcodes
void CPU::ADC()
{
}

void CPU::AHX()
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