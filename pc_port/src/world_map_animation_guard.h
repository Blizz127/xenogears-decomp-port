#ifndef PC_PORT_WORLD_MAP_ANIMATION_GUARD_H
#define PC_PORT_WORLD_MAP_ANIMATION_GUARD_H

#include <stdint.h>

#include "common.h"
#include "psx_memory.h"

/* Scheduler slot +0x4C stores the native sprite pointer returned by the
 * resident sprite allocator. Retail tests animation state with
 * lb 0xAF(pointer); it does not reinterpret the pointer as guest RAM. */
static inline s8 wm_native_animation_value(u32 sprite_bits)
{
#if defined(WM_ANIMATION_GUARD_MUTANT_GUEST_REMAP)
    return *(s8*)PSX_ADDR(sprite_bits + 0xAFu);
#else
    const s8* sprite = (const s8*)(uintptr_t)sprite_bits;

    return sprite[0xAF];
#endif
}

static inline int wm_native_animation_differs(u32 sprite_bits, s8 target)
{
    return wm_native_animation_value(sprite_bits) != target;
}

#endif
