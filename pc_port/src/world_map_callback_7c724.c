/* Exact native transcription of retail [0x8007C724, 0x8007CC6C). */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_7c724.h"
#include "world_map_common_tail.h"
#include "world_map_helper_76858.h"
#include "world_map_helper_93484.h"
#include "world_map_helper_96f18.h"
#include "world_map_helper_97070.h"
#include "world_map_helper_97244.h"

#define WM_M12C_POOL_PTR       UINT32_C(0x8009BE24)
#define WM_M12C_VIEW_HEIGHT    UINT32_C(0x8009BE0C)
#define WM_M12C_CAMERA_HEIGHT  UINT32_C(0x8009D3F0)
#define WM_M12C_CAMERA_GATE    UINT32_C(0x8009D144)
#define WM_M12C_RESET_X        UINT32_C(0x8009C5AC)
#define WM_M12C_RESET_Y        UINT32_C(0x8009C5B0)
#define WM_M12C_RESET_Z        UINT32_C(0x8009C5B4)
#define WM_M12C_LIVE_X         UINT32_C(0x8009BE28)
#define WM_M12C_LIVE_Y         UINT32_C(0x8009BE2C)
#define WM_M12C_LIVE_Z         UINT32_C(0x8009BE30)
#define WM_M12C_SHADOW_X       UINT32_C(0x8009D55C)
#define WM_M12C_SHADOW_Y       UINT32_C(0x8009D560)
#define WM_M12C_SHADOW_Z       UINT32_C(0x8009D564)
#define WM_M12C_ANGLES         UINT32_C(0x8009BD38)
#define WM_M12C_CAMERA_INPUT   UINT32_C(0x8009BD40)
#define WM_M12C_CAMERA_MATRIX  UINT32_C(0x8009C808)
#define WM_M12C_WORLD_Z_ACCUM  UINT32_C(0x8009BBBC)
#define WM_M12C_PATH_NORMAL    UINT32_C(0x8009A4F8)
#define WM_M12C_PATH_FINAL     UINT32_C(0x8009A568)
#define WM_M12C_SCRATCH        UINT32_C(0x1F800000)
#define WM_M12C_MARKER         UINT32_C(0x1F8000A0)

static s16 m12c_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u16 m12c_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 m12c_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void m12c_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void m12c_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static s32 m12c_as_s32(u32 bits)
{
    s32 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static u32 m12c_sra(u32 bits, unsigned shift)
{
    u32 shifted = bits >> shift;

    if ((bits & UINT32_C(0x80000000)) != 0u)
        shifted |= ~(UINT32_MAX >> shift);
    return shifted;
}

static u32 m12c_slot(s32 slot_index)
{
    return m12c_lw(WM_M12C_POOL_PTR) + ((u32)slot_index << 7u);
}

static void m12c_apply_latch(u32 slot)
{
    switch (m12c_lhu(slot + 0x04u)) {
    case 1u:
        m12c_sh(slot + 0x20u, 1u);
#if defined(W34N86_MUTANT_WRONG_LATCH1_THRESHOLD)
        m12c_sw(slot + 0x5Cu, UINT32_C(0x00008201));
#else
        m12c_sw(slot + 0x5Cu, UINT32_C(0x00008200));
#endif
        m12c_sh(slot + 0x04u, 0u);
        m12c_sw(slot + 0x60u, 1u);
        break;
    case 2u:
        m12c_sh(slot + 0x04u, 0u);
        m12c_sh(slot + 0x20u, 3u);
        break;
    case 3u:
        m12c_sh(slot + 0x04u, 0u);
        m12c_sh(slot + 0x20u, 4u);
        break;
    case 4u:
        m12c_sh(slot + 0x20u, 5u);
        m12c_sw(slot + 0x5Cu, UINT32_C(0x0000A800));
        m12c_sh(slot + 0x04u, 0u);
        m12c_sw(slot + 0x60u, 8u);
        break;
    case 6u:
        m12c_sh(slot + 0x20u, 6u);
        m12c_sh(slot + 0x04u, 0u);
        m12c_sw(slot + 0x50u, 0u);
        m12c_sw(slot + 0x58u, 64u);
        break;
    default:
        break;
    }
}

static void m12c_track_shadow_z(int restore_xy)
{
    u32 difference = m12c_lw(WM_M12C_SHADOW_Z) -
                     m12c_lw(WM_M12C_LIVE_Z);

    m12c_sw(WM_M12C_SCRATCH + 0u, 0u);
    m12c_sw(WM_M12C_SCRATCH + 4u, 0u);
    m12c_sw(WM_M12C_SCRATCH + 8u, difference);
    wm_80093484(WM_M12C_SCRATCH);
    m12c_sw(WM_M12C_WORLD_Z_ACCUM,
             m12c_lw(WM_M12C_WORLD_Z_ACCUM) +
             m12c_lw(WM_M12C_SCRATCH + 8u));
    if (restore_xy != 0) {
        m12c_sw(WM_M12C_LIVE_X, m12c_lw(WM_M12C_SHADOW_X));
        m12c_sw(WM_M12C_LIVE_Y, m12c_lw(WM_M12C_SHADOW_Y));
    }
    m12c_sw(WM_M12C_LIVE_Z, m12c_lw(WM_M12C_SHADOW_Z));
}

static void m12c_update_camera(u32 phase, u32 table_base)
{
    u32 record = table_base + m12c_sra(phase, 12u) * 8u;

    if (m12c_lh(record + 0x16u) != -1) {
#if defined(W34N86_MUTANT_WRONG_BLEND_REMAINDER)
        s32 remainder = (s32)(phase & UINT32_C(0x07FF));
#else
        s32 remainder = (s32)(phase & UINT32_C(0x0FFF));
#endif
        wm_80076858(remainder, record, record + 8u, record + 16u,
                     WM_M12C_SCRATCH);
        m12c_sh(WM_M12C_CAMERA_INPUT + 0x0Cu, 0u);
        m12c_sh(WM_M12C_CAMERA_INPUT + 0x08u, 0u);
        m12c_sw(WM_M12C_CAMERA_INPUT + 0x18u, 0u);
        m12c_sw(WM_M12C_CAMERA_INPUT + 0x10u, 0u);
        m12c_sw(WM_M12C_CAMERA_INPUT + 0x14u,
                 UINT32_C(0xFFFFF000));
        m12c_sh(WM_M12C_CAMERA_INPUT + 0x00u,
                 m12c_lhu(WM_M12C_SCRATCH + 2u));
        m12c_sh(WM_M12C_CAMERA_INPUT + 0x04u,
                 (u16)(0u - m12c_sra(m12c_lw(WM_M12C_SCRATCH + 8u),
                                     16u)));
        m12c_sh(WM_M12C_CAMERA_INPUT + 0x0Au,
                 (u16)m12c_sra(m12c_lw(WM_M12C_LIVE_Y), 12u));
        m12c_sh(WM_M12C_CAMERA_INPUT + 0x02u,
                 (u16)m12c_sra(m12c_lw(WM_M12C_SCRATCH + 4u), 16u));
    }
    wm_80097244(WM_M12C_CAMERA_INPUT);
    wm_80097070(WM_M12C_CAMERA_MATRIX, WM_M12C_ANGLES);
}

static void m12c_normal_state_path(u32 slot)
{
    m12c_track_shadow_z(0);
    m12c_update_camera(m12c_lw(slot + 0x50u), WM_M12C_PATH_NORMAL);
}

static void m12c_state_four(u32 slot)
{
    u32 amplitude;

    m12c_sh(WM_M12C_MARKER + 2u, 0u);
    m12c_sh(WM_M12C_MARKER + 0u,
             (u16)m12c_sra(m12c_lw(WM_M12C_LIVE_X), 12u));
    m12c_sh(WM_M12C_MARKER + 4u,
             (u16)m12c_sra(m12c_lw(WM_M12C_LIVE_Z), 12u));
    wm_80089160(26u, WM_M12C_MARKER, 0u);
#if defined(W34N86_MUTANT_WRONG_STATE4_MARKER)
    wm_80089160(28u, WM_M12C_MARKER, 0u);
#else
    wm_80089160(27u, WM_M12C_MARKER, 0u);
#endif

    amplitude = m12c_lw(slot + 0x7Cu) - 288u;
    m12c_sw(slot + 0x7Cu, amplitude);
    if (m12c_as_s32(amplitude) < 4096) {
        m12c_sw(slot + 0x7Cu, 4096u);
        m12c_sh(slot + 0x20u, 0u);
        wm_800894C8(26u);
        wm_800894C8(27u);
    }
    m12c_normal_state_path(slot);
}

static void m12c_state_six(u32 slot)
{
    u32 phase;
    u32 speed;

    m12c_track_shadow_z(1);
    phase = m12c_lw(slot + 0x50u) + m12c_lw(slot + 0x58u);
    m12c_sw(slot + 0x50u, phase);
    if (m12c_as_s32(phase) >= 18433) {
        speed = m12c_lw(slot + 0x58u) - 1u;
        m12c_sw(slot + 0x58u, speed);
        if (m12c_as_s32(speed) < 0)
            m12c_sw(slot + 0x58u, 0u);
    }
#if defined(W34N86_MUTANT_WRONG_STATE6_TABLE)
    m12c_update_camera(phase, WM_M12C_PATH_NORMAL);
#else
    m12c_update_camera(phase, WM_M12C_PATH_FINAL);
#endif
}

static void m12c_apply_state(u32 slot)
{
    switch (m12c_lh(slot + 0x20u)) {
    case 0:
        m12c_normal_state_path(slot);
        break;
    case 1: {
        u32 phase = m12c_lw(slot + 0x50u) + m12c_lw(slot + 0x58u);

        m12c_sw(slot + 0x50u, phase);
        if (m12c_as_s32(m12c_lw(slot + 0x5Cu)) < m12c_as_s32(phase))
            m12c_sh(slot + 0x20u, 2u);
        m12c_normal_state_path(slot);
        break;
    }
    case 2: {
        u32 speed = m12c_lw(slot + 0x58u) - m12c_lw(slot + 0x60u);
        u32 phase;

        m12c_sw(slot + 0x58u, speed);
        if (m12c_as_s32(speed) < 0) {
#if defined(W34N86_MUTANT_WRONG_STATE2_CLAMP)
            m12c_sw(slot + 0x58u, 1u);
#else
            m12c_sw(slot + 0x58u, 0u);
#endif
            m12c_sh(slot + 0x20u, 0u);
        }
        phase = m12c_lw(slot + 0x50u) + m12c_lw(slot + 0x58u);
        m12c_sw(slot + 0x50u, phase);
        if (m12c_as_s32(m12c_lw(slot + 0x5Cu)) < m12c_as_s32(phase))
            m12c_sh(slot + 0x20u, 2u);
        m12c_normal_state_path(slot);
        break;
    }
    case 3:
        m12c_sw(slot + 0x7Cu, UINT32_C(0x0000F000));
        m12c_sh(slot + 0x20u, 0u);
        m12c_normal_state_path(slot);
        break;
    case 4:
        m12c_state_four(slot);
        break;
    case 5:
        m12c_sw(slot + 0x58u, 256u);
        m12c_sh(slot + 0x20u, 1u);
        m12c_normal_state_path(slot);
        break;
    case 6:
        m12c_state_six(slot);
        break;
    default:
        break;
    }
}

static void m12c_apply_jitter(u32 slot)
{
    u32 amplitude_bits = m12c_lw(slot + 0x7Cu);
    s32 divisor = m12c_as_s32(m12c_sra(amplitude_bits, 12u));
    s32 half_range = m12c_as_s32(m12c_sra(amplitude_bits, 13u));
    s32 jitter = (s32)rand() % divisor - half_range;

    m12c_sh(WM_M12C_MARKER + 2u, (u16)jitter);
#if defined(W34N86_MUTANT_SKIP_JITTER_MIRROR)
    m12c_sh(WM_M12C_CAMERA_INPUT + 0x02u,
             (u16)(m12c_lhu(WM_M12C_CAMERA_INPUT + 0x02u) +
                   (u16)jitter));
#else
    m12c_sh(WM_M12C_CAMERA_INPUT + 0x02u,
             (u16)(m12c_lhu(WM_M12C_CAMERA_INPUT + 0x02u) +
                   (u16)jitter));
    m12c_sh(WM_M12C_CAMERA_INPUT + 0x0Au,
             (u16)(m12c_lhu(WM_M12C_CAMERA_INPUT + 0x0Au) +
                   m12c_lhu(WM_M12C_MARKER + 2u)));
#endif
}

s32 wm_8007C724(s32 slot_index)
{
    u32 slot = m12c_slot(slot_index);
    u32 x = m12c_lw(WM_M12C_RESET_X);
    u32 y = m12c_lw(WM_M12C_RESET_Y);
    u32 z = m12c_lw(WM_M12C_RESET_Z);

#if defined(W34N86_MUTANT_WRONG_INIT_VIEW_HEIGHT)
    m12c_sw(WM_M12C_VIEW_HEIGHT, 140u);
#else
    m12c_sw(WM_M12C_VIEW_HEIGHT, 120u);
#endif
    m12c_sw(WM_M12C_CAMERA_HEIGHT, UINT32_C(0x00400000));
    m12c_sw(WM_M12C_CAMERA_GATE, 1u);
    m12c_sw(slot + 0x7Cu, 4096u);
    m12c_sh(WM_M12C_ANGLES + 0u, UINT16_C(0xFFC0));
    m12c_sh(WM_M12C_ANGLES + 2u, 0u);
    m12c_sh(WM_M12C_ANGLES + 4u, 0u);
    m12c_sw(WM_M12C_LIVE_X, x);
    m12c_sw(WM_M12C_SHADOW_X, x);
    m12c_sw(WM_M12C_LIVE_Y, y);
    m12c_sw(WM_M12C_SHADOW_Y, y);
    m12c_sw(WM_M12C_LIVE_Z, z);
    m12c_sw(WM_M12C_SHADOW_Z, z);
    m12c_sw(slot + 0x58u, 64u);
    m12c_sw(slot + 0x50u, 0u);
    return 1;
}

s32 wm_8007C7D8(s32 slot_index)
{
    u32 slot = m12c_slot(slot_index);

    m12c_apply_latch(slot);
    if (m12c_lw(WM_M12C_CAMERA_GATE) == 0u) {
#if !defined(W34N86_MUTANT_SKIP_GENERIC_CAMERA)
        wm_80096F18(WM_M12C_CAMERA_INPUT, WM_M12C_LIVE_X,
                     m12c_as_s32(m12c_lw(WM_M12C_CAMERA_HEIGHT)),
                     WM_M12C_ANGLES);
#endif
    }
    m12c_apply_state(slot);
    m12c_apply_jitter(slot);
    return 1;
}
