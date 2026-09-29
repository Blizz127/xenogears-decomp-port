/*
 * World-map scheduler callback 0x8008A72C (slot-1 world player handler).
 *
 * Register-faithful transcription of retail world_map.bin
 * [0x8008A72C, 0x8008B2BC).  See world_map_callback_8a72c.h and
 * docs/evidence/w34b24-i8-8a72c/ (implementation) +
 * docs/evidence/w34b24-pre3-8a72c/ (audit).
 *
 * JT1 @0x80070518, 65 entries (index = lh slot[+0x20], sltiu 0x41 guard,
 * out-of-range -> common tail).  Full retail slot->target map (verified
 * word-by-word from the image; nothing collapsed silently):
 *   0x00,0x01 -> 0x8008A810  init/idle (resync gate + JT2 sub-dispatch)
 *   0x02,0x03 -> 0x8008ABB4  follow slot-7 record copy
 *   0x08      -> 0x8008ABF0  slot-2 handshake
 *   0x09      -> 0x8008AC70  slot-3 handshake
 *   0x0A      -> 0x8008ACF0  slot-4 claim
 *   0x0D      -> 0x8008AD24  approach setup
 *   0x0E      -> 0x8008AD9C  approach step
 *   0x0F      -> 0x8008AE10  handoff
 *   0x10      -> 0x8008AE44  slot-2 re-claim
 *   0x11      -> 0x8008AE84  slot-3 re-claim
 *   0x12      -> 0x8008AEC4  teleport + ring refill
 *   0x28      -> 0x8008AF5C  warp-in
 *   0x29      -> 0x8008B034  warp approach
 *   0x2A      -> 0x8008B0AC  ring reset
 *   0x2B      -> 0x8008B14C  mode wait
 *   0x2C      -> 0x8008B1D8  slot-2 release
 *   0x2D      -> 0x8008B214  slot-3 release
 *   parked -> 0x8008B240 (common tail): 0x04-0x07, 0x0B, 0x0C,
 *   0x13-0x27, 0x2E-0x40 (46 of 65 slots).
 * JT2 @0x80070620, 5 entries (index = wm_80090A84(slot)-1, sltiu 5):
 *   class1 -> 0x8008A854; class3 -> 0x8008A874;
 *   classes 2/4/5 -> 0x8008AB5C (pre-tail);
 *   OOR (class 0 or >= 6) -> 0x8008A8A8 = the LIVE MOVEMENT BLOCK.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_8a72c.h"
#include "world_map_animation_guard.h"
#include "world_map_common_tail.h"
#include "world_map_func_95414.h"
#include "world_map_helper_74794.h"
#include "world_map_helper_7528c.h"
#include "world_map_helper_8bec8.h"
#include "world_map_helper_8c040.h"
#include "world_map_helper_8c1dc.h"
#include "world_map_helper_90a84.h"
#include "world_map_helper_94238.h"
#include "world_map_helper_941c4.h"
#include "world_map_helper_97770.h"

extern void func_800245D8(void* object, s16 animation);
extern s32 wm_80093978(s32 x, s32 z);
extern long rcos(long a);
extern long rsin(long a);

/* ------------------------------------------------------------------ */
/* Guest layout                                                        */
/* ------------------------------------------------------------------ */

#define A72C_POOL_PTR    0x8009BE24u /* lw -0x41DC(0x800A) */
#define A72C_MODE_WORD   0x8009C170u /* lw -0x3E90 */
#define A72C_MODE_FLAG   0x8009BE10u /* sw/lw -0x41F0 */
#define A72C_BD04        0x8009BD04u /* sh -0x42FC */
#define A72C_AREA_BYTE   0x8009BD60u /* lbu/sb -0x42A0 */
#define A72C_BOUND_BYTE  0x8009D738u /* lbu -0x28C8 (8C040 out) */
#define A72C_CRUMB_TAB   0x8009B180u /* lhu -0x4E80 base */
#define A72C_RING_INDEX  0x8009D154u /* lhu/sh -0x2EAC */
#define A72C_RING_BASE   0x8009CEC4u /* -0x313C */
#define A72C_POSE_BLOCK  0x8009D55Cu /* -0x2AA4 */
#define A72C_HEAD_MIRROR 0x8009D52Cu /* sh -0x2AD4 */
#define A72C_D554        0x8009D554u /* sw -0x2AAC */
#define A72C_D7CC        0x8009D7CCu /* sw -0x2834 */
#define A72C_RESYNC_BYTE 0x8006F8E5u /* lbu/sb -0x71B(0x8007) */
#define A72C_BUSY2_BYTE  0x8006F8E6u /* lbu -0x71A */
#define A72C_BUSY3_BYTE  0x8006F8E7u /* lbu -0x719 */
#define A72C_SLOT4_PRES  0x8006F368u /* lbu -0xC98 */
#define A72C_SLOT2_PRES  0x8006F369u /* lbu -0xC97 */
#define A72C_SLOT3_PRES  0x8006F36Au /* lbu -0xC96 */
#define A72C_WARP_X      0x8006EF90u /* lhu -0x1070 */
#define A72C_WARP_Z      0x8006EF92u /* lhu -0x106E */
#define A72C_WARP_HEAD   0x8006EE5Au /* lhu -0x11A6 */
#define A72C_MIRROR_X    0x8006EE54u /* sh -0x11AC */
#define A72C_MIRROR_Z    0x8006EE56u /* sh -0x11AA */
#define A72C_MIRROR_HEAD 0x8006EE58u /* sh -0x11A8 */
#define A72C_SCRATCH     0x1F800000u

/* ------------------------------------------------------------------ */
/* Trace seams                                                         */
/* ------------------------------------------------------------------ */

#if defined(WM_8A72C_TEST_TRACE)
extern void wm_8a72c_test_store(u32 address, u32 width, u32 value);
extern s32 wm_8a72c_test_90a84(u32 slot_addr);
extern void wm_8a72c_test_245d8(u32 object_bits, s16 animation);
extern void wm_8a72c_test_894c8(u32 record_index);
extern void wm_8a72c_test_8c1dc(u32 ctx, u32 obj, u32 out);
extern s32 wm_8a72c_test_95414(u32 pos, u32 dir, u32 out, s32 scale, s32 mode);
extern void wm_8a72c_test_8c040(u32 vec, s32 a1, s32 a2, u32 o1, u32 o2);
extern void wm_8a72c_test_7528c(void);
extern s32 wm_8a72c_test_94238(u32 pos, u32 idx);
extern s32 wm_8a72c_test_97770(u32 slot, s32 value);
extern s32 wm_8a72c_test_941c4(u32 a, u32 b, u32 out, u32 angle);
extern s32 wm_8a72c_test_8bec8(u32 obj);
extern s32 wm_8a72c_test_93978(s32 x, s32 z);
extern long wm_8a72c_test_rcos(long a);
extern long wm_8a72c_test_rsin(long a);
extern void wm_8a72c_test_74794(s32 tag, u32 pose);
#define A72C_TRACE_STORE(a, w, v) wm_8a72c_test_store((a), (w), (v))
#define A72C_CALL_90A84(a)        wm_8a72c_test_90a84(a)
#define A72C_CALL_245D8(o, n)     wm_8a72c_test_245d8((o), (n))
#define A72C_CALL_894C8(i)        wm_8a72c_test_894c8(i)
#define A72C_CALL_8C1DC(c, o, u)  wm_8a72c_test_8c1dc((c), (o), (u))
#define A72C_CALL_95414(p, d, o, s, m) wm_8a72c_test_95414((p), (d), (o), (s), (m))
#define A72C_CALL_8C040(v, x, y, o1, o2) wm_8a72c_test_8c040((v), (x), (y), (o1), (o2))
#define A72C_CALL_7528C()         wm_8a72c_test_7528c()
#define A72C_CALL_94238(p, i)     wm_8a72c_test_94238((p), (i))
#define A72C_CALL_97770(s, v)     wm_8a72c_test_97770((s), (v))
#define A72C_CALL_941C4(a, b, o, g) wm_8a72c_test_941c4((a), (b), (o), (g))
#define A72C_CALL_8BEC8(o)        wm_8a72c_test_8bec8(o)
#define A72C_CALL_93978(x, z)     wm_8a72c_test_93978((x), (z))
#define A72C_CALL_RCOS(a)         wm_8a72c_test_rcos(a)
#define A72C_CALL_RSIN(a)         wm_8a72c_test_rsin(a)
#define A72C_CALL_74794(t, p)     wm_8a72c_test_74794((t), (p))
#else
static void wm_8a72c_native_245d8(u32 object_bits, s16 animation)
{
    func_800245D8((void*)(uintptr_t)object_bits, animation);
}
#define A72C_TRACE_STORE(a, w, v) ((void)0)
#define A72C_CALL_90A84(a)        wm_80090A84(a)
#define A72C_CALL_245D8(o, n)     wm_8a72c_native_245d8((o), (n))
#define A72C_CALL_894C8(i)        wm_800894C8(i)
#define A72C_CALL_8C1DC(c, o, u)  wm_8008C1DC((c), (o), (u))
#define A72C_CALL_95414(p, d, o, s, m) wm_80095414((p), (d), (o), (s), (m))
#define A72C_CALL_8C040(v, x, y, o1, o2) wm_8008C040((v), (x), (y), (o1), (o2))
#define A72C_CALL_7528C()         wm_8007528C()
#define A72C_CALL_94238(p, i)     wm_80094238((p), (i))
#define A72C_CALL_97770(s, v)     wm_80097770((s), (v))
#define A72C_CALL_941C4(a, b, o, g) wm_800941C4((a), (b), (o), (g))
#define A72C_CALL_8BEC8(o)        wm_8008BEC8(o)
#define A72C_CALL_93978(x, z)     wm_80093978((x), (z))
#define A72C_CALL_RCOS(a)         rcos(a)
#define A72C_CALL_RSIN(a)         rsin(a)
#define A72C_CALL_74794(t, p)     wm_80074794((t), (p))
#endif

/* ------------------------------------------------------------------ */
/* Width-faithful memory helpers                                       */
/* ------------------------------------------------------------------ */

static u32 a72c_lw(u32 a)
{
    u32 v;

    memcpy(&v, PSX_ADDR(a), 4);
    return v;
}

static void a72c_sw(u32 a, u32 v)
{
    memcpy(PSX_ADDR(a), &v, 4);
    A72C_TRACE_STORE(a, 4u, v);
}

static u16 a72c_lhu(u32 a)
{
    u16 v;

    memcpy(&v, PSX_ADDR(a), 2);
    return v;
}

static s16 a72c_lh(u32 a)
{
    s16 v;

    memcpy(&v, PSX_ADDR(a), 2);
    return v;
}

static void a72c_sh(u32 a, u16 v)
{
    memcpy(PSX_ADDR(a), &v, 2);
    A72C_TRACE_STORE(a, 2u, v);
}

static u8 a72c_lbu(u32 a)
{
    u8 v;

    memcpy(&v, PSX_ADDR(a), 1);
    return v;
}

static void a72c_sb(u32 a, u8 v)
{
    memcpy(PSX_ADDR(a), &v, 1);
    A72C_TRACE_STORE(a, 1u, v);
}

static s32 a72c_s32(u32 b)
{
    s32 v;

    memcpy(&v, &b, 4);
    return v;
}

static u32 a72c_sra(u32 b, u32 amt)
{
    u32 v = b >> amt;

    if (b & 0x80000000u)
        v |= UINT32_MAX << (32u - amt);
    return v;
}

static s32 a72c_sign16(u32 v)
{
    return (s32)(s16)(u16)(v & 0xFFFFu);
}

/* Copy four consecutive guest words (retail lw/sw quads). */
static void a72c_copy4(u32 dst, u32 src)
{
    a72c_sw(dst + 0u, a72c_lw(src + 0u));
    a72c_sw(dst + 4u, a72c_lw(src + 4u));
    a72c_sw(dst + 8u, a72c_lw(src + 8u));
    a72c_sw(dst + 12u, a72c_lw(src + 12u));
}

/* Ring entry address for index i (stride 0x14). */
static u32 a72c_ring_entry(u32 i)
{
    return A72C_RING_BASE + i * 0x14u;
}

/* ------------------------------------------------------------------ */
/* 32-entry ring refill (shared by JT1 slots 0x12 / 0x2A).  Retail:    */
/* a2 counts 0x1F..-1 with an s16-faithful test after the decrement.   */
/* ------------------------------------------------------------------ */

static void a72c_ring_fill(u32 src_vec, u32 src_head)
{
    u32 entry = A72C_RING_BASE;
    s32 counter = 0x1F;

    for (;;) {
        a72c_copy4(entry, src_vec);
        counter -= 1;
        a72c_sh(entry + 0x10u, a72c_lhu(src_head));
        if (a72c_sign16((u32)counter) == -1)
            break;
        entry += 0x14u;
    }
}

/* ------------------------------------------------------------------ */

s32 wm_8008A72C(s32 slot_idx)
{
    u32 pool = a72c_lw(A72C_POOL_PTR);
    u32 slot = pool + (u32)slot_idx * 128u;
    u32 sc = A72C_SCRATCH;
    s32 state;

    /* ---------------- transient substate (lh slot[+4]) -------------- */
    {
        s32 sub = (s32)a72c_lh(slot + 4u);

        if (sub == 3) {
#if defined(WM_8A72C_MUTANT_SUBSTATE_POLARITY)
            a72c_sh(slot + 4u, 0u);
            a72c_sh(slot + 0x20u, 0x28u);
#else
            a72c_sh(slot + 4u, 0u);
            a72c_sh(slot + 0x20u, 1u);
#endif
        } else if (sub == 2) {
#if defined(WM_8A72C_MUTANT_SUBSTATE_POLARITY)
            a72c_sh(slot + 4u, 0u);
            a72c_sh(slot + 0x20u, 1u);
#else
            a72c_sh(slot + 4u, 0u);
            a72c_sh(slot + 0x20u, 0x28u);
#endif
        } else if (sub == 6) {
            u32 lap = a72c_lw(slot + 0x58u) + 1u;
            u32 mode = a72c_lw(A72C_MODE_WORD);

            a72c_sh(slot + 4u, 0u);
            a72c_sw(slot + 0x58u, lap);
#if defined(WM_8A72C_MUTANT_LAP_GATE)
            if (mode != lap) {
#else
            if (mode == lap) {
#endif
                a72c_sh(slot + 0x20u, 0u);
                a72c_sw(A72C_MODE_FLAG, 1u);
                a72c_sh(A72C_BD04, 0u);
            }
        }
        /* other substates: fall through to dispatch */
    }

    /* ---------------- JT1 dispatch (lh slot[+0x20]) ------------------ */
    state = (s32)a72c_lh(slot + 0x20u);

#if defined(WM_8A72C_MUTANT_JT1_GUARD)
    if ((u32)state >= 0x2Du)
#else
    if ((u32)state >= 0x41u)
#endif
        goto common_tail;

    switch (state) {
    case 0x00:
    case 0x01: /* 0x8008A810: init/idle */
        if (a72c_lbu(A72C_RESYNC_BYTE) != 0u) {
            /* resync block 0x8008AB6C: follow slot 4, suppress 74794 */
            a72c_sh(slot + 0x24u, 1u);
            a72c_sw(slot + 0x28u, a72c_lw(pool + 0x228u));
            a72c_sw(slot + 0x2Cu, a72c_lw(pool + 0x22Cu));
            a72c_sw(slot + 0x30u, a72c_lw(pool + 0x230u));
            a72c_sh(slot + 0x48u, a72c_lhu(pool + 0x248u));
            A72C_CALL_894C8(0x2Fu);
            goto common_tail;
        }
        {
            s32 cls = A72C_CALL_90A84(slot);
#if defined(WM_8A72C_MUTANT_JT2_BIAS)
            u32 jt2 = (u32)cls;
#else
            u32 jt2 = (u32)(cls - 1);
#endif

            if (jt2 >= 5u) {
#if defined(WM_8A72C_MUTANT_JT2_OOR_POLARITY)
                if (slot == 0u)          /* keep the label referenced */
                    goto movement_block;
                goto pre_tail;
#else
                goto movement_block;
#endif
            }
            switch (jt2) {
            case 0: /* class 1 -> 0x8008A854 */
                a72c_sh(slot + 0x20u, 0x40u);
                a72c_sw(A72C_D554, 0u);
                a72c_sw(A72C_D7CC, 0u);
                goto pre_tail;
            case 2: /* class 3 -> 0x8008A874 */
                if (a72c_lbu(A72C_AREA_BYTE) == 7u) {
                    a72c_sh(slot + 0x20u, 8u);
                } else {
                    a72c_sb(A72C_AREA_BYTE, 4u);
                    a72c_sh(slot + 0x20u, 0x10u);
                }
                goto pre_tail;
            default: /* classes 2/4/5 -> 0x8008AB5C */
                goto pre_tail;
            }
        }

    case 0x02:
    case 0x03: /* 0x8008ABB4: follow slot-7 record */
        a72c_sw(slot + 0x28u, a72c_lw(pool + 0x3A8u));
        a72c_sw(slot + 0x2Cu, a72c_lw(pool + 0x3ACu));
        a72c_sw(slot + 0x30u, a72c_lw(pool + 0x3B0u));
        a72c_sh(slot + 0x48u, a72c_lhu(pool + 0x3C8u));
        goto common_tail;

    case 0x08: /* 0x8008ABF0: slot-2 handshake (state++ unconditional) */
        if (a72c_lbu(A72C_SLOT2_PRES) != 0xFFu) {
            if (a72c_lbu(A72C_BUSY2_BYTE) == 0u) {
#if defined(WM_8A72C_MUTANT_97770_ARGS)
                (void)A72C_CALL_97770(1u, 2);
#else
                (void)A72C_CALL_97770(2u, 1);
#endif
                a72c_sh(slot + 0x86u, a72c_lbu(A72C_AREA_BYTE));
                (void)A72C_CALL_97770(5u, 8);
            } else {
                (void)A72C_CALL_97770(5u, 1);
#if defined(WM_8A72C_MUTANT_NEIGHBOR_OFFSET)
                a72c_sh(pool + 0x282u, a72c_lbu(A72C_AREA_BYTE));
#else
                a72c_sh(pool + 0x286u, a72c_lbu(A72C_AREA_BYTE));
#endif
            }
        }
        state = (s32)a72c_lhu(slot + 0x20u) + 1;
        goto set_state;

    case 0x09: /* 0x8008AC70: slot-3 handshake (state++ unconditional) */
        if (a72c_lbu(A72C_SLOT3_PRES) != 0xFFu) {
            if (a72c_lbu(A72C_BUSY3_BYTE) == 0u) {
                (void)A72C_CALL_97770(3u, 1);
                a72c_sh(slot + 0x106u, a72c_lbu(A72C_AREA_BYTE));
                (void)A72C_CALL_97770(6u, 8);
            } else {
                (void)A72C_CALL_97770(6u, 1);
                a72c_sh(pool + 0x306u, a72c_lbu(A72C_AREA_BYTE));
            }
        }
        state = (s32)a72c_lhu(slot + 0x20u) + 1;
        goto set_state;

    case 0x0A: /* 0x8008ACF0: slot-4 claim */
        if (a72c_lbu(A72C_SLOT4_PRES) == 0xFFu) {
            state = 0xD;
            goto set_state;
        }
        if (A72C_CALL_97770(4u, 8) == 0)
            goto common_tail; /* hold */
        a72c_sh(slot + 0x20u, 0xDu);
        goto common_tail;

    case 0x0D: /* 0x8008AD24: approach setup */
    {
        u32 area = a72c_lbu(A72C_AREA_BYTE);
        u32 target = a72c_lw(A72C_POOL_PTR) + area * 128u;

        (void)A72C_CALL_941C4(slot + 0x28u, target + 0x28u,
                              slot + 0x38u, slot + 0x48u);
        area = a72c_lbu(A72C_AREA_BYTE);
        target = a72c_lw(A72C_POOL_PTR) + area * 128u;
        a72c_sw(slot + 0x50u, a72c_sra(a72c_lw(target + 0x28u), 12u));
        a72c_sw(slot + 0x54u, a72c_sra(a72c_lw(target + 0x30u), 12u));
        A72C_CALL_245D8(a72c_lw(slot + 0x4Cu), (s16)1);
        state = (s32)a72c_lhu(slot + 0x20u) + 1;
        goto set_state;
    }

    case 0x0E: /* 0x8008AD9C: approach step */
        if (A72C_CALL_8BEC8(slot) == 3) {
            state = (s32)a72c_lhu(slot + 0x20u) + 1;
            a72c_sh(slot + 0x20u, (u16)state);
        }
        a72c_copy4(A72C_POSE_BLOCK, slot + 0x28u);
        a72c_sh(A72C_HEAD_MIRROR, a72c_lhu(slot + 0x48u));
        A72C_CALL_8C1DC((u32)(slot_idx + 0x2E), slot, sc);
        goto common_tail;

    case 0x0F: /* 0x8008AE10: handoff */
        if (A72C_CALL_97770(a72c_lbu(A72C_AREA_BYTE), 4) == 0)
            goto common_tail; /* hold */
        a72c_sh(slot + 0x24u, 1u);
        a72c_sh(slot + 0x20u, 2u);
        A72C_CALL_894C8(0x2Fu);
        goto common_tail;

    case 0x10: /* 0x8008AE44: slot-2 re-claim */
        if (a72c_lbu(A72C_SLOT2_PRES) == 0xFFu) {
            state = (s32)a72c_lhu(slot + 0x20u) + 1;
            goto set_state;
        }
        if (A72C_CALL_97770(2u, 1) == 0)
            goto common_tail; /* hold */
        a72c_sh(slot + 0x86u, 5u);
        state = (s32)a72c_lhu(slot + 0x20u) + 1;
        goto set_state;

    case 0x11: /* 0x8008AE84: slot-3 re-claim */
        if (a72c_lbu(A72C_SLOT3_PRES) == 0xFFu) {
            state = (s32)a72c_lhu(slot + 0x20u) + 1;
            goto set_state;
        }
        if (A72C_CALL_97770(3u, 1) == 0)
            goto common_tail; /* hold */
        a72c_sh(slot + 0x106u, 6u);
        state = (s32)a72c_lhu(slot + 0x20u) + 1;
        goto set_state;

    case 0x12: /* 0x8008AEC4: teleport + ring refill */
    {
        u32 wx = (u32)a72c_lhu(A72C_WARP_X) << 12;
        u32 wz;
        s32 angle;

        a72c_sh(A72C_RING_INDEX, 0u);
        a72c_sw(sc + 0x30u, wx);
        wz = (u32)a72c_lhu(A72C_WARP_Z) << 12;
        a72c_sw(sc + 0x38u, wz);
        angle = A72C_CALL_93978(a72c_s32(a72c_lw(sc + 0x30u)), a72c_s32(wz));
        a72c_sw(sc + 0x34u, (u32)angle);
        a72c_sh(sc + 0xA0u, a72c_lhu(A72C_WARP_HEAD));
        /* 32 entries from scratch 0x30..0x3C (+0x3C is stale scratch,
         * copied faithfully) + heading at 0xA0. */
        a72c_ring_fill(sc + 0x30u, sc + 0xA0u);
        state = 0xD;
        goto set_state;
    }

    case 0x28: /* 0x8008AF5C: warp-in */
    {
        u32 wx = (u32)a72c_lhu(A72C_WARP_X) << 12;
        u32 wz;
        u32 head_raw;
        long c;
        long s;

        a72c_sw(slot + 0x28u, wx);
        wz = (u32)a72c_lhu(A72C_WARP_Z) << 12;
        a72c_sw(slot + 0x30u, wz);
        a72c_sw(slot + 0x2Cu,
                (u32)A72C_CALL_93978(a72c_s32(a72c_lw(slot + 0x28u)),
                                     a72c_s32(wz)));
        head_raw = a72c_lhu(A72C_WARP_HEAD);
        a72c_sw(slot + 0x5Cu, head_raw);
        a72c_sh(slot + 0x48u, (u16)head_raw);
        c = A72C_CALL_RCOS((long)a72c_sign16(head_raw));
        a72c_sw(sc + 0u, a72c_lw(slot + 0x28u) + (u32)c * 48u);
        s = A72C_CALL_RSIN((long)a72c_lh(slot + 0x48u));
        a72c_sw(sc + 8u, a72c_lw(slot + 0x30u) + (0u - (u32)s) * 48u);
        (void)A72C_CALL_941C4(slot + 0x28u, sc, slot + 0x38u, slot + 0x48u);
        a72c_sw(slot + 0x50u, a72c_sra(a72c_lw(sc + 0u), 12u));
        a72c_sh(slot + 0x24u, 0u);
        a72c_sw(slot + 0x54u, a72c_sra(a72c_lw(sc + 8u), 12u));
        A72C_CALL_245D8(a72c_lw(slot + 0x4Cu), (s16)1);
        state = (s32)a72c_lhu(slot + 0x20u) + 1;
        goto set_state;
    }

    case 0x29: /* 0x8008B034: warp approach */
        if (A72C_CALL_8BEC8(slot) == 3) {
            A72C_CALL_245D8(a72c_lw(slot + 0x4Cu), (s16)0);
            state = (s32)a72c_lhu(slot + 0x20u);
            a72c_sw(slot + 0x40u, 0u);
            a72c_sw(slot + 0x3Cu, 0u);
            a72c_sw(slot + 0x38u, 0u);
            a72c_sh(slot + 0x20u, (u16)(state + 1));
        }
        a72c_copy4(A72C_POSE_BLOCK, slot + 0x28u);
        a72c_sh(A72C_HEAD_MIRROR, a72c_lhu(slot + 0x48u));
        goto common_tail;

    case 0x2A: /* 0x8008B0AC: ring reset */
        a72c_sb(A72C_RESYNC_BYTE, 0u);
        a72c_sw(slot + 0x58u, 1u);
        a72c_sh(A72C_RING_INDEX, 0u);
        a72c_copy4(sc, slot + 0x28u);
        a72c_sh(sc + 0xA0u, a72c_lhu(slot + 0x48u));
        a72c_ring_fill(sc, sc + 0xA0u);
        state = (s32)a72c_lhu(slot + 0x20u) + 1;
        goto set_state;

    case 0x2B: /* 0x8008B14C: mode wait */
    {
        s32 mode = a72c_s32(a72c_lw(A72C_MODE_WORD));

        if (mode == 2) {
            if (a72c_lbu(A72C_BUSY2_BYTE) != 0u)
                goto common_tail; /* hold */
            state = (s32)a72c_lhu(slot + 0x20u) + 1;
            goto set_state;
        }
        if (mode == 1) {
            state = 1; /* retail: beq delay slot v0=1 -> state=1 */
            goto set_state;
        }
        if (mode == 3) {
            if ((a72c_lbu(A72C_BUSY2_BYTE) | a72c_lbu(A72C_BUSY3_BYTE)) != 0u)
                goto common_tail; /* hold */
            state = (s32)a72c_lhu(slot + 0x20u) + 1;
            goto set_state;
        }
        goto common_tail; /* hold (mode 0, negatives, >= 4) */
    }

    case 0x2C: /* 0x8008B1D8: slot-2 release */
        if (a72c_lbu(A72C_SLOT2_PRES) == 0xFFu) {
            state = (s32)a72c_lhu(slot + 0x20u) + 1;
            goto set_state;
        }
        if (A72C_CALL_97770(2u, 5) == 0)
            goto common_tail; /* hold */
        state = (s32)a72c_lhu(slot + 0x20u) + 1;
        goto set_state;

    case 0x2D: /* 0x8008B214: slot-3 release */
        if (a72c_lbu(A72C_SLOT3_PRES) == 0xFFu) {
#if defined(WM_8A72C_MUTANT_STATE2D_TARGET)
            state = (s32)a72c_lhu(slot + 0x20u) + 1;
#else
            state = 0x40;
#endif
            goto set_state;
        }
        if (A72C_CALL_97770(3u, 5) == 0)
            goto common_tail; /* hold */
#if defined(WM_8A72C_MUTANT_STATE2D_TARGET)
        state = (s32)a72c_lhu(slot + 0x20u) + 1;
#else
        state = 0x40;
#endif
        goto set_state;

    default:
        /* Parked no-op states: 0x04-0x07, 0x0B, 0x0C, 0x13-0x27,
         * 0x2E-0x40 (46 of 65 retail table slots -> 0x8008B240). */
        goto common_tail;
    }

    /* ------------------ movement block 0x8008A8A8 ------------------- */
movement_block:
    {
        u32 pos = slot + 0x28u;
        u32 vel = slot + 0x38u;
        u32 outv = sc + 0x90u;
        s32 r;

        if ((a72c_lw(slot + 0x38u) | a72c_lw(slot + 0x3Cu) |
             a72c_lw(slot + 0x40u)) == 0u) {
            /* velocity zero: idle anim */
            u32 obj = a72c_lw(slot + 0x4Cu);

            if (wm_native_animation_differs(obj, 0)) {
                A72C_CALL_245D8(obj, (s16)0);
                A72C_CALL_894C8(0x2Fu);
            }
        } else {
            u32 obj = a72c_lw(slot + 0x4Cu);

            if (wm_native_animation_differs(obj, 1))
                A72C_CALL_245D8(obj, (s16)1);
            A72C_CALL_8C1DC(0x2Fu, slot, sc);
        }

        r = A72C_CALL_95414(pos, vel, outv,
                            a72c_s32((u32)a72c_lh(slot + 0x4Au) << 12),
                            a72c_s32(a72c_lw(A72C_MODE_FLAG)));
#if defined(WM_8A72C_MUTANT_R_TEST_WIDTH)
        if (r == 0) {
#else
        if ((u16)(u32)r == 0u) {
#endif
            /* retry with the resolver's redirected velocity */
#if defined(WM_8A72C_MUTANT_RETRY_COPY_EXTENT)
            a72c_sw(slot + 0x38u, a72c_lw(sc + 0x90u));
            a72c_sw(slot + 0x3Cu, a72c_lw(sc + 0x94u));
#else
            a72c_sw(slot + 0x38u, a72c_lw(sc + 0x90u));
            a72c_sw(slot + 0x3Cu, a72c_lw(sc + 0x94u));
            a72c_sw(slot + 0x40u, a72c_lw(sc + 0x98u));
            a72c_sw(slot + 0x44u, a72c_lw(sc + 0x9Cu));
#endif
            r = A72C_CALL_95414(pos, vel, outv,
                                a72c_s32((u32)a72c_lh(slot + 0x4Au) << 12),
                                a72c_s32(a72c_lw(A72C_MODE_FLAG)));
            if ((u16)(u32)r == 0u) {
                /* final fallback zeroes ONLY +0x40 and +0x38 */
#if defined(WM_8A72C_MUTANT_ZERO_FALLBACK_EXTENT)
                a72c_sw(slot + 0x44u, 0u);
                a72c_sw(slot + 0x40u, 0u);
                a72c_sw(slot + 0x3Cu, 0u);
                a72c_sw(slot + 0x38u, 0u);
#else
                a72c_sw(slot + 0x40u, 0u);
                a72c_sw(slot + 0x38u, 0u);
#endif
            }
        }

        if (a72c_sign16((u32)r) == 1) {
            u32 area;

            A72C_CALL_8C040(outv, 0x10, 0x20, A72C_BOUND_BYTE,
                            A72C_AREA_BYTE);
            if (a72c_lbu(A72C_AREA_BYTE) == 7u)
#if defined(WM_8A72C_MUTANT_AREA_PLUS3)
                area = (u32)a72c_lbu(A72C_BOUND_BYTE) + 2u;
#else
                area = (u32)a72c_lbu(A72C_BOUND_BYTE) + 3u;
#endif
            else
                area = (u32)a72c_lbu(A72C_BOUND_BYTE);
            /* breadcrumb gate: sll 0x10 / sra 0xF halfword sign
             * extension of the area value, times 2 */
            {
                u32 off = a72c_sra(area << 16, 15u);
                u32 gate = a72c_lhu(A72C_CRUMB_TAB + off);

                if (gate != 0u) {
                    /* commit position */
#if defined(WM_8A72C_MUTANT_COMMIT_SET)
                    a72c_sw(slot + 0x28u, a72c_lw(sc + 0x90u));
                    a72c_sw(slot + 0x2Cu, a72c_lw(sc + 0x94u));
                    a72c_sw(slot + 0x30u, a72c_lw(sc + 0x98u));
#else
                    a72c_copy4(slot + 0x28u, sc + 0x90u);
#endif
                    if ((a72c_lw(slot + 0x38u) | a72c_lw(slot + 0x40u)) != 0u) {
                        /* breadcrumb ring append */
#if defined(WM_8A72C_MUTANT_RING_MASK)
                        u32 i = ((u32)a72c_lhu(A72C_RING_INDEX) + 1u) & 0x3Fu;
#else
                        u32 i = ((u32)a72c_lhu(A72C_RING_INDEX) + 1u) & 0x1Fu;
#endif
                        u32 entry = a72c_ring_entry(i);

                        a72c_sh(A72C_RING_INDEX, (u16)i);
                        a72c_copy4(entry, slot + 0x28u);
                        a72c_sh(entry + 0x10u, a72c_lhu(slot + 0x48u));
                        A72C_CALL_7528C();
                    }
                }
            }
        } else {
            A72C_CALL_8C040(slot + 0x28u, 0x10, 0x20, A72C_BOUND_BYTE,
                            A72C_AREA_BYTE);
        }

        (void)A72C_CALL_94238(slot + 0x28u, 0u);
        a72c_sw(slot + 0x40u, 0u);
        a72c_sw(slot + 0x3Cu, 0u);
        a72c_sw(slot + 0x38u, 0u);
        a72c_copy4(A72C_POSE_BLOCK, slot + 0x28u);
        a72c_sh(A72C_HEAD_MIRROR, a72c_lhu(slot + 0x48u));
        goto pre_tail;
    }

set_state:
    a72c_sh(slot + 0x20u, (u16)state);
    goto common_tail;

pre_tail: /* 0x8008AB5C */
    a72c_sh(A72C_BD04, 0u);
    a72c_sh(slot + 0x24u, 0u);
    /* fall through */

common_tail: /* 0x8008B240 */
#if defined(WM_8A72C_MUTANT_MIRROR_ADDR)
    a72c_sh(A72C_MIRROR_X, (u16)a72c_sra(a72c_lw(slot + 0x30u), 12u));
    a72c_sh(A72C_MIRROR_Z, (u16)a72c_sra(a72c_lw(slot + 0x28u), 12u));
#else
    a72c_sh(A72C_MIRROR_X, (u16)a72c_sra(a72c_lw(slot + 0x28u), 12u));
    a72c_sh(A72C_MIRROR_Z, (u16)a72c_sra(a72c_lw(slot + 0x30u), 12u));
#endif
    a72c_sh(A72C_MIRROR_HEAD, a72c_lhu(slot + 0x48u));
#if defined(WM_8A72C_MUTANT_74794_GATE)
    if (a72c_lh(slot + 0x24u) != 0)
#else
    if (a72c_lh(slot + 0x24u) == 0)
#endif
        A72C_CALL_74794(0, slot + 0x28u);
#if defined(WM_8A72C_MUTANT_RETURN_VARIABLE)
    return a72c_lh(slot + 0x20u);
#else
    return 1;
#endif
}
