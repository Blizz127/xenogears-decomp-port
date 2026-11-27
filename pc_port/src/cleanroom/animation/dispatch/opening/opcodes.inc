/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Blizz127 and contributors
 * Opening-field sprite operations. Offsets describe the compact on-disc records.
 */
#include <string.h>

static uint8_t *openingRelative24(uint8_t *origin)
{
    int32_t distance = (int32_t)AnimationRead16(origin)
                     + (int32_t)(int8_t)origin[2] * 65536;
    return origin + distance;
}

static uint8_t *openingModel(uint8_t *actor)
{
    return AnimWordPtr(AnimationRead32(actor + 0x20));
}

static void openingReplaceModel(uint8_t *actor, uint8_t opcode, uint8_t *args)
{
    HeapChangeCurrentUser(5, NULL);
    uint8_t *resource = openingRelative24(args);
    if (opcode == 0xF5) {
        func_8002C59C(resource);
    } else {
        func_8002C3E8(resource);
        resource += 16;
    }

    uint8_t *model = openingModel(actor);
    uint32_t previous = AnimationRead32(model + 0x2C);
    if (previous != 0)
        HeapFree(AnimWordPtr(previous));

    model = openingModel(actor);
    func_8002CB54(resource, (uint32_t *)(void *)(model + 0x2C),
                  (uint32_t *)(void *)(model + 0x30));
    model = openingModel(actor);
    func_8002C8CC(resource, AnimWordPtr(AnimationRead32(model + 0x2C)), 0);

    model = openingModel(actor);
    uint8_t *source = AnimWordPtr(AnimationRead32(model + 0x2C));
    uint8_t *destination = AnimWordPtr(AnimationRead32(model + 0x30));
    uint32_t size = AnimationRead32(resource + 0x34);
    if (size != 0)
        memcpy(destination, source, size);
    model = openingModel(actor);
    AnimationWrite32(model + 0x34, (uint32_t)(uintptr_t)resource);
}

/* Move the sprite along a randomly oriented radius. Its packed positions
 * use 16.16 coordinates; the geometry unit consumes their integer parts. */
static void openingRandomizeSpritePosition(uint8_t *actor, const uint8_t *operand)
{
    unsigned draw = (unsigned)rand() & 255u;
    /* The operand can refer to the live RNG seed. Read it after the draw. */
    int radius = (int)((draw * operand[0]) / 256u);
    int scale = (int16_t)AnimationRead16(actor + 0x2c);
    SVECTOR offset = {0};
    offset.vx = (int16_t)func_80022CAC(actor, radius * scale / 4096);

    SVECTOR angles = {0};
    angles.vx = (int16_t)rand();
    angles.vy = (int16_t)rand();
    MATRIX matrix = {0};
    AnimationC1RotMatrix(&angles, &matrix);
    for (unsigned axis = 0; axis < 3; ++axis)
        matrix.t[axis] = (int16_t)AnimationRead16(actor + axis * 4 + 2);
    SetRotMatrix(&matrix);
    SetTransMatrix(&matrix);

    SVECTOR position;
    long flags;
    RotTransSV(&offset, &position, &flags);
    AnimationWrite32(actor, (uint32_t)(uint16_t)position.vx << 16);
    AnimationWrite32(actor + 4, (uint32_t)(uint16_t)position.vy << 16);
    AnimationWrite32(actor + 8, (uint32_t)(uint16_t)position.vz << 16);
}

static int animationOpening(uint8_t *actor, uint8_t opcode, uint8_t *args)
{
    switch (opcode) {
    case 0x96:
        func_8001CE74((WorkListEntry *)(void *)AnimWordPtr(AnimationRead32(actor + 0x6C)));
        return 1;
    case 0xC1:
        openingRandomizeSpritePosition(actor, args);
        return 1;
    case 0xC6:
        if ((AnimationRead32(actor + 0xA8) & 1u) != 0) {
            uint8_t *parameters = AnimWordPtr(AnimationRead32(actor + 0x7C));
            AnimationWrite16(parameters + 12, args[0]);
        }
        return 1;
    case 0xE0: {
        int16_t distance = (int16_t)AnimationRead16(args);
        func_80023B84(actor, args + distance, AnimPackagePtr(AnimationRead32(actor + 0x24)));
        return 1;
    }
    case 0xF5:
    case 0xF6:
        openingReplaceModel(actor, opcode, args);
        return 1;
    case 0xFC: {
        void *scratch = HeapAlloc(8192, 0);
        uint8_t *coordinates = AnimWordPtr(AnimationRead32(actor + 0x24));
        D_800592E4 = (uint32_t)(uintptr_t)openingRelative24(args);
        D_800592E8 = (int16_t)AnimationRead16(coordinates + 4);
        D_800592EA = (int16_t)AnimationRead16(coordinates + 6);
        func_8001FB30();
        HeapFree(scratch);
        return 1;
    }
    default:
        return 0;
    }
}
