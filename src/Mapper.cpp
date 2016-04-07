#include "Mapper.h"
#include <assert.h>
#include "Mapper0.h"

Mapper* Mapper::GetMapper(Cartridge *cartridge)
{
    Mapper *mapper = NULL;
    switch(cartridge->GetMapperNumber())
    {
        case 0:
            mapper = new Mapper0(cartridge);
            break;
    }
    return mapper;
}

Mapper::Mapper(Cartridge *cartridge)
{
    this->cartridge = cartridge;
}

Mirroring Mapper::GetCartridgeMirroring()
{
    return cartridge->GetMirroring();
}

uint8_t Mapper::Read(uint16_t address)
{
    uint8_t value = 0;
    if (address < 0x4020)
    {
        // $0000-$401F
        // Just for debugging
        assert(0);
    }
    else if (address < 0x6000)
    {
        // $4020-$5FFF: Expansion ROM
        // We haven't implement mapper using expansion ROM yet
    }
    else if (address < 0x8000)
    {
        // $6000-$7FFF: SRAM is used in RPG game. We can use it to save the current state of game
        value = cartridge->ReadSRAM(address - 0x6000);
    }
    else
    {
        // $8000-$FFFF: PRG-ROM
        value = ReadPRG(address);
    }
    return value;
}

void Mapper::Write(uint16_t address, uint8_t value)
{
    if (address < 0x4020)
    {
        // $0000-$401F
        // Just for debugging
        assert(0);
    }
    else if (address < 0x6000)
    {
        // $4020-$5FFF: Expansion ROM
        // We haven't implement mapper using expansion ROM yet
    }
    else if (address < 0x8000)
    {
        // $6000-$7FFF: SRAM is used in RPG game. We can use it to save the current state of game
        cartridge->WriteSRAM(address - 0x6000, value);
    }
    else
    {
        // $8000-$FFFF: PRG-ROM
        WritePRG(address, value);
    }
}