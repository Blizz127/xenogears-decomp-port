/* Host port body of the battle SED registration walk.  The byte-exact C is
 * retail-only in main135.c. */
#include "common.h"
#ifdef XENO_PC_PORT
#include "battle_host_guest.h"

extern void func_8003852C(void* pSed);

/* func_800C1140.s: walk the offset table of `file` (count *file - 3, word
 * offsets from +0x10, each relative to the file start) and register every
 * entry tagged 0x73646573 with func_8003852C.  `file` is the guest word
 * func_800BF3A4 passes (D_800C3618). */
void func_800C1140(u32 file) {
    u32 base = file;
    s32 n = (s32)BG_U32(file) - 3;

    for (file += 0x10; n > 0; n--, file += 4) {
        u32 entry = BG_U32(file) + base;

        if (BG_U32(entry) == 0x73646573u) {
            func_8003852C(BG_PTR(entry));
        }
    }
}
#endif
