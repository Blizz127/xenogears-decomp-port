/* battle mainasm_15050: the retail-asm run at file offset 0x15050, in retail order.
 * Functions still listed as INCLUDE_ASM are not byte-matching C yet. The whole
 * file is retail-build only; any port body for these functions lives in the
 * port's own TU. */
#include "common.h"

#ifndef XENO_PC_PORT
extern u8* D_800C3EAC;
extern u8 D_800D3014;
extern u16 func_80089C08(u32);
extern void func_800BC404(u32);
extern void func_800BCD98(u32);
extern void func_800716D8(void);
extern u32 func_80084854(u32 arg0, u32 arg1);

/* func_80084B40.s, byte-exact: the target-selection loop.  While the
 * event byte D_800D3014 is 8 (none yet) it highlights the actor and the
 * selected target (func_800BC404 / func_800BCD98 on the func_80089C08
 * masks) and waits a frame (func_800716D8).  Events 4/6/7 store the
 * selection at the actor row's +0x3C and return 1; event 5 re-highlights
 * the stored target and returns 0; events 0-3 move the selection with
 * func_80084854(selected, direction).  Starts from context +0x2E8.  The
 * switch table is this object's .rodata (jtbl_80070210, placed by
 * config/battle.yaml).  Derived from the port arm in
 * target_ui_impl.inc: the row offset is computed in the cases as
 * `(arg0 & 0xFF) << 6` (that gives retail's prologue order, where
 * top-of-function locals put the loop constant 8 last; `* 64` puts the
 * offset first in the address add), and func_80089C08 is declared with a
 * 32-bit argument here so the actor is passed unmasked. */
u32 func_80084B40(u32 arg0) {
    u32 selected = ((u8 *)D_800C3EAC)[0x2E8];
    u32 status = 2;
    D_800D3014 = 8;
    do {
        while (D_800D3014 == 8) {
            u16 own_mask = func_80089C08(arg0 & 0xFF);
            u16 target_mask = func_80089C08(selected & 0xFF);
            func_800BC404((own_mask | target_mask) & 0xFFFF);
            target_mask = func_80089C08(selected & 0xFF);
            func_800BCD98(target_mask & 0xFFFF);
            func_800716D8();
        }
        switch (D_800D3014) {
        case 4:
        case 6:
        case 7:
            status = 1;
            ((u8 *)D_800C3EAC + ((arg0 & 0xFF) << 6))[0x3C] = selected;
            break;
        case 5: {
            u16 own_mask = func_80089C08(arg0 & 0xFF);
            u16 target_mask = func_80089C08(((u8 *)D_800C3EAC + ((arg0 & 0xFF) << 6))[0x3C]);

            func_800BC404((own_mask | target_mask) & 0xFFFF);
            target_mask = func_80089C08(((u8 *)D_800C3EAC + ((arg0 & 0xFF) << 6))[0x3C]);
            status = 0;
            func_800BCD98(target_mask & 0xFFFF);
            break;
        }
        case 0:
            selected = func_80084854(selected & 0xFF, 0);
            break;
        case 1:
            selected = func_80084854(selected & 0xFF, 1);
            break;
        case 2:
            selected = func_80084854(selected & 0xFF, 2);
            break;
        case 3:
            selected = func_80084854(selected & 0xFF, 3);
            break;
        }
        D_800D3014 = 8;
    } while ((status & 0xFF) == 2);
    return status & 0xFF;
}
#endif
