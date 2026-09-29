/* Exact native transcription of retail [0x80082F64, 0x800834D0). */
#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "world_map_callback_83214.h"

#define WM_M17P_POOL_PTR      UINT32_C(0x8009BE24)
#define WM_M17P_CONTEXT_PTR   UINT32_C(0x8009C620)
#define WM_M17P_BUFFER_SIDE   UINT32_C(0x8009D7F0)
#define WM_M17P_TABLE_STATE_1 UINT32_C(0x8009AABC)
#define WM_M17P_TABLE_STATE_2 UINT32_C(0x8009AB48)
#define WM_M17P_TABLE_STATE_4 UINT32_C(0x8009ABD4)
#define WM_M17P_SCRATCH       UINT32_C(0x1F800000)
#define WM_M17P_SC_VECTOR     (WM_M17P_SCRATCH + UINT32_C(0x000))
#define WM_M17P_SC_ANGLES     (WM_M17P_SCRATCH + UINT32_C(0x0A0))
#define WM_M17P_SC_MATRIX     (WM_M17P_SCRATCH + UINT32_C(0x0F0))

#define WM_M17P_OBJECT_BASE   UINT32_C(0x1998)
#define WM_M17P_ACTIVE_BASE   UINT32_C(0x1A94)
#define WM_M17P_OBJECT_STRIDE UINT32_C(84)
#define WM_M17P_PRIM_STRIDE   UINT32_C(32)

static s16 m17p_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u16 m17p_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 m17p_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void m17p_sb(u32 address, u8 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void m17p_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void m17p_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static s32 m17p_as_s32(u32 bits)
{
    s32 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static u32 m17p_slot(s32 slot_index)
{
    return m17p_lw(WM_M17P_POOL_PTR) + ((u32)slot_index << 7u);
}

static u32 m17p_object_for_init(s32 slot_index)
{
#if defined(W34N112_MUTANT_WRONG_OBJECT_STRIDE)
    return m17p_lw(WM_M17P_CONTEXT_PTR) + WM_M17P_OBJECT_BASE +
           (u32)slot_index * UINT32_C(80);
#else
    return m17p_lw(WM_M17P_CONTEXT_PTR) + WM_M17P_OBJECT_BASE +
           (u32)slot_index * WM_M17P_OBJECT_STRIDE;
#endif
}

static u32 m17p_object_for_update(s32 slot_index)
{
    return m17p_lw(WM_M17P_CONTEXT_PTR) + WM_M17P_ACTIVE_BASE +
           (u32)(slot_index - 3) * WM_M17P_OBJECT_STRIDE;
}

/* Retail 0x80083108. */
static void m17p_init_primitives(u32 owner, u32 buffer, u32 count, u32 abr)
{
    u32 index;

    (void)owner;
#if defined(W34N112_MUTANT_DROP_LAST_PRIMITIVE)
    if (count != 0u)
        count--;
#endif
    for (index = 0u; index < count; index++) {
        u32 primitive = buffer + index * WM_M17P_PRIM_STRIDE;
        u8 code;

        m17p_sb(primitive + 3u, 7u);
#if defined(W34N112_MUTANT_WRONG_PRIMITIVE_CODE)
        m17p_sb(primitive + 7u, 32u);
#else
        m17p_sb(primitive + 7u, 36u);
#endif
        m17p_sh(primitive + 22u, GetTPage(0, (int)abr, 704, 256));
        m17p_sb(primitive + 4u, 0u);
        m17p_sb(primitive + 5u, 0u);
        m17p_sb(primitive + 6u, 0u);
        code = *((u8 *)PSX_ADDR(primitive + 7u));
        m17p_sb(primitive + 7u, (u8)(code | 2u));
    }
#if !defined(W34N112_MUTANT_SKIP_BUFFER_MIRROR)
    memcpy(PSX_ADDR(m17p_lw(owner + 0x4Cu)),
           PSX_ADDR(m17p_lw(owner + 0x48u)),
           (size_t)count * (size_t)WM_M17P_PRIM_STRIDE);
#endif
}

/* Retail 0x800831D8. */
static void m17p_write_colors(u32 buffer, u32 count,
                              u32 red, u32 green, u32 blue)
{
    u32 index;

    for (index = 0u; index < count; index++) {
        u32 primitive = buffer + index * WM_M17P_PRIM_STRIDE;
        m17p_sb(primitive + 4u, (u8)red);
        m17p_sb(primitive + 5u, (u8)green);
        m17p_sb(primitive + 6u, (u8)blue);
    }
}

static void m17p_clamp_channel(u32 slot, u32 value_offset,
                               u32 velocity_offset, s32 lower)
{
    u32 bits = m17p_lw(slot + value_offset) +
               m17p_lw(slot + velocity_offset);
    s32 value = m17p_as_s32(bits);

    m17p_sw(slot + value_offset, bits);
    if (value >= 256) {
        m17p_sw(slot + value_offset, 255u);
        m17p_sw(slot + velocity_offset,
                 0u - m17p_lw(slot + velocity_offset));
    } else if (value < lower) {
#if defined(W34N112_MUTANT_WRONG_RED_LOWER_CLAMP)
        if (value_offset == 0x50u)
            m17p_sw(slot + value_offset, 63u);
        else
#endif
            m17p_sw(slot + value_offset, (u32)lower);
        m17p_sw(slot + velocity_offset,
                 0u - m17p_lw(slot + velocity_offset));
    }
}

/* Retail 0x80082F64. */
static void m17p_update_transform(u32 slot, u32 object)
{
    s16 active = m17p_lh(slot + 0x20u);
    s32 cosine;
    u32 angle;

    (void)object;
    if (active != 1)
        return;

    m17p_sh(WM_M17P_SC_ANGLES + 0u, 0u);
    m17p_sh(WM_M17P_SC_ANGLES + 2u,
             (u16)m17p_lw(slot + 0x68u));
    m17p_sh(WM_M17P_SC_ANGLES + 4u, 0u);
    m17p_sw(WM_M17P_SC_VECTOR + 0u,
             m17p_lw(slot + 0x70u) & UINT32_C(0x7FFF));
    m17p_sw(WM_M17P_SC_VECTOR + 4u, 4096u);
    cosine = (s32)rcos((int)m17p_lw(slot + 0x6Cu));
    m17p_sw(WM_M17P_SC_VECTOR + 8u, (u32)(cosine * 6));

    (void)RotMatrix((SVECTOR *)PSX_ADDR(WM_M17P_SC_ANGLES),
                    (MATRIX *)PSX_ADDR(WM_M17P_SC_MATRIX));
    (void)ScaleMatrix((MATRIX *)PSX_ADDR(WM_M17P_SC_MATRIX),
                      (VECTOR *)PSX_ADDR(WM_M17P_SC_VECTOR));
#if !defined(W34N112_MUTANT_SKIP_MATRIX_COPY)
    memcpy(PSX_ADDR(object + 0x20u), PSX_ADDR(WM_M17P_SC_MATRIX), 32u);
#endif

    m17p_clamp_channel(slot, 0x50u, 0x5Cu, 64);
    m17p_clamp_channel(slot, 0x54u, 0x60u, 128);
    angle = m17p_lw(slot + 0x6Cu) + 8u;
#if defined(W34N112_MUTANT_WRONG_ANGLE_MASK)
    angle &= UINT32_C(0x07FF);
#else
    angle &= UINT32_C(0x0FFF);
#endif
    m17p_sw(slot + 0x6Cu, angle);
    m17p_sw(slot + 0x70u,
             m17p_lw(slot + 0x70u) + m17p_lw(slot + 0x74u));
}

static void m17p_load_state(u32 slot, u32 object, u32 table,
                            u32 group)
{
    u32 source = table + group * 28u;
    u32 index;

    m17p_sh(slot + 0x20u, 1u);
    m17p_sh(slot + 0x04u, 0u);
    m17p_sw(object + 0x08u, (u32)(s32)m17p_lh(source + 0u));
    m17p_sw(object + 0x0Cu, (u32)(s32)m17p_lh(source + 2u));
    m17p_sw(object + 0x10u, (u32)(s32)m17p_lh(source + 4u));
    for (index = 3u; index < 14u; index++) {
#if defined(W34N112_MUTANT_SHORT_PARAMETER_COPY)
        if (index == 13u)
            break;
#endif
        m17p_sw(slot + 0x50u + (index - 3u) * 4u,
                 (u32)(s32)m17p_lh(source + index * 2u));
    }
}

s32 wm_80083214(s32 slot_index)
{
    u32 object = m17p_object_for_init(slot_index);
    u32 descriptor = m17p_lw(object + 0x40u);
    u32 count = (u32)m17p_lhu(descriptor + 4u);

    m17p_init_primitives(object, m17p_lw(object + 0x48u), count, 3u);
    return 1;
}

s32 wm_80083264(s32 slot_index)
{
    u32 slot = m17p_slot(slot_index);
    u32 group = (u32)(slot_index - 3);
    u32 object = m17p_object_for_update(slot_index);
    s16 state = m17p_lh(slot + 0x04u);
    u32 descriptor;
    u32 side;
    u32 buffer;

    switch (state) {
    case 1:
        m17p_load_state(slot, object, WM_M17P_TABLE_STATE_1, group);
        break;
    case 2:
#if defined(W34N112_MUTANT_STATE2_USES_STATE1_TABLE)
        m17p_load_state(slot, object, WM_M17P_TABLE_STATE_1, group);
#else
        m17p_load_state(slot, object, WM_M17P_TABLE_STATE_2, group);
#endif
        break;
    case 3:
        m17p_sh(slot + 0x04u, 0u);
        m17p_sh(slot + 0x20u, 0u);
        break;
    case 4:
        m17p_load_state(slot, object, WM_M17P_TABLE_STATE_4, group);
        break;
    default:
        break;
    }
#if defined(W34N112_MUTANT_SKIP_ACTIVE_FLAG)
    m17p_sh(slot + 0x20u, 0u);
#endif

    m17p_update_transform(slot, object);
    descriptor = m17p_lw(object + 0x40u);
    side = m17p_lw(WM_M17P_BUFFER_SIDE);
#if defined(W34N112_MUTANT_WRONG_BUFFER_SIDE)
    side ^= 1u;
#endif
    buffer = m17p_lw(object + 0x48u + side * 4u);
    m17p_write_colors(buffer, (u32)m17p_lhu(descriptor + 4u),
                      m17p_lw(slot + 0x50u),
                      m17p_lw(slot + 0x54u),
                      m17p_lw(slot + 0x58u));
    return 1;
}
