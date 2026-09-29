/* Exact native transcription of retail [0x8007AD34, 0x8007B200). */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_7ad34.h"
#include "world_map_helper_96f18.h"
#include "world_map_helper_97070.h"
#include "world_map_helper_97244.h"

#define WM_M14D_POOL_PTR       UINT32_C(0x8009BE24)
#define WM_M14D_RESET_POSITION UINT32_C(0x8009C5AC)
#define WM_M14D_ANGLES         UINT32_C(0x8009BD38)
#define WM_M14D_CAMERA_INPUT   UINT32_C(0x8009BD40)
#define WM_M14D_GEOM_Y         UINT32_C(0x8009BE0C)
#define WM_M14D_POSITION       UINT32_C(0x8009BE28)
#define WM_M14D_CAMERA_MATRIX  UINT32_C(0x8009C808)
#define WM_M14D_VIEW_HEIGHT    UINT32_C(0x8009D3F0)
#define WM_M14D_EVENT_FLAG     UINT32_C(0x8009D144)
#define WM_M14D_POSITION_COPY  UINT32_C(0x8009D55C)
#define WM_M14D_SC_JITTER      UINT32_C(0x1F8000A0)

static s16 m14d_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u16 m14d_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 m14d_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void m14d_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void m14d_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static s32 m14d_as_s32(u32 bits)
{
    s32 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static u32 m14d_sra(u32 bits, unsigned shift)
{
    u32 shifted = bits >> shift;

    if ((bits & UINT32_C(0x80000000)) != 0u)
        shifted |= ~(UINT32_MAX >> shift);
    return shifted;
}

static u32 m14d_slot(s32 slot_index)
{
    return m14d_lw(WM_M14D_POOL_PTR) + ((u32)slot_index << 7u);
}

static void m14d_clear_state(u32 slot)
{
    m14d_sh(slot + 0x20u, 0u);
}

static void m14d_build_generic_camera(void)
{
    wm_80096F18(WM_M14D_CAMERA_INPUT, WM_M14D_POSITION,
                 m14d_as_s32(m14d_lw(WM_M14D_VIEW_HEIGHT)),
                 WM_M14D_ANGLES);
}

s32 wm_8007AD34(s32 slot_index)
{
    u32 slot = m14d_slot(slot_index);
    u32 x = m14d_lw(WM_M14D_RESET_POSITION + 0u);
    u32 y = m14d_lw(WM_M14D_RESET_POSITION + 4u);
    u32 z = m14d_lw(WM_M14D_RESET_POSITION + 8u);

    m14d_sw(WM_M14D_GEOM_Y, 120u);
#if defined(W34N75_MUTANT_WRONG_INIT_HEIGHT)
    m14d_sw(WM_M14D_VIEW_HEIGHT, UINT32_C(0x001E0000));
#else
    m14d_sw(WM_M14D_VIEW_HEIGHT, UINT32_C(0x001C0000));
#endif
    m14d_sh(WM_M14D_ANGLES + 0u, 64u);
    m14d_sh(WM_M14D_ANGLES + 2u, 1152u);
    m14d_sh(WM_M14D_ANGLES + 4u, 0u);
    m14d_sw(WM_M14D_POSITION + 0u, x);
    m14d_sw(WM_M14D_POSITION + 4u, y);
    m14d_sw(WM_M14D_POSITION + 8u, z);
    m14d_sw(WM_M14D_POSITION_COPY + 0u, x);
    m14d_sw(WM_M14D_POSITION_COPY + 4u, y);
    m14d_sw(WM_M14D_POSITION_COPY + 8u, z);
    m14d_sw(slot + 0x50u, 4096u);
    return 1;
}

static void m14d_consume_latch(u32 slot)
{
    u16 latch = m14d_lhu(slot + 4u);

    switch (latch) {
    case 1u:
        m14d_sh(slot + 4u, 0u);
        m14d_sh(slot + 0x20u, 1u);
        break;
    case 2u:
        m14d_sh(slot + 0x20u, 2u);
        m14d_sh(slot + 4u, 0u);
#if defined(W34N75_MUTANT_WRONG_LATCH2_SCALE)
        m14d_sw(slot + 0x50u, UINT32_C(0x0004000));
#else
        m14d_sw(slot + 0x50u, UINT32_C(0x00040000));
#endif
        break;
    case 3u:
        m14d_sw(WM_M14D_VIEW_HEIGHT, UINT32_C(0x00640000));
        m14d_sh(slot + 4u, 0u);
        m14d_clear_state(slot);
        m14d_sh(WM_M14D_ANGLES + 0u, (u16)(s16)-480);
        m14d_sh(WM_M14D_ANGLES + 2u, 1152u);
        m14d_sh(WM_M14D_ANGLES + 4u, 0u);
        break;
    case 4u:
        m14d_sh(slot + 4u, 0u);
        m14d_clear_state(slot);
        m14d_sw(WM_M14D_VIEW_HEIGHT, UINT32_C(0x001E0000));
        m14d_sh(WM_M14D_ANGLES + 0u, 64u);
        m14d_sh(WM_M14D_ANGLES + 2u, 1048u);
        m14d_sh(WM_M14D_ANGLES + 4u, 0u);
        m14d_sw(WM_M14D_EVENT_FLAG, 0u);
        m14d_sw(WM_M14D_POSITION + 8u,
                 m14d_lw(WM_M14D_RESET_POSITION + 8u));
        break;
    case 5u:
        m14d_sh(slot + 4u, 0u);
        m14d_sh(slot + 0x20u, 3u);
        break;
    case 6u:
        m14d_sh(slot + 4u, 0u);
        m14d_sh(slot + 0x20u, 4u);
        m14d_sw(slot + 0x58u,
                 m14d_lw(WM_M14D_RESET_POSITION + 0u) +
                     UINT32_C(0x000C7C00));
        m14d_sw(slot + 0x5Cu,
                 m14d_lw(WM_M14D_RESET_POSITION + 8u) +
                     UINT32_C(0x003EC400));
        m14d_sw(WM_M14D_EVENT_FLAG, 1u);
        break;
    case 7u:
        m14d_sh(slot + 4u, 0u);
        m14d_sh(slot + 0x20u, 5u);
        break;
    default:
        break;
    }
}

static void m14d_tracking_camera(u32 slot)
{
    u32 y = m14d_sra(m14d_lw(WM_M14D_POSITION + 4u), 12u);

    m14d_sh(WM_M14D_CAMERA_INPUT + 0u,
             (u16)m14d_sra(m14d_lw(slot + 0x58u) -
                                m14d_lw(WM_M14D_POSITION + 0u),
                            12u));
#if defined(W34N75_MUTANT_WRONG_TRACKING_OFFSET)
    m14d_sh(WM_M14D_CAMERA_INPUT + 2u, (u16)(y - 63u));
#else
    m14d_sh(WM_M14D_CAMERA_INPUT + 2u, (u16)(y - 64u));
#endif
    m14d_sh(WM_M14D_CAMERA_INPUT + 4u,
             (u16)m14d_sra(m14d_lw(WM_M14D_POSITION + 8u) -
                                m14d_lw(slot + 0x5Cu),
                            12u));
    m14d_sh(WM_M14D_CAMERA_INPUT + 8u, 0u);
    m14d_sh(WM_M14D_CAMERA_INPUT + 0x0Au, (u16)y);
    m14d_sh(WM_M14D_CAMERA_INPUT + 0x0Cu, 0u);
    wm_80097244(WM_M14D_CAMERA_INPUT);
    wm_80097070(WM_M14D_CAMERA_MATRIX, WM_M14D_ANGLES);
}

static void m14d_run_state(u32 slot)
{
    s16 state = m14d_lh(slot + 0x20u);

    switch (state) {
    case 0:
        m14d_build_generic_camera();
        break;
    case 1: {
        u32 phase = m14d_lw(slot + 0x50u) + 512u;
        m14d_sw(slot + 0x50u, phase);
        if (m14d_as_s32(phase) > 32768) {
#if defined(W34N75_MUTANT_WRONG_SCALE_CLAMP)
            m14d_sw(slot + 0x50u, 32767u);
#else
            m14d_sw(slot + 0x50u, 32768u);
#endif
            m14d_clear_state(slot);
        }
        m14d_build_generic_camera();
        break;
    }
    case 2: {
        u32 phase = m14d_lw(slot + 0x50u) - 512u;
        m14d_sw(slot + 0x50u, phase);
        if (m14d_as_s32(phase) <= 32767) {
            m14d_sw(slot + 0x50u, 32768u);
            m14d_clear_state(slot);
        }
        m14d_build_generic_camera();
        break;
    }
    case 3: {
        u32 phase = m14d_lw(slot + 0x50u) - 256u;
        m14d_sw(slot + 0x50u, phase);
        if (m14d_as_s32(phase) < 4096) {
            m14d_sw(slot + 0x50u, 4096u);
            m14d_clear_state(slot);
        }
        m14d_build_generic_camera();
        break;
    }
    case 4: {
        u32 z = m14d_lw(WM_M14D_POSITION + 8u) +
                UINT32_C(0x0003A000);
        m14d_sw(WM_M14D_POSITION + 8u, z);
        if (m14d_as_s32(z) > INT32_C(0x077FFFFF))
            m14d_clear_state(slot);
        m14d_tracking_camera(slot);
        break;
    }
    case 5: {
        u16 angle = (u16)(m14d_lhu(WM_M14D_ANGLES + 2u) + 4u);
        m14d_sh(WM_M14D_ANGLES + 2u, angle);
#if defined(W34N75_MUTANT_WRONG_ANGLE_THRESHOLD)
        if ((s16)angle >= 1536)
#else
        if ((s16)angle >= 1537)
#endif
            m14d_clear_state(slot);
        m14d_build_generic_camera();
        break;
    }
    default:
        break;
    }
}

static void m14d_apply_jitter(u32 slot)
{
    s32 scale = m14d_as_s32(m14d_lw(slot + 0x50u));
    s32 divisor = scale >> 12;
    s32 bias = scale >> 13;
    s32 jitter_x = (s32)rand() % divisor - bias;
    s32 jitter_y = (s32)rand() % divisor - bias;
    u16 x = (u16)jitter_x;
    u16 y = (u16)jitter_y;

    m14d_sh(WM_M14D_SC_JITTER + 0u, x);
    m14d_sh(WM_M14D_SC_JITTER + 2u, y);
    m14d_sh(WM_M14D_CAMERA_INPUT + 0u,
             (u16)(m14d_lhu(WM_M14D_CAMERA_INPUT + 0u) + x));
    m14d_sh(WM_M14D_CAMERA_INPUT + 2u,
             (u16)(m14d_lhu(WM_M14D_CAMERA_INPUT + 2u) + y));
#if !defined(W34N75_MUTANT_SKIP_JITTER_MIRROR)
    m14d_sh(WM_M14D_CAMERA_INPUT + 8u,
             (u16)(m14d_lhu(WM_M14D_CAMERA_INPUT + 8u) + x));
    m14d_sh(WM_M14D_CAMERA_INPUT + 0x0Au,
             (u16)(m14d_lhu(WM_M14D_CAMERA_INPUT + 0x0Au) + y));
#endif
}

s32 wm_8007ADD4(s32 slot_index)
{
    u32 slot = m14d_slot(slot_index);

    m14d_consume_latch(slot);
    m14d_run_state(slot);
    m14d_apply_jitter(slot);
    return 1;
}
