#include "common.h"


#ifndef XENO_PC_PORT
/* Retail-typed callee prototypes for the matched bodies in this TU.  The port
 * prototypes below (u8 parameters) would make gcc 2.7.2 re-mask the already
 * masked index on every call. */
extern u8* D_800C3EAC;
extern u8 D_800D366C;
extern u8 D_800C3E18;
void func_80085D34(void);
void func_800879A8(s32 index, u8 column);
s32 func_800877E0(s32 index, u8 column);
s32 func_80080AE4(s32 index);
void func_800B89FC(s32 r, s32 index, u8 column, s32 target);

void func_80087A38(s32 arg0) {
    s32 index;
    s32 row;
    s32 r;

    func_80085D34();
    index = arg0 & 0xFF;
    /* Integer row address: pointer arithmetic swaps the addu operands. */
    row = index << 6;
    func_800879A8(index, *(u8*)((s32)D_800C3EAC + row + 0x3C));
    D_800D366C = 0;
    r = func_800877E0(index, *(u8*)((s32)D_800C3EAC + row + 0x3C));
    func_800B89FC(r, index, *(u8*)((s32)D_800C3EAC + row + 0x3C), func_80080AE4(index));
    D_800D366C = 1;
    D_800C3E18 = 0;
}

/* func_80087AF0.s, byte-exact: run actor `actor`'s chosen command
 * (target = the actor's D_800C3EAC row byte +0x3C).  On the first call
 * (D_800C3E18 == 0) the actor joins the target's group:
 * func_800881B8(actor, target) when its D_800D32A1 flag is set, else
 * func_80087EDC(actor, target) unless D_800D2D24[actor] is 4; then
 * D_800C3E18 = 1.  After func_80085388():
 * - flag set: D_800C3EAC +0x2DC++;
 * - else, with n = +0x2DC: n < 8 stores the D_800C34B3 entry [n][column]
 *   (3-byte rows); otherwise a nonzero (u16)func_80089C6C(mask, n - 8)
 *   (mask = the g_GameState +0x16C0 halfword of 32-byte record
 *   D_800D2D24[actor]) makes it return 1, else +0x2DC = 7.
 * A target whose D_800CCD1C halfword (0x170-byte records) has bit 0x800
 * gets a command row (actor, 0xF3, func_80089C08(target)) at +0x2DA, which
 * advances.  Then func_80085CCC(actor, func_80089C08(target),
 * +0x2DC - 1), a command row (actor, D_800D2C94, D_800D2C98's low byte) at
 * row = +0x2DA, which advances, and func_80079840(actor, target).  A
 * target >= 3 whose func_80079AB0(target) is nonzero also makes it
 * return 1.  Last, func_80085C88(row).
 * Codegen (gcc 2.6.3: main39 is BattleGcc263):
 * - column is copied into a block-local u32 c at the top of the n < 8
 *   arm.  c is then local to that block, and local-alloc gives it a0
 *   while the row address chain and the table base take v0 and v1;
 *   global-alloc then puts n in a1 and ctx in a2, as retail does (-dl/-dg
 *   dumps).  Copied before the n < 8 test, c gets an a1 preference
 *   (column's entry copy) and n takes a0 (10/251).  Used in place, or
 *   through a u8 c, column is masked late into v1 (11/251).
 * - actor is a plain u8 parameter: retail never masks it on entry and
 *   masks it once per region.
 * - The else arm rereads +0x2DC and the second command row reads +0x2DA
 *   twice, as retail; gcc 2.7.2 shares both loads (diff 22/251).
 * - func_80089C6C is declared as defined (u32); the (u16) on its result
 *   is retail's andi.
 * - The command rows go through asm-label row views, as in
 *   func_8008C4A8 (main48). */
extern u8 D_800D32A1[];
extern u8 D_800D2D24[];
extern u8 D_800C34B3__rows[][3] asm("D_800C34B3");
extern u8 D_800CCD1C[];
extern u8 D_800C400B__rows[][0x48] asm("D_800C400B");
extern u8 D_800C402F__rows[][0x48] asm("D_800C402F");
extern u16 D_800C4022__rows[][0x24] asm("D_800C4022");
extern u16 D_800C3FFE__rows[][0x24] asm("D_800C3FFE");
extern u16 D_800D2C94;
extern u16 D_800D2C98;
extern u8 g_GameState[];
extern void func_80085388(void);
extern void func_800881B8(u8 index, u8 target);
extern void func_80087EDC(u8 index, u8 target);
extern u32 func_80089C6C(u32 mask, u8 index);
extern u16 func_80089C08(u8 index);
extern void func_80085CCC(u8 a0, u16 a1, u16 a2);
extern void func_80079840(u8 a, u8 e);
extern u8 func_80079AB0(u8 actor);
extern void func_80085C88(u8 index);

u32 func_80087AF0(u8 actor, u8 column) {
    u32 done = 0;
    u8 row;
    u8* ctx;
    u32 n;

    if (D_800C3E18 == 0) {
        if (D_800D32A1[actor * 8] == 0) {
            if (D_800D2D24[actor] != 4) {
                func_80087EDC(actor, (D_800C3EAC + (actor << 6))[0x3C]);
            }
        } else {
            func_800881B8(actor, (D_800C3EAC + (actor << 6))[0x3C]);
        }
        D_800C3E18 = 1;
    }
    func_80085388();
    if (D_800D32A1[actor * 8] == 0) {
        ctx = D_800C3EAC;
        n = ctx[0x2DC];
        if (n < 8) {
            u32 c = column;

            ctx[0x2DC] = D_800C34B3__rows[n][c];
        } else if ((u16)func_80089C6C(*(u16*)(g_GameState + 0x16C0 + D_800D2D24[actor] * 32), ctx[0x2DC] - 8) != 0) {
            done = 1;
        } else {
            D_800C3EAC[0x2DC] = 7;
        }
    } else {
        D_800C3EAC[0x2DC]++;
    }
    if (*(u16*)(D_800CCD1C + (D_800C3EAC + (actor << 6))[0x3C] * 0x170) & 0x800) {
        D_800C400B__rows[D_800C3EAC[0x2DA]][0] = actor;
        D_800C402F__rows[D_800C3EAC[0x2DA]][0] = 0xF3;
        D_800C4022__rows[D_800C3EAC[0x2DA]][0] = func_80089C08((D_800C3EAC + (actor << 6))[0x3C]);
        D_800C3EAC[0x2DA]++;
    }
    func_80085CCC(actor, func_80089C08((D_800C3EAC + (actor << 6))[0x3C]), D_800C3EAC[0x2DC] - 1);
    row = D_800C3EAC[0x2DA];
    D_800C400B__rows[D_800C3EAC[0x2DA]][0] = actor;
    D_800C3FFE__rows[D_800C3EAC[0x2DA]][0] = D_800D2C94;
    D_800C402F__rows[D_800C3EAC[0x2DA]][0] = *(u8*)&D_800D2C98;
    D_800C3EAC[0x2DA]++;
    func_80079840(actor, (D_800C3EAC + (actor << 6))[0x3C]);
    if ((D_800C3EAC + (actor << 6))[0x3C] >= 3) {
        if (func_80079AB0((D_800C3EAC + (actor << 6))[0x3C]) != 0) {
            done = 1;
        }
    }
    func_80085C88(row);
    return done;
}
extern u8 D_800C3EB4__rows[][28] asm("D_800C3EB4");
extern u8 D_800C3EB0__rows[][28] asm("D_800C3EB0");
extern u8 D_800D301C__groups[][4] asm("D_800D301C");
extern u8* D_800D3364;
/* The +0xA / +0xC halfwords of the 0x1C-byte D_800C3EB4 rows, under their
 * own symbols as retail addresses them. */
typedef struct {
    u16 v;
    u8 pad[0x1A];
} M39Half;
extern M39Half D_800C3EBE[];
extern M39Half D_800C3EC0[];
void func_800883AC(u8 index);
extern u16 func_80089C9C(u32 mask, u8 index);
extern u16 func_80089C08(u8 index);

/* func_80087EDC.s, byte-exact: move combatant `index` into the group of
 * `target`.  Nothing happens when the two rows already share a group, or
 * when that group (row byte 0 + bank: 8 when `target` is a party member
 * (< 3) and `index` is not, else 0) already holds four.  Otherwise
 * func_800883AC(index) takes the combatant out of its old group, the
 * group's count goes up, row byte 1 gets the first slot 0..3 whose bit
 * func_80089C9C finds clear in the group's +1 mask (row byte 0 gets
 * target's group), func_80089C08(slot) sets that bit, and the row's +0xA
 * / +0xC halfwords come from the D_800D3364 entry at group * 32 + slot * 4
 * (+4/+6 for a party member, else +0x10/+0x12).
 * Codegen (CONSTRUCT): `own` repeats the select's inner flag and is never
 * read.  CSE turns the then-arm into a copy of it, so the flag
 * ((index >= 3) << 3) is computed before the target test and moved in the
 * then-arm, as in retail.  Without it the nested ?: computes the flag
 * inside the then-arm (7/183), and a separate flag variable gives jump.c
 * a single-move then-arm that it rewrites branch-free (708 vs 732 bytes).
 * The D_800D3364 offset keeps row byte 0 first for retail's load order;
 * func_80089C9C is read as u16 (retail masks its result, as main45's
 * callers do).  Matches under gcc 2.7.2 (main39's preset) and 2.6.x. */
void func_80087EDC(u8 index, u8 target) {
    u8 bank;
    s32 own;
    s32 slot;
    u8 group;

    if (D_800C3EB4__rows[index][0] == D_800C3EB4__rows[target][0]) {
        return;
    }
    own = (index >= 3) ? 8 : 0;
    bank = (target < 3) ? ((index >= 3) ? 8 : 0) : 0;
    if (D_800D301C__groups[D_800C3EB4__rows[target][0] + bank][0] >= 4) {
        return;
    }
    func_800883AC(index);
    D_800D301C__groups[D_800C3EB4__rows[target][0] + bank][0]++;
    for (slot = 0; slot < 4; slot++) {
        if (func_80089C9C(D_800D301C__groups[D_800C3EB4__rows[target][0] + bank][1], slot) == 0) {
            break;
        }
    }
    group = D_800C3EB4__rows[target][0];
    D_800C3EB4__rows[index][1] = slot;
    D_800C3EB4__rows[index][0] = group;
    D_800D301C__groups[D_800C3EB4__rows[index][0] + bank][1] |= func_80089C08(D_800C3EB4__rows[index][1]);
    if (index < 3) {
        D_800C3EBE[index].v = *(u16*)(D_800D3364 + (D_800C3EB4__rows[index][0] * 32 + D_800C3EB4__rows[index][1] * 4) + 4);
        D_800C3EC0[index].v = *(u16*)(D_800D3364 + (D_800C3EB4__rows[index][0] * 32 + D_800C3EB4__rows[index][1] * 4) + 6);
    } else {
        D_800C3EBE[index].v = *(u16*)(D_800D3364 + (D_800C3EB4__rows[index][0] * 32 + D_800C3EB4__rows[index][1] * 4) + 0x10);
        D_800C3EC0[index].v = *(u16*)(D_800D3364 + (D_800C3EB4__rows[index][0] * 32 + D_800C3EB4__rows[index][1] * 4) + 0x12);
    }
}

/* func_800881B8.s, byte-exact: move combatant `index` into the alternate
 * group of `target`.  Nothing happens when the two rows already share a
 * group, or when that group (row byte 0 + bank: 0x18 when `target` is a
 * party member (< 3) and `index` is not, else 0x10) is taken.  Otherwise
 * func_800883AC(index) takes the combatant out of its old group, the row
 * gets target's group and a cleared +1 byte, the group is marked (+0 and
 * +1 = 1), and the row's +0xA / +0xC halfwords come from the group's
 * D_800D3364 entry (+0x100/+0x102 for a party member, else +0x104/+0x106).
 * Codegen: `ally = index < 3` comes before the bank select (a
 * decomp-permuter find: it puts the second compare in retail's register),
 * and the select is written with `||`.  The && form lets jump.c rewrite it
 * as `bank = 0x10; if (...) bank = 0x18;` (one insn longer); the || form
 * keeps retail's two-armed select.  The row byte goes through the
 * D_800C3EB0 view (+4): a symbol + offset CONST address has no symbolic
 * term for sched's alias test, so the reloads stay behind the group
 * stores as in retail.  The byte is stored before the +1 clear, so CSE
 * forgets it at the QImode clear and reloads it as retail does.  `<< 3`
 * keeps the base-first D_800D3364 add (as in func_80088490).  Matches
 * under gcc 2.7.2 (main39's preset) and 2.6.x. */
void func_800881B8(u8 index, u8 target) {
    s32 ally;
    u8 bank;

    if (D_800C3EB0__rows[index][4] == D_800C3EB0__rows[target][4]) {
        return;
    }
    ally = index < 3;
    bank = (target >= 3 || ally) ? 0x10 : 0x18;
    if (D_800D301C__groups[D_800C3EB0__rows[target][4] + bank][0] != 0) {
        return;
    }
    func_800883AC(index);
    D_800C3EB0__rows[index][4] = D_800C3EB0__rows[target][4];
    D_800C3EB0__rows[index][5] = 0;
    D_800D301C__groups[D_800C3EB0__rows[index][4] + bank][0] = 1;
    D_800D301C__groups[D_800C3EB0__rows[index][4] + bank][1] = 1;
    if (index < 3) {
        D_800C3EBE[index].v = *(u16*)(D_800D3364 + (D_800C3EB0__rows[index][4] << 3) + 0x100);
        D_800C3EC0[index].v = *(u16*)(D_800D3364 + (D_800C3EB0__rows[index][4] << 3) + 0x102);
    } else {
        D_800C3EBE[index].v = *(u16*)(D_800D3364 + (D_800C3EB0__rows[index][4] << 3) + 0x104);
        D_800C3EC0[index].v = *(u16*)(D_800D3364 + (D_800C3EB0__rows[index][4] << 3) + 0x106);
    }
}
extern u8 D_800D32A1__2[][8] asm("D_800D32A1");
extern u32 func_80089C48(u8 index);

/* func_800883AC.s, byte-exact: remove combatant `index` from its group
 * (see the slot-bookkeeping note below): the group record (row byte 0 +
 * bank) loses one occupant and the func_80089C48(row byte 1) slot bit.
 * The tables are 2D views (28-byte rows, 4-byte groups, 8-byte D_800D32A1
 * entries), which keep every access as symbol + index; flat byte offsets
 * let CSE share a row pointer (size 212 vs 228). */
void func_800883AC(u8 index) {
    u8 bank = (index >= 3) ? 8 : 0;

    if (D_800D32A1__2[index][0] != 0) {
        bank |= 0x10;
    }
    D_800D301C__groups[D_800C3EB4__rows[index][0] + bank][0]--;
    D_800D301C__groups[D_800C3EB4__rows[index][0] + bank][1] &= func_80089C48(D_800C3EB4__rows[index][1]);
}

/* Put combatant `index` alone into an alternate group: its own group if
 * that is empty, else the first empty one of the eight; mark the group
 * (+0 and +1 = 1), clear the row's +1 byte and copy the group's two
 * D_800D3364 +0x100 halfwords into the row's +0xA / +0xC.  CAVEAT (retail
 * behaviour kept): when all eight groups are taken `pick` is never set
 * (retail stores whatever $a2 held on entry; the main39.c port arm below
 * models it as a third parameter).  Codegen: s32 loop counter (slti), and
 * the group index scaled with `<< 3` for the base-first D_800D3364 add. */
void func_80088490(u8 index) {
    u32 group = D_800C3EB4__rows[index][0];
    u32 pick;
    s32 i;

    if (D_800D301C__groups[group + 0x10][0] == 0) {
        pick = group;
    } else {
        for (i = 0; i < 8; i++) {
            if (D_800D301C__groups[0x10 + i][0] == 0) {
                pick = i;
                break;
            }
        }
    }
    D_800C3EB4__rows[index][0] = pick;
    D_800C3EB4__rows[index][1] = 0;
    D_800D301C__groups[D_800C3EB4__rows[index][0] + 0x10][1] = 1;
    D_800D301C__groups[D_800C3EB4__rows[index][0] + 0x10][0] = 1;
    D_800C3EBE[index].v = *(u16*)(D_800D3364 + (D_800C3EB4__rows[index][0] << 3) + 0x100);
    D_800C3EC0[index].v = *(u16*)(D_800D3364 + (D_800C3EB4__rows[index][0] << 3) + 0x102);
}
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
#ifdef XENO_PC_PORT
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
#endif /* XENO_PC_PORT */

/* Slot bookkeeping shared by the functions below.  D_800C3EB4 is a table of
 * 28-byte per-combatant rows: +0 group, +1 slot within the group, +0xA/+0xC
 * a cached position.  D_800D301C is a table of 4-byte group records at
 * (group + bank) * 4: +0 occupant count, +1 occupied-slot mask; bank 0 is the
 * party side, 8 the enemy side and 0x10/0x18 their alternate sets.  Combatants
 * 0-2 are the party.  These bodies are for the port and the differential
 * harnesses; the retail build compiles the byte-exact C above where it
 * exists (docs/port/shadowing_matched_c.tsv) and assembles retail bytes for
 * the rest. */
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
