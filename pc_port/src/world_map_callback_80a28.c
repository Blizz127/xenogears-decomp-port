/* Exact native transcription of retail [0x800809EC, 0x80080D00). */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_callback_7a144.h"
#include "world_map_callback_80a28.h"

#define WM_M13O_POOL_PTR    UINT32_C(0x8009BE24)
#define WM_M13O_CONTEXT_PTR UINT32_C(0x8009C620)
#define WM_M13O_RESET_X     UINT32_C(0x8009C5AC)
#define WM_M13O_RESET_Z     UINT32_C(0x8009C5B4)
#define WM_M13O_BASE_MATRIX UINT32_C(0x8009A180)
#define WM_M13O_STREAM_SIDE UINT32_C(0x8009D7F0)
#define WM_M13O_SCALE_A     UINT32_C(0x1F800000)
#define WM_M13O_SCALE_B     UINT32_C(0x1F800010)

static u16 m13o_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 m13o_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void m13o_sb(u32 address, u8 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void m13o_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void m13o_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static s32 m13o_as_s32(u32 bits)
{
    s32 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static u32 m13o_sra12(u32 bits)
{
    u32 shifted = bits >> 12u;

    if ((bits & UINT32_C(0x80000000)) != 0u)
        shifted |= UINT32_C(0xFFF00000);
    return shifted;
}

static u32 m13o_slot(s32 slot_index)
{
    return m13o_lw(WM_M13O_POOL_PTR) + ((u32)slot_index << 7u);
}

static void m13o_copy_words(u32 destination, u32 source, u32 count)
{
    u32 index;

    for (index = 0u; index < count; index++)
        m13o_sw(destination + index * 4u,
                 m13o_lw(source + index * 4u));
}

void wm_800809EC(u32 primitive_source, s32 count,
                 u8 red, u8 green, u8 blue)
{
    s32 index;

    for (index = 0; index < count; index++) {
        u32 primitive = primitive_source + (u32)index * 40u;
        m13o_sb(primitive + 4u, red);
        m13o_sb(primitive + 5u, green);
        m13o_sb(primitive + 6u, blue);
    }
}

s32 wm_80080A28(s32 slot_index)
{
    u32 context = m13o_lw(WM_M13O_CONTEXT_PTR);
    u32 slot = m13o_slot(slot_index);
    u32 second = context + 0x54u;
    u32 descriptor;

    (void)second;

    m13o_sw(slot + 0x28u, m13o_lw(WM_M13O_RESET_X));
#if defined(W34N91_MUTANT_WRONG_INITIAL_Y)
    m13o_sw(slot + 0x2Cu, 0u);
#else
    m13o_sw(slot + 0x2Cu, UINT32_C(0xFFF80000));
#endif
    m13o_sw(slot + 0x30u, m13o_lw(WM_M13O_RESET_Z));
    m13o_sw(slot + 0x50u, 0u);
    m13o_sw(slot + 0x54u, UINT32_C(0xFFFFF800));
    m13o_sw(slot + 0x58u, 128u);

    descriptor = m13o_lw(context + 0x40u);
    wm_8007A06C(context, m13o_lw(context + 0x48u),
                 m13o_lhu(descriptor + 4u));
#if !defined(W34N91_MUTANT_SKIP_SECOND_STREAM)
    descriptor = m13o_lw(second + 0x40u);
    wm_8007A06C(second, m13o_lw(second + 0x48u),
                 m13o_lhu(descriptor + 4u));
#endif
    return 3;
}

static void m13o_publish_position(u32 context, u32 slot)
{
#if defined(W34N91_MUTANT_WRONG_POSITION_SHIFT)
    u32 x = m13o_lw(slot + 0x28u) >> 16u;
#else
    u32 x = m13o_sra12(m13o_lw(slot + 0x28u));
#endif
    u32 y = m13o_sra12(m13o_lw(slot + 0x2Cu));
    u32 z = m13o_sra12(m13o_lw(slot + 0x30u));

    m13o_sw(context + 0x08u, x);
    m13o_sw(context + 0x0Cu, y);
    m13o_sw(context + 0x10u, z);
    m13o_sw(context + 0x5Cu, x);
    m13o_sw(context + 0x60u, y);
    m13o_sw(context + 0x64u, z);
}

static void m13o_scale_matrices(u32 context, u32 primary, u32 secondary)
{
    m13o_copy_words(context + 0x74u, WM_M13O_BASE_MATRIX, 8u);
    m13o_copy_words(context + 0x20u, context + 0x74u, 8u);

    m13o_sw(WM_M13O_SCALE_A + 0u, primary);
    m13o_sw(WM_M13O_SCALE_A + 4u, 4096u);
    m13o_sw(WM_M13O_SCALE_A + 8u, primary);
    m13o_sw(WM_M13O_SCALE_B + 0u, secondary);
    m13o_sw(WM_M13O_SCALE_B + 4u, 4096u);
    m13o_sw(WM_M13O_SCALE_B + 8u, secondary);
#if defined(W34N91_MUTANT_SWAP_SCALE_VECTORS)
    (void)ScaleMatrix((MATRIX *)PSX_ADDR(context + 0x20u),
                      (VECTOR *)PSX_ADDR(WM_M13O_SCALE_B));
    (void)ScaleMatrix((MATRIX *)PSX_ADDR(context + 0x74u),
                      (VECTOR *)PSX_ADDR(WM_M13O_SCALE_A));
#else
    (void)ScaleMatrix((MATRIX *)PSX_ADDR(context + 0x20u),
                      (VECTOR *)PSX_ADDR(WM_M13O_SCALE_A));
    (void)ScaleMatrix((MATRIX *)PSX_ADDR(context + 0x74u),
                      (VECTOR *)PSX_ADDR(WM_M13O_SCALE_B));
#endif
}

static u32 m13o_advance_phase(u32 phase)
{
#if defined(W34N91_MUTANT_WRONG_PHASE_STEP)
    phase += 383u;
#else
    phase += 384u;
#endif
    if (m13o_as_s32(phase) > 32767)
        phase = 32767u;
    return phase;
}

s32 wm_80080AC4(s32 slot_index)
{
    u32 context = m13o_lw(WM_M13O_CONTEXT_PTR);
    u32 slot = m13o_slot(slot_index);
    u32 second = context + 0x54u;
    u32 side;
    u32 brightness;
    u32 descriptor;
    u32 stream;

    if (m13o_lhu(slot + 0x04u) == 1u) {
#if defined(W34N91_MUTANT_SKIP_LATCH_CLEAR)
        m13o_sh(slot + 0x04u, m13o_lhu(slot + 0x04u));
#else
        m13o_sh(slot + 0x04u, 0u);
#endif
    }

    m13o_publish_position(context, slot);
    m13o_scale_matrices(context, m13o_lw(slot + 0x50u),
                         m13o_lw(slot + 0x54u));
    m13o_sw(slot + 0x50u, m13o_advance_phase(m13o_lw(slot + 0x50u)));
    m13o_sw(slot + 0x54u, m13o_advance_phase(m13o_lw(slot + 0x54u)));

    side = m13o_lw(WM_M13O_STREAM_SIDE);
#if defined(W34N91_MUTANT_WRONG_STREAM_SIDE)
    side ^= 1u;
#endif
    brightness = m13o_lw(slot + 0x58u);
    descriptor = m13o_lw(context + 0x40u);
    stream = m13o_lw(context + 0x48u + side * 4u);
    wm_800809EC(stream, (s32)m13o_lhu(descriptor + 4u),
                 (u8)brightness, (u8)brightness, (u8)brightness);
    descriptor = m13o_lw(second + 0x40u);
    stream = m13o_lw(second + 0x48u + side * 4u);
    wm_800809EC(stream, (s32)m13o_lhu(descriptor + 4u),
                 (u8)brightness, (u8)brightness, (u8)brightness);

#if defined(W34N91_MUTANT_WRONG_FADE_STEP)
    brightness -= 3u;
#else
    brightness -= 4u;
#endif
    m13o_sw(slot + 0x58u, brightness);
    if (m13o_as_s32(brightness) < 0) {
        m13o_sw(slot + 0x58u, 0u);
        return 3;
    }
    return 1;
}
