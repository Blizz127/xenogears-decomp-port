/* Exact native transcription of retail [0x800819C8, 0x80081C3C). */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_callback_80a28.h"
#include "world_map_callback_817a0.h"
#include "world_map_callback_819c8.h"

#define WM_M16D_POOL_PTR    UINT32_C(0x8009BE24)
#define WM_M16D_CONTEXT_PTR UINT32_C(0x8009C620)
#define WM_M16D_RESET_X     UINT32_C(0x8009C5AC)
#define WM_M16D_RESET_Y     UINT32_C(0x8009C5B0)
#define WM_M16D_RESET_Z     UINT32_C(0x8009C5B4)
#define WM_M16D_BASE_MATRIX UINT32_C(0x8009A180)
#define WM_M16D_STREAM_SIDE UINT32_C(0x8009D7F0)
#define WM_M16D_SCALE       UINT32_C(0x1F800000)

static s16 m16d_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u16 m16d_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 m16d_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void m16d_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void m16d_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u32 m16d_slot(s32 slot_index)
{
    return m16d_lw(WM_M16D_POOL_PTR) + ((u32)slot_index << 7u);
}

static void m16d_copy_words(u32 destination, u32 source, u32 count)
{
    u32 index;

    for (index = 0u; index < count; index++)
        m16d_sw(destination + index * 4u,
                 m16d_lw(source + index * 4u));
}

static void m16d_initialize_stream(u32 owner)
{
    u32 descriptor = m16d_lw(owner + 0x40u);

    wm_800816DC(owner, m16d_lw(owner + 0x48u),
                 m16d_lhu(descriptor + 4u), 3);
}

s32 wm_800819C8(s32 slot_index)
{
    u32 context = m16d_lw(WM_M16D_CONTEXT_PTR);
    u32 slot = m16d_slot(slot_index);

    m16d_sh(slot + 0x20u, 0u);
    m16d_sw(slot + 0x28u, m16d_lw(WM_M16D_RESET_X));
#if defined(W34N106_MUTANT_WRONG_INITIAL_Y)
    m16d_sw(slot + 0x2Cu,
             m16d_lw(WM_M16D_RESET_Y) + UINT32_C(0xFFF00000));
#else
    m16d_sw(slot + 0x2Cu, m16d_lw(WM_M16D_RESET_Y));
#endif
    m16d_sw(slot + 0x30u, m16d_lw(WM_M16D_RESET_Z));
    m16d_sw(slot + 0x50u, 0u);
    m16d_sw(slot + 0x54u, 0u);
    m16d_sw(slot + 0x58u, 0u);

    m16d_copy_words(context + 0x20u, WM_M16D_BASE_MATRIX, 8u);
    m16d_sw(WM_M16D_SCALE + 0u, 4096u);
#if defined(W34N106_MUTANT_WRONG_SCALE_Y)
    m16d_sw(WM_M16D_SCALE + 4u, 24576u);
#else
    m16d_sw(WM_M16D_SCALE + 4u, 28672u);
#endif
    m16d_sw(WM_M16D_SCALE + 8u, 4096u);
    (void)ScaleMatrix((MATRIX *)PSX_ADDR(context + 0x20u),
                      (VECTOR *)PSX_ADDR(WM_M16D_SCALE));
#if !defined(W34N106_MUTANT_SKIP_MATRIX_MIRROR)
    m16d_copy_words(context + 0x74u, context + 0x20u, 8u);
#endif

#if defined(W34N106_MUTANT_WRONG_FIRST_ABR)
    {
        u32 descriptor = m16d_lw(context + 0x40u);
        wm_800816DC(context, m16d_lw(context + 0x48u),
                     m16d_lhu(descriptor + 4u), 2);
    }
#else
    m16d_initialize_stream(context);
#endif
#if !defined(W34N106_MUTANT_SKIP_SECOND_STREAM)
    m16d_initialize_stream(context + 0x54u);
#endif
    return 1;
}

static void m16d_consume_latch(u32 slot)
{
    if (m16d_lh(slot + 0x04u) == 1) {
        m16d_sh(slot + 0x04u, 0u);
#if defined(W34N106_MUTANT_LATCH_STAYS_IDLE)
        m16d_sh(slot + 0x20u, 0u);
#else
        m16d_sh(slot + 0x20u, 1u);
#endif
    }
}

static void m16d_advance_fade(u32 slot)
{
    if (m16d_lh(slot + 0x20u) == 1) {
#if defined(W34N106_MUTANT_WRONG_FADE_STEPS)
        u32 red = m16d_lw(slot + 0x50u) + 3u;
#else
        u32 red = m16d_lw(slot + 0x50u) + 4u;
#endif
        u32 green = m16d_lw(slot + 0x54u) + 2u;
        u32 blue = m16d_lw(slot + 0x58u) + 1u;

        m16d_sw(slot + 0x50u, red);
        m16d_sw(slot + 0x54u, green);
        m16d_sw(slot + 0x58u, blue);
        if ((red & UINT32_C(0x80000000)) == 0u && red >= 252u) {
            m16d_sw(slot + 0x50u, 252u);
#if defined(W34N106_MUTANT_CLAMP_ALL_CHANNELS)
            m16d_sw(slot + 0x54u, 252u);
            m16d_sw(slot + 0x58u, 252u);
#endif
            m16d_sh(slot + 0x20u, 0u);
        }
    }
}

static void m16d_color_stream(u32 owner, u32 side, u32 slot)
{
    u32 descriptor = m16d_lw(owner + 0x40u);
    u32 stream = m16d_lw(owner + 0x48u + side * 4u);

    wm_800809EC(stream, (s32)m16d_lhu(descriptor + 4u),
                 (u8)m16d_lw(slot + 0x50u),
                 (u8)m16d_lw(slot + 0x54u),
                 (u8)m16d_lw(slot + 0x58u));
}

s32 wm_80081B24(s32 slot_index)
{
    u32 context = m16d_lw(WM_M16D_CONTEXT_PTR);
    u32 slot = m16d_slot(slot_index);
    u32 side;

    m16d_consume_latch(slot);
    m16d_advance_fade(slot);
    side = m16d_lw(WM_M16D_STREAM_SIDE);
    m16d_color_stream(context, side, slot);
    m16d_color_stream(context + 0x54u, side, slot);
    return 1;
}
