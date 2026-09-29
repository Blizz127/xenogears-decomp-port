#include "common.h"
#include "field_timed_commands.h"
#include "field_effect_constructor.h"
#include <stdint.h>

extern u32 D_801E8670[10];
extern u16 D_801E8648[20];
extern void* D_801E8644;
extern s16 D_801E86B0, D_801E863C;
extern s32 func_801E0844(u8*,s32);
extern void func_801E8330(s32,s32,s32);
extern void func_801E8394(u8*,s32,s32,s32);
extern u32 func_801E34BC(s32);
extern void func_801E165C(u8*);

/* Retail [801E5D44,801E632C), SHA-256
 * 760f9f457085e59b63a6ae781297309641b8a4e429cc7ced348c1cda0f6c4c19.
 * No command budget, guessed stack value, or skipped unknown-op length. */
void PcPort_FieldTimedCommandsWithContext(u8* object, void* pool, s32 frame,
    u32 retailObjectAddress, u16 inheritedStack50)
{
    (void)pool;
    if (*(s16*)(object + 0x98) < 0) return;
    while (*(s16*)(object + 0x9c) < *(s16*)(object + 0x9e)) {
        u8* p = (u8*)(uintptr_t)*(u32*)(object + 0xa0);
        if (*(s16*)(object + 0x98) != *(s16*)p) break;
        u32 advance = 0;
        switch (p[2]) {
        case 1: advance = 0x14; break;
        case 2:
            if (p[4] != 0) {
                if (p[3] < 2) {
                    u8* records = (u8*)D_801E8648;
                    *(u16*)(records + p[3] * 20u + 0x10) = p[5] ? 0xffff : object[0x20];
                    u8* source = D_801E8644;
                    *(u16*)(records + p[3] * 20u + 0x12) = p[6];
                    *(u16*)(source + p[3] * 2u + 2) = (u16)(p[7] << 4);
                    *(u16*)(source + p[3] * 2u + 8) = (u16)(p[8] << 4);
                    *(u16*)(source + p[3] * 2u + 0xe) = (u16)(p[9] << 4);
                    *(u16*)(records + p[3] * 20u + 8) = *(u16*)(p + 0xa);
                    *(u16*)(records + p[3] * 20u + 0xa) = *(u16*)(p + 0xc);
                    *(u16*)(records + p[3] * 20u + 0xc) = *(u16*)(p + 0xe);
                    *(u16*)(records + p[3] * 20u + 6) = *(u16*)(p + 0x10);
                }
                advance = 0x12;
            } else {
                *(u16*)((u8*)D_801E8648 + p[3] * 20u + 6) = 0;
                advance = 6;
            }
            break;
        case 3: case 4:
            func_801E0844((u8*)(uintptr_t)(*(u32*)(object + 0x110) + p[3] * 112u), frame);
            advance = p[4] ? 0x1c : 6;
            break;
        case 5: advance = 8; break;
        case 6: advance = 4; break;
        case 7:
            *(u8*)(uintptr_t)(*(u32*)(object + 4) + p[4] * 124u + 7) = p[5] & 1;
            advance = 6;
            break;
        case 8: {
            s16 savedSlot = D_801E86B0;
            u16 savedMask = (u16)D_801E863C;
            u16 mask = (u16)(1u << ((u32)(s32)savedSlot & 31));
            for (u32 i = 0; i < 8; ++i) {
                if ((*(u16*)(object + 0x10a) >> i) & 1) {
                    if (D_801E8670[i] != 0 && p[5] > 0) {
                        if (inheritedStack50 != 0) func_801E8394(object, i, mask, p[5]);
                        else func_801E8330(i, mask, p[5]);
                    }
                }
            }
            D_801E86B0 = savedSlot;
            D_801E863C = (s16)savedMask;
            advance = 0xa;
            break;
        }
        case 9:
            if (p[4] != 0) {
                if (p[3] < object[0x10e]) {
                    u8* linked = NULL;
                    if (p[5] != 0xff && p[5] < object[0x10e])
                        linked = (u8*)(uintptr_t)(*(u32*)(object + 0x118) + p[5] * 48u);
                    u8* source = (p[6] & 0x7f) < 4 ? NULL : D_801E8644;
                    u32 callback = func_801E34BC(p[7]);
                    u16 ax = *(u16*)(p + 8), ay = *(u16*)(p + 0xa);
                    u16 bx = *(u16*)(p + 0xc), by = *(u16*)(p + 0xe), bz = *(u16*)(p + 0x10);
                    if (p[6] & 0x80) {
                        if (*(s16*)(object + 0x90) < 0) break;
                        u16 x = *(u16*)(object + 0x94), y = *(u16*)(object + 0x96);
                        ax = (u16)(ax + x); ay = (u16)(ay + y);
                        if ((p[0x12] >> 4) == 1) { bx = (u16)(bx + x); by = (u16)(by + y); }
                    }
                    PcPort_FieldEffectConstructWithS1(
                        (u8*)(uintptr_t)(*(u32*)(object + 0x118) + p[3] * 48u), linked,
                        p[6] & 0x7f, p[0x12] | 0x700, source,
                        (s16)ax, (s16)ay, 0, (s16)bx, (s16)by, (s16)bz,
                        (s16)ax, (s16)ay, p[0x13], p[0x14],
                        *(s16*)(p + 0x16), *(s16*)(p + 0x18), *(s16*)(p + 0x1a),
                        callback, retailObjectAddress);
                }
                advance = 0x1c;
            } else {
                func_801E165C((u8*)(uintptr_t)(*(u32*)(object + 0x118) + p[3] * 48u));
                advance = 6;
            }
            break;
        default: break;
        }
        if (advance != 0) *(u32*)(object + 0xa0) += advance;
        *(u16*)(object + 0x9c) = (u16)(*(u16*)(object + 0x9c) + 1);
    }
    *(u16*)(object + 0x98) = (u16)(*(u16*)(object + 0x98) + 1);
    if (*(s16*)(object + 0x9a) >= 0 && *(s16*)(object + 0x98) >= *(s16*)(object + 0x9a)) {
        *(u16*)(object + 0x98) = 0;
        *(u16*)(object + 0x9c) = 0;
        *(u32*)(object + 0xa0) = *(u32*)(object + 0xa4);
    }
}
