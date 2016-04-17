#ifndef _MAPPER_2_H_
#define _MAPPER_2_H_

#include "Mapper.h"

/*
 * The first PRG-ROM bank is loaded into $8000 and the last PRG-ROM bank is
 * loaded into $C000. Switching is only allowed for the bank at $8000, the one at $C000 is
 * permanently assigned to that location. 
 * Since this mapper has no support for VROM, games using it have 8 KB of VRAM at $0000 in PPU memory
 * UNROM switches were the first chips to allow bank switching of NES games. UNROM only allowed 
 * switching of PRG-ROM banks. It provided no support for CHR-ROM. Themaximum number of 
 * 16 KB PRG-ROM banks using UNROM is 8
 */

class Mapper2 : public Mapper
{
    public:
        Mapper2(Cartridge *cartridge);
        uint8_t ReadPRG(uint16_t address);
        uint8_t ReadCHR(uint16_t address);
        void WritePRG(uint16_t address, uint8_t value);
        void WriteCHR(uint16_t address, uint8_t value);

    private:
        uint8_t currentBank;
        uint8_t lastBank; // The last bank (C000-FFFF) is permanently assigned to that location
};

#endif //_MAPPER_2_H_