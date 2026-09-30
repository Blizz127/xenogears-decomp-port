/* battle mainasm_BF14: the retail-asm run at file offset 0xBF14, in retail order.
 * Functions still listed as INCLUDE_ASM are not byte-matching C yet. The whole
 * file is retail-build only; any port body for these functions lives in the
 * port's own TU. */
#include "common.h"

extern u8 D_800D3420[];
extern u8 D_800D3410[];

#ifdef XENO_PC_PORT
#define BATTLE_PINNED(type, name, reg) type name
#define BATTLE_UADDR unsigned long
#else
#define BATTLE_PINNED(type, name, reg) register type name asm(reg)
#define BATTLE_UADDR u32
#endif

/* func_8007BA04: shared matched C body; MIPS register pins preserve retail
 * allocation and BATTLE_UADDR preserves 32-bit guest address arithmetic. */
void func_8007BA04(u8 **pp, s32 idx) {
    BATTLE_PINNED(u8 *, p, "$4");
    BATTLE_PINNED(BATTLE_UADDR, t_base, "$6");
    BATTLE_PINNED(u32, idx2, "$3");
    u32 row;
    u8 *base;
    BATTLE_UADDR temp;
    u8 *b16;
    u32 idx1;

    row = (idx & 0xFF) << 6;
    base = D_800D3420;
    t_base = (BATTLE_UADDR)base + row;
    temp = (BATTLE_UADDR)base + 0x10;
    p = *pp;
    b16 = (u8 *)(row + temp);
    idx2 = p[2];
    idx1 = p[1];
    idx2 <<= 1;
    *(u16 *)(idx2 + t_base) = b16[idx1];
}


/* func_8007BA44: shared matched C body. MIPS register pins preserve retail
 * allocation; BATTLE_UADDR keeps the same 32-bit arithmetic on MIPS and
 * pointer-width arithmetic in host differential builds. */
void func_8007BA44(u8 **pp, s32 idx) {
    BATTLE_PINNED(u8 *, base, "$3");
    BATTLE_PINNED(BATTLE_UADDR, t_base, "$6");
    BATTLE_PINNED(u32, f2, "$4");
    BATTLE_PINNED(u32, f1, "$2");
    u32 row;
    BATTLE_UADDR temp;
    u8 *p;
    BATTLE_UADDR b16;
    u16 val;

    row = (idx & 0xFF) << 6;
    base = D_800D3410;
    t_base = (BATTLE_UADDR)base + row;
    temp = (BATTLE_UADDR)base + 0x10;
    p = *pp;
    b16 = row + temp;
    f2 = p[2];
    f1 = p[1];
    f2 <<= 2;
    f1 <<= 1;
    val = *(u16 *)(f1 + b16);
    *(u32 *)(f2 + t_base) = val;
}
