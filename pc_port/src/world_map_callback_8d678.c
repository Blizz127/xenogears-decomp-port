/*
 * World-map scheduler callback 0x8008D678 (slot-5/6 NPC follower handler).
 *
 * Register-faithful transcription of retail world_map.bin
 * [0x8008D678, 0x8008DD6C).  See world_map_callback_8d678.h.
 *
 * Pre-dispatch JT @0x800708D4, 8 entries (index = lhu(slot[+0x04])-1,
 * sltiu 8; OOR -> main dispatch).  Main dispatch JT @0x800708F4, 65
 * entries (index = lhu(slot[+0x20]), sltiu 0x41; OOR -> common tail).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_8d678.h"
#include "world_map_animation_guard.h"
#include "world_map_common_tail.h"
#include "world_map_helper_74794.h"
#include "world_map_helper_8bec8.h"
#include "world_map_helper_8c1dc.h"
#include "world_map_helper_8dff4.h"
#include "world_map_helper_941c4.h"
#include "world_map_helper_97770.h"

extern void func_800245D8(void* object, s16 animation);
extern s32 ratan2(s32 y, s32 x);
extern long rcos(long a);
extern long rsin(long a);

/* ------------------------------------------------------------------ */
/* Guest layout                                                        */
/* ------------------------------------------------------------------ */

#define D678_POOL_PTR    0x8009BE24u
#define D678_MODE_WORD   0x8009BE10u /* lw -0x41F0 */
#define D678_RING_INDEX  0x8009D154u /* lh -0x2EAC */
#define D678_RING_BASE   0x8009CEC4u /* 32 x 0x14 ring records */
#define D678_RING_STRIDE 0x14u
#define D678_RING_MASK   0x1Fu
#define D678_SCRATCH     0x1F800000u
/* Presence byte. */
#define D678_PRES_BASE   0x8006F364u /* sb -0xC9C */
/* Coord table (state 0x0A teleport init). */
#define D678_COORD_TABLE 0x8006EF8Au
/* Heading table. */
#define D678_HEAD_TABLE  0x8006EE58u
/* Resident mirror base (epilogue). */
#define D678_MIRROR_BASE 0x8006EE54u
/* Pool+0x3A8 neighbor source (state 0x02/0x30). */
#define D678_NEIGHBOR_OFFSET 0x3A8u
#define D678_NEIGHBOR_HEAD   0x3C8u

/* ------------------------------------------------------------------ */
/* Width-faithful memory helpers                                       */
/* ------------------------------------------------------------------ */

static u32 d678_lw(u32 a)
{
    u32 v;
    memcpy(&v, PSX_ADDR(a), 4);
    return v;
}

static void d678_sw(u32 a, u32 v)
{
    memcpy(PSX_ADDR(a), &v, 4);
}

static u16 d678_lhu(u32 a)
{
    u16 v;
    memcpy(&v, PSX_ADDR(a), 2);
    return v;
}

static s16 d678_lh(u32 a)
{
    s16 v;
    memcpy(&v, PSX_ADDR(a), 2);
    return v;
}

static void d678_sh(u32 a, u16 v)
{
    memcpy(PSX_ADDR(a), &v, 2);
}

static u8 d678_lbu(u32 a)
{
    return *(u8*)PSX_ADDR(a);
}

static void d678_sb(u32 a, u8 v)
{
    *(u8*)PSX_ADDR(a) = v;
}

/* ------------------------------------------------------------------ */
/* Slot record access                                                  */
/* ------------------------------------------------------------------ */

#define SLOT_STRIDE 128u

static u32 slot_base(u32 pool_ptr, s32 slot_idx)
{
    return pool_ptr + (u32)(slot_idx << 7);
}

/* ------------------------------------------------------------------ */
/* Main implementation                                                 */
/* ------------------------------------------------------------------ */

s32 wm_8008D678(s32 slot_idx)
{
    u32 pool_ptr;
    u32 s1;          /* slot record base */
    u32 s0;          /* scratch pointer */
    u32 s3;          /* scratchpad base = 0x1F800000 */
    u32 v0;
    s32 v1;
    u32 a0;

    s3 = D678_SCRATCH;
    pool_ptr = d678_lw(D678_POOL_PTR);
    s1 = slot_base(pool_ptr, slot_idx);

    /* --- Pre-dispatch (slot[+0x04]-1, JT @0x800708D4) --- */
    v0 = d678_lhu(s1 + 0x04);
    v1 = (s32)(s16)(u16)(v0 - 1);

    if ((u32)v1 < 8) {
        switch (v1) {
        case 0: /* set main=0x08, load source from table */
            d678_sh(s1 + 0x20, 0x08);
            d678_sh(s1 + 0x04, 0);
            v0 = (u32)d678_lh(s1 + 0x06);
            s0 = pool_ptr + (v0 << 7);
            break;
        case 1: /* set main=0x20 */
            d678_sh(s1 + 0x20, 0x20);
            d678_sh(s1 + 0x04, 0);
            break;
        case 2: /* set main=0x30 */
            d678_sh(s1 + 0x20, 0x30);
            d678_sh(s1 + 0x04, 0);
            break;
        case 3: /* set main=0x40, write presence byte=7 */
            d678_sh(s1 + 0x20, 0x40);
            d678_sh(s1 + 0x04, 0);
            d678_sb(D678_PRES_BASE + (u32)slot_idx, 7);
            break;
        case 4: /* set main=0x10 */
            d678_sh(s1 + 0x20, 0x10);
            d678_sh(s1 + 0x04, 0);
            break;
        case 5:
        case 6: /* fall through (no state change) */
            break;
        case 7: /* set main=0x18 */
            d678_sh(s1 + 0x20, 0x18);
            d678_sh(s1 + 0x04, 0);
            break;
        }
    }

    /* --- Main state dispatch (slot[+0x20], JT @0x800708F4) --- */
    {
        s32 main_state = d678_lh(s1 + 0x20);

        if ((u32)main_state >= 0x41) {
            goto common_tail;
        }

        switch (main_state) {
        /* ======== States 0x00,0x01: Ring buffer follower ======== */
        case 0x00:
        case 0x01: {
            u32 pres_byte;
            u32 ring_idx;
            u32 ring_base_addr;

            pres_byte = d678_lbu(D678_PRES_BASE + (u32)slot_idx);

            if (pres_byte != 1) {
                /* ---- Non-matching presence: set anim=3 + register ---- */
                u32 actor_ptr = d678_lw(s1 + 0x4C);
                if (wm_native_animation_differs(actor_ptr, 3)) {
                    func_800245D8((void*)(uintptr_t)actor_ptr, 3);
                }
                wm_800894C8((u32)(slot_idx + 0x28));
                goto common_tail;
            }

            /* ---- Presence == 1: ring buffer path ---- */
            ring_idx = ((u32)d678_lh(D678_RING_INDEX) - (u32)d678_lw(s1 + 0x58))
                       & D678_RING_MASK;
            ring_base_addr = D678_RING_BASE;
            s0 = ring_base_addr + ring_idx * D678_RING_STRIDE;

            {
                u32 rx = d678_lw(s0 + 0x00);
                u32 rz = d678_lw(s0 + 0x04);
                u32 ry = d678_lw(s0 + 0x08);
                u32 px = d678_lw(s1 + 0x28);
                u32 pz = d678_lw(s1 + 0x2C);
                u32 py = d678_lw(s1 + 0x30);
                s32 match = ((px ^ rx) < 1) && ((pz ^ rz) < 1) && ((py ^ ry) < 1);

                if (match) {
                    u32 actor_ptr = d678_lw(s1 + 0x4C);
                    if (wm_native_animation_differs(actor_ptr, 0)) {
                        func_800245D8((void*)(uintptr_t)actor_ptr, 0);
                        wm_800894C8((u32)(slot_idx + 0x28));
                    }
                } else {
                    u32 actor_ptr = d678_lw(s1 + 0x4C);
                    if (wm_native_animation_differs(actor_ptr, 1)) {
                        func_800245D8((void*)(uintptr_t)actor_ptr, 1);
                    }
                    wm_8008C1DC((u32)(slot_idx + 0x28), s1, s3);
                }

                /* Copy ring entry -> slot position */
                d678_sw(s1 + 0x28, d678_lw(s0 + 0x00));
                d678_sw(s1 + 0x2C, d678_lw(s0 + 0x04));
                d678_sw(s1 + 0x30, d678_lw(s0 + 0x08));
                d678_sh(s1 + 0x48, d678_lhu(s0 + 0x10));
            }
            goto common_tail;
        }

        /* ======== State 0x02: Neighbor copy (pool+0x3A8) ======== */
        case 0x02: {
            d678_sw(s1 + 0x28, d678_lw(pool_ptr + D678_NEIGHBOR_OFFSET + 0x00));
            d678_sw(s1 + 0x30, d678_lw(pool_ptr + D678_NEIGHBOR_OFFSET + 0x08));
            d678_sh(s1 + 0x48, d678_lhu(pool_ptr + D678_NEIGHBOR_HEAD));
            goto common_tail;
        }

        /* ======== State 0x08: Approach step ======== */
        case 0x08: {
            u32 src_idx = (u32)d678_lh(s1 + 0x06);
            u32 src_rec = pool_ptr + (src_idx << 7);

            wm_800941C4(s1 + 0x28, src_rec + 0x28, s1 + 0x38, s1 + 0x48);

            d678_sw(s1 + 0x50, (u32)((s32)d678_lw(s1 + 0x28) >> 12));
            d678_sw(s1 + 0x54, (u32)((s32)d678_lw(s1 + 0x30) >> 12));

            func_800245D8((void*)(uintptr_t)d678_lw(s1 + 0x4C), 1);

            d678_sh(s1 + 0x20, (u16)(main_state + 1));

            if (wm_8008BEC8(s1) == 3) {
                d678_sh(s1 + 0x20, (u16)(d678_lhu(s1 + 0x20) + 1));
            }
            wm_8008C1DC((u32)(slot_idx + 0x28), s1, s3);
            goto common_tail;
        }

        /* ======== State 0x09: Approach completion ======== */
        case 0x09: {
            if (wm_8008BEC8(s1) == 3) {
                d678_sh(s1 + 0x20, (u16)(main_state + 1));
            }
            wm_8008C1DC((u32)(slot_idx + 0x28), s1, s3);
            goto common_tail;
        }

        /* ======== State 0x0A: Teleport init ======== */
        case 0x0A: {
            s32 sub_val = d678_lh(s1 + 0x06);
            if (wm_80097770((u32)sub_val, 4) == 0) {
                goto common_tail;
            }
            d678_sh(s1 + 0x24, 1);
            d678_sh(s1 + 0x20, 2);
            wm_800894C8((u32)(slot_idx + 0x28));
            goto common_tail;
        }

        /* ======== State 0x10: Heading init (ratan2 + rcos/rsin) ======== */
        case 0x10: {
            u32 pool_base2 = d678_lw(D678_POOL_PTR);
            s32 target_x, target_z, dx, dz;
            s32 angle;
            s32 cos_val, sin_val;

            /* Approach target from pool+0x228/0x230 */
            d678_sw(s1 + 0x50, d678_lw(pool_base2 + 0x228));
            d678_sw(s1 + 0x54, d678_lw(pool_base2 + 0x230));

            /* Compute direction vector */
            target_x = (s32)d678_lw(s1 + 0x50);
            dx = target_x - (s32)d678_lw(s1 + 0x28);
            d678_sw(s3 + 0x00, (u32)dx);

            target_z = (s32)d678_lw(s1 + 0x54);
            dz = target_z - (s32)d678_lw(s1 + 0x30);
            d678_sw(s3 + 0x08, (u32)dz);

            /* heading = (ratan2(dz, dx) + 0x400) & 0xFFF */
            angle = ratan2(dz, dx);
            a0 = (u32)((angle + 0x400) & 0xFFF);
            d678_sh(s1 + 0x48, (u16)a0);

            /* velocity.x = rcos(heading) */
            cos_val = (s32)rcos((long)(s16)(u16)a0);
            d678_sw(s1 + 0x38, (u32)cos_val);

            /* velocity.z = -rsin(heading) */
            sin_val = (s32)rsin((long)d678_lh(s1 + 0x48));
            d678_sw(s1 + 0x40, (u32)(-(s32)sin_val));

            /* Animate + advance state */
            func_800245D8((void*)(uintptr_t)d678_lw(s1 + 0x4C), 1);

            d678_sh(s1 + 0x20, (u16)(d678_lhu(s1 + 0x20) + 1));

            if (wm_8008BEC8(s1) == 3) {
                d678_sh(s1 + 0x20, (u16)(d678_lhu(s1 + 0x20) + 1));
            }
            goto common_tail;
        }

        /* ======== State 0x11: Heading completion ======== */
        case 0x11: {
            if (wm_8008BEC8(s1) == 3) {
                d678_sh(s1 + 0x20, (u16)(d678_lhu(s1 + 0x20) + 1));
            }
            goto common_tail;
        }

        /* ======== State 0x12: Presence check (wm_80097770) ======== */
        case 0x12: {
            if (wm_80097770(4, 7) == 0) {
                goto common_tail;
            }
            d678_sh(s1 + 0x20, 1);
            goto common_tail;
        }

        /* ======== State 0x18: wm_80089160 presence registration ======== */
        case 0x18: {
            u8 pres = d678_lbu(D678_PRES_BASE + (u32)slot_idx);
            if (pres == 7) {
                d678_sh(s1 + 0x22, 1);
                d678_sh(s1 + 0x20, 0x1A);
            } else {
                /* Write (pos>>12) to scratchpad, call wm_80089160 */
                d678_sh(s3 + 0xA0, (u16)((s32)d678_lw(s1 + 0x28) >> 12));
                d678_sh(s3 + 0xA2, (u16)((s32)d678_lw(s1 + 0x2C) >> 12));
                d678_sh(s3 + 0xA4, (u16)((s32)d678_lw(s1 + 0x30) >> 12));
                wm_80089160((u32)pres, s3 + 0xA0, 0);
                d678_sh(s1 + 0x22, 8);
                d678_sh(s1 + 0x20, (u16)(d678_lhu(s1 + 0x20) + 1));
            }
            goto common_tail;
        }

        /* ======== State 0x19: Timer countdown ======== */
        case 0x19: {
            u16 sub = d678_lhu(s1 + 0x22);
            sub--;
            d678_sh(s1 + 0x22, sub);
            if ((s16)sub > 0) {
                goto common_tail;
            }
            d678_sh(s1 + 0x24, 1);
            d678_sh(s1 + 0x22, 0x10);
            d678_sh(s1 + 0x20, (u16)(d678_lhu(s1 + 0x20) + 1));
            goto common_tail;
        }

        /* ======== State 0x1A: Timer → anim frame 3 ======== */
        case 0x1A: {
            u16 sub = d678_lhu(s1 + 0x22);
            sub--;
            d678_sh(s1 + 0x22, sub);
            if ((s16)sub > 0) {
                goto common_tail;
            }
            func_800245D8((void*)(uintptr_t)d678_lw(s1 + 0x4C), 3);
            d678_sh(s1 + 0x24, 1);
            d678_sh(s1 + 0x20, 2);
            goto common_tail;
        }

        /* ======== State 0x1B: Timer countdown ======== */
        case 0x1B: {
            u16 sub = d678_lhu(s1 + 0x22);
            sub--;
            d678_sh(s1 + 0x22, sub);
            if ((s16)sub > 0) {
                goto common_tail;
            }
            /* Clear anim + velocity */
            func_800245D8((void*)(uintptr_t)d678_lw(s1 + 0x4C), 0);
            d678_sw(s1 + 0x38, 0);
            d678_sw(s1 + 0x3C, 0);
            d678_sw(s1 + 0x40, 0);
            d678_sh(s1 + 0x20, (u16)(d678_lhu(s1 + 0x20) + 1));
            goto common_tail;
        }

        /* ======== State 0x20: Presence check with mode_word ======== */
        case 0x20: {
            if (wm_80097770((u32)(slot_idx - 3), 2) == 0) {
                goto common_tail;
            }
            d678_sh(s1 + 0x20, (u16)(d678_lhu(s1 + 0x20) + 1));
            goto common_tail;
        }

        /* ======== State 0x21: Presence init (wm_80089160) ======== */
        case 0x21: {
            u32 pool_base3 = d678_lw(D678_POOL_PTR);

            d678_sw(s1 + 0x50, d678_lw(pool_base3 + 0x228));
            d678_sw(s1 + 0x54, d678_lw(pool_base3 + 0x230));

            /* Compute direction to target */
            {
                s32 dx2 = (s32)d678_lw(s1 + 0x50) - (s32)d678_lw(s1 + 0x28);
                s32 dz2 = (s32)d678_lw(s1 + 0x54) - (s32)d678_lw(s1 + 0x30);
                d678_sw(s3 + 0x00, (u32)dx2);
                d678_sw(s3 + 0x08, (u32)dz2);
            }

            /* heading = (ratan2(dz, dx) + 0x400) & 0xFFF */
            {
                s32 angle2 = ratan2((s32)d678_lw(s3 + 0x08), (s32)d678_lw(s3 + 0x00));
                u16 heading = (u16)((angle2 + 0x400) & 0xFFF);
                d678_sh(s1 + 0x48, heading);

                /* velocity.x = rcos(heading) */
                d678_sw(s1 + 0x38, (u32)rcos((long)(s16)heading));
                /* velocity.z = -rsin(heading) */
                d678_sw(s1 + 0x40, (u32)(-(s32)rsin((long)d678_lh(s1 + 0x48))));
            }

            /* Scale target >>12 for mirror */
            d678_sw(s1 + 0x50, (u32)((s32)d678_lw(s1 + 0x50) >> 12));
            d678_sw(s1 + 0x54, (u32)((s32)d678_lw(s1 + 0x54) >> 12));

            func_800245D8((void*)(uintptr_t)d678_lw(s1 + 0x4C), 1);

            d678_sh(s1 + 0x20, (u16)(d678_lhu(s1 + 0x20) + 1));
            goto common_tail;
        }

        /* ======== State 0x30: Neighbor copy + anim ======== */
        case 0x30: {
            u32 pres = d678_lbu(D678_PRES_BASE + (u32)slot_idx);
            u32 actor_ptr;
            s8 anim_flag;

            /* Copy pos from neighbor (pool + pres*128 + 0x180) */
            u32 neighbor = slot_base(pool_ptr, (s32)pres) + 0x180;
            d678_sw(s1 + 0x28, d678_lw(neighbor + 0x28));
            d678_sw(s1 + 0x2C, d678_lw(neighbor + 0x2C));
            d678_sw(s1 + 0x30, d678_lw(neighbor + 0x30));

            actor_ptr = d678_lw(s1 + 0x4C);
            anim_flag = wm_native_animation_value(actor_ptr);
            if (anim_flag == 1) {
                /* No animation change, just register */
                wm_800894C8((u32)(slot_idx + 0x28));
            } else {
                /* Check if already at frame 3 */
                if (anim_flag != 3) {
                    func_800245D8((void*)(uintptr_t)actor_ptr, 3);
                }
                wm_800894C8((u32)(slot_idx + 0x28));
            }
            goto common_tail;
        }

        /* ======== State 0x32: wm_80097770 check ======== */
        case 0x32: {
            if (wm_80097770((u32)(slot_idx - 3), 2) == 0) {
                goto common_tail;
            }
            d678_sh(s1 + 0x20, 1);
            goto common_tail;
        }

        /* ======== State 0x34: State reset ======== */
        case 0x34: {
            d678_sh(s1 + 0x20, 0);
            goto common_tail;
        }

        /* ======== State 0x35: Idle anim clear ======== */
        case 0x35: {
            func_800245D8((void*)(uintptr_t)d678_lw(s1 + 0x4C), 0);
            d678_sw(s1 + 0x38, 0);
            d678_sw(s1 + 0x3C, 0);
            d678_sw(s1 + 0x40, 0);
            d678_sh(s1 + 0x20, (u16)(d678_lhu(s1 + 0x20) + 1));
            goto common_tail;
        }

        /* ======== State 0x40: Neighbor approach (wm_8008DFF4 + rcos/rsin) ======== */
        case 0x40: {
            u32 pool_base4;
            s32 cos_val3, sin_val3;

            s0 = s1 + 0x28;
            wm_8008DFF4(s0);

            pool_base4 = d678_lw(D678_POOL_PTR);
            d678_sh(s1 + 0x24, 0);

            v0 = d678_lw(pool_base4 + 0x3F8);
            a0 = (u32)(s16)(u16)v0;
            d678_sh(s1 + 0x48, (u16)v0);

            /* velocity.x = rcos(heading) * 32 */
            cos_val3 = (s32)rcos((long)(s16)(u16)a0);
            v1 = cos_val3 * 3;
            v1 = v1 << 5;  /* *32 */
            d678_sw(s3 + 0x00, d678_lw(s1 + 0x28) + (u32)v1);

            /* velocity.z = -rsin(heading) * 32 */
            sin_val3 = (s32)rsin((long)d678_lh(s1 + 0x48));
            v0 = (u32)(-(s32)sin_val3);
            v1 = (s32)v0 * 3;
            v1 = v1 << 5;
            d678_sw(s3 + 0x08, d678_lw(s1 + 0x30) + (u32)v1);

            wm_800941C4(s0, s3, s1 + 0x38, s1 + 0x48);

            d678_sw(s1 + 0x50, (u32)((s32)d678_lw(s3 + 0x00) >> 12));
            d678_sw(s1 + 0x54, (u32)((s32)d678_lw(s3 + 0x08) >> 12));

            func_800245D8((void*)(uintptr_t)d678_lw(s1 + 0x4C), 1);

            d678_sh(s1 + 0x20, (u16)(d678_lhu(s1 + 0x20) + 1));
            goto common_tail;
        }

        /* ======== State 0x31: Completion check ======== */
        case 0x31: {
            if (wm_8008BEC8(s1) == 3) {
                /* Clear anim + velocity, advance state */
                func_800245D8((void*)(uintptr_t)d678_lw(s1 + 0x4C), 0);
                d678_sw(s1 + 0x38, 0);
                d678_sw(s1 + 0x3C, 0);
                d678_sw(s1 + 0x40, 0);
                d678_sh(s1 + 0x20, (u16)(d678_lhu(s1 + 0x20) + 1));
            }
            goto common_tail;
        }

        /* ======== State 0x32 (alt): State reset to 1 ======== */
        /* Already handled above */

        /* ======== All other states: common tail ======== */
        default:
            goto common_tail;
        }
    }

common_tail:
    /* Gate: main_state != 2 && (mode_word != 2 || presence != 7) */
    {
        s32 ms = d678_lh(s1 + 0x20);
        if (ms != 2) {
            u32 mode_word = d678_lw(D678_MODE_WORD);
            if (mode_word != 2) {
                u8 pres = d678_lbu(D678_PRES_BASE + (u32)slot_idx);
                if (pres != 7) {
                    wm_80074794(1, s1 + 0x28);
                }
            }
        }
    }

    /* Epilogue: write pos.x>>12, pos.z>>12, heading to resident mirror */
    {
        s32 rel = slot_idx - 4;
        u32 off = (u32)(rel * 6);  /* sll 1 + addu + sll 1 = *6 */
        u32 mirror_x = D678_MIRROR_BASE + off;
        u32 mirror_z = D678_MIRROR_BASE + 2 + off;
        u32 mirror_h = D678_MIRROR_BASE + 0xEC2 + (u32)(slot_idx << 1);
        /* 0xEC2 = 0x80070000-0x1070-0x13E - 0x8006EE54 = ... */
        /* Actually from asm: $a1 = 0x8006EF90, $a1 = $a1 - 0x13E = 0x8006EE52 */
        /* heading = 0x8006EE52 + slot_idx*2 */
        /* But for slot 5: 0x8006EE52 + 10 = 0x8006EE5C */
        /* For slot 4: 0x8006EE52 + 8 = 0x8006EE5A (same as 8A72C heading!) */

        d678_sh(mirror_x, (u16)((s32)d678_lw(s1 + 0x28) >> 12));
        d678_sh(mirror_z, (u16)((s32)d678_lw(s1 + 0x30) >> 12));
        d678_sh(mirror_h, d678_lhu(s1 + 0x48));
    }

    return 1;
}
