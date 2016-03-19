#ifndef _MEMORY_CPU_H_
#define _MEMORY_CPU_H_

#include "Memory.h"

class MemoryCPU : public Memory
{
    public:
        uint8_t Read(uint16_t address);
        void Write(uint16_t address, uint8_t value);

    private:
        //The 6502 has a 16-bit address bus and as such could support 64KB of memery with address from 0x0000-0xFFFF
        uint8_t data[65536];
};

#endif //_MEMORY_CPU_H_