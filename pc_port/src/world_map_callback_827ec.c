/*
 * Exact native transcription of the retail mode-17 slot-2 controller.
 *
 * Retail boundaries:
 *   wm_80076DA4 [0x80076DA4, 0x80076F54)
 *   wm_80076F54 [0x80076F54, 0x80076FA8)
 *   wm_80076FA8 [0x80076FA8, 0x800771D8)
 *   wm_800771D8 [0x800771D8, 0x80077214)
 *   wm_800827EC [0x800827EC, 0x800828DC)
 *   wm_800828DC [0x800828DC, 0x80082F64)
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_827ec.h"
#include "world_map_helper_93484.h"
#include "world_map_helper_93354.h"
#include "world_map_helper_96f18.h"

#define WM_M17K_POOL_PTR      UINT32_C(0x8009BE24)
#define WM_M17K_RESET_X       UINT32_C(0x8009C5AC)
#define WM_M17K_RESET_Y       UINT32_C(0x8009C5B0)
#define WM_M17K_RESET_Z       UINT32_C(0x8009C5B4)
#define WM_M17K_POSITION      UINT32_C(0x8009BE28)
#define WM_M17K_TARGET        UINT32_C(0x8009D55C)
#define WM_M17K_ANGLES        UINT32_C(0x8009BD38)
#define WM_M17K_CAMERA_INPUT  UINT32_C(0x8009BD40)
#define WM_M17K_CAMERA_HEIGHT UINT32_C(0x8009D3F0)
#define WM_M17K_CAMERA_GATE   UINT32_C(0x8009D144)
#define WM_M17K_VIEW_HEIGHT   UINT32_C(0x8009BE0C)
#define WM_M17K_WORLD_X       UINT32_C(0x8009BBB4)
#define WM_M17K_WORLD_Z       UINT32_C(0x8009BBBC)
#define WM_M17K_JITTER_X      UINT32_C(0x8009BD42)
#define WM_M17K_JITTER_Z      UINT32_C(0x8009BD4A)
#define WM_M17K_SCRATCH       UINT32_C(0x1F800000)

static s16 m17k_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u16 m17k_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 m17k_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void m17k_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void m17k_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static s32 m17k_as_s32(u32 bits)
{
    s32 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static u32 m17k_as_u32(s32 value)
{
    u32 bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static u32 m17k_sra(u32 bits, unsigned shift)
{
    u32 value = bits >> shift;

    if ((bits & UINT32_C(0x80000000)) != 0u)
        value |= ~(UINT32_MAX >> shift);
    return value;
}

static u32 m17k_abs_bits(u32 bits)
{
    if (m17k_as_s32(bits) < 0)
        return 0u - bits;
    return bits;
}

static u32 m17k_slot(s32 slot_index)
{
    return m17k_lw(WM_M17K_POOL_PTR) + ((u32)slot_index << 7u);
}

s32 wm_800771D8(s32 current, s32 target, s32 step)
{
    u32 current_bits = m17k_as_u32(current);
    u32 target_bits = m17k_as_u32(target);
    u32 step_bits = m17k_as_u32(step);
    u32 distance;
    u32 magnitude;

    if (current_bits == target_bits)
        return current;
    distance = m17k_abs_bits(target_bits - current_bits);
    magnitude = m17k_abs_bits(step_bits);
#if defined(W34N113_MUTANT_APPROACH_OVERSHOOTS)
    if (m17k_as_s32(distance) <= m17k_as_s32(magnitude))
#else
    if (m17k_as_s32(distance) < m17k_as_s32(magnitude))
#endif
        return target;
    return m17k_as_s32(current_bits + step_bits);
}

static void m17k_approach_component(u32 slot, u32 work,
                                    u32 current_offset, u32 target_offset,
                                    u32 work_offset)
{
    u32 current = m17k_lw(slot + current_offset);
    u32 target = m17k_lw(slot + target_offset);
    u32 step = m17k_lw(work + work_offset + 0x10u);

#if defined(W34N113_MUTANT_ROTATION_THRESHOLD_512)
    if (m17k_as_s32(m17k_abs_bits(step)) < 512)
#else
    if (m17k_as_s32(m17k_abs_bits(step)) < 64)
#endif
        current = target;
    else
        current += step;
    m17k_sw(slot + current_offset, current);
}

void wm_80076DA4(u32 slot, u32 work)
{
    u32 delta;

    if (m17k_lw(slot + 0x38u) != m17k_lw(slot + 0x50u) ||
        m17k_lw(slot + 0x3Cu) != m17k_lw(slot + 0x54u) ||
        m17k_lw(slot + 0x40u) != m17k_lw(slot + 0x58u)) {
        m17k_sw(work + 0u,
                 m17k_lw(slot + 0x50u) - m17k_lw(slot + 0x38u));
        m17k_sw(work + 4u,
                 m17k_lw(slot + 0x54u) - m17k_lw(slot + 0x3Cu));
        m17k_sw(work + 8u,
                 m17k_lw(slot + 0x58u) - m17k_lw(slot + 0x40u));
        wm_80093484(work);
        delta = m17k_sra(m17k_lw(work + 0u), 3u);
        m17k_sw(work + 0x10u, delta);
        delta = m17k_sra(m17k_lw(work + 4u), 3u);
        m17k_sw(work + 0x14u, delta);
        delta = m17k_sra(m17k_lw(work + 8u), 3u);
        m17k_sw(work + 0x18u, delta);
        m17k_approach_component(slot, work, 0x38u, 0x50u, 0u);
        m17k_approach_component(slot, work, 0x3Cu, 0x54u, 4u);
        m17k_approach_component(slot, work, 0x40u, 0x58u, 8u);
    }
    m17k_sh(WM_M17K_ANGLES + 0u,
             (u16)m17k_sra(m17k_lw(slot + 0x38u), 12u));
    m17k_sh(WM_M17K_ANGLES + 2u,
             (u16)m17k_sra(m17k_lw(slot + 0x3Cu), 12u));
    m17k_sh(WM_M17K_ANGLES + 4u,
             (u16)m17k_sra(m17k_lw(slot + 0x40u), 12u));
}

void wm_80076F54(u32 slot, u32 work)
{
    u32 target = m17k_lw(slot + 0x5Cu);
    u32 current = m17k_lw(WM_M17K_CAMERA_HEIGHT);

    (void)work;
    if (target != current) {
        u32 step = m17k_sra(target - current, 3u);

        if (m17k_as_s32(m17k_abs_bits(step)) < 64)
            current = target;
        else
            current += step;
        m17k_sw(WM_M17K_CAMERA_HEIGHT, current);
    }
}

static void m17k_approach_position(u32 slot, u32 work, u32 slot_offset,
                                   u32 target_address, u32 work_offset,
                                   u32 accumulator_address)
{
    u32 current = m17k_lw(slot + slot_offset);
    u32 target = m17k_lw(target_address);
    u32 full_delta = m17k_lw(work + work_offset);
    u32 step = m17k_lw(work + work_offset + 0x10u);
    u32 applied = step;

#if defined(W34N113_MUTANT_POSITION_THRESHOLD_64)
    if (m17k_as_s32(m17k_abs_bits(step)) < 64) {
#else
    if (m17k_as_s32(m17k_abs_bits(step)) < 512) {
#endif
        current = target;
        applied = full_delta;
    } else {
        current += step;
    }
    if (accumulator_address != 0u)
        m17k_sw(accumulator_address,
                 m17k_lw(accumulator_address) + applied);
    m17k_sw(slot + slot_offset, current);
}

void wm_80076FA8(u32 slot, u32 work)
{
    if (m17k_lw(slot + 0x28u) != m17k_lw(WM_M17K_TARGET + 0u) ||
        m17k_lw(slot + 0x2Cu) != m17k_lw(WM_M17K_TARGET + 4u) ||
        m17k_lw(slot + 0x30u) != m17k_lw(WM_M17K_TARGET + 8u)) {
        m17k_sw(work + 0u,
                 m17k_lw(WM_M17K_TARGET + 0u) -
                 m17k_lw(slot + 0x28u));
        m17k_sw(work + 4u,
                 m17k_lw(WM_M17K_TARGET + 4u) -
                 m17k_lw(slot + 0x2Cu));
        m17k_sw(work + 8u,
                 m17k_lw(WM_M17K_TARGET + 8u) -
                 m17k_lw(slot + 0x30u));
        wm_80093484(work);
        m17k_sw(work + 0x10u, m17k_sra(m17k_lw(work + 0u), 3u));
        m17k_sw(work + 0x14u, m17k_sra(m17k_lw(work + 4u), 3u));
        m17k_sw(work + 0x18u, m17k_sra(m17k_lw(work + 8u), 3u));
        m17k_approach_position(slot, work, 0x28u,
                               WM_M17K_TARGET + 0u, 0u,
                               WM_M17K_WORLD_X);
        m17k_approach_position(slot, work, 0x2Cu,
                               WM_M17K_TARGET + 4u, 4u, 0u);
        m17k_approach_position(slot, work, 0x30u,
                               WM_M17K_TARGET + 8u, 8u,
                               WM_M17K_WORLD_Z);
    }
    m17k_sw(WM_M17K_POSITION + 0u, m17k_lw(slot + 0x28u));
    m17k_sw(WM_M17K_POSITION + 4u, m17k_lw(slot + 0x2Cu));
    m17k_sw(WM_M17K_POSITION + 8u, m17k_lw(slot + 0x30u));
    m17k_sw(WM_M17K_POSITION + 0x0Cu, m17k_lw(slot + 0x34u));
}

s32 wm_800827EC(s32 slot_index)
{
    u32 slot = m17k_slot(slot_index);
    u32 x = m17k_lw(WM_M17K_RESET_X);
    u32 y = m17k_lw(WM_M17K_RESET_Y);
    u32 z = m17k_lw(WM_M17K_RESET_Z);

    m17k_sw(slot + 0x7Cu, 4096u);
    m17k_sw(WM_M17K_POSITION + 0u, x);
    m17k_sw(WM_M17K_TARGET + 0u, x);
    m17k_sw(slot + 0x28u, x);
    m17k_sw(WM_M17K_POSITION + 4u, y);
    m17k_sw(WM_M17K_TARGET + 4u, y);
    m17k_sw(slot + 0x2Cu, y);
    m17k_sw(WM_M17K_POSITION + 8u, z);
    m17k_sw(WM_M17K_TARGET + 8u, z);
    m17k_sw(slot + 0x30u, z);
    m17k_sh(slot + 0x20u, 0u);
    m17k_sh(slot + 0x04u, 0u);
    m17k_sw(slot + 0x5Cu, UINT32_C(0x00500000));
#if defined(W34N113_MUTANT_WRONG_INITIAL_ANGLE)
    m17k_sh(WM_M17K_ANGLES + 0u, UINT16_C(0xFE00));
#else
    m17k_sh(WM_M17K_ANGLES + 0u, UINT16_C(0xFDE0));
#endif
    m17k_sh(WM_M17K_ANGLES + 2u, 0u);
    m17k_sh(WM_M17K_ANGLES + 4u, 0u);
    m17k_sw(slot + 0x50u, UINT32_C(0xFFDE0000));
    m17k_sw(slot + 0x38u, UINT32_C(0xFFDE0000));
    m17k_sw(WM_M17K_CAMERA_GATE, 0u);
    m17k_sw(WM_M17K_CAMERA_HEIGHT, UINT32_C(0x00500000));
    m17k_sw(slot + 0x54u, 0u);
    m17k_sw(slot + 0x3Cu, 0u);
    m17k_sw(slot + 0x58u, 0u);
    m17k_sw(slot + 0x40u, 0u);
#if defined(W34N113_MUTANT_WRONG_VIEW_HEIGHT)
    m17k_sw(WM_M17K_VIEW_HEIGHT, 140u);
#else
    m17k_sw(WM_M17K_VIEW_HEIGHT, 120u);
#endif
    return 1;
}

static void m17k_set_camera_targets(u32 slot, u32 camera_height,
                                    s16 angle_x, s16 angle_y)
{
    m17k_sw(slot + 0x5Cu, camera_height);
    m17k_sh(WM_M17K_ANGLES + 0u, (u16)angle_x);
    m17k_sh(WM_M17K_ANGLES + 2u, (u16)angle_y);
    m17k_sh(WM_M17K_ANGLES + 4u, 0u);
    m17k_sw(slot + 0x50u, (u32)(s32)angle_x << 12u);
    m17k_sw(slot + 0x38u, (u32)(s32)angle_x << 12u);
    m17k_sw(slot + 0x54u, (u32)(s32)angle_y << 12u);
    m17k_sw(slot + 0x3Cu, (u32)(s32)angle_y << 12u);
    m17k_sw(slot + 0x58u, 0u);
    m17k_sw(slot + 0x40u, 0u);
    m17k_sw(WM_M17K_CAMERA_HEIGHT, camera_height);
}

static void m17k_set_position_targets(u32 slot, u32 target_x,
                                      u32 target_y, u32 target_z)
{
    m17k_sw(WM_M17K_TARGET + 0u, target_x);
    m17k_sw(slot + 0x28u, target_x);
    m17k_sw(WM_M17K_TARGET + 4u, target_y);
    m17k_sw(slot + 0x2Cu, target_y);
    m17k_sw(WM_M17K_TARGET + 8u, target_z);
    m17k_sw(slot + 0x30u, target_z);
}

static void m17k_consume_latch(u32 slot)
{
    u16 command = m17k_lhu(slot + 0x04u);

    switch (command) {
    case 1u:
        m17k_sh(slot + 0x04u, 0u);
        m17k_sh(slot + 0x20u, 1u);
        m17k_sw(slot + 0x50u, m17k_lw(slot + 0x38u));
        m17k_sw(slot + 0x54u, m17k_lw(slot + 0x3Cu));
        m17k_sw(slot + 0x58u, m17k_lw(slot + 0x40u));
        break;
    case 2u:
        m17k_sh(slot + 0x04u, 0u);
        m17k_sh(slot + 0x20u, 0u);
        m17k_set_camera_targets(slot, UINT32_C(0x003E0000), -32, 3440);
        break;
    case 3u:
        m17k_sh(slot + 0x04u, 0u);
        m17k_sh(slot + 0x20u, 2u);
        break;
    case 4u:
        m17k_sh(slot + 0x04u, 0u);
        m17k_sh(slot + 0x20u, 3u);
        m17k_sw(WM_M17K_TARGET + 0u, UINT32_C(0x07499000));
        m17k_sw(WM_M17K_TARGET + 4u, UINT32_C(0xFFE74000));
        m17k_sw(WM_M17K_TARGET + 8u, UINT32_C(0x0408A000));
        break;
    case 5u:
        m17k_sh(slot + 0x04u, 0u);
        m17k_sh(slot + 0x20u, 4u);
        break;
    case 6u:
        m17k_sh(slot + 0x20u, 5u);
        m17k_sw(slot + 0x7Cu, 4096u);
        m17k_sh(slot + 0x04u, 0u);
        m17k_set_camera_targets(slot, UINT32_C(0x00320000), 64, 3392);
        m17k_set_position_targets(slot, UINT32_C(0x01379000),
                                  UINT32_C(0xFFEE0000),
                                  UINT32_C(0x04B2A000));
        break;
    case 7u:
        m17k_sh(slot + 0x04u, 0u);
        m17k_sh(slot + 0x20u, 6u);
        break;
    case 8u:
#if defined(W34N113_MUTANT_LATCH8_WRONG_STATE)
        m17k_sh(slot + 0x20u, 9u);
#else
        m17k_sh(slot + 0x20u, 7u);
#endif
        m17k_sh(slot + 0x04u, 0u);
        m17k_set_camera_targets(slot, UINT32_C(0x00100000), -144, -1296);
        m17k_set_position_targets(slot, UINT32_C(0x07529000),
                                  UINT32_C(0xFFE74000),
                                  UINT32_C(0x02A3A000));
        break;
    case 9u:
        m17k_sh(slot + 0x04u, 0u);
        m17k_sh(slot + 0x20u, 8u);
        break;
    case 10u:
        m17k_sh(slot + 0x20u, 9u);
        m17k_sh(slot + 0x04u, 0u);
        m17k_set_camera_targets(slot, UINT32_C(0x00320000), -352, 3552);
        m17k_set_position_targets(slot, UINT32_C(0x02659000),
                                  UINT32_C(0xFFEF0000),
                                  UINT32_C(0x06D8A000));
        break;
    case 11u:
        m17k_sh(slot + 0x20u, 10u);
        m17k_sh(slot + 0x04u, 0u);
        m17k_set_camera_targets(slot, UINT32_C(0x00290000), -416, 352);
        m17k_set_position_targets(slot, UINT32_C(0x04BA9000),
                                  UINT32_C(0xFFD78000),
                                  UINT32_C(0x01DDA000));
        break;
    case 12u:
        m17k_sh(slot + 0x04u, 0u);
#if defined(W34N113_MUTANT_WRONG_LATCH12_STATE)
        m17k_sh(slot + 0x20u, 10u);
#else
        m17k_sh(slot + 0x20u, 11u);
#endif
        break;
    default:
        break;
    }
}

static void m17k_run_state(u32 slot)
{
    switch (m17k_lh(slot + 0x20u)) {
    case 1:
        m17k_sw(slot + 0x50u,
                 m17k_as_u32(wm_800771D8(
                     m17k_as_s32(m17k_lw(slot + 0x50u)),
                     INT32_C(-131072), INT32_C(23831))));
        m17k_sw(slot + 0x54u,
                 m17k_as_u32(wm_800771D8(
                     m17k_as_s32(m17k_lw(slot + 0x54u)),
                     INT32_C(0x00580000), INT32_C(0x00010000))));
        break;
    case 2:
        m17k_sw(slot + 0x50u,
                 m17k_as_u32(wm_800771D8(
                     m17k_as_s32(m17k_lw(slot + 0x50u)),
#if defined(W34N113_MUTANT_STATE2_WRONG_STEP_SIGN)
                     INT32_C(-2031616), INT32_C(65536))));
#else
                     INT32_C(-2031616), INT32_C(-65536))));
#endif
        m17k_sw(slot + 0x54u,
                 m17k_as_u32(wm_800771D8(
                     m17k_as_s32(m17k_lw(slot + 0x54u)),
                     INT32_C(0x01290000), INT32_C(0x00010000))));
        m17k_sw(slot + 0x5Cu,
                 m17k_as_u32(wm_800771D8(
                     m17k_as_s32(m17k_lw(slot + 0x5Cu)),
                     INT32_C(0x00490000), INT32_C(0x00010000))));
        break;
    case 3:
        m17k_sw(slot + 0x50u,
                 m17k_as_u32(wm_800771D8(
                     m17k_as_s32(m17k_lw(slot + 0x50u)),
                     INT32_C(-65536), INT32_C(65536))));
        m17k_sw(slot + 0x54u,
                 m17k_as_u32(wm_800771D8(
                     m17k_as_s32(m17k_lw(slot + 0x54u)),
                     INT32_C(0x01290000), INT32_C(0x00010000))));
        m17k_sw(slot + 0x5Cu,
                 m17k_as_u32(wm_800771D8(
                     m17k_as_s32(m17k_lw(slot + 0x5Cu)),
                     INT32_C(0x002B0000), INT32_C(-65536))));
        break;
    case 4: {
        u32 scale = m17k_lw(slot + 0x7Cu) + 512u;

        m17k_sw(slot + 0x7Cu, scale);
#if defined(W34N113_MUTANT_SCALE_CLAMPS_EARLY)
        if (m17k_as_s32(scale) >= INT32_C(0x8000)) {
#else
        if (m17k_as_s32(scale) > INT32_C(0x8000)) {
#endif
            m17k_sw(slot + 0x7Cu, UINT32_C(0x8000));
            m17k_sh(slot + 0x20u, 0u);
        }
        break;
    }
    case 6:
        m17k_sw(slot + 0x54u,
                 m17k_as_u32(wm_800771D8(
                     m17k_as_s32(m17k_lw(slot + 0x54u)),
                     INT32_C(0x00F40000), INT32_C(0x00004000))));
        break;
    case 8:
        m17k_sw(slot + 0x50u,
                 m17k_as_u32(wm_800771D8(
                     m17k_as_s32(m17k_lw(slot + 0x50u)),
                     INT32_C(0x00050000), INT32_C(0x00002000))));
        m17k_sw(slot + 0x54u,
                 m17k_as_u32(wm_800771D8(
                     m17k_as_s32(m17k_lw(slot + 0x54u)),
                     INT32_C(0x00010000), INT32_C(0x00008000))));
        break;
    case 11:
        m17k_sw(slot + 0x50u,
                 m17k_as_u32(wm_800771D8(
                     m17k_as_s32(m17k_lw(slot + 0x50u)),
                     INT32_C(-524288), INT32_C(0x00002400))));
        m17k_sw(slot + 0x54u,
                 m17k_as_u32(wm_800771D8(
                     m17k_as_s32(m17k_lw(slot + 0x54u)),
                     INT32_C(0x002C0000), INT32_C(0x00002C00))));
        m17k_sw(slot + 0x5Cu,
                 m17k_as_u32(wm_800771D8(
                     m17k_as_s32(m17k_lw(slot + 0x5Cu)),
                     INT32_C(0x00100000), INT32_C(-12800))));
        m17k_sw(WM_M17K_TARGET + 0u,
                 m17k_as_u32(wm_800771D8(
                     m17k_as_s32(m17k_lw(WM_M17K_TARGET + 0u)),
                     INT32_C(0x04CD9000), INT32_C(0x00002600))));
        m17k_sw(WM_M17K_TARGET + 4u,
                 m17k_as_u32(wm_800771D8(
                     m17k_as_s32(m17k_lw(WM_M17K_TARGET + 4u)),
                     INT32_C(-1425408), INT32_C(0x00002580))));
        m17k_sw(WM_M17K_TARGET + 8u,
                 m17k_as_u32(wm_800771D8(
                     m17k_as_s32(m17k_lw(WM_M17K_TARGET + 8u)),
                     INT32_C(0x01C8A000), INT32_C(-10752))));
#if !defined(W34N113_MUTANT_SKIP_STATE11_WRAP)
        wm_80093354(WM_M17K_TARGET);
#endif
        break;
    default:
        break;
    }
}

static void m17k_apply_jitter(u32 slot)
{
    u32 amplitude = m17k_lw(slot + 0x7Cu);
    s32 divisor = m17k_as_s32(m17k_sra(amplitude, 12u));
    s32 bias = m17k_as_s32(m17k_sra(amplitude, 13u));
    s32 jitter;
    u16 bits;

    if (divisor == 0)
        abort();
    jitter = (s32)rand() % divisor;
#if defined(W34N113_MUTANT_JITTER_NOT_CENTERED)
    (void)bias;
#else
    jitter -= bias;
#endif
    bits = (u16)jitter;
    m17k_sh(WM_M17K_SCRATCH + 0xA2u, bits);
    m17k_sh(WM_M17K_JITTER_X,
             (u16)(m17k_lhu(WM_M17K_JITTER_X) + bits));
    m17k_sh(WM_M17K_JITTER_Z,
             (u16)(m17k_lhu(WM_M17K_JITTER_Z) +
                   m17k_lhu(WM_M17K_SCRATCH + 0xA2u)));
}

s32 wm_800828DC(s32 slot_index)
{
    u32 slot = m17k_slot(slot_index);

    m17k_consume_latch(slot);
#if !defined(W34N113_MUTANT_SKIP_CAMERA_BUILD)
    if (m17k_lw(WM_M17K_CAMERA_GATE) == 0u)
        wm_80096F18(WM_M17K_CAMERA_INPUT, WM_M17K_POSITION,
                     m17k_as_s32(m17k_lw(WM_M17K_CAMERA_HEIGHT)),
                     WM_M17K_ANGLES);
#endif
    m17k_run_state(slot);
#if !defined(W34N113_MUTANT_SKIP_MOTION_HELPERS)
    wm_80076DA4(slot, WM_M17K_SCRATCH);
    wm_80076F54(slot, WM_M17K_SCRATCH);
    wm_80076FA8(slot, WM_M17K_SCRATCH);
#endif
    m17k_apply_jitter(slot);
    return 1;
}
