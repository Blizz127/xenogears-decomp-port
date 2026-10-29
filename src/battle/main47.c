#include "common.h"


#ifndef XENO_PC_PORT
extern u8* D_800D2D28;
extern u8 D_800D2CE0[];
extern u8 D_800D2204[];
extern u8 D_800C3D70[];
extern u8 D_800D3688[];
extern void func_8008BC40(u8 flag);
extern void func_8008BC98(u8 index);
extern u32 func_8009A7E4();
extern void func_8009A854();
extern void func_8008AA74(u32 v);
extern u8 func_80085084(u16 arg0, u8 arg1, u8 arg2);

/* func_8008BD50.s, byte-exact: use the command-menu entry at (col, row),
 * index row * 2 + col.  The item is the D_800D2CE0 entry (D_800C3D70 when
 * mode is set); the target word is the halfword at D_800D2204 + 16 * the
 * D_800D2CE0 entry.  An empty item calls func_8008AA74(0x4F) and returns
 * 0.  Mode 0: func_8008BC40(1) and D_800D2D28 +0xC6 = 1, then
 * func_80085084(target, actor, 0); it returns 1 when that commits, else
 * it clears +0xC6, calls func_8008BC98(actor) and returns 0.  Otherwise,
 * when func_8009A7E4(item - 0x32) accepts it, func_8009A854(item) runs,
 * the entry's D_800D3688 count drops by one (clearing its D_800C3D70
 * entry at 0) and it returns 2; else 0.
 * Codegen: func_8009A7E4 and func_8009A854 are declared without
 * prototypes, because retail passes item - 0x32 unmasked and leaves $a1
 * as it is for func_8009A854 (its main55 definition takes two
 * arguments).  The success path falls through to the shared
 * `return ret`, as in retail. */
u8 func_8008BD50(u8 actor, u8 col, u8 row, u8 mode) {
    u16 target;
    u8 item;
    u8 ret;

    target = *(u16*)(D_800D2204 + D_800D2CE0[row * 2 + col] * 16);
    ret = 0;
    if (mode == 0) {
        item = D_800D2CE0[row * 2 + col];
    } else {
        item = D_800C3D70[row * 2 + col];
    }
    if (item != 0) {
        if (mode == 0) {
            func_8008BC40(1);
            D_800D2D28[0xC6] = 1;
            if (func_80085084(target, actor, 0)) {
                ret = 1;
            } else {
                D_800D2D28[0xC6] = 0;
                func_8008BC98(actor);
                return ret;
            }
        } else {
            if (func_8009A7E4(item - 0x32)) {
                func_8009A854(item);
                if (--D_800D3688[row * 2 + col] == 0) {
                    D_800C3D70[row * 2 + col] = 0;
                }
                ret = 2;
            }
            return ret;
        }
    } else {
        func_8008AA74(0x4F);
    }
    return ret;
}
extern u8 D_800D3014;
extern s32 D_800D3288;
extern u8 D_800D39D4;
extern u8 D_800D3670;
extern u8 D_800C3D00;
extern u8* D_800C3EAC;
extern void func_8008F8F4(u8 index, u16 x, u16 y, u16 w, u16 h, u8 animate, u8 refresh);
extern void func_80090B90(s32 x, s32 y, s32* frame, u8* timer);
extern void func_800716D8(void);
extern void func_80077698(void);
extern void func_8007765C(void);
extern void func_80077980(void);
extern void func_8008FA60(u32 v);
extern void func_8008FE18(u8 a, u8 b, u8 init);
extern void func_8009080C(u8 a, u8 b, u8 mode);
extern void func_8009023C(s32 arg0);
extern void func_8009093C(s32 n);

/* func_8008BED8.s, byte-exact: party actor `actor`'s item menu (the
 * func_8008B478 grid: 48 entries in two columns, 16 visible cells, `col` the
 * visible cell, `row` the first visible row, entry row * 2 + col; rows 13
 * pixels apart).  D_800D3288 / D_800D39D4 / D_800D2D28 +0xCB = 0, two
 * windows, func_80077698.  Then until a choice: refill the list
 * (func_8008FE18(actor, mode, first)) when `refresh` asks for it (1: mode 0,
 * 2: mode 1), func_8009080C(col, row, mode) on a cell change,
 * func_8009023C(D_800D3288 / 13 * 2) and func_8009093C on a scroll, the
 * cursor (x 0x2A / 0xAC, y (col / 2) * 13 + 0x63), a frame and the
 * D_800D3014 event: 5 cancels (0); 4 asks func_8008BD50(actor, col, row,
 * mode): 1 commits (D_800C3EAC +0x2E6 = the D_800D2CE0 item, D_800D3670 /
 * D_800C3D00 = col / row, result 1), 2 refills in mode 1; 0 / 2 / 1 / 3
 * move as in func_8008B478, scrolling (D_800D39D4) at the edges, and a
 * pending scroll applies at once (D_800D3288 = new row * 13).  Finally
 * func_8008BC40(result), func_8007765C, func_80077980, func_8008FA60(0, 1).
 * Codegen: the column x is `col % 2 << 7 | (col % 2 * 2 + 0x2A)` (retail
 * ORs the high bit onto the low term); the scroll cases keep the old row
 * (n) and share the D_800D3288 store, as retail's cross-jumped tail. */
u8 func_8008BED8(u8 actor) {
    s32 frame;
    u8 timer;
    u8 col;
    u8 row;
    u8 lastCol;
    u8 lastRow;
    s32 lastY;
    u8 state;
    u8 refresh;
    u8 mode;
    u8 init;
    s32 n;

    col = 0;
    row = 0;
    lastCol = 0xFF;
    lastRow = 0xFF;
    lastY = 0xFFFF;
    state = 2;
    refresh = 1;
    mode = 0;
    frame = 4;
    init = 1;
    timer = 0;
    D_800D3288 = 0;
    D_800D39D4 = 0;
    D_800D2D28[0xCB] = 0;
    func_8008F8F4(0, 0x20, 0x58, 0x118, 0x78, 0, 1);
    func_8008F8F4(1, 0x10, 0x2C, 0x128, 0x28, 0, 1);
    func_80077698();
    while (state == 2) {
        if (refresh != 0) {
            func_8008FE18(actor, refresh - 1, init);
            mode = refresh - 1;
            refresh = 0;
            init = 0;
        }
        if (col != lastCol || row != lastRow) {
            func_8009080C(col, row, mode);
            lastCol = col;
            lastRow = row;
        }
        if (D_800D3288 != lastY) {
            func_8009023C(D_800D3288 / 13 * 2);
            func_8009093C(D_800D3288);
            lastY = D_800D3288;
        }
        func_80090B90(col % 2 << 7 | (col % 2 * 2 + 0x2A), col / 2 * 13 + 0x63, &frame, &timer);
        func_800716D8();
        switch (D_800D3014) {
        case 5:
            state = 0;
            break;
        case 4:
            switch (func_8008BD50(actor, col, row, mode)) {
            case 1:
                D_800C3EAC[0x2E6] = D_800D2CE0[row * 2 + col];
                D_800D3670 = col;
                D_800C3D00 = row;
                state = 1;
                break;
            case 2:
                refresh = 2;
                break;
            }
            break;
        case 0:
            if (row * 2 + col != 0x2F) {
                if (col + 1 == 0x10) {
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
            if (row * 2 + col < 0x2E) {
                if (col + 2 >= 0x10) {
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
            n = row;
            row++;
            col--;
            D_800D3288 = (n + 1) * 13;
            D_800D39D4 = 0;
            break;
        case 2:
            n = row;
            row--;
            col++;
            D_800D3288 = (n - 1) * 13;
            D_800D39D4 = 0;
            break;
        case 3:
            n = row;
            row++;
            D_800D3288 = (n + 1) * 13;
            D_800D39D4 = 0;
            break;
        case 4:
            n = row;
            row--;
            D_800D3288 = (n - 1) * 13;
            D_800D39D4 = 0;
            break;
        }
    }
    func_8008BC40(state);
    func_8007765C();
    func_80077980();
    func_8008FA60(0);
    func_8008FA60(1);
    return state;
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
extern void func_8008FA60(u32 v);
extern void func_800716D8(void);
extern void func_8007765C(void);
extern void func_80077980(void);


/* func_8008C360.s */
void func_8008C360(u8 a) {
    u8* p;
    u8* q;

    p = D_800D2D28;
    p[0x9E] = 0;
    p[0x9D] = 0;
    p[0x9C] = 0;
    q = D_800D2D28;
    q[0xB7] = 0;
    if (a != 0) {
        D_800D2D28[0xC6] = 0;
        func_8008FA60(0);
        func_8008FA60(1);
        func_800716D8();
        func_8007765C();
        func_80077980();
    } else {
        q = D_800D2D28;
        q[0xB1] = 0;
        q[0xB0] = 0;
    }
}
/* func_8008C3F0.s */
void func_8008C3F0(u8 index) {
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
    q[0xB7] = 3;
    r = D_800D2D28;
    r[0xB1] = 1;
    r[0xB0] = 1;
    v = func_80089C08(index & 0xFF);
    off = (u32)index << 6;
    v |= func_80089C08(*(u8*)(D_800C3EAC + off + 0x3C));
    func_800BC404(v & 0xFFFF);
    func_800BCD98(func_80089C08(*(u8*)(D_800C3EAC + off + 0x3C)) & 0xFFFF);
}
