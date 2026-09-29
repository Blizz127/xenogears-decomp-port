/* Retail mode-10 sequence/context callbacks [0x800795E4,0x8007A06C). */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_callback_795e4.h"
#include "world_map_common_tail.h"
#include "world_map_helper_848b4.h"
#include "world_map_helper_93354.h"
#include "world_map_helper_97770.h"

extern void* D_8006259C;
extern void func_80039E60(s32 packed_id);

#define WM_M10P_POOL_PTR       0x8009BE24u
#define WM_M10P_POSITION       0x8009BE28u
#define WM_M10P_RENDER_Z       0x8009BBBCu
#define WM_M10P_CONTEXT_PTR    0x8009C620u
#define WM_M10P_RESET_POSITION 0x8009C5ACu

#define WM_M10P_SC_ROT_A       0x1F8000A0u
#define WM_M10P_SC_ROT_B       0x1F8000A8u
#define WM_M10P_SC_ROT_C       0x1F8000B0u
#define WM_M10P_SC_ROT_D       0x1F8000B8u
#define WM_M10P_SC_MATRIX_A    0x1F8000F0u
#define WM_M10P_SC_MATRIX_B    0x1F800110u
#define WM_M10P_SC_MATRIX_C    0x1F800130u
#define WM_M10P_SC_MATRIX_D    0x1F800150u

static s16 m10p_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u16 m10p_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 m10p_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void m10p_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void m10p_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static s32 m10p_as_s32(u32 bits)
{
    s32 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static u32 m10p_sra12(u32 bits)
{
    u32 shifted = bits >> 12u;
    if ((bits & 0x80000000u) != 0u)
        shifted |= 0xFFF00000u;
    return shifted;
}

static u32 m10p_slot(s32 slot_index)
{
    return m10p_lw(WM_M10P_POOL_PTR) + ((u32)slot_index << 7u);
}

static void m10p_copy_words(u32 destination, u32 source, u32 count)
{
    u32 index;
    for (index = 0u; index < count; index++)
        m10p_sw(destination + index * 4u, m10p_lw(source + index * 4u));
}

static s32 m10p_sound_id(u16 low)
{
    u16 bank;
    u32 bits;
    memcpy(&bank, (const uint8_t*)D_8006259C + 0x14u, sizeof(bank));
    bits = ((u32)bank << 16u) | (u32)low;
    return m10p_as_s32(bits);
}

static void m10p_write_presence_vector(u32 slot)
{
    m10p_sh(WM_M10P_SC_ROT_A + 0u,
            (u16)m10p_sra12(m10p_lw(slot + 0x28u)));
    m10p_sh(WM_M10P_SC_ROT_A + 2u,
            (u16)m10p_sra12(m10p_lw(slot + 0x2Cu)));
    m10p_sh(WM_M10P_SC_ROT_A + 4u,
            (u16)m10p_sra12(m10p_lw(slot + 0x30u)));
}

static void m10p_call_presence(u32 record)
{
    wm_80089160(record, WM_M10P_SC_ROT_A, 0u);
}

s32 wm_800795E4(s32 slot_index)
{
    u32 destination;
    u32 slot;
    u32 context;

#if defined(W34N65_MUTANT_SKIP_LAST_LINK)
    for (destination = 1u; destination < 13u; destination++)
#else
    for (destination = 1u; destination <= 13u; destination++)
#endif
        wm_800848B4(0, (s32)destination);

    slot = m10p_slot(slot_index);
    m10p_sh(slot + 0x20u, 0u);
    m10p_copy_words(slot + 0x28u, WM_M10P_RESET_POSITION, 4u);
    m10p_sw(slot + 0x50u, 0u);
    m10p_sw(slot + 0x54u, 64u);
    m10p_sw(slot + 0x58u, 512u);
    m10p_sw(slot + 0x5Cu, 96u);
    m10p_sw(slot + 0x60u, 768u);
    m10p_sw(slot + 0x64u, 80u);
    m10p_copy_words(WM_M10P_POSITION, slot + 0x28u, 4u);

    context = m10p_lw(WM_M10P_CONTEXT_PTR);
    m10p_sh(context + 0x18u, (u16)-256);
    m10p_sh(context + 0x1Au, 0u);
    m10p_sh(context + 0x1Cu, (u16)-64);
    func_80039E60(m10p_sound_id(0x0036u));
    return 1;
}

static void m10p_state0(u32 slot)
{
    m10p_write_presence_vector(slot);
    m10p_call_presence(11u);
    m10p_sw(slot + 0x2Cu, m10p_lw(slot + 0x2Cu) + 0x00004000u);
    m10p_sw(slot + 0x30u, m10p_lw(slot + 0x30u) + 0xFFFF8000u);
    m10p_sw(WM_M10P_RENDER_Z,
            m10p_lw(WM_M10P_RENDER_Z) + 0xFFFF8000u);
#if defined(W34N65_MUTANT_WRONG_STATE0_THRESHOLD)
    if (m10p_as_s32(m10p_lw(slot + 0x2Cu)) < -81920)
#else
    if (m10p_as_s32(m10p_lw(slot + 0x2Cu)) < -98304)
#endif
        return;

#if defined(W34N65_MUTANT_REBUILD_SECOND_PRESENCE)
    m10p_write_presence_vector(slot);
#endif
    m10p_call_presence(12u);
    m10p_sw(slot + 0x3Cu, 0xFFFFC000u);
    m10p_sh(slot + 0x20u, (u16)(m10p_lhu(slot + 0x20u) + 1u));
    func_80039E60(m10p_sound_id(0x0071u));
    m10p_sh(slot + 0x22u, 32u);
}

static void m10p_state1(u32 slot, u32 context)
{
    u16 timer;

    m10p_write_presence_vector(slot);
    m10p_call_presence(11u);
    m10p_call_presence(12u);
    timer = (u16)(m10p_lhu(slot + 0x22u) - 1u);
    m10p_sh(slot + 0x22u, timer);
    if ((s16)timer <= 0)
        wm_800894C8(12u);

    m10p_sh(context + 0x18u, (u16)(m10p_lhu(context + 0x18u) + 4u));
    m10p_sh(context + 0x1Cu, (u16)(m10p_lhu(context + 0x1Cu) + 2u));
    m10p_sw(slot + 0x2Cu,
            m10p_lw(slot + 0x2Cu) + m10p_lw(slot + 0x3Cu));
#if defined(W34N65_MUTANT_WRONG_STATE1_ACCELERATION)
    m10p_sw(slot + 0x3Cu, m10p_lw(slot + 0x3Cu) + 0x80u);
#else
    m10p_sw(slot + 0x3Cu, m10p_lw(slot + 0x3Cu) + 0x100u);
#endif
    m10p_sw(slot + 0x30u, m10p_lw(slot + 0x30u) + 0xFFFF8000u);
    m10p_sw(WM_M10P_RENDER_Z,
            m10p_lw(WM_M10P_RENDER_Z) + 0xFFFF8000u);
    if (m10p_as_s32(m10p_lw(slot + 0x2Cu)) < -98304)
        return;

    m10p_call_presence(12u);
    m10p_sw(slot + 0x40u, 0xFFFF8000u);
    m10p_sh(slot + 0x20u, (u16)(m10p_lhu(slot + 0x20u) + 1u));
    func_80039E60(m10p_sound_id(0x0071u));
}

static void m10p_lower_angles(u32 context)
{
    m10p_sh(context + 0x18u, (u16)(m10p_lhu(context + 0x18u) - 2u));
    m10p_sh(context + 0x1Cu, (u16)(m10p_lhu(context + 0x1Cu) - 1u));
}

static void m10p_advance_z(u32 slot)
{
    u32 velocity = m10p_lw(slot + 0x40u);
    m10p_sw(slot + 0x30u, m10p_lw(slot + 0x30u) + velocity);
    m10p_sw(WM_M10P_RENDER_Z, m10p_lw(WM_M10P_RENDER_Z) + velocity);
    m10p_sw(slot + 0x40u, velocity + 0x100u);
}

static void m10p_state2(u32 slot, u32 context)
{
    m10p_write_presence_vector(slot);
    m10p_call_presence(11u);
    m10p_call_presence(12u);
    m10p_lower_angles(context);
    m10p_advance_z(slot);
    if (m10p_as_s32(m10p_lw(slot + 0x30u)) > 0x01580000)
        return;

    m10p_sh(slot + 0x22u, 8u);
    m10p_sh(slot + 0x20u, (u16)(m10p_lhu(slot + 0x20u) + 1u));
    wm_800894C8(12u);
#if !defined(W34N65_MUTANT_SKIP_STATE2_CLAIM)
    (void)wm_80097770(1u, 1);
#endif
}

static void m10p_state3(u32 slot, u32 context)
{
    u16 timer;

    m10p_lower_angles(context);
    m10p_advance_z(slot);
    timer = (u16)(m10p_lhu(slot + 0x22u) - 1u);
    m10p_sh(slot + 0x22u, timer);
    if ((s16)timer > 0)
        return;

    (void)wm_80097770(1u, 1);
#if !defined(W34N65_MUTANT_SKIP_STATE3_ADVANCE)
    m10p_sh(slot + 0x20u, (u16)(m10p_lhu(slot + 0x20u) + 1u));
#endif
}

static void m10p_state4(u32 slot, u32 context)
{
    u32 index;

    if (m10p_lh(slot + 4u) == 0)
        return;
    m10p_sh(slot + 4u, 0u);
#if defined(W34N65_MUTANT_SHORT_STATE4_FLAGS)
    for (index = 0u; index < 13u; index++)
#else
    for (index = 0u; index < 14u; index++)
#endif
        m10p_sh(context + index * 0x54u, 1u);
    wm_800894C8(11u);
}

static void m10p_publish_context(u32 slot, u32 context)
{
    u32 phase_a;
    u32 phase_b;
    u32 phase_c;

#if !defined(W34N65_MUTANT_SKIP_WRAP)
    wm_80093354(slot + 0x28u);
#endif
    m10p_sw(context + 0x08u, m10p_sra12(m10p_lw(slot + 0x28u)));
    m10p_sw(context + 0x0Cu, m10p_sra12(m10p_lw(slot + 0x2Cu)));
    m10p_sw(context + 0x10u, m10p_sra12(m10p_lw(slot + 0x30u)));
    m10p_copy_words(WM_M10P_POSITION, slot + 0x28u, 4u);

    phase_a = m10p_lw(slot + 0x50u) + m10p_lw(slot + 0x54u);
    phase_b = m10p_lw(slot + 0x58u) + m10p_lw(slot + 0x5Cu);
    phase_c = m10p_lw(slot + 0x60u) + m10p_lw(slot + 0x64u);
#if defined(W34N65_MUTANT_WRONG_PHASE_MASK)
    phase_a &= 0x07FFu;
#else
    phase_a &= 0x0FFFu;
#endif
    phase_b &= 0x0FFFu;
    phase_c &= 0x0FFFu;
    m10p_sw(slot + 0x50u, phase_a);
    m10p_sw(slot + 0x58u, phase_b);
    m10p_sw(slot + 0x60u, phase_c);

    m10p_sh(WM_M10P_SC_ROT_A + 0u, 0u);
    m10p_sh(WM_M10P_SC_ROT_A + 2u, (u16)phase_a);
    m10p_sh(WM_M10P_SC_ROT_A + 4u, 0u);
    m10p_sh(WM_M10P_SC_ROT_B + 0u, 0u);
    m10p_sh(WM_M10P_SC_ROT_B + 2u, (u16)phase_b);
    m10p_sh(WM_M10P_SC_ROT_B + 4u, 0u);
    m10p_sh(WM_M10P_SC_ROT_C + 0u, 0u);
    m10p_sh(WM_M10P_SC_ROT_C + 2u, (u16)phase_c);
    m10p_sh(WM_M10P_SC_ROT_C + 4u, 0u);
    m10p_sh(WM_M10P_SC_ROT_D + 0u, 0u);
    m10p_sh(WM_M10P_SC_ROT_D + 2u, 0u);
    m10p_sh(WM_M10P_SC_ROT_D + 4u, (u16)phase_c);

    (void)RotMatrixYXZ((SVECTOR*)PSX_ADDR(WM_M10P_SC_ROT_A),
                       (MATRIX*)PSX_ADDR(WM_M10P_SC_MATRIX_A));
    (void)RotMatrixYXZ((SVECTOR*)PSX_ADDR(WM_M10P_SC_ROT_B),
                       (MATRIX*)PSX_ADDR(WM_M10P_SC_MATRIX_B));
    (void)RotMatrixYXZ((SVECTOR*)PSX_ADDR(WM_M10P_SC_ROT_C),
                       (MATRIX*)PSX_ADDR(WM_M10P_SC_MATRIX_C));
    (void)RotMatrixYXZ((SVECTOR*)PSX_ADDR(WM_M10P_SC_ROT_D),
                       (MATRIX*)PSX_ADDR(WM_M10P_SC_MATRIX_D));

#if defined(W34N65_MUTANT_WRONG_MATRIX_SOURCE)
    m10p_copy_words(context + 0x11Cu, WM_M10P_SC_MATRIX_C, 8u);
#else
    m10p_copy_words(context + 0x11Cu, WM_M10P_SC_MATRIX_D, 8u);
#endif
    m10p_copy_words(context + 0x0C8u, context + 0x11Cu, 8u);
    m10p_copy_words(context + 0x074u, context + 0x0C8u, 8u);
    m10p_copy_words(context + 0x1C4u, WM_M10P_SC_MATRIX_C, 8u);
    m10p_copy_words(context + 0x170u, context + 0x1C4u, 8u);
    m10p_copy_words(context + 0x410u, WM_M10P_SC_MATRIX_A, 8u);
    m10p_copy_words(context + 0x2C0u, context + 0x410u, 8u);
    m10p_copy_words(context + 0x368u, context + 0x2C0u, 8u);
    m10p_copy_words(context + 0x218u, context + 0x368u, 8u);
    m10p_copy_words(context + 0x464u, WM_M10P_SC_MATRIX_B, 8u);
    m10p_copy_words(context + 0x314u, context + 0x464u, 8u);
    m10p_copy_words(context + 0x3BCu, context + 0x314u, 8u);
    m10p_copy_words(context + 0x26Cu, context + 0x3BCu, 8u);

    (void)RotMatrixYXZ((SVECTOR*)PSX_ADDR(context + 0x18u),
                       (MATRIX*)PSX_ADDR(WM_M10P_SC_MATRIX_A));
#if !defined(W34N65_MUTANT_SKIP_VIEW_MATRIX)
    m10p_copy_words(context + 0x20u, WM_M10P_SC_MATRIX_A, 8u);
#endif
}

s32 wm_80079778(s32 slot_index)
{
    u32 slot = m10p_slot(slot_index);
    u32 context = m10p_lw(WM_M10P_CONTEXT_PTR);

    switch (m10p_lh(slot + 0x20u)) {
    case 0: m10p_state0(slot); break;
    case 1: m10p_state1(slot, context); break;
    case 2: m10p_state2(slot, context); break;
    case 3: m10p_state3(slot, context); break;
    case 4: m10p_state4(slot, context); break;
    default: break;
    }

    m10p_publish_context(slot, context);
    return 1;
}
