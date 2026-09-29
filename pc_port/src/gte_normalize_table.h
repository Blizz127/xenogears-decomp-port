/* gte_normalize_table.h -- reciprocal square-root table for vector
 * normalisation (VectorNormal / VectorNormalS / VectorNormalSS).
 *
 * The normaliser shifts |v|^2 into a mantissa m in [0x40, 0x100) (1.0..4.0
 * in 1/64 steps) and scales each component by 4096 / sqrt(m / 64).  Entry
 * i = m - 0x40 is therefore floor(4096 / sqrt((64 + i) / 64)), which is
 * exactly the integer square root of floor(2^30 / (64 + i)).  The table is
 * computed from that definition at startup; no SDK data is copied.  Six
 * trailing zero entries pad the table to 198 halfwords like the retail
 * layout the port's callers index against.
 */
#ifndef XENO_GTE_NORMALIZE_TABLE_H
#define XENO_GTE_NORMALIZE_TABLE_H

#include <stdint.h>

#define XENO_GTE_NORM_TABLE_USED 192
#define XENO_GTE_NORM_TABLE_LEN  198

static inline int16_t XenoGteNormTableEntry(int i)
{
    uint32_t x, r, bit;
    if (i < 0 || i >= XENO_GTE_NORM_TABLE_USED)
        return 0;
    x = (uint32_t)((1u << 30) / (uint32_t)(64 + i));
    /* bitwise integer square root */
    r = 0;
    for (bit = 1u << 30; bit != 0; bit >>= 2) {
        if (x >= r + bit) {
            x -= r + bit;
            r = (r >> 1) + bit;
        } else {
            r >>= 1;
        }
    }
    return (int16_t)r;
}

static inline void XenoGteNormTableFill(int16_t* table)
{
    int i;
    for (i = 0; i < XENO_GTE_NORM_TABLE_LEN; i++)
        table[i] = XenoGteNormTableEntry(i);
}

#endif
