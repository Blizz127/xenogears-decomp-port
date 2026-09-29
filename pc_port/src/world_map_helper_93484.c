/*
 * World-map helper 0x80093484 (position wrap/clamp).
 * Leaf function, no external calls.
 */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_93484.h"

#define D_8009D160 0x8009D160u  /* -0x2EA0: X wrap threshold */
#define D_8009D2B4 0x8009D2B4u  /* -0x2D4C: Z wrap threshold */

static s32 w84_lw(u32 a) { s32 v; memcpy(&v, PSX_ADDR(a), 4); return v; }
static void w84_sw(u32 a, s32 v) { memcpy(PSX_ADDR(a), &v, 4); }

void wm_80093484(u32 pos_vec)
{
    s32 x = w84_lw(pos_vec);
    s32 threshold_x = w84_lw(D_8009D160);
    s32 offset_x = threshold_x << 23;

    /* Wrap X */
    if (x < (s32)0xFC000000) {
        w84_sw(pos_vec, x + offset_x);
    } else if (x > (s32)0x04000000) {
        w84_sw(pos_vec, x - offset_x);
    }

    /* Wrap Z */
    {
        s32 z = w84_lw(pos_vec + 8);
        s32 threshold_z = w84_lw(D_8009D2B4);
        s32 offset_z = threshold_z << 23;

        if (z < (s32)0xFC000000) {
            w84_sw(pos_vec + 8, z + offset_z);
        } else if (z > (s32)0x04000000) {
            w84_sw(pos_vec + 8, z - offset_z);
        }
    }
}
