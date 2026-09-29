/* Retail mode-8/11 scheduler callbacks [0x8007756C,0x80077954). */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_7756c.h"
#include "world_map_helper_96f18.h"

extern void SetGeomScreen(int h);

#define WM_7756C_POOL_PTR 0x8009BE24u
#define WM_7756C_ANGLES   0x8009BD38u
#define WM_7756C_MATRIX   0x8009BD40u
#define WM_7756C_MATRIX_B 0x8009BD48u
#define WM_7756C_POSITION 0x8009BE28u
#define WM_7756C_CAMERA   0x8009C5ACu
#define WM_7756C_HEIGHT   0x8009D3F0u
#define WM_7756C_STATE    0x8009D144u
#define WM_7756C_GEOM_Y   0x8009BE0Cu
#define WM_7756C_HELD     0x8009CD4Cu
#define WM_7756C_RELEASED 0x8009BD10u
#define WM_7756C_D554     0x8009D554u
#define WM_7756C_D7CC     0x8009D7CCu
#define WM_7756C_GEOM_H   0x8009BCDCu
#define WM_7756C_SCRATCH  0x1F800000u

static u16 m811_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static s16 m811_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 m811_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void m811_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void m811_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u32 m811_add(u32 lhs, u32 rhs)
{
    return lhs + rhs;
}

static u32 m811_slot(s32 slot_index)
{
#if defined(W34N56_MUTANT_WRONG_SLOT_STRIDE)
    return m811_lw(WM_7756C_POOL_PTR) + ((u32)slot_index << 6);
#else
    return m811_lw(WM_7756C_POOL_PTR) + ((u32)slot_index << 7);
#endif
}

static void m811_swap_matrix_prefix(void)
{
    memcpy(PSX_ADDR(WM_7756C_SCRATCH + 0xA0u),
           PSX_ADDR(WM_7756C_MATRIX), 8u);
#if !defined(W34N56_MUTANT_SKIP_MATRIX_SWAP)
    memcpy(PSX_ADDR(WM_7756C_MATRIX),
           PSX_ADDR(WM_7756C_MATRIX_B), 8u);
    memcpy(PSX_ADDR(WM_7756C_MATRIX_B),
           PSX_ADDR(WM_7756C_SCRATCH + 0xA0u), 8u);
#endif
}

s32 wm_8007756C(s32 slot_index)
{
    u32 slot = m811_slot(slot_index);
    s32 pitch;

    m811_sh(WM_7756C_ANGLES + 0u, (u16)-128);
    m811_sh(WM_7756C_ANGLES + 2u, 0x0200u);
    m811_sh(WM_7756C_ANGLES + 4u, 0u);
    m811_sw(WM_7756C_HEIGHT, 0x00400000u);
    m811_sw(slot + 0x28u, 0xFFF80000u);
    m811_sw(WM_7756C_GEOM_Y, 120u);

    pitch = (s32)m811_lh(WM_7756C_ANGLES + 2u);
    m811_sw(slot + 0x2Cu, (u32)(pitch << 12));
    m811_sw(slot + 0x38u, m811_lw(slot + 0x28u));
    m811_sw(slot + 0x3Cu, m811_lw(slot + 0x2Cu));
    m811_sw(slot + 0x40u, m811_lw(slot + 0x30u));
    m811_sw(slot + 0x44u, m811_lw(slot + 0x34u));

    m811_sw(WM_7756C_STATE, 0u);
    m811_sw(WM_7756C_POSITION + 0u, m811_lw(WM_7756C_CAMERA + 0u));
    m811_sw(WM_7756C_POSITION + 4u, m811_lw(WM_7756C_CAMERA + 4u));
    m811_sw(WM_7756C_POSITION + 8u, m811_lw(WM_7756C_CAMERA + 8u));

    wm_80096F18(WM_7756C_MATRIX, WM_7756C_POSITION,
                (s32)m811_lw(WM_7756C_HEIGHT), WM_7756C_ANGLES);
    m811_swap_matrix_prefix();
    return 1;
}

s32 wm_800776E0(s32 slot_index)
{
    u32 slot = m811_slot(slot_index);
    u16 held;
    u32 target_x;
    u32 target_y;
    u32 smooth_x;
    u32 smooth_y;
#if !defined(W34N56_MUTANT_SKIP_SMOOTHING)
    s32 delta;
#endif

    if ((m811_lhu(WM_7756C_RELEASED) & 0x0040u) != 0u) {
        m811_sw(WM_7756C_D554, 0u);
        m811_sw(WM_7756C_D7CC, 0u);
    }

    held = m811_lhu(WM_7756C_HELD);
    target_x = m811_lw(slot + 0x28u);
    if ((held & 0x1000u) != 0u)
        target_x = m811_add(target_x, 0x00008000u);
    if ((held & 0x4000u) != 0u)
        target_x = m811_add(target_x, 0xFFFF8000u);

#if defined(W34N56_MUTANT_WRONG_X_LIMIT)
    if ((s32)target_x < -0x170000 || (s32)target_x > 0x18000)
#else
    if ((s32)target_x < -0x180000 || (s32)target_x > 0x18000)
#endif
        target_x = 0x00018000u;
    m811_sw(slot + 0x28u, target_x);

    target_y = m811_lw(slot + 0x2Cu);
    if ((held & 0x8004u) != 0u)
        target_y = m811_add(target_y, 0xFFFF0000u);
    if ((held & 0x2008u) != 0u)
        target_y = m811_add(target_y, 0x00010000u);
    m811_sw(slot + 0x2Cu, target_y);

    smooth_x = m811_lw(slot + 0x38u);
    smooth_y = m811_lw(slot + 0x3Cu);
#if !defined(W34N56_MUTANT_SKIP_SMOOTHING)
    delta = (s32)(target_x - smooth_x);
    smooth_x = m811_add(smooth_x, (u32)(delta >> 3));
    delta = (s32)(target_y - smooth_y);
    smooth_y = m811_add(smooth_y, (u32)(delta >> 3));
#endif
    m811_sw(slot + 0x38u, smooth_x);
    m811_sw(slot + 0x3Cu, smooth_y);

    m811_sh(WM_7756C_ANGLES + 0u, (u16)((s32)smooth_x >> 12));
    m811_sh(WM_7756C_ANGLES + 2u, (u16)((s32)smooth_y >> 12));
    wm_80096F18(WM_7756C_MATRIX, WM_7756C_POSITION,
                (s32)m811_lw(WM_7756C_HEIGHT), WM_7756C_ANGLES);
    m811_swap_matrix_prefix();

#if defined(W34N56_MUTANT_NO_HALF_TURN)
    m811_sh(WM_7756C_ANGLES + 2u, m811_lhu(WM_7756C_ANGLES + 2u));
#else
    m811_sh(WM_7756C_ANGLES + 2u,
            (u16)((m811_lhu(WM_7756C_ANGLES + 2u) + 0x0800u) & 0x0FFFu));
#endif
    SetGeomScreen((int)m811_lw(WM_7756C_GEOM_H));
    return 1;
}
