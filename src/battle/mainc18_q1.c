#include "common.h"


#ifndef XENO_PC_PORT
extern u8 D_800D3420[];
#endif


/* The mainc18 operator family: each entry applies one arithmetic operation to
 * the 16-bit cell D_800D3420[index * 0x40][p[1]], where the board record p
 * carries the cell index in p[1] and a little-endian 16-bit operand in
 * p[2..3].  They differ only in the operator and its clamp.
 *
 * The TU is split around func_8007AC80 (the multiply), whose C body is not
 * yet byte-exact: every candidate reproduces all 23 retail instructions but
 * lands the product in $a0 instead of retail's $v1.  The two orderings are
 * coupled -- writing `op * cell` yields retail's `mflo $v1` but reverses the
 * `mult` operands, and `cell * op` yields retail's `mult` order but `mflo
 * $a0`.  Since file-scope INCLUDE_ASM is emitted ahead of every compiled
 * body, the two functions preceding it stay in the asm segment too.
 */


/* func_8007ABD8.s: add the operand into the row cell, saturating at 0xFFFF. */
void func_8007ABD8(u8** ppBoard, u8 index) {
    u32 off = (u32)index << 6;
    u8* base = D_800D3420;
    u8* p = *ppBoard;
    u16* row = (u16*)(base + off);
    s32 v = row[p[1]] + ((p[3] << 8) + p[2]);

    if (v > 0xFFFF) {
        v = 0xFFFF;
    }
    row[p[1]] = v;
}


/* func_8007AC30.s: subtract the operand from the row cell, clamping at 0. */
void func_8007AC30(u8** ppBoard, u8 index) {
    u32 off = (u32)index << 6;
    u8* base = D_800D3420;
    u8* p = *ppBoard;
    u16* row = (u16*)(base + off);
    s32 v = row[p[1]] - ((p[3] << 8) + p[2]);

    if (v < 0) {
        v = 0;
    }
    row[p[1]] = v;
}
