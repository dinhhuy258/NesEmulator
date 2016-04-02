#ifndef _MEMORY_PPU_H_
#define _MEMORY_PPU_H_

#include "Memory.h"

class MemoryPPU : public Memory
{
    public:
        uint8_t Read(uint16_t address);
        void Write(uint16_t address, uint8_t value);

    private:
        /*
         * The PPU memory has a 16-bit address bus and as such could support 64KB of memery with address from 0x0000-0xFFFF
         * But data address from 0x4000-0xFFFF mirror 0x0000-0x3FFF so I don't implement it here
         */
        uint8_t data[16384];
};

#endif