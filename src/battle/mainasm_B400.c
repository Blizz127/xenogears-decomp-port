/* battle mainasm_B400: the retail-asm run at file offset 0xB400, in retail order.
 * The byte-exact C body below is shared with the port. */
#include "common.h"

extern u8 D_800D3430[];

#ifdef XENO_PC_PORT
#define BATTLE_PINNED(type, name, reg) type name
#else
#define BATTLE_PINNED(type, name, reg) register type name asm(reg)
#endif

/* func_8007AEF0: row = D_800D3430 + ((idx&0xFF)<<6); prod = row[p[1]] *
 * row[p[2]]; row[p[3]] = (prod as s16 >= 0x100) ? 0xFF : (u8)prod. Row-table
 * idiom shared with src/battle/main21.c/main17.c/mainc25.c (same
 * D_800D3430 array). Byte-exact reg alloc needs p pinned to $6/a2 (retail
 * moves *pp there right after loading it, freeing a0) and both mult
 * operands pinned to $4/a0 and $2/v0 respectively; without pins gcc keeps
 * p in a0 and picks the opposite v0/v1 pair for the mult, which mismatches
 * retail's mult a0,v0 encoding. */
void func_8007AEF0(u8 **pp, s32 idx) {
    BATTLE_PINNED(u8 *, p, "$6");
    BATTLE_PINNED(u32, v1term, "$4");
    BATTLE_PINNED(u32, v2term, "$2");
    u8 *row;
    u32 prod;
    s16 clipped;
    u8 result;

    p = *pp;
    row = D_800D3430 + ((idx & 0xFF) << 6);
    v1term = row[p[1]];
    v2term = row[p[2]];
    prod = v1term * v2term;
    result = (u8)prod;
    clipped = (s16)prod;
    if (clipped >= 0x100) {
        result = 0xFF;
    }
    row[p[3]] = result;
}
