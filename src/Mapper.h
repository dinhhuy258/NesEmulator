#ifndef _MAPPER_H_
#define _MAPPER_H_

#include "Cartridge.h"

class Mapper
{
    public:
        static Mapper* GetMapper(Cartridge *cartridge);
        Mapper(Cartridge *cartridge);
        Mirroring GetCartridgeMirroring();
        uint8_t Read(uint16_t address);
        void Write(uint16_t address, uint8_t value);

        virtual uint8_t ReadPRG(uint16_t address) = 0;
        virtual uint8_t ReadCHR(uint16_t address) = 0;    
        virtual void WriteCHR(uint16_t address, uint8_t value) = 0;
        virtual void WritePRG(uint16_t address, uint8_t value) = 0;
        
    protected:
        Cartridge *cartridge;
};

#endif //_MAPPER_H_