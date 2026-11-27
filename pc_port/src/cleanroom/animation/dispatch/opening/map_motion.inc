/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Blizz127 and contributors.
 * Sprite movement uses a wrapping 32-bit product before Q12 truncation.
 */
static int animationMapOpening(uint8_t *actor, uint8_t opcode, uint8_t *args)
{
    if (opcode != 0xa0 && opcode != 0xa1)
        return 0;

    uint32_t velocity = 0;
    if (opcode == 0xa1 && (AnimationRead32(actor + 0xa8) & 1u)) {
        uint8_t *motion = AnimWordPtr(AnimationRead32(actor + 0x7c));
        if (motion)
            velocity = AnimationRead32(motion);
    }
    if (!velocity) {
        uint32_t step = (uint32_t)((int8_t)args[0] * 16);
        uint32_t ticks = AnimationRead32((const uint8_t *)&D_80059198) + 1u;
        int16_t scale = (int16_t)AnimationRead16(actor + 0x82);
        int32_t product = (int32_t)(step * ticks * (uint32_t)(int32_t)scale);
        velocity = (uint32_t)(product / 4096) << 8;
    }
    if (opcode == 0xa0) {
        AnimationWrite32(actor + 0x18, velocity);
        func_80022974(actor);
    } else {
        uint32_t divisor = (AnimationRead32(actor + 0xac) >> 7) & 4095u;
        int32_t numerator = (int32_t)(velocity << 8);
        int32_t quotient = divisor ? numerator / (int32_t)divisor :
                           (numerator < 0 ? 1 : -1);
        AnimationWrite32(actor + 0x10, (uint32_t)quotient);
    }
    return 1;
}
