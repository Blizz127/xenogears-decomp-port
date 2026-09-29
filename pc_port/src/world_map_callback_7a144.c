/*
 * Retail mode-10 scaled-stream callback unit:
 *   helper [0x8007A06C, 0x8007A144)
 *   pair   [0x8007A144, 0x8007A410)
 */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_callback_7a144.h"

extern u16 GetTPage(int tp, int abr, int x, int y);
extern u16 GetClut(int x, int y);

#define WM_M10M_POOL_PTR    0x8009BE24u
#define WM_M10M_CONTEXT_PTR 0x8009C620u
#define WM_M10M_BASE_MATRIX 0x8009A180u
#define WM_M10M_MATRIX_A    0x1F8000F0u
#define WM_M10M_MATRIX_B    0x1F800110u
#define WM_M10M_SCALE_A     0x1F800000u
#define WM_M10M_SCALE_B     0x1F800010u

static u16 m10m_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 m10m_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void m10m_sb(u32 address, u8 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void m10m_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void m10m_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u8 m10m_lbu(u32 address)
{
    u8 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static s32 m10m_as_s32(u32 bits)
{
    s32 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static u32 m10m_slot(s32 slot_index)
{
    return m10m_lw(WM_M10M_POOL_PTR) + ((u32)slot_index << 7u);
}

static void m10m_copy_words(u32 destination, u32 source, u32 count)
{
    u32 index;
    for (index = 0u; index < count; index++)
        m10m_sw(destination + index * 4u,
                 m10m_lw(source + index * 4u));
}

void wm_8007A06C(u32 owner, u32 primitive_source, u16 count)
{
    u32 index;

    for (index = 0u; index < (u32)count; index++) {
        u32 primitive = primitive_source + index * 40u;
        u16 tpage;
        u16 clut;
        u8 code;

        m10m_sb(primitive + 3u, 9u);
#if defined(W34N63_MUTANT_WRONG_PRIMITIVE_CODE)
        m10m_sb(primitive + 7u, 43u);
#else
        m10m_sb(primitive + 7u, 44u);
#endif
        tpage = GetTPage(1, 3, 832, 256);
        m10m_sh(primitive + 22u, tpage);
        clut = GetClut(256, 511);
        m10m_sh(primitive + 14u, clut);
        m10m_sb(primitive + 4u, 128u);
        m10m_sb(primitive + 5u, 128u);
        m10m_sb(primitive + 6u, 128u);
        code = m10m_lbu(primitive + 7u);
        m10m_sb(primitive + 7u, (u8)(code | 2u));
    }

#if defined(W34N63_MUTANT_REVERSE_STREAM_COPY)
    memcpy(PSX_ADDR(m10m_lw(owner + 0x48u)),
           PSX_ADDR(m10m_lw(owner + 0x4Cu)), (size_t)count * 40u);
#else
    memcpy(PSX_ADDR(m10m_lw(owner + 0x4Cu)),
           PSX_ADDR(m10m_lw(owner + 0x48u)), (size_t)count * 40u);
#endif
}

s32 wm_8007A144(s32 slot_index)
{
    u32 slot = m10m_slot(slot_index);
    u32 context = m10m_lw(WM_M10M_CONTEXT_PTR);
    u32 descriptor;

    m10m_sh(slot + 0x20u, 0u);
    m10m_sw(slot + 0x70u, 0u);
    m10m_sw(slot + 0x6Cu, 0u);

    descriptor = m10m_lw(context + 0x4D8u);
    wm_8007A06C(context + 0x498u, m10m_lw(context + 0x4E0u),
                 m10m_lhu(descriptor + 4u));
#if !defined(W34N63_MUTANT_SKIP_SECOND_STREAM)
    descriptor = m10m_lw(context + 0x52Cu);
    wm_8007A06C(context + 0x4ECu, m10m_lw(context + 0x534u),
                 m10m_lhu(descriptor + 4u));
#endif
    return 3;
}

s32 wm_8007A1B4(s32 slot_index)
{
    u32 context = m10m_lw(WM_M10M_CONTEXT_PTR);
    u32 slot = m10m_slot(slot_index);
    u32 primary_phase;
    u32 secondary_phase;
    s32 return_state = 1;

    m10m_copy_words(context + 0x4A0u, context + 8u, 4u);
    m10m_copy_words(context + 0x4F4u, context + 0x4A0u, 4u);

#if defined(W34N63_MUTANT_WRONG_PRIMARY_STEP)
    primary_phase = m10m_lw(slot + 0x6Cu) + 191u;
#else
    primary_phase = m10m_lw(slot + 0x6Cu) + 192u;
#endif
    m10m_sw(slot + 0x6Cu, primary_phase);
    if (m10m_as_s32(primary_phase) >= 2048) {
        secondary_phase = (m10m_lw(slot + 0x70u) + 256u) & 0x7FFFu;
        m10m_sw(slot + 0x70u, secondary_phase);
    }

    secondary_phase = m10m_lw(slot + 0x70u);
#if defined(W34N63_MUTANT_LATE_TERMINAL_THRESHOLD)
    if (m10m_as_s32(secondary_phase) >= 24577) {
#else
    if (m10m_as_s32(secondary_phase) >= 24576) {
#endif
        return_state = 3;
        m10m_sh(slot + 4u, 0u);
        m10m_sw(slot + 0x70u, 0u);
        m10m_sw(slot + 0x6Cu, 0u);
        primary_phase = 0u;
        secondary_phase = 0u;
    } else {
        primary_phase = m10m_lw(slot + 0x6Cu);
    }

    m10m_copy_words(WM_M10M_MATRIX_A, WM_M10M_BASE_MATRIX, 8u);
    m10m_copy_words(WM_M10M_MATRIX_B, WM_M10M_MATRIX_A, 8u);
    m10m_sw(WM_M10M_SCALE_A + 0u, primary_phase);
    m10m_sw(WM_M10M_SCALE_A + 4u, 4096u);
    m10m_sw(WM_M10M_SCALE_A + 8u, primary_phase);
    m10m_sw(WM_M10M_SCALE_B + 0u, secondary_phase);
    m10m_sw(WM_M10M_SCALE_B + 4u, 4096u);
    m10m_sw(WM_M10M_SCALE_B + 8u, secondary_phase);

#if defined(W34N63_MUTANT_SWAP_SCALE_VECTORS)
    (void)ScaleMatrix((MATRIX*)PSX_ADDR(WM_M10M_MATRIX_A),
                      (VECTOR*)PSX_ADDR(WM_M10M_SCALE_B));
    (void)ScaleMatrix((MATRIX*)PSX_ADDR(WM_M10M_MATRIX_B),
                      (VECTOR*)PSX_ADDR(WM_M10M_SCALE_A));
#else
    (void)ScaleMatrix((MATRIX*)PSX_ADDR(WM_M10M_MATRIX_A),
                      (VECTOR*)PSX_ADDR(WM_M10M_SCALE_A));
    (void)ScaleMatrix((MATRIX*)PSX_ADDR(WM_M10M_MATRIX_B),
                      (VECTOR*)PSX_ADDR(WM_M10M_SCALE_B));
#endif

#if defined(W34N63_MUTANT_SHORT_MATRIX_COPY)
    m10m_copy_words(context + 0x4B8u, WM_M10M_MATRIX_A, 7u);
#else
    m10m_copy_words(context + 0x4B8u, WM_M10M_MATRIX_A, 8u);
#endif
    m10m_copy_words(context + 0x50Cu, WM_M10M_MATRIX_B, 8u);
    return return_state;
}
