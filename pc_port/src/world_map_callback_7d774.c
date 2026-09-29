/* Exact native transcription of retail [0x8007D774, 0x8007D918). */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_callback_7d774.h"
#include "world_map_helper_93354.h"

#define WM_M12T_POOL_PTR      UINT32_C(0x8009BE24)
#define WM_M12T_CONTEXT_PTR   UINT32_C(0x8009C620)
#define WM_M12T_WORLD_X       UINT32_C(0x8009BE28)
#define WM_M12T_WORLD_Z       UINT32_C(0x8009BE30)
#define WM_M12T_POSITION_COPY UINT32_C(0x8009D55C)

static s16 m12t_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 m12t_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void m12t_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void m12t_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u32 m12t_sra12(u32 bits)
{
    u32 shifted = bits >> 12u;

    if ((bits & UINT32_C(0x80000000)) != 0u)
        shifted |= UINT32_C(0xFFF00000);
    return shifted;
}

static u32 m12t_slot(s32 slot_index)
{
    return m12t_lw(WM_M12T_POOL_PTR) + ((u32)slot_index << 7u);
}

s32 wm_8007D7FC(s32 slot_index)
{
    u32 context = m12t_lw(WM_M12T_CONTEXT_PTR);
    u32 slot = m12t_slot(slot_index);
    s16 latch = m12t_lh(slot + 4u);

    if (latch == 1) {
        m12t_sh(slot + 4u, 0u);
        m12t_sh(context + 0x540u, 0u);
        m12t_sw(slot + 0x28u, m12t_lw(WM_M12T_WORLD_X));
#if defined(W34N79_MUTANT_WRONG_LATCH1_OFFSET)
        m12t_sw(slot + 0x30u,
                m12t_lw(WM_M12T_WORLD_Z) + UINT32_C(0x00300000));
#else
        m12t_sw(slot + 0x30u,
                m12t_lw(WM_M12T_WORLD_Z) + UINT32_C(0x00400000));
#endif
    } else if (latch == 2) {
        m12t_sh(slot + 4u, 0u);
#if defined(W34N79_MUTANT_WRONG_LATCH2_STATE)
        m12t_sh(slot + 0x20u, 1u);
#else
        m12t_sh(slot + 0x20u, 2u);
#endif
    }

    m12t_sw(slot + 0x30u,
            m12t_lw(slot + 0x30u) + m12t_lw(slot + 0x40u));
#if defined(W34N79_MUTANT_WRAP_WRONG_VECTOR)
    wm_80093354(slot + 0x2Cu);
#else
    wm_80093354(slot + 0x28u);
#endif
    m12t_sw(context + 0x548u, m12t_sra12(m12t_lw(slot + 0x28u)));
    m12t_sw(context + 0x54Cu, m12t_sra12(m12t_lw(slot + 0x2Cu)));
    m12t_sw(context + 0x550u, m12t_sra12(m12t_lw(slot + 0x30u)));

#if !defined(W34N79_MUTANT_SKIP_POSITION_COPY)
    if (m12t_lh(slot + 0x20u) == 2) {
        m12t_sw(WM_M12T_POSITION_COPY + 0u, m12t_lw(slot + 0x28u));
        m12t_sw(WM_M12T_POSITION_COPY + 4u, m12t_lw(slot + 0x2Cu));
        m12t_sw(WM_M12T_POSITION_COPY + 8u, m12t_lw(slot + 0x30u));
    }
#endif
    return 1;
}
