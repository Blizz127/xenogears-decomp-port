/* Exact native transcription of retail [0x8007DE14,0x8007E450), with
 * direct leaf dependency [0x80089514,0x80089580). */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_7de14.h"
#include "world_map_helper_97770.h"

#define WM_M15S_POOL_PTR        UINT32_C(0x8009BE24)
#define WM_M15S_SELECTOR        UINT32_C(0x8009D3D4)
#define WM_M15S_SCRIPT_TABLES   UINT32_C(0x8009A65C)
#define WM_M15S_SOUND_TABLE     UINT32_C(0x8009A5A0)
#define WM_M15S_PARTICLE_POOL   UINT32_C(0x8009BDF4)
#define WM_M15S_GLOBAL_CCA4     UINT32_C(0x8009CCA4)
#define WM_M15S_GLOBAL_D3CC     UINT32_C(0x8009D3CC)
#define WM_M15S_GLOBAL_D554     UINT32_C(0x8009D554)
#define WM_M15S_GLOBAL_D7CC     UINT32_C(0x8009D7CC)

#define WM_M15S_PARTICLE_COUNT  UINT32_C(256)
#define WM_M15S_PARTICLE_STRIDE UINT32_C(0x4C)

extern void *D_8006259C;
extern void func_80039E60(s32 packed_id);

static s16 m15s_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u16 m15s_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 m15s_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void m15s_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void m15s_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u32 m15s_slot(s32 slot_index)
{
    return m15s_lw(WM_M15S_POOL_PTR) + ((u32)slot_index << 7u);
}

static s32 m15s_sound_id(u16 id)
{
    u16 bank;
    memcpy(&bank, (const uint8_t *)D_8006259C + 0x14u, sizeof(bank));
    return (s32)(((u32)bank << 16u) | (u32)id);
}

static void m15s_play_fixed_three(u16 first)
{
    func_80039E60(m15s_sound_id(first));
    func_80039E60(m15s_sound_id((u16)(first + 1u)));
    func_80039E60(m15s_sound_id((u16)(first + 2u)));
}

static void m15s_claim(u32 slot, s32 value)
{
    (void)wm_80097770(slot, value);
}

static void m15s_command_done(u32 slot)
{
    m15s_sh(slot + 0x20u, 1u);
}

void wm_80089514(u32 group_index)
{
    u32 record = m15s_lw(WM_M15S_PARTICLE_POOL);
    u32 i;

    for (i = 0u; i < WM_M15S_PARTICLE_COUNT; i++) {
        u32 member;
        for (member = 0u; member < 8u; member++) {
            s32 expected = (s32)(group_index * 8u + member);
            if ((s32)m15s_lh(record) == expected &&
                m15s_lh(record + 6u) != 0) {
#if !defined(W34N98_MUTANT_SKIP_PARTICLE_CLEAR)
                m15s_sh(record + 4u, 0u);
#endif
                break;
            }
        }
        record += WM_M15S_PARTICLE_STRIDE;
    }
}

s32 wm_8007DE14(s32 slot_index)
{
    u32 slot = m15s_slot(slot_index);
    u32 selector = m15s_lw(WM_M15S_SELECTOR);
    u32 commands = m15s_lw(WM_M15S_SCRIPT_TABLES + selector * 8u);
#if defined(W34N98_MUTANT_WRONG_INITIAL_TIMER_TABLE)
    u32 timers = commands;
#else
    u32 timers = m15s_lw(WM_M15S_SCRIPT_TABLES + selector * 8u + 4u);
#endif

    m15s_sw(slot + 0x54u, commands);
    m15s_sw(slot + 0x50u, 0u);
    m15s_sw(slot + 0x58u, timers);
    m15s_sh(slot + 0x20u, m15s_lhu(commands));
    m15s_sh(slot + 0x22u, m15s_lhu(timers));
#if !defined(W34N98_MUTANT_KEEP_INITIAL_INDEX_ZERO)
    m15s_sw(slot + 0x50u, 1u);
#endif
    return 1;
}

s32 wm_8007DE98(s32 slot_index)
{
    u32 slot = m15s_slot(slot_index);
    s16 command = m15s_lh(slot + 0x20u);

    switch (command) {
    case 1: {
        u16 timer = (u16)(m15s_lhu(slot + 0x22u) - 1u);
        u32 index;

        m15s_sh(slot + 0x22u, timer);
#if defined(W34N98_MUTANT_EXPIRE_TIMER_AT_ZERO)
        if ((s16)timer > 0)
#else
        if ((s16)timer >= 0)
#endif
            break;
        index = m15s_lw(slot + 0x50u);
        m15s_sh(slot + 0x20u,
                 m15s_lhu(m15s_lw(slot + 0x54u) + index * 2u));
        m15s_sh(slot + 0x22u,
                 m15s_lhu(m15s_lw(slot + 0x58u) + index * 2u));
        m15s_sw(slot + 0x50u, index + 1u);
        break;
    }
    case 2:
        m15s_play_fixed_three(0x1Cu);
        m15s_claim(2u, 2);
        m15s_claim(3u, 2);
        m15s_claim(4u, 3);
        m15s_claim(5u, 3);
        m15s_claim(6u, 3);
        m15s_claim(7u, 3);
#if defined(W34N98_MUTANT_COMMAND2_WRONG_FINAL_CLAIM)
        m15s_claim(8u, 2);
#else
        m15s_claim(8u, 3);
#endif
        m15s_command_done(slot);
        break;
    case 3:
        wm_80089514(34u);
        wm_80089514(35u);
        wm_80089514(36u);
        m15s_claim(2u, 3);
        m15s_claim(3u, 3);
        m15s_claim(4u, 3);
        m15s_claim(5u, 3);
        m15s_claim(6u, 3);
        m15s_claim(7u, 3);
        m15s_claim(8u, 3);
        m15s_command_done(slot);
        break;
    case 4:
        m15s_claim(2u, 4);
        m15s_claim(3u, 4);
        m15s_claim(4u, 3);
        m15s_claim(5u, 3);
        m15s_claim(6u, 3);
        m15s_claim(7u, 3);
        m15s_claim(8u, 3);
        m15s_command_done(slot);
        break;
    case 5:
        m15s_claim(2u, 5);
        m15s_command_done(slot);
        break;
    case 6:
        m15s_claim(0u, 13);
        m15s_sw(WM_M15S_GLOBAL_CCA4, 1u);
        m15s_sw(WM_M15S_GLOBAL_D3CC, 64u);
        m15s_command_done(slot);
        break;
    case 7:
        m15s_claim(3u, 5);
        m15s_claim(4u, 4);
        m15s_claim(5u, 4);
        m15s_claim(6u, 4);
        m15s_claim(7u, 4);
        m15s_claim(8u, 4);
        m15s_claim(0u, 12);
        m15s_sw(WM_M15S_GLOBAL_CCA4, 1u);
        m15s_sw(WM_M15S_GLOBAL_D3CC, 64u);
        m15s_command_done(slot);
        break;
    case 8:
        m15s_claim(2u, 6);
        m15s_command_done(slot);
        break;
    case 9:
        m15s_claim(0u, 13);
        m15s_sw(WM_M15S_GLOBAL_CCA4, 1u);
        m15s_sw(WM_M15S_GLOBAL_D3CC, 128u);
        m15s_command_done(slot);
        break;
    case 10:
        m15s_claim(0u, 12);
#if defined(W34N98_MUTANT_COMMAND10_WRONG_GLOBALS)
        m15s_sw(WM_M15S_GLOBAL_CCA4, 2u);
#else
        m15s_sw(WM_M15S_GLOBAL_CCA4, 1u);
#endif
        m15s_sw(WM_M15S_GLOBAL_D3CC, 128u);
        m15s_claim(9u, 1);
        m15s_claim(2u, 7);
        m15s_command_done(slot);
        break;
    case 16:
        m15s_claim(2u, 4);
        m15s_claim(3u, 16);
        m15s_claim(4u, 3);
        m15s_claim(5u, 3);
        m15s_claim(6u, 3);
        m15s_claim(7u, 3);
        m15s_claim(8u, 3);
        m15s_command_done(slot);
        break;
    case 17:
        m15s_claim(2u, 16);
        m15s_command_done(slot);
        break;
    case 18:
        m15s_claim(2u, 17);
        m15s_command_done(slot);
        break;
    case 24:
        m15s_claim(2u, 4);
        m15s_claim(3u, 24);
        m15s_claim(4u, 3);
        m15s_claim(5u, 3);
        m15s_claim(6u, 3);
        m15s_claim(7u, 3);
        m15s_claim(8u, 3);
        m15s_command_done(slot);
        break;
    case 25:
        m15s_claim(2u, 24);
        m15s_command_done(slot);
        break;
    case 61: {
        u32 selector = m15s_lw(WM_M15S_SELECTOR);
#if defined(W34N98_MUTANT_SOUND_TABLE_WRONG_STRIDE)
        u32 source = WM_M15S_SOUND_TABLE + selector * 8u;
#else
        u32 source = WM_M15S_SOUND_TABLE + selector * 6u;
#endif
        u32 i;
        for (i = 0u; i < 3u; i++) {
            u16 id = m15s_lhu(source + i * 2u);
            func_80039E60(m15s_sound_id(id));
        }
        m15s_command_done(slot);
        break;
    }
    case 62:
        m15s_play_fixed_three(0x19u);
        m15s_command_done(slot);
        break;
    case 63:
        m15s_claim(0u, 13);
        m15s_sw(WM_M15S_GLOBAL_CCA4, 2u);
        m15s_sw(WM_M15S_GLOBAL_D3CC, 4u);
        m15s_command_done(slot);
        break;
    case 64:
#if !defined(W34N98_MUTANT_SKIP_COMMAND64_EXIT)
        m15s_sw(WM_M15S_GLOBAL_D554, 0u);
        m15s_sw(WM_M15S_GLOBAL_D7CC, 0u);
#endif
        m15s_sh(slot + 0x20u, 0u);
        break;
    default:
        break;
    }
    return 1;
}
