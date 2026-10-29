#include "common.h"
#ifndef XENO_PC_PORT
#include "psyq/libgpu.h"
#endif


#ifndef XENO_PC_PORT
void func_80076D58(POLY_FT4* prims, u8 sel, u8 mode);
extern u32 D_800C3EA4;
void func_80077074(void);
#endif

/* Fill a POLY_FT4's corners and texture coordinates: a 13-pixel-high strip
 * (func_80076C78) or a w x h quad (func_80076CE8) at (x, y) / (u, v). */
#ifndef XENO_PC_PORT
void func_80076C78(POLY_FT4* p, s16 x, s16 y, u8 u, u8 v, u8 w) {
    p->x0 = x;
    p->y0 = y;
    p->y1 = y;
    p->x2 = x;
    p->y2 = y + 13;
    p->y3 = y + 13;
    p->u0 = u;
    p->u2 = u;
    p->x1 = x + w;
    p->x3 = x + w;
    p->v0 = v;
    p->u1 = u + w;
    p->v1 = v;
    p->v2 = v + 13;
    p->u3 = u + w;
    p->v3 = v + 13;
}

void func_80076CE8(POLY_FT4* p, s16 x, s16 y, u8 u, u8 v, s32 w, s32 h) {
    p->x0 = x;
    p->y0 = y;
    p->y1 = y;
    p->x2 = x;
    p->u0 = u;
    p->u2 = u;
    p->x1 = x + w;
    p->y2 = y + h;
    p->x3 = x + w;
    p->y3 = y + h;
    p->v0 = v;
    p->u1 = u + w;
    p->v1 = v;
    p->v2 = v + h;
    p->u3 = u + w;
    p->v3 = v + h;
}
#endif
#ifndef XENO_PC_PORT
/* func_80076D58.s: initialise the two POLY_FT4s at `prims`: SetPolyFT4,
 * mid-grey colour, SetSemiTrans(0), SetShadeTex(1), the tpage for `mode`
 * (0..3: x 0x380/0x3C0, y 0/0x100; other values leave it) and the clut
 * g_SystemPalette2 when `sel` is set, else g_SystemPalette1.  The int
 * counter gives retail's signed loop test (gcc rewrites it onto the prim
 * pointer); the clut ternary gives the pointer copy both stores go through,
 * which only gcc 2.6.x emits (2.7.2: 4 instructions off). */
extern u16 g_SystemPalette1;
extern u16 g_SystemPalette2;

void func_80076D58(POLY_FT4* prims, u8 sel, u8 mode) {
    s32 i;

    for (i = 0; i < 2; i++) {
        SetPolyFT4(&prims[i]);
        setRGB0(&prims[i], 0x80, 0x80, 0x80);
        SetSemiTrans(&prims[i], 0);
        SetShadeTex(&prims[i], 1);
        switch (mode) {
        case 0:
            prims[i].tpage = GetTPage(0, 0, 0x380, 0);
            break;
        case 1:
            prims[i].tpage = GetTPage(0, 0, 0x380, 0x100);
            break;
        case 2:
            prims[i].tpage = GetTPage(0, 0, 0x3C0, 0x100);
            break;
        case 3:
            prims[i].tpage = GetTPage(0, 0, 0x3C0, 0);
            break;
        }
        prims[i].clut = sel ? g_SystemPalette2 : g_SystemPalette1;
    }
}
/* 4-byte records at D_800D2F68; only `width` (SystemRenderStringEntry's
 * result) is backed by another reader, the other names are by offset. */
typedef struct {
    u8 width;
    u8 unk1;
    u8 unk2;
    u8 unk3;
} BattleTextRec;
extern BattleTextRec D_800D2F68[];
/* Image pointer wrapped as an aggregate element on purpose: arrays of
 * aggregates keep gcc's offset-giv addressing (lw %lo(sym)(at)) that
 * retail has; a plain void*[] turns into a pointer walk. */
typedef struct {
    void* data;
} BattleImageRef;
extern BattleImageRef D_800C3E5C[];
extern void* func_8008AC00(s32 kind);
extern void* func_800338D8(s32 id);
extern s32 SystemRenderStringEntry(void* pString, void* pWork, s32 height, s32 flag);
extern void func_800769E8(void* pRect, void* pData);
extern void HeapFree(void* p);
extern void* bzero(unsigned char* p, int size);

/* For i in 0..10: render the strings func_800338D8 returns for ids 0xA + i
 * and 0x15 + i into a fresh 0x30C-byte buffer (SystemRenderStringEntry, 0x1B,
 * flag 0 then 1), store the widths in D_800D2F68 records i and 11 + i (with
 * +2 = 0x78, +3 = 13 (i + 1), +1 = 0 / 1) and upload the buffer to
 * (0x3DE, 0x10D + 13 i) 30x13.  The buffers are freed, then the ten
 * D_800C3E5C images go to (0x3DE + 2 i, 0x100) 6x13. */
void func_80076EA4(void) {
    BattleImageRef buf[11];
    RECT r;
    s32 i;

    for (i = 0; i < 11; i++) {
        buf[i].data = func_8008AC00(0x1B);
        bzero(buf[i].data, 0x30C);
        D_800D2F68[i].width = SystemRenderStringEntry(func_800338D8(i + 0xA), buf[i].data, 0x1B, 0);
        D_800D2F68[i + 11].width = SystemRenderStringEntry(func_800338D8(i + 0x15), buf[i].data, 0x1B, 1);
        D_800D2F68[i].unk2 = D_800D2F68[i + 11].unk2 = 0x78;
        D_800D2F68[i].unk3 = D_800D2F68[i + 11].unk3 = i * 0xD + 0xD;
        D_800D2F68[i].unk1 = 0;
        D_800D2F68[i + 11].unk1 = 1;
        r.y = i * 0xD + 0x10D;
        r.x = 0x3DE;
        r.w = 0x1E;
        r.h = 0xD;
        func_800769E8(&r, buf[i].data);
    }
    for (i = 0; i < 11; i++) {
        HeapFree(buf[i].data);
    }
    for (i = 0; i < 10; i++) {
        r.x = 0x3DE + i * 2;
        r.y = 0x100;
        r.w = 6;
        r.h = 0xD;
        func_800769E8(&r, D_800C3E5C[i].data);
    }
}

#ifndef XENO_PC_PORT
void func_80077074(void) {
    *(s8 *)((s8*)(*(void **)((s8*)((*(void **)&D_800C3EA4)) + 0xA230)) + 0x669) = 0;
    *(s8 *)((s8*)(*(void **)((s8*)((*(void **)&D_800C3EA4)) + 0xA230)) + 0x66B) = 0;
    *(s8 *)((s8*)(*(void **)((s8*)((*(void **)&D_800C3EA4)) + 0xA230)) + 0x66D) = 0;
    func_80076D58(*(void **)((s8*)((*(void **)&D_800C3EA4)) + 0xA230), 0, 1);
    func_80076D58(*(void **)((s8*)((*(void **)&D_800C3EA4)) + 0xA230) + 0x50, 0, 1);
    func_80076D58(*(void **)((s8*)((*(void **)&D_800C3EA4)) + 0xA230) + 0xA0, 0, 2);
    func_80076D58(*(void **)((s8*)((*(void **)&D_800C3EA4)) + 0xA230) + 0xF0, 0, 2);
    func_80076D58(*(void **)((s8*)((*(void **)&D_800C3EA4)) + 0xA230) + 0x140, 0, 1);
    func_80076D58(*(void **)((s8*)((*(void **)&D_800C3EA4)) + 0xA230) + 0x190, 0, 2);
    func_80076D58(*(void **)((s8*)((*(void **)&D_800C3EA4)) + 0xA230) + 0x1E0, 0, 2);
    func_80076D58(*(void **)((s8*)((*(void **)&D_800C3EA4)) + 0xA230) + 0x230, 0, 2);
    func_80076D58(*(void **)((s8*)((*(void **)&D_800C3EA4)) + 0xA230) + 0x280, 0, 3);
    func_80076D58(*(void **)((s8*)((*(void **)&D_800C3EA4)) + 0xA230) + 0x2D0, 1, 3);
    func_80076D58(*(void **)((s8*)((*(void **)&D_800C3EA4)) + 0xA230) + 0x320, 0, 2);
    func_80076D58(*(void **)((s8*)((*(void **)&D_800C3EA4)) + 0xA230) + 0x370, 1, 2);
    func_80076D58(*(void **)((s8*)((*(void **)&D_800C3EA4)) + 0xA230) + 0x3C0, 0, 2);
    func_80076D58(*(void **)((s8*)((*(void **)&D_800C3EA4)) + 0xA230) + 0x410, 0, 2);
    func_80076D58(*(void **)((s8*)((*(void **)&D_800C3EA4)) + 0xA230) + 0x460, 0, 2);
    func_80076D58(*(void **)((s8*)((*(void **)&D_800C3EA4)) + 0xA230) + 0x4B0, 0, 2);
    func_80076D58(*(void **)((s8*)((*(void **)&D_800C3EA4)) + 0xA230) + 0x500, 0, 2);
    func_80076D58(*(void **)((s8*)((*(void **)&D_800C3EA4)) + 0xA230) + 0x550, 0, 2);
}
#endif

void func_80077364(POLY_FT4* prims, u8 index) {
    s32 i;
    s32 off;

    for (i = 0; i < 4; i++) {
        POLY_FT4* p = &prims[i];

        SetPolyFT4(p);
        SetShadeTex(p, 1);
        setRGB0(p, 0xFF, 0xFF, 0xFF);
        off = index * 0x18;
        p->tpage = GetTPage(*(s32*)(D_800C3EA4 + off + 0xA238), 0,
                            *(s32*)(D_800C3EA4 + off + 0xA244),
                            *(s32*)(D_800C3EA4 + off + 0xA248));
        p->clut = GetClut(*(s32*)(D_800C3EA4 + off + 0xA23C),
                          *(s32*)(D_800C3EA4 + off + 0xA240));
    }
}

extern u32 D_800D2E38[];
extern u8 D_800594D4[];
extern s32 D_800595A0;
extern u8* D_800D2D28;

/* Prepare battle window `index`'s prims: for both buffers, the background
 * POLY_G4 (block +0x3C0) gets the D_800594D4 colour on all four corners and
 * semi-transparency, and its DR_MODE (block +0x408) the tpage of the frame
 * texture (UI state +0xA25C/+0xA260, abr D_800595A0); then func_80077364
 * sets up the four edge prim pairs (+0x140 .. +0x320). */
void func_80077454(u8 index) {
    u8* blk;
    u8 i;

    blk = (u8*)D_800D2E38[index];
    for (i = 0; i < 2; i++) {
        SetPolyG4((POLY_G4*)((i * 0x24 + 0x3C0) + blk));
        (blk + i * 0x24)[0x3C4] = D_800594D4[0];
        (blk + i * 0x24)[0x3C5] = D_800594D4[1];
        (blk + i * 0x24)[0x3C6] = D_800594D4[2];
        (blk + i * 0x24)[0x3CC] = D_800594D4[0];
        (blk + i * 0x24)[0x3CD] = D_800594D4[1];
        (blk + i * 0x24)[0x3CE] = D_800594D4[2];
        (blk + i * 0x24)[0x3D4] = D_800594D4[0];
        (blk + i * 0x24)[0x3D5] = D_800594D4[1];
        (blk + i * 0x24)[0x3D6] = D_800594D4[2];
        (blk + i * 0x24)[0x3DC] = D_800594D4[0];
        (blk + i * 0x24)[0x3DD] = D_800594D4[1];
        (blk + i * 0x24)[0x3DE] = D_800594D4[2];
        SetSemiTrans((POLY_G4*)((i * 0x24 + 0x3C0) + blk), 1);
        SetDrawMode((DR_MODE*)((i * 0xC + 0x408) + blk), 0, 0,
                    GetTPage(0, D_800595A0, *(s32*)((u8*)D_800C3EA4 + 0xA25C), *(s32*)((u8*)D_800C3EA4 + 0xA260)),
                    (RECT*)D_800D2D28);
    }
    func_80077364((POLY_FT4*)(blk + 0x140), 1);
    func_80077364((POLY_FT4*)(blk + 0x1E0), 2);
    func_80077364((POLY_FT4*)(blk + 0x280), 3);
    func_80077364((POLY_FT4*)(blk + 0x320), 4);
}
#endif


#ifndef XENO_PC_PORT
extern u8* D_800D2D28;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D3420[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D3430[];
#endif
#ifndef XENO_PC_PORT
extern u32 D_800D3410[];
#endif
extern u16 D_8005A3A0[];
#ifndef XENO_PC_PORT
extern u8 D_800D366C;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800C3EAC;
#endif
extern u32 func_80089C08(u32 v);
extern void func_800BC404(u32 v);
extern void func_800BCD98(u32 v);
extern void func_80085454(s32);
extern void func_80085618(s32);
extern void func_8008AA40(u8 v);
#ifndef XENO_PC_PORT
extern u32 D_800C3EA4;
#endif
extern void* func_8008ABB8(s32 size, s32 flag);
#ifndef XENO_PC_PORT
extern void* bzero(unsigned char* p, int size);
#endif
extern void func_80077074(void);


/* func_80077610.s */
#ifndef XENO_PC_PORT /* port: not adopted; stays out of the port TU */
void func_80077610(void) {
    void* p = func_8008ABB8(0x670, 0);

    *(u32*)((u8*)D_800C3EA4 + 0xA230) = (u32)p;
    bzero(p, 0x670);
    func_80077074();
}
#endif
/* func_8007765C.s */
#ifndef XENO_PC_PORT /* port: not adopted; stays out of the port TU */
void func_8007765C(void) {
    func_800716D8();
    HeapFree(*(u32*)((u8*)D_800C3EA4 + 0xA230));
}
#endif

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

/* func_80077364.s (60 instructions, frame 0x28; lifted with --args 2 --void).
 * Callees through the jal service: GetClut, GetTPage, SetPolyFT4, SetShadeTex. */
void func_80077364(u32 arg0, u32 arg1) {
    static const char FN[] = "func_80077364";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x28u, &bl_saved);
    r_a0 = arg0;
    r_a1 = arg1;
    r_sp = r_sp + 0xFFFFFFD8u; /* 80077364 addiu $sp, $sp, -0x28 */
    bl_sw(r_sp + 0x14u, r_s1); /* 80077368 sw $s1, 0x14($sp) */
    r_s1 = 0u + 0u; /* 8007736C addu $s1, $zero, $zero */
    bl_sw(r_sp + 0x1Cu, r_s3); /* 80077370 sw $s3, 0x1C($sp) */
    r_s3 = 0u | 0xFFu; /* 80077374 ori $s3, $zero, 0xFF */
    r_a1 = r_a1 & 0xFFu; /* 80077378 andi $a1, $a1, 0xFF */
    r_v0 = r_a1 << 1; /* 8007737C sll $v0, $a1, 1 */
    r_v0 = r_v0 + r_a1; /* 80077380 addu $v0, $v0, $a1 */
    bl_sw(r_sp + 0x18u, r_s2); /* 80077384 sw $s2, 0x18($sp) */
    r_s2 = r_v0 << 3; /* 80077388 sll $s2, $v0, 3 */
    bl_sw(r_sp + 0x10u, r_s0); /* 8007738C sw $s0, 0x10($sp) */
    r_s0 = r_a0 + 0u; /* 80077390 addu $s0, $a0, $zero */
    bl_sw(r_sp + 0x20u, r_ra); /* 80077394 sw $ra, 0x20($sp) */
L_80077398:
    /* 80077398 jal SetPolyFT4 */
    r_a0 = r_s0 + 0u;
    bl_jal(FN, 0x80043CB0u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* SetPolyFT4 */
    r_a0 = r_s0 + 0u; /* 800773A0 addu $a0, $s0, $zero */
    /* 800773A4 jal SetShadeTex */
    r_a1 = 0u | 0x1u;
    bl_jal(FN, 0x80043C24u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* SetShadeTex */
    bl_sb(r_s0 + 0x4u, r_s3); /* 800773AC sb $s3, 0x4($s0) */
    bl_sb(r_s0 + 0x5u, r_s3); /* 800773B0 sb $s3, 0x5($s0) */
    bl_sb(r_s0 + 0x6u, r_s3); /* 800773B4 sb $s3, 0x6($s0) */
    r_v0 = 0x800C0000u; /* 800773B8 lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 800773BC lw $v0, %lo(D_800C3EA4)($v0) */
    ; /* 800773C0 nop  */
    r_v0 = r_v0 + r_s2; /* 800773C4 addu $v0, $v0, $s2 */
    r_at = 0x10000u; /* 800773C8 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 800773CC addu $at, $v0, $at */
    r_a0 = bl_lw(r_at + 0xFFFFA238u); /* 800773D0 lw $a0, -0x5DC8($at) */
    r_at = 0x10000u; /* 800773D4 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 800773D8 addu $at, $v0, $at */
    r_a2 = bl_lw(r_at + 0xFFFFA244u); /* 800773DC lw $a2, -0x5DBC($at) */
    r_at = 0x10000u; /* 800773E0 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 800773E4 addu $at, $v0, $at */
    r_a3 = bl_lw(r_at + 0xFFFFA248u); /* 800773E8 lw $a3, -0x5DB8($at) */
    /* 800773EC jal GetTPage */
    r_a1 = 0u + 0u;
    bl_jal(FN, 0x80043A1Cu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* GetTPage */
    r_v1 = 0x800C0000u; /* 800773F4 lui $v1, %hi(D_800C3EA4) */
    r_v1 = bl_lw(r_v1 + 0x3EA4u); /* 800773F8 lw $v1, %lo(D_800C3EA4)($v1) */
    bl_sh(r_s0 + 0x16u, r_v0); /* 800773FC sh $v0, 0x16($s0) */
    r_v1 = r_v1 + r_s2; /* 80077400 addu $v1, $v1, $s2 */
    r_at = 0x10000u; /* 80077404 lui $at, (0x10000 >> 16) */
    r_at = r_v1 + r_at; /* 80077408 addu $at, $v1, $at */
    r_a0 = bl_lw(r_at + 0xFFFFA23Cu); /* 8007740C lw $a0, -0x5DC4($at) */
    r_at = 0x10000u; /* 80077410 lui $at, (0x10000 >> 16) */
    r_at = r_v1 + r_at; /* 80077414 addu $at, $v1, $at */
    r_a1 = bl_lw(r_at + 0xFFFFA240u); /* 80077418 lw $a1, -0x5DC0($at) */
    /* 8007741C jal GetClut */
    r_s1 = r_s1 + 0x1u;
    bl_jal(FN, 0x80043A58u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* GetClut */
    bl_sh(r_s0 + 0xEu, r_v0); /* 80077424 sh $v0, 0xE($s0) */
    r_v0 = (u32)((s32)r_s1 < 4); /* 80077428 slti $v0, $s1, 0x4 */
    /* 8007742C bnez $v0, .L80077398 */
    { int c_ = (r_v0 != 0); r_s0 = r_s0 + 0x28u; if (c_) goto L_80077398; }
    r_ra = bl_lw(r_sp + 0x20u); /* 80077434 lw $ra, 0x20($sp) */
    r_s3 = bl_lw(r_sp + 0x1Cu); /* 80077438 lw $s3, 0x1C($sp) */
    r_s2 = bl_lw(r_sp + 0x18u); /* 8007743C lw $s2, 0x18($sp) */
    r_s1 = bl_lw(r_sp + 0x14u); /* 80077440 lw $s1, 0x14($sp) */
    r_s0 = bl_lw(r_sp + 0x10u); /* 80077444 lw $s0, 0x10($sp) */
    r_sp = r_sp + 0x28u; /* 80077448 addiu $sp, $sp, 0x28 */
    /* 8007744C jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    (void)r_v0;
}

/* func_80076C78.s (28 instructions, frame 0x0; lifted with --args 6 --void).
 * Callees through the jal service: none. */
void func_80076C78(u32 arg0, u32 arg1, u32 arg2, u32 arg3, u32 arg4, u32 arg5) {
    static const char FN[] = "func_80076C78";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x0u, &bl_saved);
    r_a0 = arg0;
    r_a1 = arg1;
    r_a2 = arg2;
    r_a3 = arg3;
    bl_sw(r_sp + 0x10u, arg4);
    bl_sw(r_sp + 0x14u, arg5);
    r_v1 = r_a1 + 0u; /* 80076C78 addu $v1, $a1, $zero */
    r_v0 = r_a2 + 0u; /* 80076C7C addu $v0, $a2, $zero */
    r_a2 = r_a2 + 0xDu; /* 80076C80 addiu $a2, $a2, 0xD */
    bl_sh(r_a0 + 0x8u, r_v1); /* 80076C84 sh $v1, 0x8($a0) */
    bl_sh(r_a0 + 0xAu, r_v0); /* 80076C88 sh $v0, 0xA($a0) */
    bl_sh(r_a0 + 0x12u, r_v0); /* 80076C8C sh $v0, 0x12($a0) */
    bl_sh(r_a0 + 0x18u, r_v1); /* 80076C90 sh $v1, 0x18($a0) */
    bl_sh(r_a0 + 0x1Au, r_a2); /* 80076C94 sh $a2, 0x1A($a0) */
    bl_sh(r_a0 + 0x22u, r_a2); /* 80076C98 sh $a2, 0x22($a0) */
    r_v1 = bl_lw(r_sp + 0x10u); /* 80076C9C lw $v1, 0x10($sp) */
    r_a2 = bl_lw(r_sp + 0x14u); /* 80076CA0 lw $a2, 0x14($sp) */
    r_v0 = r_a3 + 0u; /* 80076CA4 addu $v0, $a3, $zero */
    bl_sb(r_a0 + 0xCu, r_v0); /* 80076CA8 sb $v0, 0xC($a0) */
    bl_sb(r_a0 + 0x1Cu, r_v0); /* 80076CAC sb $v0, 0x1C($a0) */
    r_v0 = r_a2 & 0xFFu; /* 80076CB0 andi $v0, $a2, 0xFF */
    r_a1 = r_a1 + r_v0; /* 80076CB4 addu $a1, $a1, $v0 */
    r_v0 = r_v1 + 0u; /* 80076CB8 addu $v0, $v1, $zero */
    r_a3 = r_a3 + r_a2; /* 80076CBC addu $a3, $a3, $a2 */
    r_v1 = r_v1 + 0xDu; /* 80076CC0 addiu $v1, $v1, 0xD */
    bl_sh(r_a0 + 0x10u, r_a1); /* 80076CC4 sh $a1, 0x10($a0) */
    bl_sh(r_a0 + 0x20u, r_a1); /* 80076CC8 sh $a1, 0x20($a0) */
    bl_sb(r_a0 + 0xDu, r_v0); /* 80076CCC sb $v0, 0xD($a0) */
    bl_sb(r_a0 + 0x14u, r_a3); /* 80076CD0 sb $a3, 0x14($a0) */
    bl_sb(r_a0 + 0x15u, r_v0); /* 80076CD4 sb $v0, 0x15($a0) */
    bl_sb(r_a0 + 0x1Du, r_v1); /* 80076CD8 sb $v1, 0x1D($a0) */
    bl_sb(r_a0 + 0x24u, r_a3); /* 80076CDC sb $a3, 0x24($a0) */
    /* 80076CE0 jr $ra */
    bl_sb(r_a0 + 0x25u, r_v1); goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    (void)r_v0;
}

/* func_80076CE8.s (28 instructions, frame 0x0; lifted with --args 7 --void).
 * Callees through the jal service: none. */
void func_80076CE8(u32 arg0, u32 arg1, u32 arg2, u32 arg3, u32 arg4, u32 arg5, u32 arg6) {
    static const char FN[] = "func_80076CE8";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x0u, &bl_saved);
    r_a0 = arg0;
    r_a1 = arg1;
    r_a2 = arg2;
    r_a3 = arg3;
    bl_sw(r_sp + 0x10u, arg4);
    bl_sw(r_sp + 0x14u, arg5);
    bl_sw(r_sp + 0x18u, arg6);
    r_v1 = r_a1 + 0u; /* 80076CE8 addu $v1, $a1, $zero */
    r_v0 = r_a2 + 0u; /* 80076CEC addu $v0, $a2, $zero */
    bl_sh(r_a0 + 0x8u, r_v1); /* 80076CF0 sh $v1, 0x8($a0) */
    bl_sh(r_a0 + 0xAu, r_v0); /* 80076CF4 sh $v0, 0xA($a0) */
    bl_sh(r_a0 + 0x12u, r_v0); /* 80076CF8 sh $v0, 0x12($a0) */
    bl_sh(r_a0 + 0x18u, r_v1); /* 80076CFC sh $v1, 0x18($a0) */
    r_v1 = bl_lw(r_sp + 0x10u); /* 80076D00 lw $v1, 0x10($sp) */
    r_t0 = bl_lw(r_sp + 0x14u); /* 80076D04 lw $t0, 0x14($sp) */
    r_t1 = bl_lw(r_sp + 0x18u); /* 80076D08 lw $t1, 0x18($sp) */
    r_v0 = r_a3 + 0u; /* 80076D0C addu $v0, $a3, $zero */
    bl_sb(r_a0 + 0xCu, r_v0); /* 80076D10 sb $v0, 0xC($a0) */
    bl_sb(r_a0 + 0x1Cu, r_v0); /* 80076D14 sb $v0, 0x1C($a0) */
    r_a1 = r_a1 + r_t0; /* 80076D18 addu $a1, $a1, $t0 */
    r_a2 = r_a2 + r_t1; /* 80076D1C addu $a2, $a2, $t1 */
    r_v0 = r_v1 + 0u; /* 80076D20 addu $v0, $v1, $zero */
    r_a3 = r_a3 + r_t0; /* 80076D24 addu $a3, $a3, $t0 */
    r_v1 = r_v1 + r_t1; /* 80076D28 addu $v1, $v1, $t1 */
    bl_sh(r_a0 + 0x10u, r_a1); /* 80076D2C sh $a1, 0x10($a0) */
    bl_sh(r_a0 + 0x1Au, r_a2); /* 80076D30 sh $a2, 0x1A($a0) */
    bl_sh(r_a0 + 0x20u, r_a1); /* 80076D34 sh $a1, 0x20($a0) */
    bl_sh(r_a0 + 0x22u, r_a2); /* 80076D38 sh $a2, 0x22($a0) */
    bl_sb(r_a0 + 0xDu, r_v0); /* 80076D3C sb $v0, 0xD($a0) */
    bl_sb(r_a0 + 0x14u, r_a3); /* 80076D40 sb $a3, 0x14($a0) */
    bl_sb(r_a0 + 0x15u, r_v0); /* 80076D44 sb $v0, 0x15($a0) */
    bl_sb(r_a0 + 0x1Du, r_v1); /* 80076D48 sb $v1, 0x1D($a0) */
    bl_sb(r_a0 + 0x24u, r_a3); /* 80076D4C sb $a3, 0x24($a0) */
    /* 80076D50 jr $ra */
    bl_sb(r_a0 + 0x25u, r_v1); goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    (void)r_v0;
}
#endif /* XENO_PC_PORT */

#ifdef XENO_PC_PORT /* h-frontier lifted bodies */
/* Instruction-faithful port bodies (native-only frontier), generated by
 *   tools/scripts/battle_lift_asm.py asm/battle/matchings/<tu>/<fn>.s --args N [--void] --jal-service
 * (include/battle_lift.h; every call goes through the interpreter's own
 * jal path, so each callee gets the binding an interpreted jal gets).  The
 * matched C above stays the matching build's. */
#include "battle_lift.h"

/* func_80077454.s (111 instructions; --args 1 --void), frontier step2.  Callees through the jal service: GetTPage, SetDrawMode, SetPolyG4, SetSemiTrans, func_80077364. */
void func_80077454(u32 arg0) {
    static const char FN[] = "func_80077454";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x38u, &bl_saved);
    r_a0 = arg0;
    r_sp = r_sp + 0xFFFFFFC8u; /* 80077454 addiu $sp, $sp, -0x38 */
    r_a0 = r_a0 & 0xFFu; /* 80077458 andi $a0, $a0, 0xFF */
    r_a0 = r_a0 << 2; /* 8007745C sll $a0, $a0, 2 */
    bl_sw(r_sp + 0x2Cu, r_s5); /* 80077460 sw $s5, 0x2C($sp) */
    r_s5 = 0u + 0u; /* 80077464 addu $s5, $zero, $zero */
    bl_sw(r_sp + 0x24u, r_s3); /* 80077468 sw $s3, 0x24($sp) */
    r_s3 = 0x80060000u; /* 8007746C lui $s3, %hi(D_800594D4) */
    r_s3 = r_s3 + 0xFFFF94D4u; /* 80077470 addiu $s3, $s3, %lo(D_800594D4) */
    bl_sw(r_sp + 0x30u, r_ra); /* 80077474 sw $ra, 0x30($sp) */
    bl_sw(r_sp + 0x28u, r_s4); /* 80077478 sw $s4, 0x28($sp) */
    bl_sw(r_sp + 0x20u, r_s2); /* 8007747C sw $s2, 0x20($sp) */
    bl_sw(r_sp + 0x1Cu, r_s1); /* 80077480 sw $s1, 0x1C($sp) */
    bl_sw(r_sp + 0x18u, r_s0); /* 80077484 sw $s0, 0x18($sp) */
    r_at = 0x800D0000u; /* 80077488 lui $at, %hi(D_800D2E38) */
    r_at = r_at + r_a0; /* 8007748C addu $at, $at, $a0 */
    r_s4 = bl_lw(r_at + 0x2E38u); /* 80077490 lw $s4, %lo(D_800D2E38)($at) */
L_80077494:
    r_s2 = r_s5 & 0xFFu; /* 80077494 andi $s2, $s5, 0xFF */
    r_s0 = r_s2 << 3; /* 80077498 sll $s0, $s2, 3 */
    r_s0 = r_s0 + r_s2; /* 8007749C addu $s0, $s0, $s2 */
    r_s0 = r_s0 << 2; /* 800774A0 sll $s0, $s0, 2 */
    r_s1 = r_s0 + 0x3C0u; /* 800774A4 addiu $s1, $s0, 0x3C0 */
    r_s1 = r_s4 + r_s1; /* 800774A8 addu $s1, $s4, $s1 */
    /* 800774AC jal SetPolyG4 */
    r_a0 = r_s1 + 0u;
    bl_jal(FN, 0x80043CC4u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* SetPolyG4 */
    r_v0 = bl_lbu(r_s3); /* 800774B4 lbu $v0, 0x0($s3) */
    r_s0 = r_s0 + r_s4; /* 800774B8 addu $s0, $s0, $s4 */
    bl_sb(r_s0 + 0x3C4u, r_v0); /* 800774BC sb $v0, 0x3C4($s0) */
    r_v0 = bl_lbu(r_s3 + 0x1u); /* 800774C0 lbu $v0, 0x1($s3) */
    ; /* 800774C4 nop  */
    bl_sb(r_s0 + 0x3C5u, r_v0); /* 800774C8 sb $v0, 0x3C5($s0) */
    r_v0 = bl_lbu(r_s3 + 0x2u); /* 800774CC lbu $v0, 0x2($s3) */
    ; /* 800774D0 nop  */
    bl_sb(r_s0 + 0x3C6u, r_v0); /* 800774D4 sb $v0, 0x3C6($s0) */
    r_v0 = bl_lbu(r_s3); /* 800774D8 lbu $v0, 0x0($s3) */
    ; /* 800774DC nop  */
    bl_sb(r_s0 + 0x3CCu, r_v0); /* 800774E0 sb $v0, 0x3CC($s0) */
    r_v0 = bl_lbu(r_s3 + 0x1u); /* 800774E4 lbu $v0, 0x1($s3) */
    ; /* 800774E8 nop  */
    bl_sb(r_s0 + 0x3CDu, r_v0); /* 800774EC sb $v0, 0x3CD($s0) */
    r_v0 = bl_lbu(r_s3 + 0x2u); /* 800774F0 lbu $v0, 0x2($s3) */
    ; /* 800774F4 nop  */
    bl_sb(r_s0 + 0x3CEu, r_v0); /* 800774F8 sb $v0, 0x3CE($s0) */
    r_v0 = bl_lbu(r_s3); /* 800774FC lbu $v0, 0x0($s3) */
    ; /* 80077500 nop  */
    bl_sb(r_s0 + 0x3D4u, r_v0); /* 80077504 sb $v0, 0x3D4($s0) */
    r_v0 = bl_lbu(r_s3 + 0x1u); /* 80077508 lbu $v0, 0x1($s3) */
    ; /* 8007750C nop  */
    bl_sb(r_s0 + 0x3D5u, r_v0); /* 80077510 sb $v0, 0x3D5($s0) */
    r_v0 = bl_lbu(r_s3 + 0x2u); /* 80077514 lbu $v0, 0x2($s3) */
    ; /* 80077518 nop  */
    bl_sb(r_s0 + 0x3D6u, r_v0); /* 8007751C sb $v0, 0x3D6($s0) */
    r_v0 = bl_lbu(r_s3); /* 80077520 lbu $v0, 0x0($s3) */
    r_a0 = r_s1 + 0u; /* 80077524 addu $a0, $s1, $zero */
    bl_sb(r_s0 + 0x3DCu, r_v0); /* 80077528 sb $v0, 0x3DC($s0) */
    r_v0 = bl_lbu(r_s3 + 0x1u); /* 8007752C lbu $v0, 0x1($s3) */
    r_a1 = 0u | 0x1u; /* 80077530 ori $a1, $zero, 0x1 */
    bl_sb(r_s0 + 0x3DDu, r_v0); /* 80077534 sb $v0, 0x3DD($s0) */
    r_v0 = bl_lbu(r_s3 + 0x2u); /* 80077538 lbu $v0, 0x2($s3) */
    r_s5 = r_s5 + 0x1u; /* 8007753C addiu $s5, $s5, 0x1 */
    /* 80077540 jal SetSemiTrans */
    bl_sb(r_s0 + 0x3DEu, r_v0);
    bl_jal(FN, 0x80043BFCu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* SetSemiTrans */
    r_v0 = 0x800C0000u; /* 80077548 lui $v0, %hi(D_800C3EA4) */
    r_v0 = bl_lw(r_v0 + 0x3EA4u); /* 8007754C lw $v0, %lo(D_800C3EA4)($v0) */
    r_a1 = 0x80060000u; /* 80077550 lui $a1, %hi(D_800595A0) */
    r_a1 = bl_lw(r_a1 + 0xFFFF95A0u); /* 80077554 lw $a1, %lo(D_800595A0)($a1) */
    r_at = 0x10000u; /* 80077558 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 8007755C addu $at, $v0, $at */
    r_a2 = bl_lw(r_at + 0xFFFFA25Cu); /* 80077560 lw $a2, -0x5DA4($at) */
    r_at = 0x10000u; /* 80077564 lui $at, (0x10000 >> 16) */
    r_at = r_v0 + r_at; /* 80077568 addu $at, $v0, $at */
    r_a3 = bl_lw(r_at + 0xFFFFA260u); /* 8007756C lw $a3, -0x5DA0($at) */
    /* 80077570 jal GetTPage */
    r_a0 = 0u + 0u;
    bl_jal(FN, 0x80043A1Cu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* GetTPage */
    r_a0 = r_s2 << 1; /* 80077578 sll $a0, $s2, 1 */
    r_a0 = r_a0 + r_s2; /* 8007757C addu $a0, $a0, $s2 */
    r_a0 = r_a0 << 2; /* 80077580 sll $a0, $a0, 2 */
    r_a0 = r_a0 + 0x408u; /* 80077584 addiu $a0, $a0, 0x408 */
    r_a0 = r_s4 + r_a0; /* 80077588 addu $a0, $s4, $a0 */
    r_a1 = 0u + 0u; /* 8007758C addu $a1, $zero, $zero */
    r_a2 = 0u + 0u; /* 80077590 addu $a2, $zero, $zero */
    r_v1 = 0x800D0000u; /* 80077594 lui $v1, %hi(D_800D2D28) */
    r_v1 = bl_lw(r_v1 + 0x2D28u); /* 80077598 lw $v1, %lo(D_800D2D28)($v1) */
    r_a3 = r_v0 & 0xFFFFu; /* 8007759C andi $a3, $v0, 0xFFFF */
    /* 800775A0 jal SetDrawMode */
    bl_sw(r_sp + 0x10u, r_v1);
    bl_jal(FN, 0x800454DCu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* SetDrawMode */
    r_v0 = r_s5 & 0xFFu; /* 800775A8 andi $v0, $s5, 0xFF */
    r_v0 = (u32)(r_v0 < 0x2u); /* 800775AC sltiu $v0, $v0, 0x2 */
    /* 800775B0 bnez $v0, .L80077494 */
    { int c_ = (r_v0 != 0); ; if (c_) goto L_80077494; }
    r_a0 = r_s4 + 0x140u; /* 800775B8 addiu $a0, $s4, 0x140 */
    /* 800775BC jal func_80077364 */
    r_a1 = 0u | 0x1u;
    bl_jal(FN, 0x80077364u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80077364 */
    r_a0 = r_s4 + 0x1E0u; /* 800775C4 addiu $a0, $s4, 0x1E0 */
    /* 800775C8 jal func_80077364 */
    r_a1 = 0u | 0x2u;
    bl_jal(FN, 0x80077364u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80077364 */
    r_a0 = r_s4 + 0x280u; /* 800775D0 addiu $a0, $s4, 0x280 */
    /* 800775D4 jal func_80077364 */
    r_a1 = 0u | 0x3u;
    bl_jal(FN, 0x80077364u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80077364 */
    r_a0 = r_s4 + 0x320u; /* 800775DC addiu $a0, $s4, 0x320 */
    /* 800775E0 jal func_80077364 */
    r_a1 = 0u | 0x4u;
    bl_jal(FN, 0x80077364u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_80077364 */
    r_ra = bl_lw(r_sp + 0x30u); /* 800775E8 lw $ra, 0x30($sp) */
    r_s5 = bl_lw(r_sp + 0x2Cu); /* 800775EC lw $s5, 0x2C($sp) */
    r_s4 = bl_lw(r_sp + 0x28u); /* 800775F0 lw $s4, 0x28($sp) */
    r_s3 = bl_lw(r_sp + 0x24u); /* 800775F4 lw $s3, 0x24($sp) */
    r_s2 = bl_lw(r_sp + 0x20u); /* 800775F8 lw $s2, 0x20($sp) */
    r_s1 = bl_lw(r_sp + 0x1Cu); /* 800775FC lw $s1, 0x1C($sp) */
    r_s0 = bl_lw(r_sp + 0x18u); /* 80077600 lw $s0, 0x18($sp) */
    r_sp = r_sp + 0x38u; /* 80077604 addiu $sp, $sp, 0x38 */
    /* 80077608 jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    (void)r_v0;
}

/* func_80077364: adopted from the h-lift block above (321613d2), whose body is
 * identical to this step's lift and to a fresh --jal-service lift. */
#endif /* XENO_PC_PORT h-frontier */
