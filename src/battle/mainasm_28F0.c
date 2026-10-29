/* battle mainasm_28F0: the retail run at file offset 0x28F0 (func_800723E0),
 * split out of main2 because its natural C matches only under gcc 2.7.2 (this
 * TU's Default preset), while main2's head (mainasm_1FF0) and tail (main2) are
 * gcc 2.6.3 code.  The whole file is retail-build only. */
#include "common.h"
#ifndef XENO_PC_PORT
extern u16 D_800D39E0;
extern u8 D_800D2DC0;
extern u8* D_800C3EAC;
extern u8 D_800D2DE4[];
extern u16 D_800D2E06[];
extern u8 D_800D2DD7[];
extern u8 D_800D2DD8[];
extern void func_80071B94(s32 a0);
extern void func_80072324(void);

/* func_800723E0.s, byte-exact under gcc 2.7.2 (this TU's Default preset).
 * main2 (gcc 2.6.3) gives 388 vs 332 bytes for the same body: at 2.6.3's
 * first jump pass the loop exit code is only 19 insns, so jump.c copies it
 * before loop.c, which then rejects the loop ("multiple entry points") and
 * hoists nothing.  CDK 2.7.2: diff 51/83.  Split out of main2 together with
 * mainasm_1FF0, main2's head.
 *
 * When D_800D39E0 is clear, pick the next current actor (D_800C3EAC[0x2D3],
 * 1-based) and run func_80071B94(0):
 * - a pending actor in D_800D2DC0 wins: mark its D_800D2DE4 entry, clear its
 *   D_800D2E06 entry and clear D_800D2DC0;
 * - otherwise scan the 11-slot ring D_800D2DD8 from the cursor D_800D2DD7
 *   for the first actor whose D_800D2DE4 flag is 1, and move the cursor past
 *   it, wrapping 11 to 0.  The break after the wrap is redundant, because
 *   the loop test ends the scan either way, but retail has it.
 * When D_800D39E0 is set, run func_80072324 instead.
 *
 * Codegen notes, all natural C:
 * - The new cursor goes through the u8 temp `t`, computed before the 0x2D3
 *   store; the found block then has retail's registers (k + 1 in v0, the
 *   pointer in a0, `a` and its in-place a + 1 in v1).  Writing
 *   `D_800D2DD7[0] = k + 1` after the store gives 9/83; `++a` or `a++` for
 *   the a + 1 gives 8/83.
 * - The do-body from `a = ...` to the loop end must stay at 21+ insns at
 *   the first jump pass.  At 20 or fewer, jump.c copies it before loop.c,
 *   which then rejects the loop and hoists nothing: an s32 `t` gives 384
 *   bytes.
 * - D_800D2E06[D_800D2DC0 - 1] is cleared before D_800D2DC0 (a copied index
 *   temp gives 3/83). */
void func_800723E0(void) {
    s32 k;
    u32 a;
    u8 t;

    if (D_800D39E0 == 0) {
        if (D_800D2DC0 != 0) {
            D_800C3EAC[0x2D3] = D_800D2DC0;
            D_800D2DE4[D_800D2DC0 - 1] = 1;
            D_800D2E06[D_800D2DC0 - 1] = 0;
            D_800D2DC0 = 0;
        } else {
            D_800C3EAC[0x2D3] = 0;
            k = D_800D2DD7[0];
            do {
                a = D_800D2DD8[k];
                if (D_800D2DE4[a] == 1) {
                    t = k + 1;
                    D_800C3EAC[0x2D3] = a + 1;
                    D_800D2DD7[0] = t;
                    if (D_800D2DD7[0] == 11) {
                        D_800D2DD7[0] = 0;
                        break;
                    }
                }
                k++;
                if (k == 11) {
                    k = 0;
                }
            } while (k != D_800D2DD7[0]);
        }
        func_80071B94(0);
    } else {
        func_80072324();
    }
}
#endif
