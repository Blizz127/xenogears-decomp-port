/* Exact native transcription of retail [0x8007E450,0x8007EBBC). */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_7e450.h"
#include "world_map_helper_93354.h"
#include "world_map_helper_96f18.h"

#define WM_M15C_POOL_PTR      UINT32_C(0x8009BE24)
#define WM_M15C_RESET_POS     UINT32_C(0x8009C5AC)
#define WM_M15C_LIVE_POS      UINT32_C(0x8009BE28)
#define WM_M15C_SHADOW_POS    UINT32_C(0x8009D55C)
#define WM_M15C_VIEW_Y        UINT32_C(0x8009BE0C)
#define WM_M15C_CAMERA_HEIGHT UINT32_C(0x8009D3F0)
#define WM_M15C_CAMERA_GATE   UINT32_C(0x8009D144)
#define WM_M15C_ANGLES        UINT32_C(0x8009BD38)
#define WM_M15C_CAMERA_INPUT  UINT32_C(0x8009BD40)
#define WM_M15C_WORLD_X_ACCUM UINT32_C(0x8009BBB4)
#define WM_M15C_WORLD_Z_ACCUM UINT32_C(0x8009BBBC)
#define WM_M15C_JITTER        UINT32_C(0x1F8000A0)

static s16 m15c_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u16 m15c_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 m15c_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void m15c_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void m15c_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static s32 m15c_as_s32(u32 bits)
{
    s32 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static u32 m15c_sra(u32 bits, unsigned shift)
{
    u32 shifted = bits >> shift;
    if ((bits & UINT32_C(0x80000000)) != 0u)
        shifted |= ~(UINT32_MAX >> shift);
    return shifted;
}

static u32 m15c_slot(s32 slot_index)
{
    return m15c_lw(WM_M15C_POOL_PTR) + ((u32)slot_index << 7u);
}

static void m15c_set_angles(u16 x, u16 y)
{
    m15c_sh(WM_M15C_ANGLES + 0u, x);
    m15c_sh(WM_M15C_ANGLES + 2u, y);
    m15c_sh(WM_M15C_ANGLES + 4u, 0u);
}

static void m15c_consume_latch(u32 slot)
{
    switch (m15c_lhu(slot + 4u)) {
    case 1u:
        m15c_sw(WM_M15C_CAMERA_HEIGHT, UINT32_C(0x00500000));
        m15c_sh(slot + 4u, 0u);
#if defined(W34N99_MUTANT_LATCH1_WRONG_ANGLE)
        m15c_set_angles(337u, 3920u);
#else
        m15c_set_angles(336u, 3920u);
#endif
        break;
    case 2u:
        m15c_sw(WM_M15C_CAMERA_HEIGHT, UINT32_C(0x004D0000));
        m15c_sh(slot + 4u, 0u);
        m15c_set_angles(3168u, 896u);
        break;
    case 3u:
        m15c_sw(WM_M15C_CAMERA_HEIGHT, UINT32_C(0x00400000));
        m15c_sh(slot + 4u, 0u);
        m15c_set_angles(48u, 352u);
        break;
    case 4u:
        m15c_sw(WM_M15C_CAMERA_HEIGHT, UINT32_C(0x00400000));
        m15c_sh(slot + 4u, 0u);
        m15c_set_angles(48u, 2400u);
        break;
    case 5u:
        m15c_sh(slot + 0x20u, 1u);
        m15c_sh(slot + 4u, 0u);
        m15c_sw(slot + 0x5Cu, UINT32_C(0xFFFF8000));
        break;
    case 6u:
        m15c_sh(slot + 0x20u, 3u);
        m15c_sw(slot + 0x7Cu, 4096u);
        m15c_sw(WM_M15C_CAMERA_HEIGHT, UINT32_C(0x00200000));
        m15c_sh(slot + 4u, 0u);
        m15c_set_angles((u16)(s16)-88, 3672u);
        break;
    case 7u:
        m15c_sh(slot + 0x20u, 5u);
        m15c_sw(WM_M15C_CAMERA_HEIGHT, UINT32_C(0x00040000));
        m15c_sh(slot + 4u, 0u);
        break;
    case 16u:
        m15c_sh(slot + 0x20u, 16u);
        m15c_sh(slot + 4u, 0u);
        m15c_sw(slot + 0x5Cu, UINT32_C(0xFFFF8000));
        break;
    case 17u:
        m15c_sh(slot + 0x20u, 16u);
        m15c_sw(slot + 0x7Cu, 4096u);
        m15c_sw(WM_M15C_CAMERA_HEIGHT, UINT32_C(0x00400000));
        m15c_sh(slot + 4u, 0u);
        m15c_set_angles(224u, 1944u);
        break;
    case 24u:
        m15c_sh(slot + 0x20u, 24u);
        m15c_sh(slot + 4u, 0u);
        m15c_sw(slot + 0x5Cu, UINT32_C(0xFFFF8000));
        break;
    default:
        break;
    }
}

static void m15c_build_camera(u32 slot)
{
    wm_80093354(slot + 0x28u);
#if !defined(W34N99_MUTANT_SKIP_CAMERA_BUILD)
    wm_80096F18(WM_M15C_CAMERA_INPUT, WM_M15C_LIVE_POS,
                 m15c_as_s32(m15c_lw(WM_M15C_CAMERA_HEIGHT)),
                 WM_M15C_ANGLES);
#endif
}

static void m15c_accumulate_path(void)
{
    m15c_sw(WM_M15C_WORLD_X_ACCUM,
             m15c_lw(WM_M15C_WORLD_X_ACCUM) + UINT32_C(0xFFFBD300));
    m15c_sw(WM_M15C_WORLD_Z_ACCUM,
             m15c_lw(WM_M15C_WORLD_Z_ACCUM) + UINT32_C(0x0006D300));
}

static void m15c_curve_tail(u32 slot)
{
    u32 pitch_speed;
    u32 pitch;

    m15c_build_camera(slot);
    m15c_sh(WM_M15C_ANGLES + 2u,
             (u16)(m15c_lhu(WM_M15C_ANGLES + 2u) - 8u));
    pitch_speed = m15c_lw(slot + 0x5Cu);
    pitch = (u32)m15c_lhu(WM_M15C_ANGLES + 0u) +
            m15c_sra(pitch_speed, 12u);
    m15c_sh(WM_M15C_ANGLES + 0u, (u16)pitch);
    m15c_sw(slot + 0x5Cu, pitch_speed + 768u);
    m15c_sw(WM_M15C_CAMERA_HEIGHT,
             m15c_lw(WM_M15C_CAMERA_HEIGHT) + UINT32_C(0x00008000));
    if (m15c_lh(WM_M15C_ANGLES + 0u) >= 129)
        m15c_sh(WM_M15C_ANGLES + 0u, 128u);
}

static void m15c_state_path(u32 slot, u16 next_state,
                            u32 threshold_x, u32 threshold_z,
                            u32 next_amplitude)
{
    u32 x = m15c_lw(WM_M15C_LIVE_POS + 0u) + UINT32_C(0xFFFBD300);
    u32 z = m15c_lw(WM_M15C_LIVE_POS + 8u) + UINT32_C(0x0006D300);

    m15c_sw(WM_M15C_LIVE_POS + 0u, x);
    m15c_sw(WM_M15C_LIVE_POS + 8u, z);
    if (m15c_as_s32(x) <= m15c_as_s32(threshold_x) &&
        m15c_as_s32(z) > m15c_as_s32(threshold_z)) {
        m15c_sw(WM_M15C_LIVE_POS + 0u, threshold_x + 1u);
        m15c_sw(WM_M15C_LIVE_POS + 8u, threshold_z);
        m15c_sh(slot + 0x20u, next_state);
        m15c_sw(slot + 0x7Cu, next_amplitude);
    } else {
        m15c_accumulate_path();
    }
    m15c_curve_tail(slot);
}

static void m15c_state_three(u32 slot)
{
    u32 x = m15c_lw(WM_M15C_LIVE_POS + 0u) + UINT32_C(0xFFFB3C00);
#if defined(W34N99_MUTANT_STATE3_SIGN_EXTEND_Y)
    u32 y = m15c_lw(WM_M15C_LIVE_POS + 4u) + UINT32_C(0xFFFFFF80);
#else
    u32 y = m15c_lw(WM_M15C_LIVE_POS + 4u) + UINT32_C(0x0000FF80);
#endif
    u32 z = m15c_lw(WM_M15C_LIVE_POS + 8u) + UINT32_C(0xFFF9AA80);

    m15c_sw(WM_M15C_LIVE_POS + 0u, x);
    m15c_sw(WM_M15C_LIVE_POS + 4u, y);
    m15c_sw(WM_M15C_LIVE_POS + 8u, z);
    if (m15c_as_s32(x) <= INT32_C(0x01497FFF) &&
        m15c_as_s32(z) <= INT32_C(0x04AF1FFF)) {
        m15c_sw(WM_M15C_LIVE_POS + 0u, UINT32_C(0x01498000));
        m15c_sw(WM_M15C_LIVE_POS + 8u, UINT32_C(0x04AF2000));
        m15c_sh(slot + 0x20u, 4u);
        m15c_sw(slot + 0x5Cu, UINT32_C(0x00020000));
    } else {
        m15c_sw(WM_M15C_WORLD_X_ACCUM,
                 m15c_lw(WM_M15C_WORLD_X_ACCUM) + UINT32_C(0xFFFB3C00));
        m15c_sw(WM_M15C_WORLD_Z_ACCUM,
                 m15c_lw(WM_M15C_WORLD_Z_ACCUM) + UINT32_C(0xFFF9AA80));
    }
    m15c_build_camera(slot);
    m15c_sw(WM_M15C_CAMERA_HEIGHT,
             m15c_lw(WM_M15C_CAMERA_HEIGHT) + UINT32_C(0x00020000));
    m15c_sh(WM_M15C_ANGLES + 2u,
             (u16)(m15c_lhu(WM_M15C_ANGLES + 2u) + 32u));
    m15c_sh(WM_M15C_ANGLES + 0u,
             (u16)(m15c_lhu(WM_M15C_ANGLES + 0u) - 4u));
}

static void m15c_decay_amplitude(u32 slot, u32 amount)
{
    u32 amplitude = m15c_lw(slot + 0x7Cu) - amount;
    m15c_sw(slot + 0x7Cu, amplitude);
    if (m15c_as_s32(amplitude) < 4096)
        m15c_sw(slot + 0x7Cu, 4096u);
}

static void m15c_apply_state(u32 slot)
{
    switch (m15c_lh(slot + 0x20u)) {
    case 1:
#if defined(W34N99_MUTANT_STATE1_WRONG_DELTA)
        m15c_sw(WM_M15C_LIVE_POS + 0u,
                 m15c_lw(WM_M15C_LIVE_POS + 0u) + UINT32_C(0xFFFBD301));
        m15c_accumulate_path();
        m15c_curve_tail(slot);
#else
        m15c_state_path(slot, 2u, UINT32_C(0x01F9DFFF),
                         UINT32_C(0x05998000), UINT32_C(0x00010000));
#endif
        break;
    case 2:
        m15c_decay_amplitude(slot, 128u);
        break;
    case 3:
        m15c_state_three(slot);
        break;
    case 4: {
        u32 speed = m15c_lw(slot + 0x5Cu);
        m15c_sh(WM_M15C_ANGLES + 2u,
                 (u16)(m15c_lhu(WM_M15C_ANGLES + 2u) +
                       m15c_sra(speed, 12u)));
        speed -= 2048u;
        m15c_sw(slot + 0x5Cu, speed);
        if (m15c_as_s32(speed) < 0) {
            m15c_sw(slot + 0x5Cu, 0u);
            m15c_sh(slot + 0x20u, 0u);
        }
        break;
    }
    case 5: {
        u32 amplitude = m15c_lw(slot + 0x7Cu) - 8192u;
        m15c_sw(slot + 0x7Cu, amplitude);
        if (m15c_as_s32(amplitude) < 4096) {
#if defined(W34N99_MUTANT_STATE5_WRONG_CLAMP)
            m15c_sw(slot + 0x7Cu, 8192u);
#else
            m15c_sw(slot + 0x7Cu, 4096u);
#endif
            m15c_sh(slot + 0x20u, 0u);
        }
        break;
    }
    case 16:
        m15c_state_path(slot, 17u, UINT32_C(0x01F9DFFF),
                         UINT32_C(0x05998000), UINT32_C(0x00008000));
        break;
    case 17:
        m15c_decay_amplitude(slot, 512u);
        break;
    case 24:
#if defined(W34N99_MUTANT_STATE24_WRONG_AMPLITUDE)
        m15c_state_path(slot, 25u, UINT32_C(0x02151FFF),
                         UINT32_C(0x05AA3000), UINT32_C(0x00008000));
#else
        m15c_state_path(slot, 25u, UINT32_C(0x02151FFF),
                         UINT32_C(0x05AA3000), UINT32_C(0x00010000));
#endif
        if (m15c_lhu(slot + 0x20u) == 25u)
            m15c_sw(slot + 0x60u, UINT32_C(0x00010000));
        break;
    case 25: {
        u32 speed = m15c_lw(slot + 0x60u);
        m15c_sh(WM_M15C_ANGLES + 2u,
                 (u16)(m15c_lhu(WM_M15C_ANGLES + 2u) -
                       m15c_sra(speed, 12u)));
        speed -= 512u;
        m15c_sw(slot + 0x60u, speed);
        if (m15c_as_s32(speed) < 0)
            m15c_sw(slot + 0x60u, 0u);
        break;
    }
    default:
        break;
    }
}

static void m15c_apply_jitter(u32 slot)
{
    u32 amplitude = m15c_lw(slot + 0x7Cu);
    s32 divisor = m15c_as_s32(m15c_sra(amplitude, 12u));
    s32 bias = m15c_as_s32(m15c_sra(amplitude, 13u));
    s32 jitter = (s32)rand() % divisor - bias;
    u16 bits = (u16)jitter;

    m15c_sh(WM_M15C_JITTER + 2u, bits);
    m15c_sh(WM_M15C_CAMERA_INPUT + 2u,
             (u16)(m15c_lhu(WM_M15C_CAMERA_INPUT + 2u) + bits));
#if !defined(W34N99_MUTANT_SKIP_JITTER_MIRROR)
    m15c_sh(WM_M15C_CAMERA_INPUT + 0x0Au,
             (u16)(m15c_lhu(WM_M15C_CAMERA_INPUT + 0x0Au) +
                   m15c_lhu(WM_M15C_JITTER + 2u)));
#endif
}

s32 wm_8007E450(s32 slot_index)
{
    u32 slot = m15c_slot(slot_index);
    u32 x = m15c_lw(WM_M15C_RESET_POS + 0u);
    u32 y = m15c_lw(WM_M15C_RESET_POS + 4u);
    u32 z = m15c_lw(WM_M15C_RESET_POS + 8u);

    m15c_sw(WM_M15C_CAMERA_GATE, 0u);
    m15c_sw(slot + 0x7Cu, 4096u);
#if defined(W34N99_MUTANT_WRONG_INIT_VIEW_Y)
    m15c_sw(WM_M15C_VIEW_Y, 140u);
#else
    m15c_sw(WM_M15C_VIEW_Y, 120u);
#endif
    m15c_sw(WM_M15C_LIVE_POS + 0u, x);
    m15c_sw(WM_M15C_SHADOW_POS + 0u, x);
    m15c_sw(WM_M15C_LIVE_POS + 4u, y);
    m15c_sw(WM_M15C_SHADOW_POS + 4u, y);
    m15c_sw(WM_M15C_LIVE_POS + 8u, z);
    m15c_sw(WM_M15C_SHADOW_POS + 8u, z);
    m15c_sh(slot + 4u, 1u);
    m15c_sw(slot + 0x58u, 64u);
    m15c_sh(slot + 0x20u, 0u);
    m15c_sw(slot + 0x50u, 0u);
    return 1;
}

s32 wm_8007E4E4(s32 slot_index)
{
    u32 slot = m15c_slot(slot_index);

    m15c_consume_latch(slot);
    if (m15c_lw(WM_M15C_CAMERA_GATE) == 0u)
        m15c_build_camera(slot);
    m15c_apply_state(slot);
    m15c_apply_jitter(slot);
    return 1;
}
