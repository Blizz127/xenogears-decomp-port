// SPDX-License-Identifier: MIT
// Independently authored from the native ports' observed animation checkpoint ABI.
#include <stdint.h>
#include <string.h>

static uint16_t animCheckpointHalf(const unsigned char *address)
{
    uint16_t result;
    memcpy(&result, address, sizeof(result));
    return result;
}

static uint32_t animCheckpointWord(const unsigned char *address)
{
    uint32_t result;
    memcpy(&result, address, sizeof(result));
    return result;
}

static void animCheckpointPutHalf(unsigned char *address, uint16_t value)
{
    memcpy(address, &value, sizeof(value));
}

static void animCheckpointPutWord(unsigned char *address, uint32_t value)
{
    memcpy(address, &value, sizeof(value));
}

static unsigned char *animCheckpointChild(unsigned char *actor, size_t at)
{
    return AnimWordPtr(animCheckpointWord(actor + at));
}

void func_80021D50(void *actor_data, void *checkpoint_data)
{
    unsigned char *actor = actor_data;
    const unsigned char *checkpoint = checkpoint_data;
    const int32_t previous_mode = D_80059198;
    int16_t wanted_frame;

    D_80059198 = 0;
    animCheckpointPutHalf(actor + 0x80, animCheckpointHalf(checkpoint + 0x10));
    actor[0xaf] = checkpoint[0x14];
    actor[0xb0] = checkpoint[0x16];
    for (size_t i = 0; i < 3; ++i) {
        unsigned char *pose = animCheckpointChild(actor, 0x20);
        animCheckpointPutHalf(pose + 6 + 2 * i, animCheckpointHalf(checkpoint + 0x24 + 2 * i));
    }
    animCheckpointPutHalf(actor + 0x82, animCheckpointHalf(checkpoint + 0x2c));
    animCheckpointPutHalf(actor + 0x2c, animCheckpointHalf(checkpoint + 0x2a));

    func_800245D8(actor, (int8_t)actor[0xaf]);
    wanted_frame = (int16_t)animCheckpointHalf(checkpoint + 0x18);
    while ((int)((animCheckpointWord(actor + 0xa8) >> 22) & 63) != wanted_frame) {
        AnimScriptTick(actor);
        for (size_t i = 0; i < 3; ++i) {
            animCheckpointPutWord(actor + 4 * i, animCheckpointWord(actor + 4 * i)
                                  + animCheckpointWord(actor + 0x0c + 4 * i));
        }
        animCheckpointPutWord(actor + 0x10, animCheckpointWord(actor + 0x10)
                              + animCheckpointWord(actor + 0x1c));
    }

    for (size_t i = 0; i < 3; ++i)
        animCheckpointPutWord(actor + 4 * i, animCheckpointWord(checkpoint + 4 * i));
    for (size_t i = 0; i < 2; ++i) {
        unsigned char *track = animCheckpointChild(actor, 0x7c);
        animCheckpointPutWord(track + 4 * i, animCheckpointWord(checkpoint + 0x1c + 4 * i));
    }
    D_80059198 = previous_mode;
}

void func_80021EBC(void *actor_data, void *checkpoint_data)
{
    unsigned char *actor = actor_data;
    unsigned char *checkpoint = checkpoint_data;
    const unsigned char *track = animCheckpointChild(actor, 0x7c);
    const unsigned char *pose = animCheckpointChild(actor, 0x20);
    const uint32_t flags = animCheckpointWord(actor + 0xa8);

    for (size_t i = 0; i < 3; ++i)
        animCheckpointPutWord(checkpoint + 4 * i, animCheckpointWord(actor + 4 * i));
    animCheckpointPutHalf(checkpoint + 0x10, animCheckpointHalf(actor + 0x80));
    animCheckpointPutHalf(checkpoint + 0x12, (flags >> 11) & 63);
    animCheckpointPutHalf(checkpoint + 0x14, (uint16_t)(int8_t)actor[0xaf]);
    animCheckpointPutHalf(checkpoint + 0x16, (uint16_t)(int8_t)actor[0xb0]);
    animCheckpointPutHalf(checkpoint + 0x18, (flags >> 22) & 63);
    for (size_t i = 0; i < 2; ++i)
        animCheckpointPutWord(checkpoint + 0x1c + 4 * i, animCheckpointWord(track + 4 * i));
    for (size_t i = 0; i < 3; ++i)
        animCheckpointPutHalf(checkpoint + 0x24 + 2 * i, animCheckpointHalf(pose + 6 + 2 * i));
    animCheckpointPutHalf(checkpoint + 0x2c, animCheckpointHalf(actor + 0x82));
    animCheckpointPutHalf(checkpoint + 0x2a, animCheckpointHalf(actor + 0x2c));
}

