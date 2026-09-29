/* Exact native transcription of retail [0x8007EBBC,0x8007F8AC). */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_callback_7eca4.h"
#include "world_map_common_tail.h"
#include "world_map_helper_848b4.h"
#include "world_map_helper_93354.h"
#include "world_map_helper_97070.h"
#include "world_map_helper_97770.h"
#include "world_map_terrain_sampler.h"

extern u16 GetTPage(int tp, int abr, int x, int y);
extern u16 GetClut(int x, int y);
extern long VectorNormal(VECTOR *input, VECTOR *output);
extern void OuterProduct12(VECTOR *left, VECTOR *right, VECTOR *output);

#define WM_M15O_POOL_PTR       UINT32_C(0x8009BE24)
#define WM_M15O_CONTEXT_PTR    UINT32_C(0x8009C620)
#define WM_M15O_RESET_POS      UINT32_C(0x8009C5AC)
#define WM_M15O_MODE_SELECTOR  UINT32_C(0x8009D3D4)
#define WM_M15O_TARGET_TABLE   UINT32_C(0x8009A674)

#define WM_M15O_SC_VECTOR_A    UINT32_C(0x1F800000)
#define WM_M15O_SC_VECTOR_B    UINT32_C(0x1F800010)
#define WM_M15O_SC_VECTOR_C    UINT32_C(0x1F800020)
#define WM_M15O_SC_POSITION    UINT32_C(0x1F8000A0)
#define WM_M15O_SC_ANGLES      UINT32_C(0x1F8000A8)
#define WM_M15O_SC_MATRIX_A    UINT32_C(0x1F8000F0)
#define WM_M15O_SC_MATRIX_B    UINT32_C(0x1F800110)
#define WM_M15O_SC_MATRIX_C    UINT32_C(0x1F800150)

static s16 m15o_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u16 m15o_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 m15o_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u8 m15o_lbu(u32 address)
{
    u8 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void m15o_sb(u32 address, u8 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void m15o_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void m15o_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static s32 m15o_as_s32(u32 bits)
{
    s32 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static u32 m15o_sra(u32 bits, unsigned shift)
{
    u32 result = bits >> shift;
    if ((bits & UINT32_C(0x80000000)) != 0u)
        result |= ~(UINT32_MAX >> shift);
    return result;
}

static u32 m15o_slot(s32 slot_index)
{
    return m15o_lw(WM_M15O_POOL_PTR) + ((u32)slot_index << 7u);
}

static u32 m15o_scaled_107x128(u32 value)
{
#if defined(W34N100_MUTANT_WRONG_INIT_DISTANCE)
    return (value * 106u) << 7u;
#else
    return (value * 107u) << 7u;
#endif
}

static u32 m15o_scaled_45x256(u32 value)
{
    return (value * 45u) << 8u;
}

static u32 m15o_scaled_15x512(u32 value)
{
    return (value * 15u) << 9u;
}

static u32 m15o_scaled_51x256(u32 value)
{
    return (value * 51u) << 8u;
}

static void m15o_transpose_rotation(u32 input, u32 output)
{
    u16 m00 = m15o_lhu(input + 0x00u);
    u16 m01 = m15o_lhu(input + 0x02u);
    u16 m02 = m15o_lhu(input + 0x04u);
    u16 m10 = m15o_lhu(input + 0x06u);
    u16 m11 = m15o_lhu(input + 0x08u);
    u16 m12 = m15o_lhu(input + 0x0Au);
    u16 m20 = m15o_lhu(input + 0x0Cu);
    u16 m21 = m15o_lhu(input + 0x0Eu);
    u16 m22 = m15o_lhu(input + 0x10u);

    m15o_sh(output + 0x00u, m00);
    m15o_sh(output + 0x02u, m10);
    m15o_sh(output + 0x04u, m20);
    m15o_sh(output + 0x06u, m01);
    m15o_sh(output + 0x08u, m11);
    m15o_sh(output + 0x0Au, m21);
    m15o_sh(output + 0x0Cu, m02);
    m15o_sh(output + 0x0Eu, m12);
    m15o_sh(output + 0x10u, m22);
}

void wm_8007EBBC(u32 owner, u32 primitive_source, u16 count, u32 abr)
{
    u32 index;

    for (index = 0u; index < (u32)count; index++) {
        u32 primitive = primitive_source + index * 40u;
        u8 code;

        m15o_sb(primitive + 3u, 9u);
        m15o_sb(primitive + 7u, 44u);
#if defined(W34N100_MUTANT_WRONG_TPAGE)
        m15o_sh(primitive + 22u, GetTPage(0, (int)(s32)abr, 767, 256));
#else
        m15o_sh(primitive + 22u, GetTPage(0, (int)(s32)abr, 768, 256));
#endif
        m15o_sh(primitive + 14u, GetClut(0, 511));
        m15o_sb(primitive + 4u, 192u);
        m15o_sb(primitive + 5u, 60u);
        m15o_sb(primitive + 6u, 60u);
        code = m15o_lbu(primitive + 7u);
        m15o_sb(primitive + 7u, (u8)(code | 2u));
    }

    memcpy(PSX_ADDR(m15o_lw(owner + 0x4Cu)),
           PSX_ADDR(m15o_lw(owner + 0x48u)), (size_t)count * 40u);
}

static void m15o_initialize_stream(u32 context, u32 owner_offset,
                                   u32 descriptor_offset, u32 source_offset,
                                   u32 abr)
{
    u32 descriptor = m15o_lw(context + descriptor_offset);
    wm_8007EBBC(context + owner_offset, m15o_lw(context + source_offset),
                 m15o_lhu(descriptor + 4u), abr);
}

s32 wm_8007ECA4(s32 slot_index)
{
    u32 context;
    u32 slot;
    u32 selector;
    u32 vx;
    u32 vz;

    wm_800848B4(1, 2);
#if !defined(W34N100_MUTANT_SKIP_THIRD_LINK)
    wm_800848B4(1, 3);
#endif
    context = m15o_lw(WM_M15O_CONTEXT_PTR);
    slot = m15o_slot(slot_index);

    m15o_initialize_stream(context, 0x54u, 0x94u, 0x9Cu, 3u);
    m15o_initialize_stream(context, 0xA8u, 0xE8u, 0xF0u, 3u);
    m15o_initialize_stream(context, 0xFCu, 0x13Cu, 0x144u, 1u);

    m15o_sh(context + 0x54u, 0u);
    m15o_sh(context + 0x6Cu, 0u);
    m15o_sh(context + 0x6Eu, 0u);
    m15o_sh(context + 0x70u, 0u);
    (void)RotMatrixYXZ((SVECTOR *)PSX_ADDR(context + 0x6Cu),
                       (MATRIX *)PSX_ADDR(context + 0x74u));

    vx = UINT32_C(0xFFFFF7A6);
    vz = UINT32_C(0x00000DA6);
    m15o_sh(slot + 0x20u, 0u);
    m15o_sw(slot + 0x38u, vx);
    m15o_sw(slot + 0x3Cu, 0u);
    m15o_sw(slot + 0x40u, vz);
    selector = m15o_lw(WM_M15O_MODE_SELECTOR) << 3u;
#if defined(W34N100_MUTANT_WRONG_TARGET_STRIDE)
    selector = m15o_lw(WM_M15O_MODE_SELECTOR) << 2u;
#endif
    m15o_sw(slot + 0x50u,
             (u32)(s32)m15o_lh(WM_M15O_TARGET_TABLE + selector) << 12u);
    m15o_sw(slot + 0x54u,
             (u32)(s32)m15o_lh(WM_M15O_TARGET_TABLE + selector + 4u) << 12u);
    m15o_sw(slot + 0x28u,
             m15o_lw(WM_M15O_RESET_POS) - m15o_scaled_107x128(vx));
    m15o_sw(slot + 0x2Cu, m15o_lw(WM_M15O_RESET_POS + 4u));
    m15o_sw(slot + 0x30u,
             m15o_lw(WM_M15O_RESET_POS + 8u) -
                 m15o_scaled_107x128(vz));
    return 1;
}

static void m15o_set_position_from_velocity(u32 slot, u16 state,
                                            u32 extra_x, u32 extra_z)
{
    u32 vx = m15o_lw(slot + 0x38u);
    u32 vz = m15o_lw(slot + 0x40u);

    m15o_sh(slot + 4u, 0u);
    m15o_sh(slot + 0x20u, state);
    m15o_sw(slot + 0x28u,
             m15o_lw(WM_M15O_RESET_POS) - m15o_scaled_107x128(vx) +
                 extra_x);
    m15o_sw(slot + 0x30u,
             m15o_lw(WM_M15O_RESET_POS + 8u) -
                 m15o_scaled_107x128(vz) + extra_z);
}

static void m15o_consume_latch(u32 slot)
{
    switch (m15o_lhu(slot + 4u)) {
    case 1u:
        m15o_set_position_from_velocity(slot, 0u, 0u, 0u);
        break;
    case 2u: {
        u32 vx = m15o_lw(slot + 0x38u);
        u32 vz = m15o_lw(slot + 0x40u);
        m15o_set_position_from_velocity(slot, 0u,
                                        m15o_scaled_45x256(vx),
                                        m15o_scaled_45x256(vz));
        break;
    }
    case 3u: {
        u32 vx = m15o_lw(slot + 0x38u);
        u32 vz = m15o_lw(slot + 0x40u);
        m15o_set_position_from_velocity(slot, 0u,
                                        m15o_scaled_15x512(vx),
                                        m15o_scaled_15x512(vz));
        break;
    }
    case 4u: {
        u32 vx = m15o_lw(slot + 0x38u);
        u32 vz = m15o_lw(slot + 0x40u);
        m15o_set_position_from_velocity(slot, 1u,
                                        m15o_scaled_51x256(vx),
                                        m15o_scaled_51x256(vz));
        break;
    }
    case 5u:
        m15o_sh(slot + 4u, 0u);
        m15o_sh(slot + 0x20u, 2u);
        m15o_sw(slot + 0x38u, UINT32_C(0xFFFFF678));
        m15o_sw(slot + 0x3Cu, 551u);
        m15o_sw(slot + 0x40u, UINT32_C(0xFFFFF355));
        m15o_sw(slot + 0x28u, UINT32_C(0x01F9E000));
        m15o_sw(slot + 0x30u, UINT32_C(0x05998000));
        break;
    case 16u: {
        u32 vx = m15o_lw(slot + 0x38u);
        u32 vz = m15o_lw(slot + 0x40u);
        m15o_sh(slot + 4u, 0u);
        m15o_sh(slot + 0x20u, 16u);
        m15o_sw(slot + 0x28u, UINT32_C(0x056F2000) +
                 m15o_scaled_51x256(vx));
        m15o_sw(slot + 0x30u, UINT32_C(0x07F2C000) +
                 m15o_scaled_51x256(vz));
        break;
    }
    case 24u: {
        u32 vx = m15o_lw(slot + 0x38u);
        u32 vz = m15o_lw(slot + 0x40u);
        m15o_set_position_from_velocity(slot, 24u,
                                        m15o_scaled_51x256(vx),
                                        m15o_scaled_51x256(vz));
        break;
    }
    default:
        break;
    }
}

static void m15o_mark_context_objects(u32 context)
{
    m15o_sh(context + 0x54u, 1u);
    m15o_sh(context + 0xA8u, 1u);
    m15o_sh(context + 0xFCu, 1u);
}

static void m15o_claim_range(u32 state)
{
#if defined(W34N100_MUTANT_SKIP_CLAIM_RANGE)
    (void)state;
#else
    u32 record;
    for (record = 4u; record <= 8u; record++)
        (void)wm_80097770(record, (s32)state);
#endif
}

static void m15o_set_marker_position(u32 slot)
{
    m15o_sh(WM_M15O_SC_POSITION + 0u,
             (u16)m15o_sra(m15o_lw(slot + 0x28u), 12u));
    m15o_sh(WM_M15O_SC_POSITION + 2u,
             (u16)m15o_sra(m15o_lw(slot + 0x2Cu), 12u));
    m15o_sh(WM_M15O_SC_POSITION + 4u,
             (u16)m15o_sra(m15o_lw(slot + 0x30u), 12u));
}

static s32 m15o_apply_transition_state(u32 slot, u32 context)
{
    s16 state = m15o_lh(slot + 0x20u);
    s32 result = 1;

    switch (state) {
    case 0:
        if (m15o_as_s32(m15o_lw(slot + 0x28u)) <
                m15o_as_s32(m15o_lw(slot + 0x50u)) &&
            m15o_as_s32(m15o_lw(slot + 0x54u)) <
                m15o_as_s32(m15o_lw(slot + 0x30u))) {
            m15o_sw(slot + 0x28u, m15o_lw(slot + 0x50u));
            m15o_sw(slot + 0x30u, m15o_lw(slot + 0x54u));
            m15o_claim_range(2u);
        }
        break;
    case 1:
#if defined(W34N100_MUTANT_WRONG_STATE1_LIMIT)
        if (m15o_as_s32(m15o_lw(slot + 0x28u)) <= INT32_C(0x01F9E000) &&
#else
        if (m15o_as_s32(m15o_lw(slot + 0x28u)) <= INT32_C(0x01F9DFFF) &&
#endif
            m15o_as_s32(m15o_lw(slot + 0x30u)) > INT32_C(0x05998000)) {
            m15o_sw(slot + 0x28u, UINT32_C(0x01F9E000));
            m15o_sw(slot + 0x30u, UINT32_C(0x05998000));
            m15o_sh(slot + 0x20u, 64u);
            m15o_set_marker_position(slot);
            wm_80089160(32u, WM_M15O_SC_POSITION, 0u);
        }
        break;
    case 2:
        if (m15o_as_s32(m15o_lw(slot + 0x2Cu)) > -INT32_C(0x00080000)) {
            s32 height = wm_80093978(INT32_C(0x01498000),
                                     INT32_C(0x04AF2000));
            m15o_sh(WM_M15O_SC_POSITION + 0u, 5272u);
            m15o_sh(WM_M15O_SC_POSITION + 2u,
                     (u16)m15o_sra((u32)height, 12u));
            m15o_sh(WM_M15O_SC_POSITION + 4u, 19186u);
            wm_80089160(37u, WM_M15O_SC_POSITION, 0u);
            wm_80089160(38u, WM_M15O_SC_POSITION, 0u);
            wm_80089160(39u, WM_M15O_SC_POSITION, 0u);
            m15o_sh(slot + 0x20u, 3u);
        }
        break;
    case 3:
        if (m15o_as_s32(m15o_lw(slot + 0x2Cu)) > INT32_C(0x00100000)) {
            m15o_sw(slot + 0x28u, m15o_lw(slot + 0x28u) -
                     (m15o_lw(slot + 0x38u) << 7u));
            m15o_sw(slot + 0x2Cu, m15o_lw(slot + 0x2Cu) -
                     (m15o_lw(slot + 0x3Cu) << 7u));
            m15o_sw(slot + 0x30u, m15o_lw(slot + 0x30u) -
                     (m15o_lw(slot + 0x40u) << 7u));
            m15o_mark_context_objects(context);
            m15o_sh(slot + 0x20u, 65u);
        }
        break;
    case 16:
        if (m15o_as_s32(m15o_lw(slot + 0x28u)) <= INT32_C(0x01F9DFFF) &&
            m15o_as_s32(m15o_lw(slot + 0x30u)) > INT32_C(0x05998000)) {
            m15o_sh(slot + 0x20u, 17u);
            m15o_sh(WM_M15O_SC_POSITION + 0u, 8094u);
            m15o_sh(WM_M15O_SC_POSITION + 2u,
                     (u16)m15o_sra(m15o_lw(slot + 0x2Cu), 12u));
            m15o_sh(WM_M15O_SC_POSITION + 4u, 22936u);
            wm_80089160(32u, WM_M15O_SC_POSITION, 0u);
            wm_80089160(33u, WM_M15O_SC_POSITION, 0u);
        }
        break;
    case 17:
        if (m15o_as_s32(m15o_lw(slot + 0x28u)) <= INT32_C(0x0199CFFF) &&
            m15o_as_s32(m15o_lw(slot + 0x30u)) > INT32_C(0x06367000)) {
            m15o_sw(slot + 0x28u, UINT32_C(0x0199D000));
            m15o_sw(slot + 0x30u, UINT32_C(0x06367000));
            m15o_mark_context_objects(context);
            m15o_claim_range(5u);
            result = 3;
        }
        break;
    case 24:
        if (m15o_as_s32(m15o_lw(slot + 0x28u)) <= INT32_C(0x01EB4FFF) &&
            m15o_as_s32(m15o_lw(slot + 0x30u)) > INT32_C(0x05EE6000)) {
            m15o_mark_context_objects(context);
            m15o_claim_range(2u);
            m15o_sh(slot + 0x20u, 65u);
        }
        break;
    case 64:
        m15o_sw(slot + 0x28u, UINT32_C(0x01F9E000));
        m15o_sw(slot + 0x30u, UINT32_C(0x05998000));
        break;
    case 65:
        result = 3;
        break;
    default:
        break;
    }
    return result;
}

static void m15o_publish_position(u32 slot, u32 owner)
{
    wm_80093354(slot + 0x28u);
    m15o_sw(owner + 0x08u, m15o_sra(m15o_lw(slot + 0x28u), 12u));
    m15o_sw(owner + 0x0Cu, m15o_sra(m15o_lw(slot + 0x2Cu), 12u));
    m15o_sw(owner + 0x10u, m15o_sra(m15o_lw(slot + 0x30u), 12u));
}

static void m15o_seed_scale(void)
{
    m15o_sw(WM_M15O_SC_VECTOR_A + 0u, 2048u);
    m15o_sw(WM_M15O_SC_VECTOR_A + 4u, 2048u);
    m15o_sw(WM_M15O_SC_VECTOR_A + 8u, 6144u);
}

static void m15o_render_fixed(u32 slot, u32 owner)
{
    u16 yaw;

#if defined(W34N100_MUTANT_WRONG_FIXED_MATRIX)
    m15o_sh(WM_M15O_SC_MATRIX_A + 0x00u, 3493u);
#else
    m15o_sh(WM_M15O_SC_MATRIX_A + 0x00u, 3494u);
#endif
    m15o_sh(WM_M15O_SC_MATRIX_A + 0x02u, 0u);
    m15o_sh(WM_M15O_SC_MATRIX_A + 0x04u, 2138u);
    m15o_sh(WM_M15O_SC_MATRIX_A + 0x06u, 0u);
    m15o_sh(WM_M15O_SC_MATRIX_A + 0x08u, (u16)(s16)-4096);
    m15o_sh(WM_M15O_SC_MATRIX_A + 0x0Au, 0u);
    m15o_sh(WM_M15O_SC_MATRIX_A + 0x0Cu, (u16)(s16)-2138);
    m15o_sh(WM_M15O_SC_MATRIX_A + 0x0Eu, 0u);
    m15o_sh(WM_M15O_SC_MATRIX_A + 0x10u, 3494u);

    m15o_sh(owner + 0x1Cu, (u16)((m15o_lhu(owner + 0x1Cu) + 256u) &
                                  0x0FFFu));
    (void)RotMatrixYXZ((SVECTOR *)PSX_ADDR(owner + 0x18u),
                       (MATRIX *)PSX_ADDR(WM_M15O_SC_MATRIX_B));
    (void)MulMatrix0((MATRIX *)PSX_ADDR(WM_M15O_SC_MATRIX_A),
                     (MATRIX *)PSX_ADDR(WM_M15O_SC_MATRIX_B),
                     (MATRIX *)PSX_ADDR(owner + 0x20u));
    m15o_seed_scale();
    (void)ScaleMatrix((MATRIX *)PSX_ADDR(owner + 0x20u),
                      (VECTOR *)PSX_ADDR(WM_M15O_SC_VECTOR_A));
    m15o_set_marker_position(slot);
    m15o_sh(WM_M15O_SC_ANGLES + 0u, 0u);
    m15o_sh(WM_M15O_SC_ANGLES + 4u, 0u);
    yaw = (u16)((u32)ratan2(2138, 3494) & 0x0FFFu);
    m15o_sh(WM_M15O_SC_ANGLES + 2u, yaw);
    wm_80089160(34u, WM_M15O_SC_POSITION, WM_M15O_SC_ANGLES);
}

static void m15o_render_aligned(u32 slot, u32 owner)
{
    u32 matrix = WM_M15O_SC_MATRIX_C;

    m15o_sw(WM_M15O_SC_VECTOR_A + 0u, 2440u);
    m15o_sw(WM_M15O_SC_VECTOR_A + 4u, UINT32_C(0xFFFFFDD9));
    m15o_sw(WM_M15O_SC_VECTOR_A + 8u, UINT32_C(0xFFFFF355));
    m15o_sw(WM_M15O_SC_VECTOR_B + 0u, 0u);
    m15o_sw(WM_M15O_SC_VECTOR_B + 4u, 4096u);
    m15o_sw(WM_M15O_SC_VECTOR_B + 8u, 0u);
    OuterProduct12((VECTOR *)PSX_ADDR(WM_M15O_SC_VECTOR_B),
                   (VECTOR *)PSX_ADDR(WM_M15O_SC_VECTOR_A),
                   (VECTOR *)PSX_ADDR(WM_M15O_SC_VECTOR_C));
    (void)VectorNormal((VECTOR *)PSX_ADDR(WM_M15O_SC_VECTOR_C),
                       (VECTOR *)PSX_ADDR(WM_M15O_SC_VECTOR_C));
    OuterProduct12((VECTOR *)PSX_ADDR(WM_M15O_SC_VECTOR_A),
                   (VECTOR *)PSX_ADDR(WM_M15O_SC_VECTOR_C),
                   (VECTOR *)PSX_ADDR(WM_M15O_SC_VECTOR_B));
    (void)VectorNormal((VECTOR *)PSX_ADDR(WM_M15O_SC_VECTOR_B),
                       (VECTOR *)PSX_ADDR(WM_M15O_SC_VECTOR_B));

    m15o_sh(matrix + 0x00u, m15o_lhu(WM_M15O_SC_VECTOR_C + 0u));
    m15o_sh(matrix + 0x02u, m15o_lhu(WM_M15O_SC_VECTOR_C + 4u));
    m15o_sh(matrix + 0x04u, m15o_lhu(WM_M15O_SC_VECTOR_C + 8u));
    m15o_sh(matrix + 0x06u, m15o_lhu(WM_M15O_SC_VECTOR_B + 0u));
    m15o_sh(matrix + 0x08u, m15o_lhu(WM_M15O_SC_VECTOR_B + 4u));
    m15o_sh(matrix + 0x0Au, m15o_lhu(WM_M15O_SC_VECTOR_B + 8u));
    m15o_sh(matrix + 0x0Cu, m15o_lhu(WM_M15O_SC_VECTOR_A + 0u));
    m15o_sh(matrix + 0x0Eu, m15o_lhu(WM_M15O_SC_VECTOR_A + 4u));
    m15o_sh(matrix + 0x10u, m15o_lhu(WM_M15O_SC_VECTOR_A + 8u));

    wm_80097070(matrix, WM_M15O_SC_ANGLES);
    m15o_transpose_rotation(matrix, WM_M15O_SC_MATRIX_A);
    m15o_sh(owner + 0x1Cu, (u16)((m15o_lhu(owner + 0x1Cu) + 256u) &
                                  0x0FFFu));
    (void)RotMatrixYXZ((SVECTOR *)PSX_ADDR(owner + 0x18u),
                       (MATRIX *)PSX_ADDR(WM_M15O_SC_MATRIX_B));
    (void)MulMatrix0((MATRIX *)PSX_ADDR(WM_M15O_SC_MATRIX_A),
                     (MATRIX *)PSX_ADDR(WM_M15O_SC_MATRIX_B),
                     (MATRIX *)PSX_ADDR(owner + 0x20u));
    m15o_seed_scale();
    (void)ScaleMatrix((MATRIX *)PSX_ADDR(owner + 0x20u),
                      (VECTOR *)PSX_ADDR(WM_M15O_SC_VECTOR_A));
    m15o_set_marker_position(slot);
    wm_80089160(34u, WM_M15O_SC_POSITION, WM_M15O_SC_ANGLES);
}

static void m15o_render_for_state(u32 slot, u32 owner)
{
    switch (m15o_lh(slot + 0x20u)) {
    case 0:
    case 1:
    case 16:
    case 24:
        m15o_render_fixed(slot, owner);
        break;
    case 2:
    case 3:
        m15o_render_aligned(slot, owner);
        break;
    case 4:
    case 17:
    case 65:
#if !defined(W34N100_MUTANT_SKIP_EFFECT_RELEASE)
        wm_800894C8(34u);
#endif
        break;
    default:
        break;
    }
}

s32 wm_8007EE34(s32 slot_index)
{
    u32 slot = m15o_slot(slot_index);
    u32 context = m15o_lw(WM_M15O_CONTEXT_PTR);
    u32 owner = context + 0x54u;
    s32 result;

    m15o_consume_latch(slot);
    m15o_sw(slot + 0x28u,
             m15o_lw(slot + 0x28u) + (m15o_lw(slot + 0x38u) << 7u));
    m15o_sw(slot + 0x2Cu,
             m15o_lw(slot + 0x2Cu) + (m15o_lw(slot + 0x3Cu) << 7u));
    m15o_sw(slot + 0x30u,
             m15o_lw(slot + 0x30u) + (m15o_lw(slot + 0x40u) << 7u));
    result = m15o_apply_transition_state(slot, context);
    m15o_publish_position(slot, owner);
    m15o_render_for_state(slot, owner);
    return result;
}
