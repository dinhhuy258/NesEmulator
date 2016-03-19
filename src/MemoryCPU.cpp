#include "MemoryCPU.h"

uint8_t MemoryCPU::Read(uint16_t address)
{
    if (address < 0x0800)
    {
        return data[address];  
    }
    else if (address < 0x2000)
    {
        // 0x0800-0x1FFF mirrors 0x0000-0x07FF
        return data[address & 0x07FF];
    }
    else if (address < 0x2008)
    {
        return data[address];
    }
    else if (address < 0x4000)
    {
        // 0x2008-0x3FFF mirrors 0x2000-0x2007
        return data[address & 0x2007];
    }
    else
    {
        return data[address];
    }
}

void MemoryCPU::Write(uint16_t address, uint8_t value)
{
    if (address < 0x0800)
    {
        data[address] = value;
    }
    else if (address < 0x2000)
    {
        // 0x0800-0x1FFF mirrors 0x0000-0x07FF
        data[address & 0x07FF] = value;
    }
    else if (address < 0x2008)
    {
        data[address] = value;
    }
    else if (address < 0x4000)
    {
        // 0x2008-0x3FFF mirrors 0x2000-0x2007
        data[address & 0x2007] = value;
    }
    else
    {
        data[address] = value;
    }
}