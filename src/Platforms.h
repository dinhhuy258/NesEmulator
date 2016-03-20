#ifndef _PLATFORMS_H_
#define _PLATFORMS_H_

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

#endif //_PLATFORMS_H_