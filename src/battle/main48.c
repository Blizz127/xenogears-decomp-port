#include "common.h"


#ifndef XENO_PC_PORT
extern u8* D_800D2D28;
extern u8* D_800C3EAC;
extern u8 D_800D366C;
extern u8 D_800C3E2C;
extern u8 D_800C4928;
extern u16 D_800D2C94;
extern u8 D_800CCD1C[];
extern u8 D_800C400B__rows[][0x48] asm("D_800C400B");
extern u8 D_800C402F__rows[][0x48] asm("D_800C402F");
extern u16 D_800C4022__rows[][0x24] asm("D_800C4022");
extern u16 D_800C3FFE__rows[][0x24] asm("D_800C3FFE");
extern u8 func_8008C81C(u8 actor);
extern void func_800BAF40();
extern void func_8008C360(u8 a);
extern void func_800BCD98(u32 v);
extern u32 func_800877E0(u8 a, u8 b);
extern void func_80087A38(u8 index);
extern u32 func_80089C08(u32 v);
extern void func_80085CCC(u8 a0, u16 a1, u16 a2);
extern void func_80085C88(u8 index);
extern void func_80080B64(u8 v);
extern void func_80087EDC(u8 index, u8 target);
extern void func_800716D8(void);

/* func_8008C4A8.s, byte-exact: run actor `actor`'s target menu
 * (func_8008C81C).  On cancel: func_800BAF40(actor, 0x100),
 * func_8008C360(1), D_800D2D28 +0xCB = 1, return 0.  Otherwise:
 * func_800BAF40(actor, 0x80), D_800D2D28 +0xAF = 0, func_8008C360(1),
 * D_800D366C = 0, func_800BCD98(0); the actor's D_800C3EAC row byte
 * +0x3C (64-byte rows) takes the selection D_800C3E2C;
 * func_800877E0(actor, target), D_800C4928 = 1, func_80087A38(actor).
 * A target whose D_800CCD1C halfword (0x170-byte records) has bit 0x800
 * gets a 0xF3 command row (D_800C400B = actor, D_800C4022 =
 * func_80089C08(target)).  Then for each of the seven D_800C3EAC +0x2C4
 * bytes that is not 0xFF: +0x2DC = byte + 8, func_80085CCC(actor,
 * func_80089C08(target), +0x2DC - 1), func_80085C88(row), and a command
 * row with D_800C402F = +0x2DC - 1, D_800C400B = actor and D_800C3FFE =
 * D_800D2C94.  Finally func_80080B64(actor), func_80087EDC(actor,
 * target), wait for D_800C3EAC +0x2DB (func_800716D8), return 1.
 * Codegen: the command rows go through asm-label row views, which keep
 * the symbol in each address (u8 arrays let loop.c hoist three `la`s
 * into s-registers); the actor row is `(D_800C3EAC + (actor << 6))`,
 * base first, as in func_80079840; the cancel path falls through to the
 * shared `return ret`.  Matches only under gcc 2.6.x (retail's u8
 * `+0x2DC - 1` is `addiu 0xFF`). */
u8 func_8008C4A8(u8 actor) {
    s32 i;
    u8 ret;

    ret = 1;
    if (func_8008C81C(actor)) {
        func_800BAF40(actor, 0x100);
        func_8008C360(1);
        D_800D2D28[0xCB] = 1;
        ret = 0;
    } else {
        func_800BAF40(actor, 0x80);
        D_800D2D28[0xAF] = 0;
        func_8008C360(1);
        D_800D366C = 0;
        func_800BCD98(0);
        (D_800C3EAC + (actor << 6))[0x3C] = D_800C3E2C;
        func_800877E0(actor, (D_800C3EAC + (actor << 6))[0x3C]);
        D_800C4928 = 1;
        func_80087A38(actor);
        if (*(u16*)(D_800CCD1C + (D_800C3EAC + (actor << 6))[0x3C] * 0x170) & 0x800) {
            D_800C400B__rows[D_800C3EAC[0x2DA]][0] = actor;
            D_800C402F__rows[D_800C3EAC[0x2DA]][0] = 0xF3;
            D_800C4022__rows[D_800C3EAC[0x2DA]][0] = func_80089C08((D_800C3EAC + (actor << 6))[0x3C]);
            D_800C3EAC[0x2DA]++;
        }
        for (i = 0; i < 7; i++) {
            if ((D_800C3EAC + i)[0x2C4] != 0xFF) {
                D_800C3EAC[0x2DC] = (D_800C3EAC + i)[0x2C4] + 8;
                func_80085CCC(actor, func_80089C08((D_800C3EAC + (actor << 6))[0x3C]), D_800C3EAC[0x2DC] - 1);
                func_80085C88(D_800C3EAC[0x2DA]);
                D_800C402F__rows[D_800C3EAC[0x2DA]][0] = D_800C3EAC[0x2DC] - 1;
                D_800C400B__rows[D_800C3EAC[0x2DA]][0] = actor;
                D_800C3FFE__rows[D_800C3EAC[0x2DA]][0] = D_800D2C94;
                D_800C3EAC[0x2DA]++;
            }
        }
        func_80080B64(actor);
        func_80087EDC(actor, (D_800C3EAC + (actor << 6))[0x3C]);
        while (D_800C3EAC[0x2DB] == 0) {
            func_800716D8();
        }
    }
    return ret;
}
/* func_8008C81C.s, byte-exact: the selection screen func_8008C4A8 runs.
 * ids/costs list the entries i < 7 accepted by func_80089C6C(the
 * g_GameState +0x16C0 halfword of the actor's D_80059468 slot, i); an
 * entry's cost is D_800CDD57[actor][i + 7][0].  After the two
 * func_8008F8F4 windows and the D_800D2D28 +0xBF/+0xC0 wait it calls
 * func_80092784(actor, ids, costs), then handles D_800D3014 events on a
 * 2x4 cursor (x = sel % 2, y = sel / 2):
 * - 0-3 move the cursor; 6/7/9/10 jump to cell 7.
 * - 4 on cell 7: with nothing picked (+0x2C4 == 0xFF) cancel;
 *   otherwise func_8008C360(0), D_800D2D28 +0xC6 = 1, and
 *   func_80085084(0x1000, actor, 1) either stores the remaining points
 *   to D_800D32A0[actor * 8] (done) or clears +0xC6 and calls
 *   func_8008C3F0(actor).
 * - 4 on another cell: append that entry (at most 7) to D_800C3EAC
 *   +0x2C4 and its cell to D_800C3DE0 when the points cover its cost.
 * - 5: drop the last pick and refund its cost; cancel when none is left.
 * Returns 0 when done, 1 on cancel.
 * ids has 16 bytes: func_80092784 hands it on to func_80092298, which
 * adds a sprite for each byte that is not 0xFF.  ids[8..15] are zeroed
 * here; as in retail, ids[7] is never written, so stale stack data
 * decides sprite 7 (an indeterminate read if built natively).
 * Codegen (gcc 2.6.3 only):
 * - ids[8..15] are zeroed by their own loop starting at i = 8.  Loop.c
 *   reverses that loop when its counter starts at 0 (-dL: "Reversed
 *   loop"); from 8 it keeps it and eliminates the counter against the
 *   pointer (signed slt against sp+0x30), as in retail.
 * - `(D_800C3EAC + i)[0x2C4]` puts the base first in the addu.
 * - refresh = 1 comes before n = 0; the append sets refresh last.
 * - The cursor x keeps `(s16)(sel % 2 * 4 + 0x1E)` as its own term
 *   (without the cast fold re-associates the sum).
 * - The top-tested `while (state == 0)` gives retail's entry test (a
 *   do/while is 12 bytes short).
 * The switch table jtbl_80070344 is now main48's .rodata
 * (config/battle.yaml: [0x854, .rodata, main48]). */
extern u8 D_800D32A0[];
extern u8 D_80059468[];
typedef struct {
    u16 unk0;
    u16 unk2;
    u16 unk4;
    u16 unk6;
    u8 pad8[0x18];
} M48Chr20;
typedef struct {
    u8 pad[0x16C0];
    M48Chr20 chr[11];
} M48GameState16C0;
extern M48GameState16C0 g_GameState__v asm("g_GameState");
extern u8 D_800CDD57__b[][38][40] asm("D_800CDD57");
extern u8 D_800C3DE0[];
extern u8 D_800D3014;
extern u32 func_80089C6C(u32 mask, u8 index);
extern void func_8008F8F4(u8 index, u16 x, u16 y, u16 w, u16 h, u8 animate, u8 refresh);
extern void func_80090B90(s32 x, s32 y, s32* frame, u8* timer);
extern void func_80077698(void);
extern void func_80092784(u8 a, u8* ids, u8* vals);
extern void func_80092B74(u8 a, u8 b);
extern u8 func_80085084(u16 arg0, u8 arg1, u8 arg2);
extern void func_8008C3F0(u8 index);

u8 func_8008C81C(u8 actor) {
    u8 ids[16];
    u8 costs[16];
    u8 spent[8];
    s32 frame;
    u8 timer;
    u8 state;
    u8 sel;
    u8 n;
    u8 points;
    u8 refresh;
    s32 count;
    s32 i;

    state = 0;
    sel = 0;
    refresh = 1;
    n = 0;
    points = D_800D32A0[actor * 8];
    frame = 4;
    timer = 0;
    D_800D2D28[0xCB] = 0;
    for (i = 0; i < 7; i++) {
        ids[i] = 0xFF;
        costs[i] = 0;
        (D_800C3EAC + i)[0x2C4] = 0xFF;
        D_800C3DE0[i] = 0xFF;
    }
    for (i = 8; i < 16; i++) {
        ids[i] = 0;
    }
    count = 0;
    for (i = 0; i < 7; i++) {
        if ((u16)func_80089C6C(g_GameState__v.chr[D_80059468[actor]].unk0, i)) {
            ids[count] = i;
            costs[count] = D_800CDD57__b[actor][i + 7][0];
            count++;
        }
    }
    func_8008F8F4(0, 0x16, 0x5C, 0x122, 0x4C, 1, 1);
    func_8008F8F4(1, 0x10, 0x2C, 0xE8, 0x2C, 1, 1);
    while (D_800D2D28[0xBF] == 0 || D_800D2D28[0xC0] == 0) {
        func_800716D8();
    }
    func_80092784(actor, ids, costs);
    func_80077698();
    while (state == 0) {
        if (refresh) {
            func_80092B74(actor, points);
            refresh = 0;
        }
        func_80090B90(sel % 2 * 0x88 + (s16)(sel % 2 * 4 + 0x1E), sel / 2 * 0x10 + 0x64, &frame, &timer);
        func_800716D8();
        switch (D_800D3014) {
        case 5:
            if (n != 0) {
                n--;
                points += spent[n];
                refresh = 1;
                (D_800C3EAC + n)[0x2C4] = 0xFF;
                D_800C3DE0[n] = 0xFF;
            } else {
                state = 2;
            }
            break;
        case 4:
            if (sel == 7) {
                if (D_800C3EAC[0x2C4] == 0xFF) {
                    state = 2;
                } else {
                    func_8008C360(0);
                    D_800D2D28[0xC6] = 1;
                    if (func_80085084(0x1000, actor, 1)) {
                        D_800D32A0[actor * 8] = points;
                        state = 1;
                    } else {
                        D_800D2D28[0xC6] = 0;
                        func_8008C3F0(actor);
                    }
                }
            } else if (n != 7 && ids[sel] != 0xFF && points >= costs[sel]) {
                (D_800C3EAC + n)[0x2C4] = ids[sel];
                D_800C3DE0[n] = sel;
                points -= costs[sel];
                spent[n] = costs[sel];
                n++;
                refresh = 1;
            }
            break;
        case 0:
            if (++sel >= 8) {
                sel--;
            }
            break;
        case 1:
            sel += 2;
            if (sel >= 8) {
                sel -= 2;
            }
            break;
        case 2:
            if (sel != 0) {
                sel--;
            }
            break;
        case 3:
            if (sel >= 2) {
                sel -= 2;
            }
            break;
        case 6:
        case 7:
        case 9:
        case 10:
            sel = 7;
            break;
        }
    }
    return state - 1;
}
#endif


#ifndef XENO_PC_PORT
extern u8* D_800D2D28;
#endif
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
extern u8 D_800D366C;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800C3EAC;
#endif
extern u32 func_80089C08(u32 v);
extern void func_800BC404(u32 v);
extern void func_800BCD98(u32 v);
extern void func_80085454(s32);
extern void func_80085618(s32);
extern void func_8008AA40(u8 v);
#ifndef XENO_PC_PORT
extern u32 D_800C3EA4;
#endif
extern void* func_8008ABB8(s32 size, s32 flag);
extern void* bzero(unsigned char* p, int size);
extern void func_80077074(void);


/* func_8008CCCC.s */
void func_8008CCCC(u8 flag) {
    u8* p;
    u8* q;
    u8* r;

    p = D_800D2D28;
    p[0x9E] = 0;
    p[0x9D] = 0;
    p[0x9C] = 0;
    q = D_800D2D28;
    q[0xB2] = 0;
    q[0xB1] = 0;
    q[0xB0] = 0;
    r = D_800D2D28;
    r[0xB7] = 0;
    if (flag == 0) {
        D_800D2D28[0xCB] = 1;
    }
}
/* func_8008CD28.s */
void func_8008CD28(u8 index) {
    u8* p;
    u8* q;
    u8* r;
    u32 v;
    u32 off;

    p = D_800D2D28;
    p[0x9E] = 1;
    p[0x9D] = 1;
    p[0x9C] = 1;
    q = D_800D2D28;
    q[0xB2] = 1;
    q[0xB1] = 1;
    q[0xB0] = 1;
    r = D_800D2D28;
    r[0xB7] = 4;
    v = func_80089C08(index & 0xFF);
    off = (u32)index << 6;
    v |= func_80089C08(*(u8*)(D_800C3EAC + off + 0x3C));
    func_800BC404(v & 0xFFFF);
    func_800BCD98(func_80089C08(*(u8*)(D_800C3EAC + off + 0x3C)) & 0xFFFF);
}
