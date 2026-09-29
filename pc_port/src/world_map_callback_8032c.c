/* Exact native transcription of retail [0x8008032C, 0x80080578). */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_8032c.h"
#include "world_map_helper_97770.h"

#define WM_M13S_POOL_PTR    UINT32_C(0x8009BE24)
#define WM_M13S_COMMANDS    UINT32_C(0x8009A698)
#define WM_M13S_TIMERS      UINT32_C(0x8009A6AC)
#define WM_M13S_GLOBAL_CCA4 UINT32_C(0x8009CCA4)
#define WM_M13S_GLOBAL_D3CC UINT32_C(0x8009D3CC)
#define WM_M13S_GLOBAL_D554 UINT32_C(0x8009D554)
#define WM_M13S_GLOBAL_D7CC UINT32_C(0x8009D7CC)

extern void *D_8006259C;
extern void func_80039E60(s32 packed_id);

static s16 m13s_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u16 m13s_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 m13s_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void m13s_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void m13s_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u32 m13s_slot(s32 slot_index)
{
    return m13s_lw(WM_M13S_POOL_PTR) + ((u32)slot_index << 7u);
}

static s32 m13s_sound_id(u16 id)
{
    u16 bank;
    u32 packed;

    memcpy(&bank, (const uint8_t *)D_8006259C + 0x14u, sizeof(bank));
    packed = ((u32)bank << 16u) | (u32)id;
    return (s32)packed;
}

s32 wm_8008032C(s32 slot_index)
{
    u32 slot = m13s_slot(slot_index);

    m13s_sw(slot + 0x50u, 0u);
    m13s_sh(slot + 0x20u, m13s_lhu(WM_M13S_COMMANDS));
#if defined(W34N90_MUTANT_WRONG_INITIAL_TIMER_TABLE)
    m13s_sh(slot + 0x22u, m13s_lhu(WM_M13S_COMMANDS));
#else
    m13s_sh(slot + 0x22u, m13s_lhu(WM_M13S_TIMERS));
#endif
#if defined(W34N90_MUTANT_ADVANCE_INITIAL_INDEX)
    m13s_sw(slot + 0x50u, 1u);
#endif
    return 1;
}

s32 wm_80080370(s32 slot_index)
{
    u32 slot = m13s_slot(slot_index);
    s16 command = m13s_lh(slot + 0x20u);

    switch (command) {
    case 1: {
        u16 timer = (u16)(m13s_lhu(slot + 0x22u) - 1u);
        u32 index;

        m13s_sh(slot + 0x22u, timer);
#if defined(W34N90_MUTANT_EXPIRE_TIMER_AT_ZERO)
        if ((s16)timer > 0)
#else
        if ((s16)timer >= 0)
#endif
            break;
        index = m13s_lw(slot + 0x50u);
        m13s_sh(slot + 0x20u,
                 m13s_lhu(WM_M13S_COMMANDS + index * 2u));
        m13s_sh(slot + 0x22u,
                 m13s_lhu(WM_M13S_TIMERS + index * 2u));
        m13s_sw(slot + 0x50u, index + 1u);
        break;
    }
    case 2:
#if defined(W34N90_MUTANT_WRONG_COMMAND2_CLAIM)
        (void)wm_80097770(3u, 2);
#else
        (void)wm_80097770(3u, 1);
#endif
        m13s_sh(slot + 0x20u, 1u);
        break;
    case 3:
        (void)wm_80097770(2u, 2);
        m13s_sh(slot + 0x20u, 1u);
        break;
    case 4:
        (void)wm_80097770(0u, 13);
        m13s_sw(WM_M13S_GLOBAL_CCA4, 1u);
        m13s_sw(WM_M13S_GLOBAL_D3CC, 128u);
        m13s_sh(slot + 0x20u, 1u);
        break;
    case 5:
        (void)wm_80097770(0u, 12);
#if !defined(W34N90_MUTANT_SKIP_COMMAND5_SECOND_CLAIM)
        (void)wm_80097770(4u, 1);
#endif
        m13s_sw(WM_M13S_GLOBAL_CCA4, 1u);
        m13s_sw(WM_M13S_GLOBAL_D3CC, 128u);
        m13s_sh(slot + 0x20u, 1u);
        break;
    case 6:
        (void)wm_80097770(2u, 3);
        m13s_sh(slot + 0x20u, 1u);
        break;
    case 7:
        (void)wm_80097770(0u, 13);
#if defined(W34N90_MUTANT_WRONG_COMMAND7_STATE)
        m13s_sw(WM_M13S_GLOBAL_CCA4, 1u);
#else
        m13s_sw(WM_M13S_GLOBAL_CCA4, 2u);
#endif
        m13s_sw(WM_M13S_GLOBAL_D3CC, 4u);
        m13s_sh(slot + 0x20u, 1u);
        break;
    case 8: {
        u16 first = 22u;

#if defined(W34N90_MUTANT_WRONG_SOUND_RANGE)
        first = 21u;
#endif
        func_80039E60(m13s_sound_id(first));
        func_80039E60(m13s_sound_id((u16)(first + 1u)));
        func_80039E60(m13s_sound_id((u16)(first + 2u)));
        m13s_sh(slot + 0x20u, 1u);
        break;
    }
    case 64:
#if !defined(W34N90_MUTANT_SKIP_COMMAND64_EXIT)
        m13s_sw(WM_M13S_GLOBAL_D554, 0u);
        m13s_sw(WM_M13S_GLOBAL_D7CC, 0u);
#endif
        m13s_sh(slot + 0x20u, 0u);
        break;
    default:
        break;
    }
    return 1;
}
