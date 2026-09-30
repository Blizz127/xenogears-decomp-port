#include "common.h"


#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/main39", func_80087A38);
INCLUDE_ASM("asm/battle/nonmatchings/main39", func_80087AF0);
INCLUDE_ASM("asm/battle/nonmatchings/main39", func_80087EDC);
INCLUDE_ASM("asm/battle/nonmatchings/main39", func_800881B8);
INCLUDE_ASM("asm/battle/nonmatchings/main39", func_800883AC);
INCLUDE_ASM("asm/battle/nonmatchings/main39", func_80088490);
#endif


#ifndef XENO_PC_PORT
extern u8 D_800C3EB4[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D301C[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D32A1[];
#endif
#ifndef XENO_PC_PORT
extern u8* D_800D3364;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800C3EAC;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D366C;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3E18;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2D24[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C34B3[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD1C[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C400B[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C402F[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C4022[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3FFE[];
#endif
#ifndef XENO_PC_PORT
extern u16 D_800D2C94;
#endif
#ifndef XENO_PC_PORT
extern u16 D_800D2C98;
#endif
extern u8 g_GameState[];
extern void func_80085D34(void);
extern void func_800879A8(u8 v, u8 idx);
extern u32 func_800877E0(u8 a, u8 b);
extern u8 func_80080AE4(u8 target);
extern void func_800B89FC(u32 a, u32 b, u32 c, u32 d);
extern void func_80085388(void);
extern void func_80085C88(u8 index);
extern void func_80085CCC(u8 a0, u16 a1, u16 a2);
extern void func_80079840(u8 a, u8 b);
extern u8 func_80079AB0(void);
extern u16 func_80089C08(u8 index);
extern u32 func_80089C48(u8 index);
extern u32 func_80089C6C(u32 mask, u8 index);
extern u32 func_80089C9C(u32 mask, u8 index);

/* Slot bookkeeping shared by the functions below.  D_800C3EB4 is a table of
 * 28-byte per-combatant rows: +0 group, +1 slot within the group, +0xA/+0xC
 * a cached position.  D_800D301C is a table of 4-byte group records at
 * (group + bank) * 4: +0 occupant count, +1 occupied-slot mask; bank 0 is the
 * party side, 8 the enemy side and 0x10/0x18 their alternate sets.  Combatants
 * 0-2 are the party.  These bodies are for the port and the differential
 * harnesses; the matching build still assembles retail bytes. */
#ifdef XENO_PC_PORT
#define ROW(i) ((u8*)D_800C3EB4 + (u32)(i) * 28u)
#define GROUP(i) (D_800D301C + (u32)(i) * 4u)

/* func_800883AC.s: remove combatant `index` from its group. */
void func_800883AC(u8 index) {
    u32 bank = (index >= 3) ? 8u : 0u;
    u8 mask;

    if (D_800D32A1[(u32)index * 8u] != 0)
        bank |= 0x10;
    GROUP(ROW(index)[0] + bank)[0]--;
    mask = (u8)func_80089C48(ROW(index)[1]);
    GROUP(ROW(index)[0] + bank)[1] &= mask;
}

/* func_80087EDC.s: move combatant `index` into the group of `target` when
 * that group has fewer than four occupants, taking its first free slot. */
void func_80087EDC(u8 index, u8 target) {
    u32 bank;
    u32 slot;
    u8 group;
    u8 *pos;

    if (ROW(index)[0] == ROW(target)[0])
        return;
    bank = (target < 3) ? ((index >= 3) ? 8u : 0u) : 0u;
    if (GROUP(ROW(target)[0] + bank)[0] >= 4)
        return;
    func_800883AC(index);
    GROUP(ROW(target)[0] + bank)[0]++;
    for (slot = 0; slot < 4; slot++) {
        if ((u16)func_80089C9C(GROUP(ROW(target)[0] + bank)[1], (u8)slot) == 0)
            break;
    }
    group = ROW(target)[0];
    ROW(index)[1] = (u8)slot;
    ROW(index)[0] = group;
    GROUP(ROW(index)[0] + bank)[1] |= (u8)func_80089C08(ROW(index)[1]);
    pos = D_800D3364 + (u32)ROW(index)[1] * 4u + (u32)ROW(index)[0] * 32u;
    if (index < 3) {
        *(u16*)(ROW(index) + 0xA) = *(u16*)(pos + 0x4);
        *(u16*)(ROW(index) + 0xC) = *(u16*)(pos + 0x6);
    } else {
        *(u16*)(ROW(index) + 0xA) = *(u16*)(pos + 0x10);
        *(u16*)(ROW(index) + 0xC) = *(u16*)(pos + 0x12);
    }
}

/* func_800881B8.s: move combatant `index` into the (empty) alternate group of
 * `target`. */
void func_800881B8(u8 index, u8 target) {
    u32 bank;
    u8 *pos;

    if (ROW(index)[0] == ROW(target)[0])
        return;
    bank = (target < 3 && index >= 3) ? 0x18u : 0x10u;
    if (GROUP(ROW(target)[0] + bank)[0] != 0)
        return;
    func_800883AC(index);
    {
        u8 group = ROW(target)[0];

        ROW(index)[1] = 0;
        ROW(index)[0] = group;
    }
    GROUP(ROW(index)[0] + bank)[0] = 1;
    GROUP(ROW(index)[0] + bank)[1] = 1;
    pos = D_800D3364 + (u32)ROW(index)[0] * 8u;
    if (index < 3) {
        *(u16*)(ROW(index) + 0xA) = *(u16*)(pos + 0x100);
        *(u16*)(ROW(index) + 0xC) = *(u16*)(pos + 0x102);
    } else {
        *(u16*)(ROW(index) + 0xA) = *(u16*)(pos + 0x104);
        *(u16*)(ROW(index) + 0xC) = *(u16*)(pos + 0x106);
    }
}

/* func_80088490.s: put combatant `index` alone into an alternate group: its
 * own if that is empty, else the first empty one of the eight.  When all
 * eight are taken retail stores whatever its $a2 held on entry -- the
 * caller's third argument register -- so that value is modelled as a
 * parameter here rather than invented. */
void func_80088490(u8 index, u32 unused_a1, u32 retail_a2) {
    u32 group = ROW(index)[0];
    u32 pick;

    (void)unused_a1;
    if (GROUP(group + 0x10)[0] == 0) {
        pick = group;
    } else {
        u32 i;

        pick = retail_a2;
        for (i = 0; i < 8; i++) {
            if (GROUP(0x10 + i)[0] == 0) {
                pick = i;
                break;
            }
        }
    }
    ROW(index)[0] = (u8)pick;
    ROW(index)[1] = 0;
    GROUP(ROW(index)[0] + 0x10)[1] = 1;
    GROUP(ROW(index)[0] + 0x10)[0] = 1;
    *(u16*)(ROW(index) + 0xA) = *(u16*)(D_800D3364 + (u32)ROW(index)[0] * 8u + 0x100);
    *(u16*)(ROW(index) + 0xC) = *(u16*)(D_800D3364 + (u32)ROW(index)[0] * 8u + 0x102);
}

/* func_80087A38.s */
void func_80087A38(u8 index) {
    u8 *row;
    u32 r;
    u8 t;

    func_80085D34();
    func_800879A8(index, D_800C3EAC[(u32)index * 64u + 0x3C]);
    D_800D366C = 0;
    r = func_800877E0(index, D_800C3EAC[(u32)index * 64u + 0x3C]);
    t = func_80080AE4(index);
    row = D_800C3EAC + (u32)index * 64u;
    func_800B89FC(r, index, row[0x3C], t);
    D_800D366C = 1;
    D_800C3E18 = 0;
}

/* func_80087AF0.s: returns 1 when the action ends the turn sequence. */
u32 func_80087AF0(u32 actor, u8 column) {
    u8 a = (u8)actor;
    u32 done = 0;
    u8 *ctx;
    u32 n;
    u8 s3;

    if (D_800C3E18 == 0) {
        if (D_800D32A1[(u32)a * 8u] != 0)
            func_800881B8(a, D_800C3EAC[(u32)a * 64u + 0x3C]);
        else if (D_800D2D24[a] != 4)
            func_80087EDC(a, D_800C3EAC[(u32)a * 64u + 0x3C]);
        D_800C3E18 = 1;
    }
    func_80085388();
    if (D_800D32A1[(u32)a * 8u] != 0) {
        D_800C3EAC[0x2DC]++;
    } else {
        ctx = D_800C3EAC;
        n = ctx[0x2DC];
        if (n < 8) {
            ctx[0x2DC] = D_800C34B3[n * 3u + column];
        } else {
            u16 m = *(u16*)(g_GameState + 0x16C0 + (u32)D_800D2D24[a] * 32u);

            if ((u16)func_80089C6C(m, (u8)(ctx[0x2DC] - 8)) != 0)
                done = 1;
            else
                D_800C3EAC[0x2DC] = 7;
        }
    }
    if (*(u16*)(D_800CCD1C + (u32)D_800C3EAC[(u32)a * 64u + 0x3C] * 0x170u) & 0x800) {
        D_800C400B[(u32)D_800C3EAC[0x2DA] * 72u] = (u8)actor;
        D_800C402F[(u32)D_800C3EAC[0x2DA] * 72u] = 0xF3;
        {
            u16 v = func_80089C08(D_800C3EAC[(u32)a * 64u + 0x3C]);

            *(u16*)(D_800C4022 + (u32)D_800C3EAC[0x2DA] * 72u) = v;
        }
        D_800C3EAC[0x2DA]++;
    }
    {
        u16 v = func_80089C08(D_800C3EAC[(u32)a * 64u + 0x3C]);

        func_80085CCC(a, v, (u16)(D_800C3EAC[0x2DC] - 1));
    }
    s3 = D_800C3EAC[0x2DA];
    D_800C400B[(u32)s3 * 72u] = (u8)actor;
    *(u16*)(D_800C3FFE + (u32)D_800C3EAC[0x2DA] * 72u) = D_800D2C94;
    D_800C402F[(u32)D_800C3EAC[0x2DA] * 72u] = *(u8*)&D_800D2C98;
    D_800C3EAC[0x2DA]++;
    func_80079840(a, D_800C3EAC[(u32)a * 64u + 0x3C]);
    if (D_800C3EAC[(u32)a * 64u + 0x3C] >= 3) {
        if (func_80079AB0() != 0)
            done = 1;
    }
    func_80085C88(s3);
    return done;
}
#undef ROW
#undef GROUP
#endif /* XENO_PC_PORT */


/* func_800885D0.s */
u8 func_800885D0(u8 index) {
    /* Retail: lbu at D_800C3EB4 + 28*index, +0x18, <<2, then lbu at
     * D_800D301C + that. Both are byte loads relative to the symbol's
     * ADDRESS.
     *
     * Two bugs the differential prover caught. `(u32)D_800D301C` truncated a
     * 64-bit host pointer to 32 bits and then used it as an address, which is
     * a 32-bit-host assumption and faulted here. And D_800C3EB4 is declared
     * `u8 D_800C3EB4[][28]` in main35.c, so the shared guest-RAM alias is
     * `u8 (*)[28]`; `D_800C3EB4[index * 28]` therefore scaled by 28 twice.
     * Index both through a plain byte pointer, which is what retail does and
     * what this TU's own `extern u8 D_800C3EB4[]` means. */
    const u8* rows = (const u8*)D_800C3EB4;
    const u8* table = (const u8*)D_800D301C;
    u32 t = (u32)rows[index * 28] + 0x18;

    return table[t << 2];
}
