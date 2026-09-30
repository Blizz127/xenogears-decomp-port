#include "common.h"


extern int rand(void);
#ifdef XENO_PC_PORT
/* Coexistence: decompiled and logic-faithful, but not byte-exact yet, so the
 * matching build assembles retail bytes below and the port uses this body. */
/* func_80089B50.s: a random value in [lo, hi]: 0xFFFF for a lo of 0xFFFF
 * (retail returns the register holding the compare constant), 0 when hi is 0,
 * lo when the bounds are equal, and a raw rand() when the span exceeds 0xFFFE.
 * The span is computed at the equality test so it lands in its delay slot. */
u16 func_80089B50(u16 lo, u16 hi) {
    s32 span;

    if (lo == 0xFFFF) {
        return 0xFFFF;
    }
    if (hi == 0) {
        return 0;
    }
    if (lo == hi) {
        return lo;
    }
    span = hi - lo;
    if (span > 0xFFFE) {
        return rand() & 0xFFFF;
    }
    return (lo + (rand() & 0xFFFF) % (span + 1)) & 0xFFFF;
}
#else
INCLUDE_ASM("asm/battle/nonmatchings/main41", func_80089B50);
#endif
#ifndef XENO_PC_PORT
extern u16 D_800C3468[];
#endif
#ifndef XENO_PC_PORT
extern u16 D_800C3448[];
#endif


/* func_80089BEC.s */
u16 func_80089BEC(u8 index) {
    return D_800C3468[index];
}
/* func_80089C08.s */
u16 func_80089C08(u8 index) {
    return D_800C3448[index];
}
/* func_80089C24.s */
u32 func_80089C24(u8 index) {
    /* 0xFFFF ^ zero-extended u16 load: retail's `nor $v0,$zero,$v0` with the
     * load-delay nop; a plain `~` lets GCC fill the return slot instead. */
    return 0xFFFF ^ (u32)D_800C3468[index];
}
/* func_80089C48.s */
u32 func_80089C48(u8 index) {
    return 0xFFFF ^ (u32)D_800C3448[index];
}
/* func_80089C6C.s */
u32 func_80089C6C(u32 mask, u8 index) {
    /* Out-of-range returns first: retail branches *out* of the load
     * (`beqz` on `index < 0x10`) and keeps `and` in the `j` delay slot. */
    if (index >= 0x10) {
        return 0;
    }
    return (u32)D_800C3468[index] & mask;
}
/* func_80089C9C.s */
u32 func_80089C9C(u32 mask, u8 index) {
    if (index >= 0x10) {
        return 0;
    }
    return (u32)D_800C3448[index] & mask;
}
