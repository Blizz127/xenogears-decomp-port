/* Exact native transcription of retail [0x80081174, 0x800813E8). */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_81174.h"
#include "world_map_helper_97770.h"

#define WM_M16S_POOL_PTR    UINT32_C(0x8009BE24)
#define WM_M16S_COMMANDS    UINT32_C(0x8009A6C0)
#define WM_M16S_TIMERS      UINT32_C(0x8009A70C)
#define WM_M16S_GLOBAL_CCA4 UINT32_C(0x8009CCA4)
#define WM_M16S_GLOBAL_D3CC UINT32_C(0x8009D3CC)
#define WM_M16S_GLOBAL_D554 UINT32_C(0x8009D554)
#define WM_M16S_GLOBAL_D7CC UINT32_C(0x8009D7CC)

extern void *D_80062528;
extern void *D_8006259C;
extern void func_80039E60(s32 packed_id);
extern void func_8003A89C(void *manager, s32 level, s32 steps);

static s16 m16s_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u16 m16s_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 m16s_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void m16s_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void m16s_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static s16 m16s_as_s16(u16 bits)
{
    s16 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static u32 m16s_slot(s32 slot_index)
{
    return m16s_lw(WM_M16S_POOL_PTR) + ((u32)slot_index << 7u);
}

static s32 m16s_sound_id(u16 id)
{
    u16 bank;
    u32 bits;
    s32 value;

    memcpy(&bank, (const uint8_t *)D_8006259C + 0x14u, sizeof(bank));
    bits = ((u32)bank << 16u) | (u32)id;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static void m16s_claim(u32 slot, s32 value)
{
    (void)wm_80097770(slot, value);
}

s32 wm_80081174(s32 slot_index)
{
    u32 slot = m16s_slot(slot_index);
    u32 index = 0u;

    m16s_sw(slot + 0x50u, index);
    m16s_sh(slot + 0x20u, m16s_lhu(WM_M16S_COMMANDS));
#if defined(W34N103_MUTANT_WRONG_INITIAL_TIMER_TABLE)
    m16s_sh(slot + 0x22u, m16s_lhu(WM_M16S_COMMANDS));
#else
    m16s_sh(slot + 0x22u, m16s_lhu(WM_M16S_TIMERS));
#endif
#if !defined(W34N103_MUTANT_SKIP_INITIAL_INDEX_ADVANCE)
    m16s_sw(slot + 0x50u, index + 1u);
#endif
    return 1;
}

s32 wm_800811C0(s32 slot_index)
{
    u32 slot = m16s_slot(slot_index);
    s16 command = m16s_lh(slot + 0x20u);

    if ((u32)(s32)command >= 65u)
        return 1;

    switch (command) {
    case 1: {
        u16 timer = (u16)(m16s_lhu(slot + 0x22u) - 1u);
        u32 index;

        m16s_sh(slot + 0x22u, timer);
#if defined(W34N103_MUTANT_EXPIRE_TIMER_AT_ZERO)
        if (m16s_as_s16(timer) > 0)
#else
        if (m16s_as_s16(timer) >= 0)
#endif
            break;
        index = m16s_lw(slot + 0x50u);
        m16s_sh(slot + 0x20u,
                m16s_lhu(WM_M16S_COMMANDS + index * 2u));
        m16s_sh(slot + 0x22u,
                m16s_lhu(WM_M16S_TIMERS + index * 2u));
        m16s_sw(slot + 0x50u, index + 1u);
        break;
    }
    case 2:
        m16s_claim(6u, 1);
        m16s_sh(slot + 0x20u, 1u);
        break;
    case 3:
#if defined(W34N103_MUTANT_WRONG_COMMAND3_CLAIM)
        m16s_claim(6u, 1);
#else
        m16s_claim(6u, 0);
#endif
        m16s_sh(slot + 0x20u, 1u);
        break;
    case 4:
        m16s_claim(6u, 2);
        m16s_sh(slot + 0x20u, 1u);
        break;
    case 5:
        m16s_claim(6u, 3);
        m16s_sh(slot + 0x20u, 1u);
        break;
    case 6:
        m16s_claim(6u, 4);
        m16s_sh(slot + 0x20u, 1u);
        break;
    case 7:
        m16s_claim(6u, 5);
        m16s_sh(slot + 0x20u, 1u);
        break;
    case 8: {
        u16 first = 46u;

#if defined(W34N103_MUTANT_WRONG_SOUND_RANGE)
        first = 45u;
#endif
        func_80039E60(m16s_sound_id(first));
        func_80039E60(m16s_sound_id((u16)(first + 1u)));
        func_80039E60(m16s_sound_id((u16)(first + 2u)));
        m16s_sh(slot + 0x20u, 1u);
        break;
    }
    case 16:
        m16s_claim(3u, 1);
#if !defined(W34N103_MUTANT_SKIP_COMMAND16_SECOND_CLAIM)
        m16s_claim(4u, 1);
#endif
        m16s_sh(slot + 0x20u, 1u);
        break;
    case 17:
        m16s_claim(3u, 2);
        m16s_sh(slot + 0x20u, 1u);
        break;
    case 63:
#if defined(W34N103_MUTANT_WRONG_COMMAND63_FADE)
        func_8003A89C(D_80062528, 0, 120);
#else
        func_8003A89C(D_80062528, 0, 240);
#endif
        m16s_claim(0u, 13);
        m16s_sw(WM_M16S_GLOBAL_CCA4, 2u);
        m16s_sw(WM_M16S_GLOBAL_D3CC, 4u);
        m16s_sh(slot + 0x20u, 1u);
        break;
    case 64:
#if !defined(W34N103_MUTANT_SKIP_COMMAND64_EXIT)
        m16s_sw(WM_M16S_GLOBAL_D554, 0u);
        m16s_sw(WM_M16S_GLOBAL_D7CC, 0u);
#endif
        m16s_sh(slot + 0x20u, 0u);
        break;
    default:
        break;
    }
    return 1;
}
