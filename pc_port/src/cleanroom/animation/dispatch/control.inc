/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Blizz127 and contributors.
 */
/* Native sprite control. The actor stores packed little-endian PS1 words. */
int AnimationScriptOpcodeIsNoop(uint32_t opcode)
{
    uint8_t id = (uint8_t)opcode;
    if ((id >= 0x97 && id <= 0x9f) || (id >= 0xe1 && id <= 0xe4) ||
        (id >= 0xf8 && id <= 0xfb))
        return 1;

    switch (id) {
    case 0x8b: case 0x8e: case 0x8f: case 0x95:
    case 0xb1: case 0xb2: case 0xbe:
    case 0xc2: case 0xc3: case 0xc7: case 0xc8: case 0xca: case 0xcb:
    case 0xd4: case 0xe8: case 0xec: case 0xf0: case 0xf3: case 0xf4:
        return 1;
    default:
        return 0;
    }
}

/* The PS1 signed divide yields +1 for a negative dividend over zero,
 * or -1 for a nonnegative dividend over zero. Scale is always unsigned. */
static uint32_t spriteScaleDivide(uint32_t dividend, uint32_t scale)
{
    int32_t value = (int32_t)dividend;
    if (scale == 0)
        return value < 0 ? 1u : UINT32_MAX;
    return (uint32_t)(value / (int32_t)scale);
}

static uint32_t spriteTickCount(void)
{
    return AnimationRead32((const uint8_t *)&D_80059198) + 1u;
}

static int animationControl(uint8_t *actor, uint8_t opcode, uint8_t *args)
{
    switch (opcode) {
    case 0x8d: {
        uint8_t *record = AnimWordPtr(AnimationRead32(actor + 0x24));
        if (record)
            func_8002CC10((int16_t)AnimationRead16(record + 4),
                          (int16_t)AnimationRead16(record + 6));
        break;
    }
    case 0x91:
    case 0x92:
        actor[0x2b] = (uint8_t)((actor[0x2b] & 0xfeu) | (opcode == 0x92));
        func_8001F6B0(actor);
        break;
    case 0x94: {
        if ((AnimationRead32(actor + 0x3c) & 3u) != 2u)
            break;
        uint8_t *parent = AnimWordPtr(AnimationRead32(actor + 0x70));
        uint8_t *transform = AnimWordPtr(AnimationRead32(actor + 0x20));
        if (parent && transform) {
            AnimationWrite16(transform + 2, AnimationRead16(parent + 0x32));
            AnimationWrite32(actor + 0x3c,
                             AnimationRead32(actor + 0x3c) | (1u << 28));
        }
        break;
    }
    case 0xa3: {
        if (AnimationRead32(actor + 0xa8) & 1u) {
            uint8_t *attributes = AnimWordPtr(AnimationRead32(actor + 0x7c));
            if (attributes) {
                uint32_t preset = AnimationRead32(attributes + 4);
                if (preset) {
                    AnimationWrite32(actor + 0x1c, preset);
                    break;
                }
            }
        }
        uint32_t scale = (AnimationRead32(actor + 0xac) >> 7) & 0xfffu;
        uint32_t inverse = spriteScaleDivide(65536, scale);
        uint32_t inverseSquare = (uint32_t)((int32_t)(inverse * inverse) / 256);
        int32_t strength = (int8_t)args[0] * 64 * (int16_t)AnimationRead16(actor + 0x82);
        uint32_t step = (uint32_t)(strength / 4096) * 32u;
        uint32_t weighted = (uint32_t)((int32_t)(step * inverseSquare) / 256);
        uint32_t ticks = spriteTickCount();
        AnimationWrite32(actor + 0x1c, weighted * ticks * ticks);
        break;
    }
    case 0xa4: {
        uint8_t *linked = AnimWordPtr(AnimationRead32(actor + 0x74));
        func_800245D8(linked, (int8_t)args[0]);
        break;
    }
    case 0xa6: {
        if (AnimationRead32(actor + 0xa8) & 1u)
            break;
        uint32_t magnitude = (uint32_t)((int32_t)(int8_t)args[0] * 16);
        uint32_t product = magnitude * spriteTickCount() *
                           (uint32_t)(int32_t)(int16_t)AnimationRead16(actor + 0x82);
        uint32_t fixed = (uint32_t)((int32_t)product / 4096) << 16;
        uint32_t scale = (AnimationRead32(actor + 0xac) >> 7) & 0xfffu;
        AnimationWrite32(actor + 0x10,
                         AnimationRead32(actor + 0x10) + spriteScaleDivide(fixed, scale));
        break;
    }
    case 0xb3: {
        uint32_t bits = (uint32_t)(args[0] & 63u) << 11;
        uint32_t flags = AnimationRead32(actor + 0xa8);
        AnimationWrite32(actor + 0xa8, (flags & ~(63u << 11)) | bits);
        break;
    }
    default:
        return 0;
    }
    return 1;
}
