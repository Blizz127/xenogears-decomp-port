#include "common.h"


#ifndef XENO_PC_PORT
extern u8 D_800D32A1[];
extern u8 D_800D2D24[];
extern u8 g_GameState[];
extern u16 D_800CDD40__rows[][38][20] asm("D_800CDD40");
extern u8 D_800CDD53__b[][38][40] asm("D_800CDD53");
extern u16 D_800CEF10__rows[][42][20] asm("D_800CEF10");
extern u8 D_800CEF23__b[][42][40] asm("D_800CEF23");
extern u16 D_800CCD38__rows[][0xB8] asm("D_800CCD38");
extern u8* D_800D2D28;
extern u16 func_80089C6C(u16 bits, u8 b);
extern void func_8008B108(u8 flag);
extern void func_8008B168(u8 index);
extern u8 func_80085084(u16 arg0, u8 arg1, u8 arg2);
extern void func_8008AA74(u32 v);

/* func_8008B224.s, byte-exact: use the costed entry at (col, row) of
 * party actor `actor`, the func_8008CDE4 pattern.  An actor whose
 * D_800D32A1 byte (8-byte rows) is clear takes entry row * 2 + col + 0x16
 * of its 38 x 40-byte D_800CDD40 table and asks func_80089C6C about the
 * g_GameState + 0x16C2 halfword; otherwise entry + 0x15 of its 42 x
 * 40-byte D_800CEF10 table and the + 0x16C6 halfword (both indexed by
 * the actor's D_800D2D24 byte * 32).  The entry's halfword +0 is the
 * target word and its byte +0x13 the cost.  When the actor's D_800CCD38
 * halfword covers the cost and the check passed: func_8008B108(1),
 * D_800D2D28 +0xC6 = 1, func_80085084(target, actor, 0); on commit the
 * cost is paid and it returns 1, else +0xC6 is cleared and
 * func_8008B168(actor) runs.  Otherwise func_8008AA74(0x4F).
 * First draft; it matches under gcc 2.7.2 (main46's preset) and 2.6.x. */
u8 func_8008B224(u8 actor, u8 col, u8 row) {
    u16 target;
    u8 cost;
    u8 ok;
    u8 fail;
    u8 ret;

    ret = 0;
    fail = 1;
    ok = 0;
    if (D_800D32A1[actor * 8] == 0) {
        target = D_800CDD40__rows[actor][row * 2 + col + 0x16][0];
        cost = D_800CDD53__b[actor][row * 2 + col + 0x16][0];
        ok = func_80089C6C(*(u16*)(g_GameState + 0x16C2 + D_800D2D24[actor] * 32), row * 2 + col) != 0;
    } else {
        target = D_800CEF10__rows[actor][row * 2 + col + 0x15][0];
        cost = D_800CEF23__b[actor][row * 2 + col + 0x15][0];
        if (func_80089C6C(*(u16*)(g_GameState + 0x16C6 + D_800D2D24[actor] * 32), row * 2 + col)) {
            ok = 1;
        }
    }
    if (D_800CCD38__rows[actor][0] >= cost && ok) {
        func_8008B108(1);
        D_800D2D28[0xC6] = 1;
        if (func_80085084(target, actor, 0)) {
            D_800CCD38__rows[actor][0] -= cost;
            ret = 1;
        } else {
            D_800D2D28[0xC6] = 0;
            func_8008B168(actor);
        }
        fail = 0;
    }
    if (fail) {
        func_8008AA74(0x4F);
    }
    return ret;
}
extern u8 D_800D3014;
extern s32 D_800D3288;
extern u8 D_800D39D4;
extern void func_8008F8F4(u8 index, u16 x, u16 y, u16 w, u16 h, u8 animate, u8 refresh);
extern void func_80090B90(s32 x, s32 y, s32* frame, u8* timer);
extern void func_800716D8(void);
extern void func_80077698(void);
extern void func_8007765C(void);
extern void func_80077980(void);
extern void func_8008FA60(u8 index);
extern void func_80091064(u8 a);
extern void func_80091604(s32 y);
extern void func_80091D38(u8 a, u8 b, u8 c);
extern void func_80091EC4(s32 y);
extern u8* D_800C3EAC;

/* func_8008B478.s, byte-exact: party actor `actor`'s costed-entry grid
 * menu: 16 entries in two columns, 12 visible cells; `col` is the visible
 * cell (0-11, screen column col % 2), `row` the first visible row, the
 * entry row * 2 + col.  D_800D3288 (scroll) and
 * D_800D39D4 (pending scroll) = 0, D_800D2D28 +0xCB = 0, four windows
 * (func_8008F8F4), func_80091064(actor), func_80077698.  Then until a
 * choice: func_80091D38(actor, col, row) when the cursor cell changes,
 * func_80091604 / func_80091EC4 when the scroll changes, the cursor
 * (func_80090B90 at column x 0x2A / 0xAE, y (col / 2) * 16 + 0x38), a
 * frame (func_800716D8) and the D_800D3014 event: 5 cancels (0), 4 commits
 * through func_8008B224 (1, D_800C3EAC +0x2E6 = row * 2 + col), 0 / 2 step
 * the cursor right / left and 1 / 3 down / up within 16 entries, asking for
 * a scroll (D_800D39D4 1-4) at the edge of the 12 visible cells; a pending
 * scroll finishes once D_800D3288 passes the next row (row and col adjust,
 * D_800D3288 snaps to the row).  Finally func_8008B108(result),
 * func_8007765C, func_80077980 and func_8008FA60(0..3); returns the result.
 * Codegen: the column x is spelled as two terms (`col % 2 * 0x80 +
 * col % 2 * 4`): a single `* 0x84` synthesises ((v << 5) + v) << 2, retail
 * has (v << 7) + (v << 2).  func_80091EC4 is called with the whole
 * D_800D3288 word (retail does not mask it; main51 defines it (u8)). */
u8 func_8008B478(u8 actor) {
    s32 frame;
    u8 timer;
    u8 col;
    u8 row;
    u8 lastCol;
    u8 lastRow;
    s32 lastY;
    u8 state;
    s32 y;

    col = 0;
    row = 0;
    lastCol = 0xFF;
    lastRow = 0xFF;
    lastY = 0xFFFF;
    state = 2;
    frame = 4;
    timer = 0;
    D_800D3288 = 0;
    D_800D39D4 = 0;
    D_800D2D28[0xCB] = 0;
    func_8008F8F4(0, 0x1C, 0xA0, 0xAC, 0x38, 0, 1);
    func_8008F8F4(1, 0x20, 0x2C, 0x118, 0x70, 0, 1);
    func_8008F8F4(2, 0xD0, 0xA4, 0x50, 0x18, 0, 1);
    func_8008F8F4(3, 0xE6, 0xC0, 0x4C, 0x18, 0, 1);
    func_80091064(actor);
    func_80077698();
    while (state == 2) {
        if (col != lastCol || row != lastRow) {
            func_80091D38(actor, col, row);
            lastCol = col;
            lastRow = row;
        }
        if (D_800D3288 != lastY) {
            func_80091604(D_800D3288);
            func_80091EC4(D_800D3288);
            lastY = D_800D3288;
        }
        func_80090B90(col % 2 * 0x80 + col % 2 * 4 + 0x2A, col / 2 * 0x10 + 0x38, &frame, &timer);
        func_800716D8();
        switch (D_800D3014) {
        case 5:
            state = 0;
            break;
        case 4:
            if (func_8008B224(actor, col, row)) {
                state = 1;
                D_800C3EAC[0x2E6] = col + row * 2;
            }
            break;
        case 0:
            if (row * 2 + col != 0xF) {
                if (col + 1 == 0xC) {
                    D_800D39D4 = 1;
                } else {
                    col++;
                }
            }
            break;
        case 2:
            if (row * 2 + col > 0) {
                if (col == 0 && row != 0) {
                    D_800D39D4 = 2;
                } else {
                    col--;
                }
            }
            break;
        case 1:
            if (row * 2 + col < 0xE) {
                if (col + 2 >= 0xC) {
                    D_800D39D4 = 3;
                } else {
                    col += 2;
                }
            }
            break;
        case 3:
            if (row * 2 + col >= 2) {
                if (col < 2 && row != 0) {
                    D_800D39D4 = 4;
                } else {
                    col -= 2;
                }
            }
            break;
        }
        switch (D_800D39D4) {
        case 1:
            y = (row + 1) * 0x10;
            if (D_800D3288 >= y) {
                row++;
                col--;
                D_800D3288 = y;
                D_800D39D4 = 0;
            }
            break;
        case 2:
            y = (row - 1) * 0x10;
            if (D_800D3288 < y) {
                row--;
                col++;
                D_800D3288 = y;
                D_800D39D4 = 0;
            }
            break;
        case 3:
            y = (row + 1) * 0x10;
            if (D_800D3288 >= y) {
                row++;
                D_800D3288 = y;
                D_800D39D4 = 0;
            }
            break;
        case 4:
            y = (row - 1) * 0x10;
            if (D_800D3288 < y) {
                row--;
                D_800D3288 = y;
                D_800D39D4 = 0;
            }
            break;
        }
    }
    func_8008B108(state);
    func_8007765C();
    func_80077980();
    func_8008FA60(0);
    func_8008FA60(1);
    func_8008FA60(2);
    func_8008FA60(3);
    return state;
}
extern u8* D_800C3EAC;
extern u8* D_800D2D28;
extern u8 D_800D366C;
extern u16 D_800C3D64;
extern u8 D_800C3E2C;
extern u8 D_800C3D00;
extern u8 D_800D3670;
extern u8 D_800C2050;
extern u16 D_800D2C98;
extern u16 D_800D2C94;
extern u8 D_800D2204[];
extern u8 D_800C402F__rows[][0x48] asm("D_800C402F");
extern u8 D_800C400B__rows[][0x48] asm("D_800C400B");
extern u16 D_800C4022__rows[][0x24] asm("D_800C4022");
extern u16 D_800C3FFE__rows[][0x24] asm("D_800C3FFE");
typedef struct {
    u16 flags;
    u8 pad[0x10];
    u8 counts[0x30];
    u8 items[0x30];
} BattleItemBlock;
extern BattleItemBlock D_800D2C9E__s asm("D_800D2C9E");
extern void func_800BCD98(u32 v);
extern u32 func_80089C08(u32 v);
extern s32 func_80080AE4(u8 target);
extern void func_800B89FC(u32 a, u32 b, u32 c, u32 d);
extern void func_8008AC88(u16 mask, u8 v);
extern void func_80085388(void);
extern void func_80085C48(u32 a0, u32 a1, u32 a2);
extern void func_80085C88(u8 index);
extern u16 func_80089C9C(s32 mask, u8 idx);
extern void func_80079840(u8 a, u8 e);
extern void func_80080B64(u8 v);
extern void func_800716D8(void);

/* func_8008B908.s, byte-exact: queue actor `actor`'s selected item, the
 * func_8008ADD0 pattern.  D_800D366C = 0, func_800BCD98(0), all 32
 * command rows get D_800C402F = 0xFF; func_800B89FC(1, actor, the actor
 * row's +0x3C, func_80080AE4(actor)); the item's D_800D2204 halfword
 * (16-byte entries, D_800C3EAC +0x2E6) low three bits pick the target
 * mask (D_800C3D64, else func_80089C08 of D_800C3E2C);
 * func_8008AC88(mask, actor), func_80085388, func_80085C48(actor, mask,
 * item).  Unless D_800D2C9E has bit 0x8000, the item's count at
 * D_800D2C9E +0x12 [D_800C3D00 * 2 + D_800D3670] drops by one and its
 * +0x42 entry is cleared at 0.  func_80085C88(row) runs with D_800C2050
 * set; the command row gets D_800C402F = 0xF5, D_800C4022 = D_800D2C98,
 * D_800C400B = actor, D_800C3FFE = D_800D2C94; func_80079840(actor, k)
 * for each selected enemy; the cursor advances; func_80080B64(actor);
 * wait for +0x2DB; finally D_800D2D28 +0x7C + i = 1 for each party slot
 * i the mask selects.
 * Codegen: D_800D2C9E is a small struct (BattleItemBlock) because retail
 * addresses the count and item tables from its one `la`; the party-flag
 * row is `(D_800D2D28 + i)[0x7C]`, base first.  Matches only under gcc
 * 2.6.x. */
void func_8008B908(u8 actor) {
    s32 i;
    u16 mask;

    D_800D366C = 0;
    func_800BCD98(0);
    for (i = 0; i < 32; i++) {
        D_800C402F__rows[i][0] = 0xFF;
    }
    func_800B89FC(1, actor, (D_800C3EAC + (actor << 6))[0x3C], func_80080AE4(actor));
    if (*(u16*)(D_800D2204 + D_800C3EAC[0x2E6] * 16) & 7) {
        mask = D_800C3D64;
    } else {
        mask = func_80089C08(D_800C3E2C);
    }
    func_8008AC88(mask, actor);
    func_80085388();
    func_80085C48(actor, mask, D_800C3EAC[0x2E6]);
    if (!(D_800D2C9E__s.flags & 0x8000)) {
        if (--D_800D2C9E__s.counts[D_800C3D00 * 2 + D_800D3670] == 0) {
            D_800D2C9E__s.items[D_800C3D00 * 2 + D_800D3670] = 0;
        }
    }
    D_800C2050 = 1;
    func_80085C88(D_800C3EAC[0x2DA]);
    D_800C2050 = 0;
    D_800C402F__rows[D_800C3EAC[0x2DA]][0] = 0xF5;
    D_800C4022__rows[D_800C3EAC[0x2DA]][0] = D_800D2C98;
    D_800C400B__rows[D_800C3EAC[0x2DA]][0] = actor;
    D_800C3FFE__rows[D_800C3EAC[0x2DA]][0] = D_800D2C94;
    for (i = 3; i < 11; i++) {
        if (func_80089C9C(D_800D2C94, i)) {
            func_80079840(actor, i);
        }
    }
    D_800C3EAC[0x2DA]++;
    func_80080B64(actor);
    while (D_800C3EAC[0x2DB] == 0) {
        func_800716D8();
    }
    for (i = 0; i < 3; i++) {
        if (func_80089C9C(mask, i)) {
            (D_800D2D28 + i)[0x7C] = 1;
        }
    }
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


/* func_8008BC40.s */
void func_8008BC40(u8 flag) {
    u8* p;
    u8* q;
    u8* r;

    p = D_800D2D28;
    p[0x9E] = 0;
    p[0x9D] = 0;
    p[0x9C] = 0;
    q = D_800D2D28;
    q[0xB1] = 0;
    q[0xB0] = 0;
    r = D_800D2D28;
    r[0xB7] = 0;
    if (flag == 0) {
        D_800D2D28[0xCB] = 1;
    }
}
/* func_8008BC98.s */
void func_8008BC98(u8 index) {
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
    q[0xB1] = 1;
    q[0xB0] = 1;
    r = D_800D2D28;
    r[0xB7] = 2;
    v = func_80089C08(index & 0xFF);
    off = (u32)index << 6;
    v |= func_80089C08(*(u8*)(D_800C3EAC + off + 0x3C));
    func_800BC404(v & 0xFFFF);
    func_800BCD98(func_80089C08(*(u8*)(D_800C3EAC + off + 0x3C)) & 0xFFFF);
}
