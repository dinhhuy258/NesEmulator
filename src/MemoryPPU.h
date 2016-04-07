#ifndef _MEMORY_PPU_H_
#define _MEMORY_PPU_H_

#include "Memory.h"
class MemoryPPU : public Memory
{
    public:
        void SetMapper(Mapper *mapper);
        uint8_t Read(uint16_t address);
        void Write(uint16_t address, uint8_t value);

    private:
        // $2000-$2FFF: Nametables
        uint8_t nametables[0x1000];
        // $3F00-$3F1F: Patlette
        uint8_t palette[0x20/*32*/]; 

        uint16_t GetMirrorAddress(uint16_t address);
};

#endif