/* Retail TU part 4 of main35.c (see the BATTLE_TU_PART note there):
 * func_80085084, then that file's part-4 C bodies.  The run before it
 * (func_80084D28, func_80084DE4) is gcc 2.6.3 code in mainasm_15238.c. */
#define BATTLE_TU_PART 4
#include "common.h"
#ifndef XENO_PC_PORT
extern u8 D_800D3014;
extern u8 D_800C3E2C;
extern u16 D_800C3D64;
extern s32 D_800C3E24;
extern void func_80084DE4(u32, u32, u32, u32, u32);
extern u16 func_80084D28(void);
extern u32 func_80084854(u32 arg0, u32 arg1);
extern u16 func_80089C08(u8);
extern u16 func_80089C9C(u16, u8);
extern void func_800BC404(u32);
extern void func_800BCD98(u32);
extern void func_800716D8(void);

/* func_80085084.s, byte-exact: the target-list selection loop.  It
 * starts the list with func_80084DE4(arg0, arg0, arg1, 0, arg2) and returns
 * 0 at once when the selection D_800C3E2C is 0xFF; otherwise it loops until
 * an event ends it:
 * each frame (func_800716D8) it highlights per mode arg0 & 0xF (0: the
 * selected actor's func_80089C08 mask, 1: the D_800C3D64 mask, 2: the
 * func_80084D28 mask, kept as the new mask); event 5 cancels (returns 0),
 * event 4 commits the mask to D_800C3D64 (returns 1), events 0-3 move the
 * selection with func_80084854 when func_80089C9C accepts it; after each
 * change of D_800D3014 it refreshes the four direction flags at
 * D_800C3E24 + 0xE6.. as func_8008189C does.  The switch table is this
 * object's .rodata (jtbl_80070230, placed by config/battle.yaml); events 6
 * and 7 are explicit empty cases (retail's table has 8 entries).  The mode
 * is assigned to a u8 local before the switch: switching on arg0 & 0xF
 * directly lets loop.c hoist it out of the loop (decomp-permuter found
 * this; 29/163 without it). */
u8 func_80085084(u16 arg0, u8 arg1, u8 arg2) {
    u8 last = 0xFE;
    u16 mask;
    u8 status;
    u8 n;
    u8 mode;
    s32 i;

    func_80084DE4(arg0, arg0, arg1, 0, arg2);
    mask = D_800C3D64;
    status = D_800C3E2C == 0xFF;
    if (status == 0) {
        do {
            func_800716D8();
            mode = arg0 & 0xF;
            switch (mode) {
            case 0:
                func_800BCD98(func_80089C08(D_800C3E2C));
                func_800BC404(func_80089C08(D_800C3E2C));
                break;
            case 1:
                func_800BCD98(D_800C3D64);
                func_800BC404(D_800C3D64);
                break;
            case 2:
                mask = func_80084D28();
                func_800BCD98(mask);
                func_800BC404(mask);
                break;
            }
            switch (D_800D3014) {
            case 5:
                status = 1;
                break;
            case 4:
                D_800C3D64 = mask;
                status = 2;
                break;
            case 0:
                n = func_80084854(D_800C3E2C, 0);
                if (func_80089C9C(D_800C3D64, n)) {
                    D_800C3E2C = n;
                }
                break;
            case 1:
                n = func_80084854(D_800C3E2C, 1);
                if (func_80089C9C(D_800C3D64, n)) {
                    D_800C3E2C = n;
                }
                break;
            case 2:
                n = func_80084854(D_800C3E2C, 2);
                if (func_80089C9C(D_800C3D64, n)) {
                    D_800C3E2C = n;
                }
                break;
            case 3:
                n = func_80084854(D_800C3E2C, 3);
                if (func_80089C9C(D_800C3D64, n)) {
                    D_800C3E2C = n;
                }
                break;
            case 6:
            case 7:
                break;
            }
            if (D_800D3014 != last) {
                last = D_800D3014;
                for (i = 0; i < 4; i++) {
                    if ((func_80084854(D_800C3E2C, i & 0xFF) & 0xFF) != D_800C3E2C) {
                        *(s8 *)((s8 *)(D_800C3E24 + i) + 0xE6) = 1;
                    } else {
                        *(s8 *)((s8 *)(D_800C3E24 + i) + 0xE6) = 0;
                    }
                }
            }
        } while (status == 0);
    }
    return status - 1;
}
#endif
#include "main35.c"
