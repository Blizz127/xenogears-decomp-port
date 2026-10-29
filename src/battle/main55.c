#include "common.h"


#ifndef XENO_PC_PORT
extern u8* D_800C34B0;
#endif
extern u8 g_GameState[];

/* func_8009A7E4: 1 when entry idx's byte +0x581E (16-byte stride off
 * D_800C34B0) equals one of the four party slot ids at g_GameState+0x502,
 * +0x50A, +0x512, +0x51A. Signature/logic taken from src/battle/main55.c's
 * not-yet-matching INCLUDE_ASM comment/body for this function.
 * Two source-shape requirements were needed to get byte-exact scheduling:
 * (1) fold the id load into the first comparison via `(id = ...)` so it's
 * scheduled right after the first g_GameState fetch, not before it (a bare
 * preceding `id = ...;` statement makes gcc load id first, mismatching
 * retail's state-then-id order); (2) use if/else-if assigning to a single
 * `result` with one `return result;` at the end, not a chain of
 * `if (...) return 1;` early returns — the early-return chain's third
 * branch gets a different merge/tail-jump treatment from gcc (an extra
 * `j` plus a relocated `ori v0,zero,1`), while the if/else-if chain
 * naturally reuses the same branch target for all three "hit" arms and
 * matches retail exactly. */
u32 func_8009A7E4(u8 idx) {
    u32 id;
    u32 result;

    if (g_GameState[0x502] == (id = D_800C34B0[idx * 16 + 0x581E])) {
        result = 1;
    } else if (g_GameState[0x50A] == id) {
        result = 1;
    } else if (g_GameState[0x512] == id) {
        result = 1;
    } else {
        result = g_GameState[0x51A] == id;
    }
    return result;
}

#ifndef XENO_PC_PORT
/* func_8009A854.s, byte-exact: copy entry a0's bytes +0xC/+0xB/+0xA (16-byte
 * stride at D_800C34B0+0x54F8) into the party slot whose id matches its +6
 * byte (the last match wins; no match keeps a1), record a0 in the slot,
 * mirror +3 into g_GameState+0x2286[a0], then patch every type-4 actor
 * among the first three.  Retail stores +0xB and the slot byte twice.  The
 * loop stores are pointer sums with the shift, *(p + (s << 3) + 4): with
 * `s * 8`, or as p[(s << 3) + 4], the loop's address adds come out index
 * first. */
void func_8009A854(u8 a0, u8 a1) {
    u8* e = D_800C34B0 + (a0 * 16 + 0x54F8);
    u8 id = e[6];
    u8 s;
    u8 i;

    if (g_GameState[0x502] == id) {
        a1 = 0;
    }
    if (g_GameState[0x50A] == id) {
        a1 = 1;
    }
    if (g_GameState[0x512] == id) {
        a1 = 2;
    }
    if (g_GameState[0x51A] == id) {
        a1 = 3;
    }
    s = a1;
    g_GameState[(s << 3) + 0x500] = e[0xC];
    g_GameState[(s << 3) + 0x4FF] = e[0xB];
    g_GameState[(s << 3) + 0x4FE] = e[0xA];
    g_GameState[(s << 3) + 0x4FF] = e[0xB];
    g_GameState[0x56B + s] = a0;
    g_GameState[0x2286 + a0] = e[3];
    g_GameState[0x56B + s] = a0;
    for (i = 0; i < 3; i++) {
        u8* p = D_800C34B0 + i * 0x170;

        if (p[0x56] == 4) {
            *(p + (s << 3) + 4) = e[0xC];
            *(p + (s << 3) + 3) = e[0xB];
            *(p + (s << 3) + 2) = e[0xA];
            *(p + (s << 3) + 3) = e[0xB];
            *(p + s + 0x6F) = a0;
        }
    }
}
#endif
extern int rand(void);
extern void func_8009BE0C(void);

/* Byte-exact under gcc 2.6.3 (BattleGcc263); the port compiles the same
 * body. */
/* func_8009A9D0.s: clear the +0x5FC2 mode byte; on a 50% rand() roll run
 * func_8009BE0C and return 1, else return 0. */
u32 func_8009A9D0(void) {
    D_800C34B0[0x5FC2] = 0;
    if ((s32)(rand() % 100) < 0x32) {
        func_8009BE0C();
        return 1;
    }
    return 0;
}
#ifndef XENO_PC_PORT
extern u8* D_800C34B0;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2C34;
#endif
/* func_8009AA44.s: clear the D_800C34B0 mode byte, set bit0 of the actor's
 * +0x15A flag and, when that flag already had bit7 with +0x126 bit4, drop the
 * 0x1B0 bits of +0x120 and bit12 of +0x7C.  Finishes by stamping 0x3D into
 * +0x5FC7 while D_800D2C34 is 4.  D_800C34B0 is re-read for every access. */
void func_8009AA44(u8 idx) {
    u32 off = (u32)idx * 368;
    D_800C34B0[0x5FC2] = 0;
    (D_800C34B0 + off)[0x15A] |= 1;
    {
        /* Copying the pointer to a local first keeps retail's addu
         * operand order under gcc 2.6.3. */
        u8* base = D_800C34B0;
        u8* p = base + off;

        if (p[0x15A] & 0x80) {
            if (*(u16*)(p + 0x126) & 0x10) {
                *(u16*)(p + 0x120) &= 0xFE4F;
                *(u16*)(p + 0x7C) &= 0xEFFF;
            }
        }
    }

    if (D_800D2C34 == 4) {
        D_800C34B0[0x5FC7] = 0x3D;
    }
}
#ifndef XENO_PC_PORT
extern u8* D_800C34B0;
#endif


/* func_8009AB00.s (local copy of the pointer: see func_8009AA44) */
void func_8009AB00(u8 index) {
    u8* base = D_800C34B0;
    u8* p = base + index * 0x170;

    p[0x15A] &= 0xFE;
}


/* ---- Port bodies (fo/bat3). ---- */
#ifdef XENO_PC_PORT
extern int rand(void);
extern void func_8009BE0C(void);

/* func_8009A854.s: copy entry a0's three bytes (+0xA/+0xB/+0xC, 16-byte
 * stride at D_800C34B0+0x54F8) into the party slot whose id matches its +6
 * byte (the LAST match wins; no match keeps the caller's a1), record a0 in
 * the slot, mirror +3 into g_GameState+0x2286[a0], then patch every actor of
 * type 4 among the first three.  Retail stores +0xB twice. */
void func_8009A854(u32 a0, u32 a1) {
    u32 idx = a0 & 0xFF;
    u8* e = D_800C34B0 + idx * 16 + 0x54F8;
    u32 id = e[6];
    u32 s;
    u32 o;
    u32 i;

    if (g_GameState[0x502] == id) a1 = 0;
    if (g_GameState[0x50A] == id) a1 = 1;
    if (g_GameState[0x512] == id) a1 = 2;
    if (g_GameState[0x51A] == id) a1 = 3;
    s = a1 & 0xFF;
    o = s * 8;
    g_GameState[0x500 + o] = e[0xC];
    g_GameState[0x4FF + o] = e[0xB];
    g_GameState[0x4FE + o] = e[0xA];
    g_GameState[0x4FF + o] = e[0xB];
    g_GameState[0x56B + s] = a0;
    g_GameState[0x2286 + idx] = e[3];
    g_GameState[0x56B + s] = a0;
    for (i = 0; i < 3; i++) {
        u8* p = D_800C34B0 + i * 0x170;

        if (p[0x56] == 4) {
            p[o + 4] = e[0xC];
            p[o + 3] = e[0xB];
            p[o + 2] = e[0xA];
            p[o + 3] = e[0xB];
            p[s + 0x6F] = a0;
        }
    }
}

#endif /* XENO_PC_PORT */
