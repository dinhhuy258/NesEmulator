#include "CPU.h"
#include "PPU.h"
#include "Cartridge.h"
#include "Platforms.h"
int main()
{
    Cartridge cartridge;
    cartridge.LoadNESFile("../rom/Mario.nes");
    return 0;
}