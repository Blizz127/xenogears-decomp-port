/* Exact native transcription of retail mode-18 private initializers. */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_838e8.h"

#define WM_M18I_POOL_PTR       UINT32_C(0x8009BE24)
#define WM_M18I_STATIC_RECORD  UINT32_C(0x8009AC60)
#define WM_M18I_RESET_POSITION UINT32_C(0x8009C5AC)
#define WM_M18I_POSITION       UINT32_C(0x8009BE28)
#define WM_M18I_TARGET         UINT32_C(0x8009D55C)
#define WM_M18I_ANGLES         UINT32_C(0x8009BD38)
#define WM_M18I_CAMERA_GATE    UINT32_C(0x8009D144)
#define WM_M18I_CAMERA_HEIGHT  UINT32_C(0x8009D3F0)
#define WM_M18I_VIEW_HEIGHT    UINT32_C(0x8009BE0C)
#define WM_M18I_OBJECT         UINT32_C(0x8009C620)

static u32 m18i_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void m18i_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void m18i_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u32 m18i_slot(s32 slot_index)
{
    return m18i_lw(WM_M18I_POOL_PTR) + ((u32)slot_index << 7u);
}

s32 wm_800838E8(s32 slot_index)
{
    u32 slot = m18i_slot(slot_index);

#if defined(W34N115_MUTANT_WRONG_STATIC_RECORD)
    m18i_sw(slot + 0x50u, UINT32_C(0x8009AC64));
#else
    m18i_sw(slot + 0x50u, WM_M18I_STATIC_RECORD);
#endif
    return 1;
}

s32 wm_8008390C(s32 slot_index)
{
    u32 slot = m18i_slot(slot_index);
    u32 x = m18i_lw(WM_M18I_RESET_POSITION + 0u);
    u32 y = m18i_lw(WM_M18I_RESET_POSITION + 4u);
    u32 z = m18i_lw(WM_M18I_RESET_POSITION + 8u);

#if defined(W34N115_MUTANT_WRONG_SCALE)
    m18i_sw(slot + 0x7Cu, UINT32_C(0x0800));
#else
    m18i_sw(slot + 0x7Cu, UINT32_C(0x1000));
#endif
#if !defined(W34N115_MUTANT_SKIP_POSITION_FANOUT)
    m18i_sw(WM_M18I_POSITION + 0u, x);
    m18i_sw(WM_M18I_POSITION + 4u, y);
    m18i_sw(WM_M18I_POSITION + 8u, z);
    m18i_sw(WM_M18I_TARGET + 0u, x);
    m18i_sw(WM_M18I_TARGET + 4u, y);
    m18i_sw(WM_M18I_TARGET + 8u, z);
    m18i_sw(slot + 0x28u, x);
    m18i_sw(slot + 0x2Cu, y);
    m18i_sw(slot + 0x30u, z);
#else
    (void)x;
    (void)y;
    (void)z;
#endif
    m18i_sh(slot + 0x20u, 0u);
    m18i_sh(slot + 0x04u, 0u);
    m18i_sw(slot + 0x5Cu, UINT32_C(0x00960000));
#if defined(W34N115_MUTANT_WRONG_ANGLES)
    m18i_sh(WM_M18I_ANGLES + 0u, 0u);
#else
    m18i_sh(WM_M18I_ANGLES + 0u, UINT16_C(0xFFE0));
#endif
    m18i_sh(WM_M18I_ANGLES + 2u, UINT16_C(0x0400));
    m18i_sh(WM_M18I_ANGLES + 4u, 0u);
    m18i_sw(slot + 0x50u, UINT32_C(0xFFFE0000));
    m18i_sw(slot + 0x38u, UINT32_C(0xFFFE0000));
    m18i_sw(slot + 0x54u, UINT32_C(0x00400000));
    m18i_sw(slot + 0x3Cu, UINT32_C(0x00400000));
    m18i_sw(slot + 0x58u, 0u);
    m18i_sw(slot + 0x40u, 0u);
    m18i_sw(WM_M18I_CAMERA_GATE, 0u);
#if defined(W34N115_MUTANT_WRONG_CAMERA_HEIGHT)
    m18i_sw(WM_M18I_CAMERA_HEIGHT, UINT32_C(0x00500000));
#else
    m18i_sw(WM_M18I_CAMERA_HEIGHT, UINT32_C(0x00960000));
#endif
    m18i_sw(WM_M18I_VIEW_HEIGHT, 120u);
    return 1;
}

s32 wm_80083FE4(s32 slot_index)
{
    u32 slot = m18i_slot(slot_index);
    u32 object = m18i_lw(WM_M18I_OBJECT);
    u32 x = UINT32_C(0x01800000);
    u32 y = UINT32_C(0x00080000);
    u32 z = UINT32_C(0x01A00000);

    m18i_sw(slot + 0x28u, x);
    m18i_sw(slot + 0x2Cu, y);
#if defined(W34N115_MUTANT_WRONG_OBJECT_Z)
    m18i_sw(slot + 0x30u, UINT32_C(0x01900000));
#else
    m18i_sw(slot + 0x30u, z);
#endif
    m18i_sh(slot + 0x20u, 0u);
#if !defined(W34N115_MUTANT_SKIP_OBJECT_ENABLE)
    m18i_sh(object + 0x2A0u, 1u);
    m18i_sh(object + 0x24Cu, 1u);
#endif
    m18i_sw(object + 0x2A8u, x >> 12u);
    m18i_sw(object + 0x254u, x >> 12u);
    m18i_sw(object + 0x2ACu, y >> 12u);
    m18i_sw(object + 0x258u, y >> 12u);
#if defined(W34N115_MUTANT_SKIP_OBJECT_TRANSLATION)
    m18i_sw(object + 0x2B0u, 0u);
    m18i_sw(object + 0x25Cu, 0u);
#else
    m18i_sw(object + 0x2B0u, z >> 12u);
    m18i_sw(object + 0x25Cu, z >> 12u);
#endif
    return 1;
}
