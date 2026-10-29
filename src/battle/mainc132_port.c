/* Host port body of the command-step party refresh.  The byte-exact C is
 * retail-only in mainc132.c. */
#include "common.h"
#ifdef XENO_PC_PORT
#include "battle_host_guest.h"

extern void func_800B8D04(void);
extern void func_800BE790(void);
extern void func_800BB760(u32 slot);

/* func_800BFA9C.s: when D_800C362C is set, run func_800B8D04, wait two
 * frames, then for each party slot whose 28-byte D_800C3EB0 row has byte 8
 * set and byte 6 below 0x11 call func_800BB760(slot) -- except, in mode 2,
 * the slot equal to the 4-bit class of the actor D_800C3E1C (+0xAC bits
 * 0..1 above +0xA8 bits 30..31); wait while D_800C35D8 is set; clear
 * D_800C362C.  The mode byte, the actor's class and the row bytes are read
 * where retail reads them: the mode on every slot, the row bytes at each
 * pass (func_800BB760 may change them), the class once. */
void func_800BFA9C(void) {
    u32 base = 0x800C3EB0; /* D_800C3EB0 */
    s32 i;
    u32 actor;
    u32 lo;
    u32 cls;

    if (BG_U8(0x800C362C) == 0) { /* D_800C362C */
        return;
    }
    func_800B8D04();
    func_800BE790();
    func_800BE790();
    actor = BG_U32(0x800C3E1C); /* D_800C3E1C */
    lo = BG_U32(actor + 0xA8) >> 30;
    cls = ((BG_U32(actor + 0xAC) & 3) << 2) | lo;
    for (i = 0; i != 3; i++) {
        if (BG_U8(base + (u32)i * 28 + 8) != 0) {
            if (BG_U8(0x800C362C) != 2 || (u32)i != cls) {
                if (BG_U8(base + (u32)i * 28 + 6) < 0x11) {
                    func_800BB760((u32)i);
                }
            }
        }
    }
    while (BG_U32(0x800C35D8) != 0) { /* D_800C35D8 */
        func_800BE790();
    }
    BG_U8(0x800C362C) = 0;
}
#endif
