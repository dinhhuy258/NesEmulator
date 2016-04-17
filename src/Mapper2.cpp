#include "Mapper2.h"
#include <assert.h>

Mapper2::Mapper2(Cartridge *cartridge) : Mapper(cartridge)
{
    currentBank = 0;
    lastBank = cartridge->GetNumPRG() - 1;
}

uint8_t Mapper2::ReadPRG(uint16_t address)
{
    /*
     * The first PRG-ROM bank is loaded into $8000 and the last PRG-ROM bank is
     * loaded into $C000. Switching is only allowed for the bank at $8000, the one at $C000 is
     * permanently assigned to that location.
     */
    uint8_t returnValue = 0xFF;
    if (address < 0xC000)
    {
        returnValue = cartridge->ReadPRG(currentBank, address - 0x8000);
    }
    else
    {
        returnValue = cartridge->ReadPRG(lastBank, address - 0xC000);
    }
    return returnValue;
}

uint8_t Mapper2::ReadCHR(uint16_t address)
{
    // In mapper2 CHR-ROM/RAM has only 1 bank 
    return cartridge->ReadCHR(0, address);
}   

void Mapper2::WritePRG(uint16_t address, uint8_t value)
{
    /*
     * 7  bit  0
     * ---- ----
     * xxxx xPPP
     *       |||
     *       +++- Select 16 KB PRG ROM bank for CPU $8000-$BFFF
     *            (UNROM uses bits 2-0)
     */
    currentBank = value & 0x07;
}

void Mapper2::WriteCHR(uint16_t address, uint8_t value)
{
    // In mapper2 CHR-ROM/RAM has only 1 bank
    cartridge->WriteCHR(0, address, value);
}