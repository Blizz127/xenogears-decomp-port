// Native sprite facing and script-position replay. The animation package,
// instruction lengths and bytecode are supplied at runtime.
#include <stdint.h>

static u16 spriteSeekRead16(const u8 *bytes)
{
    return (u16)(bytes[0] | ((u16)bytes[1] << 8));
}

static u32 spriteSeekRead32(const u8 *bytes)
{
    return spriteSeekRead16(bytes) | ((u32)spriteSeekRead16(bytes + 2) << 16);
}

static void spriteSeekWrite16(u8 *bytes, u16 value)
{
    bytes[0] = (u8)value;
    bytes[1] = (u8)(value >> 8);
}

static void spriteSeekWrite32(u8 *bytes, u32 value)
{
    spriteSeekWrite16(bytes, (u16)value);
    spriteSeekWrite16(bytes + 2, (u16)(value >> 16));
}

static void spriteSeekPutPointer(u8 *bytes, const void *address)
{
    spriteSeekWrite32(bytes, (u32)(uintptr_t)address);
}

static void spriteSeekSyncFlip(u8 *sprite)
{
    u32 pose = spriteSeekRead32(sprite + 0xac);
    u32 render = spriteSeekRead32(sprite + 0x3c);
    spriteSeekWrite32(sprite + 0x3c, (render & ~8u) | ((pose ^ (pose << 1)) & 8));
}

static void spriteSeekAdvanceClock(u8 *sprite, unsigned ticks)
{
    spriteSeekWrite16(sprite + 0x9e, (u16)(spriteSeekRead16(sprite + 0x9e) + ticks));
    u32 counter = spriteSeekRead32(sprite + 0xa8);
    if (((counter >> 22) & 63) != 63) {
        spriteSeekWrite32(sprite + 0xa8, counter + (1u << 22));
    }
}

void func_80022660(void *object, void *stop, s32 stopFrame)
{
    u8 *sprite = object;
    unsigned heldDelay = 0;
    for (;;) {
        u8 *pc = AnimWordPtr(spriteSeekRead32(sprite + 0x64));
        u32 state = spriteSeekRead32(sprite + 0xa8);
        if (pc == stop && (s32)((state >> 22) & 63) == stopFrame) {
            return;
        }
        unsigned opcode = *pc;
        if (opcode < 0x80) {
            spriteSeekPutPointer(sprite + 0x64, pc + 1);
            if (opcode < 0x40) {
                unsigned family = opcode >> 4;
                if (family == 0 || family == 2) {
                    int delta = family == 0 ? 1 : -1;
                    func_8001D2B0(sprite, (s16)(spriteSeekRead16(sprite + 0x34) + delta));
                } else if (family == 1) {
                    const u32 positionMask = 63u << 11;
                    u32 next = (state + (1u << 11)) & positionMask;
                    spriteSeekWrite32(sprite + 0xa8, (state & ~positionMask) | next);
                    func_80022D44(sprite);
                }
                heldDelay = 1 + (opcode & 15);
            }
            spriteSeekAdvanceClock(sprite, heldDelay);
            continue;
        }
        if (opcode <= 0x82 ||
            (pc == stop && (opcode == 0x86 || opcode == 0x87 || opcode == 0x97))) {
            return;
        }
        if (opcode == 0xb3) {
            u32 newPosition = (u32)(pc[1] & 63) << 11;
            spriteSeekWrite32(sprite + 0xa8, (state & ~(63u << 11)) | newPosition);
        } else if (opcode == 0xbe) {
            u16 entry = spriteSeekRead16(pc + 1);
            u32 pose = spriteSeekRead32(sprite + 0xac);
            spriteSeekWrite32(sprite + 0xac, (pose & ~8u) | ((entry >> 6) & 8));
            spriteSeekSyncFlip(sprite);
            s16 frame = (s16)(entry & 511);
            if (spriteSeekRead16(sprite + 0x34) != (u16)frame) {
                func_8001D2B0(sprite, frame);
            }
            unsigned duration = 1 + ((entry >> 11) & 15);
            spriteSeekWrite16(sprite + 0x9e, (u16)(spriteSeekRead16(sprite + 0x9e) + duration));
        } else if (opcode == 0xe2) {
            s16 displacement = (s16)spriteSeekRead16(pc + 1);
            AnimScriptStackPushU24(object, (s32)(uintptr_t)(pc + 3));
            spriteSeekPutPointer(sprite + 0x64, pc + displacement);
            continue;
        }
        spriteSeekPutPointer(sprite + 0x64, pc + D_8004FC40[opcode]);
    }
}

void func_800223B0(void *object, s16 angle)
{
    u8 *sprite = object;
    u32 facing = spriteSeekRead32(sprite + 0xa8);
    u32 pose = spriteSeekRead32(sprite + 0xac);
    unsigned direction = (facing >> 17) & 7;
    unsigned previous = direction;
    unsigned mirror = (u16)angle & 1;
    spriteSeekWrite16(sprite + 0x80, (u16)angle);
    spriteSeekWrite32(sprite + 0xac, (pose & ~4u) | (mirror << 2));
    if (!spriteSeekRead32(sprite + 0x48)) {
        return;
    }
    unsigned mode = (facing >> 20) & 3;
    u8 *animation = AnimWordPtr(spriteSeekRead32(sprite + 0x58));
    if (mode < 3) {
        unsigned entryIndex;
        unsigned circle = (u16)angle & 4095;
        if (mode == 0) {
            direction = entryIndex = 0;
            mirror = ((circle + 1024) & 4095) > 2048;
            spriteSeekPutPointer(sprite + 0x5c, animation + 6);
        } else if (mode == 1) {
            direction = ((circle + 1536) >> 10) & 3;
            mirror = direction == 3;
            entryIndex = mirror ? 1 : direction;
        } else {
            direction = ((circle + 1280) >> 9) & 7;
            mirror = direction > 4;
            if (mirror) {
                direction = 8 - direction;
            }
            entryIndex = direction;
        }
        spriteSeekWrite32(sprite + 0xac, (pose & ~4u) | (mirror << 2));
        u8 *entry = animation + 4 + entryIndex * 2;
        spriteSeekPutPointer(sprite + 0x54, entry + spriteSeekRead16(entry));
        facing = (facing & ~(7u << 17)) | (direction << 17);
        spriteSeekWrite32(sprite + 0xa8, facing);
    }

    if (previous != direction) {
        void *oldPC = AnimWordPtr(spriteSeekRead32(sprite + 0x64));
        u16 wait = spriteSeekRead16(sprite + 0x9e);
        unsigned oldFrame = (facing >> 22) & 63;
        u32 resetBits = (63u << 11) | (63u << 22);
        spriteSeekWrite32(sprite + 0xa8, (facing & ~resetBits) | (63u << 11));
        u8 *entry = animation + 2;
        spriteSeekPutPointer(sprite + 0x64, entry + spriteSeekRead16(entry));
        func_80022660(sprite, oldPC, (s32)oldFrame);
        spriteSeekWrite16(sprite + 0x9e, wait);
    }
    spriteSeekSyncFlip(sprite);
}
