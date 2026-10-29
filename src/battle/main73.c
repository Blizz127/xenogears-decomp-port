#include "common.h"
/* Retail TU split: gcc 2.7.2 cc1 emits every file-scope INCLUDE_ASM block
 * before the first compiled body, so a mixed TU whose retail layout has a C
 * function ahead of an assembly function cannot be one object.  Each
 * contiguous [asm run][C run] of the retail layout is therefore its own
 * matching TU: src/battle/main73_p<N>.c defines BATTLE_TU_PART=<N> and
 * includes this file.  Host/port builds compile this file whole (part 0). */
#ifndef BATTLE_TU_PART
#define BATTLE_TU_PART 0
#endif
#define BATTLE_PART(n) (BATTLE_TU_PART == 0 || BATTLE_TU_PART == (n))

#ifndef XENO_PC_PORT
extern u16 D_800C3E30;
#endif


#if BATTLE_PART(1)
/* func_800AF400.s: index of the lowest set bit in D_800C3E30
 * (13 when bits 0-12 are all clear). */
/* The retail build compiles this function's byte-exact C in
 * mainasm_3F910.c.  The body below is kept for the port and the
 * differential harnesses. */
#ifdef XENO_PC_PORT
s32 func_800AF400(void) {
    s32 bit = 0;

    while (((D_800C3E30 >> bit) & 1) == 0) {
        if (++bit == 13) {
            break;
        }
    }
    return bit;
}
#endif /* XENO_PC_PORT */
#ifdef XENO_PC_PORT
#include "battle_host_guest.h"
/* func_800AF270.s: for each of the (+0xA) - 1 0x7C-byte entries after the
 * first, move a pending flag (+0x7 of src entry i) to +0x7 of dst entry i. */
void func_800AF270(u8* src, u8* dst) {
    u32 s = BG_ADDR(src);
    u32 d = BG_ADDR(dst);
    s32 count = BG_U16(s + 0xA);
    s32 i;

    for (i = 1; i < count; i++) {
        u32 flag = s + 7 + (u32)i * 0x7C;

        if (BG_U8(flag) != 0) {
            BG_U8(flag) = 0;
            BG_U8(d + (u32)i * 0x7C + 7) = 1;
        }
    }
}
#endif /* XENO_PC_PORT */
#ifdef XENO_PC_PORT
#include "battle_host_guest.h"
/* Native psyq (PsyCross psx/libgte.h): int SquareRoot0(int); VECTOR is four ints. */
extern int SquareRoot0(int a);
extern void OuterProduct12(void* v0, void* v1, void* v2);

/* func_800AEEF8.s: distance from the halfword point at +0x88/+0x8A/+0x8C to
 * the word position at (+0x4)->+0x5C/+0x60/+0x64 (SUBU/MULT wrap). */
s32 func_800AEEF8(u8* obj) {
    u32 pos = *(u32*)(obj + 4);
    u32 dx = (u32)(s32)*(s16*)(obj + 0x88) - BG_U32(pos + 0x5C);
    u32 dy = (u32)(s32)*(s16*)(obj + 0x8A) - BG_U32(pos + 0x60);
    u32 dz = (u32)(s32)*(s16*)(obj + 0x8C) - BG_U32(pos + 0x64);

    return SquareRoot0((s32)(dx * dx + dy * dy + dz * dz));
}

/* func_800AF180.s: move the pending flag (+0x7) of 0x7C-byte entry `index`
 * from src to dst, release its three +0x70/+0x74/+0x78 handles through
 * func_800A23E8 (clearing each word after its call), then recurse into
 * every later src entry whose +0x0 parent word is this entry's address. */
extern void func_800A23E8(u8* a0, u32 p);
void func_800AF180(u8* ctx, u32 index, u8* src, u8* dst) {
    u8* e = src + index * 0x7C;
    u32 self = BG_ADDR(src) + index * 0x7C;
    s32 count = *(u16*)(src + 0xA);
    u8* child;
    s32 i;

    e[7] = 0;
    dst[index * 0x7C + 7] = 1;
    func_800A23E8(ctx, *(u32*)(e + 0x70));
    *(u32*)(e + 0x70) = 0;
    func_800A23E8(ctx, *(u32*)(e + 0x74));
    *(u32*)(e + 0x74) = 0;
    func_800A23E8(ctx, *(u32*)(e + 0x78));
    *(u32*)(e + 0x78) = 0;
    for (i = 1, child = src + 0x7C; i < count; i++, child += 0x7C) {
        if (*(u32*)child == self) {
            func_800AF180(ctx, *(u16*)(child + 0xA), src, dst);
        }
    }
}

/* func_800AF2C4.s: n = OuterProduct12(a, b); return the s16 of
 * ((dot(vec, n) << 4) / (|n| + 1) << 8) / divisor (signed DIVs). */
s32 func_800AF2C4(u8* vec, u8* a, u8* b, s32 divisor) {
    s32 n[4];
    u32 dot;
    u32 len;
    s32 q;

    OuterProduct12(a, b, n);
    dot = (u32)n[0] * *(u32*)(vec + 0) + (u32)n[1] * *(u32*)(vec + 4) +
          (u32)n[2] * *(u32*)(vec + 8);
    len = (u32)n[0] * (u32)n[0] + (u32)n[1] * (u32)n[1] + (u32)n[2] * (u32)n[2];
    q = bg_div((s32)(dot << 4), (s32)((u32)SquareRoot0((s32)len) + 1));
    q = bg_div((s32)((u32)q << 8), divisor);
    return (s16)q;
}
#endif /* XENO_PC_PORT */
#endif /* BATTLE_PART(1) */
#if BATTLE_PART(2)
#ifdef XENO_PC_PORT
#include "battle_host_guest.h"
/* func_800AF438.s: *out = 1 << n (SLLV, low five bits of n & 0xFF) where n
 * decodes the target code `code`: 0xFF -> func_800AF400(), 0xFE ->
 * D_800C3D40, 0xFD/0xF9 -> +0x20, 0xFC -> +0x21, 0xFA -> 31, 0xF8 ->
 * +0x20 * 2 + 13, 0xF7 -> +0x20 * 2 + 14, anything else -> code itself. */
void func_800AF438(u8* obj, u32 code, u16* out) {
    u32 c = code & 0xFF;
    u32 n;

    if (c == 0xFF) {
        n = (u32)func_800AF400();
    } else if (c == 0xFE) {
        n = BG_U8(0x800C3D40); /* D_800C3D40 */
    } else if (c == 0xFD || c == 0xF9) {
        n = obj[0x20];
    } else if (c == 0xFC) {
        n = obj[0x21];
    } else if (c == 0xFA) {
        n = 0x1F;
    } else if (c == 0xF8) {
        n = (u32)obj[0x20] * 2 + 0xD;
    } else if (c == 0xF7) {
        n = (u32)obj[0x20] * 2 + 0xE;
    } else {
        n = code;
    }
    *out = (u16)(1u << ((n & 0xFF) & 31));
}

/* func_800AF518.s: resolve slot `code` of the actor's +0x14/+0x18 pointer
 * tables.  0xFF first refreshes the +0x2A slot byte from func_8009C050's
 * flags and the +0x4A status bits (D_800CCD1E: 0x170-byte records); 0xFE
 * and 0xFF then take the slot from +0x2A (bit 7 goes to *flag).  Slots
 * below 0x40 read (+0x14)[slot + 1], others (+0x18)[slot - 0x3F]. */
extern u8 func_8009C050(u8 idx);
u32 func_800AF518(u8* obj, u32 code, u32* flag) {
    u32 c = code & 0xFF;
    u32 slot = code;

    *flag = 0;
    if (c >= 0xFE) {
        if (c == 0xFF) {
            u32 bits = func_8009C050(obj[0x20]);
            s32 value = -1;

            if ((bits & 4) != 0 && (*(u16*)(obj + 0x4A) & 0x100) != 0) {
                value = 0x1B;
            } else if ((bits & 2) != 0 && (*(u16*)(obj + 0x4A) & 0x80) != 0) {
                /* D_800CCD1E */
                if ((BG_U16(0x800CCD1E + (u32)obj[0x20] * 0x170) & 1) == 0) {
                    value = 6;
                }
            } else if ((*(u16*)(obj + 0x4A) & 0x400) == 0) {
                value = 1;
            }
            if (value >= 0) {
                obj[0x2A] = (u8)value;
            }
            if ((bits & 1) != 0) {
                obj[0x2A] = obj[0x2A] | 0x80;
            }
        }
        slot = obj[0x2A] & 0x7F;
        *flag = obj[0x2A] & 0x80;
    }
    slot &= 0xFF;
    if (slot < 0x40) {
        return BG_U32(slot * 4 + *(u32*)(obj + 0x14) + 4);
    }
    return BG_U32(slot * 4 + *(u32*)(obj + 0x18) - 0xFC);
}

/* func_800AFA98.s: set node->+0x7 to bit 0 of `flags`; with bit 7 set,
 * recurse into every later 0x7C-byte entry of (ctx+0x4)'s table whose +0x0
 * parent word is `node` (the count at table+0xA is reloaded each pass). */
void func_800AFA98(u8* ctx, u8* node, u32 flags) {
    u32 self = BG_ADDR(node);
    u32 table;
    u32 e;
    s32 i;

    node[7] = (u8)(flags & 1);
    if ((flags & 0x80) == 0) {
        return;
    }
    table = *(u32*)(ctx + 4);
    for (i = 1, e = table + 0x7C; i < (s32)BG_U16(*(u32*)(ctx + 4) + 0xA);
         i++, e += 0x7C) {
        if (BG_U32(e) == self) {
            func_800AFA98(ctx, BG_PTR(e), flags);
        }
    }
}

/* func_800AFB4C.s: build a sprite task (func_80023FD8 with 0x18 extra
 * bytes), set its direction (both setters) and scale, and fill the extra
 * block at task + (s16)task[0xBE]: +0x8 owner, +0xC info[5]; when info[0x13]
 * is set, chain the task's callback into +0x4, install func_800AFC68 and copy
 * info +0x6/+0x8/+0xA halfwords and info[0xC]. */
extern u8* func_80023FD8(s32 index, u8* package, s16* position, s32 extra);
extern void func_80021FE0(void* sprite, s16 direction);
extern void func_800223B0(void* sprite, s16 direction);
extern void SpriteSetScale(void* sprite, short scale);
extern void* WorkListTaskGetTaskCallback(void* task);
extern void TimerWorkListSetTaskCallback(void* task, void* callback);
void func_800AFB4C(u8* package, s32 index, s16* position, s32 direction,
                   s32 scale, u8* info, u8* owner) {
    u32 task;
    u32 ext;

    task = (u32)(uintptr_t)func_80023FD8(index, package, position, 0x18);
    func_80021FE0(BG_PTR(task + 0x38), (s16)direction);
    func_800223B0(BG_PTR(task + 0x38), (s16)direction);
    SpriteSetScale(BG_PTR(task + 0x38), (s16)scale);
    ext = task + (u32)(s32)BG_S16(task + 0xBE);
    BG_U32(ext + 8) = BG_ADDR(owner);
    BG_U16(ext + 0xC) = info[5];
    if (info[0x13] != 0) {
        BG_U32(ext + 4) = (u32)(uintptr_t)WorkListTaskGetTaskCallback(BG_PTR(task));
        TimerWorkListSetTaskCallback(BG_PTR(task), (void*)(uintptr_t)0x800AFC68u);
        BG_U16(ext + 0x10) = *(u16*)(info + 6);
        BG_U16(ext + 0x12) = *(u16*)(info + 8);
        BG_U16(ext + 0x14) = *(u16*)(info + 0xA);
        BG_U16(ext + 0xE) = info[0xC];
    }
}

/* func_800AFD98.s: set (flags & 0x20: add to) one of node's three xyz
 * triples chosen by flags & 7: 0 -> halfwords +0x54, 1 -> (s16) words +0x5C,
 * else halfwords +0x4C; mark +0x4/+0x5 dirty; with bit 7 recurse into the
 * children (entries of (ctx+0x4)'s table whose +0x0 parent is node). */
void func_800AFD98(u8* ctx, u8* node, u32 flags, u32 x, u32 y, u32 z) {
    u32 self = BG_ADDR(node);
    u32 e;
    s32 i;

    if ((flags & 7) == 0) {
        if ((flags & 0x20) != 0) {
            u32 a = *(u16*)(node + 0x54);
            u32 b = *(u16*)(node + 0x58);

            *(u16*)(node + 0x54) = (u16)(x + a);
            a = *(u16*)(node + 0x56);
            *(u16*)(node + 0x58) = (u16)(z + b);
            *(u16*)(node + 0x56) = (u16)(y + a);
        } else {
            *(u16*)(node + 0x54) = (u16)x;
            *(u16*)(node + 0x56) = (u16)y;
            *(u16*)(node + 0x58) = (u16)z;
        }
    } else if ((flags & 7) == 1) {
        if ((flags & 0x20) != 0) {
            *(u32*)(node + 0x5C) = (u32)(s32)(s16)x + *(u32*)(node + 0x5C);
            *(u32*)(node + 0x60) = (u32)(s32)(s16)y + *(u32*)(node + 0x60);
            *(u32*)(node + 0x64) = (u32)(s32)(s16)z + *(u32*)(node + 0x64);
        } else {
            *(u32*)(node + 0x5C) = (u32)(s32)(s16)x;
            *(u32*)(node + 0x60) = (u32)(s32)(s16)y;
            *(u32*)(node + 0x64) = (u32)(s32)(s16)z;
        }
    } else {
        if ((flags & 0x20) != 0) {
            u32 a = *(u16*)(node + 0x4C);
            u32 b = *(u16*)(node + 0x50);

            *(u16*)(node + 0x4C) = (u16)(x + a);
            a = *(u16*)(node + 0x4E);
            *(u16*)(node + 0x50) = (u16)(z + b);
            *(u16*)(node + 0x4E) = (u16)(y + a);
        } else {
            *(u16*)(node + 0x4C) = (u16)x;
            *(u16*)(node + 0x4E) = (u16)y;
            *(u16*)(node + 0x50) = (u16)z;
        }
    }
    node[4] = 1;
    node[5] = 1;
    if ((flags & 0x80) == 0) {
        return;
    }
    for (i = 1, e = *(u32*)(ctx + 4) + 0x7C; i < (s32)BG_U16(*(u32*)(ctx + 4) + 0xA);
         i++, e += 0x7C) {
        if (BG_U32(e) == self) {
            func_800AFD98(ctx, BG_PTR(e), flags & 0xFF, (u32)(s32)(s16)x,
                          (u32)(s32)(s16)y, (u32)(s32)(s16)z);
        }
    }
}
#endif /* XENO_PC_PORT */
#endif /* BATTLE_PART(2) */


#ifndef XENO_PC_PORT
extern u32 D_800D2D40;
#endif
#ifndef XENO_PC_PORT
extern u32 D_800D2D48;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3BCC[];
#endif
extern u16 D_8005A3A0[];
#ifndef XENO_PC_PORT
extern u8 D_800D2E62[];
#endif
#ifndef XENO_PC_PORT
extern u16 D_800D39E0;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800D3278;
#endif
extern void LoadImage(void* pRect, void* pData);
extern void DrawSync(s32 mode);


#if BATTLE_PART(2)
/* func_800B00D0.s: clear nine words counting down from D_800C3BCC. */
void func_800B00D0(void) {
    s32 i;

    for (i = 8; i >= 0; i--) {
        *(u32*)((u8*)D_800C3BCC + (i - 8) * 4) = 0;
    }
}
#endif /* BATTLE_PART(2) */
#if BATTLE_PART(2)
#ifdef XENO_PC_PORT
/* Instruction-faithful port bodies generated from the retail listings by
 *   tools/scripts/battle_lift_asm.py asm/battle/nonmatchings/main73_p2/<fn>.s --args N
 * (one statement per MIPS instruction; include/battle_lift.h has the
 * register, memory, COP2, frame and call contract), committed as reviewed
 * source. */
#define BL_NO_GTE /* no GTE callee; main73 declares its own GTE API */
#include "battle_lift.h"

/* func_800AFF9C.s (49 instructions; INCLUDE_ASM in main73_p2.c): walk-mesh
 * floor query for object a0 (model a0+4 position +0x5C/+0x60/+0x64, cached
 * triangle +0x1E): func_800A5914, else func_800A579C, then func_800A5870
 * into a frame local; writes the height back unless +0x36 is set.  The
 * three callees are typed native guest-ABI leaves reached through
 * bl_call_guest, so nothing here is interpreted. */
void func_800AFF9C(u32 arg0) {
    static const char FN[] = "func_800AFF9C";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x38u, &bl_saved);
    r_a0 = arg0;
    r_sp = r_sp + 0xFFFFFFC8u; /* 800AFF9C addiu $sp, $sp, -0x38 */
    bl_sw(r_sp + 0x28u, r_s0); /* 800AFFA0 sw $s0, 0x28($sp) */
    r_s0 = r_a0 + 0u; /* 800AFFA4 addu $s0, $a0, $zero */
    bl_sw(r_sp + 0x30u, r_ra); /* 800AFFA8 sw $ra, 0x30($sp) */
    bl_sw(r_sp + 0x2Cu, r_s1); /* 800AFFAC sw $s1, 0x2C($sp) */
    r_v0 = bl_lw(r_s0 + 0x4u); /* 800AFFB0 lw $v0, 0x4($s0) */
    ; /* 800AFFB4 nop  */
    r_v0 = bl_lhu(r_v0 + 0x5Cu); /* 800AFFB8 lhu $v0, 0x5C($v0) */
    ; /* 800AFFBC nop  */
    bl_sh(r_sp + 0x20u, r_v0); /* 800AFFC0 sh $v0, 0x20($sp) */
    r_v0 = bl_lw(r_s0 + 0x4u); /* 800AFFC4 lw $v0, 0x4($s0) */
    ; /* 800AFFC8 nop  */
    r_v0 = bl_lhu(r_v0 + 0x60u); /* 800AFFCC lhu $v0, 0x60($v0) */
    ; /* 800AFFD0 nop  */
    bl_sh(r_sp + 0x22u, r_v0); /* 800AFFD4 sh $v0, 0x22($sp) */
    r_v0 = bl_lw(r_s0 + 0x4u); /* 800AFFD8 lw $v0, 0x4($s0) */
    r_s1 = r_sp + 0x20u; /* 800AFFDC addiu $s1, $sp, 0x20 */
    r_v0 = bl_lhu(r_v0 + 0x64u); /* 800AFFE0 lhu $v0, 0x64($v0) */
    r_a0 = r_s1 + 0u; /* 800AFFE4 addu $a0, $s1, $zero */
    bl_sh(r_sp + 0x24u, r_v0); /* 800AFFE8 sh $v0, 0x24($sp) */
    r_a1 = bl_lh(r_s0 + 0x1Eu); /* 800AFFEC lh $a1, 0x1E($s0) */
    /* 800AFFF0 jal func_800A5914 */
    r_a2 = 0u | 0x4u;
    r_v0 = bl_call_guest(FN, 0x800A5914u, r_a0, r_a1, r_a2, r_a3, r_sp); /* func_800A5914 */
    bl_sh(r_s0 + 0x1Eu, r_v0); /* 800AFFF8 sh $v0, 0x1E($s0) */
    r_v0 = r_v0 << 16; /* 800AFFFC sll $v0, $v0, 16 */
    /* 800B0000 bgez $v0, .L800B0018 */
    { int c_ = ((s32)r_v0 >= 0); r_a0 = r_s1 + 0u; if (c_) goto L_800B0018; }
    /* 800B0008 jal func_800A579C */
    r_a0 = r_s1 + 0u;
    r_v0 = bl_call_guest(FN, 0x800A579Cu, r_a0, r_a1, r_a2, r_a3, r_sp); /* func_800A579C */
    bl_sh(r_s0 + 0x1Eu, r_v0); /* 800B0010 sh $v0, 0x1E($s0) */
    r_a0 = r_s1 + 0u; /* 800B0014 addu $a0, $s1, $zero */
L_800B0018:
    r_a1 = bl_lh(r_s0 + 0x1Eu); /* 800B0018 lh $a1, 0x1E($s0) */
    /* 800B001C jal func_800A5870 */
    r_a2 = r_sp + 0x10u;
    r_v0 = bl_call_guest(FN, 0x800A5870u, r_a0, r_a1, r_a2, r_a3, r_sp); /* func_800A5870 */
    r_v0 = bl_lhu(r_sp + 0x22u); /* 800B0024 lhu $v0, 0x22($sp) */
    r_v1 = bl_lbu(r_s0 + 0x36u); /* 800B0028 lbu $v1, 0x36($s0) */
    ; /* 800B002C nop  */
    /* 800B0030 bnez $v1, .L800B0048 */
    { int c_ = (r_v1 != 0); bl_sh(r_s0 + 0x60u, r_v0); if (c_) goto L_800B0048; }
    r_v1 = bl_lw(r_s0 + 0x4u); /* 800B0038 lw $v1, 0x4($s0) */
    r_v0 = bl_lh(r_sp + 0x22u); /* 800B003C lh $v0, 0x22($sp) */
    ; /* 800B0040 nop  */
    bl_sw(r_v1 + 0x60u, r_v0); /* 800B0044 sw $v0, 0x60($v1) */
L_800B0048:
    r_ra = bl_lw(r_sp + 0x30u); /* 800B0048 lw $ra, 0x30($sp) */
    r_s1 = bl_lw(r_sp + 0x2Cu); /* 800B004C lw $s1, 0x2C($sp) */
    r_s0 = bl_lw(r_sp + 0x28u); /* 800B0050 lw $s0, 0x28($sp) */
    r_sp = r_sp + 0x38u; /* 800B0054 addiu $sp, $sp, 0x38 */
    /* 800B0058 jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    (void)r_v0;
}
#endif /* XENO_PC_PORT */
#endif /* BATTLE_PART(2) */
