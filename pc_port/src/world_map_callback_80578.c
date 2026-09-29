/* Exact native transcription of retail [0x80080578, 0x80080900). */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_80578.h"
#include "world_map_helper_96f18.h"

#define WM_M13C_POOL_PTR      UINT32_C(0x8009BE24)
#define WM_M13C_VIEW_HEIGHT   UINT32_C(0x8009BE0C)
#define WM_M13C_RESET_X       UINT32_C(0x8009C5AC)
#define WM_M13C_RESET_Y       UINT32_C(0x8009C5B0)
#define WM_M13C_RESET_Z       UINT32_C(0x8009C5B4)
#define WM_M13C_POSITION      UINT32_C(0x8009BE28)
#define WM_M13C_POSITION_COPY UINT32_C(0x8009D55C)
#define WM_M13C_ANGLES        UINT32_C(0x8009BD38)
#define WM_M13C_CAMERA_INPUT  UINT32_C(0x8009BD40)
#define WM_M13C_CAMERA_HEIGHT UINT32_C(0x8009D3F0)
#define WM_M13C_CAMERA_GATE   UINT32_C(0x8009D144)
#define WM_M13C_JITTER        UINT32_C(0x1F8000A0)

static s16 m13c_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u16 m13c_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 m13c_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void m13c_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void m13c_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static s32 m13c_as_s32(u32 bits)
{
    s32 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static u32 m13c_sra(u32 bits, unsigned shift)
{
    u32 shifted = bits >> shift;

    if ((bits & UINT32_C(0x80000000)) != 0u)
        shifted |= ~(UINT32_MAX >> shift);
    return shifted;
}

static u32 m13c_slot(s32 slot_index)
{
    return m13c_lw(WM_M13C_POOL_PTR) + ((u32)slot_index << 7u);
}

s32 wm_80080578(s32 slot_index)
{
    u32 slot = m13c_slot(slot_index);
    u32 x = m13c_lw(WM_M13C_RESET_X);
    u32 y = m13c_lw(WM_M13C_RESET_Y);
    u32 z = m13c_lw(WM_M13C_RESET_Z);

    m13c_sw(WM_M13C_CAMERA_GATE, 0u);
    m13c_sw(slot + 0x7Cu, 4096u);
#if defined(W34N92_MUTANT_WRONG_VIEW_HEIGHT)
    m13c_sw(WM_M13C_VIEW_HEIGHT, 140u);
#else
    m13c_sw(WM_M13C_VIEW_HEIGHT, 120u);
#endif
    m13c_sw(WM_M13C_POSITION + 0u, x);
    m13c_sw(WM_M13C_POSITION + 4u, y);
    m13c_sw(WM_M13C_POSITION + 8u, z);
    m13c_sw(WM_M13C_POSITION_COPY + 0u, x);
    m13c_sw(WM_M13C_POSITION_COPY + 4u, y);
    m13c_sw(WM_M13C_POSITION_COPY + 8u, z);
    m13c_sh(slot + 0x04u, 1u);
    m13c_sh(slot + 0x20u, 0u);
    return 1;
}

static void m13c_consume_latch(u32 slot)
{
    switch (m13c_lhu(slot + 0x04u)) {
    case 1u: {
        u32 pitch = 2272u;

#if defined(W34N92_MUTANT_WRONG_LATCH1_PITCH)
        pitch = 2273u;
#endif
        m13c_sh(slot + 0x20u, 1u);
        m13c_sh(slot + 0x04u, 0u);
        m13c_sh(WM_M13C_ANGLES + 0u, 16u);
        m13c_sh(WM_M13C_ANGLES + 2u, (u16)pitch);
        m13c_sh(WM_M13C_ANGLES + 4u, 0u);
        m13c_sw(slot + 0x50u, UINT32_C(0x00800000));
        m13c_sw(slot + 0x54u, UINT32_C(0x00010000));
        m13c_sw(slot + 0x58u, UINT32_C(0x00010000));
        m13c_sw(WM_M13C_CAMERA_HEIGHT, UINT32_C(0x00800000));
        m13c_sw(slot + 0x5Cu, pitch << 12u);
        m13c_sw(slot + 0x60u, pitch << 12u);
        break;
    }
    case 2u:
        m13c_sh(slot + 0x20u, 2u);
        m13c_sh(slot + 0x04u, 0u);
        m13c_sw(slot + 0x7Cu, UINT32_C(0x00008000));
        break;
    case 3u:
        m13c_sh(slot + 0x20u, 3u);
        m13c_sh(slot + 0x04u, 0u);
        m13c_sw(slot + 0x7Cu, UINT32_C(0x00080000));
        break;
    default:
        break;
    }
}

static void m13c_build_camera(void)
{
#if !defined(W34N92_MUTANT_SKIP_GENERIC_CAMERA)
    wm_80096F18(WM_M13C_CAMERA_INPUT, WM_M13C_POSITION,
                 m13c_as_s32(m13c_lw(WM_M13C_CAMERA_HEIGHT)),
                 WM_M13C_ANGLES);
#endif
}

static void m13c_run_state_one(u32 slot)
{
    u32 primary = m13c_lw(slot + 0x50u) - UINT32_C(0x00010000);
    u32 current;
    u32 target;
    u32 difference;

#if defined(W34N92_MUTANT_WRONG_PRIMARY_CLAMP)
    if (m13c_as_s32(primary) < INT32_C(0x002F0000))
        primary = UINT32_C(0x002F0000);
#else
    if (m13c_as_s32(primary) < INT32_C(0x00300000))
        primary = UINT32_C(0x00300000);
#endif
    m13c_sw(slot + 0x50u, primary);

    current = m13c_lw(WM_M13C_CAMERA_HEIGHT);
    difference = primary - current;
    if (difference != 0u)
        m13c_sw(WM_M13C_CAMERA_HEIGHT,
                 current + m13c_sra(difference, 5u));

    target = m13c_lw(slot + 0x54u) - UINT32_C(0x00004000);
    if (m13c_as_s32(target) < -INT32_C(0x00100000))
        target = UINT32_C(0xFFF00000);
    m13c_sw(slot + 0x54u, target);
    current = m13c_lw(slot + 0x58u);
    difference = target - current;
    if (difference != 0u) {
        current += m13c_sra(difference, 4u);
        m13c_sw(slot + 0x58u, current);
        m13c_sh(WM_M13C_ANGLES + 0u,
                 (u16)m13c_sra(current, 12u));
    }

    target = m13c_lw(slot + 0x5Cu) - UINT32_C(0x00010000);
    if (m13c_as_s32(target) < INT32_C(0x002E0000))
        target = UINT32_C(0x002E0000);
    m13c_sw(slot + 0x5Cu, target);
    current = m13c_lw(slot + 0x60u);
    difference = target - current;
    if (difference != 0u) {
#if defined(W34N92_MUTANT_WRONG_PITCH_APPROACH_SHIFT)
        current += m13c_sra(difference, 4u);
#else
        current += m13c_sra(difference, 5u);
#endif
        m13c_sw(slot + 0x60u, current);
        m13c_sh(WM_M13C_ANGLES + 2u,
                 (u16)m13c_sra(current, 12u));
    }
}

static void m13c_run_state(u32 slot)
{
    switch (m13c_lh(slot + 0x20u)) {
    case 1:
        m13c_run_state_one(slot);
        break;
    case 3: {
        u32 amplitude = m13c_lw(slot + 0x7Cu) - UINT32_C(0x00004000);

        m13c_sw(slot + 0x7Cu, amplitude);
        if (m13c_as_s32(amplitude) < 4096) {
#if defined(W34N92_MUTANT_WRONG_STATE3_CLAMP)
            m13c_sw(slot + 0x7Cu, 8192u);
#else
            m13c_sw(slot + 0x7Cu, 4096u);
#endif
            m13c_sh(slot + 0x20u, 0u);
        }
        break;
    }
    default:
        break;
    }
}

static void m13c_apply_jitter(u32 slot)
{
    u32 amplitude = m13c_lw(slot + 0x7Cu);
    s32 divisor = m13c_as_s32(m13c_sra(amplitude, 12u));
#if defined(W34N92_MUTANT_WRONG_JITTER_BIAS)
    s32 bias = m13c_as_s32(m13c_sra(amplitude, 12u));
#else
    s32 bias = m13c_as_s32(m13c_sra(amplitude, 13u));
#endif
    s32 jitter = (s32)rand() % divisor - bias;
    u16 bits = (u16)jitter;

    m13c_sh(WM_M13C_JITTER + 2u, bits);
    m13c_sh(WM_M13C_CAMERA_INPUT + 2u,
             (u16)(m13c_lhu(WM_M13C_CAMERA_INPUT + 2u) + bits));
#if !defined(W34N92_MUTANT_SKIP_JITTER_MIRROR)
    m13c_sh(WM_M13C_CAMERA_INPUT + 0x0Au,
             (u16)(m13c_lhu(WM_M13C_CAMERA_INPUT + 0x0Au) +
                   m13c_lhu(WM_M13C_JITTER + 2u)));
#endif
}

s32 wm_80080600(s32 slot_index)
{
    u32 slot = m13c_slot(slot_index);

    m13c_consume_latch(slot);
    if (m13c_lw(WM_M13C_CAMERA_GATE) == 0u)
        m13c_build_camera();
    m13c_run_state(slot);
    m13c_apply_jitter(slot);
    return 1;
}
