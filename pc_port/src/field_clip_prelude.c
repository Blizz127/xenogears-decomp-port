#include "common.h"
#include "psyq/libgte.h"
#include "field_clip_prelude.h"
extern void HeapChangeCurrentUser(u32,char**);
extern s32 func_801E6338(u8*);
extern void func_801E63A8(u8*);

/* Retail [801E39F0,801E3D34), prefix SHA-256
 * ca040a2fd3119e90db71fdd0b4a12c4d7af5b560da6766ce1735fcfafa28a364.
 * This phase neither fetches opcodes nor substitutes for missing E39F0. */
int PcPort_FieldClipPrelude(u8* object,s32 ticks,u32* stream)
{
    if (ticks == 0 || *(u32*)(object + 0x10) == 0) return 0;
    HeapChangeCurrentUser(4,NULL);
    for (s32 tick = 0; tick < ticks; ++tick) {
        for (u32 axis = 0; axis < 3; ++axis)
            *(u16*)(object + 0x70 + axis * 2) = (u16)(*(u16*)(object + 0x70 + axis * 2)
                + *(u16*)(object + 0x76 + axis * 2));
        for (u32 axis = 0; axis < 3; ++axis)
            *(u16*)(object + 0x7c + axis * 2) = (u16)(*(u16*)(object + 0x7c + axis * 2)
                + *(u16*)(object + 0x82 + axis * 2));
        for (u32 axis = 0; axis < 3; ++axis) {
            u8* root = (u8*)(uintptr_t)*(u32*)(object + 4);
            *(u16*)(root + 0x54 + axis * 2) = (u16)(*(u16*)(root + 0x54 + axis * 2)
                + (*(s16*)(object + 0x70 + axis * 2) >> 3));
        }
        SVECTOR local;
        s16* components[3] = {&local.vx,&local.vy,&local.vz};
        for (u32 axis = 0; axis < 3; ++axis) {
            u8* root = (u8*)(uintptr_t)*(u32*)(object + 4);
            s32 product = (s32)*(s16*)(object + 0x7c + axis * 2) * *(s16*)(root + 0x4c + axis * 2);
            *components[axis] = (s16)(product >> 12);
        }
        local.pad = 0; /* Native ABI padding; not a fourth vector component. */
        VECTOR transformed;
        u8* root = (u8*)(uintptr_t)*(u32*)(object + 4);
        ApplyMatrix((MATRIX*)(root + 0x2c),&local,&transformed);
        s32 values[3] = {transformed.vx,transformed.vy,transformed.vz};
        for (u32 axis = 0; axis < 3; ++axis) {
            u32 product = (u32)(s32)*(s16*)(object + 0x1c) * (u32)values[axis];
            root = (u8*)(uintptr_t)*(u32*)(object + 4);
            *(u32*)(root + 0x5c + axis * 4) += (u32)((s32)product >> 12);
        }
    }
    u32 selected = *(u32*)(object + 0x10);
    if (*(u32*)(object + 0x4c) != 0) {
        s32 distance = func_801E6338(object);
        if (*(s16*)(object + 0x48) >= distance) {
            selected = *(u32*)(object + 0x4c);
            *(u32*)(object + 0x4c) = 0;
            goto target;
        }
    }
    if (*(u32*)(object + 0x54) != 0) {
        u8* root = (u8*)(uintptr_t)*(u32*)(object + 4);
        s32 height = *(s16*)(object + 0x60);
        if (height < *(s32*)(root + 0x60)) {
            *(s32*)(root + 0x60) = height;
            selected = *(u32*)(object + 0x54);
            *(u32*)(object + 0x54) = 0;
            goto target;
        }
    }
    if (*(u32*)(object + 0x50) != 0) {
        u16 limit = *(u16*)(object + 0x46);
        *(u16*)(object + 0x44) = (u16)(*(u16*)(object + 0x44) + (u32)ticks);
        if (*(u16*)(object + 0x44) >= limit) {
            selected = *(u32*)(object + 0x50);
            *(u32*)(object + 0x50) = 0;
        }
    }
target:
    if (*(s16*)(object + 0x58) != 0) func_801E63A8(object);
    *stream = selected;
    return 1;
}
