/* battle mainasm_15898: the retail-asm run at file offset 0x15898, in retail order.
 * Functions still listed as INCLUDE_ASM are not byte-matching C yet. The whole
 * file is retail-build only; any port body for these functions lives in the
 * port's own TU. */
#include "common.h"

#ifndef XENO_PC_PORT
/* One of the 32 72-byte action rows at D_800C3FE8. */
typedef struct {
    u16 unk0[12];
    u8 unk18[12];
    u16 unk24[12];
    u8 unk3C[12];
} BattleActionRow;

extern BattleActionRow D_800C3FE8[];
extern u8* D_800C3EAC;

/* func_80085388.s: clear entries 0..10 of the current action row
 * (D_800C3EAC[0x2DA]): the two halfword arrays to 0, the two byte arrays to
 * 0xFF.  The row index is re-read for every store, as retail does. */
void func_80085388(void) {
    s32 i;

    for (i = 0; i < 11; i++) {
        D_800C3FE8[D_800C3EAC[0x2DA]].unk0[i] = 0;
        D_800C3FE8[D_800C3EAC[0x2DA]].unk18[i] = 0xFF;
        D_800C3FE8[D_800C3EAC[0x2DA]].unk24[i] = 0;
        D_800C3FE8[D_800C3EAC[0x2DA]].unk3C[i] = 0xFF;
    }
}

/* func_80085454.s, byte-exact: for slots 0..10 copy the incoming amount
 * (the halfword every 4 bytes at D_800D2C54) and its kind (D_800D2C88)
 * into action row `row`, fold the amount into the slot's running total
 * D_800D2D70 according to the running kind D_800D2D5C, and mirror the
 * total and kind into the row (+0x24/+0x3C):
 * - kind 0/5 onto running 0/5, or kind 2 onto running 2: add;
 * - kind 0/5 onto running 2, or kind 2 onto running 0/5: the total
 *   becomes |amount - total| (signed halfwords), and the running kind
 *   becomes 0 (resp. 2) unless the old total was larger;
 * - any other running kind: total = amount, running kind = kind.
 * Other incoming kinds (1, 3, 4, 6..) only copy.  Codegen:
 * - The two netting blocks are inline helpers taking the halfwords as
 *   s16 parameters.  Combine then turns each parameter's load plus the
 *   test's sign extension into one `lh` and a register copy (-dc: the
 *   sign_extend load, then the parameter as its subreg), as in retail:
 *   the test runs on the `lh` values and the arms on the copies.  With
 *   plain s16 locals the variable keeps its own load (lhu) and the test
 *   gets a second `lh`.  One helper per kind constant: with the kind as
 *   a third parameter it stays in its own register and costs an s0 save
 *   (8 bytes longer).
 * - The four dead extension temps of the s16 tests make retail's empty
 *   0x20 frame.
 * - The plain int loop is biv-eliminated against the D_800D2D5C pointer
 *   (-dL: "biv 74 was eliminated"; signed slt against D_800D2D5C + 11 =
 *   D_800D2D67), as in retail. */
extern u8 D_800D2C54[];
extern u8 D_800D2C88[];
extern u8 D_800D2D5C[];
extern u16 D_800D2D70[];

static inline void F85454_Net0(s32 i, s16 n, s16 o) {
    if (n - o < 0) {
        D_800D2D70[i] = o - n;
    } else {
        D_800D2D70[i] = n - o;
        D_800D2D5C[i] = 0;
    }
}

static inline void F85454_Net2(s32 i, s16 n, s16 o) {
    if (n - o < 0) {
        D_800D2D70[i] = o - n;
    } else {
        D_800D2D70[i] = n - o;
        D_800D2D5C[i] = 2;
    }
}

void func_80085454(u8 row) {
    s32 i;
    u8 w;

    for (i = 0; i < 11; i++) {
        D_800C3FE8[row].unk0[i] = *(u16*)(D_800D2C54 + i * 4);
        D_800C3FE8[row].unk18[i] = D_800D2C88[i];
        switch (D_800D2C88[i]) {
        case 0:
        case 5:
            w = D_800D2D5C[i];
            if (w == 0 || w == 5) {
                D_800D2D70[i] += *(u16*)(D_800D2C54 + i * 4);
            } else if (w == 2) {
                F85454_Net0(i, *(s16*)(D_800D2C54 + i * 4), (s16)D_800D2D70[i]);
            } else {
                D_800D2D70[i] = *(u16*)(D_800D2C54 + i * 4);
                D_800D2D5C[i] = D_800D2C88[i];
            }
            break;
        case 2:
            w = D_800D2D5C[i];
            if (w == 2) {
                D_800D2D70[i] += *(u16*)(D_800D2C54 + i * 4);
            } else if (w == 0 || w == 5) {
                F85454_Net2(i, *(s16*)(D_800D2C54 + i * 4), (s16)D_800D2D70[i]);
            } else {
                D_800D2D70[i] = *(u16*)(D_800D2C54 + i * 4);
                D_800D2D5C[i] = D_800D2C88[i];
            }
            break;
        }
        D_800C3FE8[row].unk24[i] = D_800D2D70[i];
        D_800C3FE8[row].unk3C[i] = D_800D2D5C[i];
    }
}

typedef struct {
    u8 pad0[0x4C];
    s16 hp;
    u16 hpMax;
    s16 mp;
    u16 mpMax;
    u8 pad54[0x28];
    u16 flags;
    u8 pad7E[0x5E];
    u16 fDC;
    u16 fDE;
    u8 padE0[0x24];
    u32 f104;
    u32 f108;
    u8 pad10C[0x14];
    u16 f120;
    u8 pad122[0x4E];
} R618Actor;

typedef struct {
    u16 amt[12];
    u8 kind[0x30];
} R618Cmd;

typedef struct {
    u8 pad0[4];
    u8 rows[11][28];
    R618Cmd cmd[32];
    u16 mask;
    u8 padA3A[0x8E38 - 0xA3A];
    R618Actor actors[11];
} R618State;

extern R618State D_800C3EB0__st asm("D_800C3EB0");
extern R618Actor D_800CCCE8__a[] asm("D_800CCCE8");
extern u8 D_800C4000__k[][0x48] asm("D_800C4000");
extern u8 D_800D2DCC[];
extern u16 D_800C48E8[];
extern u8 D_800C2050;
extern u16 func_80089C08(u8 v);
extern void func_800883AC(u8 a);

/* func_80085618.s: apply action row a0's amounts to battle slots 0..10.
 * Empty slots (D_800D2DCC[i] == 0) are skipped; a slot whose actor flags
 * have bit 15 only re-sets its D_800C48E8 bit.  Otherwise the row kind
 * picks the effect: 0/5/7/8 damage (hp, or f104 when the slot's row flag
 * +4 is set; at zero the slot is knocked out, and slots >= 3 also call
 * func_800883AC), 2 hp/f104 heal and 3 mp heal with max clamps, 1/9 mp
 * damage, 10 fDC damage and 11 fDC heal.  The cases that break mark
 * D_800C3EAC[0x2EB + i].  Each one stores its own mark: the extra uses
 * of i make global-alloc give it s2 ahead of the amount walker, as in
 * retail, and cross-jumping folds the copies into one tail. */
void func_80085618(u8 a0) {
    s32 i;
    s32 v;

    for (i = 0; i < 11; i++) {
        if (D_800D2DCC[i] == 0) {
            continue;
        }
        if (D_800CCCE8__a[i].flags & 0x8000) {
            D_800C48E8[0] |= func_80089C08(i);
            continue;
        }
        switch (D_800C4000__k[a0][i]) {
        case 0:
        case 5:
        case 7:
        case 8:
            if (D_800C3EB0__st.rows[i][4] == 0) {
                if (D_800CCCE8__a[i].hp - (s16)D_800C3EB0__st.cmd[a0].amt[i] > 0) {
                    D_800CCCE8__a[i].hp -= D_800C3EB0__st.cmd[a0].amt[i];
                } else {
                    D_800CCCE8__a[i].hp = 0;
                    D_800C3EB0__st.mask |= func_80089C08(i);
                    D_800CCCE8__a[i].flags |= 0x8000;
                    if (i >= 3) {
                        func_800883AC(i);
                    }
                }
            } else {
                v = D_800CCCE8__a[i].f104 - D_800C3EB0__st.cmd[a0].amt[i];
                if (v > 0) {
                    D_800CCCE8__a[i].f104 = v;
                } else {
                    D_800CCCE8__a[i].f104 = 0;
                    D_800C3EB0__st.mask |= func_80089C08(i);
                    D_800CCCE8__a[i].f120 |= 0x8000;
                    D_800CCCE8__a[i].flags |= 0x8000;
                    if (i >= 3) {
                        func_800883AC(i);
                    }
                }
            }
            (D_800C3EAC + i)[0x2EB] = 1;
            break;
        case 2:
            if (D_800C3EB0__st.rows[i][4] != 0 && D_800C2050 == 0) {
                if (D_800CCCE8__a[i].f108 < (D_800C3EB0__st.actors[i].f104 = D_800C3EB0__st.cmd[a0].amt[i] + D_800CCCE8__a[i].f104)) {
                    D_800CCCE8__a[i].f104 = D_800CCCE8__a[i].f108;
                }
            } else {
                if (D_800CCCE8__a[i].hpMax < (u16)(D_800C3EB0__st.actors[i].hp = D_800CCCE8__a[i].hp + D_800C3EB0__st.cmd[a0].amt[i])) {
                    D_800CCCE8__a[i].hp = D_800CCCE8__a[i].hpMax;
                }
            }
            (D_800C3EAC + i)[0x2EB] = 1;
            break;
        case 1:
        case 9:
            if (D_800CCCE8__a[i].mp - (s16)D_800C3EB0__st.cmd[a0].amt[i] > 0) {
                D_800CCCE8__a[i].mp -= D_800C3EB0__st.cmd[a0].amt[i];
            } else {
                D_800CCCE8__a[i].mp = 0;
            }
            continue;
        case 3:
            if (D_800C3EB0__st.rows[i][4] == 0 || D_800C2050 != 0) {
                if (D_800CCCE8__a[i].mpMax < (u16)(D_800C3EB0__st.actors[i].mp = D_800CCCE8__a[i].mp + D_800C3EB0__st.cmd[a0].amt[i])) {
                    D_800CCCE8__a[i].mp = D_800CCCE8__a[i].mpMax;
                }
            }
            continue;
        case 10:
            if (D_800CCCE8__a[i].fDC - D_800C3EB0__st.cmd[a0].amt[i] > 0) {
                D_800CCCE8__a[i].fDC -= D_800C3EB0__st.cmd[a0].amt[i];
            } else {
                D_800CCCE8__a[i].fDC = 0;
            }
            (D_800C3EAC + i)[0x2EB] = 1;
            break;
        case 11:
            if (D_800CCCE8__a[i].fDE < (u16)(D_800C3EB0__st.actors[i].fDC = D_800CCCE8__a[i].fDC + D_800C3EB0__st.cmd[a0].amt[i])) {
                D_800CCCE8__a[i].fDC = D_800CCCE8__a[i].fDE;
            }
            (D_800C3EAC + i)[0x2EB] = 1;
            break;
        default:
            continue;
        }
    }
}


extern u8 D_800CCD34[];
extern u8 D_800CCD36[];
extern u8 D_800CCD68[];
extern u8 D_800CCD74[];

/* func_80085AC4.s: for actor a0 (368-byte stride) copy the halfword at
 * D_800CCD36 to D_800CCD34, then zero halfwords backwards, 4 bytes apart:
 * two from D_800CCD68 and three from D_800CCD74.  Each counter is set
 * before its pointer.  sched1 then keeps the D_800CCD68 address live across
 * the halfword copy, as retail does, and the copy lands in $a0, so the
 * parameter moves to $a2.  Setting the counter after the pointer is one
 * instruction short. */
void func_80085AC4(u8 a0) {
    s32 i;
    u16* p;

    i = 2;
    p = (u16*)(D_800CCD68 + a0 * 368);
    *(u16*)(D_800CCD34 + a0 * 368) = *(u16*)(D_800CCD36 + a0 * 368);
    do {
        *p = 0;
        p -= 2;
        i -= 2;
    } while (i >= 0);
    i = 4;
    p = (u16*)(D_800CCD74 + a0 * 368);
    do {
        *p = 0;
        p -= 2;
        i -= 2;
    } while (i >= 0);
}
#endif
