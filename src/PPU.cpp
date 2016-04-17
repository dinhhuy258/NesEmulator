#include "PPU.h"
#include <assert.h>
#include "Platforms.h"

PPU::PPU(Memory *vram)
{
    // Refer http://wiki.nesdev.com/w/index.php/PPU_power_up_state for more information
    this->vram = vram;
    // 241 or 0????
    scanline = 0;
    cycles = 0;
    controlRegister.byte = 0x00; // ($2000)
    maskRegister.byte = 0x00; // ($2001)
    statusRegister.byte = 0x10; // ($2002)
    oamAddress = 0x00; // ($2003)
    fineXScroll = 0; // $2005
    toggle = 0; // ($2005-$2006 latch)
    currentVRAMAddress = 0; // ($2005-$2006)
    internalBuffer = 0;
    oddFrame = false;
    spriteCount = 0;
    nmiPrevious = false;
    nmiDelay = 0;
    frontBuffer = buffer1;
    backBuffer = buffer2;
    for(uint16_t y = 0; y < SCREEN_HEIGHT; ++y)  
    {
        for(uint16_t x = 0; x < SCREEN_WIDTH; ++x)
        {
            frontBuffer[y][x] = 0;
        }
    }
}

void PPU::SetCPU(CPU *cpu)
{
    this->cpu = cpu;
}

void PPU::WriteRegister(uint16_t address, uint8_t value)
{
    // Wrrite the written value to the first 5 bits
    statusRegister.bits.reserved = value & 0x1F;
    switch(address)
    {
        case 0x2000:
            WriteControl(value);
            break;
        case 0x2001:
            WriteMask(value);
            break;
        case 0x2003:
            WriteOAMAddress(value);
            break;
        case 0x2004:
            WriteOAMData(value);
            break;
        case 0x2005:
            WriteScroll(value);
            break;
        case 0x2006:
            WriteAddress(value);
            break;
        case 0x2007:
            WriteData(value);
            break;
        case 0x4014:
            WriteDMA(value);
            break;
        default:
            assert(0);
    }
}

uint8_t PPU::ReadRegister(uint16_t address)
{
    uint8_t result;
    switch(address)
    {
        case 0x2002:
            result = ReadStatus();
            break;
        case 0x2004:
            result = ReadOAMData();
            break;
        case 0x2007:
            result = ReadData(); 
            break;
        default:
            assert(0);
    }
    return result;
}

// 0x2000
void PPU::WriteControl(uint8_t value)
{
    controlRegister.byte = value;
    // t: ...BA.. ........ = d: ......BA
    temporaryVRAMAddress = (temporaryVRAMAddress & 0xF3FF) | ((uint16_t(value) & 0x03) << 10);
    NMIChanged();
}

// 0x2001
void PPU::WriteMask(uint8_t value)
{
    maskRegister.byte = value;
}

// 0x2003
void PPU::WriteOAMAddress(uint8_t value)
{
    oamAddress = value;
}

// 0x2004
void PPU::WriteOAMData(uint8_t value)
{
    // Writes OAM data will increment OAMADDR after the write 
    primaryOAM[oamAddress] = value;
    ++oamAddress;
}

// 0x2005
void PPU::WriteScroll(uint8_t value)
{
    if (toggle == 0)
    {
        // The first write is the horizontal scroll.
        // t: ....... ...HGFED = d: HGFED...
        temporaryVRAMAddress = (temporaryVRAMAddress & 0xFFE0) | (uint16_t(value) >> 3);
        // x:              CBA = d: .....CBA
        fineXScroll = value & 0x07;
        // w:                  = 1
        toggle = 1;
    }
    else if (toggle == 1)
    {
        // The second write is the vertical scroll, the
        // t: CBA..HG FED..... = d: HGFEDCBA
        temporaryVRAMAddress = (temporaryVRAMAddress & 0x8FFF) | ((uint16_t(value) & 0x07) << 12);
        temporaryVRAMAddress = (temporaryVRAMAddress & 0xFC1F) | ((uint16_t(value) & 0xF8) << 2);
        // w:                  = 0
        toggle = 0;
    }
}

// 0x2006
void PPU::WriteAddress(uint8_t value)
{
    /*
     * Typically, this register is written to during vertical blanking so that the next frame starts rendering from the desired location,
     * but it can also be modified during rendering in order to split the screen
     * Changes made to the vertical scroll during rendering will only take effect on the next frame
     */
    if (toggle == 0)
    {
        // First write. Write upper address byte
        // t: 0FEDCBA ........ = d: ..FEDCBA
        // t: X...... ........ = 0
        temporaryVRAMAddress = (temporaryVRAMAddress & 0x80FF) | ((uint16_t(value) & 0x3F) << 8);
        // w:                  = 1
        toggle = 1;
    }
    else if (toggle == 1)
    {
        // Second write. Write lower address byte
        // t: ....... HGFEDCBA = d: HGFEDCBA
        temporaryVRAMAddress = (temporaryVRAMAddress & 0xFF00) | (uint16_t(value) & 0x00FF);
        // v                   = t
        currentVRAMAddress = temporaryVRAMAddress;
        // w:                  = 0
        toggle = 0;     
    } 
}

// 0x2007
void PPU::WriteData(uint8_t value)
{
    vram->Write(currentVRAMAddress, value);
    currentVRAMAddress = 
    (controlRegister.bits.addressIncrement == 1) ? currentVRAMAddress  + 32
                                                 : currentVRAMAddress  + 1;
}

// 0x4014
void PPU::WriteDMA(uint8_t value)
{
    uint16_t address = uint16_t(value) << 8;
    // Writing $XX will upload 256 bytes of data from CPU page $XX00-$XXFF to the internal PPU OAM
    for (uint16_t i = 0; i < 256; ++i)
    {
        primaryOAM[oamAddress++] = cpu->cpuMemory->Read(address + i);
    }
    // On sprite DMA's, cpu is suspended by 513 or 514 cycles (1 dummy read cycle while waiting for writes to complete, +1 if on an odd CPU cycle)
    cpu->stall += 513;
    if (cpu->cycles % 2 == 0)
    {
        ++cpu->stall;
    }
}

// 0x2002
uint8_t PPU::ReadStatus()
{
    // Reading status also reset the address latch
    // w = 0
    toggle = 0;
    uint8_t returnValue = statusRegister.byte;
    statusRegister.bits.vblank = 0; // This bit will be cleared after reading 0x2002
    NMIChanged();
    return returnValue;
}

// 0x2004
uint8_t PPU::ReadOAMData()
{
    return primaryOAM[oamAddress];
}

// 0x2007
uint8_t PPU::ReadData()
{
    /*
     * When reading while the VRAM address is in the range 0-$3EFF (i.e., before the palettes),  the read will return the contents of an 
     * internal read buffer. This internal buffer is updated only when reading PPUDATA, and so is preserved across frames
     * After the CPU reads and gets the contents of the internal buffer, the PPU will immediately update the internal buffer with the byte at 
     * the current VRAM address. Thus, after setting the VRAM address, one should first read this register and discard the result
     * Reading palette data from $3F00-$3FFF works differently. The palette data is placed immediately on the data bus, and hence no dummy 
     * read is required
     * Reading the palettes still updates the internal buffer though, but the data placed in it is the mirrored 
     * nametable data that would appear "underneath" the palette. 
     */
    uint8_t value;
    // 0x4000-0xFFFF mirrors 0x0000-0x3FFF
    if ((currentVRAMAddress & 0x3FFF) < 0x3F00)
    {
        // Reading VRAM address in the range 0x0000-0x3EFF    
        value = internalBuffer; // the read will return the contents of an internal read buffer
        internalBuffer = vram->Read(currentVRAMAddress); // This internal buffer is updated only when reading PPUDATA, and so is preserved across frames
    }
    else
    {
        /*
         * Reading palette data from $3F00-$3FFF works differently. The palette data is placed immediately on the data bus
         * and hence no dummy read is required
         */
        value = vram->Read(currentVRAMAddress);
        /*
         * Reading the palettes still updates the internal buffer though, but the data placed in it is the mirrored 
         * nametable data that would appear "underneath" the palette
         * Why should we minus the address for 0x1000. Because the internalBuffer palette address is from 0x3F00-0x4000 and the address of
         * nametable is 0x2000-0x2FFF (0x3000-0x3EFF mirror 0x2000-0x2FFF) so we need to minus for 0x1000 to make sure the value of internal 
         * buffer is in nametable data
         */
        internalBuffer = vram->Read(currentVRAMAddress - 0x1000);
    }
    currentVRAMAddress = 
    (controlRegister.bits.addressIncrement == 1) ? currentVRAMAddress  + 32
                                                 : currentVRAMAddress  + 1;
    return value;
}

void PPU::NMIChanged()
{   
    bool nmi = (statusRegister.bits.vblank == 1) && (controlRegister.bits.generateNMI == 1);
    if (nmi && !nmiPrevious) // if it is in the Interrup. Don't call it again
    {
        nmiDelay = 10;
    }
    nmiPrevious = nmi;
}

void PPU::CoarseXIncrement()
{
    /*
     * Refer: http://wiki.nesdev.com/w/index.php/PPU_scrolling
     *
     * Pseudocode:
     * if ((v & 0x001F) == 31) // if coarse X == 31
     *   v &= ~0x001F          // coarse X = 0
     *   v ^= 0x0400           // switch horizontal nametable
     * else
     *   v += 1                // increment coarse X
     */
    // The maximum of coarse X value is 31 because there is only 32 tile in a single nametable
    if ((currentVRAMAddress & 0x001F) == 31)
    {
        currentVRAMAddress &= ~0x001F; // coarse X = 0
        currentVRAMAddress ^= 0x0400; // switch horizontal nametable
    } 
    else
    {
        ++currentVRAMAddress; // increment coarse X
    }
}

void PPU::YIncrement()
{
    /*
     * Refer: http://wiki.nesdev.com/w/index.php/PPU_scrolling
     *
     * Pseudocode:
     * if ((v & 0x7000) != 0x7000)        // if fine Y < 7
     *   v += 0x1000                      // increment fine Y
     * else
     *   v &= ~0x7000                     // fine Y = 0
     *   int y = (v & 0x03E0) >> 5        // let y = coarse Y
     *   if (y == 29)
     *     y = 0                          // coarse Y = 0
     *     v ^= 0x0800                    // switch vertical nametable
     *   else if (y == 31)
     *     y = 0                          // coarse Y = 0, nametable not switched
     *   else
     *     y += 1                         // increment coarse Y
     *   v = (v & ~0x03E0) | (y << 5)     // put coarse Y back into v
     */
    if ((currentVRAMAddress & 0x7000) != 0x7000) // if fineYScroll < 7 (fineYScroll has only 3 bits because each tile has only 8 row pixels (0:7))
    {
        currentVRAMAddress  += 0x1000; // increment fine Y   
    }
    else
    {
        currentVRAMAddress &= ~0x7000; // fine Y = 0
        uint8_t y = (currentVRAMAddress & 0x03E0) >> 5; // let y = coarse Y
        if (y == 29) 
        {
            y = 0; // coarse Y = 0
            currentVRAMAddress ^= 0x0800; // switch vertical nametable 
        }
        else if (y == 31)
        {
            y = 0; // coarse Y = 0, nametable not switched
        }
        else
        {
            ++y; // increment coarse Y
        }
        currentVRAMAddress = (currentVRAMAddress & ~0x03E0) | (y << 5); // put coarse Y back into v
    }
}

void PPU::CopyHorizontal()
{
    // For more information refer http://wiki.nesdev.com/w/index.php/PPU_scrolling
    // v: .....F.. ...EDCBA = t: .....F.. ...EDCBA
    currentVRAMAddress = (currentVRAMAddress & 0xFBE0) | (temporaryVRAMAddress & 0x041F);
}

void PPU::CopyVertical()
{
    // For more information refer http://wiki.nesdev.com/w/index.php/PPU_scrolling
    // v: .IHGF.ED CBA..... = t: .IHGF.ED CBA.....
    currentVRAMAddress = (currentVRAMAddress & 0x841F) | (temporaryVRAMAddress & 0x7BE0);
}

void PPU::FetchNametable()
{
    // Refer: http://wiki.nesdev.com/w/index.php/PPU_scrolling
    // tile address = 0x2000 | (v & 0x0FFF)
    uint16_t address = 0x2000 | (currentVRAMAddress & 0xFFF);
    tile.nametableByte = vram->Read(address);
}   

void PPU::FetchAttribute()
{
    // Refer: http://wiki.nesdev.com/w/index.php/PPU_scrolling
    // attribute address = 0x23C0 | (v & 0x0C00) | ((v >> 4) & 0x38) | ((v >> 2) & 0x07)
    uint16_t address = 0x23C0 | (currentVRAMAddress & 0x0C00) | ((currentVRAMAddress >> 4) & 0x38) 
                                       | ((currentVRAMAddress >> 2) & 0x07);
    uint16_t shift = ((currentVRAMAddress  >> 4) & 4) | (currentVRAMAddress  & 2);
    tile.attributeTableByte = ((vram->Read(address) >> shift) & 0x03);
}

void PPU::FetchTileLow()
{
    /*
     * Nametable holds the tile number of the data kept in the Pattern Table. 
     * If the value of nametable is 2 it mean it point to tile 2th of pattern table. The pattern tables store the 8x8 pixel tiles. 
     * Each tile in the pattern table is 16 bytes, made of two planes. The first plane controls bit 0 of the color, the second plane controls bit 1
     * The address of the patterntable = 0x1000 * controlRegister.bits.backgroundPatternTableAddress + nametableByte * 16
     * bits backgroundPatternTableAddress of controlRegister register control which background table should we use (0: $0000; 1: $1000)
     * After getting the address of patterntable we plus it with fineYScroll to get the byte that we want to display on the current scanline
     */
    uint8_t fineYScroll = ((currentVRAMAddress >> 12) & 7);
    uint16_t address = 0x1000 * controlRegister.bits.backgroundPatternTableAddress + tile.nametableByte * 16 + fineYScroll;
    tile.tileLow = vram->Read(address);
}

void PPU::FetchTileHigh()
{
    /*
     * Nametable holds the tile number of the data kept in the Pattern Table. 
     * If the value of nametable is 2 it mean it point to tile 2th of pattern table. The pattern tables store the 8x8 pixel tiles. 
     * Each tile in the pattern table is 16 bytes, made of two planes. The first plane controls bit 0 of the color, the second plane controls bit 1
     * The address of the patterntable = 0x1000 * controlRegister.bits.backgroundPatternTableAddress + nametableByte * 16
     * bits backgroundPatternTableAddress of controlRegister register control which background table should we use (0: $0000; 1: $1000)
     * After getting the address of patterntable we plus it with fineYScroll to get the byte that we want to display on the current scanline
     */
    uint8_t fineYScroll = ((currentVRAMAddress >> 12) & 7);
    uint16_t address = 0x1000 * controlRegister.bits.backgroundPatternTableAddress + tile.nametableByte * 16 + fineYScroll;
    tile.tileHigh = vram->Read(address + 8); 
}

void PPU::StorePaletteIndices()
{
    /*
     * 43210
     * |||||
     * |||++- Pixel value from tile data
     * |++--- Palette number from attribute table or OAM
     * +----- Background/Sprite select (0: background 1: sprite)
     */
    uint32_t data = 0x00000000;
    for (uint8_t i = 0; i < 8; ++i)
    {
        data <<= 4;
        data |= tile.attributeTableByte << 2 | (tile.tileHigh & 0x80) >> 6 | (tile.tileLow & 0x80) >> 7;
        tile.tileHigh <<= 1;
        tile.tileLow <<= 1;
    }
    tile.paletteIndices &= 0xFFFFFFFF00000000;
    tile.paletteIndices |= data;
}

void PPU::SpriteEvaluation()
{
    uint8_t size = controlRegister.bits.spriteSize == 0 ? 8 : 16; // Sprite size (0: 8x8; 1: 8x16)
    spriteCount = 0;
    // Internal memory inside the PPU that contains a display list of up to 64 sprites
    for (uint8_t i = 0; i < 64; ++i)
    {
        //  Byte 0 - Stores the y-coordinate of the top left of the sprite minus 1
        int8_t row = int8_t(scanline) - int8_t(primaryOAM[i * 4]);
        if (row >= 0 && row < size)
        {
            // Y-coordinate of primaryOAM[i] is in range 
            if (spriteCount < 8) // The maximum of sprites on scanline is 8
            {
                secondaryOAM[spriteCount].positionY = primaryOAM[i * 4 + 0];
                secondaryOAM[spriteCount].attribute.byte = primaryOAM[i * 4 + 2];
                secondaryOAM[spriteCount].positionX = primaryOAM[i * 4 + 3];             
                secondaryOAM[spriteCount].isSpriteZero = i == 0 ? true : false;
                FetchSpritePalette(i, row, spriteCount);
                ++spriteCount;
            }   
            else
            {
                // If the number of sprites on the scanline is more than 8 set spriteOverflow bit to 1
                spriteCount = 8;
                statusRegister.bits.spriteOverflow = 1;
                return;
            }           
        }
    }

}

void PPU::FetchSpritePalette(uint8_t i, uint8_t row, uint8_t spriteNumber)
{
    uint16_t address;
    uint8_t tileIndexNumber = primaryOAM[i * 4 + 1];
    if (controlRegister.bits.spriteSize == 0)
    {
        // sprite 8x8
        uint8_t y = secondaryOAM[spriteNumber].attribute.bits.flipVertical == 1 ? 7 - row : row;
        address = 0x1000 * controlRegister.bits.spritePatternTableAddress + tileIndexNumber * 16 + y;
    } 
    else
    {
        // sprite 8x16
        uint8_t y = secondaryOAM[spriteNumber].attribute.bits.flipVertical == 1 ? 15 - row : row;
        /*
         * For 8x16 sprite 
         * tileIndexNumber
         * 76543210
         * ||||||||
         * |||||||+- Bank ($0000 or $1000) of tiles
         * +++++++-- Tile number of top of sprite (0 to 254; bottom half gets the next tile)
         * Thus, the pattern table memory map for 8x16 sprites looks like this:
         * $00: $0000-$001F
         * $01: $1000-$101F
         * $02: $0020-$003F
         * $03: $1020-$103F
         * $04: $0040-$005F
         * [...]
         * $FE: $0FE0-$0FFF
         * $FF: $1FE0-$1FFF
         */
        tileIndexNumber &= 0xFE;
        if (y > 7)
        {
            ++tileIndexNumber;
            y -= 8;
        }
        address = 0x1000 * (tileIndexNumber & 0x01) + tileIndexNumber * 16 + y;
    }
    uint8_t tileHigh = vram->Read(address + 8); 
    uint8_t tileLow = vram->Read(address); 
    uint8_t attribute = secondaryOAM[spriteNumber].attribute.bits.attributeValue;
    for (uint8_t i = 0; i < 8; ++i)
    {
        uint8_t palette = attribute << 2 | (tileHigh & 0x80) >> 6 | (tileLow & 0x80) >> 7;
        if (secondaryOAM[spriteNumber].attribute.bits.flipHorizontal == 1)
        {
            secondaryOAM[spriteNumber].paletteIndices[7 - i] = palette;
        }
        else
        {
            secondaryOAM[spriteNumber].paletteIndices[i] = palette;
        }   
        tileHigh <<= 1;
        tileLow <<= 1;
    }
}

uint8_t PPU::GetBackgroundPixel()
{
    if (maskRegister.bits.showBackground == 0)
    {
        return 0;
    }
    uint32_t data = uint32_t(tile.paletteIndices >> 32);
    uint8_t palette = uint8_t(data >> ((7 - fineXScroll) * 4)); // we have 4 bits for each pixel
    return (palette & 0x0F);
}

uint8_t PPU::GetSpritePixel(uint8_t &index)
{
    if (maskRegister.bits.showSprite == 0)
    {
        return 0;
    }
    for (uint8_t i = 0; i < spriteCount; ++i)
    {
        int8_t offset = int8_t(cycles - 1) - int8_t(secondaryOAM[i].positionX);
        if (offset >= 0 && offset < 8 && secondaryOAM[i].paletteIndices[offset] != 0)
        {
            index = i;
            return secondaryOAM[i].paletteIndices[offset];
        }
    }
    return 0;
}

void PPU::RenderPixel()
{
    /*
     * Priority multiplexer decision table
     * BG pixel    Sprite pixel    Priority    Output
     * 0               0               X       BG ($3F00)
     * 0               1-3             X       Sprite
     * 1-3             0               X       BG
     * 1-3             1-3             0       Sprite
     * 1-3             1-3             1       BG
     *
     * If the sprite has foreground priority or the BG pixel is zero, the sprite pixel is output
     * If the sprite has background priority and the BG pixel is nonzero, the BG pixel is output
     */
    uint8_t x = cycles - 1;
    uint8_t y = scanline;
    uint8_t color;
    uint8_t backgroundColor = GetBackgroundPixel();
    uint8_t index = 0;
    uint8_t spriteColor = GetSpritePixel(index);
    if ((x < 8) && (maskRegister.bits.noBackgroundClipping == 0))
    {
        backgroundColor == 0;
    }
    if ((x < 8) && (maskRegister.bits.noSpriteClipping == 0))
    {
        spriteColor == 0;
    }
    /*
     * The palette entry at $3F00 is the background colour and is used for transparency. Mirroring is used so that every four bytes in
     * the palettes is a copy of $3F00. Therefore $3F04, $3F08,$3F0C, $3F10, $3F14, $3F18 and $3F1C are just copies of $3F00 and the total
     * number of colours in each palette is 13, not 16   
     * Addresses $3F04/$3F08/$3F0C can contain unique data, though these values are not used by the PPU when normally rendering 
     * (since the pattern values that would otherwise select those cells select the backdrop color instead)
     *
     * 43210
     * |||||
     * |||++- Pixel value from tile data
     * |++--- Palette number from attribute table or OAM
     * +----- Background/Sprite select (0: background, 1: sprite)
     */

    bool b = (backgroundColor % 4) != 0;
    bool s = (spriteColor % 4) != 0;
    if (!b && !s)
    {
        color = backgroundColor;
    }
    else if (!b && s)
    {
        color = spriteColor | 0x10; 
    }
    else if (b && !s)
    {
        color = backgroundColor;
    }
    else
    {
        if (secondaryOAM[index].attribute.bits.priority == 0)
        {
            // In front of background
            color = spriteColor | 0x10;
        }
        else
        {
            // Behind background
            color = backgroundColor;
        }
        // Check zero hit
        if (secondaryOAM[index].isSpriteZero && x != 255)
        {
            statusRegister.bits.sprite0Hit = 1;
        }
    }
    backBuffer[scanline][cycles - 1] = vram->Read(color | 0x3F00);
}

void PPU::Step()
{
    if (nmiDelay > 0)
    {
        --nmiDelay;
         // Call NMI interrup
        #ifndef _DEBUG_
            if ((nmiDelay == 0) && (statusRegister.bits.vblank == 1) && (controlRegister.bits.generateNMI == 1))
            {
                cpu->TriggerNMI();
            }
        #endif
    }
    if (((maskRegister.bits.showBackground == 1) || (maskRegister.bits.showSprite == 1)) && oddFrame && (scanline == 261) && (cycles == 339))
    {
        /*
         * With rendering enabled, each odd PPU frame is one PPU clock shorter than normal. This is done by skipping the first idle tick 
         * on the first visible scanline (by jumping directly from (261,339) on the pre-render scanline to (0,0) on the first visible scanline 
         * and doing the last cycle of the last dummy nametable fetch there instead
         */
        scanline = 0;
        cycles = 0;
        oddFrame = !oddFrame;
    }
    else
    {
        ++cycles;
        if (cycles > 340)
        {
            cycles = 0;
            ++scanline;
            if (scanline > 261)
            {
                scanline = 0;
                oddFrame = !oddFrame;
            }
        }
    }

    bool preRenderScanline = (scanline == 261);
    bool visibleScanline = (scanline <= 239);
    bool postRenderScanline = (scanline == 240);
    bool VBScanline = (scanline >= 241 && scanline <= 260);

    bool visibleCycle = (cycles >= 1 && cycles <= 256);
    bool fetchCycle = visibleCycle | (cycles >= 321 && cycles <= 336); //   337->340 is unused NT fetch
    if ((maskRegister.bits.showBackground == 1) || (maskRegister.bits.showSprite == 1)) // If renderring enable
    {
        if (visibleScanline && visibleCycle)
        {
            // Render pixel at (x, y) = (fineXScroll, scanLine)
            RenderPixel();
        }   
        /*
         * Refer 
         * http://wiki.nesdev.com/w/index.php/PPU_rendering 
         * and 
         * http://wiki.nesdev.com/w/images/d/d1/Ntsc_timing.png 
         * for more information
         */
        // Background
        if ((preRenderScanline || visibleScanline) && fetchCycle)
        {
            tile.paletteIndices  <<= 4;
            switch(cycles % 8)
            {
                case 1:
                    FetchNametable();
                    break;
                case 3:
                    FetchAttribute();
                    break;
                case 5:
                    FetchTileLow();
                    break;
                case 7:
                    FetchTileHigh();
                    break;
                case 0:
                    StorePaletteIndices();
                    if (cycles != 256)
                    {
                        CoarseXIncrement();
                    }   
                    else
                    {
                        YIncrement();
                    }                
                    break;
            }
        }
        if ((preRenderScanline || visibleScanline) && cycles == 257)
        {
            CopyHorizontal();
        }
        if (preRenderScanline && cycles >= 280 && cycles <= 304)
        {
            CopyVertical();
        }
        // Sprite
        if (cycles == 257) //  Out of visible cycle
        {
            if (visibleScanline || preRenderScanline)
            {
                // Fetch sprites for the next scanline
                SpriteEvaluation(); 
            }
            else
            {
                spriteCount = 0;
            }
        }        
    }

    if (scanline == 241 && cycles == 1)
    {
        // Set VB flag
        statusRegister.bits.vblank = 1;
        SwapBuffer(); 
        NMIChanged();    
    }
    if (preRenderScanline && cycles == 1)
    {
        // Clear VB flag, sprite zero hit, sprite overflow
        statusRegister.bits.vblank = 0;
        NMIChanged();
        statusRegister.bits.sprite0Hit = 0;
        statusRegister.bits.spriteOverflow = 0;
    }
}

void PPU::SwapBuffer()
{
    uint8_t (*temp)[SCREEN_WIDTH];
    temp = frontBuffer;
    frontBuffer = backBuffer;
    backBuffer = frontBuffer;
}