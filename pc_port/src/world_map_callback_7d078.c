/* Exact native transcription of retail [0x8007D078, 0x8007D228). */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_callback_7d078.h"
#include "world_map_common_tail.h"
#include "world_map_helper_848b4.h"
#include "world_map_helper_93354.h"
#include "world_map_helper_93a5c.h"

#define WM_M12L_POOL_PTR    UINT32_C(0x8009BE24)
#define WM_M12L_CONTEXT_PTR UINT32_C(0x8009C620)
#define WM_M12L_SC_VECTOR   UINT32_C(0x1F8000A0)

static s16 m12l_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 m12l_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void m12l_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void m12l_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u32 m12l_sra12(u32 bits)
{
    u32 shifted = bits >> 12u;

    if ((bits & UINT32_C(0x80000000)) != 0u)
        shifted |= UINT32_C(0xFFF00000);
    return shifted;
}

static u32 m12l_slot(s32 slot_index)
{
    return m12l_lw(WM_M12L_POOL_PTR) + ((u32)slot_index << 7u);
}

s32 wm_8007D110(s32 slot_index)
{
    u32 slot = m12l_slot(slot_index);
    u32 context = m12l_lw(WM_M12L_CONTEXT_PTR);
    u32 height;

    if (m12l_lh(slot + 4u) == 1) {
        m12l_sh(slot + 4u, 0u);
#if defined(W34N80_MUTANT_SKIP_COMPLETION_FLAGS)
        m12l_sh(context + 0x2F4u, 0u);
#else
        m12l_sh(context + 0x2F4u, 1u);
#endif
        m12l_sh(context + 0x24Cu, 1u);
        m12l_sh(context + 0x2A0u, 1u);
        wm_800894C8(22u);
#if defined(W34N80_MUTANT_WRONG_COMPLETION_RETURN)
        return 1;
#else
        return 3;
#endif
    }

    m12l_sw(slot + 0x30u,
             m12l_lw(slot + 0x30u) + m12l_lw(slot + 0x40u));
    wm_80093354(slot + 0x28u);
    height = (u32)wm_80093A5C(m12l_lw(slot + 0x28u),
                              m12l_lw(slot + 0x30u));
#if defined(W34N80_MUTANT_SKIP_HEIGHT_BIAS)
    m12l_sw(slot + 0x2Cu, height);
#else
    m12l_sw(slot + 0x2Cu, height - UINT32_C(0x00004000));
#endif

    m12l_sw(context + 0x2FCu, m12l_sra12(m12l_lw(slot + 0x28u)));
    m12l_sw(context + 0x300u, m12l_sra12(m12l_lw(slot + 0x2Cu)));
    m12l_sw(context + 0x304u, m12l_sra12(m12l_lw(slot + 0x30u)));
    m12l_sh(WM_M12L_SC_VECTOR + 0u,
             (u16)m12l_sra12(m12l_lw(slot + 0x28u)));
    m12l_sh(WM_M12L_SC_VECTOR + 2u,
             (u16)m12l_sra12(m12l_lw(slot + 0x2Cu)));
    m12l_sh(WM_M12L_SC_VECTOR + 4u,
             (u16)m12l_sra12(m12l_lw(slot + 0x30u)));
#if defined(W34N80_MUTANT_WRONG_MARKER_ID)
    wm_80089160(21u, WM_M12L_SC_VECTOR, 0u);
#else
    wm_80089160(22u, WM_M12L_SC_VECTOR, 0u);
#endif
    return 1;
}
