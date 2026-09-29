/*
 * World-map scheduler callback 0x8008C844 (slot-4 cb1).
 *
 * Register-faithful transcription of retail world_map.bin
 * [0x8008C844, 0x8008D3F0).  See world_map_callback_8c844.h.
 */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_8c844.h"
#include "world_map_animation_guard.h"
#include "world_map_common_tail.h"
#include "world_map_func_95414.h"
#include "world_map_helper_74794.h"
#include "world_map_helper_8bec8.h"
#include "world_map_helper_8c040.h"
#include "world_map_helper_8c1dc.h"
#include "world_map_helper_8dff4.h"
#include "world_map_helper_90c68.h"
#include "world_map_helper_93f18.h"
#include "world_map_helper_94060.h"
#include "world_map_helper_941c4.h"
#include "world_map_helper_94238.h"
#include "world_map_helper_97770.h"

extern void func_800245D8(void* object, s16 animation);
extern long rcos(long a);
extern long rsin(long a);
extern void wm_8007528C(void);

/* ------------------------------------------------------------------ */
/* Guest layout                                                        */
/* ------------------------------------------------------------------ */

#define C844_POOL_PTR    0x8009BE24u
#define C844_MODE_WORD   0x8009BE10u
#define C844_AREA_BYTE   0x8009BD60u
#define C844_D8009BD04   0x8009BD04u
#define C844_RING_INDEX  0x8009D154u
#define C844_RING_BASE   0x8009CEC4u
#define C844_RING_STRIDE 0x14u
#define C844_RING_MASK   0x1Fu
#define C844_SCRATCH     0x1F800000u
#define C844_POSE_BLOCK  0x8009D55Cu
#define C844_HEAD_MIRROR 0x8009D52Cu
#define C844_D554        0x8009D554u
#define C844_D7CC        0x8009D7CCu
#define C844_BOUNDARY    0x8009D738u
#define C844_CRUMB_TAB   0x8009B180u
#define C844_RESYNC_BYTE 0x8006F8E5u
#define C844_BUSY2_BYTE  0x8006F8E6u
#define C844_BUSY3_BYTE  0x8006F8E7u
#define C844_SLOT2_PRES  0x8006F369u
#define C844_SLOT3_PRES  0x8006F36Au
#define C844_PRES_BASE   0x8006F364u
#define C844_MIRROR_X    0x8006EF90u
#define C844_MIRROR_Z    0x8006EF92u
#define C844_MIRROR_HEAD 0x8006EE5Au

/* ------------------------------------------------------------------ */
/* Width-faithful memory helpers                                       */
/* ------------------------------------------------------------------ */

static u32 c844_lw(u32 a) { u32 v; memcpy(&v, PSX_ADDR(a), 4); return v; }
static void c844_sw(u32 a, u32 v) { memcpy(PSX_ADDR(a), &v, 4); }
static u16 c844_lhu(u32 a) { u16 v; memcpy(&v, PSX_ADDR(a), 2); return v; }
static s16 c844_lh(u32 a) { s16 v; memcpy(&v, PSX_ADDR(a), 2); return v; }
static void c844_sh(u32 a, u16 v) { memcpy(PSX_ADDR(a), &v, 2); }
static u8 c844_lbu(u32 a) { return *(u8*)PSX_ADDR(a); }
static void c844_sb(u32 a, u8 v) { *(u8*)PSX_ADDR(a) = v; }

static u32 slot_base(u32 pool_ptr, s32 idx) { return pool_ptr + (u32)(idx << 7); }

/* ------------------------------------------------------------------ */
/* Main implementation                                                 */
/* ------------------------------------------------------------------ */

s32 wm_8008C844(s32 slot_idx)
{
    u32 pool_ptr;
    u32 s1;          /* slot record base */
    u32 s4;          /* scratchpad base for wm_80095414 output */
    s32 main_state;
    u32 v0;
    s32 v1;

    s4 = C844_SCRATCH;
    pool_ptr = c844_lw(C844_POOL_PTR);
    s1 = slot_base(pool_ptr, slot_idx);

    /* --- Pre-dispatch (if/else on slot[+0x04]) --- */
    v1 = c844_lh(s1 + 0x04);

    if (v1 == 4) {
        /* Init: set main=0x10, clear sub, set lap=1, write resync=1 */
        c844_sh(s1 + 0x20, 0x10);
        c844_sh(s1 + 0x04, 0);
        c844_sw(s1 + 0x58, 1);
        c844_sb(C844_RESYNC_BYTE, 1);
    } else if (v1 < 5) {
        if (v1 == 3) {
            /* Set main=0x30 */
            c844_sh(s1 + 0x04, 0);
            c844_sh(s1 + 0x20, 0x30);
        }
        /* else: fall through */
    } else if (v1 == 7) {
        /* Lap counter: increment, check against mode_word */
        u32 lap = c844_lw(s1 + 0x58);
        u32 mode = c844_lw(C844_MODE_WORD);
        c844_sh(s1 + 0x04, 0);
        lap++;
        c844_sw(s1 + 0x58, lap);
        if (mode == lap) {
            c844_sh(s1 + 0x20, 1);
            c844_sw(C844_MODE_WORD, 2);
            c844_sh(C844_D8009BD04, 0);
        }
    } else if (v1 == 8) {
        /* Set main=0x18 */
        c844_sh(s1 + 0x04, 0);
        c844_sh(s1 + 0x20, 0x18);
    }
    /* else: fall through to main dispatch */

    /* --- Main state dispatch --- */
    main_state = c844_lh(s1 + 0x20);
    if ((u32)main_state >= 0x41) {
        goto common_tail;
    }

    switch (main_state) {
    /* ======== States 0x00,0x01: Ring buffer + wm_80090C68 + wm_80095414 ======== */
    case 0x00:
    case 0x01: {
        u8 pres = c844_lbu(C844_RESYNC_BYTE);

        if (pres == 1) {
            /* wm_80090C68 path */
            s32 ret = wm_80090C68(s1);

            if (ret == 3) {
                /* Boundary hit: set main=0x08 */
                c844_sh(s1 + 0x20, 8);
                c844_sh(C844_D8009BD04, 0);
                goto common_tail;
            } else if (ret < 4) {
                if (ret == 1) {
                    /* Area mismatch: clear D554/D7CC */
                    c844_sw(C844_D554, 0);
                    c844_sw(C844_D7CC, 0);
                    c844_sh(C844_D8009BD04, 0);
                    goto common_tail;
                }
                /* ret == 0: normal, continue to movement */
            } else if (ret == 4) {
                /* wm_80093F18 + wm_80094060 path */
                s32 area_result = wm_80093F18(s1 + 0x28);
                s16 area_s = (s16)(u16)(area_result << 16 >> 16);
                s32 mode_result = wm_80094060(1, (s32)area_s);
                if ((mode_result << 16) == 0) {
                    c844_sh(C844_D8009BD04, 0);
                    goto common_tail;
                }
                c844_sh(s1 + 0x20, 0x20);
                c844_sh(C844_D8009BD04, 0);
                goto common_tail;
            }

            /* Movement path: check if velocity is zero */
            {
                u32 vx = c844_lw(s1 + 0x38);
                u32 vy = c844_lw(s1 + 0x3C);
                u32 vz = c844_lw(s1 + 0x40);

                if (vx == 0 && vy == 0 && vz == 0) {
                    /* Zero velocity: check anim flag */
                    u32 actor = c844_lw(s1 + 0x4C);
                    if (wm_native_animation_differs(actor, 0)) {
                        func_800245D8((void*)(uintptr_t)actor, 0);
                        wm_800894C8(0x2C);
                    }
                } else {
                    /* Non-zero velocity: check anim flag */
                    u32 actor = c844_lw(s1 + 0x4C);
                    if (wm_native_animation_differs(actor, 1)) {
                        func_800245D8((void*)(uintptr_t)actor, 1);
                    }
                    wm_8008C1DC(0x2C, s1, s4);
                }
            }

            /* wm_80095414 movement */
            {
                u32 pos = s1 + 0x28;
                u32 vel = s1 + 0x38;
                u32 out = s4 + 0x90;
                s32 speed = (s32)(s16)c844_lh(s1 + 0x4A) << 12;
                u32 mode_w = c844_lw(C844_MODE_WORD);
                s32 result = wm_80095414(pos, vel, out, speed, (s32)mode_w);

                if (result != 0) {
                    /* Movement occurred: check boundary */
                    u32 area_b = c844_lbu(C844_AREA_BYTE);
                    u32 bound = c844_lbu(C844_BOUNDARY);
                    u32 crumb_idx;

                    if (area_b == 7) {
                        crumb_idx = (u32)((bound + 3) << 1);
                    } else {
                        crumb_idx = (u32)(bound << 1);
                    }

                    {
                        u16 crumb = c844_lhu(C844_CRUMB_TAB + crumb_idx * 2);
                        if (crumb != 0) {
                            /* Breadcrumb active: copy output to slot */
                            c844_sw(s1 + 0x28, c844_lw(s4 + 0x90));
                            c844_sw(s1 + 0x2C, c844_lw(s4 + 0x94));
                            c844_sw(s1 + 0x30, c844_lw(s4 + 0x98));
                            c844_sw(s1 + 0x34, c844_lw(s4 + 0x9C));

                            /* Update ring buffer */
                            if (c844_lw(s1 + 0x38) != 0 || c844_lw(s1 + 0x40) != 0) {
                                u32 ri = (c844_lhu(C844_RING_INDEX) + 1) & C844_RING_MASK;
                                u32 ring_entry = C844_RING_BASE + ri * C844_RING_STRIDE;
                                c844_sh(C844_RING_INDEX, (u16)ri);
                                c844_sw(ring_entry + 0, c844_lw(s1 + 0x28));
                                c844_sw(ring_entry + 4, c844_lw(s1 + 0x2C));
                                c844_sw(ring_entry + 8, c844_lw(s1 + 0x30));
                                c844_sw(ring_entry + 12, c844_lw(s1 + 0x34));
                                c844_sh(ring_entry + 16, c844_lhu(s1 + 0x48));
                                wm_8007528C();
                            }
                        }
                    }
                } else if (result == 0) {
                    /* No movement: clear velocity */
                    c844_sw(s1 + 0x40, 0);
                    c844_sw(s1 + 0x38, 0);
                }
            }

            /* wm_80094238 + mirror write */
            wm_80094238(s1 + 0x28, 1);
            c844_sw(s1 + 0x40, 0);
            c844_sw(s1 + 0x3C, 0);
            c844_sw(s1 + 0x38, 0);

            /* Copy to pose block */
            c844_sw(C844_POSE_BLOCK + 0, c844_lw(s1 + 0x28));
            c844_sw(C844_POSE_BLOCK + 4, c844_lw(s1 + 0x2C));
            c844_sw(C844_POSE_BLOCK + 8, c844_lw(s1 + 0x30));
            c844_sw(C844_POSE_BLOCK + 12, c844_lw(s1 + 0x34));
            c844_sh(C844_HEAD_MIRROR, c844_lhu(s1 + 0x48));
            c844_sh(C844_D8009BD04, 0);
            goto common_tail;
        }

        /* pres != 1: idle path */
        {
            u32 actor = c844_lw(s1 + 0x4C);
            if (wm_native_animation_differs(actor, 3)) {
                func_800245D8((void*)(uintptr_t)actor, 3);
                wm_800894C8(0x2C);
            }
        }
        goto common_tail;
    }

    /* ======== State 0x02: Neighbor copy ======== */
    case 0x02: {
        u32 nb = pool_ptr + 0x3A8;
        c844_sw(s1 + 0x28, c844_lw(nb + 0));
        c844_sw(s1 + 0x30, c844_lw(nb + 8));
        c844_sh(s1 + 0x48, c844_lhu(nb + 0x20));
        goto common_tail;
    }

    /* ======== State 0x08: Approach step ======== */
    case 0x08: {
        u8 slot2_pres = c844_lbu(C844_SLOT2_PRES);
        if (slot2_pres == 0xFF) {
            goto advance_state;
        }
        {
            u8 busy2 = c844_lbu(C844_BUSY2_BYTE);
            if (busy2 == 1) {
                if (wm_80097770(5, 1) == 0) goto common_tail;
                c844_sh(s1 + 0x86, c844_lhu(C844_AREA_BYTE));
                c844_sh(s1 + 0x20, (u16)(main_state + 1));
                goto common_tail;
            }
            wm_80097770(5, 1);
            c844_sh(slot_base(pool_ptr, 5) + 0x106, c844_lbu(C844_AREA_BYTE));
            wm_80097770(5, 8);
        }
        goto advance_state;
    }

    /* ======== State 0x09: Approach completion ======== */
    case 0x09: {
        u8 slot3_pres = c844_lbu(C844_SLOT3_PRES);
        if (slot3_pres == 0xFF) {
            goto advance_state;
        }
        {
            u8 busy3 = c844_lbu(C844_BUSY3_BYTE);
            if (busy3 == 1) {
                if (wm_80097770(6, 1) == 0) goto common_tail;
                c844_sh(s1 + 0x106, c844_lhu(C844_AREA_BYTE));
                c844_sh(s1 + 0x20, (u16)(main_state + 1));
                goto common_tail;
            }
            wm_80097770(6, 1);
            c844_sh(slot_base(pool_ptr, 6) + 0x186, c844_lbu(C844_AREA_BYTE));
            wm_80097770(6, 8);
        }
        goto advance_state;
    }

    /* ======== State 0x0A: Approach via wm_800941C4 ======== */
    case 0x0A: {
        u32 area_idx = c844_lbu(C844_AREA_BYTE);
        u32 src_rec = slot_base(pool_ptr, (s32)area_idx);

        wm_800941C4(s1 + 0x28, src_rec + 0x28, s1 + 0x38, s1 + 0x48);

        /* Store approach target (>>12) from source record */
        c844_sw(s1 + 0x50, (u32)((s32)c844_lw(src_rec + 0x28) >> 12));
        c844_sw(s1 + 0x54, (u32)((s32)c844_lw(src_rec + 0x30) >> 12));

        func_800245D8((void*)(uintptr_t)c844_lw(s1 + 0x4C), 1);
        c844_sh(s1 + 0x20, (u16)(main_state + 1));
        goto common_tail;
    }

    /* ======== State 0x0B: Completion check + mirror ======== */
    case 0x0B: {
        if (wm_8008BEC8(s1) == 3) {
            c844_sh(s1 + 0x20, (u16)(main_state + 1));
        }
        /* Mirror write */
        c844_sw(C844_POSE_BLOCK + 0, c844_lw(s1 + 0x28));
        c844_sw(C844_POSE_BLOCK + 4, c844_lw(s1 + 0x2C));
        c844_sw(C844_POSE_BLOCK + 8, c844_lw(s1 + 0x30));
        c844_sw(C844_POSE_BLOCK + 12, c844_lw(s1 + 0x34));
        c844_sh(C844_HEAD_MIRROR, c844_lhu(s1 + 0x48));
        wm_8008C1DC(0x2C, s1, s4);
        goto common_tail;
    }

    /* ======== State 0x0C: wm_80097770 check ======== */
    case 0x0C: {
        u32 area = c844_lbu(C844_AREA_BYTE);
        if (wm_80097770(area, 4) == 0) goto common_tail;
        c844_sh(s1 + 0x24, 1);
        c844_sh(s1 + 0x20, 2);
        wm_800894C8(0x2C);
        goto common_tail;
    }

    /* ======== State 0x10: Mode word dispatch ======== */
    case 0x10: {
        u32 mode = c844_lw(C844_MODE_WORD);
        if (mode == 1) {
            c844_sh(s1 + 0x20, 1);
            goto common_tail;
        } else if (mode == 2) {
            u8 busy2 = c844_lbu(C844_BUSY2_BYTE);
            if (busy2 != 0) goto advance_state;
            goto common_tail;
        } else if (mode == 3) {
            u8 busy2 = c844_lbu(C844_BUSY2_BYTE);
            u8 busy3 = c844_lbu(C844_BUSY3_BYTE);
            if ((busy3 & busy2) != 0) goto advance_state;
            goto common_tail;
        }
        goto common_tail;
    }

    /* ======== State 0x11: Heading completion ======== */
    case 0x11: {
        u8 slot2_pres = c844_lbu(C844_SLOT2_PRES);
        if (slot2_pres == 0xFF) goto advance_state;
        if (wm_80097770(5, 5) == 0) goto common_tail;
        goto advance_state;
    }

    /* ======== State 0x12: Presence check ======== */
    case 0x12: {
        u8 slot3_pres = c844_lbu(C844_SLOT3_PRES);
        if (slot3_pres == 0xFF) {
            c844_sh(s1 + 0x20, 0x40);
            goto common_tail;
        }
        if (wm_80097770(6, 5) == 0) goto common_tail;
        c844_sh(s1 + 0x20, 0x40);
        goto common_tail;
    }

    /* ======== State 0x18: Timer start ======== */
    case 0x18: {
        u8 pres = c844_lbu(C844_PRES_BASE + (u32)slot_idx);
        if (pres == 7) {
            c844_sh(s1 + 0x22, 1);
            c844_sh(s1 + 0x20, 0x1A);
        } else {
            /* Write (pos>>12) to scratchpad, call wm_80089160 */
            c844_sh(s4 + 0xA0, (u16)((s32)c844_lw(s1 + 0x28) >> 12));
            c844_sh(s4 + 0xA2, (u16)((s32)c844_lw(s1 + 0x2C) >> 12));
            c844_sh(s4 + 0xA4, (u16)((s32)c844_lw(s1 + 0x30) >> 12));
            wm_80089160((u32)pres, s4 + 0xA0, 0);
            c844_sh(s1 + 0x22, 8);
            c844_sh(s1 + 0x20, (u16)(main_state + 1));
        }
        goto common_tail;
    }

    /* ======== State 0x19: Timer countdown ======== */
    case 0x19: {
        u16 sub = c844_lhu(s1 + 0x22);
        sub--;
        c844_sh(s1 + 0x22, sub);
        if ((s16)sub > 0) goto common_tail;
        c844_sh(s1 + 0x24, 1);
        c844_sh(s1 + 0x22, 0x10);
        c844_sh(s1 + 0x20, (u16)(main_state + 1));
        goto common_tail;
    }

    /* ======== State 0x1A: Timer end ======== */
    case 0x1A: {
        u16 sub = c844_lhu(s1 + 0x22);
        sub--;
        c844_sh(s1 + 0x22, sub);
        if ((s16)sub > 0) goto common_tail;
        func_800245D8((void*)(uintptr_t)c844_lw(s1 + 0x4C), 3);
        c844_sh(s1 + 0x24, 1);
        c844_sh(s1 + 0x20, 2);
        goto common_tail;
    }

    /* ======== State 0x20: Presence release ======== */
    case 0x20: {
        func_800245D8((void*)(uintptr_t)c844_lw(s1 + 0x4C), 0);
        {
            u8 slot2p = c844_lbu(C844_SLOT2_PRES);
            if (slot2p != 0xFF) {
                wm_80097770(5, 2);
            }
        }
        {
            u8 slot3p = c844_lbu(C844_SLOT3_PRES);
            if (slot3p != 0xFF) {
                wm_80097770(6, 2);
            }
        }
        c844_sw(s1 + 0x40, 0);
        c844_sw(s1 + 0x3C, 0);
        c844_sw(s1 + 0x38, 0);
        c844_sh(s1 + 0x20, (u16)(main_state + 1));
        goto common_tail;
    }

    /* ======== State 0x21: wm_80097770 check ======== */
    case 0x21: {
        if (wm_80097770(1, 2) == 0) goto common_tail;
        c844_sh(slot_base(pool_ptr, 1) + 0x86, (u16)slot_idx);
        goto advance_state;
    }

    /* ======== State 0x22: State reset ======== */
    case 0x22: {
        c844_sh(s1 + 0x20, 0);
        goto common_tail;
    }

    /* ======== State 0x30: Neighbor approach ======== */
    case 0x30: {
        u32 pool_base2;
        s32 cos_val, sin_val;

        wm_8008DFF4(s1 + 0x28);

        pool_base2 = c844_lw(C844_POOL_PTR);
        c844_sh(s1 + 0x24, 0);

        v0 = c844_lw(pool_base2 + 0x3F8);
        c844_sh(s1 + 0x48, (u16)v0);

        /* velocity.x = rcos(heading) * 32 */
        cos_val = (s32)rcos((long)(s16)(u16)v0);
        v1 = cos_val * 3;
        v1 = v1 << 5;
        c844_sw(s4 + 0, c844_lw(s1 + 0x28) + (u32)v1);

        /* velocity.z = -rsin(heading) * 32 */
        sin_val = (s32)rsin((long)c844_lh(s1 + 0x48));
        v0 = (u32)(-(s32)sin_val);
        v1 = (s32)v0 * 3;
        v1 = v1 << 5;
        c844_sw(s4 + 8, c844_lw(s1 + 0x30) + (u32)v1);

        wm_800941C4(s1 + 0x28, s4, s1 + 0x38, s1 + 0x48);

        c844_sw(s1 + 0x50, (u32)((s32)c844_lw(s4 + 0) >> 12));
        c844_sw(s1 + 0x54, (u32)((s32)c844_lw(s4 + 8) >> 12));

        func_800245D8((void*)(uintptr_t)c844_lw(s1 + 0x4C), 1);

        c844_sh(s1 + 0x20, (u16)(main_state + 1));
        goto common_tail;
    }

    /* ======== State 0x31: Completion ======== */
    case 0x31: {
        if (wm_8008BEC8(s1) == 3) {
            func_800245D8((void*)(uintptr_t)c844_lw(s1 + 0x4C), 0);
            c844_sw(s1 + 0x40, 0);
            c844_sw(s1 + 0x3C, 0);
            c844_sw(s1 + 0x38, 0);
            c844_sh(s1 + 0x20, (u16)(main_state + 1));
        }
        /* Mirror write */
        c844_sw(C844_POSE_BLOCK + 0, c844_lw(s1 + 0x28));
        c844_sw(C844_POSE_BLOCK + 4, c844_lw(s1 + 0x2C));
        c844_sw(C844_POSE_BLOCK + 8, c844_lw(s1 + 0x30));
        c844_sw(C844_POSE_BLOCK + 12, c844_lw(s1 + 0x34));
        c844_sh(C844_HEAD_MIRROR, c844_lhu(s1 + 0x48));
        goto common_tail;
    }

    /* ======== State 0x32: Ring buffer reset ======== */
    case 0x32: {
        u32 ring_dst = C844_RING_BASE;
        u32 i;
        c844_sh(C844_RING_INDEX, 0);

        /* Copy slot position/heading to scratchpad */
        c844_sw(s4 + 0x30, c844_lw(s1 + 0x28));
        c844_sw(s4 + 0x34, c844_lw(s1 + 0x2C));
        c844_sw(s4 + 0x38, c844_lw(s1 + 0x30));
        c844_sw(s4 + 0x3C, c844_lw(s1 + 0x34));
        c844_sh(s4 + 0xA0, c844_lhu(s1 + 0x48));

        /* Fill 32 ring entries */
        for (i = 0; i < 32; i++) {
            c844_sw(ring_dst + 0, c844_lw(s4 + 0x30));
            c844_sw(ring_dst + 4, c844_lw(s4 + 0x34));
            c844_sw(ring_dst + 8, c844_lw(s4 + 0x38));
            c844_sw(ring_dst + 12, c844_lw(s4 + 0x3C));
            c844_sh(ring_dst + 16, c844_lhu(s4 + 0xA0));
            ring_dst += C844_RING_STRIDE;
        }

        c844_sh(s1 + 0x20, 1);
        c844_sw(C844_MODE_WORD, 2);
        goto common_tail;
    }

    default:
        goto common_tail;
    }

advance_state:
    c844_sh(s1 + 0x20, (u16)(c844_lhu(s1 + 0x20) + 1));
    goto common_tail;

common_tail:
    /* Gate: main_state != 2 && (mode_word != 2 || presence != 7) */
    {
        s32 ms = c844_lh(s1 + 0x20);
        if (ms != 2) {
            u32 mode_w = c844_lw(C844_MODE_WORD);
            if (mode_w != 2) {
                u8 pres = c844_lbu(C844_PRES_BASE + (u32)slot_idx - 4);
                if (pres != 7) {
                    wm_80074794(1, s1 + 0x28);
                }
            }
        }
    }

    /* Epilogue: write pos.x>>12, pos.z>>12, heading to resident mirror */
    c844_sh(C844_MIRROR_X, (u16)((s32)c844_lw(s1 + 0x28) >> 12));
    c844_sh(C844_MIRROR_Z, (u16)((s32)c844_lw(s1 + 0x30) >> 12));
    c844_sh(C844_MIRROR_HEAD, c844_lhu(s1 + 0x48));

    return 1;
}
