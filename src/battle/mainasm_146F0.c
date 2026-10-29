/* battle mainasm_146F0: the retail-asm run at file offset 0x146F0, in retail order.
 * Functions still listed as INCLUDE_ASM are not byte-matching C yet. The whole
 * file is retail-build only; any port body for these functions lives in the
 * port's own TU. */
#include "common.h"

#ifndef XENO_PC_PORT
typedef struct {
    u8 group;
    u8 unk1[0x1B];
} M146Group; /* 0x1C */
typedef struct {
    u16 rank;
    u8 unk2[0x16E];
} M146Rank; /* 0x170 */
extern u8 D_800D3274;
extern u8 D_800C3E90[];
extern M146Group D_800C3EB4[];
extern M146Rank D_800CCD34[];
extern u8 func_80083FF4(u8 side, u8 id);

/* func_800841E0.s: list the targets func_80083FF4 accepts for `side`
 * (3..10 when side < 3, else 0..2) into D_800C3E90, counting them in
 * D_800D3274.  Targets in side's group come first, the rest keep their
 * order; then entry 0 is swapped with each later entry of lower rank (all
 * entries, or only the same-group ones when entry 0 is in the group).
 * Returns entry 0.  The port arm is src/battle/target_list_impl.inc. */
u8 func_800841E0(u8 side) {
    u8 candidates[12];
    u8 matching[12];
    s32 i;
    s32 out;
    s32 n;
    u8* p;
    u8 tmp;

    D_800D3274 = 0;
    for (i = 0; i < 12; i++) {
        candidates[i] = 0xFF;
        D_800C3E90[i] = 0xFF;
        matching[i] = 0;
    }
    if (side < 3) {
        for (i = 3, n = 0, p = candidates; i < 11; i++) {
            if (func_80083FF4(side, i)) {
                *p++ = i;
                n++;
                D_800D3274++;
            }
        }
    } else {
        for (i = 0, n = 0, p = candidates; i < 3; i++) {
            if (func_80083FF4(side, i)) {
                *p++ = i;
                n++;
                D_800D3274++;
            }
        }
    }
    out = 0;
    for (i = 0; i < n; i++) {
        if (D_800C3EB4[side].group == D_800C3EB4[candidates[i]].group) {
            D_800C3E90[out] = candidates[i];
            candidates[i] = 0xFF;
            matching[out] = 1;
            out++;
        }
    }
    for (i = 0; i < n; i++) {
        if (candidates[i] != 0xFF) {
            D_800C3E90[out] = candidates[i];
            out++;
        }
    }
    if (matching[0] != 0) {
        for (i = 1; i < n; i++) {
            if (matching[i] != 0 &&
                D_800CCD34[D_800C3E90[0]].rank > D_800CCD34[D_800C3E90[i]].rank) {
                tmp = D_800C3E90[0];
                D_800C3E90[0] = D_800C3E90[i];
                D_800C3E90[i] = tmp;
            }
        }
    } else {
        for (i = 1; i < n; i++) {
            if (D_800CCD34[D_800C3E90[0]].rank > D_800CCD34[D_800C3E90[i]].rank) {
                tmp = D_800C3E90[0];
                D_800C3E90[0] = D_800C3E90[i];
                D_800C3E90[i] = tmp;
            }
        }
    }
    return D_800C3E90[0];
}
#endif
