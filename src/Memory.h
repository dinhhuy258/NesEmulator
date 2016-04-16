#ifndef _MEMORY_H_
#define _MEMORY_H_

#include <stdint.h>
#include "Mapper.h"
#include "Controller.h"

class PPU;
class Memory
{
    public:
        Memory()
        {
            ppu = NULL;
            mapper = NULL;
        }
        void SetPPU(PPU *ppu)
        {
            this->ppu = ppu;
        }
        void SetMapper(Mapper *mapper)
        {
            this->mapper = mapper;
        }
        void SetController(Controller *controller)
        {
            this->controller = controller;
        }
        virtual uint8_t Read(uint16_t address) = 0;
        virtual void Write(uint16_t address, uint8_t value) = 0;

    protected:
        PPU *ppu; // only used in MemoryCPU to Write/Read PPU Registry
        Mapper *mapper;
        Controller *controller;
};

#endif //_MEMORY_H_