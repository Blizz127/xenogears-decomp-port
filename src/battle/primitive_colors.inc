/* Retail battle 800B2AEC..800B3348 - native port implementation.
 *
 * This is a behaviour-preserving reimplementation, not a matching
 * transcription (retail is 2140 bytes, this body compiles to 844).  The
 * matching build therefore assembles the retail body from splat instead
 * (INCLUDE_ASM in src/battle/main.c); this file is used by the port only, via
 * pc_port/src/game_overrides.c.  It becomes the matching source again only
 * once it is transcribed retail-shaped and proven byte-exact.
 *
 * Packet offsets are PSX bytes, independent of host GPU structure layouts. */
#ifdef XENO_PC_PORT
#include "psx_memory.h"
#endif

extern s32 func_80021AD8(s32 color, s32 value);

static u32 BattlePrimitiveColorRead32(const u8* p) {
    return (u32)p[0] | ((u32)p[1] << 8) |
           ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

/* Animation F2 can pass a native model alongside guest packet addresses
 * written by the battle MIPS code. Translate each argument independently;
 * native pointers and packet layout/colour arithmetic remain unchanged. */
static u8* BattlePrimitiveColorAddress(void* pointer) {
#ifdef XENO_PC_PORT
    uintptr_t address = (uintptr_t)pointer;
    if ((address & ~(uintptr_t)0x1FFFFFu) == 0x80000000u ||
        (address & ~(uintptr_t)0x1FFFFFu) == 0xA0000000u)
        return PSX_ADDR(address);
#endif
    return pointer;
}

void func_800B2AEC(void* model, void* buffer0, void* buffer1,
                   s32 red, s32 green, s32 blue) {
    u8* header = BattlePrimitiveColorAddress(model);
    u8* out0 = BattlePrimitiveColorAddress(buffer0);
    u8* out1 = BattlePrimitiveColorAddress(buffer1);
    u32 descriptorOffset = BattlePrimitiveColorRead32(header + 0x10);
    u32 count = BattlePrimitiveColorRead32(header + 0x14);
    u8* descriptor = (u8*)((uintptr_t)header + descriptorOffset);
    u32 index;
    s32 deltas[3];
    deltas[0] = red;
    deltas[1] = green;
    deltas[2] = blue;

    for (index = 0; index != count; ++index) {
        u32 key = (descriptor[3] & 0x1Cu) |
                  (((descriptor[2] ^ 1u) & 1u) << 8);
        u32 sourceOffset = 0;
        u32 outputStride = 0;
        u32 triples = 0;
        u32 constantColor = 0;
        u32 vertex;
        u32 channel;
        u32 firstRed = 0;
        u32 packetBytes;
        u32 descriptorBytes;

        switch (key) {
        case 0x000: case 0x008:
            sourceOffset = 0x04; triples = 1; break;
        case 0x010:
            sourceOffset = 0x04; outputStride = 8; triples = 3; break;
        case 0x018:
            sourceOffset = 0x04; outputStride = 8; triples = 4; break;
        case 0x004:
            sourceOffset = 0x10; triples = 1; break;
        case 0x00C:
            sourceOffset = 0x14; triples = 1; break;
        case 0x014:
            sourceOffset = 0x10; outputStride = 12; triples = 3; break;
        case 0x01C:
            sourceOffset = 0x14; outputStride = 12; triples = 4; break;
        case 0x104: case 0x10C:
            constantColor = 1; triples = 1; break;
        case 0x114:
            constantColor = 1; outputStride = 12; triples = 3; break;
        case 0x11C:
            constantColor = 1; outputStride = 12; triples = 4; break;
        }

        /* Each source is read only after the preceding output byte store.
         * Finish all of buffer0 before copying any color into buffer1. */
        for (vertex = 0; vertex != triples; ++vertex) {
            for (channel = 0; channel != 3; ++channel) {
                s32 color = constantColor ? 0x80 :
                    descriptor[sourceOffset + vertex * 4 + channel];
                s32 adjusted = func_80021AD8(color, deltas[channel]);
                if (vertex + 1 == triples && channel == 2) {
                    /* Retail snapshots this byte before the final blue store. */
                    firstRed = out0[4];
                }
                out0[4 + vertex * outputStride + channel] = (u8)adjusted;
            }
        }
        if (triples != 0) {
            out1[4] = (u8)firstRed;
            for (vertex = 0; vertex != triples; ++vertex) {
                for (channel = (vertex == 0 ? 1 : 0); channel != 3; ++channel) {
                    u32 offset = 4 + vertex * outputStride + channel;
                    out1[offset] = out0[offset];
                }
            }
        }

        /* Stores may alias the descriptor, so these are deliberately late. */
        packetBytes = ((u32)descriptor[0] + 1u) * 4u;
        descriptorBytes = ((u32)descriptor[1] + 1u) * 4u;
        out0 = (u8*)((uintptr_t)out0 + packetBytes);
        out1 = (u8*)((uintptr_t)out1 + packetBytes);
        descriptor = (u8*)((uintptr_t)descriptor + descriptorBytes);
    }
}
