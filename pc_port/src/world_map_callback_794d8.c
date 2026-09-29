/*
 * Retail mode-10 scheduler callback leaves:
 *   [0x800794D8, 0x800795E4)
 *   [0x8007A410, 0x8007A568)
 *   [0x8007A568, 0x8007A5DC)
 */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_callback_794d8.h"
#include "world_map_common_tail.h"
#include "world_map_helper_93354.h"
#include "world_map_helper_93a5c.h"

#define WM_M10_POOL_PTR       0x8009BE24u
#define WM_M10_CONTEXT_PTR    0x8009C620u
#define WM_M10_RESET_POSITION 0x8009C5ACu
#define WM_M10_SCRATCH_VECTOR 0x1F8000A0u

static s16 m10_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u16 m10_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 m10_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void m10_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void m10_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u32 m10_sra12(u32 bits)
{
    u32 shifted = bits >> 12u;
    if ((bits & 0x80000000u) != 0u)
        shifted |= 0xFFF00000u;
    return shifted;
}

static s16 m10_as_s16(u16 bits)
{
    s16 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static u32 m10_slot(s32 slot_index)
{
    return m10_lw(WM_M10_POOL_PTR) + ((u32)slot_index << 7u);
}

s32 wm_8007A430(s32 slot_index)
{
    u32 slot = m10_slot(slot_index);
    u16 timer;

    if (m10_lh(slot + 4u) == 1) {
        u32 context = m10_lw(WM_M10_CONTEXT_PTR);
        m10_sh(slot + 4u, 0u);
        m10_sw(slot + 0x28u, m10_lw(context + 8u) << 12u);
        m10_sw(slot + 0x2Cu, m10_lw(context + 0x0Cu) << 12u);
        m10_sw(slot + 0x30u, m10_lw(context + 0x10u) << 12u);
    }

    timer = (u16)(m10_lhu(slot + 0x22u) - 1u);
    m10_sh(slot + 0x22u, timer);
    if (m10_as_s16(timer) > 0) {
        m10_sw(slot + 0x30u, m10_lw(slot + 0x30u) + 0x00020000u);
        m10_sh(WM_M10_SCRATCH_VECTOR + 0u,
               (u16)m10_sra12(m10_lw(slot + 0x28u)));
        m10_sh(WM_M10_SCRATCH_VECTOR + 2u,
               (u16)m10_sra12(m10_lw(slot + 0x2Cu)));
        m10_sh(WM_M10_SCRATCH_VECTOR + 4u,
               (u16)m10_sra12(m10_lw(slot + 0x30u)));
#if defined(W34N62_MUTANT_WRONG_ACTIVE_MARKER)
        wm_80089160(8u, WM_M10_SCRATCH_VECTOR, 0u);
#else
        wm_80089160(9u, WM_M10_SCRATCH_VECTOR, 0u);
#endif
        return 1;
    }

    m10_sh(slot + 0x22u, 96u);
    m10_sw(slot + 0x28u, m10_lw(WM_M10_RESET_POSITION + 0u));
    m10_sw(slot + 0x2Cu, m10_lw(WM_M10_RESET_POSITION + 4u));
    m10_sw(slot + 0x30u, m10_lw(WM_M10_RESET_POSITION + 8u));
#if !defined(W34N62_MUTANT_SKIP_RESET_HEADING)
    m10_sw(slot + 0x34u, m10_lw(WM_M10_RESET_POSITION + 0x0Cu));
#endif
    wm_800894C8(9u);
    return 3;
}

