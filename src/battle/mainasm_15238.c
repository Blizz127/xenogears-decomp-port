/* battle mainasm_15238: the retail run at file offset 0x15238, in retail order,
 * split off the front of main35_p4 because its C is gcc 2.6.3 code
 * (BattleGcc263; main35_p4's func_80085084 onward is 2.7.2).  Functions still
 * listed as INCLUDE_ASM are not byte-matching C yet.  The whole file is
 * retail-build only; any port body for these functions lives in the port's
 * own TU. */
#include "common.h"

#ifndef XENO_PC_PORT
extern u8 D_800C3E2C;
extern u16 D_800C3D64;
extern u8 D_800C3EB4[][28];
extern u16 func_80089C08(u8);
extern u16 func_80089C9C(u16, u8);

/* func_80084D28.s, byte-exact: the target mask of the selection's group.
 * Of the 11 combatants, every one that the D_800C3D64 mask accepts
 * (func_80089C9C) and that shares the group (row byte 0 of D_800C3EB4) of
 * the selected combatant D_800C3E2C adds its func_80089C08 bit.  The
 * natural loop matches only under gcc 2.6.x; under 2.7.2 the counter
 * increment stays at the loop end (retail has it in the first branch's
 * delay slot). */
u16 func_80084D28(void) {
    u16 mask = 0;
    s32 i;

    for (i = 0; i < 11; i++) {
        if (func_80089C9C(D_800C3D64, i) != 0) {
            if (D_800C3EB4[D_800C3E2C][0] == D_800C3EB4[i][0]) {
                mask |= func_80089C08(i);
            }
        }
    }
    return mask;
}

/* func_80084DE4.s, byte-exact: target-selection setup (main35_p4's
 * func_80085084 calls it as (arg0, arg0, arg1, 0, arg2)).  mode 0 takes
 * flags from alt, 1 forces 0x3000, 2 forces 0x2001.  Bits 0x1000/0x2000
 * pick the side and kind passed to func_80084548 and the D_800C3D64 mask
 * kept (0xFFF8, 7 or 0xFFFF).  D_800C3E2C = func_80084750(actor) for a
 * single pick, else func_80084548(kind, enemy, side) with enemy = bit 15.
 * Bit 15 (every target): D_800C3E2C = 0xFF, then each of the 11
 * combatants the mask accepts (func_80089C9C) whose status halfword has
 * bit 15 set (D_800CCD64's or D_800CCE08's, as D_800D32A1's row byte 0
 * picks) becomes D_800C3E2C and joins the D_800C3E90 list; the others
 * leave the mask (func_80089C48).  The list is padded with 0xFF.  Bit 14
 * instead: the actor alone (mask func_80089C08(actor), list all 0xFF).
 * Returns flags.
 * CONSTRUCT: the scan loop sits in a do { } while (0).  flow weights each
 * reference by loop depth, so the extra level raises id's priority above
 * n's (2.25 vs 2.08) and global.c hands out retail's s-registers (id s0,
 * n s1, k s2, i s4); without it n outranks id (24/168).  Also needed: n
 * counts the bit-14 clear loop (retail's countdown is in n's s1), and
 * n = 0 comes before the 0xFF store: s1 still holds actor on the taken
 * path, so reorg cannot put n = 0 in the branch slot and takes the 0xFF
 * load as in retail.  The clear loop is written up-counting;
 * check_dbra_loop reverses it into retail's D_800C3E9A pointer loop. */
extern u8 D_800D32A1[][8];
extern u16 D_800CCD64[][184];
extern u16 D_800CCE08[][184];
extern u8 D_800C3E90[];
extern u32 func_80084750(u32 arg0);
extern u32 func_80084548(u8 mode, u8 actor, u8 flag);
extern u32 func_80089C48(u8 index);

u16 func_80084DE4(u16 flags, u16 alt, u8 actor, u8 mode, u8 single) {
    u8 enemy = 0;
    u8 side;
    u8 kind;
    u16 keep;
    s32 i;
    s32 n;

    switch (mode) {
    case 0:
        flags = alt;
        break;
    case 1:
        flags = 0x3000;
        break;
    case 2:
        flags = 0x2001;
        break;
    }
    if (flags & 0x8000) {
        enemy = 1;
    }
    if (flags & 0x1000) {
        side = 0;
        kind = 0;
        keep = 0xFFF8;
    } else {
        side = 1;
        kind = 1;
        keep = 7;
    }
    if (flags & 0x2000) {
        kind = 2;
        keep = 0xFFFF;
    }
    if (single) {
        D_800C3E2C = func_80084750(actor);
    } else {
        D_800C3E2C = func_80084548(kind, enemy, side);
    }
    D_800C3D64 &= keep;
    if (flags & 0x8000) {
        n = 0;
        D_800C3E2C = 0xFF;
        do {
            for (i = 0; i < 11; i++) {
                u8 id = i;
                s32 k = id;

                if (func_80089C9C(D_800C3D64, k) != 0) {
                    if (D_800D32A1[i][0] == 0 ? (D_800CCD64[i][0] & 0x8000) : (D_800CCE08[i][0] & 0x8000)) {
                        D_800C3E2C = id;
                        D_800C3E90[n++] = id;
                    } else {
                        D_800C3D64 &= func_80089C48(k);
                    }
                }
            }
        } while (0);
        for (; n < 11; n++) {
            D_800C3E90[n] = 0xFF;
        }
    } else if (flags & 0x4000) {
        D_800C3E2C = actor;
        D_800C3D64 = func_80089C08(actor);
        for (n = 0; n < 11; n++) {
            D_800C3E90[n] = 0xFF;
        }
    }
    return flags;
}
#endif
