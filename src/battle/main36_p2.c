/* Retail TU part 2 of main36.c (see the BATTLE_TU_PART note there): the
 * still-assembly run below, then that file's part-2 C bodies. */
#define BATTLE_TU_PART 2
#include "common.h"
#ifndef XENO_PC_PORT
extern u8* D_800C3EAC;
extern u8 D_800C3FE8[];
extern u8 D_800C4000[];
extern void func_80085388(void);
extern u32 func_8009ADA0(u8 idx, u32* out, u32 a2);
extern void func_80085618(s32 a0);
extern void func_800BE538(u8 a0, u32 a1, u32 a2, u32 a3);

/* a2 is passed through to func_8009ADA0 untouched. */
void func_80085B58(u8 slot, s32 a1, u32 a2) {
    u32 v[3];
    s32 i;

    v[2] = 0;
    v[1] = 0;
    v[0] = 0;
    if (func_8009ADA0(slot, v, a2)) {
        for (i = 0; i < 3; i++) {
            if (v[i] != 0) {
                D_800C3EAC[0x2DA] = 0;
                func_80085388();
                D_800C4000[slot] = i + 8;
                ((u16*)D_800C3FE8)[slot] = *(u16*)&v[i];
                func_80085618(D_800C3EAC[0x2DA]);
            }
        }
        func_800BE538(slot, v[0], v[1], v[2]);
    }
}

#endif
#include "main36.c"
