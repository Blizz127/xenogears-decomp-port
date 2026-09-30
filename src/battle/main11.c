#include "common.h"


#ifndef XENO_PC_PORT
extern u8 D_800D2E5D[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2E60[];
#endif
extern u32 func_80079ED8(u32 a0, u32 a1, u32 a2, u32 a3);
/* func_80079098.s: callee declared u32 (not u8) so its return value flows
 * into v0 unmasked; the final & 0xFF keeps semantics identical. */
void func_80079098(u32 a0, u32 a1) {
    u32 s1 = a0 & 0xFF;
    u32 s0 = (a1 & 0xFF) << 3;
    u32 r;
    r = func_80079ED8(s1, D_800D2E5D[s0], 0, 1);
    func_80079ED8(s1, D_800D2E5D[s0], (D_800D2E60[s0] + r) & 0xFF, 0);
}


#ifndef XENO_PC_PORT
extern u8 D_800D2E61[];
#endif
extern void func_8007A280(u8 a0, u8 a1, u32 a2, u32 a3);


/* func_80079114.s */
void func_80079114(u8 a, u8 idx) {
    func_8007A280(a & 0xFF, *(u8*)(D_800D2E5D + ((u32)idx << 3)),
                  *(u8*)(D_800D2E60 + ((u32)idx << 3)) |
                      ((u32)*(u8*)(D_800D2E61 + ((u32)idx << 3)) << 8),
                  0);
}
