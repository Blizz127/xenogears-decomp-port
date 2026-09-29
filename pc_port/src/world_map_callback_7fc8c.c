/* Exact native transcription of retail [0x8007FC8C, 0x8007FF70). */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_callback_7a144.h"
#include "world_map_callback_7fc8c.h"
#include "world_map_callback_80a28.h"

#define WM_M15S_POOL_PTR    UINT32_C(0x8009BE24)
#define WM_M15S_CONTEXT_PTR UINT32_C(0x8009C620)
#define WM_M15S_BASE_MATRIX UINT32_C(0x8009A180)
#define WM_M15S_STREAM_SIDE UINT32_C(0x8009D7F0)
#define WM_M15S_SCALE_A     UINT32_C(0x1F800000)
#define WM_M15S_SCALE_B     UINT32_C(0x1F800010)

#define WM_M15S_FIRST_OWNER  UINT32_C(0x2F4)
#define WM_M15S_SECOND_OWNER UINT32_C(0x348)
#define WM_M15S_FIRST_MATRIX UINT32_C(0x314)
#define WM_M15S_SECOND_MATRIX UINT32_C(0x368)

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

static s32 m15s_as_s32(u32 bits)
{
    s32 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static u32 m15s_sra12(u32 bits)
{
    u32 shifted = bits >> 12u;

    if ((bits & UINT32_C(0x80000000)) != 0u)
        shifted |= UINT32_C(0xFFF00000);
    return shifted;
}

static u32 m15s_slot(s32 slot_index)
{
    return m15s_lw(WM_M15S_POOL_PTR) + ((u32)slot_index << 7u);
}

static void m15s_copy_words(u32 destination, u32 source, u32 count)
{
    u32 index;

    for (index = 0u; index < count; index++)
        m15s_sw(destination + index * 4u,
                 m15s_lw(source + index * 4u));
}

s32 wm_8007FC8C(s32 slot_index)
{
    u32 context = m15s_lw(WM_M15S_CONTEXT_PTR);
    u32 slot = m15s_slot(slot_index);
    u32 owner = context + WM_M15S_FIRST_OWNER;
    u32 descriptor;

    descriptor = m15s_lw(owner + 0x40u);
    wm_8007A06C(owner, m15s_lw(owner + 0x48u),
                 m15s_lhu(descriptor + 4u));
#if !defined(W34N96_MUTANT_SKIP_SECOND_STREAM)
    owner = context + WM_M15S_SECOND_OWNER;
    descriptor = m15s_lw(owner + 0x40u);
    wm_8007A06C(owner, m15s_lw(owner + 0x48u),
                 m15s_lhu(descriptor + 4u));
#endif

#if defined(W34N96_MUTANT_WRONG_INITIAL_X)
    m15s_sw(slot + 0x28u, UINT32_C(0x01488000));
#else
    m15s_sw(slot + 0x28u, UINT32_C(0x01498000));
#endif
    m15s_sw(slot + 0x2Cu, UINT32_C(0xFFF80000));
    m15s_sw(slot + 0x30u, UINT32_C(0x04AF2000));
    m15s_sw(slot + 0x54u, UINT32_C(0xFFFFF800));
    m15s_sh(slot + 0x20u, 0u);
    m15s_sw(slot + 0x50u, 0u);
    m15s_sw(slot + 0x58u, 128u);
    return 3;
}

static void m15s_publish_position(u32 context, u32 slot)
{
#if defined(W34N96_MUTANT_WRONG_POSITION_SHIFT)
    u32 x = m15s_lw(slot + 0x28u) >> 16u;
#else
    u32 x = m15s_sra12(m15s_lw(slot + 0x28u));
#endif
    u32 y = m15s_sra12(m15s_lw(slot + 0x2Cu));
    u32 z = m15s_sra12(m15s_lw(slot + 0x30u));

    m15s_sw(context + 0x2FCu, x);
    m15s_sw(context + 0x300u, y);
    m15s_sw(context + 0x304u, z);
    m15s_sw(context + 0x350u, x);
    m15s_sw(context + 0x354u, y);
    m15s_sw(context + 0x358u, z);
}

static void m15s_scale_matrices(u32 context, u32 primary, u32 secondary)
{
    m15s_copy_words(context + WM_M15S_SECOND_MATRIX,
                    WM_M15S_BASE_MATRIX, 8u);
    m15s_copy_words(context + WM_M15S_FIRST_MATRIX,
                    context + WM_M15S_SECOND_MATRIX, 8u);

    m15s_sw(WM_M15S_SCALE_A + 0u, primary);
    m15s_sw(WM_M15S_SCALE_A + 4u, 4096u);
    m15s_sw(WM_M15S_SCALE_A + 8u, primary);
    m15s_sw(WM_M15S_SCALE_B + 0u, secondary);
    m15s_sw(WM_M15S_SCALE_B + 4u, 4096u);
    m15s_sw(WM_M15S_SCALE_B + 8u, secondary);
#if defined(W34N96_MUTANT_SWAP_SCALE_VECTORS)
    (void)ScaleMatrix((MATRIX *)PSX_ADDR(context + WM_M15S_FIRST_MATRIX),
                      (VECTOR *)PSX_ADDR(WM_M15S_SCALE_B));
    (void)ScaleMatrix((MATRIX *)PSX_ADDR(context + WM_M15S_SECOND_MATRIX),
                      (VECTOR *)PSX_ADDR(WM_M15S_SCALE_A));
#else
    (void)ScaleMatrix((MATRIX *)PSX_ADDR(context + WM_M15S_FIRST_MATRIX),
                      (VECTOR *)PSX_ADDR(WM_M15S_SCALE_A));
    (void)ScaleMatrix((MATRIX *)PSX_ADDR(context + WM_M15S_SECOND_MATRIX),
                      (VECTOR *)PSX_ADDR(WM_M15S_SCALE_B));
#endif
}

static u32 m15s_advance_phase(u32 phase)
{
#if defined(W34N96_MUTANT_WRONG_PHASE_STEP)
    phase += 383u;
#else
    phase += 384u;
#endif
    if (m15s_as_s32(phase) > 32767)
        phase = 32767u;
    return phase;
}

s32 wm_8007FD30(s32 slot_index)
{
    u32 context = m15s_lw(WM_M15S_CONTEXT_PTR);
    u32 slot = m15s_slot(slot_index);
    u32 first = context + WM_M15S_FIRST_OWNER;
    u32 second = context + WM_M15S_SECOND_OWNER;
    u32 side;
    u32 brightness;
    u32 descriptor;
    u32 stream;

    if (m15s_lhu(slot + 0x04u) == 1u) {
#if defined(W34N96_MUTANT_SKIP_LATCH_CLEAR)
        m15s_sh(slot + 0x04u, m15s_lhu(slot + 0x04u));
#else
        m15s_sh(slot + 0x04u, 0u);
#endif
    }

    m15s_publish_position(context, slot);
    m15s_scale_matrices(context, m15s_lw(slot + 0x50u),
                        m15s_lw(slot + 0x54u));
    m15s_sw(slot + 0x50u, m15s_advance_phase(m15s_lw(slot + 0x50u)));
    m15s_sw(slot + 0x54u, m15s_advance_phase(m15s_lw(slot + 0x54u)));

    side = m15s_lw(WM_M15S_STREAM_SIDE);
#if defined(W34N96_MUTANT_WRONG_STREAM_SIDE)
    side ^= 1u;
#endif
    brightness = m15s_lw(slot + 0x58u);
    descriptor = m15s_lw(first + 0x40u);
    stream = m15s_lw(first + 0x48u + side * 4u);
    wm_800809EC(stream, (s32)m15s_lhu(descriptor + 4u),
                 (u8)brightness, (u8)brightness, (u8)brightness);
    descriptor = m15s_lw(second + 0x40u);
    stream = m15s_lw(second + 0x48u + side * 4u);
    wm_800809EC(stream, (s32)m15s_lhu(descriptor + 4u),
                 (u8)brightness, (u8)brightness, (u8)brightness);

#if defined(W34N96_MUTANT_WRONG_FADE_STEP)
    brightness -= 4u;
#else
    brightness -= 3u;
#endif
    m15s_sw(slot + 0x58u, brightness);
    if (m15s_as_s32(brightness) < 0) {
        m15s_sw(slot + 0x58u, 0u);
        return 3;
    }
    return 1;
}
