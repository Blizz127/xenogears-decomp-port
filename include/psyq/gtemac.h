#ifndef XENO_GEOMETRY_OPERATIONS_H
#define XENO_GEOMETRY_OPERATIONS_H

#include "libgte.h"
#include "inline_c.h"

#ifdef XENO_PC_PORT

/* The other three composite operations are functions in the native shim.
 * Screen coordinates occupy one packed word at each of these addresses. */
static __inline__ void gte_NormalClip(const void *first, const void *second,
                                      const void *third, void *area)
{
    int a, b, c, result;
    __builtin_memcpy(&a, first, sizeof(a));
    __builtin_memcpy(&b, second, sizeof(b));
    __builtin_memcpy(&c, third, sizeof(c));
    result = NormalClip(a, b, c);
    __builtin_memcpy(area, &result, sizeof(result));
}

#else

#define gte_ApplyMatrixSV(matrix, in, out) ApplyMatrixSV((matrix), (in), (out))
#define gte_RotTransPers(in, xy, cue, flags, depth) \
    (*(depth) = RotTransPers((in), (int *)(xy), (cue), (flags)))

/* PSX-SPX, GTE NCLIP and OP. OP uses the diagonal of the rotation
 * matrix for one operand and the IR vector for the second. */
static __inline__ void gte_NormalClip(const void *a, const void *b, const void *c, void *area)
{
    XENO_GTE_WRITE_WORD(12, a);
    XENO_GTE_WRITE_WORD(13, b);
    XENO_GTE_WRITE_WORD(14, c);
    __asm__ volatile ("nop\n\tnop\n\t.word 0x4b400006" : : : "memory");
    XENO_GTE_READ_WORD(24, area);
}

static __inline__ void gte_OuterProduct12(const void *first, const void *second, void *out)
{
    const unsigned char *a = (const unsigned char *)first;
    const unsigned char *b = (const unsigned char *)second;
    XENO_GTE_CONTROL(0, xeno_gte_word(a));
    XENO_GTE_CONTROL(2, xeno_gte_word(a + 4));
    XENO_GTE_CONTROL(4, xeno_gte_word(a + 8));
    XENO_GTE_WRITE_WORD(9, b);
    XENO_GTE_WRITE_WORD(10, b + 4);
    XENO_GTE_WRITE_WORD(11, b + 8);
    __asm__ volatile ("nop\n\tnop\n\t.word 0x4b78000c" : : : "memory");
    gte_stlvnl(out);
}

#endif
#endif
