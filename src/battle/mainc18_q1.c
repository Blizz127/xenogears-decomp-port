#include "common.h"


#ifndef XENO_PC_PORT
extern u8 D_800D3420[];
#endif


/* The mainc18 operator family: each entry applies one arithmetic operation to
 * the 16-bit cell D_800D3420[index * 0x40][p[1]], where the board record p
 * carries the cell index in p[1] and a little-endian 16-bit operand in
 * p[2..3].  They differ only in the operator and its clamp.
 *
 * The TU is split around func_8007AC80 (the multiply), which sits in its own
 * file, mainasm_B190.c, between this TU and mainc18_q2.  The split dates from
 * when its C was not byte-exact; it now matches there under the same CDK
 * preset (see the note on its body for the $v1 product register).
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
