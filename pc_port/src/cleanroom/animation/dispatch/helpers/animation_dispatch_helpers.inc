/* SPDX-License-Identifier: MIT
 * Host sprite helpers independently written for the Xenogears native port.
 * Memory layouts are the engine's 32-bit little-endian record format.
 */
#include <stdint.h>

extern u8 g_PsxRam[];
extern u8 *PcPort_BattleSharedData(void *, u32, unsigned) __attribute__((weak));

static u16 AnimationRead16(const u8 *source)
{
    return (u16)((u16)source[0] | (u16)source[1] << 8);
}

static u32 AnimationRead32(const u8 *source)
{
    return (u32)AnimationRead16(source) | (u32)AnimationRead16(source + 2) << 16;
}

static void AnimationWrite16(u8 *dest, u16 value)
{
    dest[0] = (u8)value;
    dest[1] = (u8)(value >> 8);
}

static void AnimationWrite32(u8 *dest, u32 value)
{
    AnimationWrite16(dest, (u16)value);
    AnimationWrite16(dest + 2, (u16)(value >> 16));
}

/* Native packages may store a compact host pointer. KSEG0 addresses in the
 * 8 MiB mirror window use the physical 2 MiB backing store. */
static u8 *AnimWordPtr(u32 word)
{
    if (word >= 0x80000000u && word < 0x80800000u)
        return &g_PsxRam[word % 0x200000u];
    return (u8 *)(uintptr_t)word;
}

static u8 *AnimPackagePtr(u32 word)
{
    if (PcPort_BattleSharedData) {
        u8 *package = PcPort_BattleSharedData(0, word, 20u);
        if (package)
            return package;
    }
    return AnimWordPtr(word);
}

static u8 *AnimationSpriteAddress(u32 address)
{
    u32 region = address >> 21;
    if (region == 0u || region == 0x400u || region == 0x500u)
        return &g_PsxRam[address % 0x200000u];
    return (u8 *)(uintptr_t)address;
}

static u8 *AnimationPlayerSprite(void)
{
    return AnimationSpriteAddress(AnimationRead32(g_PsxRam + 0xc3e1c));
}

static u32 AnimationSoundBankSelector(void)
{
    return AnimationRead32(g_PsxRam + 0x5919c);
}

static u8 *AnimationSoundBankPointer(u32 address)
{
    return AnimationSpriteAddress(address);
}

static s16 AnimationTargetMean(u32 sum, s32 count)
{
    if (count == 0)
        return -1;
    /* Use a wide quotient so every signed input, including INT_MIN, is
     * representable. The animation position is its high signed halfword. */
    int64_t quotient = (int64_t)(s32)sum / count;
    return (s16)((u32)quotient >> 16);
}

static const char *Xeno0xBCAnchorVec(const u8 *reference, u32 index, SVECTOR *result)
{
    u32 transform = AnimationRead32(reference + 0x20);
    if (!transform)
        return "Sprite transform is missing";
    if ((reference[0x3c] & 3u) != 1u)
        return "Sprite model has no anchor coordinates";

    u32 anchors = AnimationRead32(AnimationSpriteAddress(transform) + 0x34);
    s32 x = 0;
    s32 y = 0;
    if (anchors) {
        const u8 *entry = AnimationSpriteAddress(anchors) + index * 8u;
        x = (int8_t)entry[0];
        y = (int8_t)entry[1];
        if (reference[0xac] & 4u)
            x = -x;
    }
    s32 scale = (s16)AnimationRead16(reference + 0x2c);
    result->vx = (s16)(AnimationRead16(reference + 2) + x * scale / 4096);
    result->vy = (s16)(AnimationRead16(reference + 6) + y * scale / 4096);
    result->vz = (s16)AnimationRead16(reference + 10);
    return 0;
}

/* The conventional renderer sine and cosine are exposed under opposite
 * decompiler names by the game's public trig shim. At this call site the
 * third Euler angle is fixed at zero. The signs here preserve the device's
 * floor rounding at each individual Q12 multiply. */
static void AnimationC1RotMatrix(const SVECTOR *angles, MATRIX *matrix)
{
    s32 sin_x = xeno_retail_rcos((u16)angles->vx & 4095u);
    s32 cos_x = xeno_retail_rsin((u16)angles->vx & 4095u);
    s32 sin_y = xeno_retail_rcos((u16)angles->vy & 4095u);
    s32 cos_y = xeno_retail_rsin((u16)angles->vy & 4095u);

    matrix->m[0][0] = (s16)cos_y;
    matrix->m[0][1] = 0;
    matrix->m[0][2] = (s16)sin_y;
    matrix->m[1][0] = (s16)-((-sin_x * sin_y) >> 12);
    matrix->m[1][1] = (s16)cos_x;
    matrix->m[1][2] = (s16)((-sin_x * cos_y) >> 12);
    matrix->m[2][0] = (s16)((-cos_x * sin_y) >> 12);
    matrix->m[2][1] = (s16)sin_x;
    matrix->m[2][2] = (s16)((cos_x * cos_y) >> 12);
}

static void AnimationC4ApplyMatrixLV(MATRIX *matrix, const u8 *input, VECTOR *output)
{
    s32 low[3], high[3];
    u32 partial[3];
    SetRotMatrix(matrix);
    for (unsigned axis = 0; axis < 3; ++axis) {
        s32 value = (s32)AnimationRead32(input + 4 * axis);
        low[axis] = value % 32768;
        high[axis] = value == INT32_MIN ? 65536 : value / 32768;
    }

    /* GTE MVMVA in unshifted and Q12 modes. MAC1..3 combine into a
     * full-width result while the final command leaves the live GTE state. */
    for (unsigned axis = 0; axis < 3; ++axis)
        MTC2((u32)high[axis], 9 + axis);
    doCOP2(0x41e012);
    for (unsigned axis = 0; axis < 3; ++axis)
        partial[axis] = MFC2(25 + axis) << 3;
    for (unsigned axis = 0; axis < 3; ++axis)
        MTC2((u32)low[axis], 9 + axis);
    doCOP2(0x49e012);
    output->vx = (s32)(partial[0] + MFC2(25));
    output->vy = (s32)(partial[1] + MFC2(26));
    output->vz = (s32)(partial[2] + MFC2(27));
}
