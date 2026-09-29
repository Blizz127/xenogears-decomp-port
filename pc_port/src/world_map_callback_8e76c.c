/*
 * World-map scheduler callback 0x8008E76C (slot-7 cb1).
 *
 * Register-faithful transcription of retail world_map.bin
 * [0x8008E76C, 0x800906E0).  See world_map_callback_8e76c.h.
 *
 * This is the world-map main gameplay handler — the largest single
 * function in the overlay (2013 insns).  Handles terrain following,
 * camera, movement, surface normals, sound, draw packets, and ring
 * buffer management for the world-map player character.
 */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_8e76c.h"
#include "world_map_common_tail.h"
#include "world_map_func_95414.h"
#include "world_map_helper_74794.h"
#include "world_map_helper_8bec8.h"
#include "world_map_helper_8bfd4.h"
#include "world_map_helper_8c040.h"
#include "world_map_helper_8e034.h"
#include "world_map_helper_8e078.h"
#include "world_map_helper_8e0f0.h"
#include "world_map_helper_90e14.h"
#include "world_map_helper_90fb4.h"
#include "world_map_helper_93354.h"
#include "world_map_helper_93a5c.h"
#include "world_map_helper_93f18.h"
#include "world_map_helper_94060.h"
#include "world_map_helper_941c4.h"
#include "world_map_helper_94238.h"
#include "world_map_helper_95cd4.h"
#include "world_map_helper_97770.h"
#include "world_map_state2_8eb64.h"
#include "world_map_state_vertical_8f690.h"
#include "world_map_state01_8eb30.h"
#include "world_map_terrain_sampler.h"

extern void func_800245D8(void* object, s16 animation);
extern long rcos(long a);
extern long rsin(long a);
extern void wm_80075228(void);
extern void wm_8007528C(void);
extern void wm_80093660(void);

/* Sound stubs — not yet implemented in PC port */
static void stub_func_80039E60(u32 a0) { (void)a0; }
static void stub_func_8003A89C(u32 a0, u32 a1, u32 a2) { (void)a0; (void)a1; (void)a2; }
static void stub_wm_800767D4(u32 a0, u32 a1) { (void)a0; (void)a1; }

/* ------------------------------------------------------------------ */
/* Guest layout                                                        */
/* ------------------------------------------------------------------ */

#define E76C_POOL_PTR    0x8009BE24u
#define E76C_MODE_WORD   0x8009BE10u
#define E76C_AREA_BYTE   0x8009BD60u
#define E76C_D8009BD04   0x8009BD04u
#define E76C_RING_INDEX  0x8009D154u
#define E76C_RING_BASE   0x8009CEC4u
#define E76C_RING_STRIDE 0x14u
#define E76C_RING_MASK   0x1Fu
#define E76C_SCRATCH     0x1F800000u
#define E76C_POSE_BLOCK  0x8009D55Cu
#define E76C_HEAD_MIRROR 0x8009D52Cu
#define E76C_D554        0x8009D554u
#define E76C_D7CC        0x8009D7CCu
#define E76C_BOUNDARY    0x8009D738u
#define E76C_CRUMB_TAB   0x8009B180u
#define E76C_RESYNC_BYTE 0x8006F8E5u
#define E76C_BUTTONS     0x8006EE68u
#define E76C_MIRROR_X    0x8006EF90u
#define E76C_MIRROR_Z    0x8006EF92u
#define E76C_MIRROR_HEAD 0x8006EE5Au
#define E76C_SOUND_STATE 0x8006EE6Au

/* ------------------------------------------------------------------ */
/* Width-faithful memory helpers                                       */
/* ------------------------------------------------------------------ */

static u32 e76c_lw(u32 a) { u32 v; memcpy(&v, PSX_ADDR(a), 4); return v; }
static void e76c_sw(u32 a, u32 v) { memcpy(PSX_ADDR(a), &v, 4); }
static u16 e76c_lhu(u32 a) { u16 v; memcpy(&v, PSX_ADDR(a), 2); return v; }
static s16 e76c_lh(u32 a) { s16 v; memcpy(&v, PSX_ADDR(a), 2); return v; }
static void e76c_sh(u32 a, u16 v) { memcpy(PSX_ADDR(a), &v, 2); }
static u8 e76c_lbu(u32 a) { return *(u8*)PSX_ADDR(a); }

static u32 slot_base(u32 pool_ptr, s32 idx)
{
    return pool_ptr + (u32)(idx << 7);
}

/* ------------------------------------------------------------------ */
/* Main implementation                                                 */
/* ------------------------------------------------------------------ */

s32 wm_8008E76C(s32 slot_idx)
{
    u32 pool_ptr;
    u32 s2;          /* slot record base */
    u32 s3;          /* scratchpad = 0x1F800000 */
    s32 s4;          /* slot[+0x04] value */
    u32 s0;          /* button-derived state */

    s3 = E76C_SCRATCH;
    pool_ptr = e76c_lw(E76C_POOL_PTR);
    s2 = slot_base(pool_ptr, slot_idx);

    /* --- Pre-dispatch on slot[+0x04] --- */
    s4 = e76c_lh(s2 + 0x04);

    /* Retail 0x8008E7B0 branches directly to the signed slot-state jump
     * table whenever this pre-dispatch value is not four.  Keep the restored
     * state arms ahead of the legacy partial body so that body cannot mutate
     * state or heading before states 0/1/2 execute. */
    if (s4 != 4) {
        goto restored_main_dispatch;
    }

    if (s4 == 4) {
        /* Lap counter: increment, check against mode_word */
        u32 lap = e76c_lw(s2 + 0x74);
        u32 mode = e76c_lw(E76C_MODE_WORD);
        e76c_sh(s2 + 0x04, 0);
        lap++;
        e76c_sw(s2 + 0x74, lap);
        if (mode != lap) {
            goto restored_main_dispatch;
        }
        /* Lap matched: init mode */
        wm_80097770(8, 9);
        /* Copy position to pose block */
        e76c_sw(E76C_POSE_BLOCK + 0, e76c_lw(s2 + 0x28));
        e76c_sw(E76C_POSE_BLOCK + 4, e76c_lw(s2 + 0x2C));
        e76c_sw(E76C_POSE_BLOCK + 8, e76c_lw(s2 + 0x30));
        e76c_sw(E76C_POSE_BLOCK + 12, e76c_lw(s2 + 0x34));

        /* Write heading mirror */
        e76c_sh(E76C_HEAD_MIRROR, e76c_lhu(s2 + 0x48));
        /* The retail lap-match continuation is not yet bounded as part of
         * the restored state slices.  Preserve the legacy partial owner for
         * that path instead of claiming the exact state dispatch here. */
        goto legacy_fallback;
    } else if (s4 == 5) {
        /* Init: terrain height, camera setup */
        e76c_sh(s2 + 0x04, 0);
        e76c_sh(s2 + 0x20, 0x0C);

        /* Terrain height */
        {
            s32 terrain_y = wm_80093978(
                (s32)e76c_lw(s2 + 0x28),
                (s32)e76c_lw(s2 + 0x30));
            e76c_sw(s2 + 0x68, (u32)(terrain_y + 0x18000));
        }

        /* Camera direction area */
        {
            u16 dir_area = e76c_lhu(0x8009BD3Cu);
            s32 zoom = (s32)e76c_lw(s2 + 0x70) >> 12;
            e76c_sh(s3 + 0xA0, (u16)(-zoom));
            e76c_sh(s3 + 0xA4, dir_area);
            e76c_sh(s3 + 0xA2, e76c_lhu(s2 + 0x48));
            e76c_sh(s3 + 0xA8, (u16)((s32)e76c_lw(s2 + 0x28) >> 12));
            e76c_sh(s3 + 0xAA, (u16)((s32)e76c_lw(s2 + 0x2C) >> 12));
            e76c_sh(s3 + 0xAC, (u16)((s32)e76c_lw(s2 + 0x30) >> 12));
        }

        /* Area-dependent table selection */
        wm_8008E078();

        /* Write to sound state */
        e76c_sh(E76C_SOUND_STATE, e76c_lhu(s2 + 0x48));

        /* wm_80089160 presence registration */
        {
            u8 area = e76c_lbu(E76C_AREA_BYTE);
            e76c_sh(s3 + 0xA0, (u16)((s32)e76c_lw(s2 + 0x28) >> 12));
            e76c_sh(s3 + 0xA2, (u16)((s32)e76c_lw(s2 + 0x2C) >> 12));
            e76c_sh(s3 + 0xA4, (u16)((s32)e76c_lw(s2 + 0x30) >> 12));
            wm_80089160((u32)area, s3 + 0xA0, 0);
        }

        /* wm_8008C040 draw packet setup */
        {
            u32 bound_addr = E76C_BOUNDARY;
            u32 area_addr = E76C_AREA_BYTE;
            wm_8008C040(s3 + 0xA0, 0x18, 0x30, bound_addr, area_addr);
        }

        /* Check breadcrumb table */
        {
            u8 area = e76c_lbu(E76C_AREA_BYTE);
            u8 bound = e76c_lbu(E76C_BOUNDARY);
            u32 crumb_idx;
            if (area == 7) {
                crumb_idx = (u32)((bound + 3) << 1);
            } else {
                crumb_idx = (u32)(bound << 1);
            }
            {
                u16 crumb = e76c_lhu(E76C_CRUMB_TAB + crumb_idx * 2);
                if (crumb != 0) {
                    /* Copy position to slot */
                    e76c_sw(s2 + 0x28, e76c_lw(s3 + 0xA0));
                    e76c_sw(s2 + 0x2C, e76c_lw(s3 + 0xA4));
                    e76c_sw(s2 + 0x30, e76c_lw(s3 + 0xA8));
                    e76c_sw(s2 + 0x34, e76c_lw(s3 + 0xAC));

                    /* Update ring buffer */
                    if (e76c_lw(s2 + 0x38) != 0 || e76c_lw(s2 + 0x40) != 0) {
                        u32 ri = (e76c_lhu(E76C_RING_INDEX) + 1) & E76C_RING_MASK;
                        u32 ring_entry = E76C_RING_BASE + ri * E76C_RING_STRIDE;
                        e76c_sh(E76C_RING_INDEX, (u16)ri);
                        e76c_sw(ring_entry + 0, e76c_lw(s2 + 0x28));
                        e76c_sw(ring_entry + 4, e76c_lw(s2 + 0x2C));
                        e76c_sw(ring_entry + 8, e76c_lw(s2 + 0x30));
                        e76c_sw(ring_entry + 12, e76c_lw(s2 + 0x34));
                        e76c_sh(ring_entry + 16, e76c_lhu(s2 + 0x48));
                        wm_8007528C();
                    }
                }
            }
        }

        /* wm_80094238 + mirror write */
        wm_80094238(s2 + 0x28, 1);
        e76c_sw(s2 + 0x40, 0);
        e76c_sw(s2 + 0x3C, 0);
        e76c_sw(s2 + 0x38, 0);
        e76c_sw(E76C_POSE_BLOCK + 0, e76c_lw(s2 + 0x28));
        e76c_sw(E76C_POSE_BLOCK + 4, e76c_lw(s2 + 0x2C));
        e76c_sw(E76C_POSE_BLOCK + 8, e76c_lw(s2 + 0x30));
        e76c_sw(E76C_POSE_BLOCK + 12, e76c_lw(s2 + 0x34));
        e76c_sh(E76C_HEAD_MIRROR, e76c_lhu(s2 + 0x48));
        e76c_sh(E76C_D8009BD04, 0);
        goto common_tail;
    } else if (s4 == 1) {
        /* State 1: camera heading input */
        s32 ret = wm_80090FB4(s2);
        if (ret == 3) {
            e76c_sh(s2 + 0x20, 8);
            e76c_sh(E76C_D8009BD04, 0);
            goto common_tail;
        } else if (ret == 4) {
            /* wm_80093F18 + wm_80094060 */
            s32 area_result = wm_80093F18(s2 + 0x28);
            s16 area_s = (s16)(u16)(area_result << 16 >> 16);
            s32 mode_result = wm_80094060(1, (s32)area_s);
            if ((mode_result << 16) == 0) {
                e76c_sh(E76C_D8009BD04, 0);
                goto common_tail;
            }
            e76c_sh(s2 + 0x20, 0x20);
            e76c_sh(E76C_D8009BD04, 0);
            goto common_tail;
        }
    } else if (s4 == 6) {
        /* State 6: wm_80090E14 heading variant */
        s32 ret = wm_80090E14(s2);
        if (ret == 1) {
            e76c_sh(s2 + 0x20, 1);
            goto common_tail;
        } else if (ret == 4) {
            e76c_sh(s2 + 0x20, 0x40);
            goto common_tail;
        }
    } else if (s4 == 7) {
        /* State 7: wm_8008E0F0 angle search */
        s32 angle = wm_8008E0F0(s2 + 0x28, 0, s3 + 0x90);
        if (angle != -1) {
            e76c_sh(s2 + 0x48, (u16)angle);
            e76c_sh(E76C_HEAD_MIRROR, (u16)angle);
        }
    } else if (s4 == 8) {
        /* State 8: wm_8008E078 area table */
        wm_8008E078();
    }

restored_main_dispatch:
    /* Retail dispatches on the signed halfword at slot[+0x20]. */
    {
        s16 state = e76c_lh(s2 + 0x20u);
        if (state == 0 || state == 1)
            return wm_8008E76C_state01(slot_idx, s2);
        if (state == 2)
            return wm_8008E76C_state2(s2);
        /* jtbl_80070A50 entries 8/12/16/20: the vertical transitions */
        if (state == 8)
            return wm_8008E76C_state8(s2);
        if (state == 12)
            return wm_8008E76C_state12(s2);
        if (state == 16)
            return wm_8008E76C_state16(s2);
        if (state == 20)
            return wm_8008E76C_state20(s2);
    }

    /* --- Legacy fallback for not-yet-restored states --- */
legacy_fallback:
    {
        u16 buttons = e76c_lhu(E76C_BUTTONS);
        s0 = (u32)(buttons & 0x1FFF);

        /* Dispatch on s0 */
        if (s0 == 1) {
            /* Init state: terrain height, camera setup */
            e76c_sh(s2 + 0x20, 0x0C);
            e76c_sw(E76C_MODE_WORD, (u32)s4);

            {
                s32 terrain_y = wm_80093978(
                    (s32)e76c_lw(s2 + 0x28),
                    (s32)e76c_lw(s2 + 0x30));
                e76c_sw(s2 + 0x68, (u32)(terrain_y + 0x18000));
            }

            /* Camera direction setup */
            {
                u16 dir_area = e76c_lhu(0x8009BD3Cu);
                s32 zoom = (s32)e76c_lw(s2 + 0x70) >> 12;
                e76c_sh(s3 + 0xA0, (u16)(-zoom));
                e76c_sh(s3 + 0xA4, dir_area);
                e76c_sh(s3 + 0xA2, e76c_lhu(s2 + 0x48));
                e76c_sh(s3 + 0xA8, (u16)((s32)e76c_lw(s2 + 0x28) >> 12));
                e76c_sh(s3 + 0xAA, (u16)((s32)e76c_lw(s2 + 0x2C) >> 12));
                e76c_sh(s3 + 0xAC, (u16)((s32)e76c_lw(s2 + 0x30) >> 12));
            }

            wm_8008E078();

            /* wm_80089160 */
            {
                u8 area = e76c_lbu(E76C_AREA_BYTE);
                e76c_sh(s3 + 0xA0, (u16)((s32)e76c_lw(s2 + 0x28) >> 12));
                e76c_sh(s3 + 0xA2, (u16)((s32)e76c_lw(s2 + 0x2C) >> 12));
                e76c_sh(s3 + 0xA4, (u16)((s32)e76c_lw(s2 + 0x30) >> 12));
                wm_80089160((u32)area, s3 + 0xA0, 0);
            }

            /* wm_8008C040 */
            {
                u32 bound_addr = E76C_BOUNDARY;
                u32 area_addr = E76C_AREA_BYTE;
                wm_8008C040(s3 + 0xA0, 0x18, 0x30, bound_addr, area_addr);
            }

            /* Breadcrumb check */
            {
                u8 area = e76c_lbu(E76C_AREA_BYTE);
                u8 bound = e76c_lbu(E76C_BOUNDARY);
                u32 crumb_idx;
                if (area == 7) {
                    crumb_idx = (u32)((bound + 3) << 1);
                } else {
                    crumb_idx = (u32)(bound << 1);
                }
                {
                    u16 crumb = e76c_lhu(E76C_CRUMB_TAB + crumb_idx * 2);
                    if (crumb != 0) {
                        e76c_sw(s2 + 0x28, e76c_lw(s3 + 0xA0));
                        e76c_sw(s2 + 0x2C, e76c_lw(s3 + 0xA4));
                        e76c_sw(s2 + 0x30, e76c_lw(s3 + 0xA8));
                        e76c_sw(s2 + 0x34, e76c_lw(s3 + 0xAC));
                        if (e76c_lw(s2 + 0x38) != 0 || e76c_lw(s2 + 0x40) != 0) {
                            u32 ri = (e76c_lhu(E76C_RING_INDEX) + 1) & E76C_RING_MASK;
                            u32 ring_entry = E76C_RING_BASE + ri * E76C_RING_STRIDE;
                            e76c_sh(E76C_RING_INDEX, (u16)ri);
                            e76c_sw(ring_entry + 0, e76c_lw(s2 + 0x28));
                            e76c_sw(ring_entry + 4, e76c_lw(s2 + 0x2C));
                            e76c_sw(ring_entry + 8, e76c_lw(s2 + 0x30));
                            e76c_sw(ring_entry + 12, e76c_lw(s2 + 0x34));
                            e76c_sh(ring_entry + 16, e76c_lhu(s2 + 0x48));
                            wm_8007528C();
                        }
                    }
                }
            }

            /* wm_80094238 + mirror */
            wm_80094238(s2 + 0x28, 1);
            e76c_sw(s2 + 0x40, 0);
            e76c_sw(s2 + 0x3C, 0);
            e76c_sw(s2 + 0x38, 0);
            e76c_sw(E76C_POSE_BLOCK + 0, e76c_lw(s2 + 0x28));
            e76c_sw(E76C_POSE_BLOCK + 4, e76c_lw(s2 + 0x2C));
            e76c_sw(E76C_POSE_BLOCK + 8, e76c_lw(s2 + 0x30));
            e76c_sw(E76C_POSE_BLOCK + 12, e76c_lw(s2 + 0x34));
            e76c_sh(E76C_HEAD_MIRROR, e76c_lhu(s2 + 0x48));
            e76c_sh(E76C_D8009BD04, 0);

        } else if (s0 == 2) {
            /* Main gameplay state: movement with collision */
            u32 pos = s2 + 0x28;
            u32 vel = s2 + 0x38;
            u32 out = s3 + 0x90;
            s32 speed = (s32)(s16)e76c_lh(s2 + 0x4A) << 12;
            u32 mode_w = e76c_lw(E76C_MODE_WORD);

            /* wm_80095CD4 collision-aware movement */
            s32 result = wm_80095CD4(pos, vel, out, speed,
                                     (s32)mode_w);

            if (result != 0) {
                /* Movement occurred */
                u32 area = e76c_lbu(E76C_AREA_BYTE);
                u32 bound = e76c_lbu(E76C_BOUNDARY);
                u32 crumb_idx;

                if (area == 7) {
                    crumb_idx = (u32)((bound + 3) << 1);
                } else {
                    crumb_idx = (u32)(bound << 1);
                }

                {
                    u16 crumb = e76c_lhu(E76C_CRUMB_TAB + crumb_idx * 2);
                    if (crumb != 0) {
                        /* Copy output to slot */
                        e76c_sw(s2 + 0x28, e76c_lw(s3 + 0x90));
                        e76c_sw(s2 + 0x2C, e76c_lw(s3 + 0x94));
                        e76c_sw(s2 + 0x30, e76c_lw(s3 + 0x98));
                        e76c_sw(s2 + 0x34, e76c_lw(s3 + 0x9C));

                        /* Update ring buffer */
                        if (e76c_lw(s2 + 0x38) != 0 || e76c_lw(s2 + 0x40) != 0) {
                            u32 ri = (e76c_lhu(E76C_RING_INDEX) + 1) & E76C_RING_MASK;
                            u32 ring_entry = E76C_RING_BASE + ri * E76C_RING_STRIDE;
                            e76c_sh(E76C_RING_INDEX, (u16)ri);
                            e76c_sw(ring_entry + 0, e76c_lw(s2 + 0x28));
                            e76c_sw(ring_entry + 4, e76c_lw(s2 + 0x2C));
                            e76c_sw(ring_entry + 8, e76c_lw(s2 + 0x30));
                            e76c_sw(ring_entry + 12, e76c_lw(s2 + 0x34));
                            e76c_sh(ring_entry + 16, e76c_lhu(s2 + 0x48));
                            wm_8007528C();
                        }
                    }
                }
            } else {
                /* No movement: clear velocity */
                e76c_sw(s2 + 0x40, 0);
                e76c_sw(s2 + 0x38, 0);
            }

            /* wm_80094238 + mirror */
            wm_80094238(s2 + 0x28, 1);
            e76c_sw(s2 + 0x40, 0);
            e76c_sw(s2 + 0x3C, 0);
            e76c_sw(s2 + 0x38, 0);
            e76c_sw(E76C_POSE_BLOCK + 0, e76c_lw(s2 + 0x28));
            e76c_sw(E76C_POSE_BLOCK + 4, e76c_lw(s2 + 0x2C));
            e76c_sw(E76C_POSE_BLOCK + 8, e76c_lw(s2 + 0x30));
            e76c_sw(E76C_POSE_BLOCK + 12, e76c_lw(s2 + 0x34));
            e76c_sh(E76C_HEAD_MIRROR, e76c_lhu(s2 + 0x48));
            e76c_sh(E76C_D8009BD04, 0);

        } else if (s0 == 3) {
            /* Sound state */
            u32 sound_handle = e76c_lw(0x80062528u);
            stub_func_8003A89C(sound_handle, 0, 0xF0);
            stub_func_80039E60(0x3C);
            stub_wm_800767D4(0x08, 0);
        }
    }

common_tail:
    /* Gate: main_state != 2 && (mode_word != 2 || presence != 7) */
    {
        s32 ms = e76c_lh(s2 + 0x20);
        if (ms != 2) {
            u32 mode_w = e76c_lw(E76C_MODE_WORD);
            if (mode_w != 2) {
                u8 pres = e76c_lbu(E76C_RESYNC_BYTE);
                if (pres != 7) {
                    wm_80074794(1, s2 + 0x28);
                }
            }
        }
    }

    /* Epilogue: write pos.x>>12, pos.z>>12, heading to resident mirror */
    e76c_sh(E76C_MIRROR_X, (u16)((s32)e76c_lw(s2 + 0x28) >> 12));
    e76c_sh(E76C_MIRROR_Z, (u16)((s32)e76c_lw(s2 + 0x30) >> 12));
    e76c_sh(E76C_MIRROR_HEAD, e76c_lhu(s2 + 0x48));

    return 1;
}
