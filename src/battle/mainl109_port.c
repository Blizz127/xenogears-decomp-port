/* Host port body of the pair face-off helper.  The byte-exact C is in
 * mainl109.c, fenced out of adopted host TUs. */
#include "common.h"
#ifdef XENO_PC_PORT
#include "battle_host_guest.h"

extern u32 func_800BEF24(u8* a, u8* b);
extern void func_800223B0(u8* p, s16 v);
extern void func_80021FE0(u8* p, s16 v);

/* func_800B9B54.s: turn `a` to face `b` (func_800223B0 / func_80021FE0 of
 * func_800BEF24(a, b), evaluated for each), and `b` to face `a` the same way
 * unless b's +0xAF state is 0x15.  Nothing when a == b.  The pointers are
 * sprite records (guest words or compact host addresses); both functions
 * are recomputed per call as retail does. */
void func_800B9B54(u8* a_ptr, u8* b_ptr) {
    u32 a = BG_ADDR(a_ptr);
    u32 b = BG_ADDR(b_ptr);

    if (a == b) {
        return;
    }
    func_800223B0(bg_arg(a), (s16)func_800BEF24(bg_arg(a), bg_arg(b)));
    func_80021FE0(bg_arg(a), (s16)func_800BEF24(bg_arg(a), bg_arg(b)));
    if ((s8)BG_U8(b + 0xAF) == 0x15) {
        return;
    }
    func_800223B0(bg_arg(b), (s16)func_800BEF24(bg_arg(b), bg_arg(a)));
    func_80021FE0(bg_arg(b), (s16)func_800BEF24(bg_arg(b), bg_arg(a)));
}
#endif
