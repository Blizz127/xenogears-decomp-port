#ifndef XENO_PSYQ_INLINE_C_H
#define XENO_PSYQ_INLINE_C_H

#ifdef XENO_PC_PORT

#include <psx/types.h>
#include <psx/inline_c.h>

/* Additional grouped transfers used by the game's console call sites. */
#define gte_ldv01(first, second) do { \
    const void *xeno_v0 = (first); \
    const void *xeno_v1 = (second); \
    gte_ldv0(xeno_v0); \
    gte_ldv1(xeno_v1); \
} while (0)

#define gte_ldv3c(vertices) do { \
    const unsigned char *xeno_vertices = (const unsigned char *)(vertices); \
    gte_ldv3(xeno_vertices, xeno_vertices + 8, xeno_vertices + 16); \
} while (0)

#define gte_stsxy01(first, second) do { \
    gte_stsxy0(first); \
    gte_stsxy1(second); \
} while (0)

#else

/* Hardware register layout and transfer delays:
 * https://psx-spx.consoledev.net/geometrytransformationenginegte/
 * Operands denote 32-bit words on the little-endian PlayStation. */
#define XENO_GTE_READ_WORD(reg, destination) \
    __asm__ volatile ("swc2 $" #reg ",0(%0)" : : "r"(destination) : "memory")

#define XENO_GTE_WRITE_WORD(reg, source) \
    __asm__ volatile ("lwc2 $" #reg ",0(%0)" : : "r"(source) : "memory")

#define XENO_GTE_WRITE_INTEGER(reg, value) \
    __asm__ volatile ("mtc2 %0,$" #reg : : "r"(value))

#define XENO_GTE_CONTROL(reg, value) \
    __asm__ volatile ("ctc2 %0,$" #reg : : "r"(value))

/* The matrix and long-vector interfaces use word-aligned storage. Reading
 * through a machine load also permits callers to use their own struct types. */
static __inline__ unsigned int xeno_gte_word(const unsigned char *bytes)
{
    unsigned int value;
    __asm__ volatile ("lw %0,0(%1)\n\tnop" : "=r"(value) : "r"(bytes) : "memory");
    return value;
}

static __inline__ void gte_SetRotMatrix(const void *matrix)
{
    const unsigned char *m = (const unsigned char *)matrix;
    XENO_GTE_CONTROL(0, xeno_gte_word(m));
    XENO_GTE_CONTROL(1, xeno_gte_word(m + 4));
    XENO_GTE_CONTROL(2, xeno_gte_word(m + 8));
    XENO_GTE_CONTROL(3, xeno_gte_word(m + 12));
    XENO_GTE_CONTROL(4, xeno_gte_word(m + 16));
}

static __inline__ void gte_SetTransMatrix(const void *matrix)
{
    const unsigned char *m = (const unsigned char *)matrix;
    XENO_GTE_CONTROL(5, xeno_gte_word(m + 20));
    XENO_GTE_CONTROL(6, xeno_gte_word(m + 24));
    XENO_GTE_CONTROL(7, xeno_gte_word(m + 28));
}

static __inline__ void gte_SetBackColor(unsigned int r, unsigned int g, unsigned int b)
{
    XENO_GTE_CONTROL(13, r << 4);
    XENO_GTE_CONTROL(14, g << 4);
    XENO_GTE_CONTROL(15, b << 4);
}

static __inline__ void gte_ldv01(const void *first, const void *second)
{
    XENO_GTE_WRITE_WORD(0, first);
    XENO_GTE_WRITE_WORD(1, (const unsigned char *)first + 4);
    XENO_GTE_WRITE_WORD(2, second);
    XENO_GTE_WRITE_WORD(3, (const unsigned char *)second + 4);
}

static __inline__ void gte_ldv3c(const void *vertices)
{
    const unsigned char *bytes = (const unsigned char *)vertices;
    gte_ldv01(bytes, bytes + 8);
    XENO_GTE_WRITE_WORD(4, bytes + 16);
    XENO_GTE_WRITE_WORD(5, bytes + 20);
}

static __inline__ void gte_ldlv0(const void *vector)
{
    const unsigned char *bytes = (const unsigned char *)vector;
    unsigned int x = xeno_gte_word(bytes);
    unsigned int y = xeno_gte_word(bytes + 4);
    XENO_GTE_WRITE_INTEGER(0, (x & 0xffffu) | (y << 16));
    XENO_GTE_WRITE_WORD(1, bytes + 8);
}

#define gte_ldrgb(source) XENO_GTE_WRITE_WORD(6, source)
#define gte_strgb(destination) XENO_GTE_READ_WORD(22, destination)
#define gte_stsxy0(destination) XENO_GTE_READ_WORD(12, destination)
#define gte_stsxy1(destination) XENO_GTE_READ_WORD(13, destination)
#define gte_stsxy2(destination) XENO_GTE_READ_WORD(14, destination)
#define gte_stsxy(destination) gte_stsxy2(destination)
#define gte_stsz(destination) XENO_GTE_READ_WORD(19, destination)

static __inline__ void gte_stsxy01(void *first, void *second)
{
    gte_stsxy0(first);
    gte_stsxy1(second);
}

static __inline__ void gte_stsz3(void *first, void *second, void *third)
{
    XENO_GTE_READ_WORD(17, first);
    XENO_GTE_READ_WORD(18, second);
    gte_stsz(third);
}

static __inline__ void gte_stlvnl(void *vector)
{
    unsigned char *bytes = (unsigned char *)vector;
    XENO_GTE_READ_WORD(25, bytes);
    XENO_GTE_READ_WORD(26, bytes + 4);
    XENO_GTE_READ_WORD(27, bytes + 8);
}

/* Allow two instructions after a transfer before issuing a GTE command. */
#define gte_rt() __asm__ volatile ("nop\n\tnop\n\t.word 0x4a480012" : : : "memory")
#define gte_dpcs() __asm__ volatile ("nop\n\tnop\n\t.word 0x4a780010" : : : "memory")

#endif
#endif
