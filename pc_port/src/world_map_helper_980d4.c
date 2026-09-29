/*
 * World-map helper 0x800980D4 (position wrap / paging gate).
 * W34C5 re-derivation from disc/world_map.bin; retail PCs cited inline.
 */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_980d4.h"

#define D_8009D558  0x8009D558u  /* paging flags (u16)  */
#define D_8009C838  0x8009C838u  /* map_x (s16)         */
#define D_8009C83C  0x8009C83Cu  /* map_z (s16)         */
#define D_8009D808  0x8009D808u  /* (old stub target)   */

static s32 d4_lw(u32 a) { s32 v; memcpy(&v, PSX_ADDR(a), 4); return v; }
static void d4_sw(u32 a, s32 v) { memcpy(PSX_ADDR(a), &v, 4); }
static u16 d4_lhu(u32 a) { u16 v; memcpy(&v, PSX_ADDR(a), 2); return v; }
static void d4_sh(u32 a, u16 v) { memcpy(PSX_ADDR(a), &v, 2); }

static s32 d4_sra(s32 value, unsigned shift)
{
    return value < 0 ? -(((-value) - 1) >> shift) - 1 : value >> shift;
}

void wm_800980D4(u32 pos_vec)
{
#if defined(WM_980D4_MUTANT_M4)             /* old shape: queue-counter stub */
    (void)pos_vec; (void)d4_lw; (void)d4_lhu; (void)d4_sh; (void)d4_sra;
    d4_sw(D_8009D808, 0);
    return;
#else
    s32 x = d4_lw(pos_vec);                   /* 0x800980D8 */
    s32 z = d4_lw(pos_vec + 8u);              /* 0x800980DC */

    d4_sh(D_8009D558, 0u);                    /* 0x800980E8 */

    if (x < -0x800000) {                      /* 0x800980EC slt v1,0xFF800000 */
        d4_sw(pos_vec, x + 0x800000);         /* 0x800980F8-FC */
        d4_sh(D_8009D558, 4u);                /* 0x80098104 -> 0x80098124 */
    } else if (0x800000 < x) {                /* 0x8009810C */
        d4_sw(pos_vec, x - 0x800000);         /* 0x80098114-18 */
        d4_sh(D_8009D558, 8u);                /* 0x8009811C -> 0x80098124 */
    }

    if (z < -0x800000) {                      /* 0x8009812C */
        d4_sw(pos_vec + 8u, z + 0x800000);    /* 0x80098144-4C */
        d4_sh(D_8009D558, (u16)(d4_lhu(D_8009D558) | 1u)); /* 0x80098148-54 */
    } else if (0x800000 < z) {                /* 0x80098164 */
        d4_sw(pos_vec + 8u, z - 0x800000);    /* 0x8009817C-84 */
        d4_sh(D_8009D558, (u16)(d4_lhu(D_8009D558) | 2u)); /* 0x80098180-8C */
    }

    /* 0x80098190-0x800981BC: map = (pos sra 23) + 2, from the stored value. */
    d4_sh(D_8009C838, (u16)(d4_sra(d4_lw(pos_vec), 23) + 2));
    d4_sh(D_8009C83C, (u16)(d4_sra(d4_lw(pos_vec + 8u), 23) + 2));
#endif
}
