#include "MemoryCPU.h"
#include "PPU.h"
#include "Platforms.h"
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
        switch(address)
        {
            case 0x2002:
            case 0x2004:
            case 0x2007:
                value = ppu->ReadRegister(address);
                break;
        }
        
    }
    else if (address < 0x4000)
    {
        // 0x2008-0x3FFF mirrors 0x2000-0x2007
        switch(address & 0x2007)
        {
            case 0x2002:
            case 0x2004:
            case 0x2007:
                value = ppu->ReadRegister(address & 0x2007);
                break;
        }
    }
    else if (address < 0x4020)
    {
        // 0x4000-0x401F: I/O Register (APU, Joypad)
        if (address == 0x4016) // Controller1
        {
            value = controller->Read();
        }
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
        if (address == 0x2002)
        {
            return;
        }
        ppu->WriteRegister(address, value);
    }
    else if (address < 0x4000)
    {
        // 0x2008-0x3FFF mirrors 0x2000-0x2007
        if ((address & 0x2007) == 0x2002)
        {
            return;
        }
        ppu->WriteRegister(address & 0x2007, value);
    }
    else if (address < 0x4020)
    {
        // 0x4000-0x401F: I/O Register (APU, Joypad)
        if (address == 0x4016) // Controller1
        {
            controller->Write(value);
        }
        else if (address == 0x4014) //PPU DMA
        {    
            ppu->WriteRegister(address, value);  
        }
    }
    else
    {
        // Mapper
        mapper->Write(address, value);
    }
}