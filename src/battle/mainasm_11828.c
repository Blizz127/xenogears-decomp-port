/* battle mainasm_11828: the retail run at file offset 0x11828 (func_80081318
 * .. func_800819A4), in retail order, split off the front of main35_p2 so
 * that func_80081B58 (gcc 2.6.3 code, mainasm_12068) can sit between it and
 * main35_p2's 2.7.2 tail (func_800820A4 onward).  Bodies moved verbatim from
 * main35_p2.c.  The whole file is retail-build only; any port body for these
 * functions lives in the port's own TU. */
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

extern u8 func_8008BED8(u32);
extern void func_8008B908(u32);

/* func_80081318.s, byte-exact WITH THE CROSS-JUMP CONSTRUCT:
 * command-ring step on D_800D3014 (the func_8008115C family).
 * Events 4/6/7: func_8008B908(actor) if func_8008BED8(actor) is nonzero, else
 * state 2.  Event 0 checks +0x2E (state 1), then with +0x2F6 set and
 * D_800C3E29 == 0 rejects or moves to state 7 by +0x26; event 2 -> state 3;
 * event 3 is func_8008115C's case 3 (+0x30/+0x2A, states 4/10); event 1
 * rejects on +0x32, else state 8.
 * Construct (see func_800830A8): the first of its identical "+0x2F6 = 1,
 * reject" blocks stores through an s32 local.  Not a natural match; count it apart. */
void func_80081318(u32 arg0) {
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
        ((u8 *)D_800C3EAC)[0x2DD] = 2;
        return;
    case 0: {
        u8 *row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;

        if (!*(u16 *)(row + 0x2E)) {
            ((u8 *)D_800C3EAC)[0x2DD] = 1;
            return;
        }
        if (((u8 *)D_800C3EAC)[0x2F6] && D_800C3E29 == 0) {
            if (*(u16 *)(row + 0x26)) func_8008AA74(0x4F);
            else ((u8 *)D_800C3EAC)[0x2DD] = 7;
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
    case 2:
        ((u8 *)D_800C3EAC)[0x2DD] = 3;
        return;
    case 3: {
        u8 *row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;

        if (!*(u16 *)(row + 0x30)) {
            ((u8 *)D_800C3EAC)[0x2DD] = 4;
            return;
        }
        if (((u8 *)D_800C3EAC)[0x2F6] && D_800C3E29 == 3) {
            if (*(u16 *)(row + 0x2A)) func_8008AA74(0x4F);
            else ((u8 *)D_800C3EAC)[0x2DD] = 10;
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
        ((u8 *)D_800C3EAC)[0x2DD] = 8;
        return;
    default:
        return;
    }
reject:
    func_8008AA74(arg0);
}

extern void func_8009AA44(u32);

/* func_80081504.s, byte-exact WITH THE CROSS-JUMP CONSTRUCT:
 * command-ring step on D_800D3014 (the func_8008115C family).
 * Events 4/6/7: func_8009AA44(actor), context +0x2EA = 0, +0x2DE = 1.
 * Event 0 checks +0x2E (state 1), then with +0x2F6 set and D_800C3E29 == 0
 * rejects or moves to state 7 by +0x26; event 1 rejects on +0x32, else
 * state 2; event 3 checks +0x30 (state 4), then rejects or moves to state
 * 0xA by +0x2A; event 2 rejects on +0x2C, else state 9.
 * Construct (see func_800830A8): the first of its identical "+0x2F6 = 1,
 * reject" blocks stores through an s32 local.  Not a natural match; count it apart. */
void func_80081504(u32 arg0) {
    switch (D_800D3014) {
    case 4:
    case 6:
    case 7:
        func_8009AA44(arg0 & 0xFF);
        ((u8 *)D_800C3EAC)[0x2EA] = 0;
        ((u8 *)D_800C3EAC)[0x2DE] = 1;
        return;
    case 0: {
        u8 *row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;

        if (!*(u16 *)(row + 0x2E)) {
            ((u8 *)D_800C3EAC)[0x2DD] = 1;
            return;
        }
        if (((u8 *)D_800C3EAC)[0x2F6] && D_800C3E29 == 0) {
            if (*(u16 *)(row + 0x26)) func_8008AA74(0x4F);
            else ((u8 *)D_800C3EAC)[0x2DD] = 7;
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
        ((u8 *)D_800C3EAC)[0x2DD] = 2;
        return;
    case 3: {
        u8 *row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;

        if (!*(u16 *)(row + 0x30)) {
            ((u8 *)D_800C3EAC)[0x2DD] = 4;
            return;
        }
        if (((u8 *)D_800C3EAC)[0x2F6] && D_800C3E29 == 3) {
            if (*(u16 *)(row + 0x2A)) func_8008AA74(0x4F);
            else ((u8 *)D_800C3EAC)[0x2DD] = 0xA;
            ((u8 *)D_800C3EAC)[0x2F6] = 0;
            return;
        }
        arg0 = 0x4F;
        ((u8 *)D_800C3EAC)[0x2F6] = 1;
        goto reject;
    }
    case 2: {
        u8 *row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;

        if (*(u16 *)(row + 0x2C)) {
            arg0 = 0x4F;
            goto reject;
        }
    }
        ((u8 *)D_800C3EAC)[0x2DD] = 9;
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

/* func_800816F8.s, byte-exact:
 * command-ring step on D_800D3014 (the func_8008115C family).
 * Events 4/6/7: func_8007FD38(actor), then func_8008ADD0(actor) if
 * func_8008B478(actor) is nonzero, else state 4.  Event 0 checks +0x2E
 * (state 1), then with +0x2F6 set and D_800C3E29 == 0 rejects or moves to
 * state 7 by +0x26; event 1 rejects on +0x32, else state 2; event 2 ->
 * state 3; event 3 rejects on +0x2A, else state 0xA. */
void func_800816F8(u32 arg0) {
    u32 actor;
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
        ((u8 *)D_800C3EAC)[0x2DD] = 4;
        return;
    case 0: {
        u8 *row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;

        if (!*(u16 *)(row + 0x2E)) {
            ((u8 *)D_800C3EAC)[0x2DD] = 1;
            return;
        }
        if (((u8 *)D_800C3EAC)[0x2F6] && D_800C3E29 == 0) {
            if (*(u16 *)(row + 0x26)) func_8008AA74(0x4F);
            else ((u8 *)D_800C3EAC)[0x2DD] = 7;
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
    case 2:
        ((u8 *)D_800C3EAC)[0x2DD] = 3;
        return;
    case 3: {
        u8 *row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;

        if (*(u16 *)(row + 0x2A)) {
            arg0 = 0x4F;
            goto reject;
        }
    }
        ((u8 *)D_800C3EAC)[0x2DD] = 0xA;
        return;
    default:
        return;
    }
reject:
    func_8008AA74(arg0);
}


#ifndef XENO_PC_PORT
void func_8008189C(s32 arg0) {
    s32 var_s0;

    if (*(u8 *)((s8*)((*(void **)&D_800C3EAC)) + 0x2E9) == 0) {
        var_s0 = 0;
        func_800BC404(func_80089C08(*(u8 *)((s8*)((*(void **)&D_800C3EAC)) + 0x2E8)) & 0xFFFF);
        func_800BCD98(func_80089C08(*(u8 *)((s8*)((*(void **)&D_800C3EAC)) + 0x2E8)) & 0xFFFF);
        do {
            if ((func_80084854(*(u8 *)((s8*)((*(void **)&D_800C3EAC)) + 0x2E8), var_s0 & 0xFF) & 0xFF) != *(u8 *)((s8*)((*(void **)&D_800C3EAC)) + 0x2E8)) {
                *(s8 *)((s8*)((D_800C3E24 + var_s0)) + 0xE6) = 1;
            } else {
                *(s8 *)((s8*)((D_800C3E24 + var_s0)) + 0xE6) = 0;
            }
            var_s0 += 1;
        } while (var_s0 < 4);
        if (D_800D2C34 != 4) {
            func_80093B08(arg0 & 0xFF);
        }
    }
}
#endif

extern u8 D_800D2D24[];
extern u8 D_800C400B[];
extern u8 D_800C402F[];
extern u8 D_800C4022[];
extern u8 D_800C3FFE[];
extern void func_8007FCE8(void);
extern void func_8007FDEC(void);
extern void func_80077980(void);
extern void func_8009413C(u8 idx, u8 flag);

/* func_800819A4.s, byte-exact: once per command (context +0x2E9 latch):
 * func_800BCD98(0); the actor row's +0x3C target = context +0x2E8; run
 * func_8007FCE8, func_8007FDEC and func_80077980; func_800BC404 of the
 * actor's and the target's func_80089C08 masks.  Unless D_800D2D24[actor]
 * is 4, queue command 0xFD in the +0x2DA row (D_800C400B = actor,
 * D_800C4022 = 0, D_800C3FFE = the target's mask) and advance +0x2DA.
 * Then func_8009413C(actor, 0).
 * Codegen: retail keeps the D_800C3EAC load across the halfword row
 * stores, written as array elements here; the byte stores reload it, and
 * so does a *(u16 *)(D_800C4022 + ...) store (452 vs 436 bytes).
 * Matches under gcc 2.7.2 and 2.6.x. */
void func_800819A4(u8 actor) {
    u16 mask;

    if (((u8 *)D_800C3EAC)[0x2E9] == 0) {
        ((u8 *)D_800C3EAC)[0x2E9] = 1;
        func_800BCD98(0);
        ((u8 *)D_800C3EAC + (actor << 6))[0x3C] = ((u8 *)D_800C3EAC)[0x2E8];
        func_8007FCE8();
        func_8007FDEC();
        func_80077980();
        mask = func_80089C08(actor);
        func_800BC404((mask | func_80089C08(((u8 *)D_800C3EAC + (actor << 6))[0x3C])) & 0xFFFF);
        if (D_800D2D24[actor] != 4) {
            D_800C400B[((u8 *)D_800C3EAC)[0x2DA] * 0x48] = actor;
            D_800C402F[((u8 *)D_800C3EAC)[0x2DA] * 0x48] = 0xFD;
            ((u16 *)D_800C4022)[((u8 *)D_800C3EAC)[0x2DA] * 0x24] = 0;
            ((u16 *)D_800C3FFE)[((u8 *)D_800C3EAC)[0x2DA] * 0x24] =
                func_80089C08(((u8 *)D_800C3EAC + (actor << 6))[0x3C]);
            ((u8 *)D_800C3EAC)[0x2DA]++;
        }
        func_8009413C(actor, 0);
    }
}
#endif
