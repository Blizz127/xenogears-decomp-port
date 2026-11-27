#include "common.h"
#ifndef XENO_PC_PORT
#include "system/memory.h"
#include "psyq/libgte.h"
#endif


#ifndef XENO_PC_PORT
extern u8 D_800C3664;
void func_800BAC50(void *arg0);
#endif

#ifndef XENO_PC_PORT
extern u8* WorkListsAddTasks(s32 size, s32 a1, void* cb0, void* cb1, void* cb2);
extern void func_800242F4(u8* sprite, u8* package, s16 a, s16 b, s16 c, s16 d, s16 e, s32 flags);
extern void SpriteSetScale(u8* sprite, s16 scale);
extern void func_800245D8(u8* sprite, s16 anim);
extern void func_800BAB0C(u8* task);
extern void func_800BABDC(u8* task);

/* Create a 0x19C-byte work/timer task pair (callbacks func_800BAC50 /
 * func_800BAB0C / func_800BABDC) whose sprite at +0x38 is built by
 * func_800242F4 from `package`, positioned at (x, y, z) 16.16, given
 * animation `anim` (+0xB0), +0x32 = `arg10` and scale 0x2000; returns the
 * task.  Stack arguments 11 and 12 are not read. */
u8* func_800BA984(u8* package, s32 a1, s32 a2, s32 a3, s32 arg4, s32 arg5, s32 x, s16 y, s16 z,
                  u16 anim, s16 arg10, s32 arg11, s32 arg12, s32 flags) {
    u8* task;
    u8* sprite;

    task = WorkListsAddTasks(0x19C, 0, func_800BAC50, func_800BAB0C, func_800BABDC);
    sprite = task + 0x38;
    *(u8**)(task + 4) = sprite;
    *(u8**)(task + 0x20) = sprite;
    *(s32*)(task + 0x1C) = 0;
    func_800242F4(sprite, package, a1, a2, a3, arg4, arg5, flags);
    *(u8**)(sprite + 0x6C) = task;
    *(s32*)(task + 0x38) = x << 16;
    *(s32*)(sprite + 4) = y << 16;
    *(s32*)(sprite + 8) = z << 16;
    sprite[0xB0] = anim;
    *(u32*)(sprite + 0x3C) |= 4;
    *(s16*)(sprite + 0x32) = arg10;
    SpriteSetScale(sprite, 0x2000);
    *(s16*)(sprite + 0x82) = 0x2000;
    *(s32*)(sprite + 0x78) = 0;
    func_800245D8(sprite, anim);
    return task;
}
extern MATRIX D_800D30BC;
extern s32 D_80050100;
extern u_long* g_GfxCurOT;
extern void func_8001E298(void* pSpriteData, void* ot);

/* Work callback: unless D_800C3664 is set, project the integer part of the
 * sprite's (task +4) 16.16 position through D_800D30BC, store its OT depth
 * (>> D_80050100, + sprite +0x30; 0 when flag bit 15 is set) at +0x2E and
 * render it into g_GfxCurOT[depth] when 1 <= depth <= 0xFFF.  The render
 * branch owns 16 bytes of unused stack. */
void func_800BAB0C(u8* task) {
    SVECTOR v;
    long sxy;
    long flag;
    u8* o;
    s32 z;

    if (D_800C3664 == 0) {
        o = *(u8**)(task + 4);
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
        if (z > 0 && z < 0x1000) {
            long pad[4];
            func_8001E298(o, g_GfxCurOT + z);
        }
    }
}
void func_800BABDC__ret(u8* arg0) asm("func_800BABDC");

void func_800BABDC__ret(u8* arg0) {
    register s32 temp_v0 asm("$2");
    register void *temp_s1 asm("$17");
    register u8* task asm("$16");

    temp_s1 = arg0 + 0x38;
    temp_v0 = *(s32 *)((s8*)(*(void **)((s8*)(temp_s1) + 0x20)) + 0x2C);
    task = arg0;
    if (temp_v0 != 0) {
        HeapFree(temp_v0);
    }
    func_8001CE74(task);
    func_8001D3F4(temp_s1);
    WorkListRemoveTask(task + 0x1C);
    TimerWorkListRemoveTask(task);
    HeapFree(task);
}


#ifndef XENO_PC_PORT
void func_800BAC50(void *arg0) {
    void *temp_s0;

    temp_s0 = *(void **)((s8*)(arg0) + 4);
    if (D_800C3664 == 0) {
        AnimScriptTick(temp_s0);
        func_80022CDC(temp_s0);
        if (((u32) *(u32 *)((s8*)(temp_s0) + 0xAC) >> 6) & 1) {
            AnimScriptTick(temp_s0);
            func_80022CDC(temp_s0);
        }
    }
}
#endif

extern u8 D_800C3EB0[];

/* func_800BACBC.s: project party slot `slot`'s actor (D_800C3EB0 + 0x8C8C
 * table; 16.16 position at +0/+4/+8) through D_800D30BC:
 * *depth = RotTransPers' result >> 4, *sx / *sy = the screen position,
 * *left = sx - 0x30, *width = 0x30, *mid = sx - 0x18.  Reading the
 * integer parts as `word >> 16` gives retail's signed `lh` of the high
 * halves; plain s16 reads load with `lhu`. */
void func_800BACBC(u32 slot, s16* sx, s16* sy, s16* depth, s16* left, s16* width, s16* mid) {
    u8* base = (u8*)(u32)D_800C3EB0;
    u8* actor = *(u8**)(slot * 4 + base + 0x8C8C);
    SVECTOR v;
    u16 sxy[4];
    long flag;

    v.vx = *(s32*)(actor + 0) >> 16;
    v.vy = *(s32*)(actor + 4) >> 16;
    v.vz = *(s32*)(actor + 8) >> 16;
    PushMatrix();
    SetRotMatrix(&D_800D30BC);
    SetTransMatrix(&D_800D30BC);
    *depth = RotTransPers(&v, (long*)sxy, &flag, &flag) >> 4;
    *sx = sxy[0];
    *sy = sxy[1];
    *left = sxy[0] - 0x30;
    *width = 0x30;
    *mid = sxy[0] - 0x18;
    PopMatrix();
}
extern void func_800BFC80(u8* task, s32 a1, s32 a2);
extern void func_8001CE74(u8* task);

/* Release party slot `slot`'s task (D_800C3EB0 + 0x8CB8): func_800BFC80
 * (task, 0, 2), free a slot < 3's 12-byte record buffer at D_800C3EB0 +
 * 0x8D24, run the task's +0xC callback, release its children and clear both
 * slot tables. */
void func_800BADD4(s32 slot) {
    u8* base;
    u8* task;
    u8* rec;
    u8* tbl;

    base = (u8*)(u32)D_800C3EB0;
    task = *(u8**)(slot * 4 + base + 0x8CB8);
    if (task != NULL) {
        func_800BFC80(task, 0, 2);
        if (slot < 3) {
            rec = slot * 12 + base + 0x8000;
            if (*(void**)(rec + 0xD24) != NULL) {
                HeapFree(*(void**)(rec + 0xD24));
            }
            *(void**)(rec + 0xD24) = NULL;
        }
        (*(void (**)(u8*))(task + 0xC))(task);
        func_8001CE74(task);
        tbl = (u8*)(u32)D_800C3EB0;
        *(u8**)(slot * 4 + tbl + 0x8C8C) = NULL;
        *(u8**)(slot * 4 + tbl + 0x8CB8) = NULL;
    }
}
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3EB0[];
#endif
extern void func_800223B0(u8* p, s32 v);
extern void func_80021FE0(u8* p, s32 v);
#ifdef XENO_PC_PORT
/* Port body; the retail build compiles the byte-exact C in the #else branch.
 */
/* func_800BAEB8.s: for party slot a0 (pointer table at D_800C3EB0 + 0x8C8C),
 * unless its +0xAF state byte is 0x15, apply flag 0x800 (set when the slot's
 * 28-byte record byte +0xA is nonzero) through func_800223B0/func_80021FE0. */
void func_800BAEB8(s32 a0) {
    u8* s1 = ((u8**)(D_800C3EB0 + 0x8C8C))[a0];

    if (*(s8*)(s1 + 0xAF) != 0x15) {
        s32 f = (D_800C3EB0[a0 * 28 + 0xA] != 0) << 11;

        func_800223B0(s1, f);
        func_80021FE0(s1, f);
    }
}
#else
void func_800BAEB8(s32 arg0) {
    s32 temp_s0;
    u8 *base = D_800C3EB0;
    void *temp_s1;

    temp_s1 = *(void **)((s8*)(((arg0 * 4) + base)) + 0x8C8C);
    if (*(s8 *)((s8*)(temp_s1) + 0xAF) != 0x15) {
        temp_s0 = (*(u8 *)((s8*)(((arg0 * 0x1C) + base)) + 0xA) != 0) << 0xB;
        func_800223B0(temp_s1, temp_s0);
        func_80021FE0(temp_s1, temp_s0);
    }
}

#endif
#ifndef XENO_PC_PORT
extern u8* D_800D2D28;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800D2DC8;
#endif


/* func_800BAF40.s */
void func_800BAF40(void) {
}

#ifdef XENO_PC_PORT
#include "battle_host_guest.h"
#include "psyq/libgte.h"
/* Native owners: src/slus_006.64/system/{work_list,memory,rendering,temp1}.c,
 * PsyCross. */
extern u32 WorkListsAddTasks(u32 a0, u32 a1, void* a2, void* a3, void* a4);
extern void WorkListRemoveTask(void* pEntry);
extern void TimerWorkListRemoveTask(void* pEntry);
extern u_int HeapFree(void* pMem);
extern void func_8001CE74(void* pTargetEntry);
extern void func_8001D3F4(void* pTargetSprite);
extern void func_8001E298(void* pSpriteData, void* ot);
extern void* func_800242F4(void* pSpriteData, void* pAnimPackage, s16 texX,
                           s16 texY, s16 clutX, s16 clutY, s16 arg6, s32 flags);
extern void SpriteSetScale(void* object, short scale);
extern void func_800245D8(void* object, s16 animation);
extern void AnimScriptTick(void* object);
extern void func_80022CDC(void* pSpriteData);
extern u_long* g_GfxCurOT;
extern s32 D_80050100;
/* Overlay callee (src/battle/mainl133.c). */
extern u32 func_800BFC80(u32 sprite, u32 anim, u32 mode);

/* func_800BA984.s: create a 0x19C-byte work/timer task pair (callbacks
 * func_800BAC50 / func_800BAB0C / func_800BABDC) whose sprite at +0x38 is
 * built by func_800242F4 from `package`, positioned at (x, y, z) 16.16,
 * given animation `anim` (+0xB0), +0x32 = `arg10`, scale 0x2000; returns the
 * task.  Stack arguments 11 and 12 are not read. */
#ifndef XENO_PC_PORT /* port: lifted body in the h-frontier block */
u32 func_800BA984(u8* package, s32 a1, s32 a2, s32 a3, s32 arg4, s32 arg5,
                  u32 x, u32 y, u32 z, u32 anim, u32 arg10, u32 arg11,
                  u32 arg12, s32 flags) {
    u32 s1;
    u32 s0;
    u32 v1;

    (void)arg11;
    (void)arg12;
    s1 = WorkListsAddTasks(0x19C, 0, (void*)(uintptr_t)0x800BAC50u,
                           (void*)(uintptr_t)0x800BAB0Cu,
                           (void*)(uintptr_t)0x800BABDCu);
    s0 = s1 + 0x38;
    BG_U32(s1 + 4) = s0;
    BG_U32(s1 + 0x20) = s0;
    BG_U32(s1 + 0x1C) = 0;
    func_800242F4(BG_PTR(s0), package, (s16)a1, (s16)a2, (s16)a3, (s16)arg4,
                  (s16)arg5, flags);
    BG_U32(s0 + 0x6C) = s1;
    BG_U32(s1 + 0x38) = x << 16;
    v1 = BG_U32(s0 + 0x3C);
    BG_U32(s0 + 4) = y << 16;
    BG_U32(s0 + 8) = z << 16;
    BG_U8(s0 + 0xB0) = (u8)anim;
    BG_U32(s0 + 0x3C) = v1 | 4;
    BG_U16(s0 + 0x32) = (u16)arg10;
    SpriteSetScale(BG_PTR(s0), 0x2000);
    BG_U16(s0 + 0x82) = 0x2000;
    BG_U32(s0 + 0x78) = 0;
    func_800245D8(BG_PTR(s0), (s16)anim);
    return s1;
}
#endif

/* func_800BAB0C.s: work callback: unless D_800C3664 is set, project the
 * sprite (task +4) position (+2, +6, +0xA) through D_800D30BC, store its OT
 * depth (>> D_80050100, + sprite +0x30; 0 when flag bit 15 is set) at +0x2E
 * and render it into g_GfxCurOT[depth] when 1 <= depth <= 0xFFF. */
#ifndef XENO_PC_PORT /* port: lifted body in the h-frontier block */
void func_800BAB0C(u8* task) {
    u32 sprite;
    SVECTOR v;
    long sxy;
    long flag;
    s32 z;

    if (BG_U8(0x800C3664u) != 0) { /* D_800C3664 */
        return;
    }
    sprite = *(u32*)(task + 4);
    v.vx = BG_S16(sprite + 2);
    v.vy = BG_S16(sprite + 6);
    v.vz = BG_S16(sprite + 0xA);
    v.pad = 0;
    SetRotMatrix((MATRIX*)BG_PTR(0x800D30BCu)); /* D_800D30BC */
    SetTransMatrix((MATRIX*)BG_PTR(0x800D30BCu));
    sxy = 0;
    flag = 0;
    z = RotTransPers(&v, (int*)&sxy, &sxy, &flag);
    z = (s32)((u32)(z >> (D_80050100 & 31)) + (u32)(s32)BG_S16(sprite + 0x30));
    if (((u32)flag & 0x8000) != 0) {
        z = 0;
    }
    BG_U16(sprite + 0x2E) = (u16)z;
    if ((u32)z - 1u < 0xFFFu) {
        func_8001E298(BG_PTR(sprite), (u8*)g_GfxCurOT + ((u32)z << 2));
    }
}
#endif

/* func_800BABDC.s: free callback: free the sprite's (+0x38) +0x20 record's
 * +0x2C buffer when set, release the sprite's children (func_8001CE74) and
 * the sprite (func_8001D3F4), unlink both list entries and free the task. */
#ifndef XENO_PC_PORT /* port: lifted body in the h-frontier block */
void func_800BABDC(u8* task) {
    u8* sprite = task + 0x38;
    u32 buffer = BG_U32(*(u32*)(sprite + 0x20) + 0x2C);

    if (buffer != 0) {
        HeapFree(BG_PTR(buffer));
    }
    func_8001CE74(task);
    func_8001D3F4(sprite);
    WorkListRemoveTask(task + 0x1C);
    TimerWorkListRemoveTask(task);
    HeapFree(task);
}
#endif

/* func_800BAC50.s: timer callback: unless D_800C3664 is set, tick the
 * sprite's (task +4) animation script, twice when +0xAC bit 6 is set. */
void func_800BAC50(u8* task) {
    u32 sprite = *(u32*)(task + 4);

    if (BG_U8(0x800C3664u) != 0) { /* D_800C3664 */
        return;
    }
    AnimScriptTick(BG_PTR(sprite));
    func_80022CDC(BG_PTR(sprite));
    if (((BG_U32(sprite + 0xAC) >> 6) & 1) != 0) {
        AnimScriptTick(BG_PTR(sprite));
        func_80022CDC(BG_PTR(sprite));
    }
}

/* func_800BACBC.s: project party slot `slot`'s actor position (D_800C3EB0 +
 * 0x8C8C table) through D_800D30BC: *depth = OTZ >> 4, *sx / *sy = the
 * screen position, *left = sx - 0x30, *width = 0x30, *mid = sx - 0x18. */
void func_800BACBC(u32 slot, s16* sx, s16* sy, s16* depth, s16* left,
                   s16* width, s16* mid) {
    u32 actor = BG_U32(0x800C3EB0u + slot * 4 + 0x8C8C);
    SVECTOR v;
    union {
        long word;
        u16 half[2];
    } sxy;
    long flag;
    s32 z;

    v.vx = BG_S16(actor + 2);
    v.vy = BG_S16(actor + 6);
    v.vz = BG_S16(actor + 0xA);
    v.pad = 0;
    PushMatrix();
    SetRotMatrix((MATRIX*)BG_PTR(0x800D30BCu)); /* D_800D30BC */
    SetTransMatrix((MATRIX*)BG_PTR(0x800D30BCu));
    sxy.word = 0;
    flag = 0;
    z = RotTransPers(&v, (int*)&sxy.word, &flag, &flag);
    *depth = (s16)(z >> 4);
    *(u16*)sx = sxy.half[0];
    *(u16*)sy = sxy.half[1];
    *(u16*)left = (u16)(sxy.half[0] - 0x30);
    *width = 0x30;
    *(u16*)mid = (u16)(sxy.half[0] - 0x18);
    PopMatrix();
}

/* func_800BADD4.s: release party slot `slot`'s task (D_800C3EB0 + 0x8CB8):
 * func_800BFC80(task, 0, 2), free a slot < 3's 12-byte record buffer at
 * D_800C3EB0 + 0x8D24, run the task's +0xC callback, release its children
 * and clear both slot tables. */
void func_800BADD4(u32 slot) {
    u32 base = 0x800C3EB0u; /* D_800C3EB0 */
    u32 task = BG_U32(slot * 4 + base + 0x8CB8);

    if (task == 0) {
        return;
    }
    func_800BFC80(task, 0, 2);
    if ((s32)slot < 3) {
        u32 rec = slot * 12 + base + 0x8000;
        u32 buffer = BG_U32(rec + 0xD24);

        if (buffer != 0) {
            HeapFree(BG_PTR(buffer));
        }
        BG_U32(rec + 0xD24) = 0;
    }
    bg_jalr(BG_U32(task + 0xC), task, 0, 0, 0);
    func_8001CE74(BG_PTR(task));
    BG_U32(slot * 4 + base + 0x8C8C) = 0;
    BG_U32(slot * 4 + base + 0x8CB8) = 0;
}
#endif
