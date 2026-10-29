#include "common.h"


#ifndef XENO_PC_PORT
extern u32 func_8007A280(u32 a0, u32 a1, u32 a2, u32 a3);
extern u8 D_800D2E5D[];
extern u8 D_800D2E60[];
extern u8 D_800D2E61[];

void func_8007916C(u8 a0, u8 a1, u8 a2) {
    u32 s1 = a0 & 0xFF;
    u32 s0 = (a1 & 0xFF) << 3;
    u32 r;

    r = func_8007A280(s1, D_800D2E5D[s0], 0, 1);
    func_8007A280(s1, D_800D2E5D[s0], (D_800D2E60[s0] + (r + (D_800D2E61[s0] << 8))) & 0xFFFF, 0);
}

extern void func_8007893C(u8, u8);
extern void func_800785D4(u8, u8);
extern u8* D_800C3EAC;
extern u8 D_800C402F[];

void func_800791FC(u8 a, u8 b, u8 c) {
    func_8007893C(b, a);
    D_800C402F[D_800C3EAC[0x2DA] * 0x48] = 0xF4;
    func_800785D4(a, b);
}

extern void func_800785D4(u8, u8);
extern u8* D_800C3EAC;
extern u8 D_800C402F[];
extern u16 D_800C3FFE[];
extern u16 D_800D2E62[];

void func_80079270(u8 a, u8 b, u8 c) {
    D_800C402F[D_800C3EAC[0x2DA] * 0x48] = 0xF6;
    D_800C3FFE[D_800C3EAC[0x2DA] * 0x24] = D_800D2E62[b * 4];
    func_800785D4(a, b);
}

extern void FontPrintf(char* fmt, ...);
extern s32* D_8005917C;
extern char D_8006FB08[];
extern char D_8006FB0C[];
extern char D_8006FB24[];

/* Fatal-error screen: clear the 32 command bytes, then (unless the table at
 * D_8005917C is empty) print the two ids forever, indenting 0..20 times. */
void func_800792F8(u8 a, u8 b) {
    u8 ff;
    s32 k;
    s32 i;
    s32 n;
    s32 j;

    ff = 0xFF;
    for (k = 0x8B8; k >= 0; k -= 0x48) {
        D_800C402F[k] = ff;
    }
    i = 0;
    n = 0;
    if (*D_8005917C != -1) {
        while (1) {
            for (j = 0; j < n; j++) {
                FontPrintf(D_8006FB08);
            }
            FontPrintf(D_8006FB0C);
            i++;
            FontPrintf(D_8006FB24, a, b);
            func_800716D8();
            if (i >= 3) {
                i = 0;
                if (++n >= 21) {
                    n = 0;
                }
            }
        }
    }
}

extern u8 D_800D2E5C__rows[][8] asm("D_800D2E5C");
extern u16 D_800D2E62__rows[][4] asm("D_800D2E62");
extern u8 D_800D2E60__rows[][8] asm("D_800D2E60");
extern u16 func_80089C9C(s32 mask, u8 idx);
extern void func_80085388(void);
extern void func_80078998(u8 a, u8 b, u8 c);
extern void func_80078B34(u8 a, u8 b, u8 c);
extern void func_80078C9C(u8 a, u8 b, u8 c);
extern void func_80078CEC(u8 a, u8 b, u8 c);
extern void func_80078D48(u8 a, u8 b, u8 c);
extern void func_80078D6C(u8 a, u8 b, u8 c);
extern void func_80078E24(u8 a, u8 b, u8 c);
extern void func_80079054(u8 a, u8 b, u8 c);
extern void func_80079098(u8 a, u8 b, u8 c);
extern void func_80079114(u8 a, u8 b, u8 c);
extern void func_80078658(u8 a, u8 b);
extern void func_800787E0(u8 a, u8 b);
extern void func_8007887C(u8 a);

/* func_800793F0.s, byte-exact: run actor `actor`'s queued AI commands.
 * For each of the 32 eight-byte D_800D2E5C entries i (and past them while
 * no terminator has been seen): `found` is the first actor 0..10 that
 * func_80089C9C selects from the entry's D_800D2E62 halfword (0 if
 * none); func_80085388(); then the entry's D_800D2E5C opcode picks the
 * handler: 0 ends the run (cont = 0), 1-11, 13 and 15 call
 * func_80078998 .. func_8007916C, func_800791FC, func_80079270 with
 * (actor, i, found), 12 calls func_80078658(i, actor), 14 calls
 * func_800787E0(the entry's D_800D2E60 byte, actor), 16 calls
 * func_8007887C(actor), anything else func_800792F8(actor, i).
 * The switch is a jump table (jtbl_8006FB38, placed by the main12
 * .rodata split in config/battle.yaml).  Codegen: the entry tables go
 * through asm-label row views (one 8-byte offset, as in retail); `k`, a
 * copy of j, gives retail's loop-top copy (func_80078E24); `i` and `cont`
 * are set in that order, as are `j` and `found`.  Matches only under gcc
 * 2.6.x.  Every handler takes (actor, i, found) as retail passes them, so
 * func_8007916C, func_800791FC and func_80079270 above take a third
 * (unused) parameter, and func_8007916C takes u8 parameters (retail
 * masks the arguments; its body already masked them). */
void func_800793F0(u8 actor) {
    s32 i;
    s32 j;
    s32 k;
    u8 found;
    u8 cont;

    for (i = 0, cont = 1; i < 32 || cont; i++) {
        for (j = 0, found = 0; j < 11; j++) {
            k = j;
            if (func_80089C9C(D_800D2E62__rows[i][0], k)) {
                found = k;
                break;
            }
        }
        func_80085388();
        switch (D_800D2E5C__rows[i][0]) {
        case 0:
            cont = 0;
            break;
        case 1:
            func_80078998(actor, i, found);
            break;
        case 2:
            func_80078B34(actor, i, found);
            break;
        case 3:
            func_80078C9C(actor, i, found);
            break;
        case 4:
            func_80078CEC(actor, i, found);
            break;
        case 5:
            func_80078D48(actor, i, found);
            break;
        case 6:
            func_80078D6C(actor, i, found);
            break;
        case 7:
            func_80078E24(actor, i, found);
            break;
        case 8:
            func_80079054(actor, i, found);
            break;
        case 9:
            func_80079098(actor, i, found);
            break;
        case 10:
            func_80079114(actor, i, found);
            break;
        case 11:
            func_8007916C(actor, i, found);
            break;
        case 12:
            func_80078658(i, actor);
            break;
        case 13:
            func_800791FC(actor, i, found);
            break;
        case 14:
            func_800787E0(D_800D2E60__rows[i][0], actor);
            break;
        case 15:
            func_80079270(actor, i, found);
            break;
        case 16:
            func_8007887C(actor);
            break;
        default:
            func_800792F8(actor, i);
            break;
        }
    }
}
extern u8 D_800D32A1[];
extern u8 D_800D366C;
extern u8 D_800C400B[];

void func_80079674(u8 a) {
    if (D_800D32A1[a * 8] != 0) {
        D_800C400B[D_800C3EAC[0x2DA] * 0x48] = a;
        D_800C402F[D_800C3EAC[0x2DA] * 0x48] = 0x1B;
        D_800C3EAC[0x2DA]++;
    }
    D_800C400B[D_800C3EAC[0x2DA] * 0x48] = a;
    D_800C402F[D_800C3EAC[0x2DA] * 0x48] = 0xFE;
    D_800D366C = 0;
}

extern void func_80085350(void);
extern void func_80085388(void);
extern void func_800716D8(void);
extern s32 func_80080AE4(u8);
extern void func_800B89FC(s32, u8, s32, s32);
extern void func_800793F0(u8);
extern void func_80079674(u8);
extern u8* D_800C3EAC;
extern u8 D_800C400B[];

void func_80079778(u8 a) {
    D_800C3EAC[0x2DA] = 0;
    func_80085350();
    func_80085388();
    D_800C400B[D_800C3EAC[0x2DA] * 0x48] = a;
    func_800716D8();
    func_800B89FC(1, a, 0, func_80080AE4(a));
    func_800793F0(a);
    func_80079674(a);
    while (D_800C3EAC[0x2DB] == 0) {
        func_800716D8();
    }
}

extern u16 func_80089C08(u8 idx);
extern u8 D_800D342E[];
extern u8 D_800D3439[];
extern u8 D_800D343E[];
extern u8 D_800D2CA4[];
extern u16 D_800C48E8;

/* func_80079840.s: for an enemy e (>= 3), initialise its 64-byte
 * D_800D3400 AI record (index e - 3): +0x2E = func_80089C08(a), copy the
 * five D_800D2CA4 bytes to +0x39.., and set +0x3E to whether D_800C48E8
 * masks func_80089C08 of actor a's +0x3C byte in the D_800C3EAC rows.
 * Fields are addressed through splat's per-field labels, as main12 does
 * elsewhere; (base + (a << 6))[0x3C] gives retail's base-first addu. */
void func_80079840(u8 a, u8 e) {
    u8 k;
    s32 i;

    if (e >= 3) {
        k = e - 3;
        *(u16*)(D_800D342E + (k << 6)) = func_80089C08(a);
        for (i = 0; i < 5; i++) {
            D_800D3439[(k << 6) + i] = D_800D2CA4[i];
        }
        if (D_800C48E8 & func_80089C08((D_800C3EAC + (a << 6))[0x3C])) {
            D_800D343E[k << 6] = 1;
        } else {
            D_800D343E[k << 6] = 0;
        }
    }
}
#endif


#ifndef XENO_PC_PORT
extern u32 D_800D3344;
#endif
#ifndef XENO_PC_PORT
extern u32 D_800D39CC;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3B74;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3D6C;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800C3610;
#endif


/* func_80079934.s */
void func_80079934(u32* pValue) {
    *pValue += 4;
}
