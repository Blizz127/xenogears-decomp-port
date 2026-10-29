#include "common.h"


#ifndef XENO_PC_PORT
extern u8* D_800C34B0;
extern u8* D_800C3E34;
extern u8 D_800C3E50;
extern u8 func_80099498(void);
extern void func_8009B684(u8 kind, u16 bits);
extern int rand(void);

/* func_80097964.s, byte-exact: try to inflict status `bits` of group `kind`
 * on the current target (D_800C3E34, D_800C3E50's 0x170-byte record).
 * Fails when the target's +0x15A flag has bit 7 or the roll
 * rand() % 100 exceeds `chance`.  The first switch checks resistances
 * and cancellations (group 7 clears an opposite +0x88 pair and stops;
 * groups 7 and 9 refuse a 0xF000/0xF00 status the target already has,
 * message 0x39); the second sets the bits (+0x7C, +0x80 or the
 * group's +0x7A word) before func_8009B684. */
s8 func_80097964(u8 chance, u8 kind, u16 bits) {
    u8* p = D_800C34B0;
    u8* flags = &(p + D_800C3E50 * 0x170)[0x15A];

    if (*flags & 0x80) {
        return 0;
    }
    if (chance < rand() % 100) {
        return 0;
    }
    switch (kind) {
    case 0: {
        u16 mask = bits & 0xFFFD;

        if (*(u16*)(D_800C3E34 + 0x7E) & mask) {
            return 0;
        }
        if (bits & 0x1000) {
            *(u16*)(D_800C3E34 + 0x84) &= 0x7FFF;
        }
        break;
    }
    case 2:
        if (bits & *(u16*)(D_800C3E34 + 0x82)) {
            return 0;
        }
        break;
    case 5:
        if ((*(u16*)(D_800C3E34 + 0x7C) | *(u16*)(D_800C3E34 + 0x7E)) & 1) {
            return 0;
        }
        if (bits & 0x8000) {
            *(u16*)(D_800C3E34 + 0x7C) &= 0xEFFF;
        }
        break;
    case 7:
        if ((*(u16*)(D_800C3E34 + 0x80) | *(u16*)(D_800C3E34 + 0x82)) & 1) {
            return 0;
        }
        if (bits & 0xA) {
            if (*(u16*)(D_800C3E34 + 0x88) & 5) {
                *(u16*)(D_800C3E34 + 0x88) &= 0xFFFA;
                return 1;
            }
            break;
        }
        if (bits & 5) {
            if (*(u16*)(D_800C3E34 + 0x88) & 0xA) {
                *(u16*)(D_800C3E34 + 0x88) &= 0xFFF5;
                return 1;
            }
            break;
        }
    case 9:
        if (bits & 0xF000) {
            if (*(u16*)(D_800C3E34 + 0x8E) & 0xF000) {
                D_800C34B0[0x5FC7] = 0x39;
                return 0;
            }
            *(u16*)(D_800C3E34 + 0x8C) &= 0xFFF;
        }
        if (bits & 0xF00) {
            if (*(u16*)(D_800C3E34 + 0x8E) & 0xF00) {
                D_800C34B0[0x5FC7] = 0x39;
                return 0;
            }
            *(u16*)(D_800C3E34 + 0x8C) &= 0xF0FF;
        }
        break;
    }
    switch (kind) {
    case 0:
        *(u16*)(D_800C3E34 + 0x7C) = (bits | *(u16*)(D_800C3E34 + 0x7C)) & 0xFFFD;
        if (bits & 2) {
            if (func_80099498()) {
                *(u16*)(D_800C3E34 + 0x7C) |= bits;
                D_800C34B0[0x5FC7] = 0x31;
                *(u16*)(D_800C3E34 + 0x7A) = 0xFFEF;
            } else {
                D_800C34B0[0x5FC7] = 0x32;
            }
        }
        break;
    case 2:
        *(u16*)(D_800C3E34 + 0x80) |= bits;
        if (bits & 0x800) {
            *(u16*)(D_800C3E34 + 0x7A) |= 0x20;
        }
        break;
    case 5:
    case 7:
    case 9:
        *(u16*)(D_800C3E34 + (kind << 1) + 0x7A) |= bits;
        break;
    }
    func_8009B684(kind, bits);
    return 1;
}
#endif


#ifndef XENO_PC_PORT
extern u8* D_800C34B0;
#endif


/* func_80097D08.s */
void func_80097D08(void) {
    s16 i;

    for (i = 0xB; i != -1; i--) {
        *(u8*)(D_800C34B0 + i + 0x5FA0) = 0xFF;
        *(u32*)(((u32)i << 2) + (s32)(unsigned int)D_800C34B0 + 0x5F6C) = 0;
    }
}
