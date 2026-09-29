/* Retail mode-10 camera/event callbacks [0x80078E2C, 0x800794D8). */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_78e2c.h"
#include "world_map_helper_96f18.h"
#include "world_map_helper_97770.h"

extern void* D_8006259C;
extern void func_80039E60(s32 packed_id);
extern void func_8003A3B8(s32 packed_id, s32 pitch, s32 steps);
extern int rand(void);

#define WM_M10E_POOL_PTR      0x8009BE24u
#define WM_M10E_GEOM_Y        0x8009BE0Cu
#define WM_M10E_ANGLES        0x8009BD38u
#define WM_M10E_CAMERA_INPUT  0x8009BD40u
#define WM_M10E_POSITION      0x8009BE28u
#define WM_M10E_MODE_STATE    0x8009D144u
#define WM_M10E_HEIGHT        0x8009D3F0u
#define WM_M10E_CCA4          0x8009CCA4u
#define WM_M10E_D3CC          0x8009D3CCu
#define WM_M10E_D554          0x8009D554u
#define WM_M10E_D7CC          0x8009D7CCu

static s16 m10e_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u16 m10e_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 m10e_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void m10e_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void m10e_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static s16 m10e_as_s16(u16 bits)
{
    s16 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static s32 m10e_as_s32(u32 bits)
{
    s32 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static u32 m10e_slot(s32 slot_index)
{
    return m10e_lw(WM_M10E_POOL_PTR) + ((u32)slot_index << 7u);
}

static u16 m10e_decrement_timer(u32 slot)
{
    u16 timer = (u16)(m10e_lhu(slot + 0x22u) - 1u);
    m10e_sh(slot + 0x22u, timer);
    return timer;
}

static void m10e_advance_state(u32 slot)
{
    m10e_sh(slot + 0x20u, (u16)(m10e_lhu(slot + 0x20u) + 1u));
}

static s32 m10e_sound_id(u16 low)
{
    u16 bank;
    u32 bits;
    memcpy(&bank, (const uint8_t*)D_8006259C + 0x14u, sizeof(bank));
    bits = ((u32)bank << 16u) | (u32)low;
    return m10e_as_s32(bits);
}

static void m10e_add_half(u32 address, s32 delta)
{
    u32 sum = (u32)m10e_lhu(address) + (u32)delta;
    m10e_sh(address, (u16)sum);
}

static s32 m10e_random_delta(u32 modulus, s32 bias)
{
    return (s32)((u32)rand() % modulus) - bias;
}

static void m10e_build_view(void)
{
    wm_80096F18(WM_M10E_CAMERA_INPUT, WM_M10E_POSITION,
                 m10e_as_s32(m10e_lw(WM_M10E_HEIGHT)), WM_M10E_ANGLES);
}

/* 0x80078E2C (init) is the matched C func_80078E2C (src/world_map/main.c). */

static void m10e_state0(u32 slot)
{
    if (m10e_as_s16(m10e_decrement_timer(slot)) > 0)
        return;
    m10e_sh(slot + 0x22u, 32u);
    m10e_advance_state(slot);
#if !defined(W34N64_MUTANT_SKIP_STATE0_CLAIM)
    (void)wm_80097770(5u, 1);
#endif
    func_80039E60(m10e_sound_id(0x0062u));
    func_80039E60(m10e_sound_id(0x0063u));
}

static void m10e_state1(u32 slot)
{
    if (m10e_as_s16(m10e_decrement_timer(slot)) > 0)
        return;
    m10e_sh(slot + 0x22u, 32u);
    m10e_advance_state(slot);
    (void)wm_80097770(3u, 1);
}

static void m10e_state2(u32 slot)
{
    if (m10e_as_s16(m10e_decrement_timer(slot)) > 0)
        return;
    m10e_sh(slot + 0x22u, 48u);
    m10e_advance_state(slot);
    (void)wm_80097770(4u, 1);
    (void)wm_80097770(2u, 1);
#if !defined(W34N64_MUTANT_SKIP_STATE2_LAST_CLAIM)
    (void)wm_80097770(6u, 1);
#endif
}

static void m10e_state3(u32 slot)
{
    if (m10e_as_s16(m10e_decrement_timer(slot)) > 0)
        return;
    m10e_sh(slot + 0x22u, 40u);
    m10e_advance_state(slot);
}

static void m10e_apply_jitter(u32 modulus, s32 bias)
{
    s32 delta_x = m10e_random_delta(modulus, bias);
    s32 delta_y = m10e_random_delta(modulus, bias);

    m10e_add_half(WM_M10E_CAMERA_INPUT + 0u, delta_x);
#if !defined(W34N64_MUTANT_SKIP_MIRRORED_ANGLE)
    m10e_add_half(WM_M10E_CAMERA_INPUT + 8u, delta_x);
#endif
    m10e_add_half(WM_M10E_CAMERA_INPUT + 2u, delta_y);
    m10e_add_half(WM_M10E_CAMERA_INPUT + 0x0Au, delta_y);
}

static void m10e_state4(u32 slot)
{
    m10e_build_view();
    (void)m10e_decrement_timer(slot);
#if defined(W34N64_MUTANT_WRONG_STATE4_MODULUS)
    m10e_apply_jitter(8u, 4);
#else
    m10e_apply_jitter(12u, 6);
#endif
    if (m10e_lh(slot + 0x22u) > 0)
        return;
    m10e_sh(slot + 0x22u, 120u);
    m10e_advance_state(slot);
    func_80039E60(m10e_sound_id(0x0079u));
    func_8003A3B8(m10e_sound_id(0x0062u), 0, 256);
    func_8003A3B8(m10e_sound_id(0x0063u), 0, 256);
}

static void m10e_state5(u32 slot)
{
    m10e_build_view();
    (void)m10e_decrement_timer(slot);
    m10e_apply_jitter(4u, 2);
    if (m10e_lh(slot + 0x22u) > 0)
        return;
    m10e_sh(slot + 0x22u, 90u);
    m10e_advance_state(slot);
}

static void m10e_state6(u32 slot)
{
    if (m10e_as_s16(m10e_decrement_timer(slot)) > 0)
        return;
    (void)wm_80097770(0u, 13);
    m10e_sh(slot + 0x22u, 80u);
    m10e_advance_state(slot);
    func_8003A3B8(m10e_sound_id(0x0079u), 0, 256);
#if defined(W34N64_MUTANT_WRONG_STATE6_GLOBALS)
    m10e_sw(WM_M10E_CCA4, 1u);
    m10e_sw(WM_M10E_D3CC, 8u);
#else
    m10e_sw(WM_M10E_CCA4, 2u);
    m10e_sw(WM_M10E_D3CC, 4u);
#endif
}

static void m10e_state7(u32 slot)
{
    if (m10e_as_s16(m10e_decrement_timer(slot)) > 0)
        return;
    m10e_sh(slot + 0x22u, 0u);
    m10e_sw(WM_M10E_D554, 0u);
    m10e_sw(WM_M10E_D7CC, 0u);
}

static void m10e_state16(u32 slot)
{
#if defined(W34N64_MUTANT_IGNORE_STATE16_FLAG)
    if (m10e_lh(slot + 4u) == 1)
        return;
#else
    if (m10e_lh(slot + 4u) != 1)
        return;
#endif
    func_8003A3B8(m10e_sound_id(0x0036u), 0, 8);
    (void)wm_80097770(0u, 13);
    m10e_sw(WM_M10E_D3CC, 32u);
    m10e_sw(WM_M10E_CCA4, 1u);
    m10e_sh(slot + 0x22u, 8u);
    m10e_sh(slot + 4u, 0u);
    m10e_advance_state(slot);
}

static void m10e_state17(u32 slot)
{
    if (m10e_as_s16(m10e_decrement_timer(slot)) > 0)
        return;
    m10e_sw(WM_M10E_HEIGHT, 0x00960000u);
    m10e_sh(WM_M10E_ANGLES + 0u, (u16)-48);
    m10e_sh(WM_M10E_ANGLES + 2u, 64u);
    m10e_sh(WM_M10E_ANGLES + 4u, 0u);
    m10e_sh(slot + 0x22u, 16u);
    m10e_advance_state(slot);
}

static void m10e_state18(u32 slot)
{
    if (m10e_as_s16(m10e_decrement_timer(slot)) > 0)
        return;
    (void)wm_80097770(0u, 12);
    m10e_sw(WM_M10E_CCA4, 1u);
    m10e_sw(WM_M10E_D3CC, 128u);
#if defined(W34N64_MUTANT_WRONG_LOOP_RESET)
    m10e_sh(slot + 0x20u, 1u);
#else
    m10e_sh(slot + 0x20u, 0u);
#endif
    m10e_sh(slot + 0x22u, 1u);
}

s32 wm_80078EA4(s32 slot_index)
{
    u32 slot = m10e_slot(slot_index);
    s16 state = m10e_lh(slot + 0x20u);

    switch (state) {
    case 0: m10e_state0(slot); break;
    case 1: m10e_state1(slot); break;
    case 2: m10e_state2(slot); break;
    case 3: m10e_state3(slot); break;
    case 4: m10e_state4(slot); break;
    case 5: m10e_state5(slot); break;
    case 6: m10e_state6(slot); break;
    case 7: m10e_state7(slot); break;
    case 16: m10e_state16(slot); break;
    case 17: m10e_state17(slot); break;
    case 18: m10e_state18(slot); break;
    default: break;
    }

    state = m10e_lh(slot + 0x20u);
#if defined(W34N64_MUTANT_DOUBLE_VIEW_IN_JITTER_STATES)
    m10e_build_view();
#else
    if (state != 4 && state != 5)
        m10e_build_view();
#endif
    return 1;
}
