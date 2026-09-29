/* Exact native transcription of retail [0x80081FB4, 0x80082324). */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_81fb4.h"

#define WM_M16W_POOL_PTR    UINT32_C(0x8009BE24)
#define WM_M16W_WIDTH_TABLE UINT32_C(0x8009D148)
#define WM_M16W_COUNT       UINT32_C(192)
#define WM_M16W_BAND_SIZE   UINT32_C(64)

static s16 m16w_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u16 m16w_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 m16w_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void m16w_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void m16w_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u32 m16w_slot(s32 slot_index)
{
    return m16w_lw(WM_M16W_POOL_PTR) + ((u32)slot_index << 7u);
}

static void m16w_fill(u32 table, u16 value)
{
    u32 index;

    for (index = 0u; index < WM_M16W_COUNT; index++)
        m16w_sh(table + index * 2u, value);
}

static void m16w_randomize_band(u32 table, u32 band)
{
    u32 start = ((u32)rand() & UINT32_C(0x3F)) +
                band * WM_M16W_BAND_SIZE;
    u32 end = start + ((u32)rand() & UINT32_C(0x1F)) + 1u;
    u32 index;

    for (index = start; index < end; index++) {
        if (((u32)rand() & UINT32_C(3)) == 0u)
            m16w_sh(table + index * 2u,
                    (u16)(((u32)rand() & UINT32_C(0x3F)) + 1u));
    }
}

static void m16w_consume_command(u32 slot)
{
    s16 command = (s16)(u16)(m16w_lhu(slot + 0x04u) - 1u);

    if ((u32)(s32)command < 5u) {
#if !defined(W34N108_MUTANT_KEEP_COMMAND)
        m16w_sh(slot + 0x04u, 0u);
#endif
#if defined(W34N108_MUTANT_WRONG_COMMAND_MAP)
        m16w_sh(slot + 0x20u, (u16)command);
#else
        m16w_sh(slot + 0x20u, (u16)(command + 1));
#endif
    }
}

s32 wm_80081FD8(s32 slot_index)
{
    u32 slot = m16w_slot(slot_index);
    u32 table = m16w_lw(WM_M16W_WIDTH_TABLE);
    s16 state;

    m16w_consume_command(slot);
    state = m16w_lh(slot + 0x20u);
    switch (state) {
    case 0:
#if !defined(W34N108_MUTANT_SKIP_STATE0_FILL)
        m16w_fill(table, m16w_lhu(slot + 0x50u));
#endif
        m16w_randomize_band(table, 0u);
        m16w_randomize_band(table, 1u);
        m16w_randomize_band(table, 2u);
        break;
    case 1: {
        u32 width = m16w_lw(slot + 0x50u) + 1u;

#if defined(W34N108_MUTANT_WRONG_GROW_CLAMP)
        if ((s32)width >= 65)
            width = 63u;
#else
        if ((s32)width >= 65)
            width = 64u;
#endif
        m16w_sw(slot + 0x50u, width);
        m16w_fill(table, (u16)width);
        break;
    }
    case 2: {
        u32 width = m16w_lw(slot + 0x50u) - 1u;

#if defined(W34N108_MUTANT_WRONG_SHRINK_CLAMP)
        if ((s32)width < 2)
            width = 1u;
#else
        if ((s32)width < 2)
            width = 2u;
#endif
        m16w_sw(slot + 0x50u, width);
        m16w_fill(table, (u16)width);
        break;
    }
    case 3: {
        u32 index;

        for (index = 0u; index < WM_M16W_COUNT; index++) {
#if defined(W34N108_MUTANT_WRONG_RANDOM_GATE)
            if (((u32)rand() & UINT32_C(3)) == 1u)
#else
            if (((u32)rand() & UINT32_C(3)) == 0u)
#endif
                m16w_sh(table + index * 2u,
                        (u16)(((u32)rand() & UINT32_C(0x3F)) + 1u));
        }
        break;
    }
    case 4:
#if defined(W34N108_MUTANT_WRONG_CONSTANT_FILL)
        m16w_fill(table, 1u);
#else
        m16w_fill(table, 2u);
#endif
        break;
    default:
        break;
    }
    return 1;
}
