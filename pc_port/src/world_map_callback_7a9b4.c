/* Exact native transcription of retail [0x8007A9B4, 0x8007AD34). */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_7a9b4.h"
#include "world_map_common_tail.h"
#include "world_map_helper_97770.h"

extern void func_80039E18(s32 packed_id);
extern void func_80039E60(s32 packed_id);

#define WM_M14S_POOL_PTR    UINT32_C(0x8009BE24)
#define WM_M14S_SOUND_PTR   UINT32_C(0x8006259C)
#define WM_M14S_STATE_TABLE UINT32_C(0x8009A450)
#define WM_M14S_TIMER_TABLE UINT32_C(0x8009A46C)
#define WM_M14S_D3CC        UINT32_C(0x8009D3CC)
#define WM_M14S_D554        UINT32_C(0x8009D554)
#define WM_M14S_D7CC        UINT32_C(0x8009D7CC)

static s16 m14s_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u16 m14s_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 m14s_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void m14s_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void m14s_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static s16 m14s_as_s16(u16 bits)
{
    s16 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static u32 m14s_slot(s32 slot_index)
{
    return m14s_lw(WM_M14S_POOL_PTR) + ((u32)slot_index << 7u);
}

static s32 m14s_sound_id(u16 leaf)
{
    u32 manager = m14s_lw(WM_M14S_SOUND_PTR);
    u32 bits = ((u32)m14s_lhu(manager + 0x14u) << 16u) | (u32)leaf;
    s32 value;

    memcpy(&value, &bits, sizeof(value));
    return value;
}

static void m14s_claim(u32 slot, s32 value)
{
    (void)wm_80097770(slot, value);
}

s32 wm_8007A9B4(s32 slot_index)
{
    u32 slot = m14s_slot(slot_index);

    m14s_sw(slot + 0x50u, 0u);
    m14s_sh(slot + 0x20u, m14s_lhu(WM_M14S_STATE_TABLE));
#if defined(W34N71_MUTANT_WRONG_INITIAL_TIMER_TABLE)
    m14s_sh(slot + 0x22u, m14s_lhu(WM_M14S_STATE_TABLE));
#else
    m14s_sh(slot + 0x22u, m14s_lhu(WM_M14S_TIMER_TABLE));
#endif
    return 1;
}

s32 wm_8007A9F8(s32 slot_index)
{
    u32 slot = m14s_slot(slot_index);
    s16 state = m14s_lh(slot + 0x20u);

    if ((u32)(s32)state >= 12u)
        return 1;

    switch (state) {
    case 0:
        break;
    case 1: {
        u16 timer = (u16)(m14s_lhu(slot + 0x22u) - 1u);
        u32 sequence;

        m14s_sh(slot + 0x22u, timer);
#if defined(W34N71_MUTANT_ADVANCE_AT_ZERO)
        if (m14s_as_s16(timer) > 0)
#else
        if (m14s_as_s16(timer) >= 0)
#endif
            break;
        sequence = m14s_lw(slot + 0x50u);
#if defined(W34N71_MUTANT_PREINCREMENT_SEQUENCE)
        sequence++;
#endif
        m14s_sh(slot + 0x20u,
                 m14s_lhu(WM_M14S_STATE_TABLE + sequence * 2u));
        m14s_sh(slot + 0x22u,
                 m14s_lhu(WM_M14S_TIMER_TABLE + sequence * 2u));
#if !defined(W34N71_MUTANT_PREINCREMENT_SEQUENCE)
        sequence++;
#endif
        m14s_sw(slot + 0x50u, sequence);
        break;
    }
    case 2: {
        u32 sound_ptr = m14s_lw(WM_M14S_SOUND_PTR);
        u32 bits = (u32)m14s_lhu(sound_ptr + 0x14u) << 16u;
        s32 packed;

        wm_80089160(17u, 0u, 0u);
        m14s_sh(slot + 0x20u, 1u);
        bits |= 1u;
        memcpy(&packed, &bits, sizeof(packed));
        func_80039E60(packed);
        break;
    }
    case 3:
        m14s_claim(2u, 2);
        m14s_sh(slot + 0x20u, 1u);
#if defined(W34N71_MUTANT_WRONG_CASE3_MARKER)
        wm_80089160(14u, 0u, 0u);
#else
        wm_80089160(15u, 0u, 0u);
#endif
        wm_80089160(16u, 0u, 0u);
        func_80039E60(m14s_sound_id(4u));
        func_80039E60(m14s_sound_id(5u));
        func_80039E60(m14s_sound_id(6u));
        break;
    case 4:
        m14s_claim(2u, 3);
        m14s_claim(3u, 1);
        m14s_sh(slot + 0x20u, 1u);
        break;
    case 5:
        m14s_claim(2u, 6);
        m14s_claim(4u, 1);
        m14s_claim(5u, 1);
        m14s_sh(slot + 0x20u, 1u);
        func_80039E60(m14s_sound_id(7u));
        func_80039E60(m14s_sound_id(8u));
        func_80039E60(m14s_sound_id(9u));
        break;
    case 6:
        m14s_claim(2u, 4);
        m14s_sh(slot + 0x20u, 1u);
        func_80039E60(m14s_sound_id(10u));
#if defined(W34N71_MUTANT_WRONG_SOUND_FUNCTION)
        func_80039E60(m14s_sound_id(11u));
        func_80039E60(m14s_sound_id(12u));
#else
        func_80039E18(m14s_sound_id(11u));
        func_80039E18(m14s_sound_id(12u));
#endif
        break;
    case 7:
        m14s_claim(2u, 5);
        m14s_sh(slot + 0x20u, 1u);
        break;
    case 8:
        m14s_claim(2u, 1);
        m14s_sh(slot + 0x20u, 1u);
        break;
    case 9:
        m14s_claim(6u, 1);
        m14s_claim(2u, 7);
        m14s_sh(slot + 0x20u, 1u);
        break;
    case 10:
        m14s_claim(0u, 13);
        m14s_sw(WM_M14S_D3CC, 4u);
        m14s_sh(slot + 0x20u, 1u);
        break;
    case 11:
#if !defined(W34N71_MUTANT_SKIP_TERMINAL_D554)
        m14s_sw(WM_M14S_D554, 0u);
#endif
        m14s_sw(WM_M14S_D7CC, 0u);
        m14s_sh(slot + 0x20u, 1u);
        break;
    default:
        break;
    }
    return 1;
}
