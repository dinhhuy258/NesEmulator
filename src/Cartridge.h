#ifndef _CARTRIDGE_H_
#define _CARTRIDGE_H_

#include <stdint.h>
#include <string>

enum Mirroring 
{ 
    Horizontal,
    Vertical,
    FourScreen,
    SingleScreen
};

struct NESFileHeader
{
    uint32_t identify; // 4 byte of identify. Should contain the string 'NES' (3 bytes) and the value 0x1A (1 byte MS-DOS end-of-file)
    uint8_t numPRG; // number of 16KB(16384 bytes) PRG-ROM banks. The PRG-ROM is the area of ROM used to store the program code
    uint8_t numCHR; //  number of 8KB(8192) CHR-ROM (0 indicates CHR-RAM)
    /*
     * 7       0
     * NNNN FTBM
     *
     * N: Lower 4 bits of the mapper number
     * F: Four screen mode. 0 = no, 1 = yes. (When set, the M bit has no effect)
     * T: Trainer.  0 = no trainer present, 1 = 512 byte trainer at 7000-71FFh
     * B: SRAM at 6000-7FFFh battery backed.  0 = no, 1 = yes
     * M: Mirroring.  0 = horizontal, 1 = vertical.
     */
    union RomControlByte1
    {
        uint8_t byte;
        struct RomControlBit1
        {
            uint8_t mapperNumber : 4;
            uint8_t fourScreenMode : 1;
            uint8_t trainerPresent : 1;
            uint8_t batteryBackedPresent : 1;
            uint8_t mirroring : 1;
        }bits;
    }romControlByte1;
    /*
     * 7       0
     * NNNN xxxx
     *
     * N: Upper 4 bits of the mapper number
     * x: Reserved for future usage and should all be 0
     */
    union RomControlByte2
    {
        uint8_t byte;
        struct RomControlBit2
        {
            uint8_t mapperNumber : 4;
            uint8_t reserved : 4;
        }bits;
    }romControlByte2;
    uint8_t numRam; // Number of 8 KB RAM banks. For compatibility with previous versions of the iNES format, assume 1 page of RAM when this is 0
    uint8_t reserved[7]; // Reserved for future usage and should all be 0
};

class Cartridge
{
    public:
        bool LoadNESFile(std::string fileName);

    private:
        NESFileHeader header;
        uint8_t **prgRom;
        uint8_t **chrRom;
        Mirroring mirroring;
        uint8_t mapper;
        bool isCHRRam;
};

#endif