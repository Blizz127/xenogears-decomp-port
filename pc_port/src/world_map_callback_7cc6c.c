/* Exact native transcription of retail [0x8007CC6C, 0x8007CE84). */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_callback_7cc6c.h"
#include "world_map_common_tail.h"
#include "world_map_helper_848b4.h"
#include "world_map_helper_93354.h"

#define WM_M12F_POOL_PTR    UINT32_C(0x8009BE24)
#define WM_M12F_CONTEXT_PTR UINT32_C(0x8009C620)
#define WM_M12F_SC_VECTOR   UINT32_C(0x1F8000A0)
#define WM_M12F_POSITION_Z  UINT32_C(0x8009D564)

static s16 m12f_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 m12f_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void m12f_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void m12f_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u32 m12f_sra12(u32 bits)
{
    u32 shifted = bits >> 12u;

    if ((bits & UINT32_C(0x80000000)) != 0u)
        shifted |= UINT32_C(0xFFF00000);
    return shifted;
}

static u32 m12f_slot(s32 slot_index)
{
    return m12f_lw(WM_M12F_POOL_PTR) + ((u32)slot_index << 7u);
}

static void m12f_publish_scratch(u32 slot)
{
    m12f_sh(WM_M12F_SC_VECTOR + 0u,
             (u16)m12f_sra12(m12f_lw(slot + 0x28u)));
    m12f_sh(WM_M12F_SC_VECTOR + 2u,
             (u16)m12f_sra12(m12f_lw(slot + 0x2Cu)));
    m12f_sh(WM_M12F_SC_VECTOR + 4u,
             (u16)m12f_sra12(m12f_lw(slot + 0x30u)));
}

s32 wm_8007CD20(s32 slot_index)
{
    u32 slot = m12f_slot(slot_index);
    u32 context = m12f_lw(WM_M12F_CONTEXT_PTR);
    s16 latch = m12f_lh(slot + 4u);
    s16 state;

    if (latch == 1) {
        m12f_sh(slot + 4u, 0u);
        m12f_sh(slot + 0x20u, 1u);
    } else if (latch == 2) {
        m12f_sh(slot + 4u, 0u);
#if defined(W34N84_MUTANT_WRONG_LATCH2_STATE)
        m12f_sh(slot + 0x20u, 1u);
#else
        m12f_sh(slot + 0x20u, 2u);
#endif
    }

    m12f_sw(slot + 0x30u,
             m12f_lw(slot + 0x30u) + m12f_lw(slot + 0x40u));
    wm_80093354(slot + 0x28u);
    m12f_sw(context + 0x158u, m12f_sra12(m12f_lw(slot + 0x28u)));
    m12f_sw(context + 0x15Cu, m12f_sra12(m12f_lw(slot + 0x2Cu)));
    m12f_sw(context + 0x160u, m12f_sra12(m12f_lw(slot + 0x30u)));
    m12f_publish_scratch(slot);
#if defined(W34N84_MUTANT_WRONG_PRIMARY_MARKER)
    wm_80089160(18u, WM_M12F_SC_VECTOR, 0u);
#else
    wm_80089160(19u, WM_M12F_SC_VECTOR, 0u);
#endif

    state = m12f_lh(slot + 0x20u);
    if (state == 1 || state == 2) {
#if defined(W34N84_MUTANT_SKIP_STATE_MARKER)
        if (state == 1)
#endif
        wm_80089160(31u, WM_M12F_SC_VECTOR, 0u);
    }
#if defined(W34N84_MUTANT_COPY_Z_IN_STATE2)
    if (state >= 0 && state <= 2)
#else
    if (state == 0 || state == 1)
#endif
        m12f_sw(WM_M12F_POSITION_Z, m12f_lw(slot + 0x30u));
    return 1;
}
