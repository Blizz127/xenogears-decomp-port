/* Exact native transcription of retail [0x8007C36C, 0x8007C724). */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_7c36c.h"
#include "world_map_common_tail.h"
#include "world_map_helper_97770.h"

#define WM_M12S_POOL_PTR     UINT32_C(0x8009BE24)
#define WM_M12S_COMMANDS     UINT32_C(0x8009A4D8)
#define WM_M12S_TIMERS       UINT32_C(0x8009A4E8)
#define WM_M12S_WORLD_X      UINT32_C(0x8009BE28)
#define WM_M12S_WORLD_Z      UINT32_C(0x8009BE30)
#define WM_M12S_GLOBAL_CCA4  UINT32_C(0x8009CCA4)
#define WM_M12S_GLOBAL_D3CC  UINT32_C(0x8009D3CC)
#define WM_M12S_GLOBAL_D554  UINT32_C(0x8009D554)
#define WM_M12S_GLOBAL_D7CC  UINT32_C(0x8009D7CC)
#define WM_M12S_SC_VECTOR    UINT32_C(0x1F8000A0)

extern void *D_8006259C;
extern void func_80039E60(s32 packed_id);

static s16 m12s_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u16 m12s_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 m12s_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void m12s_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void m12s_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u32 m12s_slot(s32 slot_index)
{
    return m12s_lw(WM_M12S_POOL_PTR) + ((u32)slot_index << 7u);
}

static s32 m12s_sound_id(u16 id)
{
    u16 bank;
    u32 packed;

    memcpy(&bank, (const uint8_t *)D_8006259C + 0x14u, sizeof(bank));
    packed = ((u32)bank << 16u) | (u32)id;
    return (s32)packed;
}

static void m12s_play_three(u16 first)
{
    func_80039E60(m12s_sound_id(first));
    func_80039E60(m12s_sound_id((u16)(first + 1u)));
    func_80039E60(m12s_sound_id((u16)(first + 2u)));
}

s32 wm_8007C36C(s32 slot_index)
{
    u32 slot = m12s_slot(slot_index);
    u32 index = 0u;

    m12s_sw(slot + 0x50u, index);
    m12s_sh(slot + 0x20u, m12s_lhu(WM_M12S_COMMANDS));
#if defined(W34N85_MUTANT_WRONG_INITIAL_TIMER_TABLE)
    m12s_sh(slot + 0x22u, m12s_lhu(WM_M12S_COMMANDS));
#else
    m12s_sh(slot + 0x22u, m12s_lhu(WM_M12S_TIMERS));
#endif
    m12s_sw(slot + 0x50u, index + 1u);
    return 1;
}

s32 wm_8007C3B8(s32 slot_index)
{
    u32 slot = m12s_slot(slot_index);
    s16 command = m12s_lh(slot + 0x20u);

    switch (command) {
    case 1: {
        u16 timer = (u16)(m12s_lhu(slot + 0x22u) - 1u);
        u32 index;

        m12s_sh(slot + 0x22u, timer);
#if defined(W34N85_MUTANT_EXPIRE_TIMER_AT_ZERO)
        if ((s16)timer > 0)
#else
        if ((s16)timer >= 0)
#endif
            break;
        index = m12s_lw(slot + 0x50u);
        m12s_sh(slot + 0x20u,
                 m12s_lhu(WM_M12S_COMMANDS + index * 2u));
        m12s_sh(slot + 0x22u,
                 m12s_lhu(WM_M12S_TIMERS + index * 2u));
        m12s_sw(slot + 0x50u, index + 1u);
        break;
    }
    case 2:
#if defined(W34N85_MUTANT_WRONG_COMMAND2_CLAIM)
        (void)wm_80097770(2u, 2);
#else
        (void)wm_80097770(2u, 1);
#endif
        m12s_sh(slot + 0x20u, 1u);
        m12s_play_three(13u);
        break;
    case 16:
        m12s_sh(WM_M12S_SC_VECTOR + 2u, 0u);
        m12s_sh(WM_M12S_SC_VECTOR + 0u,
                 (u16)(m12s_lw(WM_M12S_WORLD_X) >> 12u));
        m12s_sh(WM_M12S_SC_VECTOR + 4u,
                 (u16)(m12s_lw(WM_M12S_WORLD_Z) >> 12u));
#if defined(W34N85_MUTANT_WRONG_COMMAND16_MARKER)
        wm_80089160(21u, WM_M12S_SC_VECTOR, 0u);
#else
        wm_80089160(20u, WM_M12S_SC_VECTOR, 0u);
#endif
        m12s_play_three(16u);
        m12s_sh(slot + 0x20u, 1u);
        break;
    case 17:
        (void)wm_80097770(2u, 2);
        (void)wm_80097770(0u, 13);
        m12s_sw(WM_M12S_GLOBAL_CCA4, 1u);
#if defined(W34N85_MUTANT_WRONG_COMMAND17_TIMER)
        m12s_sw(WM_M12S_GLOBAL_D3CC, 32u);
#else
        m12s_sw(WM_M12S_GLOBAL_D3CC, 64u);
#endif
        m12s_sh(slot + 0x20u, 1u);
        m12s_play_three(19u);
        break;
    case 18:
        (void)wm_80097770(2u, 3);
        (void)wm_80097770(3u, 1);
        (void)wm_80097770(4u, 1);
        (void)wm_80097770(5u, 1);
        (void)wm_80097770(6u, 1);
#if !defined(W34N85_MUTANT_SKIP_COMMAND18_SLOT7)
        (void)wm_80097770(7u, 1);
#endif
        (void)wm_80097770(0u, 12);
        m12s_sw(WM_M12S_GLOBAL_CCA4, 1u);
        m12s_sw(WM_M12S_GLOBAL_D3CC, 1u);
        m12s_sh(slot + 0x20u, 1u);
        break;
    case 19:
        (void)wm_80097770(2u, 4);
        (void)wm_80097770(9u, 1);
        m12s_sh(slot + 0x20u, 1u);
        break;
    case 20:
        (void)wm_80097770(2u, 6);
        (void)wm_80097770(3u, 2);
        (void)wm_80097770(9u, 2);
        m12s_sh(slot + 0x20u, 1u);
        break;
    case 22:
        (void)wm_80097770(0u, 13);
#if defined(W34N85_MUTANT_WRONG_COMMAND22_STATE)
        m12s_sw(WM_M12S_GLOBAL_CCA4, 1u);
#else
        m12s_sw(WM_M12S_GLOBAL_CCA4, 2u);
#endif
        m12s_sw(WM_M12S_GLOBAL_D3CC, 4u);
        m12s_sh(slot + 0x20u, 1u);
        break;
    case 64:
#if !defined(W34N85_MUTANT_SKIP_COMMAND64_EXIT)
        m12s_sw(WM_M12S_GLOBAL_D554, 0u);
        m12s_sw(WM_M12S_GLOBAL_D7CC, 0u);
#endif
        m12s_sh(slot + 0x20u, 1u);
        break;
    default:
        break;
    }
    return 1;
}
