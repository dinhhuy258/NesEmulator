#include "MemoryPPU.h"

uint8_t MemoryPPU::Read(uint16_t address)
{
    uint8_t value = 0;
    if (address < 0x2000)
    {
        //$0000-$1FFF: Pattern Tables
        value = mapper->ReadCHR(address);
    }
    else if (address < 0x3000)
    {
        //$2000-$2FFF:  Nametables
        value = nametables[GetMirrorAddress(address) - 0x2000];
    }
    else if (address < 0x3F00)
    {
        //$3000-$3EFF: Mirrors $2000-$2FFF 
        value = nametables[GetMirrorAddress(address - 0x1000) - 0x2000];
    }
    else if (address < 0x3F20)
    {
        //$3F00-$3F1F: Patlette
        value = palette[address - 0x3F00];
    }
    else if (address < 0x4000)
    {
        //$3F20-$3FFF: Mirrors $3F00-$3F1F
        value = palette[(address & 0x3F1F) - 0x3F00];
    }
    else
    {
        //$4000-$FFFF: Mirrors $0000-$3FFF
        value = Read(address & 0x3FFF);
    }
    return value;
}

void MemoryPPU::Write(uint16_t address, uint8_t value)
{
    if (address < 0x2000)
    {
        //$0000-$1FFF: Pattern Tables
        mapper->WriteCHR(address, value);
    }
    else if (address < 0x3000)
    {
        //$2000-$2FFF:  Nametables
        nametables[GetMirrorAddress(address) - 0x2000] = value;
    }
    else if (address < 0x3F00)
    {
        //$3000-$3EFF: Mirrors $2000-$2FFF 
        nametables[GetMirrorAddress(address - 0x1000) - 0x2000]  = value;
    }
    else if (address < 0x3F20)
    {
        //$3F00-$3F1F: Patlette
        palette[address - 0x3F00] = value;
    }
    else if (address < 0x4000)
    {
        //$3F20-$3FFF: Mirrors $3F00-$3F1F
        palette[(address & 0x3F1F) - 0x3F00] = value;
    }
    else
    {
        //$4000-$FFFF: Mirrors $0000-$3FFF
        Write(address & 0x3FFF, value);
    }
}

uint16_t MemoryPPU::GetMirrorAddress(uint16_t address)
{
    // addreess range: $2000-$2FFF
    /*
     * Vertical mirroring: $2000 equals $2800 and $2400 equals $2C00 
     * Horizontal mirroring: $2000 equals $2400 and $2800 equals $2C00 
     * One-screen mirroring: All nametables refer to the same memory at any given time
     * Four-screen mirroring: All nametables have it's own value
     */
    uint16_t value = 0;
    Mirroring mirroring = mapper->GetCartridgeMirroring();
    switch(mirroring)
    {
        case Horizontal:
            if (address < 0x2800)
            {
                value = address & 0x23FF;
            }
            else
            {
                value = address & 0x2BFF;
            }
            break;
        case Vertical:
            value = address & 0x27FF;
            break;
        case FourScreen:
            value = address;
            break;
        case SingleScreen:
            value = address & 0x23FF;
            break;
    }
    return value;
}