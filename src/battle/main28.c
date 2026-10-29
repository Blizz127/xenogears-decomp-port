#include "common.h"
#include "battle_guest_abi.h"


#ifndef XENO_PC_PORT
extern u8 D_800D3420[];
extern u8 D_800D32A1[][8];
extern u32 func_8007A628(u8 a0, u8 a1);
extern u32 func_8007A280(u8 a0, u8 a1, u32 a2, u32 a3);
extern u32 func_8001BD40(u32 lo, u8 hi);
extern u32 func_80089C08(u8 idx);

/* func_8007D8C0 .. func_8007DFD4, byte-exact: six battle-script ops on
 * p = *ppBoard that pick a random combatant.  Each clears halfword p[2]
 * of row `index` of D_800D3420, collects the slots i that
 * func_8007A628(i, k) accepts (func_8007DB78/DCF8/DFD4: and whose
 * D_800D32A1 byte is set) and whose func_8007A280(i, p[1], 0, 1) mask
 * meets halfword p[3] of the row, and stores func_80089C08 of a random one
 * (func_8001BD40(0, count - 1)) in halfword p[2]:
 *   func_8007D8C0  slots 0..2,  k = 0
 *   func_8007DA1C  slots 3..10, k = 0
 *   func_8007DB78  slots 0..2,  k = 0, D_800D32A1
 *   func_8007DCF8  slots 3..10, k = 0, D_800D32A1
 *   func_8007DE78  slots 0..2,  k = 1
 *   func_8007DFD4  slots 0..2,  k = 1, D_800D32A1
 * gcc 2.6.x code (main28 is BattleGcc263).  The loop reads the row through
 * its own pointer `vars`, set after the func_8007A280 call: retail copies
 * the row pointer into an s-register before the first store, and reading
 * through `row` in the loop computes it there directly (func_8007D8C0: 344
 * vs 348 bytes). */
void func_8007D8C0(u8** ppBoard, u8 index) {
    u8 list[8];
    s32 i = 0;
    s32 n = 0;
    u16* row;
    u16* vars;
    u16* dst;
    u16 v;
    u32 m;

    row = (u16*)(D_800D3420 + ((u32)index << 6));
    row[(*ppBoard)[2]] = 0;
    for (; i < 3; i++) {
        if ((u8)func_8007A628(i, 0) != 0) {
            m = func_8007A280(i, (*ppBoard)[1], 0, 1);
            vars = (u16*)(D_800D3420 + ((u32)index << 6));
            if (vars[(*ppBoard)[3]] & m) {
                list[n] = i;
                n++;
            }
        }
    }
    if (n != 0) {
        v = func_80089C08(list[(u8)func_8001BD40(0, n - 1)]);
        dst = (u16*)(D_800D3420 + ((u32)index << 6));
        dst[(*ppBoard)[2]] = v;
    }
}
void func_8007DA1C(u8** ppBoard, u8 index) {
    u8 list[8];
    s32 n = 0;
    s32 i = 3;
    u16* row;
    u16* vars;
    u16* dst;
    u16 v;
    u32 m;

    row = (u16*)(D_800D3420 + ((u32)index << 6));
    row[(*ppBoard)[2]] = 0;
    for (; i < 11; i++) {
        if ((u8)func_8007A628(i, 0) != 0) {
            m = func_8007A280(i, (*ppBoard)[1], 0, 1);
            vars = (u16*)(D_800D3420 + ((u32)index << 6));
            if (vars[(*ppBoard)[3]] & m) {
                list[n] = i;
                n++;
            }
        }
    }
    if (n != 0) {
        v = func_80089C08(list[(u8)func_8001BD40(0, n - 1)]);
        dst = (u16*)(D_800D3420 + ((u32)index << 6));
        dst[(*ppBoard)[2]] = v;
    }
}
void func_8007DB78(u8** ppBoard, u8 index) {
    u8 list[8];
    s32 i = 0;
    s32 n = 0;
    u16* row;
    u16* vars;
    u16* dst;
    u16 v;
    u32 m;

    row = (u16*)(D_800D3420 + ((u32)index << 6));
    row[(*ppBoard)[2]] = 0;
    for (; i < 3; i++) {
        if ((u8)func_8007A628(i, 0) != 0 && D_800D32A1[i][0] != 0) {
            m = func_8007A280(i, (*ppBoard)[1], 0, 1);
            vars = (u16*)(D_800D3420 + ((u32)index << 6));
            if (vars[(*ppBoard)[3]] & m) {
                list[n] = i;
                n++;
            }
        }
    }
    if (n != 0) {
        v = func_80089C08(list[(u8)func_8001BD40(0, n - 1)]);
        dst = (u16*)(D_800D3420 + ((u32)index << 6));
        dst[(*ppBoard)[2]] = v;
    }
}
void func_8007DCF8(u8** ppBoard, u8 index) {
    u8 list[8];
    s32 i = 3;
    s32 n = 0;
    u16* row;
    u16* vars;
    u16* dst;
    u16 v;
    u32 m;

    row = (u16*)(D_800D3420 + ((u32)index << 6));
    row[(*ppBoard)[2]] = 0;
    for (; i < 11; i++) {
        if ((u8)func_8007A628(i, 0) != 0 && D_800D32A1[i][0] != 0) {
            m = func_8007A280(i, (*ppBoard)[1], 0, 1);
            vars = (u16*)(D_800D3420 + ((u32)index << 6));
            if (vars[(*ppBoard)[3]] & m) {
                list[n] = i;
                n++;
            }
        }
    }
    if (n != 0) {
        v = func_80089C08(list[(u8)func_8001BD40(0, n - 1)]);
        dst = (u16*)(D_800D3420 + ((u32)index << 6));
        dst[(*ppBoard)[2]] = v;
    }
}
void func_8007DE78(u8** ppBoard, u8 index) {
    u8 list[8];
    s32 i = 0;
    s32 n = 0;
    u16* row;
    u16* vars;
    u16* dst;
    u16 v;
    u32 m;

    row = (u16*)(D_800D3420 + ((u32)index << 6));
    row[(*ppBoard)[2]] = 0;
    for (; i < 3; i++) {
        if ((u8)func_8007A628(i, 1) != 0) {
            m = func_8007A280(i, (*ppBoard)[1], 0, 1);
            vars = (u16*)(D_800D3420 + ((u32)index << 6));
            if (vars[(*ppBoard)[3]] & m) {
                list[n] = i;
                n++;
            }
        }
    }
    if (n != 0) {
        v = func_80089C08(list[(u8)func_8001BD40(0, n - 1)]);
        dst = (u16*)(D_800D3420 + ((u32)index << 6));
        dst[(*ppBoard)[2]] = v;
    }
}
void func_8007DFD4(u8** ppBoard, u8 index) {
    u8 list[8];
    s32 i = 0;
    s32 n = 0;
    u16* row;
    u16* vars;
    u16* dst;
    u16 v;
    u32 m;

    row = (u16*)(D_800D3420 + ((u32)index << 6));
    row[(*ppBoard)[2]] = 0;
    for (; i < 3; i++) {
        if ((u8)func_8007A628(i, 1) != 0 && D_800D32A1[i][0] != 0) {
            m = func_8007A280(i, (*ppBoard)[1], 0, 1);
            vars = (u16*)(D_800D3420 + ((u32)index << 6));
            if (vars[(*ppBoard)[3]] & m) {
                list[n] = i;
                n++;
            }
        }
    }
    if (n != 0) {
        v = func_80089C08(list[(u8)func_8001BD40(0, n - 1)]);
        dst = (u16*)(D_800D3420 + ((u32)index << 6));
        dst[(*ppBoard)[2]] = v;
    }
}
#endif
void func_80087EDC(s32, s32, s32);               /* extern */
void func_800883AC(s32, void *, s32);            /* extern */
#ifndef XENO_PC_PORT
extern u8 D_800C3EB7[];
#endif

void func_8007E154(GSLOT(void) arg0, s32 arg1) {
    s32 temp_a0;
    s32 temp_a0_2;
    s32 idx;
    void *temp_a1;

    temp_a1 = GLOAD(void, arg0);
    idx = (arg1 & 0xFF) + 3;
    D_800C3EB7[idx * 0x1C] = *(u8 *)((s8*)(temp_a1) + 1);
    temp_a0 = arg1 + 3;
    if (*(u8 *)((s8*)(GLOAD(void, arg0)) + 1) & 0x80) {
        temp_a0_2 = temp_a0 & 0xFF;
        func_80087EDC(temp_a0_2, temp_a0_2, arg1);
        return;
    }
    func_800883AC(temp_a0 & 0xFF, temp_a1, arg1);
}
#ifndef XENO_PC_PORT

extern u8 D_800D3420[];
extern u32 func_80089C08(u8 idx);

/* func_8007E1D0: T[p[1]] = (u16)func_80089C08((idx+3)&0xFF), T = D_800D3420
 * row base (idx&0xFF)<<6. func_80089C08 is declared with varying return
 * width across TUs (u16 in main35.c/main38.c/main36.c, u32 in main27.c/
 * main46.c); u32 matches src/battle/main27.c's declaration and the call
 * result is truncated to u16 at the store, same as that TU's pattern.
 * Straightforward translation matched first try, no register pins needed. */
void func_8007E1D0(u8 **pp, s32 idx) {
    u16 result;
    u32 row;
    u8 *p;
    u32 off;

    result = (u16)func_80089C08((idx + 3) & 0xFF);
    row = (idx & 0xFF) << 6;
    p = *pp;
    off = p[1];
    row += (u32)D_800D3420;
    off <<= 1;
    *(u16 *)(off + row) = result;
}

extern u8 D_800D32A1[][8];
extern u32 func_8007A744(u8 index);

/* func_8007E234.s, byte-exact: battle-script op building a 3-bit party
 * mask into halfword p[1] of row `index` of D_800D3420 (p = *ppBoard).
 * For party members 2, 1, 0 that func_8007A744 accepts, the bit is set
 * by mode p[2]: 1 when the member's D_800D32A1 entry is set, 2 when it is
 * clear, any other mode always.  The mask is shifted once per member and
 * stored >> 1. */
void func_8007E234(u8** ppBoard, u8 index) {
    u16 flags = 0;
    s32 i;
    u16* row;

    for (i = 2; i >= 0; i--) {
        if ((u8)func_8007A744(i) != 0) {
            switch ((*ppBoard)[2]) {
            case 1:
                if (D_800D32A1[i][0] != 0) {
                    flags |= 1;
                }
                break;
            case 2:
                if (D_800D32A1[i][0] == 0) {
                    flags |= 1;
                }
                break;
            default:
                flags |= 1;
                break;
            }
        }
        flags <<= 1;
    }
    row = (u16*)(D_800D3420 + ((u32)index << 6));
    row[(*ppBoard)[1]] = flags >> 1;
}
/* func_8007E334.s, byte-exact: func_8007E234 for the enemies.  Members
 * 10 down to 3 give bits 10..3 of the mask (shifted per member, stored
 * << 2). */
void func_8007E334(u8** ppBoard, u8 index) {
    u16 flags = 0;
    s32 i;
    u16* row;

    for (i = 10; i >= 3; i--) {
        if ((u8)func_8007A744(i) != 0) {
            switch ((*ppBoard)[2]) {
            case 1:
                if (D_800D32A1[i][0] != 0) {
                    flags |= 1;
                }
                break;
            case 2:
                if (D_800D32A1[i][0] == 0) {
                    flags |= 1;
                }
                break;
            default:
                flags |= 1;
                break;
            }
        }
        flags <<= 1;
    }
    row = (u16*)(D_800D3420 + ((u32)index << 6));
    row[(*ppBoard)[1]] = flags << 2;
}
extern u8 D_800C3EB4[][28];
extern u32 func_80079E7C(u16 value);

/* func_8007E438.s, byte-exact: battle-script op building a party mask into
 * halfword p[1] of row `index` of D_800D3420 (p = *ppBoard): for members
 * 2, 1, 0 that func_8007A744 accepts, the bit is set when the member is in
 * the same group (row byte 0 of D_800C3EB4) as character
 * func_80079E7C(halfword p[2] of the row).  gcc 2.6.x code (main28 is
 * BattleGcc263): under 2.7.2 the counter decrement leaves the first
 * branch's delay slot.  `i` is initialised before `row`, and the store
 * goes through its own row pointer. */
void func_8007E438(u8** ppBoard, u8 index) {
    u16 flags = 0;
    s32 i = 2;
    u16* row = (u16*)(D_800D3420 + ((u32)index << 6));
    u16* dst;

    for (; i >= 0; i--) {
        if ((u8)func_8007A744(i) != 0) {
            if (D_800C3EB4[(u8)func_80079E7C(row[(*ppBoard)[2]])][0] == D_800C3EB4[i][0]) {
                flags |= 1;
            }
        }
        flags <<= 1;
    }
    dst = (u16*)(D_800D3420 + ((u32)index << 6));
    dst[(*ppBoard)[1]] = flags >> 1;
}
/* func_8007E554.s, byte-exact: func_8007E438 for the enemies (members 10
 * down to 3, stored << 2). */
void func_8007E554(u8** ppBoard, u8 index) {
    u16 flags = 0;
    s32 i = 10;
    u16* row = (u16*)(D_800D3420 + ((u32)index << 6));
    u16* dst;

    for (; i >= 3; i--) {
        if ((u8)func_8007A744(i) != 0) {
            if (D_800C3EB4[(u8)func_80079E7C(row[(*ppBoard)[2]])][0] == D_800C3EB4[i][0]) {
                flags |= 1;
            }
        }
        flags <<= 1;
    }
    dst = (u16*)(D_800D3420 + ((u32)index << 6));
    dst[(*ppBoard)[1]] = flags << 2;
}
#endif


#ifndef XENO_PC_PORT
extern u32 D_800D2C60[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2C8B[];
#endif


/* func_8007E674.s */
void func_8007E674(u8 idx) {
    D_800D2C60[idx] = 0;
    D_800D2C8B[idx] = 4;
}
