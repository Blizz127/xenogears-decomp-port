/*
 * Retail state-2 slice of wm_8008E76C.
 *
 * Owned retail regions:
 *   state body    [0x8008EB64, 0x8008EE78)
 *   stop arm      [0x8008FAAC, 0x8008FAC4)
 *
 * The shared tail [0x80090620, 0x800906B4) is owned by
 * world_map_vehicle_tail_90620.c.  The parent callback remains only partially
 * transcribed; keeping this state in a bounded unit makes its retail behavior
 * independently certifiable.
 */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_common_tail.h"
#include "world_map_helper_74794.h"
#include "world_map_helper_8c040.h"
#include "world_map_helper_8e034.h"
#include "world_map_helper_8e078.h"
#include "world_map_helper_8e0f0.h"
#include "world_map_helper_90fb4.h"
#include "world_map_helper_93f18.h"
#include "world_map_helper_94060.h"
#include "world_map_helper_94238.h"
#include "world_map_helper_95cd4.h"
#include "world_map_helper_97770.h"
#include "world_map_state2_8eb64.h"
#include "world_map_terrain_sampler.h"
#include "world_map_vehicle_tail_90620.h"

#define WM_S2_SCRATCH       UINT32_C(0x1F800000)
#define WM_S2_MODE          UINT32_C(0x8009BE10)
#define WM_S2_CONTEXT_PTR   UINT32_C(0x8009C620)
#define WM_S2_MATRIX_Z      UINT32_C(0x8009BD3C)
#define WM_S2_AREA          UINT32_C(0x8009BD60)
#define WM_S2_BOUNDARY      UINT32_C(0x8009D738)
#define WM_S2_TRIGGER       UINT32_C(0x8009BD04)
#define WM_S2_POSITION      UINT32_C(0x8009D55C)
#define WM_S2_HEADING       UINT32_C(0x8009D52C)
#define WM_S2_STOP_FRAME    UINT32_C(0x8009D554)
#define WM_S2_EXIT_CODE     UINT32_C(0x8009D7CC)
#define WM_S2_TRIGGER_PTR   UINT32_C(0x8009D7D8)
#define WM_S2_TRIGGER_A     UINT32_C(0x8009BD24)
#define WM_S2_TRIGGER_B     UINT32_C(0x8009CE68)
#define WM_S2_NATIVE_HEAD   UINT32_C(0x8006EE66)

#define WM_S2_OUT           (WM_S2_SCRATCH + UINT32_C(0x90))
#define WM_S2_ANGLES        (WM_S2_SCRATCH + UINT32_C(0xA0))
#define WM_S2_SHORT_POS     (WM_S2_SCRATCH + UINT32_C(0xA8))

extern void *D_80062528;
extern void func_8003A89C(void *manager, s32 level, s32 steps);

static u8 s2_lbu(u32 address)
{
    return *(const u8 *)PSX_ADDR(address);
}

static u16 s2_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 s2_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static s32 s2_as_s32(u32 bits)
{
    s32 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static void s2_sb(u32 address, u8 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void s2_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void s2_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u32 s2_sra(u32 bits, u32 amount)
{
    u32 value = bits >> amount;

    if ((bits & UINT32_C(0x80000000)) != 0u)
        value |= UINT32_MAX << (32u - amount);
    return value;
}

static u32 s2_abs_difference(u32 lhs, u32 rhs)
{
    u32 difference = lhs - rhs;

    if (s2_as_s32(difference) < 0)
        difference = 0u - difference;
    return difference;
}

static void s2_copy_words(u32 destination, u32 source, u32 count)
{
    u32 index;

    for (index = 0u; index < count; index++)
        s2_sw(destination + index * 4u, s2_lw(source + index * 4u));
}

/* Retail 0x8008EC34..0x8008EC48: the transition arm clears only the area
 * byte, the boundary byte and the trigger halfword.  The velocity words
 * survive the transition. */
static void s2_clear_transition_state(void)
{
    s2_sb(WM_S2_AREA, 0u);
    s2_sb(WM_S2_BOUNDARY, 0u);
    s2_sh(WM_S2_TRIGGER, 0u);
}

/* Retail 0x8008EE68..0x8008EC48: the walking arm clears the three velocity
 * words and the trigger halfword, then joins the shared tail.  It leaves the
 * area and boundary bytes written by wm_8008C040 for later slots. */
static void s2_clear_walking_state(u32 slot)
{
    s2_sw(slot + 0x40u, 0u);
    s2_sw(slot + 0x3Cu, 0u);
    s2_sw(slot + 0x38u, 0u);
    s2_sh(WM_S2_TRIGGER, 0u);
}

static void s2_transition(u32 slot, s32 angle)
{
#if defined(W34N123_MUTANT_M2_WRONG_TRANSITION_STATE)
    s2_sh(slot + 0x20u, UINT16_C(32));
#else
    s2_sh(slot + 0x20u, UINT16_C(16));
#endif
    s2_sw(slot + 0x78u, (u32)angle);
    s2_sw(slot + 0x68u,
          (u32)wm_80093978(s2_as_s32(s2_lw(slot + 0x28u)),
                            s2_as_s32(s2_lw(slot + 0x30u))));
    (void)wm_80097770(11u, 10);
    func_8003A89C(D_80062528, 0, 240);
    s2_sw(slot + 0x7Cu, 1u);
    wm_800894C8(60u);
    wm_800894C8(63u);
    s2_sw(WM_S2_TRIGGER_PTR, UINT32_MAX);
    s2_sh(WM_S2_TRIGGER_A, UINT16_MAX);
    s2_sh(WM_S2_TRIGGER_B, UINT16_MAX);
    s2_clear_transition_state();
}

static void s2_build_matrices(u32 slot)
{
    u32 context = s2_lw(WM_S2_CONTEXT_PTR);
    u32 pitch = s2_sra(s2_lw(slot + 0x70u), 12u);

    s2_sh(WM_S2_ANGLES + 0u, (u16)(0u - pitch));
    s2_sh(WM_S2_ANGLES + 2u, s2_lhu(slot + 0x48u));
    s2_sh(WM_S2_ANGLES + 4u, s2_lhu(WM_S2_MATRIX_Z));
    (void)RotMatrixYXZ((SVECTOR *)PSX_ADDR(WM_S2_ANGLES),
                       (MATRIX *)PSX_ADDR(context + 0x20u));
#if !defined(W34N123_MUTANT_M8_SKIP_SECOND_MATRIX)
    (void)RotMatrixYXZ((SVECTOR *)PSX_ADDR(WM_S2_ANGLES),
                       (MATRIX *)PSX_ADDR(context + 0x74u));
#endif
}

static void s2_update_presence(u32 slot)
{
    u32 terrain = (u32)wm_80093978(s2_as_s32(s2_lw(slot + 0x28u)),
                                    s2_as_s32(s2_lw(slot + 0x30u)));
    u32 difference = s2_abs_difference(terrain, s2_lw(slot + 0x2Cu));
    s32 region;

    if (difference > UINT32_C(0x0005FFFF) || s2_lw(slot + 0x60u) == 0u) {
        wm_800894C8(60u);
        wm_800894C8(63u);
        return;
    }

    s2_sh(WM_S2_SHORT_POS + 0u,
          (u16)s2_sra(s2_lw(slot + 0x28u), 12u));
    s2_sh(WM_S2_SHORT_POS + 2u,
          (u16)s2_sra(s2_lw(slot + 0x2Cu), 12u));
    s2_sh(WM_S2_SHORT_POS + 4u,
          (u16)s2_sra(s2_lw(slot + 0x30u), 12u));
    region = (s32)(s16)(u16)wm_80093F18(slot + 0x28u);

    if (region == 2) {
#if defined(W34N123_MUTANT_M9_WRONG_PRESENCE_RECORD)
        wm_80089160(60u, WM_S2_SHORT_POS, WM_S2_ANGLES);
#else
        wm_80089160(63u, WM_S2_SHORT_POS, WM_S2_ANGLES);
#endif
        wm_800894C8(60u);
    } else if (region == 3) {
        wm_80089160(60u, WM_S2_SHORT_POS, WM_S2_ANGLES);
        wm_800894C8(63u);
    } else {
        wm_800894C8(60u);
        wm_800894C8(63u);
    }
}

s32 wm_8008E76C_state2(u32 slot)
{
    s32 control = wm_80090FB4(slot);

    if (control == 1) {
        s2_sw(WM_S2_STOP_FRAME, 0u);
#if !defined(W34N123_MUTANT_M1_SKIP_EXIT_CODE_CLEAR)
        s2_sw(WM_S2_EXIT_CODE, 0u);
#endif
        wm_8008E76C_shared_tail(slot);
        return 1;
    }

    if (control == 4) {
        s32 region = (s32)(s16)(u16)wm_80093F18(slot + 0x28u);

#if defined(W34N123_MUTANT_M3_WRONG_WALKABILITY_MODE)
        s32 walkable = wm_80094060(1, region);
#else
        s32 walkable = wm_80094060(2, region);
#endif
        if ((u32)walkable << 16u != 0u) {
#if defined(W34N123_MUTANT_M4_WRONG_ANGLE_OUTPUT)
            s32 angle = wm_8008E0F0(slot + 0x28u, slot + 0x38u,
                                    WM_S2_OUT);
#else
            s32 angle = wm_8008E0F0(slot + 0x28u, slot + 0x38u,
                                    UINT32_C(0x00068000));
#endif
            if (angle != -1)
                s2_transition(slot, angle);
        }
        wm_8008E76C_shared_tail(slot);
        return 1;
    }

    {
        s32 movement;

#if defined(W34N123_MUTANT_M5_WRONG_MOVEMENT_MODE)
        movement = wm_80095CD4(slot + 0x28u, slot + 0x38u, WM_S2_OUT,
                               s2_as_s32(s2_lw(slot + 0x60u)), 0);
#else
        movement = wm_80095CD4(slot + 0x28u, slot + 0x38u, WM_S2_OUT,
                               s2_as_s32(s2_lw(slot + 0x60u)),
                               s2_as_s32(s2_lw(WM_S2_MODE)));
#endif
        if (movement == 0) {
            s2_sw(slot + 0x60u, 0u);
            s2_sw(slot + 0x40u, 0u);
            s2_sw(slot + 0x38u, 0u);
        }

#if defined(W34N123_MUTANT_M6_COPY_ANY_NONZERO_RESULT)
        if (movement != 0) {
#else
        if (movement == 1) {
#endif
            wm_8008C040(WM_S2_OUT, 64, 32, WM_S2_BOUNDARY, WM_S2_AREA);
            if (s2_lbu(WM_S2_BOUNDARY) != 2u)
                s2_copy_words(slot + 0x28u, WM_S2_OUT, 4u);
            else
                s2_sw(slot + 0x60u, 0u);
        }
    }

    s2_copy_words(WM_S2_POSITION, slot + 0x28u, 4u);
    s2_sh(WM_S2_HEADING, s2_lhu(slot + 0x48u));
#if defined(W34N123_MUTANT_M7_WRONG_TRIGGER_LIST)
    (void)wm_80094238(slot + 0x28u, 1u);
#else
    (void)wm_80094238(slot + 0x28u, 2u);
#endif
    wm_8008E078();
    s2_build_matrices(slot);
    s2_update_presence(slot);
    s2_clear_walking_state(slot);
    wm_8008E76C_shared_tail(slot);
    return 1;
}
