#include "CPU.h"
#include "Cartridge.h"
#include "Platforms.h"
int main()
{
    //CPU cpu;
    Cartridge cartridge;
    cartridge.LoadNESFile("../../Mario.nes");
    return 0;
}