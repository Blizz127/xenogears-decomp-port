/* Exact native transcription of retail mode-17 command-stream callbacks. */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_827c8.h"
#include "world_map_helper_97770.h"

#define WM_M17C_POOL_PTR    UINT32_C(0x8009BE24)
#define WM_M17C_SCRIPT      UINT32_C(0x8009A758)
#define WM_M17C_POSITION_X  UINT32_C(0x8009C5AC)
#define WM_M17C_POSITION_Y  UINT32_C(0x8009C5B0)
#define WM_M17C_POSITION_Z  UINT32_C(0x8009C5B4)
#define WM_M17C_ANGLES      UINT32_C(0x1F8000A0)
#define WM_M17C_GLOBAL_CCA4 UINT32_C(0x8009CCA4)
#define WM_M17C_GLOBAL_D3CC UINT32_C(0x8009D3CC)
#define WM_M17C_GLOBAL_D554 UINT32_C(0x8009D554)
#define WM_M17C_GLOBAL_D7CC UINT32_C(0x8009D7CC)

extern void *D_80062528;
extern void *D_8006259C;
extern void wm_80089160(u32 id, u32 vector, u32 flags);
extern void wm_800894C8(u32 record_index);
extern void wm_80089514(u32 group_index);
extern void func_80039E60(s32 packed_id);
extern void func_8003A3B8(s32 packed_id, s32 pitch, s32 steps);
extern void func_8003A89C(void *manager, s32 level, s32 steps);

typedef s32 (*m17c_handler_t)(u32 slot, s32 a1, s32 a2, s32 a3);

static s16 m17c_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 m17c_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void m17c_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void m17c_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static s16 m17c_as_s16(u16 bits)
{
    s16 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static u32 m17c_slot(s32 slot_index)
{
    return m17c_lw(WM_M17C_POOL_PTR) + ((u32)slot_index << 7u);
}

static s32 m17c_sound_id(u16 id)
{
    u16 bank;
    u32 bits;
    s32 value;

    memcpy(&bank, (const uint8_t *)D_8006259C + 0x14u, sizeof(bank));
    bits = ((u32)bank << 16u) | (u32)id;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static s32 m17c_command_0(u32 slot, s32 a1, s32 a2, s32 a3)
{
    (void)slot;
    (void)a1;
    (void)a2;
    (void)a3;
#if !defined(W34N111_MUTANT_SKIP_EXIT)
    m17c_sw(WM_M17C_GLOBAL_D554, 0u);
    m17c_sw(WM_M17C_GLOBAL_D7CC, 0u);
#endif
    return 0;
}

static s32 m17c_command_1(u32 slot, s32 a1, s32 a2, s32 a3)
{
    s16 timer = m17c_lh(slot + 0x22u);

    (void)a2;
    (void)a3;
    if (timer == 0) {
        m17c_sh(slot + 0x22u, (u16)a1);
        return 0;
    }
    {
        u16 next = (u16)((u16)timer - 1u);
        m17c_sh(slot + 0x22u, next);
#if defined(W34N111_MUTANT_TIMER_EXPIRES_LATE)
        return m17c_as_s16(next) < 0 ? 2 : 0;
#else
        return m17c_as_s16(next) < 1 ? 2 : 0;
#endif
    }
}

static s32 m17c_command_2(u32 slot, s32 a1, s32 a2, s32 a3)
{
    (void)slot;
    (void)a3;
    (void)wm_80097770((u32)a1, a2);
    return 4;
}

static s32 m17c_command_3(u32 slot, s32 a1, s32 a2, s32 a3)
{
    (void)slot;
    (void)a3;
    m17c_sw(WM_M17C_POSITION_X, (u32)a1 << 12u);
    m17c_sw(WM_M17C_POSITION_Y, (u32)a2 << 12u);
#if defined(W34N111_MUTANT_WRONG_POSITION_Z)
    m17c_sw(WM_M17C_POSITION_Z, (u32)a2 << 12u);
#else
    m17c_sw(WM_M17C_POSITION_Z, (u32)a3 << 12u);
#endif
    return 4;
}

static s32 m17c_command_4(u32 slot, s32 a1, s32 a2, s32 a3)
{
    (void)slot;
    m17c_sh(WM_M17C_ANGLES + 0u, (u16)a1);
#if defined(W34N111_MUTANT_SWAP_ANGLE_YZ)
    m17c_sh(WM_M17C_ANGLES + 2u, (u16)a3);
    m17c_sh(WM_M17C_ANGLES + 4u, (u16)a2);
#else
    m17c_sh(WM_M17C_ANGLES + 2u, (u16)a2);
    m17c_sh(WM_M17C_ANGLES + 4u, (u16)a3);
#endif
    return 4;
}

static s32 m17c_command_5(u32 slot, s32 a1, s32 a2, s32 a3)
{
    (void)slot;
    (void)a2;
    (void)a3;
#if defined(W34N111_MUTANT_WRONG_ANGLE_SOURCE)
    wm_80089160((u32)a1, WM_M17C_ANGLES + 2u, 0u);
#else
    wm_80089160((u32)a1, WM_M17C_ANGLES, 0u);
#endif
    return 2;
}

static s32 m17c_command_6(u32 slot, s32 a1, s32 a2, s32 a3)
{
    (void)slot;
    (void)a2;
    (void)a3;
    wm_800894C8((u32)a1);
    return 2;
}

static s32 m17c_command_7(u32 slot, s32 a1, s32 a2, s32 a3)
{
    (void)slot;
    (void)a2;
    (void)a3;
    wm_80089514((u32)a1);
    return 2;
}

static s32 m17c_command_8(u32 slot, s32 a1, s32 a2, s32 a3)
{
    (void)slot;
    (void)a3;
    func_8003A89C(D_80062528, a1, a2);
    return 4;
}

static s32 m17c_command_9(u32 slot, s32 a1, s32 a2, s32 a3)
{
    (void)slot;
    (void)a2;
    (void)a3;
#if defined(W34N111_MUTANT_SOUND_BANK_ZERO)
    func_80039E60((s32)(u16)a1);
#else
    func_80039E60(m17c_sound_id((u16)a1));
#endif
    return 2;
}

static s32 m17c_command_10(u32 slot, s32 a1, s32 a2, s32 a3)
{
    (void)slot;
#if defined(W34N111_MUTANT_SWAP_SOUND_CONTROL_ARGS)
    func_8003A3B8(m17c_sound_id((u16)a1), a3, a2);
#else
    func_8003A3B8(m17c_sound_id((u16)a1), a2, a3);
#endif
    return 4;
}

static s32 m17c_command_11(u32 slot, s32 a1, s32 a2, s32 a3)
{
    (void)slot;
    (void)a3;
#if defined(W34N111_MUTANT_SWAP_STATE_GLOBALS)
    m17c_sw(WM_M17C_GLOBAL_CCA4, (u32)a2);
    m17c_sw(WM_M17C_GLOBAL_D3CC, (u32)a1);
#else
    m17c_sw(WM_M17C_GLOBAL_CCA4, (u32)a1);
    m17c_sw(WM_M17C_GLOBAL_D3CC, (u32)a2);
#endif
    return 4;
}

static const m17c_handler_t s_m17c_handlers[12] = {
    m17c_command_0, m17c_command_1, m17c_command_2, m17c_command_3,
    m17c_command_4, m17c_command_5, m17c_command_6, m17c_command_7,
    m17c_command_8, m17c_command_9, m17c_command_10, m17c_command_11
};

s32 wm_800827C8(s32 slot_index)
{
    u32 slot = m17c_slot(slot_index);
#if defined(W34N111_MUTANT_WRONG_SCRIPT_POINTER)
    m17c_sw(slot + 0x50u, WM_M17C_SCRIPT + 2u);
#else
    m17c_sw(slot + 0x50u, WM_M17C_SCRIPT);
#endif
    return 1;
}

s32 wm_80076B34(s32 slot_index)
{
    u32 slot = m17c_slot(slot_index);
    s32 advance = 0;

    do {
        u32 command_address;
        u32 command_word;
        u16 opcode;
        s32 a1;
        s32 a2;
        s32 a3;

#if defined(W34N111_MUTANT_WRONG_ADVANCE_SCALE)
        command_address = m17c_lw(slot + 0x50u) + ((u32)advance << 2u);
#else
        command_address = m17c_lw(slot + 0x50u) + ((u32)advance << 1u);
#endif
        m17c_sw(slot + 0x50u, command_address);
        command_word = m17c_lw(command_address);
        opcode = (u16)command_word;
        a1 = (s32)m17c_as_s16((u16)(command_word >> 16u));
        a2 = (s32)m17c_lh(command_address + 4u);
        a3 = (s32)m17c_lh(command_address + 6u);
        advance = s_m17c_handlers[opcode](slot, a1, a2, a3);
    } while (advance != 0);
    return 1;
}
