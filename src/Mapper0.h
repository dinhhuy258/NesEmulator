#ifndef _MAPPER_0_H_
#define _MAPPER_0_H_

#include "Mapper.h"

/*
 * NROM has two configurations:
 * NROM-256 with 32 KiB PRG ROM and 8 KiB CHR ROM
 * NROM-128 with 16 KiB PRG ROM and 8 KiB CHR ROM
 * Your program is mapped into $8000-$FFFF (NROM-256) or both $8000-$BFFF and $C000-$FFFF (NROM-128)
 * Most NROM-128 games actually run in $C000-$FFFF rather than $8000-$BFFF because it makes the program easier to assemble and link. 
 */

enum NROM
{
    NROM128,
    NROM256   
};

class Mapper0 : public Mapper
{
    public:
        Mapper0(Cartridge *cartridge);
        uint8_t ReadPRG(uint16_t address);
        uint8_t ReadCHR(uint16_t address);
        void WritePRG(uint16_t address, uint8_t value);
        void WriteCHR(uint16_t address, uint8_t value);  

    private:
        NROM NROMType;
};

#endif //_MAPPER_0_H_