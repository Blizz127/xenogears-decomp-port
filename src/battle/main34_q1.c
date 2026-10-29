/* Retail run 1 of main34: the byte-exact C bodies on one side of a
 * retail-asm gap; see the BATTLE_SUB note in main34.c. */
#define BATTLE_TU_SUB 1
#include "common.h"
#ifndef XENO_PC_PORT
/* func_80080160.s, byte-exact under gcc 2.6.3 (main34_q1 is BattleGcc263;
 * gcc 2.7.2 is one instruction short, 2432 vs 2436 bytes).  The battle
 * command-menu driver for `actor`:
 * - reset the D_800C3EAC menu state, set the actor's D_800D2D28 slot to 3 and
 *   wait (func_800716D8) until it reads 1;
 * - build the actor's 16 command words at +0x1C of its 64-byte record from
 *   D_800CCD62 masked by D_800C3234, with the D_800D2D24 kind-7 and kind-4
 *   overrides;
 * - pick the opening menu state (+0x2DD) from the D_800D32A1 flag and +0x2E,
 *   and set the actor and target masks;
 * - run the menu loop: each frame, dispatch +0x2DD to the state handlers
 *   func_8008115C..func_80083948 (states 0x64/0x65 reset +0x2E2 first), until
 *   a command is chosen (+0x2DE), D_800C48EA is set or +0x2DB is set;
 * - on exit, add +0x2D4 to the actor's D_800D32A0 counter (capped at 0x1C)
 *   when +0x2E3 is set, run the end callbacks and mark the slot done.
 * Codegen notes:
 * - `t` is s16.  Its load from +0x3C then has a different mode from the
 *   test's re-read, so CSE keeps retail's two reads (an s32 t is one
 *   instruction short).
 * - g_GameState fields are read through asm-alias arrays at their addresses
 *   (`g_GameState+0xNNN`).  Retail has a separate %hi/%lo pair for each field,
 *   as field accesses on a struct-typed global give.
 * - D_800D32A0 is plain (2.6.3 CSE does not fold the QImode store into the
 *   later zero_extend load, so retail's clamp reload survives), and `a = actor`
 *   is set at the join after the +0x2E3 if. */
extern u8* D_800C3EAC;
extern u8* D_800D2D28;
extern u8 D_800D2D24[];
extern u8 D_800D32A0[][8];
extern u8 D_800D32A1[][8];
extern u8 D_800D32A4[];
extern u16 D_800C3234[];
extern u16 D_800C3238;
extern u16 D_800C323C;
extern u16 D_800C324E;
extern u16 D_800CCD62__r[][184] asm("D_800CCD62");
extern u8 D_800CCD64[];
extern u16 D_800D2E06[];
extern u8 g_GameState[];
extern u8 g_GameState_56B[] asm("g_GameState+0x56B");
extern u8 g_GameState_56E[] asm("g_GameState+0x56E");
extern u8 g_GameState_59C[] asm("g_GameState+0x59C");
extern u8 g_GameState_2286[] asm("g_GameState+0x2286");
extern u8 g_GameState_22B6[] asm("g_GameState+0x22B6");
extern u8 g_GameState_97C[] asm("g_GameState+0x97C");
extern u8 g_GameState_97F[] asm("g_GameState+0x97F");
extern u8 g_GameState_30C[] asm("g_GameState+0x30C");
extern u8 D_800C3E28;
extern u8 D_800C3E29;
extern u8 D_800C4928;
extern u8 D_800C4929;
extern u8 D_800C48EA;
extern u8 D_800D3014;
extern u8 D_800D366C;
extern void func_800716D8(void);
extern void func_8008AA74(u32);
extern void func_8007FEC4(void);
extern void func_8007FF14(u8 actor);
extern void func_80085E78(void);
extern void func_8009AB00(u8 index);
extern u16 func_80089C08(u8 idx);
extern void func_800BC404(u32 v);
extern void func_800BCD98(u32 v);
extern void func_8007FBE0(u8 count, u8 sel);
extern s8 func_8008D598(u8 actor, u8 state, u8 flag);
extern void func_800BAF40();
extern void func_8007FCE8(void);
extern void func_8007FDEC(void);
extern void func_800800E8(u8 index);
extern void func_8007FB70(u8 index);
extern void func_8008115C(u32);
extern void func_80081318(u32);
extern void func_80081504(u32);
extern void func_800816F8(u32);
extern s32 func_80081B58(u8);
extern void func_800820A4(u32);
extern void func_800822C4(u32);
extern void func_80082504(u32);
extern void func_80082820(u32);
extern void func_800829F4(u32);
extern void func_80082BB0(u32);
extern void func_80082D4C(u32);
extern void func_80082F7C(u32);
extern void func_800830A8(u32);
extern void func_80083340(u32);
extern void func_80083580(u32);
extern void func_80083748(u32);
extern s32 func_80083948(u8);

void func_80080160(u8 actor) {
    s32 i;
    u16 mask;
    s16 t;
    u32 a;

    D_800C3EAC[0x2EA] = 1;
    D_800C3EAC[0x2E9] = 0;
    D_800C3EAC[0x2DE] = 0;
    D_800C3EAC[0x2E0] = 0;
    D_800C3EAC[0x2E1] = 0;
    D_800C3EAC[0x2E2] = 0xFF;
    D_800C3EAC[0x2E3] = 0;
    D_800C3EAC[0x2E4] = 0;
    D_800C3EAC[0x2DC] = 0;
    D_800C3EAC[0x2D4] = D_800C3EAC[0x2D5] = D_800D32A4[actor * 8];
    (D_800D2D28 + actor)[0x90] = 3;
    D_800C3EAC[0x2E7] = 0;
    D_800C3E29 = 0xFF;
    D_800C4929 = 0;
    while ((D_800D2D28 + actor)[0x90] != 1) {
        func_800716D8();
    }
    D_800D366C = 1;
    func_8008AA74(0x5A);
    D_800D366C = 0;
    for (i = 0; i < 16; i++) {
        ((u16*)(D_800C3EAC + (actor << 6) + 0x1C))[i] = D_800CCD62__r[actor][0] & D_800C3234[i];
    }
    if (D_800D2D24[actor] == 7 && D_800D32A1[actor][0] != 0) {
        *(u16*)(D_800C3EAC + (actor << 6) + 0x36) = D_800C324E;
        *(u16*)(D_800C3EAC + (actor << 6) + 0x24) = D_800C323C;
    }
    if (D_800D2D24[actor] == 4) {
        if (D_800D32A1[actor][0] == 0) {
            if (g_GameState_2286[g_GameState_56B[0]] == 0) {
                *(u16*)(D_800C3EAC + (actor << 6) + 0x1C) = D_800C3234[0];
            }
            if (g_GameState_2286[g_GameState_56E[0]] == 0) {
                *(u16*)(D_800C3EAC + (actor << 6) + 0x20) = D_800C3238;
            }
        } else {
            if (g_GameState_22B6[g_GameState_97C[g_GameState_59C[0] * 0xA4]] == 0) {
                *(u16*)(D_800C3EAC + (actor << 6) + 0x1C) = D_800C3234[0];
            }
            if (g_GameState_22B6[g_GameState_97F[g_GameState_30C[D_800D2D24[actor] * 0xA4] * 0xA4]] == 0) {
                *(u16*)(D_800C3EAC + (actor << 6) + 0x20) = D_800C3238;
            }
        }
    }
    if (D_800D32A1[actor][0] == 0) {
        if (*(u16*)(D_800C3EAC + (actor << 6) + 0x2E) == 0) {
            D_800C3EAC[0x2DD] = 1;
            D_800C3E28 = 0;
        } else {
            D_800C3EAC[0x2DD] = 4;
            D_800C3E28 = 3;
        }
        if (*(u16*)(D_800CCD64 + actor * 0x170) & 2) {
            D_800C3EAC[0x2DD] = 2;
            D_800C3E28 = 1;
        }
        func_8007FEC4();
    } else {
        if (*(u16*)(D_800C3EAC + (actor << 6) + 0x2E) == 0) {
            D_800C3EAC[0x2DD] = 0x10;
            D_800C3E28 = 0;
        } else {
            D_800C3EAC[0x2DD] = 0x13;
            D_800C3E28 = 3;
        }
        func_8007FF14(actor);
    }
    func_80085E78();
    func_8009AB00(actor);
    mask = func_80089C08(actor);
    func_800BC404((mask | func_80089C08((D_800C3EAC + (actor << 6))[0x3C])) & 0xFFFF);
    t = (D_800C3EAC + (actor << 6))[0x3C];
    if ((D_800C3EAC + (actor << 6))[0x3C] == 0xFF) {
        t = 0;
    }
    func_800BCD98(func_80089C08(t) & 0xFFFF);
    while (D_800C3EAC[0x2DE] == 0 && D_800C48EA == 0 && D_800C3EAC[0x2DB] == 0) {
        if (D_800D32A1[actor][0] == 0) {
            func_8007FBE0(D_800C3EAC[0x2D5], D_800C3EAC[0x2D4]);
        }
        if (D_800C3EAC[0x2E2] != D_800C3EAC[0x2DD]) {
            D_800C3EAC[0x2F6] = 0;
            D_800C3EAC[0x2E2] = D_800C3EAC[0x2DD];
            D_800D2D28[0xCB] = 0;
            D_800D2D28[0xA3] = func_8008D598(actor, D_800C3EAC[0x2DD], D_800C3EAC[0x2E0]);
            D_800D2D28[0xCB] = 1;
        }
        if (D_800D3014 == 0xFF) {
            break;
        }
        func_800716D8();
        if (D_800C3EAC[0x2DB] != 0) {
            break;
        }
        if (D_800C3EAC[0x2E0] == 0) {
            D_800D366C = 1;
        }
        switch (D_800C3EAC[0x2DD]) {
        case 1:
            func_8008115C(actor);
            break;
        case 2:
            func_80081318(actor);
            break;
        case 3:
            func_80081504(actor);
            break;
        case 4:
            func_800816F8(actor);
            break;
        case 0x64:
            D_800C3EAC[0x2E2] = 0xFF;
        case 5:
            func_80081B58(actor);
            break;
        case 7:
            func_800820A4(actor);
            break;
        case 8:
            func_800822C4(actor);
            break;
        case 9:
            func_80082504(actor);
            break;
        case 10:
            func_80082820(actor);
            break;
        case 0x10:
            func_800829F4(actor);
            break;
        case 0x11:
            func_80082BB0(actor);
            break;
        case 0x12:
            func_80082D4C(actor);
            break;
        case 0x13:
            func_80082F7C(actor);
            break;
        case 0x15:
            func_800830A8(actor);
            break;
        case 0x16:
            func_80083340(actor);
            break;
        case 0x17:
            func_80083580(actor);
            break;
        case 0x18:
            func_80083748(actor);
            break;
        case 0x65:
            D_800C3EAC[0x2E2] = 0xFF;
        case 0x19:
            func_80083948(actor);
            break;
        }
    }
    D_800D2D28[0xCB] = 0;
    D_800D2D28[0xAF] = 0;
    (D_800D2D28 + actor)[0x90] = 0;
    (D_800D2D28 + actor)[0x7F] = 0;
    if (D_800C3EAC[0x2E3] != 0) {
        D_800D32A0[actor][0] += D_800C3EAC[0x2D4];
        if (D_800D32A0[actor][0] >= 0x1D) {
            D_800D32A0[actor][0] = 0x1C;
        }
    }
    a = actor;
    D_800D366C = 0;
    func_800BAF40(a, 0x100);
    if (D_800C3EAC[0x2DE] != 0 || D_800C48EA != 0) {
        func_8007FCE8();
        func_8007FDEC();
        func_800800E8(a);
        func_8007FB70(a);
    }
    D_800D2E06[actor] = D_800D2E06[actor - 11];
    (D_800D2D28 + actor)[0x7C] = 1;
    D_800C4928 = 0;
}

#endif
#include "main34.c"
