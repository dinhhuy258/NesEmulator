#ifndef _CPU_H_
#define _CPU_H_
#include <stdint.h>
#include <string>

union ProcessorStatus
{
    uint8_t byte;
    struct BitFlags
    {
        /*
         * Carry Flag: The carry flag is set if the last instruction resulted in an overflow 
         * from bit 7 or and underflow from bit 0
         * For example: performing 255 + 1 would result in an answer of 0 with the carry bit set
         */
        uint8_t C : 1;
        /*
         * Zero Flag: The zero flag is set if the result of the last instruction was Zero
         */
        uint8_t Z : 1;
        /*
         * Interrupt Disable: The interrupt disable flag can be used to prevent the system responding to IRQs
         * It is set by the SEI (Set Interrupt Disable) instruction and IRQs will then be ignored until 
         * execution of a CLI (Clear Interrupt Disable) instruction
         */
        uint8_t I : 1;
        /*
         * Decimal Mode: The decimal mode flag is used to switch the 6502 into BCD mode
         * However the 2A03 does not support BCD mode so although the flag can be set, its value
         * will be ignored. This flag can be set SED (Set Decimal Flag) instruction and cleared by
         * CLD (Clear Decimal Flag)
        */
        uint8_t D : 1;
        /*
         * Break Command: The break command flag is used to indicate that a BRK (Break) instruction has 
         * been executed, causing an IRQ
         */
        uint8_t B : 1;
        /*
         * This bit is not used 
         */
        uint8_t reserved : 1;
        /*
         * Overflow Flag: The overflow flag is set if an invalid two’s complement result was obtained by the previous instruction
         * This means that a negative result has been obtained when a positive one was expected or vice versa
         * However 64 + 64 gives the result -128 due to the sign bit. Therefore the overflow flag would be set
         * The overflow flag is determined by taking the exclusive-or of the carry from between bits 6 and 7 and between bit 7
         * and the carry flag 
         */
        uint8_t V : 1; 
        /*
         * Negative Flag: Bit 7 of a byte represents the sign of that byte, with 0 being positive and 1 being negative. 
         * The negative flag (also known as the sign flag) is set if this sign bit is 1
         */
        uint8_t N : 1;
    } bits;
};

enum AddressMode
{
    Absolute,
    AbsoluteX,
    AbsoluteY,
    Accumulator,
    Immediate,
    Implied,
    IndexedIndirect,
    Indirect,
    IndirectIndexed,
    Relative,
    ZeroPage,
    ZeroPageX,
    ZeroPageY
};

class CPU
{
    public:
        CPU();

    private:

    private:
        /*
         * Accumulator: The accumulator is an 8-bit register which stores the results of arithmetic and logic
         * operations. The accumulator can also be set to a value retrieved from memory
         */
        uint8_t A;
        /*
         * Index Register X: The X register is an 8-bit register typically used as a counter or an offset for certain
         * addressing modes. The X register can be set to a value retrieved from memory and can be used to get or set 
         * the value of the stack pointer
         */
        uint8_t X;
        /*
         * Index Register Y: The Y register is an 8-bit register which is used in the same way as the X register, as a
         * counter or to store an offset. Unlike the X register, the Y register cannot affect the stack pointer
         */
        uint8_t Y;
        /*
         * Stack Pointer: The stack is located at memory locations $0100-$01FF. The stack pointer is an 8-bit register
         * which serves as an offset from $0100. The stack works top-down, so when a byte is pushed on to the stack, 
         * the stack pointer is decremented and when a byte is pulled from the stack, the stack pointer is incremented
         * There is no detection of stack overflow and the stack pointer will just wrap around from $00 to $FF
         */
        uint8_t SP;
        /*
         * Program Counter: The program counter is a 16-bit register which holds the address of the next instruction to be
         * executed. As instructions are executed, the value of the program counter is updated, usually moving on to the
         * next instruction in the sequence. The value can be affected by branch and jump instructions, procedure calls and interrupts.
         */
        uint16_t PC;
        // The status register contains a number of single bit flags which are set or cleared when instructions are executed
        ProcessorStatus P;
        // Number of cycles
        uint64_t cycles;

    private:
        // Opcodes table
        std::string opcodeTable[256] = 
        {
                 /*x0     x1     x2     x3     x4     x5     x6     x7     x8     x9     xA     xB     xC     xD     xE     xF*/
            /*0x*/"BRK", "ORA", "KIL", "SLO", "NOP", "ORA", "ASL", "SLO", "PHP", "ORA", "ASL", "ANC", "NOP", "ORA", "ASL", "SLO",
            /*1x*/"BPL", "ORA", "KIL", "SLO", "NOP", "ORA", "ASL", "SLO", "CLC", "ORA", "NOP", "SLO", "NOP", "ORA", "ASL", "SLO",
            /*2x*/"JSR", "AND", "KIL", "RLA", "BIT", "AND", "ROL", "RLA", "PLP", "AND", "ROL", "ANC", "BIT", "AND", "ROL", "RLA",
            /*3x*/"BMI", "AND", "KIL", "RLA", "NOP", "AND", "ROL", "RLA", "SEC", "AND", "NOP", "RLA", "NOP", "AND", "ROL", "RLA",
            /*4x*/"RTI", "EOR", "KIL", "SRE", "NOP", "EOR", "LSR", "SRE", "PHA", "EOR", "LSR", "ALR", "JMP", "EOR", "LSR", "SRE",
            /*5x*/"BVC", "EOR", "KIL", "SRE", "NOP", "EOR", "LSR", "SRE", "CLI", "EOR", "NOP", "SRE", "NOP", "EOR", "LSR", "SRE",
            /*6x*/"RTS", "ADC", "KIL", "RRA", "NOP", "ADC", "ROR", "RRA", "PLA", "ADC", "ROR", "ARR", "JMP", "ADC", "ROR", "RRA",
            /*7x*/"BVS", "ADC", "KIL", "RRA", "NOP", "ADC", "ROR", "RRA", "SEI", "ADC", "NOP", "RRA", "NOP", "ADC", "ROR", "RRA", 
            /*8x*/"NOP", "STA", "NOP", "SAX", "STY", "STA", "STX", "SAX", "DEY", "NOP", "TXA", "XAA", "STY", "STA", "STX", "SAX", 
            /*9x*/"BCC", "STA", "KIL", "AHX", "STY", "STA", "STX", "SAX", "TYA", "STA", "TXS", "TAS", "SHY", "STA", "SHX", "AHX",
            /*Ax*/"LDY", "LDA", "LDX", "LAX", "LDY", "LDA", "LDX", "LAX", "TAY", "LDA", "TAX", "LAX", "LDY", "LDA", "LDX", "LAX",
            /*Bx*/"BCS", "LDA", "KIL", "LAX", "LDY", "LDA", "LDX", "LAX", "CLV", "LDA", "TSX", "LAS", "LDY", "LDA", "LDX", "LAX",
            /*Cx*/"CPY", "CMP", "NOP", "DCP", "CPY", "CMP", "DEC", "DCP", "INY", "CMP", "DEX", "AXS", "CPY", "CMP", "DEC", "DCP",
            /*Dx*/"BNE", "CMP", "KIL", "DCP", "NOP", "CMP", "DEC", "DCP", "CLD", "CMP", "NOP", "DCP", "NOP", "CMP", "DEC", "DCP",
            /*Ex*/"CPX", "SBC", "NOP", "ISC", "CPX", "SBC", "INC", "ISC", "INX", "SBC", "NOP", "SBC", "CPX", "SBC", "INC", "ISC",
            /*Fx*/"BEQ", "SBC", "KIL", "ISC", "NOP", "SBC", "INC", "ISC", "SED", "SBC", "NOP", "ISC", "NOP", "SBC", "INC", "ISC",
        };
        void(CPU::*opcodeFunctions[256])(void) = 
        {
                    /*x0         x1         x2         x3         x4         x5         x6         x7         x8         x9         xA         xB         xC         xD         xE         xF*/
            /*0x*/&CPU::BRK, &CPU::ORA, &CPU::KIL, &CPU::SLO, &CPU::NOP, &CPU::ORA, &CPU::ASL, &CPU::SLO, &CPU::PHP, &CPU::ORA, &CPU::ASL, &CPU::ANC, &CPU::NOP, &CPU::ORA, &CPU::ASL, &CPU::SLO,
            /*1x*/&CPU::BPL, &CPU::ORA, &CPU::KIL, &CPU::SLO, &CPU::NOP, &CPU::ORA, &CPU::ASL, &CPU::SLO, &CPU::CLC, &CPU::ORA, &CPU::NOP, &CPU::SLO, &CPU::NOP, &CPU::ORA, &CPU::ASL, &CPU::SLO,
            /*2x*/&CPU::JSR, &CPU::AND, &CPU::KIL, &CPU::RLA, &CPU::BIT, &CPU::AND, &CPU::ROL, &CPU::RLA, &CPU::PLP, &CPU::AND, &CPU::ROL, &CPU::ANC, &CPU::BIT, &CPU::AND, &CPU::ROL, &CPU::RLA,
            /*3x*/&CPU::BMI, &CPU::AND, &CPU::KIL, &CPU::RLA, &CPU::NOP, &CPU::AND, &CPU::ROL, &CPU::RLA, &CPU::SEC, &CPU::AND, &CPU::NOP, &CPU::RLA, &CPU::NOP, &CPU::AND, &CPU::ROL, &CPU::RLA,
            /*4x*/&CPU::RTI, &CPU::EOR, &CPU::KIL, &CPU::SRE, &CPU::NOP, &CPU::EOR, &CPU::LSR, &CPU::SRE, &CPU::PHA, &CPU::EOR, &CPU::LSR, &CPU::ALR, &CPU::JMP, &CPU::EOR, &CPU::LSR, &CPU::SRE,
            /*5x*/&CPU::BVC, &CPU::EOR, &CPU::KIL, &CPU::SRE, &CPU::NOP, &CPU::EOR, &CPU::LSR, &CPU::SRE, &CPU::CLI, &CPU::EOR, &CPU::NOP, &CPU::SRE, &CPU::NOP, &CPU::EOR, &CPU::LSR, &CPU::SRE,
            /*6x*/&CPU::RTS, &CPU::ADC, &CPU::KIL, &CPU::RRA, &CPU::NOP, &CPU::ADC, &CPU::ROR, &CPU::RRA, &CPU::PLA, &CPU::ADC, &CPU::ROR, &CPU::ARR, &CPU::JMP, &CPU::ADC, &CPU::ROR, &CPU::RRA,
            /*7x*/&CPU::BVS, &CPU::ADC, &CPU::KIL, &CPU::RRA, &CPU::NOP, &CPU::ADC, &CPU::ROR, &CPU::RRA, &CPU::SEI, &CPU::ADC, &CPU::NOP, &CPU::RRA, &CPU::NOP, &CPU::ADC, &CPU::ROR, &CPU::RRA, 
            /*8x*/&CPU::NOP, &CPU::STA, &CPU::NOP, &CPU::SAX, &CPU::STY, &CPU::STA, &CPU::STX, &CPU::SAX, &CPU::DEY, &CPU::NOP, &CPU::TXA, &CPU::XAA, &CPU::STY, &CPU::STA, &CPU::STX, &CPU::SAX, 
            /*9x*/&CPU::BCC, &CPU::STA, &CPU::KIL, &CPU::AHX, &CPU::STY, &CPU::STA, &CPU::STX, &CPU::SAX, &CPU::TYA, &CPU::STA, &CPU::TXS, &CPU::TAS, &CPU::SHY, &CPU::STA, &CPU::SHX, &CPU::AHX,
            /*Ax*/&CPU::LDY, &CPU::LDA, &CPU::LDX, &CPU::LAX, &CPU::LDY, &CPU::LDA, &CPU::LDX, &CPU::LAX, &CPU::TAY, &CPU::LDA, &CPU::TAX, &CPU::LAX, &CPU::LDY, &CPU::LDA, &CPU::LDX, &CPU::LAX,
            /*Bx*/&CPU::BCS, &CPU::LDA, &CPU::KIL, &CPU::LAX, &CPU::LDY, &CPU::LDA, &CPU::LDX, &CPU::LAX, &CPU::CLV, &CPU::LDA, &CPU::TSX, &CPU::LAS, &CPU::LDY, &CPU::LDA, &CPU::LDX, &CPU::LAX,
            /*Cx*/&CPU::CPY, &CPU::CMP, &CPU::NOP, &CPU::DCP, &CPU::CPY, &CPU::CMP, &CPU::DEC, &CPU::DCP, &CPU::INY, &CPU::CMP, &CPU::DEX, &CPU::AXS, &CPU::CPY, &CPU::CMP, &CPU::DEC, &CPU::DCP,
            /*Dx*/&CPU::BNE, &CPU::CMP, &CPU::KIL, &CPU::DCP, &CPU::NOP, &CPU::CMP, &CPU::DEC, &CPU::DCP, &CPU::CLD, &CPU::CMP, &CPU::NOP, &CPU::DCP, &CPU::NOP, &CPU::CMP, &CPU::DEC, &CPU::DCP,
            /*Ex*/&CPU::CPX, &CPU::SBC, &CPU::NOP, &CPU::ISC, &CPU::CPX, &CPU::SBC, &CPU::INC, &CPU::ISC, &CPU::INX, &CPU::SBC, &CPU::NOP, &CPU::SBC, &CPU::CPX, &CPU::SBC, &CPU::INC, &CPU::ISC,
            /*Fx*/&CPU::BEQ, &CPU::SBC, &CPU::KIL, &CPU::ISC, &CPU::NOP, &CPU::SBC, &CPU::INC, &CPU::ISC, &CPU::SED, &CPU::SBC, &CPU::NOP, &CPU::ISC, &CPU::NOP, &CPU::SBC, &CPU::INC, &CPU::ISC,
        };

        uint8_t opcodeTableMode[256] =
        {
            6, 7, 6, 7, 11, 11, 11, 11, 6, 5, 4, 5, 1, 1, 1, 1,
            10, 9, 6, 9, 12, 12, 12, 12, 6, 3, 6, 3, 2, 2, 2, 2,
            1, 7, 6, 7, 11, 11, 11, 11, 6, 5, 4, 5, 1, 1, 1, 1,
            10, 9, 6, 9, 12, 12, 12, 12, 6, 3, 6, 3, 2, 2, 2, 2,
            6, 7, 6, 7, 11, 11, 11, 11, 6, 5, 4, 5, 1, 1, 1, 1,
            10, 9, 6, 9, 12, 12, 12, 12, 6, 3, 6, 3, 2, 2, 2, 2,
            6, 7, 6, 7, 11, 11, 11, 11, 6, 5, 4, 5, 8, 1, 1, 1,
            10, 9, 6, 9, 12, 12, 12, 12, 6, 3, 6, 3, 2, 2, 2, 2,
            5, 7, 5, 7, 11, 11, 11, 11, 6, 5, 6, 5, 1, 1, 1, 1,
            10, 9, 6, 9, 12, 12, 13, 13, 6, 3, 6, 3, 2, 2, 3, 3,
            5, 7, 5, 7, 11, 11, 11, 11, 6, 5, 6, 5, 1, 1, 1, 1,
            10, 9, 6, 9, 12, 12, 13, 13, 6, 3, 6, 3, 2, 2, 3, 3,
            5, 7, 5, 7, 11, 11, 11, 11, 6, 5, 6, 5, 1, 1, 1, 1,
            10, 9, 6, 9, 12, 12, 12, 12, 6, 3, 6, 3, 2, 2, 2, 2,
            5, 7, 5, 7, 11, 11, 11, 11, 6, 5, 6, 5, 1, 1, 1, 1,
            10, 9, 6, 9, 12, 12, 12, 12, 6, 3, 6, 3, 2, 2, 2, 2,
        };
        uint8_t opcodeTableSize[256] =
        {
            1, 2, 0, 0, 2, 2, 2, 0, 1, 2, 1, 0, 3, 3, 3, 0,
            2, 2, 0, 0, 2, 2, 2, 0, 1, 3, 1, 0, 3, 3, 3, 0,
            3, 2, 0, 0, 2, 2, 2, 0, 1, 2, 1, 0, 3, 3, 3, 0,
            2, 2, 0, 0, 2, 2, 2, 0, 1, 3, 1, 0, 3, 3, 3, 0,
            1, 2, 0, 0, 2, 2, 2, 0, 1, 2, 1, 0, 3, 3, 3, 0,
            2, 2, 0, 0, 2, 2, 2, 0, 1, 3, 1, 0, 3, 3, 3, 0,
            1, 2, 0, 0, 2, 2, 2, 0, 1, 2, 1, 0, 3, 3, 3, 0,
            2, 2, 0, 0, 2, 2, 2, 0, 1, 3, 1, 0, 3, 3, 3, 0,
            2, 2, 0, 0, 2, 2, 2, 0, 1, 0, 1, 0, 3, 3, 3, 0,
            2, 2, 0, 0, 2, 2, 2, 0, 1, 3, 1, 0, 0, 3, 0, 0,
            2, 2, 2, 0, 2, 2, 2, 0, 1, 2, 1, 0, 3, 3, 3, 0,
            2, 2, 0, 0, 2, 2, 2, 0, 1, 3, 1, 0, 3, 3, 3, 0,
            2, 2, 0, 0, 2, 2, 2, 0, 1, 2, 1, 0, 3, 3, 3, 0,
            2, 2, 0, 0, 2, 2, 2, 0, 1, 3, 1, 0, 3, 3, 3, 0,
            2, 2, 0, 0, 2, 2, 2, 0, 1, 2, 1, 0, 3, 3, 3, 0,
            2, 2, 0, 0, 2, 2, 2, 0, 1, 3, 1, 0, 3, 3, 3, 0,
        };
        uint8_t opcodeTableCycle[256] =
        {
            7, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 4, 4, 6, 6,
            2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
            6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 4, 4, 6, 6,
            2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
            6, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 3, 4, 6, 6,
            2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
            6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 5, 4, 6, 6,
            2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
            2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,
            2, 6, 2, 6, 4, 4, 4, 4, 2, 5, 2, 5, 5, 5, 5, 5,
            2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,
            2, 5, 2, 5, 4, 4, 4, 4, 2, 4, 2, 4, 4, 4, 4, 4,
            2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,
            2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
            2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,
            2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
        };
        uint8_t opcodeTablePageCycle[256] =
        {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0,
        };

    private:
        // Opcodes
        void ADC();
        void AHX();
        void ALR();
        void ANC();
        void AND();
        void ARR();
        void ASL();
        void AXS();

        void BCC();
        void BCS();
        void BEQ();
        void BIT();
        void BMI();
        void BNE();
        void BPL();
        void BRK();
        void BVC();
        void BVS();

        void CLD();
        void CLC();
        void CLI();
        void CLV();
        void CMP();
        void CPX();
        void CPY();

        void DCP();
        void DEC();
        void DEX();
        void DEY();

        void EOR();

        void INC();
        void INX();
        void INY();
        void ISC();

        void JMP();
        void JSR();

        void KIL();

        void LAS();
        void LAX();        
        void LDA();
        void LDX();
        void LDY();
        void LSR();

        void NOP();

        void ORA();

        void PHA();
        void PHP();
        void PLA();
        void PLP();

        void RLA();
        void ROL();
        void ROR();
        void RRA();
        void RTI();
        void RTS();

        void SAX();
        void SBC();
        void SEC();
        void SED();
        void SEI();
        void SHX();
        void SHY();
        void SLO();
        void SRE();
        void STA();
        void STX();
        void STY();

        void TAS();
        void TAX();
        void TAY();
        void TSX();
        void TXA();
        void TXS();
        void TYA();

        void XAA();
};

#endif //_CPU_H_