/* Retail TU part 2 of main35.c (see the BATTLE_TU_PART note there): the
 * still-assembly run below, then that file's part-2 C bodies. */
#define BATTLE_TU_PART 2
#include "common.h"
#ifndef XENO_PC_PORT
u32 func_80084854(u32 arg0, u32 arg1);
u16 func_80089C08(u8);
void func_80093B08(s32);
void func_800BC404(u32);
void func_800BCD98(u32);
extern s32 D_800C3E24;
extern struct BattleCommandContext * D_800C3EAC;
extern u8 D_800D2C34;
void func_8008189C(s32 arg0);
extern u8 D_800D3014, D_800C3E29;
extern void func_8008AA74(u32);
#endif

#ifndef XENO_PC_PORT
extern u8 func_8008BED8(u32);
extern void func_8008B908(u32);


extern void func_8009AA44(u32);


extern void func_8007FD38(u8);
extern u8 func_8008B478(u32);
extern void func_8008ADD0(u32);




extern u8 D_800D2D24[];
extern u8 D_800C400B[];
extern u8 D_800C402F[];
extern u8 D_800C4022[];
extern u8 D_800C3FFE[];
extern void func_8007FCE8(void);
extern void func_8007FDEC(void);
extern void func_80077980(void);
extern void func_8009413C(u8 idx, u8 flag);
/* func_800819A4 now lives in mainasm_11828.c (retail-only split). */
extern void func_800819A4(u8 actor);

extern u8 func_8008C4A8(u32);

/* func_800820A4.s, byte-exact WITH THE CROSS-JUMP CONSTRUCT:
 * command-ring step on D_800D3014 (the func_8008115C family).
 * Events 4/6/7: reject when the actor row's +0x3C target is 0xFF;
 * otherwise context +0x2E1 = 1 if func_8008C4A8(actor) is nonzero, else
 * state 7 and +0x2E2 = 0xFF.  Event 1 rejects on +0x32, else state 8;
 * event 2 checks +0x2C (state 9), then moves to state 3; event 3 checks
 * +0x2A (state 0xA), then rejects or moves to state 4 by +0x30; event 0
 * rejects on +0x2E, else state 1.
 * Construct (see func_800830A8): the first of its identical "+0x2F6 = 1,
 * reject" blocks stores through an s32 local.  Not a natural match; count it apart. */
void func_800820A4(u32 arg0) {
    u32 actor;
    switch (D_800D3014) {
    case 4:
    case 6:
    case 7:
        actor = arg0 & 0xFF;
        if (((u8 *)D_800C3EAC + (actor << 6))[0x3C] == 0xFF) {
            arg0 = 0x4F;
            goto reject;
        }
        if (func_8008C4A8(actor)) {
            ((u8 *)D_800C3EAC)[0x2E1] = 1;
            return;
        }
        ((u8 *)D_800C3EAC)[0x2DD] = 7;
        ((u8 *)D_800C3EAC)[0x2E2] = 0xFF;
        return;
    case 1: {
        u8 *row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;

        if (*(u16 *)(row + 0x32)) {
            arg0 = 0x4F;
            goto reject;
        }
    }
        ((u8 *)D_800C3EAC)[0x2DD] = 8;
        return;
    case 2: {
        u8 *row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;

        if (!*(u16 *)(row + 0x2C)) {
            ((u8 *)D_800C3EAC)[0x2DD] = 9;
            return;
        }
        if (((u8 *)D_800C3EAC)[0x2F6] && D_800C3E29 == 2) {
            ((u8 *)D_800C3EAC)[0x2DD] = 3;
            ((u8 *)D_800C3EAC)[0x2F6] = 0;
            return;
        }
        arg0 = 0x4F;
        {
            s32 on = 1;

            ((u8 *)D_800C3EAC)[0x2F6] = on;
        }
        goto reject;
    }
    case 3: {
        u8 *row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;

        if (!*(u16 *)(row + 0x2A)) {
            ((u8 *)D_800C3EAC)[0x2DD] = 0xA;
            return;
        }
        if (((u8 *)D_800C3EAC)[0x2F6] && D_800C3E29 == 3) {
            if (*(u16 *)(row + 0x30)) func_8008AA74(0x4F);
            else ((u8 *)D_800C3EAC)[0x2DD] = 4;
            ((u8 *)D_800C3EAC)[0x2F6] = 0;
            return;
        }
        arg0 = 0x4F;
        ((u8 *)D_800C3EAC)[0x2F6] = 1;
        goto reject;
    }
    case 0: {
        u8 *row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;

        if (*(u16 *)(row + 0x2E)) {
            arg0 = 0x4F;
            goto reject;
        }
    }
        ((u8 *)D_800C3EAC)[0x2DD] = 1;
        return;
    default:
        return;
    }
reject:
    func_8008AA74(arg0);
}

extern u8 func_8008BED8(u32);
extern void func_8008B908(u32);


/* func_800822C4.s, byte-exact WITH THE CROSS-JUMP CONSTRUCT (twice):
 * command-ring step on D_800D3014 (the func_8008115C family).
 * Events 4/6/7: func_8008B908(actor) if func_8008BED8(actor) is nonzero, else
 * state 8.  Event 0 checks +0x26 (state 7), then rejects or moves to state 1
 * by +0x2E; event 2 checks +0x2C (state 9), then moves to state 3; event 3
 * checks +0x2A (state 0xA), then rejects or moves to state 4 by +0x30;
 * event 1 rejects on +0x32, else state 2.
 * Construct (see func_800830A8): the first of its identical "+0x2F6 = 1,
 * reject" blocks stores through an s32 local, the second through an
 * s16 local.  Not a natural match; count it apart. */
void func_800822C4(u32 arg0) {
    u32 actor;
    switch (D_800D3014) {
    case 4:
    case 6:
    case 7:
        actor = arg0 & 0xFF;
        if (func_8008BED8(actor)) {
            func_8008B908(actor);
            return;
        }
        ((u8 *)D_800C3EAC)[0x2DD] = 8;
        return;
    case 0: {
        u8 *row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;

        if (!*(u16 *)(row + 0x26)) {
            ((u8 *)D_800C3EAC)[0x2DD] = 7;
            return;
        }
        if (((u8 *)D_800C3EAC)[0x2F6] && D_800C3E29 == 0) {
            if (*(u16 *)(row + 0x2E)) func_8008AA74(0x4F);
            else ((u8 *)D_800C3EAC)[0x2DD] = 1;
            ((u8 *)D_800C3EAC)[0x2F6] = 0;
            return;
        }
        arg0 = 0x4F;
        {
            s32 on = 1;

            ((u8 *)D_800C3EAC)[0x2F6] = on;
        }
        goto reject;
    }
    case 2: {
        u8 *row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;

        if (!*(u16 *)(row + 0x2C)) {
            ((u8 *)D_800C3EAC)[0x2DD] = 9;
            return;
        }
        if (((u8 *)D_800C3EAC)[0x2F6] && D_800C3E29 == 2) {
            ((u8 *)D_800C3EAC)[0x2DD] = 3;
            ((u8 *)D_800C3EAC)[0x2F6] = 0;
            return;
        }
        arg0 = 0x4F;
        {
            s16 on = 1;

            ((u8 *)D_800C3EAC)[0x2F6] = on;
        }
        goto reject;
    }
    case 3: {
        u8 *row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;

        if (!*(u16 *)(row + 0x2A)) {
            ((u8 *)D_800C3EAC)[0x2DD] = 0xA;
            return;
        }
        if (((u8 *)D_800C3EAC)[0x2F6] && D_800C3E29 == 3) {
            if (*(u16 *)(row + 0x30)) func_8008AA74(0x4F);
            else ((u8 *)D_800C3EAC)[0x2DD] = 4;
            ((u8 *)D_800C3EAC)[0x2F6] = 0;
            return;
        }
        arg0 = 0x4F;
        ((u8 *)D_800C3EAC)[0x2F6] = 1;
        goto reject;
    }
    case 1: {
        u8 *row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;

        if (*(u16 *)(row + 0x32)) {
            arg0 = 0x4F;
            goto reject;
        }
    }
        ((u8 *)D_800C3EAC)[0x2DD] = 2;
        return;
    default:
        return;
    }
reject:
    func_8008AA74(arg0);
}

extern u8 D_800C48EA;
extern u32 func_8009A9D0(u8);

/* func_80082504.s, byte-exact:
 * command-ring step on D_800D3014 (the func_8008115C family).
 * Events 4/6/7: D_800C48EA = 0x40 if func_8009A9D0(context +0x2D3) is
 * nonzero, then +0x2DE = 1.  Event 0 checks +0x26 (state 7), then rejects
 * or moves to state 1 by +0x2E, else sets +0x2F6 and rejects; event 1
 * rejects on +0x32, else state 8; event 3 checks +0x2A (state 0xA), then
 * rejects or moves to state 4 by +0x30; event 2 -> state 3.  Direct
 * func_8008AA74(0x4F) calls, as in func_80083580, so retail's merged tails
 * need no construct. */
void func_80082504(u32 arg0) {
    switch (D_800D3014) {
    case 4:
    case 6:
    case 7:
        if (func_8009A9D0(((u8 *)D_800C3EAC)[0x2D3])) {
            D_800C48EA = 0x40;
        }
        ((u8 *)D_800C3EAC)[0x2DE] = 1;
        return;
    case 0: {
        u8 *row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;

        if (!*(u16 *)(row + 0x26)) {
            ((u8 *)D_800C3EAC)[0x2DD] = 7;
            return;
        }
        if (((u8 *)D_800C3EAC)[0x2F6] && D_800C3E29 == 0) {
            if (*(u16 *)(row + 0x2E)) func_8008AA74(0x4F);
            else ((u8 *)D_800C3EAC)[0x2DD] = 1;
            ((u8 *)D_800C3EAC)[0x2F6] = 0;
            return;
        }
        ((u8 *)D_800C3EAC)[0x2F6] = 1;
        func_8008AA74(0x4F);
        return;
    }
    case 1: {
        u8 *row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;

        if (*(u16 *)(row + 0x32)) {
            func_8008AA74(0x4F);
            return;
        }
    }
        ((u8 *)D_800C3EAC)[0x2DD] = 8;
        return;
    case 3: {
        u8 *row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;

        if (!*(u16 *)(row + 0x2A)) {
            ((u8 *)D_800C3EAC)[0x2DD] = 0xA;
            return;
        }
        if (((u8 *)D_800C3EAC)[0x2F6] && D_800C3E29 == 3) {
            if (*(u16 *)(row + 0x30)) func_8008AA74(0x4F);
            else ((u8 *)D_800C3EAC)[0x2DD] = 4;
            ((u8 *)D_800C3EAC)[0x2F6] = 0;
            return;
        }
        ((u8 *)D_800C3EAC)[0x2F6] = 1;
        func_8008AA74(0x4F);
        return;
    }
    case 2:
        ((u8 *)D_800C3EAC)[0x2DD] = 3;
        return;
    }
}

/* func_800826CC.s, byte-exact: run func_80088490, func_8009AEFC and
 * func_800BAF48 on actor, set bit 7 of its D_800CCE42 record byte, run
 * func_800883AC, and set its D_800D32A0 row byte 1 to 2.  Unless its
 * D_800D2D24 kind is 7, set byte +0x853D of its 0x1E4-byte D_800C3EA4
 * record to 2.  Then set its D_800C3EB8 row byte and
 * D_800C3EAC[0x2EB + actor] to 1, and flag g_GameState[0x22B1 + i] for
 * each of the three +0x1D34 entries equal to its kind (only while
 * D_80059179 is clear).  Codegen (gcc 2.7.2): i = 0 is written before the
 * kind test, so reorg puts it in the branch delay slot (as the for-init
 * the slot is filled from the fall-through).  Computing the record
 * pointer by itself, p = D_800C3EA4 + actor * 0x1E4 and then p[0x853D],
 * gives retail's base-first add.  Folding 0x853D into the index comes out
 * offset-first: written directly, through a register copy of the base,
 * or in five other spellings.  The D_800D32A0 row view and the `a` copy
 * of actor were already in the 10-13 draft. */
extern void func_80088490(u32);
extern void func_8009AEFC(u32);
extern void func_800BAF48(u32);
extern void func_800883AC(u32);
extern u8 D_800CCE42[][0x170];
extern u8 D_800D32A0__r[][8] asm("D_800D32A0");
extern u8 D_800D2D24[];
extern u8* D_800C3EA4;
extern u8 D_800C3EB8[][28];
extern u8 D_80059179;
extern u8 g_GameState[];

void func_800826CC(u32 arg0) {
    u32 actor = arg0 & 0xFF;
    s32 i;
    u32 a;

    func_80088490(actor);
    func_8009AEFC(actor);
    func_800BAF48(actor);
    D_800CCE42[actor][0] |= 0x80;
    func_800883AC(actor);
    D_800D32A0__r[actor][1] = 2;
    i = 0;
    if (D_800D2D24[actor] != 7) {
        u8* p = D_800C3EA4 + actor * 0x1E4;

        p[0x853D] = 2;
    }
    a = actor;
    D_800C3EB8[a][0] = 1;
    *((u8 *)D_800C3EAC + a + 0x2EB) = 1;
    for (; i < 3; i++) {
        if (g_GameState[0x1D34 + i] == D_800D2D24[a] && D_80059179 == 0) {
            g_GameState[0x22B1 + i] = 1;
        }
    }
}
extern void func_800826CC(u32);

/* func_80082820.s, byte-exact WITH THE CROSS-JUMP CONSTRUCT:
 * command-ring step on D_800D3014 (the func_8008115C family).
 * Events 4/6/7: func_800826CC(actor), then context +0x2DE = 1.  Event 0
 * checks +0x26 (state 7), then rejects or moves to state 1 by +0x2E; event
 * 1 rejects on +0x32, else state 8; event 2 checks +0x2C (state 9), then
 * moves to state 3; event 3 rejects on +0x30, else state 4.
 * Construct (see func_800830A8): the first of its identical "+0x2F6 = 1,
 * reject" blocks stores through an s32 local.  Not a natural match; count it apart. */
void func_80082820(u32 arg0) {
    switch (D_800D3014) {
    case 4:
    case 6:
    case 7:
        func_800826CC(arg0 & 0xFF);
        ((u8 *)D_800C3EAC)[0x2DE] = 1;
        return;
    case 0: {
        u8 *row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;

        if (!*(u16 *)(row + 0x26)) {
            ((u8 *)D_800C3EAC)[0x2DD] = 7;
            return;
        }
        if (((u8 *)D_800C3EAC)[0x2F6] && D_800C3E29 == 0) {
            if (*(u16 *)(row + 0x2E)) func_8008AA74(0x4F);
            else ((u8 *)D_800C3EAC)[0x2DD] = 1;
            ((u8 *)D_800C3EAC)[0x2F6] = 0;
            return;
        }
        arg0 = 0x4F;
        {
            s32 on = 1;

            ((u8 *)D_800C3EAC)[0x2F6] = on;
        }
        goto reject;
    }
    case 1: {
        u8 *row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;

        if (*(u16 *)(row + 0x32)) {
            arg0 = 0x4F;
            goto reject;
        }
    }
        ((u8 *)D_800C3EAC)[0x2DD] = 8;
        return;
    case 2: {
        u8 *row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;

        if (!*(u16 *)(row + 0x2C)) {
            ((u8 *)D_800C3EAC)[0x2DD] = 9;
            return;
        }
        if (((u8 *)D_800C3EAC)[0x2F6] && D_800C3E29 == 2) {
            ((u8 *)D_800C3EAC)[0x2DD] = 3;
            ((u8 *)D_800C3EAC)[0x2F6] = 0;
            return;
        }
        arg0 = 0x4F;
        ((u8 *)D_800C3EAC)[0x2F6] = 1;
        goto reject;
    }
    case 3: {
        u8 *row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;

        if (*(u16 *)(row + 0x30)) {
            arg0 = 0x4F;
            goto reject;
        }
    }
        ((u8 *)D_800C3EAC)[0x2DD] = 4;
        return;
    default:
        return;
    }
reject:
    func_8008AA74(arg0);
}

extern u8 D_800D366C;
extern void func_80087A38(u32);
extern void func_80084A7C(u32);
extern void func_80077698(void);

/* func_800829F4.s, byte-exact: a command-ring step on D_800D3014 (the
 * func_8008115C family).  Events 4/6/7: the reject call func_8008AA74(0x4F)
 * when the actor row's +0x3C target is 0xFF, else func_80087A38(actor),
 * func_80084A7C(actor), state 0x19, func_80077698() and D_800D366C = 0.
 * Events 1 and 0 reject when the row's +0x34/+0x36 halfword is set, else
 * state 0x11/0x15; event 2 -> state 0x12; event 3 follows
 * func_8008115C's case 3 with +0x28/+0x24 and states 0x13/0x18.  The
 * switch table is this object's .rodata (jtbl_80070110).  The target row
 * is addressed with `actor << 6`: as `actor * 64` the base and offset
 * registers swap (3/111). */
void func_800829F4(u32 arg0) {
    u32 actor;
    switch (D_800D3014) {
    case 4:
    case 6:
    case 7:
        actor = arg0 & 0xFF;
        if (((u8 *)D_800C3EAC + (actor << 6))[0x3C] == 0xFF) {
            arg0 = 0x4F;
            goto reject;
        }
        func_80087A38(actor);
        func_80084A7C(actor);
        ((u8 *)D_800C3EAC)[0x2DD] = 0x19;
        func_80077698();
        D_800D366C = 0;
        return;
    case 1: {
        u8 *row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;

        if (*(u16 *)(row + 0x34)) {
            arg0 = 0x4F;
            goto reject;
        }
    }
        ((u8 *)D_800C3EAC)[0x2DD] = 0x11;
        return;
    case 2:
        ((u8 *)D_800C3EAC)[0x2DD] = 0x12;
        return;
    case 3: {
        u8 *row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;

        if (!*(u16 *)(row + 0x28)) {
            ((u8 *)D_800C3EAC)[0x2DD] = 0x13;
            return;
        }
        if (((u8 *)D_800C3EAC)[0x2F6] && D_800C3E29 == 3) {
            if (*(u16 *)(row + 0x24)) func_8008AA74(0x4F);
            else ((u8 *)D_800C3EAC)[0x2DD] = 0x18;
            ((u8 *)D_800C3EAC)[0x2F6] = 0;
            return;
        }
        arg0 = 0x4F;
        ((u8 *)D_800C3EAC)[0x2F6] = 1;
        goto reject;
    }
    case 0: {
        u8 *row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;

        if (*(u16 *)(row + 0x36)) {
            arg0 = 0x4F;
            goto reject;
        }
    }
        ((u8 *)D_800C3EAC)[0x2DD] = 0x15;
        return;
    default:
        return;
    }
reject:
    func_8008AA74(arg0);
}

extern u8 func_8008BED8(u32);
extern void func_8008B908(u32);

/* func_80082BB0.s, byte-exact: a command-ring step on D_800D3014 (the
 * func_8008115C family).  Events 4/6/7: func_8008B908(actor) if
 * func_8008BED8(actor) is nonzero, else state 0x11.  Events 0 and 1 make
 * the reject call func_8008AA74(0x4F) when the row's +0x2E/+0x34 halfword
 * is set, else state 0x10/0x16; event 2 -> state 0x12; event 3 follows
 * func_8008115C's case 3 with +0x28/+0x24 and states 0x13/0x18.  The
 * switch table is this object's .rodata (jtbl_80070130).  Each event
 * keeps its own row local, as in func_80082D4C. */
void func_80082BB0(u32 arg0) {
    u32 actor;
    switch (D_800D3014) {
    case 4:
    case 6:
    case 7:
        actor = arg0 & 0xFF;
        if (func_8008BED8(actor)) {
            func_8008B908(actor);
            return;
        }
        ((u8 *)D_800C3EAC)[0x2DD] = 0x11;
        return;
    case 0: {
        u8 *row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;

        if (*(u16 *)(row + 0x2E)) {
            arg0 = 0x4F;
            goto reject;
        }
    }
        ((u8 *)D_800C3EAC)[0x2DD] = 0x10;
        return;
    case 2:
        ((u8 *)D_800C3EAC)[0x2DD] = 0x12;
        return;
    case 3: {
        u8 *row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;

        if (!*(u16 *)(row + 0x28)) {
            ((u8 *)D_800C3EAC)[0x2DD] = 0x13;
            return;
        }
        if (((u8 *)D_800C3EAC)[0x2F6] && D_800C3E29 == 3) {
            if (*(u16 *)(row + 0x24)) func_8008AA74(0x4F);
            else ((u8 *)D_800C3EAC)[0x2DD] = 0x18;
            ((u8 *)D_800C3EAC)[0x2F6] = 0;
            return;
        }
        arg0 = 0x4F;
        ((u8 *)D_800C3EAC)[0x2F6] = 1;
        goto reject;
    }
    case 1: {
        u8 *row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;

        if (*(u16 *)(row + 0x34)) {
            arg0 = 0x4F;
            goto reject;
        }
    }
        ((u8 *)D_800C3EAC)[0x2DD] = 0x16;
        return;
    default:
        return;
    }
reject:
    func_8008AA74(arg0);
}

extern void func_8009AA44(u32);
extern volatile u16 D_800CCDC4[][184];
extern u16 D_800CCDC6[][184];
extern u16 D_800D2C32;

/* func_80082D4C.s, byte-exact WITH A VOLATILE VIEW (see below): a
 * command-ring step on D_800D3014 (the func_8008115C family).  Events
 * 4/6/7: func_8009AA44(actor), add D_800D2C32 to the actor's u16 at
 * D_800CCDC4 (0x170 stride), clamp it to the u16 at D_800CCDC6, then set
 * context +0x2EB + actor to 1, +0x2EA to 0 and +0x2DE to 1.  Events 0, 1
 * and 2 make the reject call func_8008AA74(0x4F) when the actor row's
 * +0x2E/+0x34/+0x2C halfword is set and otherwise move to state
 * 0x10/0x11/0x17.  Event 3 follows func_8008115C's case 3 with +0x28/+0x24
 * and states 0x13/0x18.  The switch table is this object's .rodata
 * (jtbl_80070150, placed by config/battle.yaml).
 * Retail re-reads the sum from memory before the clamp; with a plain u16
 * array CSE forwards the stored value instead (548 vs 560 bytes), and
 * every non-volatile spelling tried keeps that or reorders the loads.
 * D_800CCDC4 is therefore declared volatile here -- a construct, not a
 * natural match; count it apart and revisit.  Each event keeps its own
 * row local: one shared local stays live across event 3's branches and
 * moves every row into $a0. */
void func_80082D4C(u32 arg0) {
    u32 actor;
    switch (D_800D3014) {
    case 4:
    case 6:
    case 7:
        actor = arg0 & 0xFF;
        func_8009AA44(actor);
        D_800CCDC4[actor][0] += D_800D2C32;
        if (D_800CCDC4[actor][0] > D_800CCDC6[actor][0]) {
            D_800CCDC4[actor][0] = D_800CCDC6[actor][0];
        }
        *((u8 *)D_800C3EAC + actor + 0x2EB) = 1;
        ((u8 *)D_800C3EAC)[0x2EA] = 0;
        ((u8 *)D_800C3EAC)[0x2DE] = 1;
        return;
    case 0:
        {
            u8 *row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;

            if (*(u16 *)(row + 0x2E)) {
                arg0 = 0x4F;
                goto reject;
            }
        }
        ((u8 *)D_800C3EAC)[0x2DD] = 0x10;
        return;
    case 1:
        {
            u8 *row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;

            if (*(u16 *)(row + 0x34)) {
                arg0 = 0x4F;
                goto reject;
            }
        }
        ((u8 *)D_800C3EAC)[0x2DD] = 0x11;
        return;
    case 3: {
        u8 *row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;

        if (!*(u16 *)(row + 0x28)) {
            ((u8 *)D_800C3EAC)[0x2DD] = 0x13;
            return;
        }
        if (((u8 *)D_800C3EAC)[0x2F6] && D_800C3E29 == 3) {
            if (*(u16 *)(row + 0x24)) func_8008AA74(0x4F);
            else ((u8 *)D_800C3EAC)[0x2DD] = 0x18;
            ((u8 *)D_800C3EAC)[0x2F6] = 0;
            return;
        }
        arg0 = 0x4F;
        ((u8 *)D_800C3EAC)[0x2F6] = 1;
        goto reject;
    }
    case 2:
        {
            u8 *row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;

            if (*(u16 *)(row + 0x2C)) {
                arg0 = 0x4F;
                goto reject;
            }
        }
        ((u8 *)D_800C3EAC)[0x2DD] = 0x17;
        return;
    default:
        return;
    }
reject:
    func_8008AA74(arg0);
}

extern void func_8007FD38(u8);
extern u8 func_8008B478(u32);
extern void func_8008ADD0(u32);

/* func_80082F7C.s, byte-exact: a command-ring step on D_800D3014 (the
 * main35_p1 func_8008115C family).  Events 4/6/7: func_8007FD38(actor), then
 * func_8008ADD0(actor) if func_8008B478(actor) accepts, else state 0x13;
 * events 0, 1 and 3 make the reject call func_8008AA74(0x4F) when the actor
 * row's +0x2E/+0x34/+0x24 halfword is set and otherwise move to state
 * 0x10/0x11/0x18;
 * event 2 moves to state 0x12.  The switch table is this object's .rodata
 * (jtbl_80070170, placed by config/battle.yaml).  Each test goes through a
 * `row` local: indexing the context directly puts the row offset first in
 * the address add. */
void func_80082F7C(u32 arg0) {
    u32 actor;
    u8 *row;
    switch (D_800D3014) {
    case 4:
    case 6:
    case 7:
        actor = arg0 & 0xFF;
        func_8007FD38(actor);
        if (func_8008B478(actor)) {
            func_8008ADD0(actor);
            return;
        }
        ((u8 *)D_800C3EAC)[0x2DD] = 0x13;
        return;
    case 0:
        row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;
        if (*(u16 *)(row + 0x2E)) {
            arg0 = 0x4F;
            goto reject;
        }
        ((u8 *)D_800C3EAC)[0x2DD] = 0x10;
        return;
    case 1:
        row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;
        if (*(u16 *)(row + 0x34)) {
            arg0 = 0x4F;
            goto reject;
        }
        ((u8 *)D_800C3EAC)[0x2DD] = 0x11;
        return;
    case 2:
        ((u8 *)D_800C3EAC)[0x2DD] = 0x12;
        return;
    case 3:
        row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;
        if (*(u16 *)(row + 0x24)) {
            arg0 = 0x4F;
            goto reject;
        }
        ((u8 *)D_800C3EAC)[0x2DD] = 0x18;
        return;
    default:
        return;
    }
reject:
    func_8008AA74(arg0);
}
extern u16 D_800CCD64[][184], D_800CCD6C[][184], D_800CCE08[][184], D_800CCE0C[][184];

/* func_800830A8.s, byte-exact WITH THE FAMILY'S CROSS-JUMP CONSTRUCT (see
 * below): a command-ring step on D_800D3014 (the func_8008115C family).
 * Events 4/6/7 toggle the actor's 0x8000 status bit: when the u16 at
 * D_800CCE0C (0x170 stride) has it, clear it there and at D_800CCD6C;
 * otherwise clear bit 0x20 at D_800CCE08 and bit 0x1000 at D_800CCD64 and
 * set 0x8000 at D_800CCE0C and D_800CCD6C; then context +0x2DE = 1.
 * Events 1 and 0 make the reject call func_8008AA74(0x4F) when the row's
 * +0x34/+0x2E halfword is set, else state 0x16/0x10.  Event 2 checks
 * +0x2C (state 0x17), then with +0x2F6 set and D_800C3E29 == 2 moves to
 * state 0x12; event 3 follows func_8008115C's case 3 with +0x24/+0x28 and
 * states 0x18/0x13.  The switch table is this object's .rodata
 * (jtbl_80070190).  The four status arrays are separate symbols: retail
 * schedules their loads across each other's stores.
 * Construct: retail keeps events 2 and 3's identical "+0x2F6 = 1, reject"
 * blocks apart, but gcc's final jump pass cross-jumps them in every
 * natural spelling tried.  Storing the first one through an s32 local
 * keeps them distinct (decomp-permuter found this on func_80081318).  It
 * is not a natural match; count it apart. */
void func_800830A8(u32 arg0) {
    u32 actor;
    switch (D_800D3014) {
    case 4:
    case 6:
    case 7:
        actor = arg0 & 0xFF;
        if (D_800CCE0C[actor][0] & 0x8000) {
            D_800CCE0C[actor][0] &= 0x7FFF;
            D_800CCD6C[actor][0] &= 0x7FFF;
        } else {
            D_800CCE08[actor][0] &= 0xFFDF;
            D_800CCD64[actor][0] &= 0xEFFF;
            D_800CCE0C[actor][0] |= 0x8000;
            D_800CCD6C[actor][0] |= 0x8000;
        }
        ((u8 *)D_800C3EAC)[0x2DE] = 1;
        return;
    case 1: {
        u8 *row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;

        if (*(u16 *)(row + 0x34)) {
            arg0 = 0x4F;
            goto reject;
        }
    }
        ((u8 *)D_800C3EAC)[0x2DD] = 0x16;
        return;
    case 2: {
        u8 *row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;

        if (!*(u16 *)(row + 0x2C)) {
            ((u8 *)D_800C3EAC)[0x2DD] = 0x17;
            return;
        }
        if (((u8 *)D_800C3EAC)[0x2F6] && D_800C3E29 == 2) {
            ((u8 *)D_800C3EAC)[0x2DD] = 0x12;
            ((u8 *)D_800C3EAC)[0x2F6] = 0;
            return;
        }
        arg0 = 0x4F;
        {
            s32 on = 1;

            ((u8 *)D_800C3EAC)[0x2F6] = on;
        }
        goto reject;
    }
    case 3: {
        u8 *row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;

        if (!*(u16 *)(row + 0x24)) {
            ((u8 *)D_800C3EAC)[0x2DD] = 0x18;
            return;
        }
        if (((u8 *)D_800C3EAC)[0x2F6] && D_800C3E29 == 3) {
            if (*(u16 *)(row + 0x28)) func_8008AA74(0x4F);
            else ((u8 *)D_800C3EAC)[0x2DD] = 0x13;
            ((u8 *)D_800C3EAC)[0x2F6] = 0;
            return;
        }
        arg0 = 0x4F;
        ((u8 *)D_800C3EAC)[0x2F6] = 1;
        goto reject;
    }
    case 0: {
        u8 *row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;

        if (*(u16 *)(row + 0x2E)) {
            arg0 = 0x4F;
            goto reject;
        }
    }
        ((u8 *)D_800C3EAC)[0x2DD] = 0x10;
        return;
    default:
        return;
    }
reject:
    func_8008AA74(arg0);
}

extern u8 func_8008BED8(u32);
extern void func_8008B908(u32);

/* func_80083340.s, byte-exact WITH THE CROSS-JUMP CONSTRUCT (twice): a
 * command-ring step on D_800D3014 (the func_8008115C family).  Events
 * 4/6/7: func_8008B908(actor) if func_8008BED8(actor) is nonzero, else
 * state 0x16.  Event 0 checks +0x36 (state 0x15), then with +0x2F6 set and
 * D_800C3E29 == 0 rejects or moves to state 0x10 by +0x2E; event 2 checks
 * +0x2C (state 0x17), then moves to state 0x12; event 3 follows
 * func_8008115C's case 3 with +0x24/+0x28 and states 0x18/0x13; event 1
 * rejects when +0x34 is set, else state 0x11.  The switch table is this
 * object's .rodata (jtbl_800701B0).  Construct, as in func_800830A8:
 * events 0, 2 and 3 end in identical "+0x2F6 = 1, reject" blocks that
 * retail keeps apart; the first stores through an s32 local and the second
 * through an s16 local, so the final jump pass cannot merge any two of
 * them.  Not a natural match; count it apart. */
void func_80083340(u32 arg0) {
    u32 actor;
    switch (D_800D3014) {
    case 4:
    case 6:
    case 7:
        actor = arg0 & 0xFF;
        if (func_8008BED8(actor)) {
            func_8008B908(actor);
            return;
        }
        ((u8 *)D_800C3EAC)[0x2DD] = 0x16;
        return;
    case 0: {
        u8 *row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;

        if (!*(u16 *)(row + 0x36)) {
            ((u8 *)D_800C3EAC)[0x2DD] = 0x15;
            return;
        }
        if (((u8 *)D_800C3EAC)[0x2F6] && D_800C3E29 == 0) {
            if (*(u16 *)(row + 0x2E)) func_8008AA74(0x4F);
            else ((u8 *)D_800C3EAC)[0x2DD] = 0x10;
            ((u8 *)D_800C3EAC)[0x2F6] = 0;
            return;
        }
        arg0 = 0x4F;
        {
            s32 on = 1;

            ((u8 *)D_800C3EAC)[0x2F6] = on;
        }
        goto reject;
    }
    case 2: {
        u8 *row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;

        if (!*(u16 *)(row + 0x2C)) {
            ((u8 *)D_800C3EAC)[0x2DD] = 0x17;
            return;
        }
        if (((u8 *)D_800C3EAC)[0x2F6] && D_800C3E29 == 2) {
            ((u8 *)D_800C3EAC)[0x2DD] = 0x12;
            ((u8 *)D_800C3EAC)[0x2F6] = 0;
            return;
        }
        arg0 = 0x4F;
        {
            s16 on = 1;

            ((u8 *)D_800C3EAC)[0x2F6] = on;
        }
        goto reject;
    }
    case 3: {
        u8 *row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;

        if (!*(u16 *)(row + 0x24)) {
            ((u8 *)D_800C3EAC)[0x2DD] = 0x18;
            return;
        }
        if (((u8 *)D_800C3EAC)[0x2F6] && D_800C3E29 == 3) {
            if (*(u16 *)(row + 0x28)) func_8008AA74(0x4F);
            else ((u8 *)D_800C3EAC)[0x2DD] = 0x13;
            ((u8 *)D_800C3EAC)[0x2F6] = 0;
            return;
        }
        arg0 = 0x4F;
        ((u8 *)D_800C3EAC)[0x2F6] = 1;
        goto reject;
    }
    case 1: {
        u8 *row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;

        if (*(u16 *)(row + 0x34)) {
            arg0 = 0x4F;
            goto reject;
        }
    }
        ((u8 *)D_800C3EAC)[0x2DD] = 0x11;
        return;
    default:
        return;
    }
reject:
    func_8008AA74(arg0);
}

extern u8 D_800C48EA;
extern u32 func_8009A9D0(u8);

/* func_80083580.s, byte-exact: a command-ring step on D_800D3014 (the
 * func_8008115C family).  Events 4/6/7: D_800C48EA = 0x40 if
 * func_8009A9D0(context +0x2D3) is nonzero, then context +0x2DE = 1.
 * Event 0 checks +0x36 (state 0x15), then with +0x2F6 set and
 * D_800C3E29 == 0 rejects or moves to state 0x10 by +0x2E, else sets
 * +0x2F6 and rejects; event 1 rejects on +0x34, else state 0x16; event 3
 * is event 0 with +0x24/+0x28, states 0x18/0x13 and D_800C3E29 == 3;
 * event 2 moves to state 0x12.  Unlike its siblings this one calls
 * func_8008AA74(0x4F) directly at each reject, and retail cross-jumps
 * the identical tails (events 0 and 3 share one "+0x2F6 = 1, reject"
 * block), so no construct is needed.  The switch table is this object's
 * .rodata (jtbl_800701D0).  func_8009A9D0 (main55) is defined (void);
 * retail passes the context byte anyway. */
void func_80083580(u32 arg0) {
    switch (D_800D3014) {
    case 4:
    case 6:
    case 7:
        if (func_8009A9D0(((u8 *)D_800C3EAC)[0x2D3])) {
            D_800C48EA = 0x40;
        }
        ((u8 *)D_800C3EAC)[0x2DE] = 1;
        return;
    case 0: {
        u8 *row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;

        if (!*(u16 *)(row + 0x36)) {
            ((u8 *)D_800C3EAC)[0x2DD] = 0x15;
            return;
        }
        if (((u8 *)D_800C3EAC)[0x2F6] && D_800C3E29 == 0) {
            if (*(u16 *)(row + 0x2E)) func_8008AA74(0x4F);
            else ((u8 *)D_800C3EAC)[0x2DD] = 0x10;
            ((u8 *)D_800C3EAC)[0x2F6] = 0;
            return;
        }
        ((u8 *)D_800C3EAC)[0x2F6] = 1;
        func_8008AA74(0x4F);
        return;
    }
    case 1: {
        u8 *row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;

        if (*(u16 *)(row + 0x34)) {
            func_8008AA74(0x4F);
            return;
        }
    }
        ((u8 *)D_800C3EAC)[0x2DD] = 0x16;
        return;
    case 3: {
        u8 *row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;

        if (!*(u16 *)(row + 0x24)) {
            ((u8 *)D_800C3EAC)[0x2DD] = 0x18;
            return;
        }
        if (((u8 *)D_800C3EAC)[0x2F6] && D_800C3E29 == 3) {
            if (*(u16 *)(row + 0x28)) func_8008AA74(0x4F);
            else ((u8 *)D_800C3EAC)[0x2DD] = 0x13;
            ((u8 *)D_800C3EAC)[0x2F6] = 0;
            return;
        }
        ((u8 *)D_800C3EAC)[0x2F6] = 1;
        func_8008AA74(0x4F);
        return;
    }
    case 2:
        ((u8 *)D_800C3EAC)[0x2DD] = 0x12;
        return;
    }
}

extern void func_8007FE3C(u32);
extern u8 func_8008CFB8(u32);
extern void func_8008ADD0(u32);

/* func_80083748.s, byte-exact WITH THE CROSS-JUMP CONSTRUCT (see
 * func_800830A8): a command-ring step on D_800D3014 (the func_8008115C
 * family).  Events 4/6/7: func_8007FE3C(actor), then func_8008ADD0(actor)
 * if func_8008CFB8(actor) is nonzero, else state 0x18.  Event 0 checks
 * +0x36 (state 0x15), then with +0x2F6 set and D_800C3E29 == 0 rejects or
 * moves to state 0x10 by +0x2E; event 1 rejects on +0x34, else state
 * 0x16; event 2 checks +0x2C (state 0x17), then moves to state 0x12;
 * event 3 rejects on +0x28, else state 0x13.  Events 0 and 2 end in
 * identical "+0x2F6 = 1, reject" blocks that retail keeps apart; the
 * first stores through an s32 local.  Not a natural match; count it
 * apart.  The switch table is this object's .rodata (jtbl_800701F0).
 * func_8007FE3C (main33) is defined (void); retail passes the actor. */
void func_80083748(u32 arg0) {
    u32 actor;
    switch (D_800D3014) {
    case 4:
    case 6:
    case 7:
        actor = arg0 & 0xFF;
        func_8007FE3C(actor);
        if (func_8008CFB8(actor)) {
            func_8008ADD0(actor);
            return;
        }
        ((u8 *)D_800C3EAC)[0x2DD] = 0x18;
        return;
    case 0: {
        u8 *row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;

        if (!*(u16 *)(row + 0x36)) {
            ((u8 *)D_800C3EAC)[0x2DD] = 0x15;
            return;
        }
        if (((u8 *)D_800C3EAC)[0x2F6] && D_800C3E29 == 0) {
            if (*(u16 *)(row + 0x2E)) func_8008AA74(0x4F);
            else ((u8 *)D_800C3EAC)[0x2DD] = 0x10;
            ((u8 *)D_800C3EAC)[0x2F6] = 0;
            return;
        }
        arg0 = 0x4F;
        {
            s32 on = 1;

            ((u8 *)D_800C3EAC)[0x2F6] = on;
        }
        goto reject;
    }
    case 1: {
        u8 *row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;

        if (*(u16 *)(row + 0x34)) {
            arg0 = 0x4F;
            goto reject;
        }
    }
        ((u8 *)D_800C3EAC)[0x2DD] = 0x16;
        return;
    case 2: {
        u8 *row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;

        if (!*(u16 *)(row + 0x2C)) {
            ((u8 *)D_800C3EAC)[0x2DD] = 0x17;
            return;
        }
        if (((u8 *)D_800C3EAC)[0x2F6] && D_800C3E29 == 2) {
            ((u8 *)D_800C3EAC)[0x2DD] = 0x12;
            ((u8 *)D_800C3EAC)[0x2F6] = 0;
            return;
        }
        arg0 = 0x4F;
        ((u8 *)D_800C3EAC)[0x2F6] = 1;
        goto reject;
    }
    case 3: {
        u8 *row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;

        if (*(u16 *)(row + 0x28)) {
            arg0 = 0x4F;
            goto reject;
        }
    }
        ((u8 *)D_800C3EAC)[0x2DD] = 0x13;
        return;
    default:
        return;
    }
reject:
    func_8008AA74(arg0);
}

/* func_80083948.s, byte-exact: input step of actor `actor`'s cost-paid
 * command list (event D_800D3014, context D_800C3EAC, mode D_800D2C34).
 * D_800D366C = 0; nothing while +0x2E1 is set.  After func_8008189C:
 * event 5 closes the list (func_80080B64, +0x2E1 = 1) when +0x2E7 is set
 * outside mode 4, else clears D_800D2D28[0xA8], runs func_800B8DA4, sets
 * D_800D366C, +0x2DD = 0x10, func_80077980 and func_8009413C(actor, 1)
 * (mode 4: +0x2E7 = +0x2D6 = 0 instead); events 4/7/6 become event 5
 * with func_8008AA40(0x4F) while the actor row's +0x20/+0x1E/+0x1C is
 * set.  While +0x2E9 is clear, events 0-3 move the +0x2E8 target
 * (func_80084854) with func_8008AA40(0x4C).  D_800D2D28[0x7C + actor] =
 * 1.  With +0x2E7 set the list closes when the mode is not 4 and (mode
 * == 0 or mode - 1 < +0x2CC), or when func_80089C9C(D_800C48E8, actor
 * row +0x3C, the target) is nonzero.  Events 4/7/6 then pick entry k = 3/2/1.  In mode 4
 * the first pick (+0x2E7 == 0; events other than 4-7 take it too) bumps
 * +0x2E7 and calls func_80086F98(0xFF, actor).  Otherwise, when
 * func_80086B88(k - 1, actor) allows it and the actor's D_800CCDC4 value
 * (0x170-byte records) covers the cost (D_800D2C0A[k] with no list,
 * [mode * 3 + k] in mode 4, else [(+0x2CC + 1) * 3 + k]):
 * func_8008AA40(0x4D), D_800C4929 = 1, func_800819A4, +0x2E4++ when
 * listing, +0x2DD/+0x2E2 = 0x65/0xFF and +0x2E0 = 1 (D_800D2D28[0xCB] set)
 * or 0x19/0x19, +0x2E4 = !func_80086F98(k - 1, actor),
 * func_80087AF0(actor, k), the value drops by D_800D2C0A[+0x2DC]
 * (func_800898F0, which re-reads it), +0x2E7++; a short value plays
 * func_8008AA74(0x4F) under D_800D366C.  A set +0x2E4 closes the list.
 * Codegen: declared s32 with no return value, as func_80081B58 (v0 stays
 * live, so the event-5 branch keeps an empty delay slot); the other
 * events' first pick jumps into the k path's block (`goto first`: retail
 * reloads +0x2E7 there through the same ctx register); the cost is picked
 * before ONE `mp - cost` test (retail's three subu are reorg copies, and
 * a subtraction per branch ranks mp above ok); D_800D2C0A is a cost table
 * whose +0x2A byte is the mode (retail addresses the table from the mode
 * byte's address), and mode 4 indexes it as mode * 3 + k (cse turns that
 * into k + 12 after fold, so the 12 stays out of the displacement); the
 * func_80087AF0 argument goes through a u8 local; D_800CCDC4 is read
 * through a non-volatile view (the TU's volatile declaration belongs to
 * func_80082D4C's construct).  The only caller, the main34_q1 dispatcher,
 * ignores the result. */
extern u8* D_800D2D28;
extern u16 D_800C48E8;
extern u8 D_800C4929;
typedef struct {
    u16 cost[21];
    u8 mode;
} M35CostTable;
extern M35CostTable D_800D2C0A;
extern u16 D_800CCDC4__nv[][184] asm("D_800CCDC4");
extern void func_80080B64(u8 v);
extern void func_800B8DA4(void);
extern void func_8008AA40(u8 v);
extern u16 func_80089C9C(u16 mask, u8 target);
extern u8 func_80086B88(u8 index, u8 slot);
extern u8 func_80086F98(u8 index, u8 actor);
extern u8 func_80087AF0(u8 a, u8 b);
extern void func_800898F0(u8 actor);

s32 func_80083948(u8 actor) {
    s32 k = 0;
    u16 mp = D_800CCDC4__nv[actor][0];
    u8 ok = 0;
    u8 go;
    u16 cost;
    u8* ctx;

    D_800D366C = 0;
    if (((u8*)D_800C3EAC)[0x2E1] != 0) {
        return;
    }
    func_8008189C(actor);
    switch (D_800D3014) {
    case 5:
        go = 1;
        if (((u8*)D_800C3EAC)[0x2E7] != 0 && D_800D2C34 != 4) {
            func_80080B64(actor);
            ((u8*)D_800C3EAC)[0x2E1] = 1;
            go = 0;
        }
        if (go) {
            D_800D2D28[0xA8] = 0;
            func_800B8DA4();
            D_800D366C = 1;
            ((u8*)D_800C3EAC)[0x2DD] = 0x10;
            func_80077980();
            if (D_800D2C34 != 4) {
                func_8009413C(actor, 1);
            } else {
                ((u8*)D_800C3EAC)[0x2E7] = 0;
                ((u8*)D_800C3EAC)[0x2D6] = 0;
            }
        }
        break;
    case 4:
        if (*(u16*)((u8*)D_800C3EAC + (actor << 6) + 0x20) != 0) {
            D_800D3014 = 5;
            func_8008AA40(0x4F);
        }
        break;
    case 7:
        if (*(u16*)((u8*)D_800C3EAC + (actor << 6) + 0x1E) != 0) {
            D_800D3014 = 5;
            func_8008AA40(0x4F);
        }
        break;
    case 6:
        if (*(u16*)((u8*)D_800C3EAC + (actor << 6) + 0x1C) != 0) {
            D_800D3014 = 5;
            func_8008AA40(0x4F);
        }
        break;
    }
    if (((u8*)D_800C3EAC)[0x2E9] == 0) {
        switch (D_800D3014) {
        case 0:
            ((u8*)D_800C3EAC)[0x2E8] = func_80084854(((u8*)D_800C3EAC)[0x2E8], 0);
            func_8008AA40(0x4C);
            break;
        case 1:
            ((u8*)D_800C3EAC)[0x2E8] = func_80084854(((u8*)D_800C3EAC)[0x2E8], 1);
            func_8008AA40(0x4C);
            break;
        case 2:
            ((u8*)D_800C3EAC)[0x2E8] = func_80084854(((u8*)D_800C3EAC)[0x2E8], 2);
            func_8008AA40(0x4C);
            break;
        case 3:
            ((u8*)D_800C3EAC)[0x2E8] = func_80084854(((u8*)D_800C3EAC)[0x2E8], 3);
            func_8008AA40(0x4C);
            break;
        }
    }
    (D_800D2D28 + actor)[0x7C] = 1;
    if (((u8*)D_800C3EAC)[0x2E7] != 0) {
        if (D_800D2C34 != 4 && (D_800D2C34 == 0 || D_800D2C34 - 1 < ((u8*)D_800C3EAC)[0x2CC])) {
            func_80080B64(actor);
            ((u8*)D_800C3EAC)[0x2E1] = 1;
            return;
        }
        if (func_80089C9C(D_800C48E8, ((u8*)D_800C3EAC + (actor << 6))[0x3C]) != 0) {
            func_80080B64(actor);
            ((u8*)D_800C3EAC)[0x2E1] = 1;
            return;
        }
    }
    switch (D_800D3014) {
    case 5:
        break;
    default:
        if (D_800D2C34 != 4) {
            break;
        }
        ctx = (u8*)D_800C3EAC;
        if (ctx[0x2E7] != 0) {
            break;
        }
        goto first;
    case 4:
        k++;
    case 7:
        k++;
    case 6:
        k++;
        if (D_800D2C34 == 4) {
            ctx = (u8*)D_800C3EAC;
            if (ctx[0x2E7] == 0) {
            first:
                ctx[0x2E7]++;
                func_80086F98(0xFF, actor);
                break;
            }
        }
        if (func_80086B88(k - 1, actor)) {
            if (((u8*)D_800C3EAC)[0x2D6] == 0) {
                cost = D_800D2C0A.cost[k];
            } else if (D_800D2C0A.mode == 4) {
                cost = D_800D2C0A.cost[D_800D2C0A.mode * 3 + k];
            } else {
                cost = D_800D2C0A.cost[(((u8*)D_800C3EAC)[0x2CC] + 1) * 3 + k];
            }
            if (mp - cost >= 0) {
                ok = 1;
            }
            if (ok) {
                func_8008AA40(0x4D);
                D_800C4929 = 1;
                func_800819A4(actor);
                if (((u8*)D_800C3EAC)[0x2D6] != 0) {
                    ((u8*)D_800C3EAC)[0x2E4]++;
                }
                if (D_800D2D28[0xCB] != 0) {
                    ((u8*)D_800C3EAC)[0x2DD] = 0x65;
                    ((u8*)D_800C3EAC)[0x2E2] = 0xFF;
                    ((u8*)D_800C3EAC)[0x2E0] = 1;
                } else {
                    ((u8*)D_800C3EAC)[0x2DD] = 0x19;
                    ((u8*)D_800C3EAC)[0x2E2] = 0x19;
                }
                ((u8*)D_800C3EAC)[0x2E4] = func_80086F98(k - 1, actor) == 0;
                {
                    u8 cmd = k;

                    func_80087AF0(actor, cmd);
                }
                D_800CCDC4__nv[actor][0] -= D_800D2C0A.cost[((u8*)D_800C3EAC)[0x2DC]];
                func_800898F0(actor);
                ((u8*)D_800C3EAC)[0x2E7]++;
            } else {
                D_800D366C = 1;
                func_8008AA74(0x4F);
                D_800D366C = 0;
            }
            if (((u8*)D_800C3EAC)[0x2E4] != 0) {
                func_80080B64(actor);
                ((u8*)D_800C3EAC)[0x2E1] = 1;
            }
        }
        break;
    }
}
#endif
#include "main35.c"
