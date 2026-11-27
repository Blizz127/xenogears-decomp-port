/* SPDX-License-Identifier: MIT */
#include <stdlib.h>

/* The command operands and either sprite record may refer to the same bytes. */
static void animationAssignColor(uint8_t *actor, const uint8_t *operand)
{
    uint8_t red = operand[0];
    uint8_t *model = (uint8_t *)(uintptr_t)AnimationRead32(actor + 0x20);
    actor[0x28] = red;
    actor[0x29] = operand[1];
    unsigned mode = AnimationRead32(actor + 0x3c) & 3u;
    actor[0x2a] = operand[2];
    if (mode == 2) {
        for (unsigned channel = 0; channel < 3; ++channel)
            AnimationWrite16(model + 0x38 + 2 * channel, operand[channel]);
    }
    if ((AnimationRead32(actor + 0x3c) & 3u) == 1)
        func_8001F6B0(actor);
}

static void animationAdjustColor(uint8_t *actor, const uint8_t *operand)
{
    uint8_t red = actor[0x28];
    int redChange = (int8_t)operand[0];
    uint8_t *firstModel = (uint8_t *)(uintptr_t)AnimationRead32(actor + 0x20);
    int adjusted = func_80021AD8(red, redChange);
    uint8_t green = actor[0x29];
    actor[0x28] = (uint8_t)adjusted;
    adjusted = func_80021AD8(green, (int8_t)operand[1]);
    uint8_t blue = actor[0x2a];
    actor[0x29] = (uint8_t)adjusted;
    adjusted = func_80021AD8(blue, (int8_t)operand[2]);
    unsigned mode = AnimationRead32(actor + 0x3c) & 3u;
    actor[0x2a] = (uint8_t)adjusted;

    if (mode == 2) {
        for (unsigned channel = 0; channel < 3; ++channel) {
            int delta = (int8_t)operand[channel];
            uint8_t *component = firstModel + 0x38 + 2 * channel;
            AnimationWrite16(component, (uint16_t)(AnimationRead16(component) + delta));
        }
    }
    if ((AnimationRead32(actor + 0x3c) & 3u) == 1)
        func_8001F6B0(actor);

    uint32_t flags = AnimationRead32(actor + 0x40);
    if (((flags >> 13) & 15u) != 15)
        return;

    uint8_t *currentModel = (uint8_t *)(uintptr_t)AnimationRead32(actor + 0x20);
    uint32_t modelAddress = AnimationRead32(currentModel + 0x34);
    if (!modelAddress || (flags & 2u))
        return;

    func_800B2AEC((void *)(uintptr_t)modelAddress,
                  (void *)(uintptr_t)AnimationRead32(currentModel + 0x2c),
                  (void *)(uintptr_t)AnimationRead32(currentModel + 0x30),
                  (int16_t)AnimationRead16(firstModel + 0x38),
                  (int16_t)AnimationRead16(firstModel + 0x3a),
                  (int16_t)AnimationRead16(firstModel + 0x3c));
}

static int animationEvents(uint8_t *actor, uint8_t opcode, uint8_t *args)
{
    switch (opcode) {
    case 0xe5: {
        uint8_t *slot = func_8001FBA4((SpriteData *)actor, args);
        unsigned draw = (unsigned)rand() & 255u;
        *slot = (uint8_t)((draw * args[1]) >> 8);
        break;
    }
    case 0xe6: {
        uint8_t *slot = func_8001FBA4((SpriteData *)actor, args);
        AnimationWrite16(slot, args[1]);
        break;
    }
    case 0xe7: {
        uint16_t change = (uint16_t)(2u * AnimationRead16(args));
        uint16_t scale = AnimationRead16(actor + 0x2c);
        SpriteSetScale((SpriteData *)actor, (int16_t)(scale + change));
        break;
    }
    case 0xf1:
        animationAssignColor(actor, args);
        break;
    case 0xf2:
        animationAdjustColor(actor, args);
        break;
    default:
        return 0;
    }
    return 1;
}
