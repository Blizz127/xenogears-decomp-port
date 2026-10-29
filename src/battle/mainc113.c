#include "common.h"


#ifndef XENO_PC_PORT
extern s32 ArchiveDataSync(void);
extern void* HeapAlloc(u32 size, u32 flags);
extern u32 HeapFree(void* p);
extern void func_800BB690(u8* task);
extern u8 D_800C3CB8[];

/* once the archive is idle and D_800C3CB8 is clear, run func_800BB690(task)
 * on a HeapAlloc(0x1000, 1) stack (caller $sp saved at +0xF00). */
void func_800BB6E0(u8* task) {
    u8* stack;

    if (ArchiveDataSync() == 0 && D_800C3CB8[0] == 0) {
        stack = HeapAlloc(0x1000, 1);
        __asm__ volatile(
            "\taddu  $t0, %0, $zero\n"
            "\tsw    $sp, 0($t0)\n"
            "\taddiu $t0, $t0, -4\n"
            "\taddu  $sp, $t0, $zero\n"
            :
            : "r"(stack + 0xF00)
            : "t0", "memory");
        func_800BB690(task);
        __asm__ volatile(
            "\taddiu $sp, $sp, 4\n"
            "\tlw    $sp, 0($sp)\n"
            :
            :
            : "memory");
        HeapFree(stack);
    }
}

extern u8* TimerWorkListAllocateTask(u32 owner, u32 size);
extern void TimerWorkListSetTaskCallback(void* pTask, void* callback);
extern void func_800BB6E0(u8* task);
extern u8 D_800591AC[];
extern u8 D_800591AF[];
extern s32 D_800C35D8[];

/* queue a 4-byte timer task (callback func_800BB6E0) for `slot` with
 * D_800591AC cleared and D_800591AF set around the allocation, and count it
 * in D_800C35D8. */
void func_800BB760(u32 slot) {
    u8 saved = D_800591AC[0];
    u8* task;
    s32 count;

    D_800591AC[0] = 0;
    D_800591AF[0] = 1;
    task = TimerWorkListAllocateTask(0, 4);
    TimerWorkListSetTaskCallback(task, func_800BB6E0);
    count = D_800C35D8[0];
    *(u32*)(task + 0x1C) = slot;
    D_800591AF[0] = 0;
    D_800C35D8[0] = count + 1;
    D_800591AC[0] = saved;
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


/* func_800BB7F8.s */
#ifndef XENO_PC_PORT
extern u32 D_800C3674[];
#endif
#ifndef XENO_PC_PORT
extern u32 D_800C3678[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3CC4[];
#endif
#ifndef XENO_PC_PORT
extern u32 D_800C3CBC[];
#endif
extern void func_800BC2F0(u32 v);
void func_800BB7F8(void) {
    D_800C3674[0] = 0x200;
    D_800C3678[0] = 0xFFFFFFFF;
    D_800C3CC4[0] = 0;
    D_800C3CBC[0] = 1;
    func_800BC2F0(0);
}

#ifdef XENO_PC_PORT
#include "battle_host_guest.h"
/* Native owners: src/slus_006.64/system/{memory,libarchive}.c, sdata. */
extern void* HeapAlloc(u_int allocSize, u_int allocFlags);
extern u_int HeapFree(void* pMem);
extern s32 ArchiveDataSync(void);
extern u8 D_800591AC;
extern u8 D_800591AF;
extern void func_800BB690(u32 task);

/* func_800BB6E0.s: timer callback: once the archive is idle and the
 * D_800C3CB8 counter is 0, run func_800BB690(task) on a private 0x1000-byte
 * heap stack (retail saves $sp at block + 0xF00 and switches to it; the
 * native body just calls through) and free it. */
#ifndef XENO_PC_PORT /* port: lifted body in the h-frontier block */
void func_800BB6E0(u8* task) {
    if (ArchiveDataSync() == 0 && BG_U8(0x800C3CB8u) == 0) { /* D_800C3CB8 */
        void* stack = HeapAlloc(0x1000, 1);

        func_800BB690(task);
        HeapFree(stack);
    }
}
#endif

/* func_800BB760.s: queue a 4-byte timer task (callback func_800BB6E0) that
 * carries `slot` at +0x1C, with the heap flags D_800591AC forced to 0 and
 * D_800591AF to 1 around the allocation, and bump D_800C35D8. */
#ifndef XENO_PC_PORT /* port: lifted body in the h-frontier block */
void func_800BB760(u32 slot) {
    u8 saved = D_800591AC;
    u8* task;
    u32 pending;

    D_800591AC = 0;
    D_800591AF = 1;
    task = TimerWorkListAllocateTask(0, 4);
    TimerWorkListSetTaskCallback(task, (void*)(uintptr_t)0x800BB6E0u);
    pending = BG_U32(0x800C35D8u); /* D_800C35D8 */
    *(u32*)(task + 0x1C) = slot;
    D_800591AF = 0;
    BG_U32(0x800C35D8u) = pending + 1;
    D_800591AC = saved;
}
#endif
#endif

#ifdef XENO_PC_PORT /* h-frontier lifted bodies */
/* Instruction-faithful port bodies (native-only frontier), generated by
 *   tools/scripts/battle_lift_asm.py asm/battle/matchings/<tu>/<fn>.s --args N [--void] --jal-service
 * (include/battle_lift.h; every call goes through the interpreter's own
 * jal path, so each callee gets the binding an interpreted jal gets).  The
 * matched C above stays the matching build's. */
#define BL_NO_GTE /* no GTE call here */
#include "battle_lift.h"
BL_EXPORT_FRAME_TOP(mainc113)

/* func_800BB760.s (38 instructions; --args 1 --void), frontier step10a: the
 * slot task 801E6A4C queues.  Callees through the jal service:
 * TimerWorkListAllocateTask, TimerWorkListSetTaskCallback. */
void func_800BB760(u32 arg0) {
    static const char FN[] = "func_800BB760";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x28u, &bl_saved);
    r_a0 = arg0;
    r_sp = r_sp + 0xFFFFFFD8u; /* 800BB760 addiu $sp, $sp, -0x28 */
    bl_sw(r_sp + 0x1Cu, r_s3); /* 800BB764 sw $s3, 0x1C($sp) */
    r_s3 = r_a0 + 0u; /* 800BB768 addu $s3, $a0, $zero */
    r_a0 = 0u + 0u; /* 800BB76C addu $a0, $zero, $zero */
    bl_sw(r_sp + 0x14u, r_s1); /* 800BB770 sw $s1, 0x14($sp) */
    r_s1 = 0x80060000u; /* 800BB774 lui $s1, %hi(D_800591AC) */
    r_a1 = 0u + 0x4u; /* 800BB778 addiu $a1, $zero, 0x4 */
    bl_sw(r_sp + 0x18u, r_s2); /* 800BB77C sw $s2, 0x18($sp) */
    r_s2 = 0x80060000u; /* 800BB780 lui $s2, %hi(D_800591AF) */
    bl_sw(r_sp + 0x20u, r_s4); /* 800BB784 sw $s4, 0x20($sp) */
    r_s4 = bl_lbu(r_s1 + 0xFFFF91ACu); /* 800BB788 lbu $s4, %lo(D_800591AC)($s1) */
    r_v0 = 0u + 0x1u; /* 800BB78C addiu $v0, $zero, 0x1 */
    bl_sw(r_sp + 0x24u, r_ra); /* 800BB790 sw $ra, 0x24($sp) */
    bl_sw(r_sp + 0x10u, r_s0); /* 800BB794 sw $s0, 0x10($sp) */
    bl_sb(r_s1 + 0xFFFF91ACu, 0u); /* 800BB798 sb $zero, %lo(D_800591AC)($s1) */
    /* 800BB79C jal TimerWorkListAllocateTask */
    bl_sb(r_s2 + 0xFFFF91AFu, r_v0);
    bl_jal(FN, 0x8001CD08u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* TimerWorkListAllocateTask */
    r_s0 = r_v0 + 0u; /* 800BB7A4 addu $s0, $v0, $zero */
    r_a0 = r_s0 + 0u; /* 800BB7A8 addu $a0, $s0, $zero */
    r_a1 = 0x800C0000u; /* 800BB7AC lui $a1, %hi(func_800BB6E0) */
    /* 800BB7B0 jal TimerWorkListSetTaskCallback */
    r_a1 = r_a1 + 0xFFFFB6E0u;
    bl_jal(FN, 0x8001CD6Cu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* TimerWorkListSetTaskCallback */
    r_v1 = 0x800C0000u; /* 800BB7B8 lui $v1, %hi(D_800C35D8) */
    r_v0 = bl_lw(r_v1 + 0x35D8u); /* 800BB7BC lw $v0, %lo(D_800C35D8)($v1) */
    bl_sw(r_s0 + 0x1Cu, r_s3); /* 800BB7C0 sw $s3, 0x1C($s0) */
    bl_sb(r_s2 + 0xFFFF91AFu, 0u); /* 800BB7C4 sb $zero, %lo(D_800591AF)($s2) */
    r_v0 = r_v0 + 0x1u; /* 800BB7C8 addiu $v0, $v0, 0x1 */
    bl_sw(r_v1 + 0x35D8u, r_v0); /* 800BB7CC sw $v0, %lo(D_800C35D8)($v1) */
    bl_sb(r_s1 + 0xFFFF91ACu, r_s4); /* 800BB7D0 sb $s4, %lo(D_800591AC)($s1) */
    r_ra = bl_lw(r_sp + 0x24u); /* 800BB7D4 lw $ra, 0x24($sp) */
    r_s4 = bl_lw(r_sp + 0x20u); /* 800BB7D8 lw $s4, 0x20($sp) */
    r_s3 = bl_lw(r_sp + 0x1Cu); /* 800BB7DC lw $s3, 0x1C($sp) */
    r_s2 = bl_lw(r_sp + 0x18u); /* 800BB7E0 lw $s2, 0x18($sp) */
    r_s1 = bl_lw(r_sp + 0x14u); /* 800BB7E4 lw $s1, 0x14($sp) */
    r_s0 = bl_lw(r_sp + 0x10u); /* 800BB7E8 lw $s0, 0x10($sp) */
    r_sp = r_sp + 0x28u; /* 800BB7EC addiu $sp, $sp, 0x28 */
    /* 800BB7F0 jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    (void)r_v0;
}

/* func_800BB6E0.s (32 instructions; --args 1 --void), frontier step11.  Callees through the jal service: ArchiveDataSync, HeapAlloc, HeapFree, func_800BB690. */
void func_800BB6E0(u32 arg0) {
    static const char FN[] = "func_800BB6E0";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x20u, &bl_saved);
    r_a0 = arg0;
    r_sp = r_sp + 0xFFFFFFE0u; /* 800BB6E0 addiu $sp, $sp, -0x20 */
    bl_sw(r_sp + 0x14u, r_s1); /* 800BB6E4 sw $s1, 0x14($sp) */
    r_s1 = r_a0 + 0u; /* 800BB6E8 addu $s1, $a0, $zero */
    bl_sw(r_sp + 0x18u, r_ra); /* 800BB6EC sw $ra, 0x18($sp) */
    /* 800BB6F0 jal ArchiveDataSync */
    bl_sw(r_sp + 0x10u, r_s0);
    bl_jal(FN, 0x800286CCu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* ArchiveDataSync */
    /* 800BB6F8 bnez $v0, .L800BB748 */
    { int c_ = (r_v0 != 0); r_v0 = 0x800C0000u; if (c_) goto L_800BB748; }
    r_v0 = bl_lbu(r_v0 + 0x3CB8u); /* 800BB700 lbu $v0, %lo(D_800C3CB8)($v0) */
    ; /* 800BB704 nop  */
    /* 800BB708 bnez $v0, .L800BB748 */
    { int c_ = (r_v0 != 0); r_a0 = 0u + 0x1000u; if (c_) goto L_800BB748; }
    /* 800BB710 jal HeapAlloc */
    r_a1 = 0u + 0x1u;
    bl_jal(FN, 0x80031BDCu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* HeapAlloc */
    r_s0 = r_v0 + 0u; /* 800BB718 addu $s0, $v0, $zero */
    r_v0 = r_s0 + 0xF00u; /* 800BB71C addiu $v0, $s0, 0xF00 */
    r_t0 = r_v0 + 0u; /* 800BB720 addu $t0, $v0, $zero */
    bl_sw(r_t0, r_sp); /* 800BB724 sw $sp, 0x0($t0) */
    r_t0 = r_t0 + 0xFFFFFFFCu; /* 800BB728 addiu $t0, $t0, -0x4 */
    r_sp = r_t0 + 0u; /* 800BB72C addu $sp, $t0, $zero */
    /* 800BB730 jal func_800BB690 */
    r_a0 = r_s1 + 0u;
    bl_jal(FN, 0x800BB690u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_800BB690 */
    r_sp = r_sp + 0x4u; /* 800BB738 addiu $sp, $sp, 0x4 */
    r_sp = bl_lw(r_sp); /* 800BB73C lw $sp, 0x0($sp) */
    /* 800BB740 jal HeapFree */
    r_a0 = r_s0 + 0u;
    bl_jal(FN, 0x800320E8u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* HeapFree */
L_800BB748:
    r_ra = bl_lw(r_sp + 0x18u); /* 800BB748 lw $ra, 0x18($sp) */
    r_s1 = bl_lw(r_sp + 0x14u); /* 800BB74C lw $s1, 0x14($sp) */
    r_s0 = bl_lw(r_sp + 0x10u); /* 800BB750 lw $s0, 0x10($sp) */
    r_sp = r_sp + 0x20u; /* 800BB754 addiu $sp, $sp, 0x20 */
    /* 800BB758 jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    (void)r_v0;
}
#endif /* XENO_PC_PORT h-frontier */
