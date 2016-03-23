#include "Cartridge.h"
#include <fstream> 
#include "Platforms.h"

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
}