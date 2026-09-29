/* Exact native transcription of retail [0x8007D228, 0x8007D414). */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_callback_7d228.h"
#include "world_map_common_tail.h"
#include "world_map_helper_848b4.h"
#include "world_map_helper_93354.h"
#include "world_map_helper_93a5c.h"

#define WM_M12B_POOL_PTR    UINT32_C(0x8009BE24)
#define WM_M12B_CONTEXT_PTR UINT32_C(0x8009C620)
#define WM_M12B_SC_VECTOR   UINT32_C(0x1F8000A0)

static s16 m12b_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 m12b_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void m12b_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void m12b_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u32 m12b_sra12(u32 bits)
{
    u32 shifted = bits >> 12u;

    if ((bits & UINT32_C(0x80000000)) != 0u)
        shifted |= UINT32_C(0xFFF00000);
    return shifted;
}

static u32 m12b_slot(s32 slot_index)
{
    return m12b_lw(WM_M12B_POOL_PTR) + ((u32)slot_index << 7u);
}

static void m12b_publish_scratch(u32 slot)
{
    m12b_sh(WM_M12B_SC_VECTOR + 0u,
             (u16)m12b_sra12(m12b_lw(slot + 0x28u)));
    m12b_sh(WM_M12B_SC_VECTOR + 2u,
             (u16)m12b_sra12(m12b_lw(slot + 0x2Cu)));
    m12b_sh(WM_M12B_SC_VECTOR + 4u,
             (u16)m12b_sra12(m12b_lw(slot + 0x30u)));
}

s32 wm_8007D2B8(s32 slot_index)
{
    u32 slot = m12b_slot(slot_index);
    u32 context = m12b_lw(WM_M12B_CONTEXT_PTR);
    u32 height;

    if (m12b_lh(slot + 4u) == 1) {
        m12b_sh(slot + 4u, 0u);
        m12b_sh(context + 0x348u, 1u);
#if defined(W34N81_MUTANT_SKIP_COMPLETION_PEER)
        m12b_sh(context + 0x39Cu, 0u);
#else
        m12b_sh(context + 0x39Cu, 1u);
#endif
        wm_800894C8(23u);
        m12b_publish_scratch(slot);
#if defined(W34N81_MUTANT_WRONG_COMPLETION_MARKER)
        wm_80089160(27u, WM_M12B_SC_VECTOR, 0u);
#else
        wm_80089160(28u, WM_M12B_SC_VECTOR, 0u);
#endif
        return 3;
    }

    m12b_sw(slot + 0x30u,
             m12b_lw(slot + 0x30u) + m12b_lw(slot + 0x40u));
    wm_80093354(slot + 0x28u);
    height = (u32)wm_80093A5C(m12b_lw(slot + 0x28u),
                              m12b_lw(slot + 0x30u));
#if defined(W34N81_MUTANT_SKIP_HEIGHT_BIAS)
    m12b_sw(slot + 0x2Cu, height);
#else
    m12b_sw(slot + 0x2Cu, height - UINT32_C(0x00004000));
#endif
    m12b_sw(context + 0x350u, m12b_sra12(m12b_lw(slot + 0x28u)));
    m12b_sw(context + 0x354u, m12b_sra12(m12b_lw(slot + 0x2Cu)));
    m12b_sw(context + 0x358u, m12b_sra12(m12b_lw(slot + 0x30u)));
    m12b_publish_scratch(slot);
#if defined(W34N81_MUTANT_WRONG_MOVING_MARKER)
    wm_80089160(22u, WM_M12B_SC_VECTOR, 0u);
#else
    wm_80089160(23u, WM_M12B_SC_VECTOR, 0u);
#endif
    return 1;
}
