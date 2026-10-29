/* battle mainasm_AE3C: the retail-asm run at file offset 0xAE3C, in retail order.
 * Functions still listed as INCLUDE_ASM are not byte-matching C yet. The whole
 * file is retail-build only; any port body for these functions lives in the
 * port's own TU. */
#include "common.h"

#ifndef XENO_PC_PORT
/* The 64-byte enemy AI records at D_800D3400 (index = actor - 3): +0x00
 * the script func_800799C8 runs, +0x08 / +0x0C the ones func_80079AB0 /
 * func_80079C24 run, then the battle-script ops' slots: four words at
 * +0x10, eight halfwords at +0x20 and sixteen bytes at +0x30. */
typedef struct {
    u8* script;
    u8 pad4[4];
    u8* script8;
    u8* scriptC;
    u32 w[4];
    u16 h[8];
    u8 b[16];
} EnemyAiRec;
extern EnemyAiRec D_800D3400__rec[] asm("D_800D3400");

/* func_8007A92C.s, byte-exact: battle-script op on p = *ppBoard: store the
 * halfword p[2] | p[3] << 8 in slot p[1] of enemy `index`'s halfword table.
 * Codegen: the slot goes through the D_800D3400 record view (field h at
 * +0x20).  A `D_800D3420 + index * 64` row pointer instead gets its `la`
 * scheduled ahead of the p[1] load into another register; through the
 * record, retail's `la` lands after that load, in p's register.  Matches under
 * gcc 2.7.2 (the TU's preset) and 2.6.x. */
void func_8007A92C(u8** ppBoard, u8 index) {
    u8* p = *ppBoard;
    u16 v = p[2] | (p[3] << 8);

    D_800D3400__rec[index].h[p[1]] = v;
}

/* func_8007A968.s, byte-exact: as func_8007A92C for the word table: slot
 * p[1] of enemy `index`'s words gets ((p[3] << 8) + p[2]) << 4 (retail adds
 * the halves; `|` is one insn off).  Same record view.  Matches under gcc
 * 2.7.2 and 2.6.x. */
void func_8007A968(u8** ppBoard, u8 index) {
    u8* p = *ppBoard;
    u32 v = ((p[3] << 8) + p[2]) << 4;

    D_800D3400__rec[index].w[p[1]] = v;
}
#endif
