/*
 * World-map scheduler callback 0x8008BB40.
 *
 * Register-faithful, finite-width transcription of retail world_map.bin
 * [0x8008BB40,0x8008BD1C).  This initializes the scheduler slot associated
 * with resource channel 2, unless that channel is suppressed by 0xFF, then
 * samples terrain and applies the world-mode setup family.
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_8bb40.h"
#include "world_map_terrain_sampler.h"

#define WM_U8(a)  (*(u8*)PSX_ADDR(a))
#define WM_U16(a) (*(u16*)PSX_ADDR(a))
#define WM_U32(a) (*(u32*)PSX_ADDR(a))

#define WM_POOL_PTR              0x8009BE24u
#define WM_RESOURCE_CONTROL      0x8006F36Au
#define WM_PACKAGE_PTR           0x8009CD3Cu
#define WM_MODE                  0x8009BE10u
#define WM_SEED_X                0x8006EE54u
#define WM_SEED_Z                0x8006EE56u
#define WM_SEED_STATE            0x8006EE58u
#define WM_RUNTIME_X             0x8009C5ACu
#define WM_RUNTIME_Z             0x8009C5B4u
#define WM_STATE_VALUE           0x8009C584u
#define WM_MODE_FLAG             0x8006F8E7u

#define WM_SLOT_OFF_CONTROL      0x20u
#define WM_SLOT_OFF_FLAG         0x24u
#define WM_SLOT_OFF_X            0x28u
#define WM_SLOT_OFF_Y            0x2Cu
#define WM_SLOT_OFF_Z            0x30u
#define WM_SLOT_OFF_CLEAR38      0x38u
#define WM_SLOT_OFF_CLEAR3C      0x3Cu
#define WM_SLOT_OFF_CLEAR40      0x40u
#define WM_SLOT_OFF_STATE        0x48u
#define WM_SLOT_OFF_CONST        0x4Au
#define WM_SLOT_OFF_OBJECT       0x4Cu
#define WM_SLOT_OFF_CONST58      0x58u
#define WM_SLOT_OFF_SIGNED_STATE 0x5Cu

#define WM_OBJECT_FLAGS          0x3Cu

extern void* func_80024524(void* package, s16 tex_x, s16 tex_y,
                           s16 clut_x, s16 clut_y, s16 arg5);
extern void func_800245D8(void* object, s16 animation);
extern void SpriteSetScale(void* object, short scale);

static s32 wm_bits_to_s32(u32 bits)
{
    s32 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static s32 wm_sign_extend_u16(u16 bits)
{
    if (bits <= 0x7FFFu)
        return (s32)bits;
    return (s32)(u32)bits - 0x10000;
}

/* [0x8009CD3C] is populated by world initialization as a KSEG guest address.
 * It is not a packed native SpriteData pointer. */
static void* wm_package_to_native(u32 package_bits)
{
    if (package_bits == 0u)
        return NULL;
    return PSX_ADDR(package_bits);
}

/* SpriteData helpers use raw low native pointers in their 32-bit fields.
 * The canonical port is linked -no-pie to make the conversion lossless.
 * Abort rather than silently truncate if that invariant is ever broken. */
static u32 wm_native_pointer_bits(void* pointer)
{
    uintptr_t bits = (uintptr_t)pointer;
    if (bits > (uintptr_t)UINT32_MAX)
        abort();
    return (u32)bits;
}

static void* wm_native_pointer_from_bits(u32 bits)
{
    return (void*)(uintptr_t)bits;
}

s32 wm_8008BB40(s32 slot_index)
{
    u32 offset_bits = (u32)slot_index << 7;
    u32 slot_addr = WM_U32(WM_POOL_PTR) + offset_bits;
    u8 resource_control = WM_U8(WM_RESOURCE_CONTROL);
    s32 return_state = 1;
    u32 x_bits;
    u32 z_bits;
    u32 terrain_y_bits;
    u32 mode_bits;
    u32 mode_index;
    u16 state_bits;

    if (resource_control != 0xFFu) {
        u32 package_bits = WM_U32(WM_PACKAGE_PTR);
        void* object;
        u32 object_bits;

        object = func_80024524(wm_package_to_native(package_bits),
                               (s16)0x120, (s16)0x1E0, (s16)0x160,
                               (s16)0x100, (s16)0x40);
        object_bits = wm_native_pointer_bits(object);
        WM_U32(slot_addr + WM_SLOT_OFF_OBJECT) = object_bits;

        object = wm_native_pointer_from_bits(object_bits);
        func_800245D8(object, (s16)0);
        object = wm_native_pointer_from_bits(
            WM_U32(slot_addr + WM_SLOT_OFF_OBJECT));
        SpriteSetScale(object, (short)0x1800);
        object = wm_native_pointer_from_bits(
            WM_U32(slot_addr + WM_SLOT_OFF_OBJECT));
        *(u32*)((u8*)object + WM_OBJECT_FLAGS) &= 0xFFFFFFFBu;

        WM_U16(slot_addr + WM_SLOT_OFF_FLAG) = 0u;
    } else {
        WM_U16(slot_addr + WM_SLOT_OFF_FLAG) = 1u;
        return_state = 3;
    }

    x_bits = (u32)WM_U16(WM_SEED_X) << 12;
    WM_U32(slot_addr + WM_SLOT_OFF_X) = x_bits;
    z_bits = (u32)WM_U16(WM_SEED_Z);
    x_bits = WM_U32(slot_addr + WM_SLOT_OFF_X);
    z_bits <<= 12;
    WM_U32(slot_addr + WM_SLOT_OFF_Z) = z_bits;
    WM_U32(slot_addr + WM_SLOT_OFF_Y) =
        (u32)wm_80093978(wm_bits_to_s32(x_bits), wm_bits_to_s32(z_bits));

    WM_U32(slot_addr + WM_SLOT_OFF_CLEAR40) = 0u;
    WM_U32(slot_addr + WM_SLOT_OFF_CLEAR3C) = 0u;
    WM_U32(slot_addr + WM_SLOT_OFF_CLEAR38) = 0u;

    state_bits = WM_U16(WM_SEED_STATE);
    mode_bits = WM_U32(WM_MODE);
    WM_U16(slot_addr + WM_SLOT_OFF_CONST) = 8u;
    WM_U32(slot_addr + WM_SLOT_OFF_CONST58) = 30u;
    mode_index = mode_bits - 1u;
    WM_U16(slot_addr + WM_SLOT_OFF_STATE) = state_bits;
    state_bits = WM_U16(slot_addr + WM_SLOT_OFF_STATE);
    WM_U32(slot_addr + WM_SLOT_OFF_SIGNED_STATE) =
        (u32)wm_sign_extend_u16(state_bits);

    if (mode_index < 13u) {
        if (mode_index <= 2u) {
            if (WM_U8(WM_MODE_FLAG) == 0u) {
                x_bits = WM_U32(WM_RUNTIME_X);
                WM_U32(slot_addr + WM_SLOT_OFF_X) = x_bits;
                z_bits = WM_U32(WM_RUNTIME_Z);
                x_bits = WM_U32(slot_addr + WM_SLOT_OFF_X);
                WM_U32(slot_addr + WM_SLOT_OFF_Z) = z_bits;
                terrain_y_bits =
                    (u32)wm_80093978(wm_bits_to_s32(x_bits),
                                     wm_bits_to_s32(z_bits));

                state_bits = (u16)WM_U32(WM_STATE_VALUE);
                WM_U32(slot_addr + WM_SLOT_OFF_Y) = terrain_y_bits;
                WM_U16(slot_addr + WM_SLOT_OFF_STATE) = state_bits;
                /* Retail does not recompute slot+0x5C after this reload. */
            } else {
                WM_U16(slot_addr + WM_SLOT_OFF_CONTROL) = 1u;
                WM_U16(slot_addr + WM_SLOT_OFF_FLAG) = 1u;
            }
        } else if (mode_index <= 6u) {
            WM_U16(slot_addr + WM_SLOT_OFF_CONTROL) =
                mode_index == 6u ? 2u : 3u;
            WM_U16(slot_addr + WM_SLOT_OFF_FLAG) = 1u;
            if (WM_U8(WM_RESOURCE_CONTROL) != 0xFFu)
                WM_U8(WM_MODE_FLAG) = 1u;
        }
    }

    return return_state;
}
