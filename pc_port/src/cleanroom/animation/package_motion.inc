/* SPDX-License-Identifier: MIT
 * Host animation-package setup and polar motion, reconstructed from observed
 * package writes, service calls and the two native ABIs. Guest pointers occupy
 * four bytes in a sprite package even on a 64-bit host.
 */
#include <stdint.h>

static u32 animationPackageCoordinates(SVEC2 pair)
{
    return (u32)(u16)pair.x | ((u32)(u16)pair.y << 16);
}

void func_80022224(SpriteAnimPackage *package, void *file, SVEC2 texture, SVEC2 palette, u32 unused)
{
    u8 *slots = (u8 *)package;
    const u8 *header = file;
    u32 base = (u32)(uintptr_t)file;

    (void)unused;
    AnimationWrite32(slots + 4, animationPackageCoordinates(texture));
    AnimationWrite32(slots + 8, animationPackageCoordinates(palette));
    AnimationWrite32(slots + 12, base + AnimationRead32(header + 12));
    AnimationWrite32(slots, base + AnimationRead32(header + 8));
    D_800591B0 = 0;
    u32 animations = base + AnimationRead32(header + 4);
    AnimationWrite32(slots + 16, animations);

    if (D_800591AD) {
        unsigned count = (AnimationRead16(AnimWordPtr(animations)) >> 6) & 63;
        if (count)
            D_800591B3 = (u8)count;
    }
}

void func_80022974(void *sprite)
{
    u8 *state = sprite;
    u32 divisor = (AnimationRead32(state + 0xac) >> 7) & 0xfff;
    s32 input = (s32)AnimationRead32(state + 0x18);
    s32 numerator = (s32)((u32)(input >> 4) << 8);
    s32 radius = divisor ? numerator / (s32)divisor : (numerator < 0 ? 1 : -1);
    int angle = AnimationRead16(state + 0x32) & 0xfff;

    /* Quantize each trigonometric coefficient before the wrapping product. */
    u32 deltaX = (u32)radius * (u32)(rsin(angle) >> 2);
    angle = AnimationRead16(state + 0x32) & 0xfff;
    AnimationWrite32(state + 0x0c, (u32)((s32)deltaX >> 6));
    u32 deltaZ = 0u - (u32)radius * (u32)(rcos(angle) >> 2);
    AnimationWrite32(state + 0x14, (u32)((s32)deltaZ >> 6));
}

s32 func_80022A00(s32 *word)
{
    return *word;
}
