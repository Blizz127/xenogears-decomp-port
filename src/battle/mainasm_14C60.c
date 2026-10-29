/* battle mainasm_14C60: the retail-asm run at file offset 0x14C60, in retail order.
 * Functions still listed as INCLUDE_ASM are not byte-matching C yet. The whole
 * file is retail-build only; any port body for these functions lives in the
 * port's own TU. */
#include "common.h"

#ifndef XENO_PC_PORT
extern u16 D_800C3D64;
extern u8 D_800C3E90[];
extern u8 D_800D3274;
extern u32 func_80083FF4(u32 actor, u32 target);
extern u32 func_80089C08(u32 v);

/* Byte-exact under gcc 2.6.3 (BattleGcc263; the port body is in main35.c).
 * Fill the 12-byte target list D_800C3E90 with 0xFF (from the end), then
 * for targets 3..10 keep those func_80083FF4(arg0, target) accepts:
 * append each to the list, OR func_80089C08(target) into D_800C3D64 and
 * count them in D_800D3274.  Returns the first list entry. */
u32 func_80084750(u32 arg0) {
    s32 remaining = 8;
    s32 fill = 0xFF;
    s32 i = 11;
    s32 output;
    u8* clear = &D_800C3E90[11];

    for (; i >= 0; i--) {
        *clear = fill;
        clear--;
    }
    output = 0;
    remaining--;
    D_800D3274 = 0;
    D_800C3D64 = 0;
    for (i = 3; remaining >= 0; remaining--) {
        u8 target = i;
        u32 eligible = func_80083FF4(arg0 & 0xFF, target);

        if (eligible & 0xFF) {
            u32 mask;

            D_800C3E90[output] = target;
            mask = func_80089C08(target);
            output++;
            D_800C3D64 |= mask;
            D_800D3274++;
        }
        i++;
    }
    return D_800C3E90[0];
}
extern u16 D_800C3EBE__rows[][14] asm("D_800C3EBE");
extern u16 D_800C3EC0__rows[][14] asm("D_800C3EC0");

/* func_80084854.s, byte-exact: the actor that cursor direction `dir` moves
 * to from `actor`.  Among the D_800C3E90 slots (0xFF = empty, `actor`
 * skipped), keep those whose ratan2 angle from `actor` (D_800C3EC0 z,
 * D_800C3EBE x halfwords of the 28-byte records) falls in dir's quarter
 * turn: 0 [-0x200, 0x200), 1 [-0x600, -0x200), 3 [0x200, 0x600), 2 the
 * rest ([-0x800, -0x600) or [0x600, 0x800]); return the nearest by
 * squared distance (`actor` itself when none qualifies).  Codegen: the records go through asm-label row
 * views; each squared term is computed in both arms of its sign test,
 * which reloads the slot byte after each join as retail does.  Matches
 * under gcc 2.6.x (2.7.2: diff 7/138); the port body is in main35.c. */
u8 func_80084854(u8 actor, u8 dir) {
    s32 best = 0xFFFFFF;
    u8 result = actor;
    s32 i;
    s32 found;
    s32 angle;
    s32 dz;
    s32 dx;
    s32 dist;

    for (i = 0; i < 11; i++) {
        if (D_800C3E90[i] != 0xFF && D_800C3E90[i] != actor) {
            found = 0;
            angle = ratan2(D_800C3EC0__rows[D_800C3E90[i]][0] - D_800C3EC0__rows[actor][0],
                           D_800C3EBE__rows[D_800C3E90[i]][0] - D_800C3EBE__rows[actor][0]);
            switch (dir) {
            case 0:
                if ((u16)(angle + 0x200) < 0x400) {
                    found = 1;
                }
                break;
            case 1:
                if ((u16)(angle + 0x600) < 0x400) {
                    found = 1;
                }
                break;
            case 2:
                if ((u16)(angle + 0x800) < 0x200) {
                    found = 1;
                }
                if ((u16)(angle - 0x600) < 0x201) {
                    found = 1;
                }
                break;
            case 3:
                if ((u16)(angle - 0x200) < 0x400) {
                    found = 1;
                }
                break;
            }
            if (found) {
                dz = D_800C3EC0__rows[D_800C3E90[i]][0] - D_800C3EC0__rows[actor][0];
                if (dz < 0) {
                    dist = (D_800C3EC0__rows[actor][0] - D_800C3EC0__rows[D_800C3E90[i]][0]) *
                           (D_800C3EC0__rows[actor][0] - D_800C3EC0__rows[D_800C3E90[i]][0]);
                } else {
                    dist = dz * dz;
                }
                dx = D_800C3EBE__rows[D_800C3E90[i]][0] - D_800C3EBE__rows[actor][0];
                if (dx < 0) {
                    dist += (D_800C3EBE__rows[actor][0] - D_800C3EBE__rows[D_800C3E90[i]][0]) *
                            (D_800C3EBE__rows[actor][0] - D_800C3EBE__rows[D_800C3E90[i]][0]);
                } else {
                    dist += dx * dx;
                }
                if (dist < best) {
                    best = dist;
                    result = D_800C3E90[i];
                }
            }
        }
    }
    return result;
}
#endif
