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

#ifdef XENO_PC_PORT /* h-frontier lifted bodies */
/* Instruction-faithful port bodies (native-only frontier), generated by
 *   tools/scripts/battle_lift_asm.py asm/battle/matchings/<tu>/<fn>.s --args N [--void] --jal-service
 * (include/battle_lift.h; every call goes through the interpreter's own
 * jal path, so each callee gets the binding an interpreted jal gets).  The
 * matched C above stays the matching build's. */
#define BL_NO_GTE /* no inline GTE here (its GTE library calls go through the jal
                   * service); main111 declares its own MATRIX */
#include "battle_lift.h"

/* func_800BA984.s (98 instructions; --args 14), frontier step7.  Callees through the jal service: SpriteSetScale, WorkListsAddTasks, func_800242F4, func_800245D8. */
u32 func_800BA984(u32 arg0, u32 arg1, u32 arg2, u32 arg3, u32 arg4, u32 arg5, u32 arg6, u32 arg7, u32 arg8, u32 arg9, u32 arg10, u32 arg11, u32 arg12, u32 arg13) {
    static const char FN[] = "func_800BA984";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x58u, &bl_saved);
    r_a0 = arg0;
    r_a1 = arg1;
    r_a2 = arg2;
    r_a3 = arg3;
    bl_sw(r_sp + 0x10u, arg4);
    bl_sw(r_sp + 0x14u, arg5);
    bl_sw(r_sp + 0x18u, arg6);
    bl_sw(r_sp + 0x1Cu, arg7);
    bl_sw(r_sp + 0x20u, arg8);
    bl_sw(r_sp + 0x24u, arg9);
    bl_sw(r_sp + 0x28u, arg10);
    bl_sw(r_sp + 0x2Cu, arg11);
    bl_sw(r_sp + 0x30u, arg12);
    bl_sw(r_sp + 0x34u, arg13);
    r_sp = r_sp + 0xFFFFFFA8u; /* 800BA984 addiu $sp, $sp, -0x58 */
    bl_sw(r_sp + 0x44u, r_s5); /* 800BA988 sw $s5, 0x44($sp) */
    r_s5 = r_a1 + 0u; /* 800BA98C addu $s5, $a1, $zero */
    bl_sw(r_sp + 0x48u, r_s6); /* 800BA990 sw $s6, 0x48($sp) */
    r_s6 = r_a2 + 0u; /* 800BA994 addu $s6, $a2, $zero */
    bl_sw(r_sp + 0x40u, r_s4); /* 800BA998 sw $s4, 0x40($sp) */
    r_s4 = r_a3 + 0u; /* 800BA99C addu $s4, $a3, $zero */
    bl_sw(r_sp + 0x58u, r_a0); /* 800BA9A0 sw $a0, 0x58($sp) */
    r_a0 = 0u + 0x19Cu; /* 800BA9A4 addiu $a0, $zero, 0x19C */
    r_a1 = 0u + 0u; /* 800BA9A8 addu $a1, $zero, $zero */
    bl_sw(r_sp + 0x38u, r_s2); /* 800BA9AC sw $s2, 0x38($sp) */
    r_s2 = bl_lw(r_sp + 0x68u); /* 800BA9B0 lw $s2, 0x68($sp) */
    r_a2 = 0x800C0000u; /* 800BA9B4 lui $a2, %hi(func_800BAC50) */
    bl_sw(r_sp + 0x3Cu, r_s3); /* 800BA9B8 sw $s3, 0x3C($sp) */
    r_s3 = bl_lw(r_sp + 0x6Cu); /* 800BA9BC lw $s3, 0x6C($sp) */
    r_a2 = r_a2 + 0xFFFFAC50u; /* 800BA9C0 addiu $a2, $a2, %lo(func_800BAC50) */
    bl_sw(r_sp + 0x4Cu, r_s7); /* 800BA9C4 sw $s7, 0x4C($sp) */
    r_s7 = bl_lw(r_sp + 0x70u); /* 800BA9C8 lw $s7, 0x70($sp) */
    r_t0 = bl_lw(r_sp + 0x74u); /* 800BA9CC lw $t0, 0x74($sp) */
    r_a3 = 0x800C0000u; /* 800BA9D0 lui $a3, %hi(func_800BAB0C) */
    bl_sw(r_sp + 0x50u, r_fp); /* 800BA9D4 sw $fp, 0x50($sp) */
    r_fp = bl_lw(r_sp + 0x7Cu); /* 800BA9D8 lw $fp, 0x7C($sp) */
    r_a3 = r_a3 + 0xFFFFAB0Cu; /* 800BA9DC addiu $a3, $a3, %lo(func_800BAB0C) */
    bl_sw(r_sp + 0x20u, r_t0); /* 800BA9E0 sw $t0, 0x20($sp) */
    r_t0 = bl_lw(r_sp + 0x78u); /* 800BA9E4 lw $t0, 0x78($sp) */
    r_v0 = 0x800C0000u; /* 800BA9E8 lui $v0, %hi(func_800BABDC) */
    bl_sw(r_sp + 0x24u, r_t0); /* 800BA9EC sw $t0, 0x24($sp) */
    r_t0 = bl_lw(r_sp + 0x80u); /* 800BA9F0 lw $t0, 0x80($sp) */
    r_v0 = r_v0 + 0xFFFFABDCu; /* 800BA9F4 addiu $v0, $v0, %lo(func_800BABDC) */
    bl_sw(r_sp + 0x54u, r_ra); /* 800BA9F8 sw $ra, 0x54($sp) */
    bl_sw(r_sp + 0x34u, r_s1); /* 800BA9FC sw $s1, 0x34($sp) */
    bl_sw(r_sp + 0x30u, r_s0); /* 800BAA00 sw $s0, 0x30($sp) */
    bl_sw(r_sp + 0x10u, r_v0); /* 800BAA04 sw $v0, 0x10($sp) */
    /* 800BAA08 jal WorkListsAddTasks */
    bl_sw(r_sp + 0x28u, r_t0);
    bl_jal(FN, 0x8001D1D8u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* WorkListsAddTasks */
    r_s1 = r_v0 + 0u; /* 800BAA10 addu $s1, $v0, $zero */
    r_s0 = r_s1 + 0x38u; /* 800BAA14 addiu $s0, $s1, 0x38 */
    r_a0 = r_s0 + 0u; /* 800BAA18 addu $a0, $s0, $zero */
    r_s5 = r_s5 << 16; /* 800BAA1C sll $s5, $s5, 16 */
    r_a2 = BL_SRA(r_s5, 16); /* 800BAA20 sra $a2, $s5, 16 */
    r_s6 = r_s6 << 16; /* 800BAA24 sll $s6, $s6, 16 */
    r_a3 = BL_SRA(r_s6, 16); /* 800BAA28 sra $a3, $s6, 16 */
    r_s4 = r_s4 << 16; /* 800BAA2C sll $s4, $s4, 16 */
    r_s4 = BL_SRA(r_s4, 16); /* 800BAA30 sra $s4, $s4, 16 */
    r_s2 = r_s2 << 16; /* 800BAA34 sll $s2, $s2, 16 */
    r_s2 = BL_SRA(r_s2, 16); /* 800BAA38 sra $s2, $s2, 16 */
    r_a1 = bl_lw(r_sp + 0x58u); /* 800BAA3C lw $a1, 0x58($sp) */
    r_s3 = r_s3 << 16; /* 800BAA40 sll $s3, $s3, 16 */
    bl_sw(r_s1 + 0x4u, r_s0); /* 800BAA44 sw $s0, 0x4($s1) */
    bl_sw(r_s1 + 0x20u, r_s0); /* 800BAA48 sw $s0, 0x20($s1) */
    bl_sw(r_s1 + 0x1Cu, 0u); /* 800BAA4C sw $zero, 0x1C($s1) */
    r_t0 = bl_lw(r_sp + 0x8Cu); /* 800BAA50 lw $t0, 0x8C($sp) */
    r_s3 = BL_SRA(r_s3, 16); /* 800BAA54 sra $s3, $s3, 16 */
    bl_sw(r_sp + 0x10u, r_s4); /* 800BAA58 sw $s4, 0x10($sp) */
    bl_sw(r_sp + 0x14u, r_s2); /* 800BAA5C sw $s2, 0x14($sp) */
    bl_sw(r_sp + 0x18u, r_s3); /* 800BAA60 sw $s3, 0x18($sp) */
    /* 800BAA64 jal func_800242F4 */
    bl_sw(r_sp + 0x1Cu, r_t0);
    bl_jal(FN, 0x800242F4u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_800242F4 */
    r_a0 = r_s0 + 0u; /* 800BAA6C addu $a0, $s0, $zero */
    r_a1 = 0u + 0x2000u; /* 800BAA70 addiu $a1, $zero, 0x2000 */
    r_s7 = r_s7 << 16; /* 800BAA74 sll $s7, $s7, 16 */
    bl_sw(r_s0 + 0x6Cu, r_s1); /* 800BAA78 sw $s1, 0x6C($s0) */
    bl_sw(r_s1 + 0x38u, r_s7); /* 800BAA7C sw $s7, 0x38($s1) */
    r_t0 = bl_lw(r_sp + 0x20u); /* 800BAA80 lw $t0, 0x20($sp) */
    r_v1 = bl_lw(r_s0 + 0x3Cu); /* 800BAA84 lw $v1, 0x3C($s0) */
    r_v0 = r_t0 << 16; /* 800BAA88 sll $v0, $t0, 16 */
    bl_sw(r_s0 + 0x4u, r_v0); /* 800BAA8C sw $v0, 0x4($s0) */
    r_t0 = bl_lw(r_sp + 0x24u); /* 800BAA90 lw $t0, 0x24($sp) */
    r_v1 = r_v1 | 0x4u; /* 800BAA94 ori $v1, $v1, 0x4 */
    r_v0 = r_t0 << 16; /* 800BAA98 sll $v0, $t0, 16 */
    bl_sw(r_s0 + 0x8u, r_v0); /* 800BAA9C sw $v0, 0x8($s0) */
    r_v0 = r_fp + 0u; /* 800BAAA0 addu $v0, $fp, $zero */
    bl_sb(r_s0 + 0xB0u, r_v0); /* 800BAAA4 sb $v0, 0xB0($s0) */
    r_t0 = bl_lhu(r_sp + 0x28u); /* 800BAAA8 lhu $t0, 0x28($sp) */
    bl_sw(r_s0 + 0x3Cu, r_v1); /* 800BAAAC sw $v1, 0x3C($s0) */
    /* 800BAAB0 jal SpriteSetScale */
    bl_sh(r_s0 + 0x32u, r_t0);
    bl_jal(FN, 0x80022000u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* SpriteSetScale */
    r_a0 = r_s0 + 0u; /* 800BAAB8 addu $a0, $s0, $zero */
    r_v0 = r_fp << 16; /* 800BAABC sll $v0, $fp, 16 */
    r_a1 = BL_SRA(r_v0, 16); /* 800BAAC0 sra $a1, $v0, 16 */
    r_v0 = 0u + 0x2000u; /* 800BAAC4 addiu $v0, $zero, 0x2000 */
    bl_sh(r_a0 + 0x82u, r_v0); /* 800BAAC8 sh $v0, 0x82($a0) */
    /* 800BAACC jal func_800245D8 */
    bl_sw(r_a0 + 0x78u, 0u);
    bl_jal(FN, 0x800245D8u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_800245D8 */
    r_v0 = r_s1 + 0u; /* 800BAAD4 addu $v0, $s1, $zero */
    r_ra = bl_lw(r_sp + 0x54u); /* 800BAAD8 lw $ra, 0x54($sp) */
    r_fp = bl_lw(r_sp + 0x50u); /* 800BAADC lw $fp, 0x50($sp) */
    r_s7 = bl_lw(r_sp + 0x4Cu); /* 800BAAE0 lw $s7, 0x4C($sp) */
    r_s6 = bl_lw(r_sp + 0x48u); /* 800BAAE4 lw $s6, 0x48($sp) */
    r_s5 = bl_lw(r_sp + 0x44u); /* 800BAAE8 lw $s5, 0x44($sp) */
    r_s4 = bl_lw(r_sp + 0x40u); /* 800BAAEC lw $s4, 0x40($sp) */
    r_s3 = bl_lw(r_sp + 0x3Cu); /* 800BAAF0 lw $s3, 0x3C($sp) */
    r_s2 = bl_lw(r_sp + 0x38u); /* 800BAAF4 lw $s2, 0x38($sp) */
    r_s1 = bl_lw(r_sp + 0x34u); /* 800BAAF8 lw $s1, 0x34($sp) */
    r_s0 = bl_lw(r_sp + 0x30u); /* 800BAAFC lw $s0, 0x30($sp) */
    r_sp = r_sp + 0x58u; /* 800BAB00 addiu $sp, $sp, 0x58 */
    /* 800BAB04 jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    return r_v0;
}

/* func_800BAB0C.s (52 instructions; --args 1 --void), frontier step8: 800BA984's
 * work callback.  Callees through the jal service: RotTransPers, SetRotMatrix,
 * SetTransMatrix, func_8001E298. */
void func_800BAB0C(u32 arg0) {
    static const char FN[] = "func_800BAB0C";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x40u, &bl_saved);
    r_a0 = arg0;
    r_v0 = 0x800C0000u; /* 800BAB0C lui $v0, %hi(D_800C3664) */
    r_v0 = bl_lbu(r_v0 + 0x3664u); /* 800BAB10 lbu $v0, %lo(D_800C3664)($v0) */
    r_sp = r_sp + 0xFFFFFFC0u; /* 800BAB14 addiu $sp, $sp, -0x40 */
    bl_sw(r_sp + 0x38u, r_ra); /* 800BAB18 sw $ra, 0x38($sp) */
    bl_sw(r_sp + 0x34u, r_s1); /* 800BAB1C sw $s1, 0x34($sp) */
    /* 800BAB20 bnez $v0, .L800BABC4 */
    { int c_ = (r_v0 != 0); bl_sw(r_sp + 0x30u, r_s0); if (c_) goto L_800BABC4; }
    r_s1 = bl_lw(r_a0 + 0x4u); /* 800BAB28 lw $s1, 0x4($a0) */
    ; /* 800BAB2C nop  */
    r_v0 = bl_lh(r_s1 + 0x2u); /* 800BAB30 lh $v0, 0x2($s1) */
    r_s0 = 0x800D0000u; /* 800BAB34 lui $s0, %hi(D_800D30BC) */
    bl_sh(r_sp + 0x10u, r_v0); /* 800BAB38 sh $v0, 0x10($sp) */
    r_v0 = bl_lh(r_s1 + 0x6u); /* 800BAB3C lh $v0, 0x6($s1) */
    r_s0 = r_s0 + 0x30BCu; /* 800BAB40 addiu $s0, $s0, %lo(D_800D30BC) */
    bl_sh(r_sp + 0x12u, r_v0); /* 800BAB44 sh $v0, 0x12($sp) */
    r_v0 = bl_lh(r_s1 + 0xAu); /* 800BAB48 lh $v0, 0xA($s1) */
    r_a0 = r_s0 + 0u; /* 800BAB4C addu $a0, $s0, $zero */
    /* 800BAB50 jal SetRotMatrix */
    bl_sh(r_sp + 0x14u, r_v0);
    bl_jal(FN, 0x80049EFCu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* SetRotMatrix */
    /* 800BAB58 jal SetTransMatrix */
    r_a0 = r_s0 + 0u;
    bl_jal(FN, 0x80049F8Cu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* SetTransMatrix */
    r_a0 = r_sp + 0x10u; /* 800BAB60 addiu $a0, $sp, 0x10 */
    r_a1 = r_sp + 0x18u; /* 800BAB64 addiu $a1, $sp, 0x18 */
    r_a2 = r_a1 + 0u; /* 800BAB68 addu $a2, $a1, $zero */
    /* 800BAB6C jal RotTransPers */
    r_a3 = r_sp + 0x1Cu;
    bl_jal(FN, 0x8004A64Cu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* RotTransPers */
    r_v1 = 0x80050000u; /* 800BAB74 lui $v1, %hi(D_80050100) */
    r_v1 = bl_lw(r_v1 + 0x100u); /* 800BAB78 lw $v1, %lo(D_80050100)($v1) */
    r_a0 = bl_lh(r_s1 + 0x30u); /* 800BAB7C lh $a0, 0x30($s1) */
    r_v0 = BL_SRA(r_v0, r_v1); /* 800BAB80 srav $v0, $v0, $v1 */
    r_v1 = bl_lw(r_sp + 0x1Cu); /* 800BAB84 lw $v1, 0x1C($sp) */
    ; /* 800BAB88 nop  */
    r_v1 = r_v1 & 0x8000u; /* 800BAB8C andi $v1, $v1, 0x8000 */
    /* 800BAB90 beqz $v1, .L800BAB9C */
    { int c_ = (r_v1 == 0); r_a1 = r_v0 + r_a0; if (c_) goto L_800BAB9C; }
    r_a1 = 0u + 0u; /* 800BAB98 addu $a1, $zero, $zero */
L_800BAB9C:
    r_v0 = r_a1 + 0xFFFFFFFFu; /* 800BAB9C addiu $v0, $a1, -0x1 */
    r_v0 = (u32)(r_v0 < 0xFFFu); /* 800BABA0 sltiu $v0, $v0, 0xFFF */
    /* 800BABA4 beqz $v0, .L800BABC4 */
    { int c_ = (r_v0 == 0); bl_sh(r_s1 + 0x2Eu, r_a1); if (c_) goto L_800BABC4; }
    r_a0 = r_s1 + 0u; /* 800BABAC addu $a0, $s1, $zero */
    r_v0 = 0x80060000u; /* 800BABB0 lui $v0, %hi(g_GfxCurOT) */
    r_v0 = bl_lw(r_v0 + 0xFFFF956Cu); /* 800BABB4 lw $v0, %lo(g_GfxCurOT)($v0) */
    r_a1 = r_a1 << 2; /* 800BABB8 sll $a1, $a1, 2 */
    /* 800BABBC jal func_8001E298 */
    r_a1 = r_v0 + r_a1;
    bl_jal(FN, 0x8001E298u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_8001E298 */
L_800BABC4:
    r_ra = bl_lw(r_sp + 0x38u); /* 800BABC4 lw $ra, 0x38($sp) */
    r_s1 = bl_lw(r_sp + 0x34u); /* 800BABC8 lw $s1, 0x34($sp) */
    r_s0 = bl_lw(r_sp + 0x30u); /* 800BABCC lw $s0, 0x30($sp) */
    r_sp = r_sp + 0x40u; /* 800BABD0 addiu $sp, $sp, 0x40 */
    /* 800BABD4 jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    (void)r_v0;
}

/* func_800BABDC.s (29 instructions; --args 1 --void), frontier step8: 800BA984's
 * free callback.  Callees through the jal service: HeapFree, func_8001CE74,
 * func_8001D3F4, WorkListRemoveTask, TimerWorkListRemoveTask. */
void func_800BABDC(u32 arg0) {
    static const char FN[] = "func_800BABDC";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x20u, &bl_saved);
    r_a0 = arg0;
    r_sp = r_sp + 0xFFFFFFE0u; /* 800BABDC addiu $sp, $sp, -0x20 */
    bl_sw(r_sp + 0x14u, r_s1); /* 800BABE0 sw $s1, 0x14($sp) */
    r_s1 = r_a0 + 0x38u; /* 800BABE4 addiu $s1, $a0, 0x38 */
    bl_sw(r_sp + 0x18u, r_ra); /* 800BABE8 sw $ra, 0x18($sp) */
    bl_sw(r_sp + 0x10u, r_s0); /* 800BABEC sw $s0, 0x10($sp) */
    r_v0 = bl_lw(r_s1 + 0x20u); /* 800BABF0 lw $v0, 0x20($s1) */
    ; /* 800BABF4 nop  */
    r_v0 = bl_lw(r_v0 + 0x2Cu); /* 800BABF8 lw $v0, 0x2C($v0) */
    ; /* 800BABFC nop  */
    /* 800BAC00 beqz $v0, .L800BAC10 */
    { int c_ = (r_v0 == 0); r_s0 = r_a0 + 0u; if (c_) goto L_800BAC10; }
    /* 800BAC08 jal HeapFree */
    r_a0 = r_v0 + 0u;
    bl_jal(FN, 0x800320E8u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* HeapFree */
L_800BAC10:
    /* 800BAC10 jal func_8001CE74 */
    r_a0 = r_s0 + 0u;
    bl_jal(FN, 0x8001CE74u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_8001CE74 */
    /* 800BAC18 jal func_8001D3F4 */
    r_a0 = r_s1 + 0u;
    bl_jal(FN, 0x8001D3F4u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_8001D3F4 */
    /* 800BAC20 jal WorkListRemoveTask */
    r_a0 = r_s0 + 0x1Cu;
    bl_jal(FN, 0x8001CB48u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* WorkListRemoveTask */
    /* 800BAC28 jal TimerWorkListRemoveTask */
    r_a0 = r_s0 + 0u;
    bl_jal(FN, 0x8001CD94u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* TimerWorkListRemoveTask */
    /* 800BAC30 jal HeapFree */
    r_a0 = r_s0 + 0u;
    bl_jal(FN, 0x800320E8u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* HeapFree */
    r_ra = bl_lw(r_sp + 0x18u); /* 800BAC38 lw $ra, 0x18($sp) */
    r_s1 = bl_lw(r_sp + 0x14u); /* 800BAC3C lw $s1, 0x14($sp) */
    r_s0 = bl_lw(r_sp + 0x10u); /* 800BAC40 lw $s0, 0x10($sp) */
    r_sp = r_sp + 0x20u; /* 800BAC44 addiu $sp, $sp, 0x20 */
    /* 800BAC48 jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    (void)r_v0;
}
#endif /* XENO_PC_PORT h-frontier */
