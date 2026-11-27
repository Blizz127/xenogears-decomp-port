/* Sprite records use little-endian 32-bit addresses and fixed-point coordinates. */
#include <stdint.h>

static inline void animStoreField32(void *record, unsigned int offset, u32 value)
{
    u8 *field = (u8 *)record + offset;
    field[0] = (u8)value;
    field[1] = (u8)(value >> 8);
    field[2] = (u8)(value >> 16);
    field[3] = (u8)(value >> 24);
}

void SpriteSetSpecialAnimFile(SpriteData *sprite, void *file)
{
    animStoreField32(sprite, 0x4c, (u32)(uintptr_t)file);
}

void func_80021BF8(void *sprite, s32 value)
{
    animStoreField32(sprite, 0x68, (u32)value);
}

void func_80021C00(void *sprite, u32 value)
{
    u8 *flags = (u8 *)sprite + 0x41;
    *flags = (u8)((*flags & 0xe0u) | (value & 0x1fu));
}

void func_80021D3C(void *sprite, s32 x, s32 z)
{
    animStoreField32(sprite, 8, (u32)z << 16);
    animStoreField32(sprite, 0, (u32)x << 16);
}

void func_80021FB8(u8 *sprite, u8 value)
{
    sprite[0xb0] = value;
}

void func_80021FC0(void *sprite, s32 value)
{
    animStoreField32(sprite, 0x18, (u32)value);
    func_80022974(sprite);
}

void func_80021FE0(void *sprite, s16 value)
{
    u8 *angle = (u8 *)sprite + 0x32;
    unsigned int bits = (unsigned short)value;
    angle[0] = (u8)bits;
    angle[1] = (u8)(bits >> 8);
    func_80022974(sprite);
}
