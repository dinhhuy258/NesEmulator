#include "MemoryCPU.h"
#include "PPU.h"
uint8_t MemoryCPU::Read(uint16_t address)
{
    uint8_t value = 0;
    if (address < 0x0800)
    {
        value = ram[address];  
    }
    else if (address < 0x2000)
    {
        // 0x0800-0x1FFF mirrors 0x0000-0x07FF
        value = ram[address & 0x07FF];
    }
    else if (address < 0x2008)
    {
        // 0x2000-0x2007: I/O Register (PPU)
        value = ppu->ReadRegister(address);
    }
    else if (address < 0x4000)
    {
        // 0x2008-0x3FFF mirrors 0x2000-0x2007
        value = ppu->ReadRegister(address & 0x2007);
    }
    else if (address < 0x4020)
    {
        // 0x4000-0x401F: I/O Register (APU, Joypad)
    }
    else
    {
        // Mapper
        value = mapper->Read(address);
    }
    return value;
}

void MemoryCPU::Write(uint16_t address, uint8_t value)
{
    if (address < 0x0800)
    {
        ram[address] = value;  
    }
    else if (address < 0x2000)
    {
        // 0x0800-0x1FFF mirrors 0x0000-0x07FF
        ram[address & 0x07FF] = value;
    }
    else if (address < 0x2008)
    {
        // 0x2000-0x2007: I/O Register (PPU)
        ppu->WriteRegister(address, value);
    }
    else if (address < 0x4000)
    {
        // 0x2008-0x3FFF mirrors 0x2000-0x2007
        ppu->WriteRegister(address & 0x2007, value);
    }
    else if (address < 0x4020)
    {
        // 0x4000-0x401F: I/O Register (APU, Joypad)
    }
    else
    {
        // Mapper
        mapper->Write(address, value);
    }
}