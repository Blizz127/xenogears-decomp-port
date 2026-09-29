/* Exact native transcription of retail [0x8007F8AC,0x8007FC8C). */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_callback_7eca4.h"
#include "world_map_callback_7f8ac.h"
#include "world_map_common_tail.h"

#define WM_M15T_POOL_PTR       UINT32_C(0x8009BE24)
#define WM_M15T_CONTEXT_PTR    UINT32_C(0x8009C620)
#define WM_M15T_SCALE_TABLE    UINT32_C(0x8009A684)

#define WM_M15T_SC_SCALE       UINT32_C(0x1F800000)
#define WM_M15T_SC_POSITION    UINT32_C(0x1F8000A0)
#define WM_M15T_SC_ANGLES      UINT32_C(0x1F8000A8)
#define WM_M15T_SC_MATRIX_A    UINT32_C(0x1F8000F0)
#define WM_M15T_SC_MATRIX_B    UINT32_C(0x1F800110)

static s16 m15t_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u16 m15t_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 m15t_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void m15t_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void m15t_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static s32 m15t_as_s32(u32 bits)
{
    s32 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static u32 m15t_sra12(u32 bits)
{
    u32 shifted = bits >> 12u;

    if ((bits & UINT32_C(0x80000000)) != 0u)
        shifted |= UINT32_C(0xFFF00000);
    return shifted;
}

static u32 m15t_slot(s32 slot_index)
{
    return m15t_lw(WM_M15T_POOL_PTR) + ((u32)slot_index << 7u);
}

static u32 m15t_owner(u32 context, s32 slot_index)
{
#if defined(W34N101_MUTANT_WRONG_OWNER_STRIDE)
    return context + (u32)slot_index * 80u;
#else
    return context + (u32)slot_index * 84u;
#endif
}

static u32 m15t_scale_for_slot(s32 slot_index)
{
    return (u32)m15t_lhu(WM_M15T_SCALE_TABLE + (u32)slot_index * 2u);
}

s32 wm_8007F8AC(s32 slot_index)
{
    u32 context = m15t_lw(WM_M15T_CONTEXT_PTR);
    u32 slot = m15t_slot(slot_index);
    u32 owner = m15t_owner(context, slot_index);
    u32 descriptor = m15t_lw(owner + 0x40u);
    u32 abr = slot_index == 8 ? 1u : 3u;

#if defined(W34N101_MUTANT_WRONG_SLOT8_ABR)
    abr = 3u;
#endif
    wm_8007EBBC(owner, m15t_lw(owner + 0x48u),
                 m15t_lhu(descriptor + 4u), abr);
    m15t_sw(slot + 0x38u, UINT32_C(0xFFFFF7A6));
    m15t_sw(slot + 0x3Cu, 0u);
    m15t_sw(slot + 0x40u, UINT32_C(0x00000DA6));
    m15t_sh(slot + 0x20u, 0u);
#if defined(W34N101_MUTANT_WRONG_INITIAL_TIMER)
    m15t_sh(slot + 0x22u, 59u);
#else
    m15t_sh(slot + 0x22u, 60u);
#endif
    m15t_sw(slot + 0x5Cu, m15t_scale_for_slot(slot_index));
    return 1;
}

static void m15t_clear_latch(u32 slot)
{
    m15t_sh(slot + 0x04u, 0u);
}

static void m15t_begin_visible(u32 slot, u32 owner, u16 state, u16 timer)
{
    m15t_sh(slot + 0x22u, timer);
    m15t_sh(slot + 0x20u, state);
    m15t_clear_latch(slot);
    m15t_sw(slot + 0x5Cu, m15t_scale_for_slot((s32)((slot -
                 m15t_lw(WM_M15T_POOL_PTR)) >> 7u)));
    m15t_sh(owner + 0x00u, 0u);
    m15t_sh(owner + 0x18u, 0u);
    m15t_sh(owner + 0x1Au, 0u);
    m15t_sh(owner + 0x1Cu, 0u);
    (void)RotMatrixYXZ((SVECTOR *)PSX_ADDR(owner + 0x18u),
                       (MATRIX *)PSX_ADDR(owner + 0x20u));
}

static void m15t_consume_latch(u32 slot, u32 owner)
{
    switch (m15t_lhu(slot + 0x04u)) {
    case 1u:
        m15t_clear_latch(slot);
        break;
    case 2u:
        m15t_clear_latch(slot);
#if !defined(W34N101_MUTANT_SKIP_LATCH2_STATE)
        m15t_sh(slot + 0x20u, 1u);
#endif
        break;
    case 3u:
        m15t_sw(slot + 0x38u, UINT32_C(0xFFFFF7A6));
        m15t_sw(slot + 0x3Cu, 0u);
        m15t_sw(slot + 0x40u, UINT32_C(0x00000DA6));
        m15t_begin_visible(slot, owner, 0u, 60u);
        wm_800894C8(35u);
        wm_800894C8(36u);
        break;
    case 4u:
        m15t_begin_visible(slot, owner, 1u, 4u);
        break;
    case 5u:
        m15t_clear_latch(slot);
        m15t_sh(slot + 0x20u, 1u);
        m15t_sh(slot + 0x22u, 30u);
        break;
    default:
        break;
    }
}

static void m15t_follow_primary(u32 slot, u32 primary)
{
    u32 x;
    u32 z;
    u16 yaw;

    m15t_sw(slot + 0x28u, m15t_lw(primary + 0x28u));
    m15t_sw(slot + 0x2Cu, m15t_lw(primary + 0x2Cu));
    m15t_sw(slot + 0x30u, m15t_lw(primary + 0x30u));
    x = m15t_lw(slot + 0x28u) + UINT32_C(0x0042D000);
    z = m15t_lw(slot + 0x30u) + UINT32_C(0xFF92D000);
#if defined(W34N101_MUTANT_WRONG_MARKER_OFFSET)
    x = m15t_lw(slot + 0x28u) + UINT32_C(0x0042C000);
#endif
    m15t_sh(WM_M15T_SC_POSITION + 0u, (u16)m15t_sra12(x));
    m15t_sh(WM_M15T_SC_POSITION + 2u,
             (u16)m15t_sra12(m15t_lw(slot + 0x2Cu)));
    m15t_sh(WM_M15T_SC_POSITION + 4u, (u16)m15t_sra12(z));
    m15t_sh(WM_M15T_SC_ANGLES + 0u, 0u);
    m15t_sh(WM_M15T_SC_ANGLES + 4u, 0u);
    yaw = (u16)((u32)ratan2(2138, 3494) & 0x0FFFu);
    m15t_sh(WM_M15T_SC_ANGLES + 2u, yaw);
    wm_80089160(35u, WM_M15T_SC_POSITION, WM_M15T_SC_ANGLES);
    wm_80089160(36u, WM_M15T_SC_POSITION, WM_M15T_SC_ANGLES);
}

static void m15t_advance_fade(u32 slot)
{
    u16 timer = (u16)(m15t_lhu(slot + 0x22u) - 1u);

    m15t_sh(slot + 0x22u, timer);
    if ((s16)timer < 0) {
        u32 scale = m15t_lw(slot + 0x5Cu);
        m15t_sh(slot + 0x22u, 0u);
#if defined(W34N101_MUTANT_WRONG_FADE_STEP)
        scale -= 127u;
#else
        scale -= 128u;
#endif
        m15t_sw(slot + 0x5Cu, scale);
    }
}

static void m15t_seed_fixed_matrix(void)
{
#if defined(W34N101_MUTANT_WRONG_FIXED_MATRIX)
    m15t_sh(WM_M15T_SC_MATRIX_A + 0x00u, 3493u);
#else
    m15t_sh(WM_M15T_SC_MATRIX_A + 0x00u, 3494u);
#endif
    m15t_sh(WM_M15T_SC_MATRIX_A + 0x02u, 0u);
    m15t_sh(WM_M15T_SC_MATRIX_A + 0x04u, 2138u);
    m15t_sh(WM_M15T_SC_MATRIX_A + 0x06u, 0u);
    m15t_sh(WM_M15T_SC_MATRIX_A + 0x08u, (u16)(s16)-4096);
    m15t_sh(WM_M15T_SC_MATRIX_A + 0x0Au, 0u);
    m15t_sh(WM_M15T_SC_MATRIX_A + 0x0Cu, (u16)(s16)-2138);
    m15t_sh(WM_M15T_SC_MATRIX_A + 0x0Eu, 0u);
    m15t_sh(WM_M15T_SC_MATRIX_A + 0x10u, 3494u);
}

static void m15t_publish_render_state(u32 slot, u32 owner)
{
    m15t_seed_fixed_matrix();
    m15t_sh(owner + 0x1Cu,
             (u16)((m15t_lhu(owner + 0x1Cu) + 256u) & 0x0FFFu));
    (void)RotMatrixYXZ((SVECTOR *)PSX_ADDR(owner + 0x18u),
                       (MATRIX *)PSX_ADDR(WM_M15T_SC_MATRIX_B));
    (void)MulMatrix0((MATRIX *)PSX_ADDR(WM_M15T_SC_MATRIX_A),
                     (MATRIX *)PSX_ADDR(WM_M15T_SC_MATRIX_B),
                     (MATRIX *)PSX_ADDR(owner + 0x20u));
    m15t_sw(owner + 0x08u, m15t_sra12(m15t_lw(slot + 0x28u)));
    m15t_sw(owner + 0x0Cu, m15t_sra12(m15t_lw(slot + 0x2Cu)));
    m15t_sw(owner + 0x10u, m15t_sra12(m15t_lw(slot + 0x30u)));
    m15t_sw(WM_M15T_SC_SCALE + 0u, m15t_lw(slot + 0x5Cu));
    m15t_sw(WM_M15T_SC_SCALE + 4u, m15t_lw(slot + 0x5Cu));
#if defined(W34N101_MUTANT_WRONG_SCALE_Z)
    m15t_sw(WM_M15T_SC_SCALE + 8u, 4096u);
#else
    m15t_sw(WM_M15T_SC_SCALE + 8u, 8192u);
#endif
    (void)ScaleMatrix((MATRIX *)PSX_ADDR(owner + 0x20u),
                      (VECTOR *)PSX_ADDR(WM_M15T_SC_SCALE));
}

s32 wm_8007F968(s32 slot_index)
{
    u32 pool = m15t_lw(WM_M15T_POOL_PTR);
    u32 slot = pool + ((u32)slot_index << 7u);
    u32 primary = pool + 3u * 128u;
    u32 owner = m15t_owner(m15t_lw(WM_M15T_CONTEXT_PTR), slot_index);

    m15t_consume_latch(slot, owner);
    switch (m15t_lh(slot + 0x20u)) {
    case 0:
        m15t_follow_primary(slot, primary);
        break;
    case 1:
        m15t_advance_fade(slot);
        break;
    default:
        break;
    }
    m15t_publish_render_state(slot, owner);
    if (m15t_as_s32(m15t_lw(slot + 0x5Cu)) < 0) {
        m15t_sw(slot + 0x5Cu, 0u);
        m15t_sh(owner + 0x00u, 1u);
        return 3;
    }
    return 1;
}
