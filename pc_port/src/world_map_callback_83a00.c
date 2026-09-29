/* Exact native transcription of retail [0x80083A00, 0x80083FE4). */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_827ec.h"
#include "world_map_callback_83a00.h"
#include "world_map_helper_96f18.h"

#define WM_M18C_POOL_PTR      UINT32_C(0x8009BE24)
#define WM_M18C_RESET_X       UINT32_C(0x8009C5AC)
#define WM_M18C_POSITION      UINT32_C(0x8009BE28)
#define WM_M18C_TARGET        UINT32_C(0x8009D55C)
#define WM_M18C_ANGLES        UINT32_C(0x8009BD38)
#define WM_M18C_CAMERA_INPUT  UINT32_C(0x8009BD40)
#define WM_M18C_CAMERA_HEIGHT UINT32_C(0x8009D3F0)
#define WM_M18C_CAMERA_GATE   UINT32_C(0x8009D144)
#define WM_M18C_WORLD_X       UINT32_C(0x8009BBB4)
#define WM_M18C_WORLD_Z       UINT32_C(0x8009BBBC)
#define WM_M18C_JITTER_X      UINT32_C(0x8009BD42)
#define WM_M18C_JITTER_Z      UINT32_C(0x8009BD4A)
#define WM_M18C_SCRATCH       UINT32_C(0x1F800000)

static s16 m18c_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u16 m18c_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 m18c_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void m18c_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void m18c_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static s32 m18c_as_s32(u32 bits)
{
    s32 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static u32 m18c_as_u32(s32 value)
{
    u32 bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static u32 m18c_sra(u32 bits, unsigned shift)
{
    u32 value = bits >> shift;

    if ((bits & UINT32_C(0x80000000)) != 0u)
        value |= ~(UINT32_MAX >> shift);
    return value;
}

static u32 m18c_slot(s32 slot_index)
{
    return m18c_lw(WM_M18C_POOL_PTR) + ((u32)slot_index << 7u);
}

static void m18c_set_camera(u32 slot, u16 state, u32 height,
                            s16 angle_x, s16 angle_y)
{
    u32 x = (u32)(s32)angle_x << 12u;
    u32 y = (u32)(s32)angle_y << 12u;

    m18c_sh(slot + 0x20u, state);
    m18c_sh(slot + 0x04u, 0u);
    m18c_sw(slot + 0x5Cu, height);
    m18c_sh(WM_M18C_ANGLES + 0u, (u16)angle_x);
    m18c_sh(WM_M18C_ANGLES + 2u, (u16)angle_y);
    m18c_sh(WM_M18C_ANGLES + 4u, 0u);
    m18c_sw(slot + 0x50u, x);
    m18c_sw(slot + 0x38u, x);
    m18c_sw(slot + 0x54u, y);
    m18c_sw(slot + 0x3Cu, y);
    m18c_sw(slot + 0x58u, 0u);
    m18c_sw(slot + 0x40u, 0u);
    m18c_sw(WM_M18C_CAMERA_HEIGHT, height);
}

static void m18c_set_axis(u32 slot, u32 offset, u32 target_address,
                          u32 value)
{
    m18c_sw(WM_M18C_POSITION + offset, value);
    m18c_sw(target_address + offset, value);
    m18c_sw(slot + 0x28u + offset, value);
}

static void m18c_consume_latch(u32 slot)
{
    switch (m18c_lhu(slot + 0x04u)) {
    case 1u:
#if defined(W34N116_MUTANT_LATCH1_WRONG_STATE)
        m18c_set_camera(slot, 2u, UINT32_C(0x00960000), -32, 1024);
#else
        m18c_set_camera(slot, 1u, UINT32_C(0x00960000), -32, 1024);
#endif
        break;
    case 2u:
        m18c_set_camera(slot, 2u, UINT32_C(0x00180000), 64, 416);
#if defined(W34N116_MUTANT_LATCH2_WRONG_Y)
        m18c_set_axis(slot, 4u, WM_M18C_TARGET, UINT32_C(0xFFF70000));
#else
        m18c_set_axis(slot, 4u, WM_M18C_TARGET, UINT32_C(0xFFF60000));
#endif
        break;
    case 3u:
        m18c_set_camera(slot, 3u, UINT32_C(0x00120000), -672, 1696);
        m18c_set_axis(slot, 0u, WM_M18C_TARGET, UINT32_C(0x01700000));
#if defined(W34N116_MUTANT_SKIP_LATCH3_WORLD_X)
        break;
#else
        m18c_sw(WM_M18C_WORLD_X,
                m18c_lw(WM_M18C_WORLD_X) + UINT32_C(0x01700000) -
                m18c_lw(WM_M18C_RESET_X));
        break;
#endif
    case 4u:
        m18c_set_camera(slot, 4u, UINT32_C(0x00180000), -16, 2640);
        m18c_set_axis(slot, 0u, WM_M18C_TARGET, UINT32_C(0x01800000));
        m18c_set_axis(slot, 4u, WM_M18C_TARGET, UINT32_C(0xFFF80000));
        m18c_set_axis(slot, 8u, WM_M18C_TARGET, UINT32_C(0x01900000));
#if defined(W34N116_MUTANT_LATCH4_WRONG_DRIFT)
        m18c_sw(WM_M18C_WORLD_X, m18c_lw(WM_M18C_WORLD_X) - 0x100u);
#else
        m18c_sw(WM_M18C_WORLD_X, m18c_lw(WM_M18C_WORLD_X) + 0x100u);
#endif
        m18c_sw(WM_M18C_WORLD_Z, m18c_lw(WM_M18C_WORLD_Z) - 0x100u);
        break;
    case 5u:
        m18c_set_camera(slot, 5u, UINT32_C(0x00260000), -256, 1648);
        m18c_set_axis(slot, 4u, WM_M18C_TARGET, UINT32_C(0xFFEF0000));
        m18c_set_axis(slot, 8u, WM_M18C_TARGET, UINT32_C(0x01A80000));
        m18c_sw(WM_M18C_WORLD_Z, m18c_lw(WM_M18C_WORLD_Z) + 0x180u);
        break;
    case 6u:
        m18c_set_camera(slot, 6u, UINT32_C(0x00620000), -128, 2672);
        m18c_set_axis(slot, 4u, WM_M18C_TARGET, UINT32_C(0xFFEE0000));
        m18c_set_axis(slot, 8u, WM_M18C_TARGET, UINT32_C(0x01980000));
        m18c_sw(WM_M18C_WORLD_Z, m18c_lw(WM_M18C_WORLD_Z) - 0x100u);
        break;
    default:
        break;
    }
}

static void m18c_approach(u32 address, s32 target, s32 step)
{
    s32 current = m18c_as_s32(m18c_lw(address));
    m18c_sw(address, m18c_as_u32(wm_800771D8(current, target, step)));
}

static void m18c_run_state(u32 slot)
{
    switch (m18c_lh(slot + 0x20u)) {
    case 1:
#if defined(W34N116_MUTANT_STATE1_WRONG_STEP)
        m18c_approach(slot + 0x50u, INT32_C(-0x00400000), INT32_C(0x00008000));
#else
        m18c_approach(slot + 0x50u, INT32_C(-0x00400000), INT32_C(-0x00008000));
#endif
        m18c_approach(slot + 0x54u, 0, INT32_C(-0x00008000));
        m18c_approach(slot + 0x5Cu, INT32_C(0x00380000),
                      INT32_C(-0x0000D000));
        break;
    case 5:
#if defined(W34N116_MUTANT_STATE5_WRONG_TARGET)
        m18c_approach(slot + 0x50u, INT32_C(0x00100000),
                      INT32_C(0x00002000));
#else
        m18c_approach(slot + 0x50u, INT32_C(0x00200000),
                      INT32_C(0x00002000));
#endif
        break;
    default:
        break;
    }
}

static void m18c_apply_jitter(u32 slot)
{
    u32 amplitude = m18c_lw(slot + 0x7Cu);
    s32 divisor = m18c_as_s32(m18c_sra(amplitude, 12u));
    s32 bias = m18c_as_s32(m18c_sra(amplitude, 13u));
    s32 jitter;
    u16 bits;

    if (divisor == 0)
        abort();
    jitter = (s32)rand() % divisor;
#if defined(W34N116_MUTANT_JITTER_NOT_CENTERED)
    (void)bias;
#else
    jitter -= bias;
#endif
    bits = (u16)jitter;
    m18c_sh(WM_M18C_SCRATCH + 0xA2u, bits);
    m18c_sh(WM_M18C_JITTER_X,
            (u16)(m18c_lhu(WM_M18C_JITTER_X) + bits));
    m18c_sh(WM_M18C_JITTER_Z,
            (u16)(m18c_lhu(WM_M18C_JITTER_Z) +
                  m18c_lhu(WM_M18C_SCRATCH + 0xA2u)));
}

s32 wm_80083A00(s32 slot_index)
{
    u32 slot = m18c_slot(slot_index);

    m18c_consume_latch(slot);
#if !defined(W34N116_MUTANT_SKIP_CAMERA_BUILD)
    if (m18c_lw(WM_M18C_CAMERA_GATE) == 0u)
        wm_80096F18(WM_M18C_CAMERA_INPUT, WM_M18C_POSITION,
                    m18c_as_s32(m18c_lw(WM_M18C_CAMERA_HEIGHT)),
                    WM_M18C_ANGLES);
#endif
    m18c_run_state(slot);
#if !defined(W34N116_MUTANT_SKIP_MOTION_HELPERS)
    wm_80076DA4(slot, WM_M18C_SCRATCH);
    wm_80076F54(slot, WM_M18C_SCRATCH);
    wm_80076FA8(slot, WM_M18C_SCRATCH);
#endif
    m18c_apply_jitter(slot);
    return 1;
}
