#include "common.h"
#ifndef BATTLE_TU_SUB
#define BATTLE_TU_SUB 0
#endif
/* Selects one run of byte-exact C bodies when a retail-asm gap splits them;
 * 0 (host/port/tests) compiles every run. */
#define BATTLE_SUB(n) (BATTLE_TU_SUB == 0 || BATTLE_TU_SUB == (n))


extern void *HeapAlloc(u_int allocSize, u_int allocFlags);
extern u_int HeapFree(void *pMem);
extern void func_800AA320(u32 arg0, u32 arg1, u32 arg2);

/* func_800BEE2C: HeapAlloc(0x1000, 1), then run func_800AA320(arg0 & 0xFFFF,
 * arg1 & 0xFFFF, arg2) on a stack switched onto that heap block -- retail
 * stores the caller's $sp at buf+0xF9C, sets $sp to buf+0xF98, calls, then
 * reloads $sp from the slot -- and finally HeapFree(buf).  Reassigning $sp is
 * not expressible in portable C, so this stays retail assembly; the port does
 * not link this reference-only TU. */
#ifndef XENO_PC_PORT
extern u8 D_800C3EB0[];
#endif

/* func_800BEEB4: scan 11 mask bits; for each set bit whose
 * D_800C3EB0[0x8C8C + i*4] row pointer is non-null, publish value at row+0x74
 * and append the row to out, returning the number appended.  Retail advances
 * the row base by 4 each iteration (re-adding the 0x8C8C constant) and writes
 * the terminator through out[count], not through the append cursor. */
/* Not yet byte-matching: the matching build assembles retail bytes for
 * this function from its own asm segment (see config/battle.yaml).  The
 * body below is kept for the port and the differential harnesses. */
#ifdef XENO_PC_PORT
u32 func_800BEEB4(u32 mask, u32 *out, u32 value) {
    u8 *p = D_800C3EB0;
    u32 *dst = out;
    u32 count = 0;
    s32 i;

    for (i = 0; i != 11; i++) {
        if ((mask & 1) != 0) {
            u32 entry = *(u32 *)(p + 0x8C8C);

            if (entry != 0) {
                *(u32 *)(entry + 0x74) = value;
                *dst = entry;
                dst++;
                count++;
            }
        }
        p += 4;
        mask = (mask & 0xFFFF) >> 1;
    }
    out[count] = 0;
    return count;
}
#endif /* XENO_PC_PORT */

extern s32 func_80023124(s32 pointA, s32 pointB);

/* Retail builds each coordinate pair in a stack local and reads it back as
 * two halfwords, so the pair is a struct, not a fused expression. */
typedef struct { s16 lo, hi; } BattlePairXZ;
/* Not yet byte-matching: the matching build assembles retail bytes for
 * this function from its own asm segment (see config/battle.yaml).  The
 * body below is kept for the port and the differential harnesses. */
#ifdef XENO_PC_PORT
u32 func_800BEF24(u8 *arg0, u8 *arg1) {
    BattlePairXZ a, b;

    {
        s32 t = *(s16 *)(arg0 + 2);
        a.lo = t;
        t = *(s16 *)(arg0 + 0xA);
        a.hi = t;
    }
    {
        s32 t = *(s16 *)(arg1 + 2);
        b.lo = t;
        t = *(s16 *)(arg1 + 0xA);
        b.hi = t;
    }
    return (u32)(s32)(s16)func_80023124(
        (s32)((u32)(u16)b.lo | ((u32)(u16)b.hi << 16)),
        (s32)((u32)(u16)a.lo | ((u32)(u16)a.hi << 16)));
}
#endif /* XENO_PC_PORT */
/* Not yet byte-matching: the matching build assembles retail bytes for
 * this function from its own asm segment (see config/battle.yaml).  The
 * body below is kept for the port and the differential harnesses. */
#ifdef XENO_PC_PORT
u32 func_800BEF8C(u8 *arg0) {
    BattlePairXZ a, b;

    {
        s32 t = *(s16 *)(arg0 + 2);
        a.lo = t;
        t = *(s16 *)(arg0 + 0xA);
        a.hi = t;
    }
    {
        /* The +0xA0/+0xA4 pair is read UNSIGNED here (retail uses lhu), unlike
         * the +2/+0xA pair above. */
        u32 t = *(u16 *)(arg0 + 0xA0);
        b.lo = t;
        t = *(u16 *)(arg0 + 0xA4);
        b.hi = t;
    }
    return (u32)(s32)(s16)func_80023124(
        (s32)((u32)(u16)b.lo | ((u32)(u16)b.hi << 16)),
        (s32)((u32)(u16)a.lo | ((u32)(u16)a.hi << 16)));
}
#endif /* XENO_PC_PORT */

extern void func_800245D8(u32 arg0, u32 arg1);
#ifndef XENO_PC_PORT
extern u8 *D_800C3610;
#endif

/* Not yet byte-matching: the matching build assembles retail bytes for
 * this function from its own asm segment (see config/battle.yaml).  The
 * body below is kept for the port and the differential harnesses. */
#ifdef XENO_PC_PORT
void func_800BEFF4(u32 arg0) {
    u8 *base = D_800C3610;
    u32 unk = *(u32 *)(base + 4);

    if ((unk != 0) && (*(u32 *)(base + 0x24) != arg0)) {
        u32 idx = ((*(u32 *)(unk + 0xAC) & 3) << 2) |
                  (*(u32 *)(unk + 0xA8) >> 30);
        u8 *entry = D_800C3EB0 + idx * 0x1C;

        if (entry[7] == 0) {
            func_800245D8(unk, (u32)(s32)*(s8 *)(unk + 0xB0));
        }
    }
    {
        u32 *table = (u32 *)(D_800C3EB0 + 0x8C8C);
        u8 *cur = D_800C3610;

        *(u32 *)(cur + 0x24) = arg0;
        *(u32 *)(cur + 4) = table[arg0];
    }
}
#endif /* XENO_PC_PORT */


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


/* func_800BF0B4.s */
#if BATTLE_SUB(2)
void func_800BF0B4(u32 value) {
    *(u32*)(D_800C3610 + 0x1C) = value;
}
#endif /* BATTLE_SUB(2) */
