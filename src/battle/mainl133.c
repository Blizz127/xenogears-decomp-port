#include "common.h"


#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/mainl133", func_800BFC80);
#endif


#ifndef XENO_PC_PORT
extern void func_800BFC80(u32 a0, u32 a1, u32 a2);
#else
/* The port body returns v0 (the matched sprite for mode 0). */
extern u32 func_800BFC80(u32 a0, u32 a1, u32 a2);
#endif


/* func_800BFD88.s */
void func_800BFD88(u32 a0, u32 a1) {
    func_800BFC80(a0, a1, 1);
}

#ifdef XENO_PC_PORT
#include "battle_host_guest.h"
/* Native owner: pc_port/src/work_list_port.c (u32-field WorkListEntry list;
 * entries live below 4 GiB, so u32 holds them). */
extern u8* g_TimerWorkList;

/* func_800BFC80.s: walk the timer work list for entries owned by `sprite`'s
 * task (+0x6C) with the same 29-bit id (+0x14 vs owner +0x10) and bit 29
 * set, whose sprite (+4) uses the D_8006BE10 package (+0x24).  mode 2: run
 * every such sprite's task +0xC callback; otherwise only sprites whose +0xAF
 * animation equals `anim`, and mode 0 returns the first of them instead of
 * calling.  Returns 0 otherwise.  `sprite` is a u32 because the existing
 * caller func_800BFD88 passes one (u32 also carries a raw host address). */
u32 func_800BFC80(u32 sprite, u32 anim, u32 mode) {
    u32 e = (u32)(uintptr_t)g_TimerWorkList;
    u32 owner = BG_U32(sprite + 0x6C);

    while (e != 0) {
        if (BG_U32(e) == owner) {
            u32 id = BG_U32(e + 0x14);

            if ((id & 0x1FFFFFFFu) == (BG_U32(owner + 0x10) & 0x1FFFFFFFu) &&
                ((id >> 29) & 1) != 0) {
                u32 child = BG_U32(e + 4);

                /* Retail compares with the lui/addiu address of D_8006BE10. */
                if (BG_U32(child + 0x24) == 0x8006BE10u) {
                    if (mode == 2 || (s32)BG_S8(child + 0xAF) == (s32)anim) {
                        u32 task;

                        if (mode != 2 && mode == 0) {
                            return child;
                        }
                        task = BG_U32(child + 0x6C);
                        bg_jalr(BG_U32(task + 0xC), task, anim, mode, 0);
                    }
                }
            }
        }
        e = BG_U32(e + 0x18);
    }
    return 0;
}
#endif
