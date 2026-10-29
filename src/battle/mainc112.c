#include "common.h"


#ifndef XENO_PC_PORT
extern void func_800B14CC(u32 skip);
extern u8 D_800C3EB0[];

/* The D_800C3EB0 block as seen here: three task slots at +0x8C8C and their
 * flags at +0x8CB8. */
typedef struct {
    u8 pad[0x8C8C];
    u8* tasks[11];
    s32 flags[3];
} BattleTaskTable;

extern void func_800BC404(u32 v);
extern void func_800B8D7C(void);
extern void func_800245D8(void* object, s16 animation);
extern void func_800BE790(void);
extern void func_800BADD4(u32 i);
extern void func_800BB760(u32 i);
extern s32 g_NumTimerWorkListEntries[];
extern s32 D_800C35D8[];

/* Retire party slot `slot`: flush its command mask (func_800BC404), play
 * animation 0x16 on its task and pump frames (func_800BE790) while the
 * task's +0x9E is set and its +0xAF still reads 0x16, wait until the timer
 * list is back to its entry count, release the slot (func_800BADD4,
 * func_800BB760) and wait for D_800C35D8 to reach 0.
 * The timer-list count is declared as an array: for a small-data scalar
 * the CDK cc1 emits a `lw` macro, where retail splits %hi/%lo and keeps
 * the %hi in s0 across the wait loop. */
void func_800BAF48(u32 slot) {
    u8* actor;
    s32 count;

    func_800BC404(1 << slot);
    func_800BC404(0);
    count = g_NumTimerWorkListEntries[0];
    actor = ((BattleTaskTable*)D_800C3EB0)->tasks[slot];
    func_800B8D7C();
    func_800245D8(actor, 0x16);
    while (*(s16*)(actor + 0x9E) != 0 && *(s8*)(actor + 0xAF) == 0x16) {
        func_800BE790();
    }
    while (g_NumTimerWorkListEntries[0] != count) {
        func_800BE790();
    }
    func_800BADD4(slot);
    func_800BE790();
    func_800BE790();
    func_800BB760(slot);
    while (D_800C35D8[0] != 0) {
        func_800BE790();
    }
}

/* Free every task in the table except `skip` whose +0x48 is clear (owner
 * +0x6C's +0xC callback), zeroing its slot and flag. */
void func_800BB080(s32 skip) {
    s32 i;
    u8* t;

    for (i = 0; i != 3; i++) {
        if (i != skip) {
            t = ((BattleTaskTable*)D_800C3EB0)->tasks[i];
            if (t != NULL && *(s32*)(t + 0x48) == 0) {
                u8* o = *(u8**)(t + 0x6C);
                (*(void (**)(u8*))(o + 0xC))(o);
                ((BattleTaskTable*)D_800C3EB0)->tasks[i] = NULL;
                ((BattleTaskTable*)D_800C3EB0)->flags[i] = 0;
            }
        }
    }
    func_800B14CC(skip);
}

extern void AnimScriptTick(void* obj);
extern void func_80022CDC(void* obj);
extern u8* D_800D3368[];

/* Tick the sprite's animation (twice when +0xAC bit 6 is set) while +0x64 is
 * live, then copy its actor's (+0x5C/+0x60/+0x64) position as 16.16. */
void func_800BB13C(u8* task) {
    u8 pad[8];
    u8* o = *(u8**)(task + 4);
    u32 hi = *(u32*)(o + 0xA8) >> 30;
    u8* actor = D_800D3368[((*(u32*)(o + 0xAC) & 3) << 2) | hi];

    if (actor != NULL) {
        if (*(s16*)(o + 0x9E) == 0) {
            *(s32*)(o + 0x64) = 0;
        }
        if (*(s32*)(o + 0x64) != 0) {
            AnimScriptTick(o);
            func_80022CDC(o);
            if ((*(u32*)(o + 0xAC) >> 6) & 1) {
                AnimScriptTick(o);
                func_80022CDC(o);
            }
        }
        *(s32*)(o + 0) = *(s32*)(*(u8**)(actor + 4) + 0x5C) << 16;
        *(s32*)(o + 4) = *(s32*)(*(u8**)(actor + 4) + 0x60) << 16;
        *(s32*)(o + 8) = *(s32*)(*(u8**)(actor + 4) + 0x64) << 16;
    }
}

#include "psyq/libgte.h"
extern s32 func_800AA600(s32 index);
extern MATRIX D_800D30BC;
extern s32 D_80050100;

/* Work callback: cache the sprite's (task +4) D_800D3368 actor value from
 * func_800AA600 at +0x36 (+0x38 = half of it), then project the integer part
 * of its 16.16 position (+0, +4, +8) through D_800D30BC and store the OT
 * depth (>> D_80050100, + sprite +0x30; 0 when flag bit 15 is set) at +0x2E.
 * sxy doubles as RotTransPers' interpolation output. */
void func_800BB248(u8* task) {
    SVECTOR v;
    long sxy;
    long flag;
    u8* o = *(u8**)(task + 4);
    u32 hi = *(u32*)(o + 0xA8) >> 30;
    s32 z;

    *(s16*)(o + 0x36) = func_800AA600(((*(u32*)(o + 0xAC) & 3) << 2) | hi);
    *(s16*)(o + 0x38) = *(u16*)(o + 0x36) / 2;
    v.vx = *(s32*)(o + 0) >> 16;
    v.vy = *(s32*)(o + 4) >> 16;
    v.vz = *(s32*)(o + 8) >> 16;
    SetRotMatrix(&D_800D30BC);
    SetTransMatrix(&D_800D30BC);
    z = (RotTransPers(&v, &sxy, &sxy, &flag) >> D_80050100) + *(s16*)(o + 0x30);
    if (flag & 0x8000) {
        z = 0;
    }
    *(s16*)(o + 0x2E) = z;
}
extern void WorkListRemoveTask(void *pTask);
extern void TimerWorkListRemoveTask(void *pTask);
extern void HeapFree(void *pMem);

void func_800BB314(u8 *p) {
    u8 pad[8];

    WorkListRemoveTask(p + 0x1C);
    TimerWorkListRemoveTask(p);
    HeapFree(p);
}

extern u32 WorkListsAddTasks(u32 a0, u32 a1, void* a2, void* a3, void* a4);
extern void func_80023804(void* pSpriteData);
extern void func_800239A0(void* pSpriteData);
extern void SpriteSetScale(void* object, short scale);
extern void func_800BB314(u8* p);
/* Read and written through casts so they are plain (not in-struct) fixed
 * accesses; that lets them schedule past the sprite's field stores, as retail. */
extern s8 D_800591AC__s[] asm("D_800591AC");
extern s8 D_800591A8__s[] asm("D_800591A8");
extern u8 D_8006BE10[];

typedef struct {
    u8 pad0[8];
    s32 unk8;
    s16 unkC;
} B350Anim;

typedef struct {
    s32 x;              /* 0x00 */
    s32 y;              /* 0x04 */
    s32 z;              /* 0x08 */
    u8 pad0C[0x18];
    u8* unk24;          /* 0x24 */
    u8 pad28[0xE];
    u16 unk36;          /* 0x36 */
    u16 unk38;          /* 0x38 */
    u8 pad3A[2];
    u32 unk3C;          /* 0x3C */
    u32 unk40;          /* 0x40 */
    u8 pad44[4];
    s32 unk48;          /* 0x48 */
    s32 unk4C;          /* 0x4C */
    u8 pad50[0x1C];
    u8* unk6C;          /* 0x6C */
    u8 pad70[0xC];
    B350Anim* unk7C;    /* 0x7C */
    u8 pad80[2];
    u16 unk82;          /* 0x82 */
    u8 pad84[0x24];
    u32 unkA8_0 : 1;    /* 0xA8 */
    u32 unkA8_1 : 29;
    u32 slotLo : 2;     /* battle slot, low two bits */
    u32 slotHi : 2;     /* 0xAC: battle slot, high two bits */
    u32 unkAC_2 : 30;
} B350Sprite;

/* Create the actor for party slot `slot` when its task-table entry
 * (D_800C3EB0 + 0x8C8C) is empty: a 0x19C-byte work/timer task (callbacks
 * func_800BB13C / func_800BB248 / func_800BB314) whose sprite at +0x38 takes
 * its x/z from the slot's 0x1C-byte record, its scale from func_800AA600 and
 * the 4-bit slot in the +0xA8/+0xAC bitfields; the sprite and task go into the
 * table's +0x8C8C and +0x8CB8 rows.  The heap flag D_800591AC is held at 0
 * around the allocation. */
void func_800BB350(u32 slot) {
    u8* task;
    B350Sprite* spr;
    u8 saved;
    u8* rec;

    if (((BattleTaskTable*)D_800C3EB0)->tasks[slot] == NULL) {
        saved = *(u8*)D_800591AC__s;
        *(u8*)D_800591AC__s = 0;
        task = (u8*)WorkListsAddTasks(0x19C, 0, func_800BB13C, func_800BB248, func_800BB314);
        spr = (B350Sprite*)(task + 0x38);
        spr->unk6C = task;
        *(B350Sprite**)(task + 4) = spr;
        *(B350Sprite**)(task + 0x20) = spr;
        func_80023804(spr);
        func_800239A0(spr);
        spr->unk40 = (spr->unk40 & 0xFFFE1FFF) | 0x8000;
        spr->unk6C = task;
        spr->unk3C &= ~3;
        spr->unkA8_0 = 0;
        spr->unk7C->unk8 = 0;
        spr->unk7C->unkC = 0;
        spr->unk82 = *(u16*)D_800591A8__s;
        rec = D_800C3EB0 + slot * 0x1C;
        *(s32*)(task + 0x38) = *(u16*)(rec + 0xE) << 16; /* spr->x, addressed off the task as retail */
        spr->z = *(u16*)(rec + 0x10) << 16;
        spr->y = 0;
        spr->unk36 = func_800AA600(slot);
        spr->unk82 = 0x2000;
        spr->unk38 = spr->unk36 >> 1;
        SpriteSetScale(spr, 0x2000);
        spr->unk24 = D_8006BE10;
        ((BattleTaskTable*)D_800C3EB0)->tasks[slot] = (u8*)spr;
        ((BattleTaskTable*)D_800C3EB0)->flags[slot] = (s32)task;
        spr->unk4C = 0;
        spr->unk48 = 0;
        *(u8*)D_800591AC__s = saved;
        spr->slotLo = slot;
        spr->slotHi = slot >> 2;
    }
}

typedef struct { s16 x, y; } Pos;
typedef struct Task {
    u8 pad0[0xC];
    void (*done)(struct Task*);
    u8 pad10[0xC];
    s32 id;
} Task;
extern void func_800A979C(s32, s16, s16, s32, s16);
extern void func_800BB350(u32);
extern u16 D_800C3666[];
extern Pos D_800C3668[];
extern u8 D_800C3CB8[];
extern s32 D_800C35D8[];
extern u8 D_800C37CC[];

void func_800BB540(Task* t) {
    s32 i;
    s32 bit;
    s32 id;

    for (i = 0, bit = 1; i != 3; i++, bit <<= 1) {
        if (!(D_800C3666[0] & bit)) {
            D_800C3666[0] |= bit;
            break;
        }
    }
    func_800A979C(t->id, D_800C3668[i].x, D_800C3668[i].y, 0, *(u16*)&t->id + 0x1C0);
    id = t->id;
    D_800C3CB8[0]--;
    func_800BB350(id);
    t->done(t);
    if (--D_800C35D8[0] == 0) {
        D_800C37CC[0] = 1;
    }
}

void func_800BB620(u8* p) {
    u8* stack;

    if (ArchiveDataSync() == 0) {
        stack = HeapAlloc(0x1000, 1);
        /* run func_800BB540 on the heap block's stack (top at +0xF00) */
        __asm__ volatile("addu $8, %0, $0\n\tsw $sp, 0($8)\n\taddiu $8, $8, -4\n\taddu $sp, $8, $0"
                         : : "r"(stack + 0xF00) : "$8");
        func_800BB540(p);
        __asm__ volatile("addiu $sp, $sp, 4\n\tlw $sp, 0($sp)");
        HeapFree(stack);
    }
}

#endif


extern u8* TimerWorkListAllocateTask(u32 owner, u32 size);
extern void TimerWorkListSetTaskCallback(void* pTask, void* callback);
extern u32 func_800B57E4(u8* s);
extern void func_800B5B3C(u8* task);
extern void func_800B5854(void);
extern void func_800BF73C(void);
extern void func_800B5CC0(void);
extern void func_800BDC14(void);
extern void func_800BDF1C(void);
extern void func_8001E148(u32 v);
extern void func_800C08CC(u32 a0, void* a1, void* a2);
extern void func_800B51B0(void);
extern void func_800245D8(void* object, s16 animation);
#ifndef XENO_PC_PORT
extern u8 D_800C3EB0[];
#endif
extern u32 WorkListsAddTasks(u32 a0, u32 a1, void* a2, void* a3, void* a4);
extern void func_800B7424(u32 p);
extern void func_800B7364(void);
extern void func_800B6F0C(void);
extern void func_800B7134(void);
extern void WorkListSetTaskCallback(void* pTask, void* callback);
extern void D_80025A88(void);
#ifndef XENO_PC_PORT
extern u8 D_800D3420[];
#endif
#ifndef XENO_PC_PORT
extern u32 D_800C3CE8[];
#endif
extern void WorkListTaskSetOnFreeCallback(void* pTask, void* callback);
extern void func_800B3358(u8* p);
extern void func_800B3588(u8* p);
#ifndef XENO_PC_PORT
extern u32 D_800C3548[];
#endif


/* func_800BB690.s */
#ifndef XENO_PC_PORT
extern u8 D_800C3CB8[];
#endif
extern void func_800A9540(u32 v);
extern void func_800BB620(u8* p);
extern void TimerWorkListSetTaskCallback(void* pTask, void* callback);
#ifndef XENO_PC_PORT /* port: lifted body in the h-frontier block */
void func_800BB690(u8* p) {
    func_800A9540(*(u32*)(p + 0x1C));
    D_800C3CB8[0] = D_800C3CB8[0] + 1;
    TimerWorkListSetTaskCallback(p, func_800BB620);
}
#endif

#ifdef XENO_PC_PORT
#include "battle_host_guest.h"
#include "psyq/libgte.h"
/* Native owners: src/slus_006.64/system/{work_list,memory,temp1}.c, PsyCross. */
extern void WorkListRemoveTask(void* pEntry);
extern void TimerWorkListRemoveTask(void* pEntry);
extern void* HeapAlloc(u_int allocSize, u_int allocFlags);
extern u_int HeapFree(void* pMem);
extern s32 ArchiveDataSync(void);
extern s32 g_NumTimerWorkListEntries;
extern u8 D_800591AC;
extern unsigned char D_800591A8[4];
extern s32 D_80050100;
extern void AnimScriptTick(void* object);
extern void func_80022CDC(void* pSpriteData);
extern void func_80023804(void* pSpriteData);
extern void func_800239A0(void* pSpriteData);
extern void SpriteSetScale(void* object, short scale);
/* Overlay callees. */
extern void func_800BC404(u32 v);
extern void func_800B8D7C(void);
extern void func_800BE790(void);
extern void func_800BADD4(u32 i);
extern void func_800BB760(u32 i);
extern void func_800B14CC(u32 skip);
extern s32 func_800AA600(u32 slot);
extern void func_800A979C(u32 slot, s32 x, s32 z, s32 a3, s32 a4);
void func_800BB350(u32 slot);
void func_800BB540(u8* task);

/* func_800BAF48.s: retire party slot `slot`: flush its command mask
 * (func_800BC404), play animation 0x16 on its actor
 * (D_800C3EB0 + 0x8C8C table) and pump frames (func_800BE790) until the
 * animation ends, the timer list drains back to its entry count, the slot
 * is released (func_800BADD4 / func_800BB760) and D_800C35D8 reaches 0. */
void func_800BAF48(u32 slot) {
    u32 actor;
    s32 count;

    func_800BC404(1u << (slot & 31));
    func_800BC404(0);
    count = g_NumTimerWorkListEntries;
    actor = BG_U32(0x800C3EB0u + slot * 4 + 0x8C8C);
    func_800B8D7C();
    func_800245D8(BG_PTR(actor), 0x16);
    if (BG_S16(actor + 0x9E) != 0) {
        while (BG_S8(actor + 0xAF) == 0x16) {
            func_800BE790();
            if (BG_S16(actor + 0x9E) == 0) {
                break;
            }
        }
    }
    while (g_NumTimerWorkListEntries != count) {
        func_800BE790();
    }
    func_800BADD4(slot);
    func_800BE790();
    func_800BE790();
    func_800BB760(slot);
    while (BG_U32(0x800C35D8) != 0) { /* D_800C35D8 */
        func_800BE790();
    }
}

/* func_800BB080.s: for slots 0..2 other than `skip`, an occupied actor
 * (D_800C3EB0 + 0x8C8C) with a zero +0x48 has its owner's (+0x6C) +0xC
 * callback called on the owner, then both slot tables (+0x8C8C, +0x8CB8)
 * are cleared; finally func_800B14CC(skip). */
void func_800BB080(u32 skip) {
    u32 base = 0x800C3EB0u; /* D_800C3EB0 */
    u32 i;

    for (i = 0; i != 3; i++, base += 4) {
        u32 actor;
        u32 owner;

        if (i == skip) {
            continue;
        }
        actor = BG_U32(base + 0x8C8C);
        if (actor == 0) {
            continue;
        }
        if (BG_U32(actor + 0x48) != 0) {
            continue;
        }
        owner = BG_U32(actor + 0x6C);
        bg_jalr(BG_U32(owner + 0xC), owner, 0, 0, 0);
        BG_U32(base + 0x8C8C) = 0;
        BG_U32(base + 0x8CB8) = 0;
    }
    func_800B14CC(skip);
}

/* func_800BB13C.s: timer callback of a slot actor (task +4).  When the
 * slot's model record (D_800D3368[slot]) exists: clear +0x64 once the
 * animation (+0x9E) is idle, tick the animation script (twice when +0xAC
 * bit 6 is set) while +0x64 is non-zero, then copy the model's
 * +0x5C/+0x60/+0x64 position into the actor's 16.16 x/y/z. */
void func_800BB13C(u8* task) {
    u32 actor = *(u32*)(task + 4);
    u32 slot = ((BG_U32(actor + 0xAC) & 3) << 2) | (BG_U32(actor + 0xA8) >> 30);
    u32 model = BG_U32(0x800D3368u + slot * 4); /* D_800D3368 */

    if (model == 0) {
        return;
    }
    if (BG_S16(actor + 0x9E) == 0) {
        BG_U32(actor + 0x64) = 0;
    }
    if (BG_U32(actor + 0x64) != 0) {
        AnimScriptTick(BG_PTR(actor));
        func_80022CDC(BG_PTR(actor));
        if (((BG_U32(actor + 0xAC) >> 6) & 1) != 0) {
            AnimScriptTick(BG_PTR(actor));
            func_80022CDC(BG_PTR(actor));
        }
    }
    BG_U32(actor + 0) = BG_U32(BG_U32(model + 4) + 0x5C) << 16;
    BG_U32(actor + 4) = BG_U32(BG_U32(model + 4) + 0x60) << 16;
    BG_U32(actor + 8) = BG_U32(BG_U32(model + 4) + 0x64) << 16;
}

/* func_800BB248.s: work callback of a slot actor (task +4): refresh its
 * scale (+0x36 = func_800AA600(slot), +0x38 = half of it), project its
 * (+2, +6, +0xA) position through the D_800D30BC matrix and store the
 * OT depth (>> D_80050100, + actor +0x30; 0 when the flag bit 15 is set)
 * at +0x2E. */
#ifndef XENO_PC_PORT /* port: lifted body in the h-frontier block */
void func_800BB248(u8* task) {
    u32 actor = *(u32*)(task + 4);
    u32 slot = ((BG_U32(actor + 0xAC) & 3) << 2) | (BG_U32(actor + 0xA8) >> 30);
    SVECTOR v;
    long sxy;
    long flag;
    s32 z;
    s32 bias;

    BG_U16(actor + 0x36) = (u16)func_800AA600(slot);
    BG_U16(actor + 0x38) = (u16)(BG_U16(actor + 0x36) >> 1);
    v.vx = BG_S16(actor + 2);
    v.vy = BG_S16(actor + 6);
    v.vz = BG_S16(actor + 0xA);
    v.pad = 0;
    SetRotMatrix((MATRIX*)BG_PTR(0x800D30BCu)); /* D_800D30BC */
    SetTransMatrix((MATRIX*)BG_PTR(0x800D30BCu));
    sxy = 0;
    flag = 0;
    z = RotTransPers(&v, (int*)&sxy, &sxy, &flag);
    bias = BG_S16(actor + 0x30);
    z = (s32)((u32)(z >> (D_80050100 & 31)) + (u32)bias);
    if (((u32)flag & 0x8000) != 0) {
        z = 0;
    }
    BG_U16(actor + 0x2E) = (u16)z;
}
#endif

/* func_800BB314.s: free callback of a slot actor task: unlink its work and
 * timer entries and free the block. */
void func_800BB314(u8* task) {
    WorkListRemoveTask(task + 0x1C);
    TimerWorkListRemoveTask(task);
    HeapFree(task);
}

/* func_800BB350.s: create the actor for party slot `slot` when its
 * D_800C3EB0 + 0x8C8C entry is empty: a 0x19C-byte work/timer task pair
 * (callbacks func_800BB13C / func_800BB248 / func_800BB314) whose sprite at
 * +0x38 is initialised from the slot's 0x1C-byte D_800C3EB0 record and
 * registered in both slot tables (+0x8C8C sprite, +0x8CB8 task).  The heap
 * flag D_800591AC is forced to 0 around the allocation. */
void func_800BB350(u32 slot) {
    u32 fp = 0x800C3EB0u; /* D_800C3EB0 */
    u32 s7 = slot * 4 + fp;
    u32 s6 = s7 + 0x8C8C;

    if (BG_U32(s6) == 0) {
        u8 saved = D_800591AC;
        u32 s1;
        u32 s0;
        u32 a0;
        u32 v0;
        u32 v1;
        u32 rec;

        D_800591AC = 0;
        s1 = (u32)(uintptr_t)WorkListsAddTasks(0x19C, 0,
                                               (void*)(uintptr_t)0x800BB13Cu,
                                               (void*)(uintptr_t)0x800BB248u,
                                               (void*)(uintptr_t)0x800BB314u);
        s0 = s1 + 0x38;
        BG_U32(s0 + 0x6C) = s1;
        BG_U32(s1 + 4) = s0;
        BG_U32(s1 + 0x20) = s0;
        func_80023804(BG_PTR(s0));
        func_800239A0(BG_PTR(s0));
        v0 = BG_U32(s0 + 0x40);
        a0 = BG_U32(s0 + 0x7C);
        BG_U32(s0 + 0x40) = (v0 & 0xFFFE1FFFu) | 0x8000;
        v0 = BG_U32(s0 + 0x3C);
        BG_U32(s0 + 0x6C) = s1;
        BG_U32(s0 + 0x3C) = v0 & ~3u;
        v0 = BG_U32(s0 + 0xA8);
        BG_U32(s0 + 0xA8) = v0 & ~1u;
        BG_U32(a0 + 8) = 0;
        v1 = BG_U32(s0 + 0x7C);
        v0 = *(u16*)D_800591A8;
        BG_U16(v1 + 0xC) = 0;
        rec = slot * 0x1C + fp;
        BG_U16(s0 + 0x82) = (u16)v0;
        v0 = BG_U16(rec + 0xE);
        BG_U32(s1 + 0x38) = v0 << 16;
        v0 = BG_U16(rec + 0x10);
        BG_U32(s0 + 4) = 0;
        BG_U32(s0 + 8) = v0 << 16;
        BG_U16(s0 + 0x36) = (u16)func_800AA600(slot);
        v0 = BG_U16(s0 + 0x36);
        BG_U16(s0 + 0x82) = 0x2000;
        BG_U16(s0 + 0x38) = (u16)(v0 >> 1);
        SpriteSetScale(BG_PTR(s0), 0x2000);
        /* Retail stores the main-exe address of D_8006BE10 (lui/addiu). */
        BG_U32(s0 + 0x24) = 0x8006BE10u;
        BG_U32(s6) = s0;
        BG_U32(s7 + 0x8CB8) = s1;
        a0 = BG_U32(s0 + 0xA8);
        BG_U32(s0 + 0x4C) = 0;
        BG_U32(s0 + 0x48) = 0;
        D_800591AC = saved;
        a0 = (a0 & 0x3FFFFFFFu) | (slot << 30);
        v1 = BG_U32(s0 + 0xAC);
        BG_U32(s0 + 0xA8) = a0;
        BG_U32(s0 + 0xAC) = (v1 & ~3u) | ((slot >> 2) & 3);
    }
}

/* func_800BB540.s: claim the first free bit of D_800C3666 (3 entries;
 * index 3 when none is free), spawn the task's model (+0x1C) at the
 * matching D_800C3668 (x, z) pair with angle +0x1C + 0x1C0, drop the
 * D_800C3CB8 counter, create the slot actor (func_800BB350), run the task's
 * +0xC callback and set D_800C37CC once D_800C35D8 counts down to 0. */
void func_800BB540(u8* task) {
    u32 i = 0;
    u32 bit = 1;
    u32 pair;
    u32 slot;
    s32 angle;
    u32 n;

    do {
        u32 v = BG_U16(0x800C3666u); /* D_800C3666 */

        if ((v & bit) == 0) {
            BG_U16(0x800C3666u) = (u16)(v | bit);
            break;
        }
        i++;
        bit <<= 1;
    } while (i != 3);
    pair = i * 4 + 0x800C3668u; /* D_800C3668 */
    angle = (s16)(u16)(*(u16*)(task + 0x1C) + 0x1C0);
    func_800A979C(*(u32*)(task + 0x1C), BG_S16(pair), BG_S16(pair + 2), 0, angle);
    n = BG_U8(0x800C3CB8u) - 1; /* D_800C3CB8 */
    slot = *(u32*)(task + 0x1C);
    BG_U8(0x800C3CB8u) = (u8)n;
    func_800BB350(slot);
    bg_jalr(*(u32*)(task + 0xC), BG_ADDR(task), 0, 0, 0);
    n = BG_U32(0x800C35D8u) - 1; /* D_800C35D8 */
    BG_U32(0x800C35D8u) = n;
    if (n == 0) {
        BG_U8(0x800C37CCu) = 1; /* D_800C37CC */
    }
}

/* func_800BB620.s: once the archive is idle, run func_800BB540(task) on a
 * private 0x1000-byte heap stack (retail saves $sp at block + 0xF00 and
 * switches to it; the native body just calls through) and free it. */
void func_800BB620(u8* task) {
    if (ArchiveDataSync() == 0) {
        void* stack = HeapAlloc(0x1000, 1);

        func_800BB540(task);
        HeapFree(stack);
    }
}
#endif

#ifdef XENO_PC_PORT /* h-frontier lifted bodies */
/* Instruction-faithful port bodies (native-only frontier), generated by
 *   tools/scripts/battle_lift_asm.py asm/battle/matchings/<tu>/<fn>.s --args N [--void] --jal-service
 * (include/battle_lift.h; every call goes through the interpreter's own
 * jal path, so each callee gets the binding an interpreted jal gets).  The
 * matched C above stays the matching build's. */
/* All calls use bl_jal; keep the TU's existing host GTE declarations. */
#define BL_NO_GTE
#include "battle_lift.h"
#undef BL_NO_GTE

/* func_800BB248.s (51 instructions; --args 1 --void), frontier step5.  Callees through the jal service: RotTransPers, SetRotMatrix, SetTransMatrix, func_800AA600. */
void func_800BB248(u32 arg0) {
    static const char FN[] = "func_800BB248";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x30u, &bl_saved);
    r_a0 = arg0;
    r_sp = r_sp + 0xFFFFFFD0u; /* 800BB248 addiu $sp, $sp, -0x30 */
    bl_sw(r_sp + 0x28u, r_ra); /* 800BB24C sw $ra, 0x28($sp) */
    bl_sw(r_sp + 0x24u, r_s1); /* 800BB250 sw $s1, 0x24($sp) */
    bl_sw(r_sp + 0x20u, r_s0); /* 800BB254 sw $s0, 0x20($sp) */
    r_s1 = bl_lw(r_a0 + 0x4u); /* 800BB258 lw $s1, 0x4($a0) */
    ; /* 800BB25C nop  */
    r_v0 = bl_lw(r_s1 + 0xA8u); /* 800BB260 lw $v0, 0xA8($s1) */
    r_a0 = bl_lw(r_s1 + 0xACu); /* 800BB264 lw $a0, 0xAC($s1) */
    r_v0 = r_v0 >> 30; /* 800BB268 srl $v0, $v0, 30 */
    r_a0 = r_a0 & 0x3u; /* 800BB26C andi $a0, $a0, 0x3 */
    r_a0 = r_a0 << 2; /* 800BB270 sll $a0, $a0, 2 */
    /* 800BB274 jal func_800AA600 */
    r_a0 = r_a0 | r_v0;
    bl_jal(FN, 0x800AA600u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_800AA600 */
    r_s0 = 0x800D0000u; /* 800BB27C lui $s0, %hi(D_800D30BC) */
    bl_sh(r_s1 + 0x36u, r_v0); /* 800BB280 sh $v0, 0x36($s1) */
    r_v0 = bl_lhu(r_s1 + 0x36u); /* 800BB284 lhu $v0, 0x36($s1) */
    r_v1 = bl_lh(r_s1 + 0x2u); /* 800BB288 lh $v1, 0x2($s1) */
    r_v0 = r_v0 >> 1; /* 800BB28C srl $v0, $v0, 1 */
    bl_sh(r_s1 + 0x38u, r_v0); /* 800BB290 sh $v0, 0x38($s1) */
    bl_sh(r_sp + 0x10u, r_v1); /* 800BB294 sh $v1, 0x10($sp) */
    r_v0 = bl_lh(r_s1 + 0x6u); /* 800BB298 lh $v0, 0x6($s1) */
    r_s0 = r_s0 + 0x30BCu; /* 800BB29C addiu $s0, $s0, %lo(D_800D30BC) */
    bl_sh(r_sp + 0x12u, r_v0); /* 800BB2A0 sh $v0, 0x12($sp) */
    r_v0 = bl_lh(r_s1 + 0xAu); /* 800BB2A4 lh $v0, 0xA($s1) */
    r_a0 = r_s0 + 0u; /* 800BB2A8 addu $a0, $s0, $zero */
    /* 800BB2AC jal SetRotMatrix */
    bl_sh(r_sp + 0x14u, r_v0);
    bl_jal(FN, 0x80049EFCu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* SetRotMatrix */
    /* 800BB2B4 jal SetTransMatrix */
    r_a0 = r_s0 + 0u;
    bl_jal(FN, 0x80049F8Cu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* SetTransMatrix */
    r_a0 = r_sp + 0x10u; /* 800BB2BC addiu $a0, $sp, 0x10 */
    r_a1 = r_sp + 0x18u; /* 800BB2C0 addiu $a1, $sp, 0x18 */
    r_a2 = r_a1 + 0u; /* 800BB2C4 addu $a2, $a1, $zero */
    /* 800BB2C8 jal RotTransPers */
    r_a3 = r_sp + 0x1Cu;
    bl_jal(FN, 0x8004A64Cu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* RotTransPers */
    r_v1 = 0x80050000u; /* 800BB2D0 lui $v1, %hi(D_80050100) */
    r_v1 = bl_lw(r_v1 + 0x100u); /* 800BB2D4 lw $v1, %lo(D_80050100)($v1) */
    r_a0 = bl_lh(r_s1 + 0x30u); /* 800BB2D8 lh $a0, 0x30($s1) */
    r_v0 = BL_SRA(r_v0, r_v1); /* 800BB2DC srav $v0, $v0, $v1 */
    r_v1 = bl_lw(r_sp + 0x1Cu); /* 800BB2E0 lw $v1, 0x1C($sp) */
    ; /* 800BB2E4 nop  */
    r_v1 = r_v1 & 0x8000u; /* 800BB2E8 andi $v1, $v1, 0x8000 */
    /* 800BB2EC beqz $v1, .L800BB2F8 */
    { int c_ = (r_v1 == 0); r_v0 = r_v0 + r_a0; if (c_) goto L_800BB2F8; }
    r_v0 = 0u + 0u; /* 800BB2F4 addu $v0, $zero, $zero */
L_800BB2F8:
    bl_sh(r_s1 + 0x2Eu, r_v0); /* 800BB2F8 sh $v0, 0x2E($s1) */
    r_ra = bl_lw(r_sp + 0x28u); /* 800BB2FC lw $ra, 0x28($sp) */
    r_s1 = bl_lw(r_sp + 0x24u); /* 800BB300 lw $s1, 0x24($sp) */
    r_s0 = bl_lw(r_sp + 0x20u); /* 800BB304 lw $s0, 0x20($sp) */
    r_sp = r_sp + 0x30u; /* 800BB308 addiu $sp, $sp, 0x30 */
    /* 800BB30C jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    (void)r_v0;
}

/* func_800BB690.s (20 instructions; --args 1 --void), frontier step11.  Callees through the jal service: TimerWorkListSetTaskCallback, func_800A9540. */
void func_800BB690(u32 arg0) {
    static const char FN[] = "func_800BB690";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x18u, &bl_saved);
    r_a0 = arg0;
    r_sp = r_sp + 0xFFFFFFE8u; /* 800BB690 addiu $sp, $sp, -0x18 */
    bl_sw(r_sp + 0x10u, r_s0); /* 800BB694 sw $s0, 0x10($sp) */
    r_s0 = r_a0 + 0u; /* 800BB698 addu $s0, $a0, $zero */
    bl_sw(r_sp + 0x14u, r_ra); /* 800BB69C sw $ra, 0x14($sp) */
    r_a0 = bl_lw(r_s0 + 0x1Cu); /* 800BB6A0 lw $a0, 0x1C($s0) */
    /* 800BB6A4 jal func_800A9540 */
    ;
    bl_jal(FN, 0x800A9540u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_800A9540 */
    r_v1 = 0x800C0000u; /* 800BB6AC lui $v1, %hi(D_800C3CB8) */
    r_a0 = r_s0 + 0u; /* 800BB6B0 addu $a0, $s0, $zero */
    r_a1 = 0x800C0000u; /* 800BB6B4 lui $a1, %hi(func_800BB620) */
    r_v0 = bl_lbu(r_v1 + 0x3CB8u); /* 800BB6B8 lbu $v0, %lo(D_800C3CB8)($v1) */
    r_a1 = r_a1 + 0xFFFFB620u; /* 800BB6BC addiu $a1, $a1, %lo(func_800BB620) */
    r_v0 = r_v0 + 0x1u; /* 800BB6C0 addiu $v0, $v0, 0x1 */
    /* 800BB6C4 jal TimerWorkListSetTaskCallback */
    bl_sb(r_v1 + 0x3CB8u, r_v0);
    bl_jal(FN, 0x8001CD6Cu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* TimerWorkListSetTaskCallback */
    r_ra = bl_lw(r_sp + 0x14u); /* 800BB6CC lw $ra, 0x14($sp) */
    r_s0 = bl_lw(r_sp + 0x10u); /* 800BB6D0 lw $s0, 0x10($sp) */
    r_sp = r_sp + 0x18u; /* 800BB6D4 addiu $sp, $sp, 0x18 */
    /* 800BB6D8 jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    (void)r_v0;
}
#endif /* XENO_PC_PORT h-frontier */
