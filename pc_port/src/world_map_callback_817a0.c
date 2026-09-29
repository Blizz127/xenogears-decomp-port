/* Exact native transcription of retail [0x800816DC, 0x800819C8). */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_80a28.h"
#include "world_map_callback_817a0.h"

extern u16 GetTPage(int tp, int abr, int x, int y);

#define WM_M16F_POOL_PTR    UINT32_C(0x8009BE24)
#define WM_M16F_CONTEXT_PTR UINT32_C(0x8009C620)
#define WM_M16F_RESET_X     UINT32_C(0x8009C5AC)
#define WM_M16F_RESET_Y     UINT32_C(0x8009C5B0)
#define WM_M16F_RESET_Z     UINT32_C(0x8009C5B4)
#define WM_M16F_STREAM_SIDE UINT32_C(0x8009D7F0)

static s16 m16f_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u16 m16f_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 m16f_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u8 m16f_lbu(u32 address)
{
    u8 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void m16f_sb(u32 address, u8 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void m16f_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void m16f_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u32 m16f_sra12(u32 bits)
{
    u32 result = bits >> 12u;

    if ((bits & UINT32_C(0x80000000)) != 0u)
        result |= UINT32_C(0xFFF00000);
    return result;
}

static u32 m16f_slot(s32 slot_index)
{
    return m16f_lw(WM_M16F_POOL_PTR) + ((u32)slot_index << 7u);
}

void wm_800816DC(u32 owner, u32 primitive_source, u16 count, s32 abr)
{
    u32 index;

    for (index = 0u; index < (u32)count; index++) {
        u32 primitive = primitive_source + index * 40u;
        u8 code;

        m16f_sb(primitive + 3u, 9u);
#if defined(W34N105_MUTANT_WRONG_PRIMITIVE_CODE)
        m16f_sb(primitive + 7u, 43u);
#else
        m16f_sb(primitive + 7u, 44u);
#endif
#if defined(W34N105_MUTANT_WRONG_TPAGE_X)
        m16f_sh(primitive + 22u, GetTPage(0, (int)abr, 383, 0));
#else
        m16f_sh(primitive + 22u, GetTPage(0, (int)abr, 384, 0));
#endif
        code = m16f_lbu(primitive + 7u);
        m16f_sb(primitive + 7u, (u8)(code | 2u));
    }

#if defined(W34N105_MUTANT_REVERSE_STREAM_COPY)
    memcpy(PSX_ADDR(m16f_lw(owner + 0x48u)),
           PSX_ADDR(m16f_lw(owner + 0x4Cu)), (size_t)count * 40u);
#else
    memcpy(PSX_ADDR(m16f_lw(owner + 0x4Cu)),
           PSX_ADDR(m16f_lw(owner + 0x48u)), (size_t)count * 40u);
#endif
}

s32 wm_800817A0(s32 slot_index)
{
    u32 slot = m16f_slot(slot_index);
    u32 context = m16f_lw(WM_M16F_CONTEXT_PTR);
    u32 descriptor;

    m16f_sh(slot + 0x20u, 0u);
    m16f_sw(slot + 0x28u, m16f_lw(WM_M16F_RESET_X));
#if defined(W34N105_MUTANT_WRONG_INITIAL_Y)
    m16f_sw(slot + 0x2Cu, m16f_lw(WM_M16F_RESET_Y));
#else
    m16f_sw(slot + 0x2Cu,
             m16f_lw(WM_M16F_RESET_Y) + UINT32_C(0xFFF00000));
#endif
    m16f_sw(slot + 0x30u, m16f_lw(WM_M16F_RESET_Z));
    m16f_sw(slot + 0x50u, 0u);
    m16f_sw(slot + 0x54u, 0u);
    m16f_sw(slot + 0x58u, 0u);

    descriptor = m16f_lw(context + 0xE8u);
    wm_800816DC(context + 0xA8u, m16f_lw(context + 0xF0u),
                 m16f_lhu(descriptor + 4u), 1);

    m16f_sw(context + 0xB0u, m16f_sra12(m16f_lw(slot + 0x28u)));
#if defined(W34N105_MUTANT_LOGICAL_Y_SHIFT)
    m16f_sw(context + 0xB4u, m16f_lw(slot + 0x2Cu) >> 12u);
#else
    m16f_sw(context + 0xB4u, m16f_sra12(m16f_lw(slot + 0x2Cu)));
#endif
    m16f_sw(context + 0xB8u, m16f_sra12(m16f_lw(slot + 0x30u)));
    return 1;
}

static void m16f_restore_stream(u32 owner)
{
    u32 side = m16f_lw(WM_M16F_STREAM_SIDE);
    u32 descriptor = m16f_lw(owner + 0x40u);
    u32 stream = m16f_lw(owner + 0x48u + side * 4u);
    u32 count = (u32)m16f_lhu(descriptor + 4u);
    u32 index;

    for (index = 0u; index < count; index++) {
        u32 primitive = stream + index * 40u;
        u8 code;

        m16f_sb(primitive + 4u, 128u);
        m16f_sb(primitive + 5u, 128u);
        m16f_sb(primitive + 6u, 128u);
        code = m16f_lbu(primitive + 7u);
#if defined(W34N105_MUTANT_KEEP_SEMITRANS_BIT)
        m16f_sb(primitive + 7u, code);
#else
        m16f_sb(primitive + 7u, (u8)(code & UINT8_C(0xFD)));
#endif
    }
}

static void m16f_consume_latch(u32 slot, u32 owner)
{
    s16 latch = m16f_lh(slot + 0x04u);

    if (latch == 1) {
        m16f_sh(slot + 0x04u, 0u);
#if defined(W34N105_MUTANT_LATCH1_STAYS_IDLE)
        m16f_sh(slot + 0x20u, 0u);
#else
        m16f_sh(slot + 0x20u, 1u);
#endif
    } else if (latch == 2) {
        m16f_sh(slot + 0x04u, 0u);
        m16f_sh(slot + 0x20u, 0u);
        m16f_restore_stream(owner);
    }
}

static void m16f_advance_fade(u32 slot)
{
    if (m16f_lh(slot + 0x20u) == 1) {
        u32 red = m16f_lw(slot + 0x50u) + 1u;
        u32 green = m16f_lw(slot + 0x54u) + 1u;
        u32 blue = m16f_lw(slot + 0x58u) + 1u;

        m16f_sw(slot + 0x50u, red);
        m16f_sw(slot + 0x54u, green);
        m16f_sw(slot + 0x58u, blue);
#if defined(W34N105_MUTANT_WRONG_FADE_LIMIT)
        if (red > 255u) {
#else
        if ((red & UINT32_C(0x80000000)) == 0u && red >= 255u) {
#endif
            m16f_sw(slot + 0x50u, 255u);
            m16f_sw(slot + 0x54u, 255u);
            m16f_sw(slot + 0x58u, 255u);
            m16f_sh(slot + 0x20u, 0u);
        }
    }
}

s32 wm_80081868(s32 slot_index)
{
    u32 context = m16f_lw(WM_M16F_CONTEXT_PTR);
    u32 owner = context + 0xA8u;
    u32 slot = m16f_slot(slot_index);
    u32 side;
    u32 descriptor;
    u32 stream;

    m16f_consume_latch(slot, owner);
    m16f_advance_fade(slot);

    side = m16f_lw(WM_M16F_STREAM_SIDE);
    descriptor = m16f_lw(owner + 0x40u);
    stream = m16f_lw(owner + 0x48u + side * 4u);
    wm_800809EC(stream, (s32)m16f_lhu(descriptor + 4u),
                 (u8)m16f_lw(slot + 0x50u),
                 (u8)m16f_lw(slot + 0x54u),
                 (u8)m16f_lw(slot + 0x58u));
    return 1;
}
