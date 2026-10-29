#include "common.h"


#ifndef XENO_PC_PORT
extern u8* D_800C3EAC;
extern u8 D_800CCD1C[];
extern u8 D_800C400B[];
extern u8 D_800C402F[];
extern u8 D_800C4022[];
extern u16 func_80089C9C(s32 mask, u8 idx);
extern u16 func_80089C48(u8 idx);

/* func_8008AC88.s, byte-exact: queue a 0xF3 command for the enemies in
 * `mask`.  The party bits (0-2) are dropped; each enemy slot k = 3..10
 * that func_80089C9C selects from the mask is cleared from it
 * (func_80089C48) unless the D_800CCD1C halfword of its 0x170-byte
 * record has bit 0x800.  If any slot is left, the command row at
 * D_800C3EAC +0x2DA gets D_800C400B = v, D_800C402F = 0xF3 and the
 * D_800C4022 halfword = mask, and the row cursor advances.
 * Codegen: the record offset is its own counter (`off`, from 0x450 in
 * steps of 0x170) and the address is the symbol plus that offset, as in
 * retail; `(i + 3) * 0x170` lets loop.c reduce the whole address to a
 * pointer.  Matches only under gcc 2.6.x; 2.7.2 schedules i++ at the
 * loop end, where retail fills the first branch's delay slot with it. */
void func_8008AC88(u16 mask, u8 v) {
    s32 i;
    s32 off;
    u8 k;

    mask &= 0xFFF8;
    for (i = 0, off = 0x450; i < 8; i++, off += 0x170) {
        k = i + 3;
        if (func_80089C9C(mask, k) != 0) {
            if (!(*(u16*)(D_800CCD1C + off) & 0x800)) {
                mask &= func_80089C48(k);
            }
        }
    }
    if (mask != 0) {
        D_800C400B[D_800C3EAC[0x2DA] * 0x48] = v;
        D_800C402F[D_800C3EAC[0x2DA] * 0x48] = 0xF3;
        ((u16*)D_800C4022)[D_800C3EAC[0x2DA] * 0x24] = mask;
        D_800C3EAC[0x2DA]++;
    }
}
extern u8 D_800D366C;
extern u8 D_800D32A1[];
extern u16 D_800C3D64;
extern u8 D_800C3E2C;
extern u8 D_800D2C98;
extern u16 D_800D2C94__a[] asm("D_800D2C94");
extern u16 D_800CDD40__rows[][38][20] asm("D_800CDD40");
extern u16 D_800CEF10__rows[][42][20] asm("D_800CEF10");
extern u8 D_800C402F__rows[][0x48] asm("D_800C402F");
extern u8 D_800C400B__rows[][0x48] asm("D_800C400B");
extern u16 D_800C3FFE__rows[][0x24] asm("D_800C3FFE");
extern void func_800BCD98(u32 v);
extern u32 func_80089C08(u32 v);
extern void func_80085388(void);
extern void func_80085CCC(u8 a0, u16 a1, u16 a2);
extern void func_80085C88(u8 index);
extern void func_800BC404(u32 v);
extern s32 func_80080AE4(u8 target);
extern void func_800B89FC(u32 a, u32 b, u32 c, u32 d);
extern u16 func_80089C9C(s32 mask, u8 idx);
extern void func_80079840(u8 a, u8 e);
extern void func_80080B64(u8 v);
extern void func_800716D8(void);

/* func_8008ADD0.s, byte-exact: queue actor `actor`'s selected technique.
 * D_800D366C = 0, func_800BCD98(0), and all 32 command rows get
 * D_800C402F = 0xFF (retail counts the offset down).  The technique is
 * entry D_800C3EAC +0x2E6 + 0x16 of the actor's D_800CDD40 table (+0x2DC
 * = entry + 0x17), or + 0x15 of its D_800CEF10 table (+0x2DC = entry +
 * 0x16) when the actor's D_800D32A1 byte is set; its halfword's low three
 * bits pick the target mask (D_800C3D64, else func_80089C08 of the
 * D_800C3E2C selection).  func_8008AC88(mask, actor) queues the 0xF3
 * row; then func_80085388, func_80085CCC(actor, mask, +0x2DC - 1),
 * func_80085C88(row), func_800BC404(D_800D2C94 | func_80089C08(actor)),
 * func_800B89FC(1, actor, the actor row's +0x3C, func_80080AE4(actor)),
 * a command row (D_800C402F = D_800D2C98, D_800C400B = actor, D_800C3FFE
 * = D_800D2C94), func_80079840(actor, k) for each enemy k = 3..10 that
 * func_80089C9C selects from D_800D2C94, the row cursor advance,
 * func_80080B64(actor), and a wait for +0x2DB (func_800716D8).
 * First draft from the func_8008B224/func_8008C4A8 idioms (row views,
 * base-first actor row); matches under gcc 2.6.x (main45's preset). */
void func_8008ADD0(u8 actor) {
    s32 i;
    u16 mask;
    u16 t;

    D_800D366C = 0;
    func_800BCD98(0);
    for (i = 0; i < 32; i++) {
        D_800C402F__rows[i][0] = 0xFF;
    }
    if (D_800D32A1[actor * 8] == 0) {
        D_800C3EAC[0x2DC] = D_800C3EAC[0x2E6] + 0x17;
        t = D_800CDD40__rows[actor][D_800C3EAC[0x2E6] + 0x16][0] & 7;
    } else {
        D_800C3EAC[0x2DC] = D_800C3EAC[0x2E6] + 0x16;
        t = D_800CEF10__rows[actor][D_800C3EAC[0x2E6] + 0x15][0] & 7;
    }
    if (t) {
        mask = D_800C3D64;
    } else {
        mask = func_80089C08(D_800C3E2C);
    }
    func_8008AC88(mask, actor);
    func_80085388();
    func_80085CCC(actor, mask, D_800C3EAC[0x2DC] - 1);
    func_80085C88(D_800C3EAC[0x2DA]);
    func_800BC404((u16)(D_800D2C94__a[0] | func_80089C08(actor)));
    func_800B89FC(1, actor, (D_800C3EAC + (actor << 6))[0x3C], func_80080AE4(actor));
    D_800C402F__rows[D_800C3EAC[0x2DA]][0] = D_800D2C98;
    D_800C400B__rows[D_800C3EAC[0x2DA]][0] = actor;
    D_800C3FFE__rows[D_800C3EAC[0x2DA]][0] = D_800D2C94__a[0];
    for (i = 3; i < 11; i++) {
        if (func_80089C9C(D_800D2C94__a[0], i)) {
            func_80079840(actor, i);
        }
    }
    D_800C3EAC[0x2DA]++;
    func_80080B64(actor);
    while (D_800C3EAC[0x2DB] == 0) {
        func_800716D8();
    }
}
#endif


#ifndef XENO_PC_PORT
extern u8* D_800D2D28;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800C3EAC;
#endif
extern u32 func_80089C08(u32 v);
extern void func_800BC404(u32 v);
extern void func_800BCD98(u32 v);


/* func_8008B108.s */
void func_8008B108(u8 flag) {
    u8* p;
    u8* q;
    u8* r;

    p = D_800D2D28;
    p[0x9E] = 0;
    p[0x9D] = 0;
    p[0x9C] = 0;
    q = D_800D2D28;
    q[0xB3] = 0;
    q[0xB2] = 0;
    q[0xB1] = 0;
    q[0xB0] = 0;
    r = D_800D2D28;
    r[0xB7] = 0;
    if (flag == 0) {
        D_800D2D28[0xCB] = 1;
    }
}
/* func_8008B168.s */
void func_8008B168(u8 index) {
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
    q[0xB3] = 1;
    q[0xB2] = 1;
    q[0xB1] = 1;
    q[0xB0] = 1;
    r = D_800D2D28;
    r[0xB7] = 1;
    v = func_80089C08(index & 0xFF);
    off = (u32)index << 6;
    v |= func_80089C08(*(u8*)(D_800C3EAC + off + 0x3C));
    func_800BC404(v & 0xFFFF);
    func_800BCD98(func_80089C08(*(u8*)(D_800C3EAC + off + 0x3C)) & 0xFFFF);
}
