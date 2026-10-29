#include "common.h"


#ifndef XENO_PC_PORT
/* func_800B3B94.s, byte-exact: the timer callback of func_800B3CD4's
 * task.  While the +0x3C countdown runs, +0x46 moves from +0x40 towards
 * +0x42: n = +0x3C - 1 (stored back), f = (n << 5) / +0x38, and
 * +0x46 = +0x42 - (+0x42 - +0x40) * f / 32.  Once the countdown is 0 and
 * +0x46 is 0, it calls the task's +0xC function with the task.
 * Codegen (counted natural, flagged):
 * - x carries the whole chain (the +0x40 value, the product, the quotient,
 *   the result).  That gives retail's rounding copy, which reads the
 *   product, and retail's registers.  The +0x40 read must stay its own
 *   statement: folded into the product the registers come out mirrored
 *   (v0/v1, 14/38).  With separate expressions the copy is coalesced (one
 *   insn short).
 * - p is a second name for t, used only in the else branch; it gives
 *   retail's a1 (then) / a0 (else) split (t alone is one insn short). */
void func_800B3B94(u8* t) {
    u8* p = t;
    s32 n;
    s32 f;
    s32 a;
    s32 x;

    if (*(s32*)(t + 0x3C) != 0) {
        n = *(s32*)(t + 0x3C) - 1;
        f = (n << 5) / *(s32*)(t + 0x38);
        a = *(s16*)(t + 0x42);
        *(s32*)(t + 0x3C) = n;
        x = *(s16*)(t + 0x40);
        x = (a - x) * f;
        x /= 32;
        x = a - x;
        *(s16*)(t + 0x46) = x;
    } else if (*(s16*)(p + 0x46) == 0) {
        (*(void (**)(u8*))(p + 0xC))(p);
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
extern void func_800245D8(u32 a0, u32 a1);
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


/* func_800B3C2C.s */
#ifndef XENO_PC_PORT
extern u32 D_800C3560[];
#endif
extern void WorkListRemoveTask(u32 p);
extern void TimerWorkListRemoveTask(u32 p);
extern void HeapFree(u32 p);
extern void func_800A6F98(void);
void func_800B3C2C(u8* p) {
    WorkListRemoveTask((u32)(p + 0x1C));
    TimerWorkListRemoveTask((u32)p);
    HeapFree((u32)p);
    D_800C3560[0] = 0;
    func_800A6F98();
}

#ifdef XENO_PC_PORT
/* h-lift: instruction-faithful port bodies of functions the battle still
 * interpreted, generated from the retail listings by
 * tools/scripts/battle_lift_asm.py --jal-service (one statement per MIPS
 * instruction; include/battle_lift.h has the register, memory, frame and
 * call contract; every call goes through the interpreter's own jal path)
 * and committed as generated.  The matched C above stays the matching
 * build's.  Regenerate with
 *   tools/scripts/battle_lift_asm.py asm/battle/matchings/<tu>/<fn>.s --args N [--void] --jal-service
 */
#include "battle_lift.h"

/* func_800B3B94.s (38 instructions, frame 0x18; lifted with --args 1 --void).
 * Callees through the jal service: none; 1 indirect call(s) (jalr) through the same path. */
void func_800B3B94(u32 arg0) {
    static const char FN[] = "func_800B3B94";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x18u, &bl_saved);
    r_a0 = arg0;
    r_sp = r_sp + 0xFFFFFFE8u; /* 800B3B94 addiu $sp, $sp, -0x18 */
    r_a1 = r_a0 + 0u; /* 800B3B98 addu $a1, $a0, $zero */
    bl_sw(r_sp + 0x10u, r_ra); /* 800B3B9C sw $ra, 0x10($sp) */
    r_v0 = bl_lw(r_a1 + 0x3Cu); /* 800B3BA0 lw $v0, 0x3C($a1) */
    ; /* 800B3BA4 nop  */
    /* 800B3BA8 beqz $v0, .L800B3BFC */
    { int c_ = (r_v0 == 0); ; if (c_) goto L_800B3BFC; }
    r_a0 = r_v0 + 0xFFFFFFFFu; /* 800B3BB0 addiu $a0, $v0, -0x1 */
    r_v1 = bl_lw(r_a1 + 0x38u); /* 800B3BB4 lw $v1, 0x38($a1) */
    r_v0 = r_a0 << 5; /* 800B3BB8 sll $v0, $a0, 5 */
    BL_DIV(r_v0, r_v1); /* 800B3BBC div $zero, $v0, $v1 */
    r_v0 = lo; /* 800B3BC0 mflo $v0 */
    r_a2 = bl_lh(r_a1 + 0x42u); /* 800B3BC4 lh $a2, 0x42($a1) */
    r_v1 = bl_lh(r_a1 + 0x40u); /* 800B3BC8 lh $v1, 0x40($a1) */
    ; /* 800B3BCC nop  */
    r_v1 = r_a2 - r_v1; /* 800B3BD0 subu $v1, $a2, $v1 */
    BL_MULT(r_v1, r_v0); /* 800B3BD4 mult $v1, $v0 */
    bl_sw(r_a1 + 0x3Cu, r_a0); /* 800B3BD8 sw $a0, 0x3C($a1) */
    r_v1 = lo; /* 800B3BDC mflo $v1 */
    /* 800B3BE0 bgez $v1, .L800B3BEC */
    { int c_ = ((s32)r_v1 >= 0); r_v0 = r_v1 + 0u; if (c_) goto L_800B3BEC; }
    r_v0 = r_v1 + 0x1Fu; /* 800B3BE8 addiu $v0, $v1, 0x1F */
L_800B3BEC:
    r_v1 = BL_SRA(r_v0, 5); /* 800B3BEC sra $v1, $v0, 5 */
    r_v1 = r_a2 - r_v1; /* 800B3BF0 subu $v1, $a2, $v1 */
    /* 800B3BF4 j .L800B3C1C */
    bl_sh(r_a1 + 0x46u, r_v1); goto L_800B3C1C;
L_800B3BFC:
    r_v0 = bl_lh(r_a0 + 0x46u); /* 800B3BFC lh $v0, 0x46($a0) */
    ; /* 800B3C00 nop  */
    /* 800B3C04 bnez $v0, .L800B3C1C */
    { int c_ = (r_v0 != 0); ; if (c_) goto L_800B3C1C; }
    r_v0 = bl_lw(r_a0 + 0xCu); /* 800B3C0C lw $v0, 0xC($a0) */
    ; /* 800B3C10 nop  */
    /* 800B3C14 jalr $v0 */
    { u32 t_ = r_v0; ; bl_jal(FN, t_, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); } /* indirect call */
L_800B3C1C:
    r_ra = bl_lw(r_sp + 0x10u); /* 800B3C1C lw $ra, 0x10($sp) */
    r_sp = r_sp + 0x18u; /* 800B3C20 addiu $sp, $sp, 0x18 */
    /* 800B3C24 jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    (void)r_v0;
}
#endif /* XENO_PC_PORT */
