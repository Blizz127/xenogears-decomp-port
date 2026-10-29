/* Retail run 1 of mainc25: the byte-exact C bodies on one side of a
 * retail-asm gap; see the BATTLE_SUB note in mainc25.c. */
#define BATTLE_TU_SUB 1
#include "common.h"
#ifndef XENO_PC_PORT
extern u32 func_8007A628(u32 a0, u32 a1);
extern u8 func_8001BD40(u8 a0, u8 a1);
extern u32 func_80089C08(u8 idx);
extern u8 D_800D32A1__rows[][8] asm("D_800D32A1");
extern u8 D_800C3EB4__rows[][28] asm("D_800C3EB4");
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

/* func_8007C840.s, byte-exact: battle-script op on pp: clear halfword
 * slot (*pp)[1] of enemy `idx`'s D_800D3400 record, then draw
 * r = func_8001BD40(0, 2) not tried yet: when func_8007A628(r, (*pp)[2])
 * accepts r, r's D_800D32A1 byte is SET and r's group (byte 0 of its
 * 28-byte D_800C3EB4 row) is that of slot idx + 3, store func_80089C08(r)
 * in the slot and return; otherwise mark r tried, until all three are.
 * Codegen: the slot goes through the D_800D3400 record view (field h at
 * +0x20).  A `D_800D3420 + index * 64` row pointer instead gets its `la`
 * scheduled ahead of the p[1] load into another register; through the
 * record, retail's `la` lands after that load, in p's register.  Matches
 * under gcc 2.6.x (the TU's preset) and 2.7.2. */
void func_8007C840(u8** pp, u8 idx) {
    u8 tried[3];
    u16 v;
    u8 r;

    tried[0] = tried[1] = tried[2] = 0;
    D_800D3400__rec[idx].h[(*pp)[1]] = 0;
    while (!(tried[0] & tried[1] & tried[2])) {
        r = func_8001BD40(0, 2);
        if (tried[r] == 0) {
            if ((u8)func_8007A628(r, (*pp)[2]) != 0
                && D_800D32A1__rows[r][0] != 0
                && D_800C3EB4__rows[idx + 3][0] == D_800C3EB4__rows[r][0]) {
                v = func_80089C08(r);
                D_800D3400__rec[idx].h[(*pp)[1]] = v;
                return;
            }
            tried[r] = 1;
        }
    }
}

extern u8 D_800D32A1[];
extern u8 D_800D3420[];

/* func_8007C9D4.s: clear u16 slot (*pp)[1] of row idx of D_800D3420, then
 * draw party actors with func_8001BD40(0, 2) until one not tried yet passes
 * func_8007A628(r, (*pp)[2]) with D_800D32A1[r * 8] set; store
 * func_80089C08(r) in the slot.  Each failed draw marks tried[r]; stop when
 * all three are marked.  The found branch recomputes the row address into
 * dst: loop.c hoists that invariant into the preheader, where it becomes
 * retail's copy of the first row ($a1 -> $s4); reusing row is 3 insns
 * longer.  k holds r * 8 so D_800D32A1 is indexed as D_800D32A1($at)
 * instead of a hoisted la.  Found with a decomp-permuter hint. */
void func_8007C9D4(u8** pp, u8 idx) {
    u8 tried[3];
    u8* row;
    u8* dst;
    u8 r;
    u32 k;

    tried[2] = 0;
    tried[1] = 0;
    tried[0] = 0;
    row = D_800D3420 + (idx << 6);
    *(u16*)(row + (*pp)[1] * 2) = 0;
    while (!(tried[0] & tried[1] & tried[2])) {
        r = func_8001BD40(0, 2);
        if (tried[r] == 0) {
            if ((func_8007A628(r, (*pp)[2]) & 0xFF) != 0) {
                k = r * 8;
                if (D_800D32A1[k] != 0) {
                    dst = D_800D3420 + (idx << 6);
                    *(u16*)(dst + (*pp)[1] * 2) = func_80089C08(r);
                    return;
                }
            }
            tried[r] = 1;
        }
    }
}

extern u8 D_800D32A1__rows[][8] asm("D_800D32A1");

/* func_8007CB20.s, byte-exact: clear u16 slot (*pp)[1] of row idx of
 * D_800D3420, collect the actors i in 3..10 that pass
 * func_8007A628(i, (*pp)[2]) with their D_800D32A1 byte (8-byte rows) set,
 * and store func_80089C08 of a random one (func_8001BD40(0, n - 1)) in the
 * slot.  The func_8007D344 form (mainasm_D854).  D_800D32A1 is read
 * through a [][8] row view: indexed flat (i * 8), the symbol joins the
 * strength-reduced offset (1 insn short). */
void func_8007CB20(u8** pp, u8 idx) {
    u8 list[8];
    s32 i = 3;
    s32 n = 0;
    u16* row;
    u16* dst;
    u16 v;

    row = (u16*)(D_800D3420 + ((u32)idx << 6));
    row[(*pp)[1]] = 0;
    for (; i < 11; i++) {
        if ((u8)func_8007A628((u8)i, (*pp)[2]) != 0 && D_800D32A1__rows[i][0] != 0) {
            list[n] = i;
            n++;
        }
    }
    if (n != 0) {
        v = func_80089C08(list[func_8001BD40(0, n - 1)]);
        dst = (u16*)(D_800D3420 + ((u32)idx << 6));
        dst[(*pp)[1]] = v;
    }
}

extern u32 func_8007A628(u32 a0, u32 a1);
extern u8 D_800C3EB4[];
extern u8 D_800D3430[];

/* func_8007CC50.s: count the party actors 0..2 that pass func_8007A628(i, 0)
 * and whose 28-byte D_800C3EB4 record starts with (*pp)[2]; store the count
 * in byte (*pp)[1] of row idx of D_800D3430.  The record offset is its own
 * counter, as in main27's func_8007D6A8: gcc 2.6.3 then reduces the offset
 * and indexes the table as D_800C3EB4($at).  n is set first, as retail
 * clears it before i and off. */
void func_8007CC50(u8** pp, s32 idx) {
    u8 n = 0;
    s32 i = 0;
    u32 off = 0;
    u8* row;

    for (; i < 3; i++) {
        if ((func_8007A628((u32)i & 0xFF, 0) & 0xFF) != 0) {
            if (D_800C3EB4[off] == (*pp)[2]) {
                n++;
            }
        }
        off += 28;
    }
    row = D_800D3430 + ((idx & 0xFF) << 6);
    row[(*pp)[1]] = n;
}

/* func_8007CD10.s: func_8007CC50 for the enemy actors 3..10. */
void func_8007CD10(u8** pp, s32 idx) {
    u8 n = 0;
    s32 i = 3;
    u32 off = 0x54;
    u8* row;

    for (; i < 0xB; i++) {
        if ((func_8007A628((u32)i & 0xFF, 0) & 0xFF) != 0) {
            if (D_800C3EB4[off] == (*pp)[2]) {
                n++;
            }
        }
        off += 28;
    }
    row = D_800D3430 + ((idx & 0xFF) << 6);
    row[(*pp)[1]] = n;
}

extern u32 func_80079E7C(u16 value);
extern u8* D_800D3364;

/* func_8007CDD0.s, byte-exact: battle-script op on pp (func_8007B424's
 * kind): v = func_80079E7C(halfword slot (*pp)[2] of enemy `index`'s
 * D_800D3400 record), then byte slot (*pp)[1] of the same record gets the
 * D_800D3364 byte at +0x140 + group(index + 3) * 64 + group(v) * 8 (groups
 * are byte 0 of the 28-byte D_800C3EB4 rows).
 * Codegen: both slots go through the record view, so retail's
 * `la D_800D3420` is reused for the byte table as `+ 0x10`.  The table byte
 * is read through an integer address ((u32)D_800D3364 + sum + 0x140, as
 * func_80087A38 in main39 does); pointer indexing swaps the addu operands.
 * Matches under gcc 2.6.x (the TU's preset) and 2.7.2. */
void func_8007CDD0(u8** pp, u8 index) {
    u8 v;

    v = func_80079E7C(D_800D3400__rec[index].h[(*pp)[2]]);
    D_800D3400__rec[index].b[(*pp)[1]] = *(u8*)((u32)D_800D3364 + ((D_800C3EB4__rows[v][0] << 3) + (D_800C3EB4__rows[index + 3][0] << 6)) + 0x140);
}
extern u8 D_800D3420[];

/* func_8007CEA4.s: func_8007CC50 with the record byte taken from an actor:
 * count the party actors 0..2 that pass func_8007A628(i, 0) and whose
 * D_800C3EB4 record starts with the same byte as the record of actor
 * func_80079E7C(mask), mask being u16 slot (*pp)[2] of row idx of
 * D_800D3420.  The mask is re-read for every actor, as retail does.  k
 * holds v * 28 so the table index is a register (a bare v * 28 index makes
 * expand load D_800C3EB4 into a register, which loop.c hoists); idx is u8
 * (an s32 idx copies a1 to s5 before n and i are cleared). */
void func_8007CEA4(u8** pp, u8 idx) {
    u8 n = 0;
    s32 i = 0;
    u8* src = D_800D3420 + (idx << 6);
    u32 off = 0;
    u8* row;

    for (; i < 3; i++) {
        if ((func_8007A628((u32)i & 0xFF, 0) & 0xFF) != 0) {
            u8 v = func_80079E7C(*(u16*)(src + (*pp)[2] * 2));
            u32 k = v * 28;

            if (D_800C3EB4[off] == D_800C3EB4[k]) {
                n++;
            }
        }
        off += 28;
    }
    row = D_800D3430 + (idx << 6);
    row[(*pp)[1]] = n;
}

/* func_8007CFB8.s: func_8007CEA4 for the enemy actors 3..10. */
void func_8007CFB8(u8** pp, u8 idx) {
    u8 n = 0;
    s32 i = 3;
    u8* src = D_800D3420 + (idx << 6);
    u32 off = 0x54;
    u8* row;

    for (; i < 0xB; i++) {
        if ((func_8007A628((u32)i & 0xFF, 0) & 0xFF) != 0) {
            u8 v = func_80079E7C(*(u16*)(src + (*pp)[2] * 2));
            u32 k = v * 28;

            if (D_800C3EB4[off] == D_800C3EB4[k]) {
                n++;
            }
        }
        off += 28;
    }
    row = D_800D3430 + (idx << 6);
    row[(*pp)[1]] = n;
}
#endif
#include "mainc25.c"
