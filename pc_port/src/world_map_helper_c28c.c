/*
 * World-map SpriteData allocation helper 0x8008C28C.
 *
 * Register-faithful, finite-width transcription of retail world_map.bin
 * [0x8008C28C, 0x8008C364).  This leaf selects four independent signed
 * halfword parameters and one guest package pointer by channel, publishes the
 * allocated native object in a 32-bit slot field, initializes its animation
 * and scale, then clears object flag bit 2.
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_c28c.h"

#define WM_C28C_TABLE_TEX_X   0x8009B18Cu
#define WM_C28C_TABLE_TEX_Y   0x8009B194u
#define WM_C28C_TABLE_CLUT_X  0x8009B19Cu
#define WM_C28C_TABLE_CLUT_Y  0x8009B1A4u
#define WM_C28C_PACKAGE_TABLE 0x8009BDF8u
#define WM_C28C_FLAG_BASE     0x8006F8E5u
#define WM_C28C_SLOT_OBJECT   0x4Cu
#define WM_C28C_OBJECT_FLAGS  0x3Cu

extern void* func_80024524(void* package, s16 tex_x, s16 tex_y,
                           s16 clut_x, s16 clut_y, s16 arg5);
extern void func_800245D8(void* object, s16 animation);
extern void SpriteSetScale(void* object, short scale);

static s16 wm_c28c_load_s16(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}
static u32 wm_c28c_load_u32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void wm_c28c_store_u32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

/* BDF8 entries contain guest/KSEG package bits, not native object handles. */
static void* wm_c28c_package_to_native(u32 package_bits)
{
    if (package_bits == 0u)
        return NULL;
    return PSX_ADDR(package_bits);
}

/* SpriteData fields use the established raw-low-native-u32 convention.
 * Abort rather than silently truncate if the native representation does not
 * fit.  NULL is representable and deliberately receives no recovery path. */
static u32 wm_c28c_native_pointer_bits(void* pointer)
{
    uintptr_t bits = (uintptr_t)pointer;
    if (bits > (uintptr_t)UINT32_MAX)
        abort();
    return (u32)bits;
}

static void* wm_c28c_native_pointer_from_bits(u32 bits)
{
    return (void*)(uintptr_t)bits;
}

void wm_8008C28C(u32 slot_addr, s32 channel)
{
    u32 channel_bits = (u32)channel;
    u32 halfword_offset = channel_bits << 1;
    s16 tex_x = wm_c28c_load_s16(WM_C28C_TABLE_TEX_X + halfword_offset);
    s16 tex_y = wm_c28c_load_s16(WM_C28C_TABLE_TEX_Y + halfword_offset);
    s16 clut_x = wm_c28c_load_s16(WM_C28C_TABLE_CLUT_X + halfword_offset);
    s16 clut_y = wm_c28c_load_s16(WM_C28C_TABLE_CLUT_Y + halfword_offset);
    u32 package_offset = channel_bits << 2;
    u32 package_bits =
        wm_c28c_load_u32(WM_C28C_PACKAGE_TABLE + package_offset);
    void* object = func_80024524(
        wm_c28c_package_to_native(package_bits), tex_x, tex_y, clut_x, clut_y,
        (s16)0x40);
    u32 object_bits = wm_c28c_native_pointer_bits(object);
    s16 animation;
    u32 flags;

    wm_c28c_store_u32(slot_addr + WM_C28C_SLOT_OBJECT, object_bits);

    animation = *(u8*)PSX_ADDR(WM_C28C_FLAG_BASE + channel_bits) == 1u
                    ? (s16)0
                    : (s16)3;
    func_800245D8(object, animation);

    object_bits = wm_c28c_load_u32(slot_addr + WM_C28C_SLOT_OBJECT);
    object = wm_c28c_native_pointer_from_bits(object_bits);
    SpriteSetScale(object, (short)0x2000);

    object_bits = wm_c28c_load_u32(slot_addr + WM_C28C_SLOT_OBJECT);
    object = wm_c28c_native_pointer_from_bits(object_bits);
    memcpy(&flags, (u8*)object + WM_C28C_OBJECT_FLAGS, sizeof(flags));
    flags &= 0xFFFFFFFBu;
    memcpy((u8*)object + WM_C28C_OBJECT_FLAGS, &flags, sizeof(flags));
}
