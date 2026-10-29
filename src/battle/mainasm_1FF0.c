/* battle mainasm_1FF0: the retail run at file offset 0x1FF0, main2's head
 * (func_80071AE0, func_80071B94, func_80072270, func_80072324), moved verbatim
 * out of main2 when func_800723E0 (mainasm_28F0, gcc 2.7.2) was split off
 * between them.  This head stays on gcc 2.6.3 like main2's tail
 * (func_80071AE0 and func_80071B94 differ under 2.7.2).  The whole file is
 * retail-build only. */
#include "common.h"
#ifndef XENO_PC_PORT
void AddPrim(s32, void *);
extern void *D_800C3E24;
extern s32 D_800CCB04;
extern void * D_800D2D28;
void func_80034888(s32, s32, s32);
extern s32 D_800CCB34;
extern s32 D_800D2DAC;
extern void *D_800D3278;
void func_80074D4C(void);
void func_80074F70(void);
#endif

#ifndef XENO_PC_PORT
#include "system/memory.h"
#endif


#ifndef XENO_PC_PORT
void func_800728B8(void *prims, s32 count, s32 start);
extern void *D_800D2D28;
extern void *D_800D2DB4;
void func_80074AB8(void);
void func_80074EEC(void);
#endif

#ifndef XENO_PC_PORT
extern u8 D_800D2CAF;
extern u16 D_800D2C94;
extern u16 D_800C48E8;
extern u8* D_800C3EAC;
extern u8 D_800D3014;
extern void func_80079E18(u8 index);
extern void func_80079E4C(u8 index);
extern void func_800716D8(void);

/* func_80071AE0.s: when D_800D2CAF is set and D_800C48E8 masks nothing in
 * D_800D2C94 (func_80071964's test): func_80079E18(7), clear byte 0x2DB
 * of D_800C3EAC, run the battle frame (func_800716D8) up to 59 times while
 * D_800D3014 stays 8, set byte 0x2DB back to 1, func_80079E4C(7) and run
 * one more frame.  First draft. */
void func_80071AE0(void) {
    s32 n;

    if (D_800D2CAF != 0 && (D_800D2C94 & D_800C48E8) == 0) {
        func_80079E18(7);
        n = 0x3B;
        D_800C3EAC[0x2DB] = 0;
        do {
            func_800716D8();
            if (D_800D3014 != 8) {
                break;
            }
        } while (--n != 0);
        D_800C3EAC[0x2DB] = 1;
        func_80079E4C(7);
        func_800716D8();
    }
}
extern u8 D_800C3E8C;
extern u8 D_800D3298;
extern u8 D_800C4922;
extern u8 D_800D2CA4[];
extern u16 D_800CCD64__rows[][184] asm("D_800CCD64");
extern u16 D_800CCD68__rows[][184] asm("D_800CCD68");
extern u16 D_800CCD1C__rows[][184] asm("D_800CCD1C");
extern u8 D_800C3EB4__r[][28] asm("D_800C3EB4");
extern u8 D_800C3E3D[];
extern s32 D_800C3DDC;
extern u8 D_800D3718[];
extern u8 D_800C3D1A[];
extern u16 D_800C3254[];
extern u8 D_800D3280;
extern s32 D_800CCB04__a_s32[] asm("D_800CCB04");
extern u8* D_800C3EA4__b asm("D_800C3EA4");
extern u16 D_8005941C;
extern u8 D_800D36C0;
extern u8 D_800C48EA[];
extern void func_80085350(void);
extern void func_80071A08(void);
extern u16 func_80099890(u8 actor);
extern void func_800799C8(u8 actor, s32 flag);
extern u8* GetStringEntry(s32 table, s32 id);
extern s32 SystemRenderStringEntry(u8* str, u8* buf, s32 w, s32 flags);
extern void func_800769E8(u8* rect, u8* buf);
extern void func_80079778(u8 actor);
extern void func_80071A8C(void);
extern void func_80071964(void);
extern void func_80070EB0(s32 arg);
extern void func_80080160(u8 actor);
extern void func_80080C94(u8 actor);
extern void func_800BCD98(s32 arg);
extern u16 func_80089C9C(s32 mask, u8 idx);
extern void func_80079C24(void);
extern void func_80071A38(void);
void func_80072270(void);
extern u8 func_800841E0(u8 idx);
extern u8 func_80085310(u8 idx, u8 v);
extern s32 func_80080AE4(u8 actor);
extern void func_800BA4E0(s32 arg);
void func_8007252C(void);
extern void func_80085B58(u8 actor);
extern void func_800BFE48(void);
extern void func_800718BC(void);

/* func_80071B94.s, byte-exact.
 * CONSTRUCTS (counted separately):
 * - The x1 (+0x63D4) / x3 (+0x63DC) cursor sums assign their D_800C3254
 *   operand to write-only u16 temps `t` / `u` inside the expression, so the
 *   operand is already a REG while actor*0x60 arrives as a narrowed SUBREG.
 *   expand_binop's commutative swap then gives retail's base-first
 *   `addu v0,v0,v1`.  One shared temp gives 1748.
 * - Inherited: the asm-alias row views, `(u8)replay`, the reused
 *   `k = D_800D3280 * 3`, a separate clear-loop counter `m`, and
 *   `(D_800C3EAC + (i << 6))[0x3C]`. */
void func_80071B94(s32 replay) {
    s32 i;
    s32 k;
    s32 m;
    u16 t;
    u16 u;

    D_800C3E8C = 1;
    D_800D2CA4[0xB] = 0;
    if (D_800C3EAC[0x2D3] != 0) {
        func_80085350();
        func_80071A08();
        D_800D3298 = 0;
        D_800C3EAC[0x2D3]--;
        D_800C4922 = D_800C3EAC[0x2D3];
        D_800C3EAC[0x2DA] = 0;
        D_800C3EAC[0x2DB] = 0;
        for (i = 0; i < 5; i++) {
            D_800D2CA4[i] = 0;
        }
        D_800D2C94 = 0;
        func_80099890(D_800C3EAC[0x2D3]);
        if (D_800C3EAC[0x2D3] >= 3) {
            if ((u8)replay == 0) {
                func_800799C8(D_800C3EAC[0x2D3], D_800CCD68__rows[D_800C3EAC[0x2D3]][0] & 0x2000);
                if (!(D_800C3EB4__r[D_800C3EAC[0x2D3]][3] & 0x80) && !(D_800CCD1C__rows[D_800C3EAC[0x2D3]][0] & 0x400)) {
                    ((u8*)D_800D2D28)[0xB5] = 1;
                    D_800D3718[0xE] = SystemRenderStringEntry(GetStringEntry(D_800C3DDC, D_800C3E3D[D_800C3EAC[0x2D3]]),
                                                              *(u8**)(D_800D3718 + 8), 0x39, 0);
                    func_800769E8(D_800D3718, *(u8**)(D_800D3718 + 8));
                    D_800D3718[0xD] = 1;
                }
            }
            if (!(D_800CCD64__rows[D_800C3EAC[0x2D3]][0] & 0x2080) && !(D_800CCD68__rows[D_800C3EAC[0x2D3]][0] & 0x1000)) {
                func_80079778(D_800C3EAC[0x2D3]);
                func_80071A8C();
                func_80071964();
                func_80071AE0();
            }
            func_80071A8C();
            func_80070EB0(0);
        } else {
            for (m = 0x1C; m >= 0; m -= 4) {
                D_800C3D1A[m] = 0;
            }
            /* the turn cursor POLY_F4 of this buffer */
            *(s16*)(D_800C3EA4__b + D_800CCB04__a_s32[12] * 0x18 + 0x63D0) = D_800C3EAC[0x2D3] * 0x60 + (D_800C3254[D_800D3280 * 3 + D_800C3EAC[0x2D3]] + 0x10);
            *(s16*)(D_800C3EA4__b + D_800CCB04__a_s32[12] * 0x18 + 0x63D2) = 8;
            *(s16*)(D_800C3EA4__b + D_800CCB04__a_s32[12] * 0x18 + 0x63D4) = D_800C3EAC[0x2D3] * 0x60 + (t = D_800C3254[D_800D3280 * 3 + D_800C3EAC[0x2D3]]) + 0x28;
            *(s16*)(D_800C3EA4__b + D_800CCB04__a_s32[12] * 0x18 + 0x63D6) = 8;
            k = D_800D3280 * 3;
            *(s16*)(D_800C3EA4__b + D_800CCB04__a_s32[12] * 0x18 + 0x63D8) = D_800C3EAC[0x2D3] * 0x60 + (D_800C3254[k + D_800C3EAC[0x2D3]] + 0x10);
            *(s16*)(D_800C3EA4__b + D_800CCB04__a_s32[12] * 0x18 + 0x63DA) = 0x20;
            *(s16*)(D_800C3EA4__b + D_800CCB04__a_s32[12] * 0x18 + 0x63DC) = D_800C3EAC[0x2D3] * 0x60 + (u = D_800C3254[k + D_800C3EAC[0x2D3]]) + 0x28;
            *(s16*)(D_800C3EA4__b + D_800CCB04__a_s32[12] * 0x18 + 0x63DE) = 0x20;
            D_800C3EA4__b[0x6414] = D_800CCB04__a_s32[12];
            D_800C3EA4__b[0x6415] = 1;
            if (!(D_800CCD64__rows[D_800C3EAC[0x2D3]][0] & 0x2080) && !(D_800CCD68__rows[D_800C3EAC[0x2D3]][0] & 0x1000)) {
                if (!(D_800CCD68__rows[D_800C3EAC[0x2D3]][0] & 0x2000)) {
                    func_80080160(D_800C3EAC[0x2D3]);
                } else {
                    func_80080C94(D_800C3EAC[0x2D3]);
                }
                if (D_800C3EAC[0x2EA] != 0) {
                    D_8005941C++;
                }
                D_800D36C0 = D_800C3EAC[0x2D3];
                ((u8*)D_800D2D28)[0x97] = 0;
                func_800BCD98(0);
            }
            func_80071964();
            func_80071AE0();
            func_80071A8C();
            func_80070EB0(0);
            for (i = 0; i < 8; i++) {
                if (func_80089C9C(D_800D2C94, i + 3) != 0) {
                    D_800C3D1A[i << 2] = 1;
                }
            }
            func_80079C24();
        }
        func_80071A38();
        func_80072270();
        D_800C3EA4__b[0x6415] = 0;
        for (i = 0; i < 11; i++) {
            (D_800C3EAC + (i << 6))[0x3C] = func_800841E0(i);
            D_800C3EB4__r[i][6] = func_80085310(i, (D_800C3EAC + (i << 6))[0x3C]);
        }
        func_800BA4E0(func_80080AE4(D_800C3EAC[0x2D3]));
        func_80071A08();
        func_8007252C();
        if (D_800C48EA[0] == 0) {
            func_80085B58(D_800C3EAC[0x2D3]);
        }
        func_8007252C();
        func_80071A38();
        func_800BFE48();
        func_800718BC();
        D_800D3298 = 1;
    }
}

extern u16 D_800D2C9E;
extern u16 D_800D2E06[];
extern u8 D_800D2DE4[];
extern u16 func_80089C9C(s32 mask, u8 idx);

/* func_80072270.s: when D_800D2C9E has any of its low three bits set, for
 * each party actor i < 3 that func_80089C9C(D_800D2C9E, i) selects: clear
 * D_800D2DE4[i], copy the u16 eleven slots back (D_800D2E06[i - 11]) into
 * D_800D2E06[i] and set byte +0x7C + i of D_800D2D28; then clear
 * D_800D2C9E.  The D_800D2D28 byte is addressed as (base + i)[0x7C], which
 * gives retail's base-first addu. */
void func_80072270(void) {
    s32 i;

    if (D_800D2C9E & 7) {
        for (i = 0; i < 3; i++) {
            if (func_80089C9C(D_800D2C9E, i) != 0) {
                D_800D2DE4[i] = 0;
                D_800D2E06[i] = D_800D2E06[i - 11];
                ((u8*)D_800D2D28 + i)[0x7C] = 1;
            }
        }
    }
    D_800D2C9E = 0;
}
extern u16 D_800D39E0;
extern u8 D_800C402F[];
extern u8 D_800D2E5C;
extern u8 D_800D2E60;
extern u16 func_80089C9C(s32 mask, u8 idx);
extern void func_80085AC4(u8 a0);
extern void func_80071B94(s32 a0);

/* func_80072324.s: for each enemy actor i in 3..10 that
 * func_80089C9C(D_800D39E0, i) selects: clear 256 bytes, reset the 32
 * D_800C402F command bytes to 0xFF (as func_800792F8), set D_800D2E5C = 4
 * and D_800D2E60 = 0x17, then call func_80085AC4(i) and func_80071B94(1).
 * ORIGINAL BUG, kept on purpose: the 256-byte clear goes through p, which
 * is never assigned (retail clears from whatever $s2 holds on entry; its
 * caller func_800723E0 does not set $s2 either), apparently meant to be
 * D_800D2E5C (func_800799C8 clears that block the same way).  The bound is
 * computed from the running copy q, so loop.c does not hoist it into
 * another saved register; 0xFF is a constant in the store, which loop.c
 * hoists after the i setup, as in retail. */
void func_80072324(void) {
    s32 i;
    s32 k;
    u8* p;
    u8* q;
    u8* end;

    for (i = 3; i < 11; i++) {
        if (func_80089C9C(D_800D39E0, i) != 0) {
            q = p;
            end = q + 0x100;
            do {
                *q++ = 0;
            } while (q < end);
            for (k = 0x8B8; k >= 0; k -= 0x48) {
                D_800C402F[k] = 0xFF;
            }
            D_800D2E5C = 4;
            D_800D2E60 = 0x17;
            func_80085AC4(i);
            func_80071B94(1);
        }
    }
}
#endif
