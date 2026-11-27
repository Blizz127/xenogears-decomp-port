/* SPDX-License-Identifier: MIT
 * Native sprite record operations. Written from observations of host object
 * interfaces and synthetic byte-record experiments. */
void func_80021B04(void *destination, s16 x, s16 y, s16 z)
{
    u8 *out = destination;
    __builtin_memcpy(out, &x, sizeof x);
    __builtin_memcpy(out + sizeof x, &y, sizeof y);
    __builtin_memcpy(out + sizeof x + sizeof y, &z, sizeof z);
}

void func_80021B14(void *destination, s32 x, s32 y, s32 z)
{
    u8 *out = destination;
    __builtin_memcpy(out, &x, sizeof x);
    __builtin_memcpy(out + sizeof x, &y, sizeof y);
    __builtin_memcpy(out + sizeof x + sizeof y, &z, sizeof z);
}

void func_80021B24(void *destination, void *source)
{
    u8 *out = destination;
    const u8 *in = source;
    for (unsigned i = 0; i < 3; ++i) {
        s16 coordinate;
        __builtin_memcpy(&coordinate, in + i * sizeof coordinate, sizeof coordinate);
        __builtin_memcpy(out + i * sizeof coordinate, &coordinate, sizeof coordinate);
    }
}

void func_80021B48(void *destination, void *source)
{
    u8 *out = destination;
    const u8 *in = source;
    for (unsigned i = 0; i < 3; ++i) {
        s32 coordinate;
        __builtin_memcpy(&coordinate, in + i * sizeof coordinate, sizeof coordinate);
        __builtin_memcpy(out + i * sizeof coordinate, &coordinate, sizeof coordinate);
    }
}

void func_80021B6C(SpriteData *sprite)
{
    u8 *record = (u8 *)sprite;
    record[0x2b] |= 1u;
    func_8001F6B0(sprite);
}

void SpriteSetColor(SpriteData *sprite, u8 red, u8 green, u8 blue)
{
    u8 *record = (u8 *)sprite;
    record[0x28] = red;
    record[0x29] = green;
    record[0x2a] = blue;
    record[0x2b] &= (u8)~1u;
    func_8001F6B0(sprite);
}

void func_80021BCC(void *sprite, u32 value)
{
    u8 *field = (u8 *)sprite + 0xac;
    u32 flags;
    const unsigned shift = 7;
    const u32 mask = 0xfffu << shift;
    __builtin_memcpy(&flags, field, sizeof flags);
    flags = (flags & ~mask) | ((value << shift) & mask);
    __builtin_memcpy(field, &flags, sizeof flags);
}
