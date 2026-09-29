/* Exact native transcription of retail [0x800813E8, 0x800817A0). */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_813e8.h"
#include "world_map_helper_96f18.h"

#define WM_M16C_POOL_PTR      UINT32_C(0x8009BE24)
#define WM_M16C_VIEW_HEIGHT   UINT32_C(0x8009BE0C)
#define WM_M16C_RESET_X       UINT32_C(0x8009C5AC)
#define WM_M16C_RESET_Y       UINT32_C(0x8009C5B0)
#define WM_M16C_RESET_Z       UINT32_C(0x8009C5B4)
#define WM_M16C_POSITION      UINT32_C(0x8009BE28)
#define WM_M16C_POSITION_COPY UINT32_C(0x8009D55C)
#define WM_M16C_ANGLES        UINT32_C(0x8009BD38)
#define WM_M16C_CAMERA_INPUT  UINT32_C(0x8009BD40)
#define WM_M16C_CAMERA_HEIGHT UINT32_C(0x8009D3F0)
#define WM_M16C_CAMERA_GATE   UINT32_C(0x8009D144)
#define WM_M16C_JITTER        UINT32_C(0x1F8000A0)

static s16 m16c_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u16 m16c_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 m16c_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void m16c_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void m16c_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static s32 m16c_as_s32(u32 bits)
{
    s32 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static u32 m16c_sra(u32 bits, unsigned shift)
{
    u32 shifted = bits >> shift;

    if ((bits & UINT32_C(0x80000000)) != 0u)
        shifted |= ~(UINT32_MAX >> shift);
    return shifted;
}

static u32 m16c_slot(s32 slot_index)
{
    return m16c_lw(WM_M16C_POOL_PTR) + ((u32)slot_index << 7u);
}

s32 wm_800813E8(s32 slot_index)
{
    u32 slot = m16c_slot(slot_index);
    u32 x = m16c_lw(WM_M16C_RESET_X);
    u32 y = m16c_lw(WM_M16C_RESET_Y);
    u32 z = m16c_lw(WM_M16C_RESET_Z);

    m16c_sw(WM_M16C_CAMERA_GATE, 0u);
    m16c_sw(slot + 0x7Cu, 4096u);
#if defined(W34N104_MUTANT_WRONG_INIT_VIEW_HEIGHT)
    m16c_sw(WM_M16C_VIEW_HEIGHT, 140u);
#else
    m16c_sw(WM_M16C_VIEW_HEIGHT, 120u);
#endif
    m16c_sw(WM_M16C_POSITION + 0u, x);
    m16c_sw(WM_M16C_POSITION_COPY + 0u, x);
    m16c_sw(WM_M16C_POSITION + 4u, y);
    m16c_sw(WM_M16C_POSITION_COPY + 4u, y);
    m16c_sw(WM_M16C_POSITION + 8u, z);
    m16c_sw(WM_M16C_POSITION_COPY + 8u, z);
    m16c_sh(slot + 0x04u, 1u);
    m16c_sh(slot + 0x20u, 0u);
    return 1;
}

static void m16c_consume_latch(u32 slot)
{
    if (m16c_lhu(slot + 0x04u) == 1u) {
        u32 pitch = UINT32_C(0xFFFFFE00);

#if defined(W34N104_MUTANT_WRONG_LATCH1_PITCH)
        pitch = UINT32_C(0xFFFFFE01);
#endif
        m16c_sh(slot + 0x20u, 1u);
        m16c_sh(slot + 0x04u, 0u);
        m16c_sh(WM_M16C_ANGLES + 0u, UINT16_C(0xFF80));
        m16c_sh(WM_M16C_ANGLES + 2u, (u16)pitch);
        m16c_sh(WM_M16C_ANGLES + 4u, 0u);
        m16c_sw(slot + 0x50u, UINT32_C(0x00980000));
        m16c_sw(slot + 0x54u, UINT32_C(0xFFF80000));
        m16c_sw(slot + 0x58u, UINT32_C(0xFFF80000));
        m16c_sw(WM_M16C_CAMERA_HEIGHT, UINT32_C(0x00980000));
        m16c_sw(slot + 0x5Cu, pitch << 12u);
        m16c_sw(slot + 0x60u, pitch << 12u);
    }
}

static void m16c_build_camera(void)
{
#if !defined(W34N104_MUTANT_SKIP_GENERIC_CAMERA)
    wm_80096F18(WM_M16C_CAMERA_INPUT, WM_M16C_POSITION,
                 m16c_as_s32(m16c_lw(WM_M16C_CAMERA_HEIGHT)),
                 WM_M16C_ANGLES);
#endif
}

static void m16c_run_state_one(u32 slot)
{
    u32 target = m16c_lw(slot + 0x50u) - UINT32_C(0x00010000);
    u32 current;
    u32 difference;

#if defined(W34N104_MUTANT_WRONG_PRIMARY_CLAMP)
    if (m16c_as_s32(target) < INT32_C(0x00620000))
        target = UINT32_C(0x00620000);
#else
    if (m16c_as_s32(target) < INT32_C(0x00630000))
        target = UINT32_C(0x00630000);
#endif
    m16c_sw(slot + 0x50u, target);
    current = m16c_lw(WM_M16C_CAMERA_HEIGHT);
    difference = target - current;
    if (difference != 0u)
        m16c_sw(WM_M16C_CAMERA_HEIGHT,
                 current + m16c_sra(difference, 5u));

    target = m16c_lw(slot + 0x54u) + UINT32_C(0x00001000);
    if (m16c_as_s32(target) > INT32_C(0x00010000))
        target = UINT32_C(0x00010000);
    m16c_sw(slot + 0x54u, target);
    current = m16c_lw(slot + 0x58u);
    difference = target - current;
    if (difference != 0u) {
#if defined(W34N104_MUTANT_WRONG_ROLL_APPROACH_SHIFT)
        current += m16c_sra(difference, 5u);
#else
        current += m16c_sra(difference, 4u);
#endif
        m16c_sw(slot + 0x58u, current);
        m16c_sh(WM_M16C_ANGLES + 0u,
                 (u16)m16c_sra(current, 12u));
    }

    target = m16c_lw(slot + 0x5Cu) + UINT32_C(0x00010000);
    if (m16c_as_s32(target) > INT32_C(0x004B0000))
        target = UINT32_C(0x004B0000);
    m16c_sw(slot + 0x5Cu, target);
    current = m16c_lw(slot + 0x60u);
    difference = target - current;
    if (difference != 0u) {
#if defined(W34N104_MUTANT_WRONG_PITCH_APPROACH_SHIFT)
        current += m16c_sra(difference, 4u);
#else
        current += m16c_sra(difference, 5u);
#endif
        m16c_sw(slot + 0x60u, current);
        m16c_sh(WM_M16C_ANGLES + 2u,
                 (u16)m16c_sra(current, 12u));
    }
}

static void m16c_apply_jitter(u32 slot)
{
    u32 amplitude = m16c_lw(slot + 0x7Cu);
    s32 divisor = m16c_as_s32(m16c_sra(amplitude, 12u));
#if defined(W34N104_MUTANT_WRONG_JITTER_BIAS)
    s32 bias = m16c_as_s32(m16c_sra(amplitude, 12u));
#else
    s32 bias = m16c_as_s32(m16c_sra(amplitude, 13u));
#endif
    s32 jitter = (s32)rand() % divisor - bias;
    u16 bits = (u16)jitter;

    m16c_sh(WM_M16C_JITTER + 2u, bits);
    m16c_sh(WM_M16C_CAMERA_INPUT + 2u,
             (u16)(m16c_lhu(WM_M16C_CAMERA_INPUT + 2u) + bits));
#if !defined(W34N104_MUTANT_SKIP_JITTER_MIRROR)
    m16c_sh(WM_M16C_CAMERA_INPUT + 0x0Au,
             (u16)(m16c_lhu(WM_M16C_CAMERA_INPUT + 0x0Au) +
                   m16c_lhu(WM_M16C_JITTER + 2u)));
#endif
}

s32 wm_80081470(s32 slot_index)
{
    u32 slot = m16c_slot(slot_index);

    m16c_consume_latch(slot);
    if (m16c_lw(WM_M16C_CAMERA_GATE) == 0u)
        m16c_build_camera();
    if (m16c_lh(slot + 0x20u) == 1)
        m16c_run_state_one(slot);
    m16c_apply_jitter(slot);
    return 1;
}
