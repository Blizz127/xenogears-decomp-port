#include "common.h"


extern u32 func_800BEF24(u8* a, u8* b);
extern void func_800223B0(u8* p, s16 v);
extern void func_80021FE0(u8* p, s16 v);


/* func_800B9B54.s */
void func_800B9B54(u8* a, u8* b) {
    if (a == b) {
        return;
    }
    func_800223B0(a, (s16)func_800BEF24(a, b));
    func_80021FE0(a, (s16)func_800BEF24(a, b));
    if (*(s8*)(b + 0xAF) == 0x15) {
        return;
    }
    func_800223B0(b, (s16)func_800BEF24(b, a));
    func_80021FE0(b, (s16)func_800BEF24(b, a));
}
