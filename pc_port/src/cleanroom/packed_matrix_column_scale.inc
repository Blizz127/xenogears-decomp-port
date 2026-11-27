#ifndef XENO_PACKED_MATRIX_COLUMN_SCALE
#define XENO_PACKED_MATRIX_COLUMN_SCALE

typedef u32 PackedMatrixWord __attribute__((__may_alias__));

/* The console projection scales each column of its packed fixed-point matrix.
 * The last coefficient and its padding occupy one signed output word. */
static s32 packedMatrixCoefficient(u32 bits, s32 factor)
{
    return (s32)((u32)(s32)(s16)bits * (u32)factor) >> 12;
}

static u32 packedMatrixScalePair(u32 pair, s32 first, s32 second)
{
    u16 low = (u16)packedMatrixCoefficient(pair, first);
    u32 high = (u32)packedMatrixCoefficient(pair >> 16, second);
    return (high << 16) | low;
}

static void packedMatrixScaleColumns(MATRIX *matrix, const VECTOR *factors)
{
    volatile PackedMatrixWord *basis = (void *)matrix;
    const volatile VECTOR *axes = factors;
    u32 first = basis[0];
    s32 x = axes->vx;
    s32 y = axes->vy;
    s32 z = axes->vz;
    u32 second = basis[1];
    basis[0] = packedMatrixScalePair(first, x, y);
    u32 third = basis[2];
    basis[1] = packedMatrixScalePair(second, z, x);
    u32 fourth = basis[3];
    basis[2] = packedMatrixScalePair(third, y, z);
    u32 last = basis[4];
    basis[3] = packedMatrixScalePair(fourth, x, y);
    basis[4] = (u32)packedMatrixCoefficient(last, z);
}

#endif
