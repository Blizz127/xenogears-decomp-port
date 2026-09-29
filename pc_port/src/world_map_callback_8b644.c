/*
 * World-map scheduler callback 0x8008B644 (slot-2/3 NPC follower handler).
 *
 * Register-faithful transcription of retail world_map.bin
 * [0x8008B644, 0x8008BB40).  See world_map_callback_8b644.h.
 *
 * JT @0x80070670, 65 entries (index = sh slot[+0x20], sltiu 0x41 guard,
 * out-of-range -> common tail).  Full retail slot->target map:
 *   0x00,0x01 -> 0x8008B718  breadcrumb path follower
 *   0x02      -> 0x8008B880  copy from neighbor slot+0x180
 *   0x08      -> 0x8008B8BC  approach step (941C4)
 *   0x09      -> 0x8008B904  approach completion (8BEC8)
 *   0x0A      -> 0x8008B93C  teleport init (coord table + 97770)
 *   0x28      -> 0x8008B96C  teleport step (rcos/rsin + 941C4)
 *   0x29      -> 0x8008BABC  teleport completion (8BEC8)
 *   0x2A      -> 0x8008BA48  state reset to 0x28
 *   0x30      -> 0x8008BA60  presence clear + state reset
 *   0x32      -> 0x8008BA60  copy from neighbor pool+0xA8
 *   parked -> 0x8008BAFC (common tail): 0x03-0x07, 0x0B-0x27,
 *   0x2B-0x2F, 0x31, 0x33-0x40 (59 of 65 entries).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_8b644.h"
#include "world_map_animation_guard.h"
#include "world_map_common_tail.h"
#include "world_map_helper_74794.h"
#include "world_map_helper_8bec8.h"
#include "world_map_helper_8c1dc.h"
#include "world_map_helper_941c4.h"
#include "world_map_helper_97770.h"
#include "world_map_terrain_sampler.h"

extern void func_800245D8(void* object, s16 animation);
extern long rcos(long a);
extern long rsin(long a);

/* ------------------------------------------------------------------ */
/* Guest layout                                                        */
/* ------------------------------------------------------------------ */

#define B644_POOL_PTR    0x8009BE24u /* lw -0x41DC(0x800A) */
#define B644_RING_INDEX  0x8009D154u /* lh -0x2EAC */
#define B644_RING_BASE   0x8009CEC4u /* -0x313C: 32 x 0x14 ring records */
#define B644_RING_STRIDE 0x14u       /* 20 bytes per ring entry */
#define B644_RING_MASK   0x1Fu       /* 32 entries, modular */
#define B644_SCRATCH     0x1F800000u

/* Resident bytes (below overlay base). */
#define B644_SLOT2_PRES  0x8006F369u /* sb -0xC97 */
#define B644_SLOT3_PRES  0x8006F36Au /* sb -0xC96 */
#define B644_WARP_HEAD   0x8006EE5Au /* lhu -0x11A6 */
#define B644_MIRROR_X    0x8006EE54u /* sh -0x11AC */
#define B644_MIRROR_Z    0x8006EE56u /* sh -0x11AA */
#define B644_MIRROR_HEAD 0x8006EE58u /* sh -0x11A8 */

/* Coord table for state 0x0A teleport init.
 * Indexed by substate (lh slot[+0x06]), stride 6 bytes.
 * Each entry: u16 x_tile, u16 z_tile, u16 heading. */
#define B644_COORD_TABLE 0x8006EF8Au /* -0x1076(0x8007) */
/* Heading table for state 0x0A, indexed by substate, stride 2. */
#define B644_HEAD_TABLE  0x8006EE58u /* -0x11A8(0x8007) */

/* ------------------------------------------------------------------ */
/* Width-faithful memory helpers                                       */
/* ------------------------------------------------------------------ */

static u32 b644_lw(u32 a)
{
    u32 v;
    memcpy(&v, PSX_ADDR(a), 4);
    return v;
}

static void b644_sw(u32 a, u32 v)
{
    memcpy(PSX_ADDR(a), &v, 4);
}

static u16 b644_lhu(u32 a)
{
    u16 v;
    memcpy(&v, PSX_ADDR(a), 2);
    return v;
}

static s16 b644_lh(u32 a)
{
    s16 v;
    memcpy(&v, PSX_ADDR(a), 2);
    return v;
}

static void b644_sh(u32 a, u16 v)
{
    memcpy(PSX_ADDR(a), &v, 2);
}

static u8 b644_lbu(u32 a)
{
    return *(u8*)PSX_ADDR(a);
}

static void b644_sb(u32 a, u8 v)
{
    *(u8*)PSX_ADDR(a) = v;
}

/* ------------------------------------------------------------------ */
/* Slot record access                                                  */
/* ------------------------------------------------------------------ */

/* Slot record is 128 bytes (sll 7). */
#define SLOT_STRIDE 128u

static u32 slot_base(u32 pool_ptr, s32 slot_idx)
{
    return pool_ptr + (u32)(slot_idx << 7);
}

/* ------------------------------------------------------------------ */
/* Main implementation                                                 */
/* ------------------------------------------------------------------ */

s32 wm_8008B644(s32 slot_idx)
{
    u32 pool_ptr;
    u32 s1;          /* slot record base */
    u32 s0;          /* scratch pointer (ring entry, neighbor, etc.) */
    u32 s3;          /* scratchpad base = 0x1F800000 */
    s32 substate;    /* slot[+0x04] */
    s32 main_state;  /* slot[+0x20] */
    s32 v1;
    u32 v0;
    u32 a1;

    s3 = B644_SCRATCH;
    pool_ptr = b644_lw(B644_POOL_PTR);
    s1 = slot_base(pool_ptr, slot_idx);

    /* --- Substate dispatch (slot[+0x04]) --- */
    substate = b644_lh(s1 + 0x04);

    if (substate == 2) {
        /* substate 2: set main=0x28, sub=0 */
        b644_sh(s1 + 0x04, 0);
        b644_sh(s1 + 0x20, 0x28);
    } else if (substate < 3) {
        if (substate == 1) {
            /* substate 1: clear sub, set main=8, load record from table */
            b644_sh(s1 + 0x04, 0);
            b644_sh(s1 + 0x20, 8);
            v0 = (u32)b644_lh(s1 + 0x06);
            s0 = slot_base(pool_ptr, (s32)(v0 << 0)); /* sll 7 implicit */
            /* Actually: v0 = lh(s1+0x06); s0 = pool_ptr + (v0 << 7) */
            s0 = pool_ptr + (v0 << 7);
        } else {
            /* substate 0: fall through to main dispatch */
            goto main_dispatch;
        }
    } else if (substate == 3) {
        /* substate 3: set main=1, sub=0 */
        b644_sh(s1 + 0x04, 0);
        b644_sh(s1 + 0x20, 1);
    } else if (substate == 5) {
        /* substate 5: set main=0x30, sub=0 */
        b644_sh(s1 + 0x04, 0);
        b644_sh(s1 + 0x20, 0x30);
    } else {
        /* other substates: fall through to main dispatch */
        goto main_dispatch;
    }

main_dispatch:
    /* --- Main state dispatch (slot[+0x20]) --- */
    main_state = b644_lh(s1 + 0x20);
    if ((u32)main_state >= 0x41) {
        goto common_tail;
    }

    switch (main_state) {
    /* ======== States 0x00,0x01: Breadcrumb path follower ======== */
    case 0x00:
    case 0x01: {
        u32 pres_byte;
        u32 ring_idx;
        u32 ring_base;

        /* Check presence byte: 0x8006F8E4 + slot_idx */
        pres_byte = b644_lbu(0x8006F8E4u + (u32)slot_idx);
        if (pres_byte != 0) {
            /* ---- Neighbor copy path (substate=0, presence != 0) ---- */
            u32 neighbor;

            /* Neighbor record = pool_ptr + (pres_byte << 7) + 0x180 */
            neighbor = slot_base(pool_ptr, (s32)pres_byte) + 0x180;

            /* Copy pos.x, pos.y, pos.z from neighbor */
            b644_sw(s1 + 0x28, b644_lw(neighbor + 0x28));
            b644_sw(s1 + 0x2C, b644_lw(neighbor + 0x2C));
            b644_sw(s1 + 0x30, b644_lw(neighbor + 0x30));

            /* Copy heading from neighbor */
            v0 = b644_lhu(neighbor + 0x48);
            b644_sh(s1 + 0x24, 1);
            b644_sh(s1 + 0x48, (u16)v0);

            /* Register: wm_800894C8(slot_idx + 0x2E) */
            wm_800894C8((u32)(slot_idx + 0x2E));
            goto common_tail;
        }

        /* ---- Ring buffer path (substate=0, presence == 0) ---- */
        /* ring_idx = (D_8009D154 - slot[+0x58]) & 0x1F */
        ring_idx = ((u32)b644_lh(B644_RING_INDEX) - (u32)b644_lw(s1 + 0x58))
                   & B644_RING_MASK;

        /* ring_entry = ring_base + ring_idx * 20 */
        ring_base = B644_RING_BASE;
        s0 = ring_base + ring_idx * B644_RING_STRIDE;

        /* Compare pos.x, pos.z, pos.y against ring entry */
        {
            u32 rx = b644_lw(s0 + 0x00);
            u32 rz = b644_lw(s0 + 0x04);
            u32 ry = b644_lw(s0 + 0x08);
            u32 px = b644_lw(s1 + 0x28);
            u32 pz = b644_lw(s1 + 0x2C);
            u32 py = b644_lw(s1 + 0x30);

            s32 match = ((px ^ rx) < 1) && ((pz ^ rz) < 1) && ((py ^ ry) < 1);

            if (match) {
                /* Position matches: check anim flag */
                u32 actor_ptr = b644_lw(s1 + 0x4C);
                if (wm_native_animation_differs(actor_ptr, 0)) {
                    /* Animate + register */
                    func_800245D8((void*)(uintptr_t)actor_ptr, 0);
                    wm_800894C8((u32)(slot_idx + 0x2E));
                }
            } else {
                /* Position differs: set anim=1 if needed, then path-step */
                u32 actor_ptr = b644_lw(s1 + 0x4C);
                if (wm_native_animation_differs(actor_ptr, 1)) {
                    func_800245D8((void*)(uintptr_t)actor_ptr, 1);
                }

                /* Path step: wm_8008C1DC(slot_idx+0x2E, s1, scratchpad) */
                wm_8008C1DC((u32)(slot_idx + 0x2E), s1, s3);
            }

            /* Copy ring entry -> slot position */
            b644_sw(s1 + 0x28, b644_lw(s0 + 0x00));
            b644_sw(s1 + 0x2C, b644_lw(s0 + 0x04));
            b644_sw(s1 + 0x30, b644_lw(s0 + 0x08));
            b644_sh(s1 + 0x24, 0);
            b644_sh(s1 + 0x48, b644_lhu(s0 + 0x10));
        }
        goto common_tail;
    }

    /* ======== State 0x02: Copy from neighbor slot+0x180 ======== */
    case 0x02: {
        u32 neighbor;

        /* Neighbor = pool_ptr + slot_idx*128 + 0x3A8 (slot 4 relative?) */
        /* Actually from asm: lw 0x3A8($v1) where $v1 = pool_ptr */
        /* This reads from the POOL BASE + 0x3A8, not slot-relative */
        neighbor = pool_ptr + 0x3A8;

        b644_sw(s1 + 0x28, b644_lw(neighbor + 0x00));
        b644_sw(s1 + 0x2C, b644_lw(neighbor + 0x04));
        b644_sw(s1 + 0x30, b644_lw(neighbor + 0x08));
        b644_sh(s1 + 0x48, b644_lhu(neighbor + 0x28));
        goto common_tail;
    }

    /* ======== State 0x08: Approach step ======== */
    case 0x08: {

        /* wm_800941C4(pos, neighbor_pos+0x28, vel, heading) */
        /* $a0 = s1+0x28, $a1 = s0+0x28, $a2 = s1+0x38, $a3 = s1+0x48 */
        /* s0 is loaded from substate 1 path, but here it's the neighbor */
        /* From asm: s0 was set in the substate=1 path (slot record from table) */
        /* Need to re-derive: substate 1 sets s0 = pool + (lh(s1+0x06) << 7) */
        {
            u32 src_idx = (u32)b644_lh(s1 + 0x06);
            u32 src_rec = pool_ptr + (src_idx << 7);

            wm_800941C4(s1 + 0x28, src_rec + 0x28, s1 + 0x38, s1 + 0x48);
        }

        /* Store approach target (>>12) */
        b644_sw(s1 + 0x50, (u32)((s32)b644_lw(s1 + 0x28) >> 12));
        b644_sw(s1 + 0x54, (u32)((s32)b644_lw(s1 + 0x30) >> 12));

        /* Animate: func_800245D8(slot[+0x4C], 1) */
        func_800245D8((void*)(uintptr_t)b644_lw(s1 + 0x4C), 1);

        /* Advance state */
        b644_sh(s1 + 0x20, (u16)(main_state + 1));

        /* Check completion: wm_8008BEC8(s1) */
        if (wm_8008BEC8(s1) == 3) {
            b644_sh(s1 + 0x20, (u16)(b644_lhu(s1 + 0x20) + 1));
        }

        /* Register: wm_8008C1DC(slot_idx+0x2E, s1, scratchpad) */
        wm_8008C1DC((u32)(slot_idx + 0x2E), s1, s3);
        goto common_tail;
    }

    /* ======== State 0x09: Approach completion check ======== */
    case 0x09: {
        /* wm_8008BEC8(s1) */
        if (wm_8008BEC8(s1) == 3) {
            b644_sh(s1 + 0x20, (u16)(main_state + 1));
        }
        /* Register: wm_8008C1DC(slot_idx+0x2E, s1, scratchpad) */
        wm_8008C1DC((u32)(slot_idx + 0x2E), s1, s3);
        goto common_tail;
    }

    /* ======== State 0x0A: Teleport init ======== */
    case 0x0A: {
        s32 sub_val = b644_lh(s1 + 0x06);

        /* wm_80097770(sub_val, 4) — returns nonzero if ready */
        if (wm_80097770((u32)sub_val, 4) == 0) {
            goto common_tail;
        }

        /* Set flag and advance to state 2 */
        b644_sh(s1 + 0x24, 1);
        b644_sh(s1 + 0x20, 2);

        /* Register: wm_800894C8(slot_idx + 0x2E) */
        wm_800894C8((u32)(slot_idx + 0x2E));
        goto common_tail;
    }

    /* ======== State 0x28: Teleport step ======== */
    case 0x28: {
        u32 coord_off;
        u32 coord_entry;
        s32 terrain_y;
        s16 heading;
        s32 cos_val, sin_val;

        /* coord_entry = B644_COORD_TABLE + (slot_idx*3*2) */
        /* asm: sll s0, s2, 1; addu v1, s0, s2; sll v1, 1 → v1 = slot_idx * 6 */
        coord_off = (u32)(slot_idx * 6);
        coord_entry = B644_COORD_TABLE + coord_off;

        /* pos.x = coord.x << 12 */
        v0 = b644_lhu(coord_entry + 0);
        b644_sw(s1 + 0x28, v0 << 12);

        /* pos.z = coord.z << 12 */
        v0 = b644_lhu(coord_entry + 2);
        a1 = v0 << 12;
        b644_sw(s1 + 0x30, a1);

        /* terrain_y = wm_80093978(pos.x, pos.z) */
        terrain_y = wm_80093978((s32)b644_lw(s1 + 0x28), (s32)a1);
        b644_sw(s1 + 0x2C, (u32)terrain_y);

        /* heading from B644_HEAD_TABLE[slot_idx*2] */
        v0 = b644_lhu(B644_HEAD_TABLE + (u32)(slot_idx << 1));
        heading = (s16)(u16)v0;
        b644_sh(s1 + 0x48, (u16)v0);
        b644_sw(s1 + 0x5C, v0);

        /* Compute forward offset using rcos/rsin */
        /* rcos returns sin(angle) (retail naming swap) */
        cos_val = (s32)rcos((long)heading);
        v1 = cos_val * 3;
        v1 = v1 << 4;  /* v1 = cos_val * 48 */
        b644_sw(s3 + 0x00, b644_lw(s1 + 0x28) + (u32)v1);

        /* rsin returns cos(angle) (retail naming swap) */
        sin_val = (s32)rsin((long)heading);
        v0 = (u32)(-(s32)sin_val);
        v1 = (s32)v0 * 3;
        v1 = v1 << 4;  /* v1 = -sin_val * 48 */
        b644_sw(s3 + 0x08, b644_lw(s1 + 0x30) + (u32)v1);

        /* wm_800941C4(pos, scratchpad+pos, vel, heading) */
        wm_800941C4(s1 + 0x28, s3, s1 + 0x38, s1 + 0x48);

        /* Update mirror (>>12) */
        b644_sw(s1 + 0x50, (u32)((s32)b644_lw(s3 + 0x00) >> 12));
        b644_sw(s1 + 0x54, (u32)((s32)b644_lw(s3 + 0x08) >> 12));

        /* Clear suppress flag */
        b644_sh(s1 + 0x24, 0);

        /* Animate: func_800245D8(slot[+0x4C], 1) */
        func_800245D8((void*)(uintptr_t)b644_lw(s1 + 0x4C), 1);

        /* Advance state */
        b644_sh(s1 + 0x20, (u16)(b644_lhu(s1 + 0x20) + 1));

        /* Check completion */
        if (wm_8008BEC8(s1) == 3) {
            b644_sh(s1 + 0x20, (u16)(b644_lhu(s1 + 0x20) + 1));
        }
        goto common_tail;
    }

    /* ======== State 0x29: Teleport completion check ======== */
    case 0x29: {
        if (wm_8008BEC8(s1) == 3) {
            b644_sh(s1 + 0x20, (u16)(main_state + 1));
        }
        goto common_tail;
    }

    /* ======== State 0x2A: State reset to 0x28 ======== */
    case 0x2A: {
        b644_sb(0x8006F8E4u + (u32)slot_idx, 0);
        b644_sh(s1 + 0x20, 0x28);
        goto common_tail;
    }

    /* ======== State 0x30: Presence clear + state reset ======== */
    /* ======== State 0x32: Copy from neighbor pool+0xA8 ======== */
    case 0x30:
    case 0x32: {
        u32 pool_base_local;

        pool_base_local = b644_lw(B644_POOL_PTR);

        if (main_state == 0x30) {
            /* State 0x30: use pool+0xA8 as source */
            u32 src = pool_base_local + 0xA8;

            b644_sw(s1 + 0x28, b644_lw(src + 0x00));
            b644_sw(s1 + 0x2C, b644_lw(src + 0x04));
            b644_sw(s1 + 0x30, b644_lw(src + 0x08));
            b644_sh(s1 + 0x48, b644_lhu(src + 0x28));
        } else {
            /* State 0x32: same structure as 0x30 */
            u32 src = pool_base_local + 0xA8;

            b644_sw(s1 + 0x28, b644_lw(src + 0x00));
            b644_sw(s1 + 0x2C, b644_lw(src + 0x04));
            b644_sw(s1 + 0x30, b644_lw(src + 0x08));
            b644_sh(s1 + 0x48, b644_lhu(src + 0x28));
        }

        b644_sh(s1 + 0x24, 1);
        wm_800894C8((u32)(slot_idx + 0x2E));
        goto common_tail;
    }

    /* ======== All other states: common tail ======== */
    default:
        goto common_tail;
    }

common_tail:
    /* Common tail: if suppress flag (slot[+0x24]) == 0, mirror + register */
    if (b644_lh(s1 + 0x24) == 0) {
        wm_80074794(0, s1 + 0x28);
    }

    return 1;
}
