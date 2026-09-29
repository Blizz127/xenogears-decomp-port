/* Exact native transcription of retail [0x8007BA08, 0x8007BB60). */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_7ba08.h"
#include "world_map_common_tail.h"

#define WM_M14_POOL_PTR       UINT32_C(0x8009BE24)
#define WM_M14_RESET_POSITION UINT32_C(0x8009C5AC)
#define WM_M14_MARKER_DATA    UINT32_C(0x8009A488)
#define WM_M14_SCRATCH_VECTOR UINT32_C(0x1F8000A0)

static s16 m14_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u16 m14_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 m14_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void m14_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void m14_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static s16 m14_as_s16(u16 bits)
{
    s16 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static u32 m14_sra12(u32 bits)
{
    u32 shifted = bits >> 12u;
    if ((bits & UINT32_C(0x80000000)) != 0u)
        shifted |= UINT32_C(0xFFF00000);
    return shifted;
}

static u32 m14_slot(s32 slot_index)
{
    return m14_lw(WM_M14_POOL_PTR) + ((u32)slot_index << 7u);
}

s32 wm_8007BA10(s32 slot_index)
{
    u32 slot = m14_slot(slot_index);
    u16 timer;

    if (m14_lh(slot + 4u) == 1) {
        m14_sh(slot + 4u, 0u);
#if defined(W34N70_MUTANT_WRONG_TIMER_SEED)
        m14_sh(slot + 0x22u, 59u);
#else
        m14_sh(slot + 0x22u, 60u);
#endif
        m14_sw(slot + 0x28u, m14_lw(WM_M14_RESET_POSITION + 0u));
        m14_sw(slot + 0x2Cu, m14_lw(WM_M14_RESET_POSITION + 4u));
        m14_sw(slot + 0x30u, m14_lw(WM_M14_RESET_POSITION + 8u));
    }

    timer = (u16)(m14_lhu(slot + 0x22u) - 1u);
    m14_sh(slot + 0x22u, timer);
    if (m14_as_s16(timer) > 0) {
#if !defined(W34N70_MUTANT_SKIP_X_STEP)
        m14_sw(slot + 0x28u,
               m14_lw(slot + 0x28u) + UINT32_C(0xFFFF6A30));
#endif
        m14_sw(slot + 0x30u,
               m14_lw(slot + 0x30u) + UINT32_C(0x0002F130));
        m14_sh(WM_M14_SCRATCH_VECTOR + 0u,
               (u16)m14_sra12(m14_lw(slot + 0x28u)));
        m14_sh(WM_M14_SCRATCH_VECTOR + 2u,
               (u16)m14_sra12(m14_lw(slot + 0x2Cu)));
        m14_sh(WM_M14_SCRATCH_VECTOR + 4u,
               (u16)m14_sra12(m14_lw(slot + 0x30u)));
#if defined(W34N70_MUTANT_WRONG_MARKER_DATA)
        wm_80089160(9u, WM_M14_SCRATCH_VECTOR, 0u);
#else
        wm_80089160(9u, WM_M14_SCRATCH_VECTOR, WM_M14_MARKER_DATA);
#endif
        return 1;
    }

    m14_sh(slot + 0x22u, 60u);
    m14_sw(slot + 0x28u, m14_lw(WM_M14_RESET_POSITION + 0u));
    m14_sw(slot + 0x2Cu, m14_lw(WM_M14_RESET_POSITION + 4u));
    m14_sw(slot + 0x30u, m14_lw(WM_M14_RESET_POSITION + 8u));
#if !defined(W34N70_MUTANT_SKIP_RESET_HEADING)
    m14_sw(slot + 0x34u, m14_lw(WM_M14_RESET_POSITION + 0x0Cu));
#endif
    wm_800894C8(9u);
    return 3;
}
