#include "Cartridge.h"
#include <fstream> 
#include "Platforms.h"

Cartridge::Cartridge()
{
    prgRom = NULL;
    chrRomRam = NULL;
}

Cartridge::~Cartridge()
{
    for (uint8_t i = 0; i < header.numPRG; ++i)
    {
        SAFE_DEL_ARRAY(prgRom[i]);
    }
    for (uint8_t i = 0; i < header.numCHR; ++i)
    {
        SAFE_DEL_ARRAY(chrRomRam[i]);
    }
    if (header.numCHR == 0)
    {
        SAFE_DEL_ARRAY(chrRomRam[0]);
    }
    SAFE_DEL_ARRAY(prgRom);
    SAFE_DEL_ARRAY(chrRomRam);
}

bool Cartridge::LoadNESFile(std::string fileName)
{
    std::ifstream is(fileName, std::ifstream::binary);
    if (is)
    {
        char nesHeader[16];
        is.read(reinterpret_cast<char *>(&header), 16); // Read header of the NES file

        if (!header.identify == 0x1A53454E) // N = 0x4E, E = 0x45, S = 0x53, Break character = 0x1A 
        {
            LOGI("This is not a NES file");
            return false;    
        }
        if (header.romControlByte1.bits.trainerPresent == SET)
        {
            // Following the header is the 512-byte trainer, if one is presen
            uint8_t trainer[512];
            is.read(reinterpret_cast<char *>(trainer), 512);   
        }
        // Read PRG-ROM
        prgRom = new uint8_t*[header.numPRG];
        for (uint8_t i = 0; i < header.numPRG; ++i)
        {
            prgRom[i] = new uint8_t[16384]; //16KB
            is.read(reinterpret_cast<char *>(prgRom[i]), 16384);
        }
        // Read CHR-ROM/ CHR-RAM
        if (header.numCHR == 0)
        {
            // If this value is equal 0. It mean we have 8192 byte charactor RAM pages instead of charactor ROM pages
            isCHRRam = true; 
            chrRomRam = new uint8_t*[1];
            chrRomRam[0] = new uint8_t[8192]; //8KB

            // Initialize ram memory
            for (uint16_t i = 0; i < 0x2000; i += 0x10)
            {
                for (uint8_t j = 0; j <= 0x0F; ++j)
                {
                    chrRomRam[0][i | j] = ((j <= 0x03) || ((j > 0x07) && (j <= 0x0B))) ? 0x00 : 0xFF;
                }
            }
        }
        else
        {
            chrRomRam = new uint8_t*[header.numCHR];
            for (uint8_t i = 0; i < header.numCHR; ++i)
            {
                chrRomRam[i] = new uint8_t[8192]; //8KB
                is.read(reinterpret_cast<char *>(chrRomRam[i]), 8192);   
            }    
        }
        
        mapper = header.romControlByte2.bits.mapperNumber;
        mapper = mapper << 4;
        mapper |= (header.romControlByte1.bits.mapperNumber & 0x0F);
        mirroring = (header.romControlByte1.bits.mirroring == CLEAR) ? Horizontal : Vertical;
        mirroring = (header.romControlByte1.bits.fourScreenMode == SET) ? FourScreen : mirroring;
        is.close();
    }
    else
    {
        LOGI("Can't open NES file");
        return false;
    }
    return true;
}

uint8_t Cartridge::ReadPRG(uint8_t bank, uint16_t address)
{
    return prgRom[bank][address];
}

uint8_t Cartridge::ReadCHR(uint8_t bank, uint16_t address)
{
    return chrRomRam[bank][address];
}

void Cartridge::WriteCHR(uint8_t bank, uint16_t address, uint8_t value)
{
    if (isCHRRam)
    {
        chrRomRam[bank][address] = value;  
    }
}

uint8_t Cartridge::ReadSRAM(uint16_t address)
{
    return sram[address];
}

void Cartridge::WriteSRAM(uint16_t address, uint8_t value)
{
    sram[address] = value;
}

uint8_t Cartridge::GetMapperNumber()
{
    return mapper;
}

uint8_t Cartridge::GetNumPRG()
{
    return header.numPRG;
}

Mirroring Cartridge::GetMirroring()
{
    return mirroring;
}