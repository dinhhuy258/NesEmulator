#include "Mapper0.h"
#include <assert.h>

Mapper0::Mapper0(Cartridge *cartridge) : Mapper(cartridge)
{
    switch(cartridge->GetNumPRG())
    {
        case 1:
            NROMType = NROM128;
            break;
        case 2:
            NROMType = NROM256;
            break;
        default:
            assert(0);
    }
}

uint8_t Mapper0::ReadPRG(uint16_t address)
{
    /*
     * The PRG-ROM is the area of ROM used to store the program code
     * The address of PRG-ROM in CPU memory is from 0x8000-0xFFFF
     */
    uint8_t returnValue = 0xFF;
    if (address < 0xC000)
    {
        // 0x8000-0xBFFF: First 16 KB of rom
        returnValue = cartridge->ReadPRG(0, address - 0x8000);
    }
    else
    {
        // 0xC000-0xFFFF: Last 16 KB of ROM (NROM-256) or mirror of $8000-$BFFF (NROM-128)
        if (NROMType == NROM128)
        {
            returnValue = cartridge->ReadPRG(0, address - 0xC000);
        }
        else
        {
            returnValue = cartridge->ReadPRG(1, address - 0xC000);
        }
    }
    return returnValue;
}

uint8_t Mapper0::ReadCHR(uint16_t address)
{
    // In mapper0 CHR-ROM/RAM has only 1 bank 
    return cartridge->ReadCHR(0, address);
}   

void Mapper0::WritePRG(uint16_t address, uint8_t value)
{
    //Can't write to this mapper
}

void Mapper0::WriteCHR(uint16_t address, uint8_t value)
{
    // In mapper0 CHR-ROM/RAM has only 1 bank 
    cartridge->WriteCHR(0, address, value);
}