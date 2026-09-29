/* Exact native transcription of retail [0x8007B604, 0x8007BA08). */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_callback_7a144.h"
#include "world_map_callback_7b604.h"

#define WM_M14P_POOL_PTR       UINT32_C(0x8009BE24)
#define WM_M14P_CONTEXT_PTR    UINT32_C(0x8009C620)
#define WM_M14P_RESET_POSITION UINT32_C(0x8009C5AC)
#define WM_M14P_BASE_MATRIX    UINT32_C(0x8009A180)
#define WM_M14P_SC_VECTOR_A    UINT32_C(0x1F800000)
#define WM_M14P_SC_VECTOR_B    UINT32_C(0x1F800010)
#define WM_M14P_SC_MATRIX_A    UINT32_C(0x1F8000F0)
#define WM_M14P_SC_MATRIX_B    UINT32_C(0x1F800110)

static s16 m14p_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u16 m14p_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 m14p_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void m14p_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void m14p_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static s32 m14p_as_s32(u32 bits)
{
    s32 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static u32 m14p_sra12(u32 bits)
{
    u32 shifted = bits >> 12u;

    if ((bits & UINT32_C(0x80000000)) != 0u)
        shifted |= UINT32_C(0xFFF00000);
    return shifted;
}

static u32 m14p_slot(s32 slot_index)
{
    return m14p_lw(WM_M14P_POOL_PTR) + ((u32)slot_index << 7u);
}

static void m14p_copy32(u32 destination, u32 source)
{
    u32 offset;

    for (offset = 0u; offset < 32u; offset += 4u)
        m14p_sw(destination + offset, m14p_lw(source + offset));
}

static void m14p_seed_scale_vector(u32 destination, u32 xz)
{
    m14p_sw(destination + 0u, xz);
    m14p_sw(destination + 4u, 4096u);
    m14p_sw(destination + 8u, xz);
}

s32 wm_8007B604(s32 slot_index)
{
    u32 context = m14p_lw(WM_M14P_CONTEXT_PTR);
    u32 slot = m14p_slot(slot_index);
    u32 index;
    u32 final_owner;

    for (index = 0u; index < 2u; index++) {
        u32 owner = context + 0x1F8u + index * 0x54u;
        u32 descriptor = m14p_lw(context + 0x238u + index * 0x54u);
        u32 source = m14p_lw(context + 0x240u + index * 0x54u);

#if defined(W34N74_MUTANT_SKIP_SECOND_PRIMITIVE_INIT)
        if (index == 1u)
            continue;
#endif
#if defined(W34N74_MUTANT_COUNT_FROM_PRIMITIVE_SOURCE)
        (void)descriptor;
        wm_8007A06C(owner, source, m14p_lhu(source + 4u));
#else
        wm_8007A06C(owner, source, m14p_lhu(descriptor + 4u));
#endif
    }

#if !defined(W34N74_MUTANT_SKIP_SECOND_PHASE_SEED)
    m14p_sw(slot + 0x54u, 0u);
#endif
    m14p_sw(slot + 0x50u, 0u);
    m14p_copy32(WM_M14P_SC_MATRIX_A, WM_M14P_BASE_MATRIX);
    m14p_seed_scale_vector(WM_M14P_SC_VECTOR_A,
                           m14p_lw(slot + 0x50u));
    (void)ScaleMatrix((MATRIX *)PSX_ADDR(WM_M14P_SC_MATRIX_A),
                      (VECTOR *)PSX_ADDR(WM_M14P_SC_VECTOR_A));

    final_owner = context + 0x2A0u;
    m14p_copy32(final_owner + 0x20u, WM_M14P_SC_MATRIX_A);
    m14p_copy32(final_owner + 0x74u, WM_M14P_SC_MATRIX_A);
    m14p_sh(final_owner + 0x54u, 1u);
    m14p_sh(final_owner + 0x00u, 1u);
    return 3;
}

static u32 m14p_clamp_phase(u32 value)
{
    if (m14p_as_s32(value) >= 32513) {
#if defined(W34N74_MUTANT_WRONG_PHASE_CLAMP)
        return 32513u;
#else
        return 32512u;
#endif
    }
    return value;
}

s32 wm_8007B798(s32 slot_index)
{
    u32 context = m14p_lw(WM_M14P_CONTEXT_PTR);
    u32 slot = m14p_slot(slot_index);
    u32 primary;
    u32 secondary;

    if (m14p_lh(slot + 4u) != 0) {
        m14p_sh(slot + 4u, 0u);
        m14p_sh(context + 0x24Cu, 0u);
        m14p_sh(context + 0x1F8u, 0u);
    }

    m14p_sw(context + 0x258u, (u32)(s32)-64);
    m14p_sw(context + 0x204u, (u32)(s32)-64);
    m14p_sw(context + 0x254u,
             m14p_sra12(m14p_lw(WM_M14P_RESET_POSITION + 0u)));
    m14p_sw(context + 0x200u,
             m14p_sra12(m14p_lw(WM_M14P_RESET_POSITION + 0u)));
    m14p_sw(context + 0x25Cu,
             m14p_sra12(m14p_lw(WM_M14P_RESET_POSITION + 8u)));
    m14p_sw(context + 0x208u,
             m14p_sra12(m14p_lw(WM_M14P_RESET_POSITION + 8u)));

    primary = m14p_lw(slot + 0x50u) + 384u;
    m14p_sw(slot + 0x50u, primary);
#if defined(W34N74_MUTANT_EARLY_SECONDARY_PHASE)
    if (m14p_as_s32(primary) >= 2048)
#else
    if (m14p_as_s32(primary) >= 2049)
#endif
        m14p_sw(slot + 0x54u, m14p_lw(slot + 0x54u) + 384u);

    primary = m14p_clamp_phase(m14p_lw(slot + 0x50u));
    secondary = m14p_clamp_phase(m14p_lw(slot + 0x54u));
    m14p_sw(slot + 0x50u, primary);
    m14p_sw(slot + 0x54u, secondary);

    m14p_copy32(WM_M14P_SC_MATRIX_A, WM_M14P_BASE_MATRIX);
    m14p_copy32(WM_M14P_SC_MATRIX_B, WM_M14P_SC_MATRIX_A);
    m14p_seed_scale_vector(WM_M14P_SC_VECTOR_A, primary);
    m14p_seed_scale_vector(WM_M14P_SC_VECTOR_B, secondary);
    (void)ScaleMatrix((MATRIX *)PSX_ADDR(WM_M14P_SC_MATRIX_A),
                      (VECTOR *)PSX_ADDR(WM_M14P_SC_VECTOR_A));
    (void)ScaleMatrix((MATRIX *)PSX_ADDR(WM_M14P_SC_MATRIX_B),
                      (VECTOR *)PSX_ADDR(WM_M14P_SC_VECTOR_B));

    m14p_copy32(context + 0x218u, WM_M14P_SC_MATRIX_A);
#if defined(W34N74_MUTANT_WRONG_SECOND_MATRIX_DESTINATION)
    m14p_copy32(context + 0x218u, WM_M14P_SC_MATRIX_B);
#else
    m14p_copy32(context + 0x26Cu, WM_M14P_SC_MATRIX_B);
#endif
    return 1;
}
