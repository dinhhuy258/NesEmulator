#ifndef _MEMORY_H_
#define _MEMORY_H_

#include <stdint.h>

class Memory
{
    public:
        virtual uint8_t Read(uint16_t address) = 0;
        virtual void Write(uint16_t address, uint8_t value) = 0;
};

#endif //_MEMORY_H_