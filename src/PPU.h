#ifndef _PPU_H_
#define _PPU_H_

#include "CPU.h"
#include "MemoryPPU.h"
#include "Platforms.h"

struct Sprite
{
    uint8_t positionY;
    uint8_t positionX;
    uint8_t paletteIndices[8];
    union Attribute
    { 
        struct AttributeBits
        {
            uint8_t attributeValue : 2;
            uint8_t unimplemented : 3;
            uint8_t priority : 1; // (0: in front of background; 1: behind background)
            uint8_t flipHorizontal: 1;
            uint8_t flipVertical : 1;
        }bits;
        uint8_t byte;
    }attribute;
    bool isSpriteZero;
};

struct Tile
{
    /*
     * - 2 16-bit shift registers - These contain the bitmap data for two tiles
     * - 2 8-bit shift registers - These contain the palette attributes for the lower 8 pixels of the 16-bit shift register
     * These registers are fed by a latch which contains the palette attribute for the next tile. Every 8 cycles, the latch is loaded with the
     * palette attribute for the next tile
     * In this program I used nametableByte, attributeTableByte, tileLow, tileHigh to fetch the data of tile then I use 64 bit variable to
     * store 2 tiles (1 tile = 8 * 4 = 32bits)
     */
    uint8_t nametableByte; // tile address = 0x2000 | (v & 0x0FFF)     
    uint8_t attributeTableByte; // attribute address = 0x23C0 | (v & 0x0C00) | ((v >> 4) & 0x38) | ((v >> 2) & 0x07)
    uint8_t tileLow;
    uint8_t tileHigh;

    uint64_t paletteIndices;
};

class PPU
{
    public:
        PPU(Memory *vram);
        void SetCPU(CPU *cpu);
        void WriteRegister(uint16_t address, uint8_t value);
        uint8_t ReadRegister(uint16_t address);
        void Step();
        uint8_t paletteIndex[SCREEN_HEIGHT][SCREEN_WIDTH];
        
    private:
        // CPU 8266
        CPU *cpu;
        // Video ram of PPU
        Memory *vram;
        /*
         * The PPU renders 262 scanlines per frame. Each scanline lasts for 341 PPU clock cycles (113.667 CPU clock cycles; 1 CPU cycle = 3 PPU cycles), 
         * with each clock cycle producing one pixel
         * The scanline has value in range  0-261 
         * where:
         * - Visible scanlines (0-239)
         * These are the visible scanlines, which contain the graphics to be displayed on the screen
         * This includes the rendering of both the background and the sprites. During these scanlines, the PPU is busy fetching data, 
         * so the program should not access PPU memory during this time, unless rendering is turned off
         * - Post-render scanline (240)
         * The PPU just idles during this scanline. Even though accessing PPU memory from the program would be safe here, the VBlank flag isn't set until after this scanline.
         * - Vertical blanking lines (241-260)
         * The VBlank flag of the PPU is set at tick 1 (the second tick) of scanline 241, where the VBlank NMI also occurs 
         * The PPU makes no memory accesses during these scanlines, so PPU memory can be freely accessed by the program
         * - Pre-render scanline (261)
         * This is a dummy scanline, whose sole purpose is to fill the shift registers with the data for the first two tiles of the next scanline
         * Although no pixels are rendered for this scanline, the PPU still makes the same memory accesses it would for a regular scanline
         * This scanline varies in length, depending on whether an even or an odd frame is being rendered. For odd frames, 
         * the cycle at the end of the scanline is skipped (this is done internally by jumping directly from (339,261) to (0,0), 
         * replacing the idle tick at the beginning of the first visible scanline with the last tick of the last dummy nametable fetch)
         * For even frames, the last cycle occurs normally.
         * During pixels 280 through 304 of this scanline, the vertical scroll bits are reloaded if rendering is enabled.
         */
        uint16_t scanline; 
        /*
         * Each scanline lasts for 341 PPU clock cycles with each clock cycle producing one pixel
         * The cycle has value in range 0-340
         * - Cycle 0
         * This is an idle cycle. The value on the PPU address bus during this cycle appears to be the same CHR address that is later used to fetch 
         * the low background tile byte starting a2
         * - Cycle 1-256
         * The data for each tile is fetched during this phase. Each memory access takes 2 PPU cycles to complete, and 4 must be performed per tile:
         *     1: Nametable byte (Fetch a nametable entry from $2000-$2FBF)
         *     2: Attribute table byte (Fetch the corresponding attribute table entry from $23C0-$2FFF and increment the current VRAM address within the same row)
         *     3: Tile bitmap low (Fetch the low-order byte of a 8x1 pixel sliver of pattern table from $0000-$0FF7 or $1000-$1FF7)
         *     4: Tile bitmap high (Fetch the high-order byte of this sliver from an address 8 bytes higher)
         * Finally PPU turn the attribute data and the pattern table data into palette indices, and combine them with data from sprite data using priority
         * The data fetched from these accesses is placed into internal latches, and then fed to the appropriate shift registers when 
         * it's time to do so (every 8 cycles). Because the PPU can only fetch an attribute byte every 8 cycles, each sequential string 
         * of 8 pixels is forced to have the same palette attribute
         * The shifters are reloaded during ticks 9, 17, 25, ..., 257.
         * At the beginning of each scanline, the data for the first two tiles is already loaded into the shift registers 
         * (and ready to be rendered), so the first tile that gets fetched is Tile 3
         * - Cycles 257-320
         * The tile data for the sprites on the next scanline are fetched here. Again, each memory access takes 2 PPU cycles to complete,
         * and 4 are performed for each of the 8 sprites:
         *     1: Garbage nametable byte
         *     2: Garbage nametable byte
         *     3: Tile bitmap low
         *     4: Tile bitmap high (+8 bytes from tile bitmap low)
         * - Cycles 321-336
         * This is where the first two tiles for the next scanline are fetched, and loaded into the shift registers
         * Again, each memory access takes 2 PPU cycles to complete, and 4 are performed for the two tiles:
         *     1: Nametable byte
         *     2: Attribute table byte
         *     3: Tile bitmap low
         *     4: Tile bitmap high (+8 bytes from tile bitmap low)
         * - Cycles 337-340
         * Two bytes are fetched, but the purpose for this is unknown. These fetches are 2 PPU cycles each
         *     1: Nametable byte
         *     2: Nametable byte
         * Both of the bytes fetched here are the same nametable byte that will be fetched at the beginning of the next scanline (tile 3, in other word
         */
        uint16_t cycles;

        // 0x2000 Write
        union PPU_CTRL 
        {
            /*
             * 7  bit  0
             * ---- ----
             * VPHB SINN
             */
            struct PPU_CTRL_BITS
            {
                /*
                 * Base nametable address
                 * (0 = $2000; 1 = $2400; 2 = $2800; 3 = $2C00)
                 */
                uint8_t nametableAddress : 2;
                /*
                 * VRAM address increment per CPU read/write of PPUDATA
                 * (0: add 1, going across; 1: add 32, going down)
                 */
                uint8_t addressIncrement : 1;
                /*
                 * Sprite pattern table address for 8x8 sprites
                 * (0: $0000; 1: $1000; ignored in 8x16 mode)
                 */
                uint8_t spritePatternTableAddress : 1;
                // Background pattern table address (0: $0000; 1: $1000)
                uint8_t backgroundPatternTableAddress : 1;
                // Sprite size (0: 8x8; 1: 8x16)
                uint8_t spriteSize : 1;
                // PPU master/slave select
                uint8_t P : 1; 
                // Generate an NMI at the start of the vertical blanking interval (0: off; 1: on)
                uint8_t generateNMI : 1;
            } bits;
            uint8_t byte;
        }controlRegister;

        // 0x2001 Write
        union PPU_MASK
        {
            /*
             * 7  bit  0
             * ---- ----
             * BGRs bMmG
             */
            struct PPU_MASK_BITS
            {
                // Grayscale (0: normal color, 1: produce a greyscale display)
                uint8_t grayScale : 1;
                // Show background in leftmost 8 pixels of screen (1: show, 0: hide)
                uint8_t noBackgroundClipping : 1;
                // Show sprite in leftmost 8 pixels of screen (1: show, 0: hide)
                uint8_t noSpriteClipping : 1;
                // Show background (1: show, 0: hide)
                uint8_t showBackground : 1;
                // Show sprite (1: show, 0: hide)
                uint8_t showSprite : 1;
                // R-G-B (NTSC colors). In PAL and Dendy swaps green and red
                uint8_t color : 3;
            }bits;
            uint8_t byte;
        }maskRegister;

        // 0x2002 Read 
        union PPU_STATUS
        {
            /*
             * 7  bit  0
             * ---- ----
             * VSO. ....
             */
            struct PPU_STATUS_BITS
            {
                // Least significant bits previously written into a PPU register
                uint8_t reserved : 5; 
                /*
                 * Sprite overflow. The intent was for this flag to be set whenever more than eight sprites appear on a scanline
                 * This flag is set during sprite evaluation
                 * Cleared at dot 1 (the second dot) of the pre-render line.
                 */
                uint8_t spriteOverflow : 1;
                /*
                 * Set when a nonzero pixel of sprite 0 overlaps a nonzero background pixel
                 * Cleared at dot 1 of the pre-render line.  
                 * Used for raster timing.
                 * - Sprite 0 hit does not happen:
                 *   + If background or sprite rendering is disabled in PPUMASK ($2001)
                 *   + At x=0 to x=7 if the left-side clipping window is enabled (if bit 2 or bit 1 of PPUMASK is 0)
                 *   + At x=255, for an obscure reason related to the pixel pipeline
                 *   + At any pixel where the background or sprite pixel is transparent (2-bit color index from the CHR pattern is %00)
                 *   + If sprite 0 hit has already occurred this frame. Bit 6 of PPUSTATUS ($2002) is cleared to 0 at dot 1 of the
                 *     pre-render line. This means only the first sprite 0 hit in a frame can be detected
                 * - Sprite 0 hit happens regardless of the following::
                 *   + Sprite priority. Sprite 0 can still hit the background from behind.
                 *   + The pixel colors. Only the CHR pattern bits are relevant, not the actual rendered colors, and any CHR color index except %00 is considered opaque
                 *   + The palette. The contents of the palette are irrelevant to sprite 0 hits. For example: a black ($0F) sprite pixel can hit a black ($0F) background as long as neither is the transparent color index %00.
                 *   + The PAL PPU blanking on the left and right edges at x=0, x=1, and x=254 
                 */
                uint8_t sprite0Hit : 1;
                /*
                 * Vertical blank has started (0: not in vblank; 1: in vblank)
                 * Set at dot 1 of line 241 (the line *after* the post-render line) 
                 * Cleared after reading $2002 and at dot 1 of the pre-render line 
                 * Read PPUSTATUS: Return old status of NMI_occurred in bit 7, then set NMI_occurred to false
                 */
                uint8_t vblank : 1;
            }bits;
            uint8_t byte;
        }statusRegister;

        /*
         * 0x2003 Write. OAMADDR is set to 0 during each of ticks 257-320 of the pre-render and visible scanlines.
         * Writes OAMDATA will increment OAMADDR after the write
         * Reads during vertical or forced blanking return the value from OAM at that address but do not increment.
         */    
        uint8_t oamAddress;
        /* 
         * 0x2004 Read/Write
         * Write OAM data here. Writes will increment OAMADDR after the write 
         * Reads during vertical or forced blanking return the value from OAM at that address but do not increment
         * There are a maximum of 64 sprites, each of which uses four bytes in SPR-RAM
         * Byte 0 - Stores the y-coordinate of the top left of the sprite minus 1
         *   + Sprite data is delayed by one scanline. You must subtract 1 from the sprite's Y coordinate before writing it here 
         *     Hide a sprite by writing any values in $EF-$FF here. Sprites are never displayed on the first line of the picture, 
         *     and it is impossible to place a sprite partially off the top of the screen.
         * Byte 1 - Index number of the sprite in the pattern tables
         *   + For 8x8 sprites, this is the tile number of this sprite within the pattern table selected in bit 3 of PPUCTRL ($2000)
         *   + For 8x16 sprites, the PPU ignores the pattern table selection and selects a pattern table from bit 0 of this number
         *     76543210
         *     ||||||||
         *     |||||||+- Bank ($0000 or $1000) of tiles
         *     +++++++-- Tile number of top of sprite (0 to 254; bottom half gets the next tile)
         * Byte 2 - Stores the attributes of the sprite.
         *   + 76543210
         *     ||||||||
         *     ||||||++- Palette (4 to 7) of sprite
         *     |||+++--- Unimplemented
         *     ||+------ Priority (0: in front of background; 1: behind background)
         *     |+------- Flip sprite horizontally
         *     +-------- Flip sprite vertically
         *   + Flipping does not change the position of the sprite's bounding box, just the position of pixels within the sprite
         *     For example, a sprite covers (120, 130) through (127, 137), it'll still cover the same area when flipped
         * Byte 3
         *   + X position of left side of sprite
         */
        // Sprite rendering
        uint8_t primaryOAM[64 * 4];
        // Holds 8 sprites for the current scanline
        Sprite secondaryOAM[8];
        // The maximum of spriteCount variable is 8
        uint8_t spriteCount;
        
        // Background rendering 
        /*
         * PPUSCROLL an PPUADDR use the same memory. They have a shared internal register and using PPUADDR will overwrite the PPUSCROLL.
         * The scroll position written to PPUSCROLL is applied at the end of vertical blanking, just before rendering begins therefore 
         * these writes need to occur before the end of vblank
         * Also, because writes to PPUADDR ($2006) can overwrite the scroll position, the two writes to PPUSCROLL must be done after any 
         * updates to VRAM using PPUADDR     
         */
        union PPU_VRAM_ADDRESS
        {
            /*
             * The 15 bit registers currentVRAMAddress and temporaryVRAMAddress are composed this way during rendering:
             *  yyy NN YYYYY XXXXX
             *  ||| || ||||| +++++-- coarse X scroll
             *  ||| || +++++-------- coarse Y scroll
             *  ||| ++-------------- nametable select
             *  +++----------------- fine Y scroll      
             */
            struct PPUSCROLLBits
            {
                uint8_t coarseXScroll : 5;
                uint8_t coarseYScroll : 5;
                uint8_t nametableSelect : 2;
                uint8_t fineYScroll : 3;
                uint8_t reserved : 1;
            }bits;
            uint16_t value;
        }currentVRAMAddress, temporaryVRAMAddress;  // 0x2005, 0x2006 x2Write. PPU SCROLL and PPU ADDRESS
        uint8_t fineXScroll; // x 3 bits
        uint8_t toggle; // w 1 bits first/second write toggle 
        Tile tile;
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
        uint8_t internalBuffer; //  used in read data at address $2007
        /*
         * - The PPU has an even/odd flag that is toggled every frame, regardless of whether rendering is enabled or disabled
         * - With rendering disabled (background and sprites disabled in PPUMASK ($2001)), each PPU frame is 341 * 262 = 89342 
         * PPU clocks long. There is no skipped clock every other frame
         * - With rendering enabled, each odd PPU frame is one PPU clock shorter than normal. This is done by skipping the first
         * idle tick on the first visible scanline (by jumping directly from (261,339) on the pre-render scanline to (0,0) on the first
         * visible scanline and doing the last cycle of the last dummy nametable fetch there instead
         */
        bool oddFrame;

        // 0x2000
        void WriteControl(uint8_t value);
        // 0x2001
        void WriteMask(uint8_t value);
        // 0x2003
        void WriteOAMAddress(uint8_t value);
        // 0x2004
        void WriteOAMData(uint8_t value);
        // 0x2005
        void WriteScroll(uint8_t value);
        // 0x2006
        void WriteAddress(uint8_t value);
        // 0x2007
        void WriteData(uint8_t value);
        // 0x4014
        void WriteDMA(uint8_t value);

        // 0x2002
        uint8_t ReadStatus();
        // 0x2004
        uint8_t ReadOAMData();
        // 0x2007
        uint8_t ReadData();
        // The coarse X component of v needs to be incremented when the next tile is reached
        void CoarseXIncrement();
        /*
         * If rendering is enabled, fine Y is incremented at dot 256 of each scanline, overflowing to coarse Y, and 
         * finally adjusted to wrap among the nametables vertically
         */
        void YIncrement();
        void CopyHorizontal();
        void CopyVertical();

        void FetchNametable();
        void FetchAttribute();
        void FetchTileLow();
        void FetchTileHigh();
        void StorePaletteIndices();

        void SpriteEvaluation();
        void FetchSpritePalette(uint8_t i, uint8_t row, uint8_t spriteNumber);

        uint8_t GetBackgroundPixel();
        uint8_t GetSpritePixel(uint8_t &index);
        void RenderPixel();
};

#endif