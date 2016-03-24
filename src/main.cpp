#include "CPU.h"
#include "Cartridge.h"
#include "Platforms.h"
int main()
{
    Cartridge cartridge;
    cartridge.LoadNESFile("../rom/Mario.nes");
    return 0;
}