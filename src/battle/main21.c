#include "common.h"

extern u8 D_800D3420[];
extern u8 D_800D3430[];
extern u32 func_80079E7C(u16 value);
extern u32 func_80079ED8(u8 a0, u8 a1, u8 a2, s32 a3);
extern u32 func_8007A280(u8 a0, u8 a1, u32 a2, u32 a3);

#ifdef XENO_PC_PORT
#ifndef XENO_PC_PORT
extern u8 D_800D3420[];
#endif
extern u32 func_80079E7C(u16 value);
extern u32 func_80079ED8(u8 a0, u8 a1, u8 a2, s32 a3);
extern u32 func_8007A280(u8 a0, u8 a1, u32 a2, u32 a3);
#endif

#ifdef XENO_PC_PORT
/* Port body for the retail row-to-party helper wrapper; the retail build
 * compiles the byte-exact C in the #else branch. */
void func_8007B424(u8** ppBoard, u8 index) {
    u8* p = *ppBoard;
    u8* row = D_800D3420 + ((u32)index << 6);
    u8 slot = p[3];
    u8 value = (u8)func_80079E7C(*(u16*)(row + ((u32)slot << 1)));

    value = (u8)func_80079ED8(value, p[2], 0, 1);
    row += 0x10;
    row[p[1]] = value;
}
#else
/* func_8007B424.s, byte-exact.  base keeps a single set and the +0x10 row
 * is its own variable.  With `base += 0x10` base has two sets, so sched1
 * does not boost its `la` and places it ahead of the *ppBoard load. */
void func_8007B424(u8** ppBoard, u8 index) {
    u32 off = (u32)index << 6;
    u8* base = D_800D3420;
    u8* row;
    u8 v;

    v = func_80079E7C(*(u16*)(base + off + (*ppBoard)[3] * 2));
    row = base + 0x10;
    v = func_80079ED8(v, (*ppBoard)[2], 0, 1);
    (row + off)[(*ppBoard)[1]] = v;
}
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2E5C;

/* func_8007B4B8.s: v = func_80079E7C(u16 slot (*pp)[3] of row index of
 * D_800D3420).  For v < 3 set D_800D2E5C to 0x20 and return n + 1;
 * otherwise call func_80079ED8(v, (*pp)[2], byte (*pp)[1] of the row's
 * +0x10 half, 0) and return n.  As in func_8007B424, base keeps a single
 * set and the +0x10 row is its own pointer (else CSE reuses off + base and
 * folds the 0x10 into the load).  Declaring r first gives retail's $s2 (r)
 * and $s3 (off); declaring off first swaps them. */
u8 func_8007B4B8(u8** pp, u8 index, u8 n) {
    u8 r = n;
    u32 off = (u32)index << 6;
    u8* base = D_800D3420;
    u8* row;
    u8 v;

    v = func_80079E7C(*(u16*)(base + off + (*pp)[3] * 2));
    if (v < 3) {
        D_800D2E5C = 0x20;
        r = n + 1;
    } else {
        row = base + 0x10;
        func_80079ED8(v, (*pp)[2], (row + off)[(*pp)[1]], 0);
    }
    return r;
}
#endif
void func_8007B578(u8** pp, u8 index) {
    u16* row = (u16*)(D_800D3420 + ((u32)index << 6));
    u16 result = func_8007A280(func_80079E7C(row[(*pp)[3]]), (*pp)[2], 0, 1);

    row[(*pp)[1]] = result;
}
#ifndef XENO_PC_PORT
extern u8 D_800D3420[];
extern u32 func_80079E7C(u16 value);
extern u32 func_8007A280(u8 a0, u8 a1, u32 a2, u32 a3);
extern u8 D_800D2E5C;

u8 func_8007B608(u8** pp, u8 index, s32 n) {
    u16* row = (u16*)(D_800D3420 + ((u32)index << 6));
    u8 r = n;
    u8 v = func_80079E7C(row[(*pp)[3]]);

    if (v < 3) {
        D_800D2E5C = 0x20;
        r = n + 1;
    } else {
        func_8007A280(v, (*pp)[2], row[(*pp)[1]], 0);
    }
    return r;
}

extern u8 g_GameState[];
extern u8 D_800CCDEC[];
extern u32 D_800D3410[];

/* func_8007B6C0.s, byte-exact: battle-script op storing a u32 into word
 * p[1] of row `index` of D_800D3410 (p = *ppBoard).  Mode p[2] == 1 stores
 * the +0xCDEC word of the 0x170-byte record of character
 * func_80079E7C(halfword p[3] of the D_800D3420 row); mode 2 stores the
 * g_GameState +0x1924 word.  As in func_8007B424, base keeps one set and
 * the -0x10 row is its own variable; mode 2's row is a local too, which
 * keeps D_800D3410 in a register (a flat `D_800D3410 + off + i * 4` lets
 * combine fold the symbol into an $at store). */
void func_8007B6C0(u8** ppBoard, u8 index) {
    switch ((*ppBoard)[2]) {
    case 1: {
        u32 off = (u32)index << 6;
        u8* base = D_800D3420;
        u8* row;
        u8 v;

        v = func_80079E7C(*(u16*)(base + off + (*ppBoard)[3] * 2));
        row = base - 0x10;
        *(u32*)(row + off + (*ppBoard)[1] * 4) = *(u32*)(D_800CCDEC + v * 0x170);
        break;
    }
    case 2: {
        u32 off = (u32)index << 6;
        u8* row = (u8*)D_800D3410;

        *(u32*)(row + off + (*ppBoard)[1] * 4) = *(u32*)(g_GameState + 0x1924);
        break;
    }
    }
}
/* The 64-byte enemy AI records at D_800D3400 (index = actor - 3): +0x00
 * the script func_800799C8 runs, +0x08 / +0x0C the ones func_80079AB0 /
 * func_80079C24 run, then the battle-script ops' slots: four words at
 * +0x10, eight halfwords at +0x20 and sixteen bytes at +0x30. */
typedef struct {
    u8* script;
    u8 pad4[4];
    u8* script8;
    u8* scriptC;
    u32 w[4];
    u16 h[8];
    u8 b[16];
} EnemyAiRec;
extern EnemyAiRec D_800D3400__rec[] asm("D_800D3400");
extern u8 D_800CCDF0[];
extern u8 D_800CCDEC[];

/* func_8007B7B0.s, byte-exact: the func_8007B4B8 / func_8007B608 op for
 * the word slots.  v = func_80079E7C(halfword slot (*pp)[3] of enemy
 * `index`'s D_800D3400 record); for v < 3 set D_800D2E5C to 0x20 and
 * return n + 1; otherwise store word slot (*pp)[1] of the record into
 * actor v's D_800CCDF0 word ((*pp)[2] == 0) or D_800CCDEC word (0x170-byte
 * records) and return n.
 * Codegen: both slots go through the record view.  Retail keeps
 * `la D_800D3420` across the call and derives the word table as
 * `- 0x10` inside each branch, which a separate `base - 0x10` row pointer
 * (parked draft, 21/73) did not reproduce.  Matches under gcc 2.7.2
 * (main21's preset) and 2.6.x. */
u8 func_8007B7B0(u8** pp, u8 index, u8 n) {
    u8 r = n;
    u8 v;

    v = func_80079E7C(D_800D3400__rec[index].h[(*pp)[3]]);
    if (v < 3) {
        D_800D2E5C = 0x20;
        r = n + 1;
    } else if ((*pp)[2] == 0) {
        *(u32*)(D_800CCDF0 + v * 0x170) = D_800D3400__rec[index].w[(*pp)[1]];
    } else {
        *(u32*)(D_800CCDEC + v * 0x170) = D_800D3400__rec[index].w[(*pp)[1]];
    }
    return r;
}
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D3420[];
#endif
extern u16 D_8005A3A0[];
/* func_8007B8D4.s: board-to-party halfword setter: row[p[1]] = D_8005A3A0[p[2]]. */
void func_8007B8D4(u8** ppBoard, u8 index) {
    u32 off = (u32)index << 6;
    u8* p = *ppBoard;
    u8* base;

    base = D_800D3420;
    ((u16*)(base + off))[p[1]] = D_8005A3A0[p[2]];
}
#ifndef XENO_PC_PORT
extern u8 D_800D3420[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D3430[];
#endif
#ifndef XENO_PC_PORT
extern u32 D_800D3410[];
#endif
extern u16 D_8005A3A0[];
#ifndef XENO_PC_PORT
extern u8 D_800CCE34[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCE3E[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D343F;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D342E;
#endif


/* func_8007B914.s */
void func_8007B914(u8** ppBoard, u8 index) {
    u8* p = *ppBoard;
    u32 i1 = p[1];
    u32 i2 = p[2];
    u32 row = (u32)D_800D3420 + ((u32)index << 6);

    D_8005A3A0[i2] = *(u16*)((i1 << 1) + row);
}
/* func_8007B958.s */
void func_8007B958(u8** ppBoard, u8 index) {
    u8* p = *ppBoard;
    u8* pRow = D_800D3430 + ((u32)index << 6);

    pRow[p[2]] = pRow[p[1]];
}
