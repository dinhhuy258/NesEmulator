#ifndef _PLATFORMS_H_
#define _PLATFORMS_H_

//#define _DEBUG_
// NES
#define NES_FILE "../rom/nestest.nes"
//#define CPU_FREQUENCY 1789773
#define CPU_FREQUENCY 1789772.7272727272727272
// Display
#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT 240
// Bit
#define SET 1
#define CLEAR 0
// Flag
#define FLAG_CARRY       0x01
#define FLAG_ZERO        0x02
#define FLAG_INTERRUPT   0x04
#define FLAG_DECIMAL     0x08
#define FLAG_BREAK       0x10
#define FLAG_CONSTANT    0x20
#define FLAG_OVERFLOW    0x40
#define FLAG_NEGATIVE    0x80
// The interrup vector
#define NMI_VECTOR_LOW      0xFFFA
#define NMI_VECTOR_HIGH     0xFFFB
#define RESET_VECTOR_LOW    0xFFFC
#define RESET_VECTOR_HIGH   0xFFFD
#define IRQ_VECTOR_LOW      0xFFFE
#define IRQ_VECTOR_HIGH     0xFFFF
// Macro
#define SAFE_DEL(p) if ((p) != NULL) { delete (p); (p) = NULL;}
#define SAFE_DEL_ARRAY(p) if ((p) != NULL) { delete[] (p); (p) = NULL;}
#define LOGI(...) printf(__VA_ARGS__); printf("\n");

#endif //_PLATFORMS_H_