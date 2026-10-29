#include "common.h"


#ifndef XENO_PC_PORT
extern u8* g_TimerWorkList__a[] asm("g_TimerWorkList");
extern u8 D_8006BE10[];

u8* func_800BFC80__ret(u8* sprite, s32 index, s32 mode) asm("func_800BFC80");

/* Walk g_TimerWorkList for the timer tasks of `sprite`'s owner (+0x6C) whose
 * +0x14 id matches the owner's +0x10 (bit 29 set) and whose child uses the
 * D_8006BE10 handler.  Mode 0 returns the child with +0xAF == index; mode 1
 * runs that child owner's +0xC callback; mode 2 runs it for every such child.
 * Returns NULL otherwise.  Defined under an asm name because this TU's caller
 * keeps the void prototype below. */
u8* func_800BFC80__ret(u8* sprite, s32 index, s32 mode) {
    u8* t;
    u8* owner;
    u8* child;

    t = g_TimerWorkList__a[0];
    owner = *(u8**)(sprite + 0x6C);
    while (t != NULL) {
        if (*(u8**)(t + 0) == owner
            && (*(u32*)(t + 0x14) & 0x1FFFFFFF) == (*(u32*)(owner + 0x10) & 0x1FFFFFFF)
            && ((*(u32*)(t + 0x14) >> 29) & 1)) {
            child = *(u8**)(t + 4);
            if (*(u8**)(child + 0x24) == D_8006BE10) {
                if (mode == 2) {
                    (*(void (**)(u8*))(*(u8**)(child + 0x6C) + 0xC))(*(u8**)(child + 0x6C));
                } else if (*(s8*)(child + 0xAF) == index) {
                    if (mode == 0) {
                        return child;
                    }
                    (*(void (**)(u8*))(*(u8**)(child + 0x6C) + 0xC))(*(u8**)(child + 0x6C));
                }
            }
        }
        t = *(u8**)(t + 0x18);
    }
    return NULL;
}
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
