/*
 * World-map scheduler callback 0x800922AC (slot-11 cb1).
 * Leaf function, no external calls.
 */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_922ac.h"

#define POOL_PTR  0x8009BE24u
#define D_8009BE0C 0x8009BE0Cu

static s16 ac_lh(u32 a) { s16 v; memcpy(&v, PSX_ADDR(a), 2); return v; }
static void ac_sh(u32 a, u16 v) { memcpy(PSX_ADDR(a), &v, 2); }
static s32 ac_lw(u32 a) { s32 v; memcpy(&v, PSX_ADDR(a), 4); return v; }
static void ac_sw(u32 a, s32 v) { memcpy(PSX_ADDR(a), &v, 4); }

s32 wm_800922AC(s32 slot_idx)
{
    u32 pool_ptr = (u32)ac_lw(POOL_PTR);
    u32 slot = pool_ptr + (u32)(slot_idx << 7);
    s32 return_val = 1;

    /* Pre-dispatch on slot[+0x04] */
    {
        s16 sub = ac_lh(slot + 0x04);
        if (sub == 9) {
            ac_sh(slot + 0x04, 0);
            ac_sh(slot + 0x20, 1);
        } else if (sub == 0x0A) {
            ac_sh(slot + 0x04, 0);
            ac_sh(slot + 0x20, 2);
        }
    }

    /* Main dispatch on slot[+0x20] */
    {
        s16 state = ac_lh(slot + 0x20);

        if (state == 0) {
            return_val = 3;
        } else if (state == 1) {
            /* Decrement scroll value */
            s32 val = ac_lw(slot + 0x50) - 0x1000;
            ac_sw(slot + 0x50, val);
            ac_sw(D_8009BE0C, val >> 12);
            if ((val >> 12) < 0x78) {
                ac_sw(D_8009BE0C, 0x78);
                ac_sw(slot + 0x50, 0x78000);
                ac_sh(slot + 0x20, 0);
            }
        } else if (state == 2) {
            /* Increment scroll value */
            s32 val = ac_lw(slot + 0x50) + 0x1000;
            ac_sw(slot + 0x50, val);
            ac_sw(D_8009BE0C, val >> 12);
            if ((val >> 12) >= 0x8C) {
                ac_sw(D_8009BE0C, 0x8C);
                ac_sw(slot + 0x50, 0x8C000);
                ac_sh(slot + 0x20, 0);
            }
        }
    }

    return return_val;
}
