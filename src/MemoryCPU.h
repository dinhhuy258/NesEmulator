#ifndef _MEMORY_CPU_H_
#define _MEMORY_CPU_H_

#include "Memory.h"

class MemoryCPU : public Memory
{
    public:
        MemoryCPU();
        void SetPPU(PPU *ppu);
        void SetMapper(Mapper *mapper);
        uint8_t Read(uint16_t address);
        void Write(uint16_t address, uint8_t value);

    private:
        /* 
         * The 6502 has a 16-bit address bus and as such could support 64KB of memery with address from 0x0000-0xFFFF
         *
         * Address range    Size    Device
         * $0000-$07FF      $0800   2KB internal RAM
         * $0800-$0FFF      $0800   Mirrors of $0000-$07FF
         * $1000-$17FF      $0800   Mirrors of $0000-$07FF
         * $1800-$1FFF      $0800   Mirrors of $0000-$07FF
         * $2000-$2007      $0008   NES PPU registers
         * $2008-$3FFF      $1FF8   Mirrors of $2000-2007 (repeats every 8 bytes)
         * $4000-$401F      $0020   NES APU and I/O registerss
         * $4020-$FFFF      $BFE0   Cartridge space: PRG ROM, PRG RAM, and mapper registers
         */
        uint8_t ram[0x800];
};

#endif //_MEMORY_CPU_H_