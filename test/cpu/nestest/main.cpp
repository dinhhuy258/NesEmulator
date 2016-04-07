#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string.h>

#define private public

#include "PPU.h"
#include "Cartridge.h"
#include "Platforms.h"
#include "Mapper.h"
#include "MemoryCPU.h"
#include "MemoryPPU.h"
#include "CPU.h"

int getch(void)
{
    struct termios oldattr, newattr;
    int ch;
    tcgetattr( STDIN_FILENO, &oldattr );
    newattr = oldattr;
    newattr.c_lflag &= ~( ICANON | ECHO );
    tcsetattr( STDIN_FILENO, TCSANOW, &newattr );
    ch = getchar();
    tcsetattr( STDIN_FILENO, TCSANOW, &oldattr );
    return ch;
}

int main()
{
    // Initialize
    Cartridge *cartridge = new Cartridge();
    if (cartridge->LoadNESFile("nestest.nes") == false)
    {
        LOGI("Invalid NES file");
        return 1;
    }
    Mapper *mapper = Mapper::GetMapper(cartridge);
    if (mapper == NULL)
    {
        LOGI("We haven't supported this mapper");
        return 1;
    }
    Memory *memoryPPU = new MemoryPPU();
    memoryPPU->SetMapper(mapper);
    PPU *ppu = new PPU(memoryPPU);

    Memory *memoryCPU = new MemoryCPU();
    memoryCPU->SetMapper(mapper);
    memoryCPU->SetPPU(ppu);
    CPU *cpu = new CPU(memoryCPU);
    cpu->PC = 0xC000;
    uint32_t count = 0;
    unsigned int A, X, Y, P, SP, CYC, SL, PC;
    char opcodeName[4];
    std::ifstream file("nestest.log");  
    std::string line;
    if (!file.is_open())
    {
        LOGI("Can't open log file");
        return 1;
    }
    opcodeName[3] = '\0';


    while(getline (file, line))
    {      
        ++count; 
        sscanf(line.c_str(), "%x", &PC);
        sscanf(&line.c_str()[48], "A:%x X:%x Y:%x P:%x SP:%x CYC:%u SL:%u", &A, &X, &Y, &P, &SP, &CYC, &SL);
        opcodeName[0] = line.c_str()[16];
        opcodeName[1] = line.c_str()[17];
        opcodeName[2] = line.c_str()[18];
        
        if (cpu->A != A || cpu->X != X || cpu->Y != Y || cpu->P.byte != P || cpu->SP != SP || cpu->PC != PC)
        {
            LOGI("=======================================================");
            LOGI("PC: %x A: %x X: %x Y: %x P: %x SP: %x", PC, A, X, Y, P, SP);
            LOGI("PC: %x A: %x X: %x Y: %x P: %x SP: %x", cpu->PC, cpu->A, cpu->X, cpu->Y, cpu->P.byte, cpu->SP);
            LOGI("%d", count);
            getch();
        } 
             
        uint8_t cpuCycles = cpu->Step();
        uint8_t ppuCycles = cpuCycles * 3;
        for (uint8_t i = 0; i < ppuCycles; ++i)
        {
            ppu->Step();  
        }

    }
    LOGI("Done!");
    // Deallocate
    file.close();   
    SAFE_DEL(cartridge);
    SAFE_DEL(mapper);
    SAFE_DEL(memoryPPU);
    SAFE_DEL(ppu);
    SAFE_DEL(memoryCPU);
    SAFE_DEL(cpu);
    return 0;
} 