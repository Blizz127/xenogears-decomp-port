#include "common.h"
#ifndef XENO_PC_PORT
s16 func_800A3490(s16 arg0, s16 arg1, s32 arg2);
s16 func_800A3578(s16 arg0, s16 arg1, s32 arg2);
s16 func_800A35C8(s16 arg0, s16 arg1, s16 arg2);
#endif

#ifndef XENO_PC_PORT
#include "system/memory.h"
#endif
#ifdef XENO_PC_PORT
extern u_int HeapFree(void* p);
#endif
#ifndef XENO_PC_PORT
void func_800A429C(void *arg0);
#endif

#ifdef XENO_PC_PORT
#include "psyq/libgte.h"
#include "psyq/libgpu.h"
#endif


#ifndef XENO_PC_PORT

#endif
/* Shared by the matching build and the port (matched C). */
s16 func_800A3490(s16 arg0, s16 arg1, s32 arg2) {
    return (s16) (arg2 + ((s32) (rsin(arg0) + 0x1000) / arg1));
}
/* The retail target traps on a zero divisor. Keep that host seam out of the
 * matching preprocessor output so the retail body remains the plain divide. */
#ifdef XENO_PC_PORT
#define M68_EFFECT_DIVIDE(numerator, divisor) \
    ((divisor) == 0 ? (__builtin_trap(), 0) : (numerator) / (divisor))
#else
#define M68_EFFECT_DIVIDE(numerator, divisor) ((numerator) / (divisor))
#endif
#ifndef XENO_PC_PORT
#define M68_EFFECT_RESULT_DECL register s32 result asm("$2") = -1
#else
#define M68_EFFECT_RESULT_DECL s32 result = -1
#endif

/* Matched C. Pin the live result to v0 only for the MIPS matching build; the
 * host uses the same control flow without a target-specific register name. */
s32 func_800A3514(s16 phase, s16 divisor, s16 base) {
    M68_EFFECT_RESULT_DECL;
    s16 value = (s16)(base + M68_EFFECT_DIVIDE(phase, divisor));
    if (value < 0x21) {
        result = value;
    }
    return result;
}
#undef M68_EFFECT_DIVIDE
#undef M68_EFFECT_RESULT_DECL
/* Shared by the matching build and the port (matched C). */
s16 func_800A3578(s16 arg0, s16 arg1, s32 arg2) {
    return (s16) (arg2 - (arg0 / arg1));
}
#ifndef XENO_PC_PORT


#endif
/* Shared by the matching build and the port (matched C). */
s16 func_800A35C8(s16 arg0, s16 arg1, s16 arg2) {
    s16 temp_v0;
    s16 var_v1;

    temp_v0 = 0x20 - (arg0 / arg1);
    var_v1 = temp_v0;
    if (temp_v0 < arg2) {
        var_v1 = arg2;
    }
    return var_v1;
}
#ifndef XENO_PC_PORT

INCLUDE_ASM("asm/battle/nonmatchings/main68", func_800A3640);
INCLUDE_ASM("asm/battle/nonmatchings/main68", func_800A3E98);
#endif

/* Rebase stored PS1 heap pointers only on the native port. */
#ifdef XENO_PC_PORT
#define M68_EFFECT_HEAP_PTR(p) ((void*)PSX_ADDR((u32)(p)))
#else
#define M68_EFFECT_HEAP_PTR(p) (p)
#endif
void func_800A429C(void *arg0) {
    s32 temp_a0;
    s32 temp_a0_2;
    s32 temp_a1;

    if (*(u16 *)((s8*)(arg0) + 0x1A) != 0) {
        temp_a1 = *(s32 *)((s8*)(arg0) + 4);
        if (temp_a1 != 0) {
            if ((u8) *(u8 *)((s8*)(arg0) + 0x10) < 4U) {
                LoadImage(arg0 + 0x28, M68_EFFECT_HEAP_PTR(temp_a1));
            }
            HeapFree(M68_EFFECT_HEAP_PTR(*(s32 *)((s8*)(arg0) + 4)));
            *(s32 *)((s8*)(arg0) + 4) = 0;
        }
        temp_a0 = *(s32 *)((s8*)(arg0) + 8);
        if (temp_a0 != 0) {
            HeapFree(M68_EFFECT_HEAP_PTR(temp_a0));
            *(s32 *)((s8*)(arg0) + 8) = 0;
        }
        temp_a0_2 = *(s32 *)((s8*)(arg0) + 0xC);
        if (temp_a0_2 != 0) {
            HeapFree(M68_EFFECT_HEAP_PTR(temp_a0_2));
            *(s32 *)((s8*)(arg0) + 0xC) = 0;
        }
        *(u16 *)((s8*)(arg0) + 0x1A) = 0U;
    }
}
#undef M68_EFFECT_HEAP_PTR

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/main68", func_800A4348);
INCLUDE_ASM("asm/battle/nonmatchings/main68", func_800A43F8);
INCLUDE_ASM("asm/battle/nonmatchings/main68", func_800A44C0);
INCLUDE_ASM("asm/battle/nonmatchings/main68", func_800A4654);
INCLUDE_ASM("asm/battle/nonmatchings/main68", func_800A4820);
INCLUDE_ASM("asm/battle/nonmatchings/main68", func_800A48EC);
INCLUDE_ASM("asm/battle/nonmatchings/main68", func_800A4B3C);
INCLUDE_ASM("asm/battle/nonmatchings/main68", func_800A4CF8);
INCLUDE_ASM("asm/battle/nonmatchings/main68", func_800A4DB8);
#endif


#ifndef XENO_PC_PORT
extern u32 D_800D3344;
#endif
#ifndef XENO_PC_PORT
extern u32 D_800D39CC;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3B74;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3D6C;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800C3610;
#endif


/* func_800A577C.s */
u32 func_800A577C(void) {
    return D_800D3344;
}
/* func_800A578C.s */
u32 func_800A578C(void) {
    return D_800D39CC;
}


/* ---- Remaining port bodies (fo/bat3, effect-image / effect-render group).
 * These are compiled only for the port and differential harness. Guest
 * pointers loaded from RAM go through PSX_ADDR, pointers stored into RAM are
 * guest addresses (PsxMemory_GuestAddr), and main-exe / libgpu / libgte
 * callees take host pointers. ---- */
#ifndef XENO_PC_PORT
extern u8 D_800D2FC8[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2FD0[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3E38[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3E48[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3D50[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3EA0[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3DA0[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D3308[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D33E4[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCC5C[];
#endif
extern u8* D_800658C8;
extern s32 D_80050100;
#ifdef XENO_PC_PORT
#include <stdio.h>

extern void MTC2(unsigned int value, int reg);
extern unsigned int MFC2(int reg);
extern int doCOP2(int op);
extern long VectorNormalS(VECTOR* v0, SVECTOR* v1);
extern void* HeapAlloc(u32 size, u32 flags);
extern u_int HeapFree(void* p);
extern void HeapChangeCurrentUser(u32 user, u32 types);
extern void func_80026F44(s32 count, s32 factor, u16* dst, const u16* src);
extern void func_80026FE8(s32 count, s32 factor, u16* dst, const u16* srcA,
                          const u16* srcB);
extern void func_80027D40(void* p);
extern s32 func_800273C4(void* ctx, SVECTOR* eye, SVECTOR* at, MATRIX* mtx,
                         void* ot, s32 renderCtxIndex);
extern s32 func_8002C700(u8* model, u8* buffer, u32* ot, s32 variant);
extern void GfxLineScrollUpdate(void* pLineScroll);
extern void GfxLineScrollFree(void* pLineScroll);
extern void func_800A9FF0(u32 index);
extern void func_800A6AE8(void);
extern s32 func_800AA650(u32 index);
extern void func_800B10EC(u32 index, s32 x, s32 z, s32 y);
void func_800A4DB8(u8* s, SVECTOR* eye, SVECTOR* at, MATRIX* view, u32* ot,
                   s32 idx);

#define M68_GP(a) ((u8*)PSX_ADDR(a))
#define M68_GA(p) PsxMemory_GuestAddr(p)
/* Retail 0x1F800000 (scratchpad). */
#define M68_SCRATCH ((u8*)g_PsxScratchpad)

/* Link a primitive into an ordering-table slot (retail addPrim inline): the
 * OT words hold 24-bit guest addresses. */
static void m68_add_prim(u32* ot, u8* prim) {
    *(u32*)prim = (*(u32*)prim & 0xFF000000u) | (*ot & 0x00FFFFFFu);
    *ot = (*ot & 0xFF000000u) | (M68_GA(prim) & 0x00FFFFFFu);
}

/* The effect record's +0x24 holds a retail code address produced by
 * func_800AA820 (one of the four progression callbacks above).  Retail JALRs
 * to it; the host resolves only those proven entries.  func_800AA820's port
 * body returns host function pointers, so accept that spelling too (the port
 * links non-PIE below 4 GiB, so the 32-bit value is lossless). An unknown
 * address is unresolved execution, never an implied default frame. */
static s32 m68_effect_callback(u32 fn, s32 phase, s32 divisor, s32 base) {
    if (fn == 0x800A3490u || fn == (u32)(uintptr_t)func_800A3490)
        return func_800A3490((s16)phase, (s16)divisor, (s16)base);
    if (fn == 0x800A3514u || fn == (u32)(uintptr_t)func_800A3514)
        return func_800A3514((s16)phase, (s16)divisor, (s16)base);
    if (fn == 0x800A3578u || fn == (u32)(uintptr_t)func_800A3578)
        return func_800A3578((s16)phase, (s16)divisor, (s16)base);
    if (fn == 0x800A35C8u || fn == (u32)(uintptr_t)func_800A35C8)
        return func_800A35C8((s16)phase, (s16)divisor, (s16)base);
    fprintf(stderr, "[battle-effect] unresolved retail callback %08x\n", fn);
    __builtin_trap();
}

/* func_800A4348.s: type-4 frame: scale each +0x4 texel by factor/32 into the
 * +0x1C buffer (row stride 6 bytes, as retail computes it). */
void func_800A4348(u8* e, s32 factor) {
    u8* src = M68_GP(*(u32*)(e + 0x4));
    s32 scale = (s16)factor;
    s32 y;
    s32 x;

    for (y = 0; y < *(s16*)(e + 0x2E); y++) {
        for (x = 0; x < *(s16*)(e + 0x2C); x++) {
            s32 product = (s32)*(u16*)src * scale;
            u32 dst = *(u32*)(e + 0x1C) + (u32)(*(s16*)(e + 0x2A) + y) * 6u
                    + (u32)(*(s16*)(e + 0x28) + x) * 2u;

            *(u16*)M68_GP(dst) = (u16)(product / 32);
            src += 2;
        }
    }
}

/* func_800A43F8.s: type-5 frame: +0x8 texel plus (+0x4 - +0x8) * factor/32
 * into the +0x1C buffer. */
void func_800A43F8(u8* e, s32 factor) {
    u8* src = M68_GP(*(u32*)(e + 0x4));
    u8* base = M68_GP(*(u32*)(e + 0x8));
    s32 scale = (s16)factor;
    s32 y;
    s32 x;

    for (y = 0; y < *(s16*)(e + 0x2E); y++) {
        for (x = 0; x < *(s16*)(e + 0x2C); x++) {
            s32 product = ((s32)*(u16*)src - (s32)*(u16*)base) * scale;
            u32 dst = *(u32*)(e + 0x1C) + (u32)(*(s16*)(e + 0x2A) + y) * 6u
                    + (u32)(*(s16*)(e + 0x28) + x) * 2u;

            src += 2;
            *(u16*)M68_GP(dst) = (u16)(*(u16*)base + product / 32);
            base += 2;
        }
    }
}

/* func_800A3640.s: construct an effect-image record (19 arguments; the
 * battle twin of the field's 801E0A00).  Returns NULL when the record is
 * already active, else the record.  `parent` and `source` are stored as guest
 * addresses (+0x0 / +0x1C); `callback` is a retail code address (+0x24).
 * Retail reads the caller's $s1 as the running colour of the type-4/5 stripe
 * fill when (outY + row) % 3 is negative; the host starts it at 0. */
u8* func_800A3640(u8* e, u8* parent, s32 typeArg, s32 flagsArg, u8* source,
                  s32 ax, s32 ay, s32 az, s32 bx, s32 by, s32 bz,
                  s32 outX, s32 outY, s32 widthArg, s32 heightArg,
                  s32 period, s32 param0, s32 param1, u32 callback) {
    u16 type = (u16)typeArg;
    u16 flags = (u16)flagsArg;
    u16 values[2][3];
    u16 width = (u16)widthArg;
    u16 height = (u16)heightArg;
    u16 carry = 0;
    u32 plane;

    values[0][0] = (u16)ax; values[0][1] = (u16)ay; values[0][2] = (u16)az;
    values[1][0] = (u16)bx; values[1][1] = (u16)by; values[1][2] = (u16)bz;
    if (*(u16*)(e + 0x1A) != 0) {
        return NULL;
    }
    HeapChangeCurrentUser(4, 0);
    *(u16*)(e + 0x1A) = 1;
    e[0x11] = 0;
    e[0x10] = (u8)type;
    *(u32*)(e + 0x0) = M68_GA(parent);
    *(u16*)(e + 0x14) = 0;
    *(u16*)(e + 0x18) = 0xFFFF;
    *(u16*)(e + 0x16) = (u16)period;
    *(u16*)(e + 0x20) = (u16)param0;
    *(u16*)(e + 0x22) = (u16)param1;
    *(u32*)(e + 0x24) = callback;
    *(u32*)(e + 0x1C) = M68_GA(source);
    if (!(type & 1)) {
        flags = (u16)flagsArg & 0xFD0F;
    }
    if (type & 4) {
        flags &= 0xFEFF;
    }
    if (width == 0) {
        width = 0x100;
    }
    if (height == 0) {
        height = 0x100;
    }
    if (type >= 6 || type == 2 || type == 3) {
        return e;
    }
    if (type < 2) {
        s32 half = ((s32)(s16)width + 1) / 2;

        width = (u16)(half * 2);
    }
    *(u16*)(e + 0x28) = (u16)outX;
    *(u16*)(e + 0x2A) = (u16)outY;
    *(u16*)(e + 0x2C) = width;
    *(u16*)(e + 0x2E) = height;
    *(u16*)(e + 0x12) = (u16)((u32)width * height);
    {
        u32 size = ((u32)(s32)(s16)width * (u32)(s32)(s16)height) << 1;

        if (flags & 0x100) {
            *(u32*)(e + 0xC) = M68_GA(HeapAlloc(size, 0));
        }
        if (flags & 0x200) {
            *(u32*)(e + 0x8) = M68_GA(HeapAlloc(size, 0));
        }
        if (flags & 0x400) {
            *(u32*)(e + 0x4) = M68_GA(HeapAlloc(size, 0));
        }
    }
    for (plane = 0; plane < 2; plane++) {
        u32 mode = (flags >> (plane * 4)) & 0xF;
        u32 slot = 4 + plane * 4;

        if (type < 2) {
            if (mode == 1) {
                RECT16 rect;

                rect.x = (s16)values[plane][0];
                rect.y = (s16)values[plane][1];
                rect.w = (s16)width;
                rect.h = (s16)height;
                StoreImage(&rect, (u_long*)M68_GP(*(u32*)(e + slot)));
                DrawSync(0);
            } else if (mode == 2) {
                u16 color = (u16)(((values[plane][2] & 0x3F) << 10)
                                  + ((values[plane][1] & 0x1F) << 5)
                                  | (values[plane][0] & 0x1F));
                s32 count = (s32)((u32)(s32)(s16)width * (u32)(s32)(s16)height);
                s32 i;

                for (i = 0; i < count; i++) {
                    *(u16*)M68_GP(*(u32*)(e + slot) + (u32)i * 2) = color;
                }
            }
        } else if (mode == 1 || mode == 2) {
            s32 row;
            s32 column;

            for (row = 0; row < (s16)height; row++) {
                s32 component = ((s32)(s16)outY + row) % 3;

                for (column = 0; column < (s16)width; column++) {
                    u16 value;

                    if (mode == 1) {
                        u32 off = ((u32)(s32)(s16)values[plane][1] + (u32)row) * 6u
                                + ((u32)(s32)(s16)values[plane][0] + (u32)column) * 2u;

                        value = *(u16*)M68_GP(M68_GA(source) + off);
                    } else {
                        if (component >= 0 && component < 3) {
                            carry = values[plane][component];
                        }
                        value = carry;
                    }
                    *(u16*)M68_GP(*(u32*)(e + slot)
                                  + ((u32)row * (u32)(s32)(s16)width + (u32)column) * 2u)
                        = value;
                }
            }
        }
    }
    return e;
}

/* func_800A3E98.s: advance an effect-image record by ticks: step the +0x14
 * phase, ask the +0x24 callback for the frame (negative = finished: release
 * and return it), rebuild the image for a new frame by type, then blit the
 * overlap into the linked record at +0x0.  Returns the (s16) frame. */
s32 func_800A3E98(u8* e, s32 ticks) {
    s32 frame;
    u16 raw;
    u8* linked;
    s16 dstOff[2];
    s16 srcOff[2];
    s16 extent[2];
    u32 axis;
    s32 y;
    s32 x;

    if (*(u16*)(e + 0x1A) == 0) {
        return -1;
    }
    *(u16*)(e + 0x14) = (u16)(*(u16*)(e + 0x14) + *(u16*)(e + 0x16) * (u32)(ticks + 1));
    raw = (u16)m68_effect_callback(*(u32*)(e + 0x24), *(u16*)(e + 0x14),
                                   *(s16*)(e + 0x20), *(s16*)(e + 0x22));
    frame = (s16)raw;
    if (frame < 0) {
        func_800A429C(e);
        return frame;
    }
    if (frame == *(u16*)(e + 0x18)) {
        return frame;
    }
    *(u16*)(e + 0x18) = raw;
    switch (e[0x10]) {
    case 0:
        func_80026F44(*(s16*)(e + 0x12), frame, (u16*)M68_GP(*(u32*)(e + 0xC)),
                      (const u16*)M68_GP(*(u32*)(e + 0x4)));
        if (*(u32*)(e + 0x0) == 0) {
            LoadImage((RECT16*)(e + 0x28), (u_long*)M68_GP(*(u32*)(e + 0xC)));
        }
        break;
    case 1:
        func_80026FE8(*(s16*)(e + 0x12), frame, (u16*)M68_GP(*(u32*)(e + 0xC)),
                      (const u16*)M68_GP(*(u32*)(e + 0x8)),
                      (const u16*)M68_GP(*(u32*)(e + 0x4)));
        if (*(u32*)(e + 0x0) == 0) {
            LoadImage((RECT16*)(e + 0x28), (u_long*)M68_GP(*(u32*)(e + 0xC)));
        }
        break;
    case 4:
        func_800A4348(e, frame);
        break;
    case 5:
        func_800A43F8(e, frame);
        break;
    }
    if (*(u32*)(e + 0x0) == 0) {
        return frame;
    }
    linked = M68_GP(*(u32*)(e + 0x0));
    if (*(u16*)(linked + 0x1A) == 0) {
        return frame;
    }
    for (axis = 0; axis < 2; axis++) {
        u32 pos = 0x28 + axis * 2;
        u32 size = pos + 4;

        if (*(s16*)(linked + pos) < *(s16*)(e + pos)) {
            dstOff[axis] = (s16)(*(s16*)(e + pos) - *(s16*)(linked + pos));
            srcOff[axis] = 0;
            extent[axis] = (s16)(*(u16*)(linked + pos) + *(u16*)(linked + size)
                                 - *(u16*)(e + pos));
        } else {
            dstOff[axis] = 0;
            srcOff[axis] = (s16)(*(u16*)(linked + pos) - *(u16*)(e + pos));
            extent[axis] = (s16)(*(u16*)(e + pos) + *(u16*)(e + size)
                                 - *(u16*)(linked + pos));
        }
    }
    if (extent[0] <= 0 || extent[1] <= 0) {
        return frame;
    }
    {
        u8 linkedType = linked[0x10];
        u32 dstBase = *(u32*)(linked + 0x4);

        linked[0x11] = 1;
        for (y = 0; y < extent[1]; y++) {
            for (x = 0; x < extent[0]; x++) {
                u32 dst = dstBase + (u32)dstOff[0] * 2u + (u32)x * 2u
                        + (u32)((dstOff[1] + y) * *(s16*)(linked + 0x2C)) * 2u;
                u32 src;

                if (linkedType < 4) {
                    src = *(u32*)(e + 0xC) + (u32)srcOff[0] * 2u + (u32)x * 2u
                        + (u32)((srcOff[1] + y) * *(s16*)(e + 0x2C)) * 2u;
                } else {
                    src = *(u32*)(e + 0x1C) + (u32)(srcOff[1] + y) * 6u
                        + (u32)(srcOff[0] + x) * 2u;
                }
                *(u16*)M68_GP(dst) = *(u16*)M68_GP(src);
            }
        }
    }
    return frame;
}

/* func_800A44C0.s: for the two anchor records at D_800D3304 (0x14 stride:
 * out xyz, active, local SVECTOR, model slot, bone): when the slot's model is
 * present transform the local point by model->+0xC composed with the bone
 * matrix (MVMVA rt*v0+tr), else copy the local point through. */
void func_800A44C0(u8* models) {
    u32 i;

    for (i = 0; i < 2; i++) {
        u8* r = D_800D3308 - 4 + i * 0x14;
        s32 slot;
        u32 model;

        if (*(s16*)(r + 0x6) == 0) {
            continue;
        }
        slot = *(s16*)(r + 0x10);
        if (slot >= 0 && (model = *(u32*)(models + slot * 4)) != 0) {
            u32 body = *(u32*)(M68_GP(model) + 0x4);
            u8* v = r + 0x8;

            CompMatrix((MATRIX*)M68_GP(body + 0xC),
                       (MATRIX*)M68_GP(body + *(s16*)(r + 0x12) * 0x7C + 0xA8),
                       (MATRIX*)M68_SCRATCH);
            SetRotMatrix((MATRIX*)M68_SCRATCH);
            SetTransMatrix((MATRIX*)M68_SCRATCH);
            MTC2(*(u32*)(v + 0), 0);
            MTC2(*(u32*)(v + 4), 1);
            doCOP2(0x0480012); /* MVMVA sf=1 R*V0+TR */
            *(u16*)(r + 0x0) = (u16)MFC2(25);
            *(u16*)(r + 0x2) = (u16)MFC2(26);
            *(u16*)(r + 0x4) = (u16)MFC2(27);
        } else {
            *(u16*)(r + 0x0) = *(u16*)(r + 0x8);
            *(u16*)(r + 0x2) = *(u16*)(r + 0xA);
            *(u16*)(r + 0x4) = *(u16*)(r + 0xC);
        }
    }
}

/* func_800A48EC.s: draw the model list hdr (count at +0xA, 0x7C entries from
 * +0x7C) through func_8002C700 with each entry's composed matrix; billboard
 * kinds 1/2 replace the rotation, kind 0 draws into the last OT slot with
 * D_80050100 = 0x10.  D_80050100 is restored afterwards.  unused2/unused3
 * are the retail a3 / 5th argument, which the body never reads. */
void func_800A48EC(u8* models, u8* hdr, MATRIX* view, void* unused3,
                   u32 unused4, u8* ot, s32 bufIndex, s32 otCount) {
    MATRIX* m = (MATRIX*)(M68_SCRATCH + 0x40);
    u8* s = (u8*)m;
    s32 saved = D_80050100;
    u32 n = (u32)*(u16*)(hdr + 0xA) - 1;
    u8* entry = hdr + 0x7C;
    u32 i;

    (void)unused3;
    (void)unused4;
    for (i = 0; i < n; i++, entry += 0x7C) {
        u8* f = entry + 8;
        s32 kind;
        s32 variant;
        u8* dstOt;
        u8* model;
        u8* buffer;

        if (*(u16*)f == 0xFFFF || f[-1] == 0) {
            continue;
        }
        CompMatrix(view, (MATRIX*)(entry + 0x2C), m);
        if ((u16)(*(u16*)(f + 0x4A) - 1) < 2) {
            *(s16*)(s + 0x0) = 0x1000;
            *(s16*)(s + 0x4) = 0;
            *(s16*)(s + 0x6) = 0;
            *(s16*)(s + 0xA) = 0;
            *(s16*)(s + 0xC) = 0;
            *(s16*)(s + 0x10) = 0x1000;
            if (*(s16*)(f + 0x4A) == 1) {
                *(u16*)(s + 0x2) = *(u16*)((u8*)view + 0x2);
                *(u16*)(s + 0x8) = *(u16*)((u8*)view + 0x8);
                *(u16*)(s + 0xE) = *(u16*)((u8*)view + 0xE);
            } else {
                *(s16*)(s + 0x2) = 0;
                *(s16*)(s + 0x8) = 0x1000;
                *(s16*)(s + 0xE) = 0;
            }
        }
        SetRotMatrix(m);
        SetTransMatrix(m);
        kind = *(s16*)(f + 0x4A);
        switch (kind) {
        case 4: variant = 2; break;
        case 5: variant = 3; break;
        case 6: variant = 4; break;
        case 7: variant = 5; break;
        default: variant = 0; break;
        }
        buffer = M68_GP(*(u32*)(entry + bufIndex * 4 + 0x68));
        model = M68_GP(*(u32*)(M68_GP(*(u32*)models) + *(u16*)f * 4));
        if (kind == 0) {
            dstOt = ot + otCount * 4 - 4;
            D_80050100 = 0x10;
        } else {
            dstOt = ot;
            D_80050100 = saved;
        }
        func_8002C700(model, buffer, (u32*)dstOt, variant);
    }
    D_80050100 = saved;
}

/* func_800A4654.s: per-frame battle-effect render: light matrix, advance
 * every effect-image record of D_800D33E4, run func_800A6AE8 (retail on the
 * scratchpad stack), line scrolls, the model list, the two D_800C3D50
 * packets and the D_800C3EA0 effect. */
void func_800A4654(MATRIX* view, MATRIX* light, u32 a2, u8* ot, s32 bufIndex,
                   SVECTOR* eye, SVECTOR* at, s32 otCount) {
    u8* owner;
    u8* rec;
    s32 i;

    if (light != NULL) {
        SetLightMatrix(light);
    }
    owner = M68_GP(*(u32*)D_800D33E4);
    rec = M68_GP(*(u32*)(owner + 0x118));
    for (i = 0; i < owner[0x10E]; i++) {
        func_800A3E98(rec, *(u32*)D_800CCC5C);
        owner = M68_GP(*(u32*)D_800D33E4);
        rec += 0x30;
    }
    func_800A6AE8();
    for (i = 0; i < 2; i++) {
        GfxLineScrollUpdate(D_800C3DA0 + i * 0x18);
    }
    if (*(u32*)D_800C3E38 != 0) {
        func_800A48EC(M68_GP(*(u32*)D_800C3E48), M68_GP(*(u32*)D_800C3E38), view,
                      light, a2, ot, bufIndex, otCount);
    }
    for (i = 0; i < 2; i++) {
        u32 packet = *(u32*)(D_800C3D50 + i * 4);

        func_800273C4(packet != 0 ? (void*)M68_GP(packet) : NULL, eye, at, view,
                      ot + otCount * 4 - 4, bufIndex);
    }
    {
        u32 effect = *(u32*)D_800C3EA0;

        func_800A4DB8(effect != 0 ? M68_GP(effect) : NULL, eye, at, view,
                      (u32*)(ot + otCount * 4 - 4), bufIndex);
    }
}

/* func_800A4820.s: tear down the battle effect renderer: free effect slot
 * 0x1F, the D_800658C8 buffer, the two D_800C3D50 packets, D_800C3EA0 and
 * the two line scrolls. */
void func_800A4820(void) {
    s32 i;

    func_800A9FF0(0x1F);
    *(u32*)D_800C3E38 = 0;
    if (D_800658C8 != NULL) {
        HeapFree(D_800658C8);
    }
    D_800658C8 = NULL;
    for (i = 0; i < 2; i++) {
        u32 p = *(u32*)(D_800C3D50 + i * 4);

        if (p != 0) {
            func_80027D40(M68_GP(p));
        }
        *(u32*)(D_800C3D50 + i * 4) = 0;
    }
    if (*(u32*)D_800C3EA0 != 0) {
        HeapFree(M68_GP(*(u32*)D_800C3EA0));
    }
    *(u32*)D_800C3EA0 = 0;
    for (i = 0; i < 2; i++) {
        GfxLineScrollFree(D_800C3DA0 + i * 0x18);
    }
}

/* func_800A4B3C.s: push pos (+0 x, +4 z) out of the first D_800D2FD0 circle
 * (x, z, radius; relative to base) it lies inside.  Returns 1 when moved. */
u32 func_800A4B3C(u8* base, u8* pos) {
    u8* t = M68_GP(*(u32*)D_800D2FD0);
    s32 i;

    for (i = 0; i < *(s16*)D_800D2FC8; i++) {
        u32 cx = (u32)*(u16*)(t + 0) - *(u16*)(base + 0);
        s32 dx = (s32)*(s16*)(pos + 0) - (s32)(s16)cx;
        u32 cz = (u32)*(u16*)(t + 2) - *(u16*)(base + 4);
        s32 dz = (s32)*(s16*)(pos + 4) - (s32)(s16)cz;
        s32 r = *(s16*)(t + 4);
        s32 d;

        t += 6;
        d = SquareRoot0((s32)((u32)dx * (u32)dx + (u32)dz * (u32)dz)) + 1;
        if (d < r) {
            /* d >= 1, so neither retail BREAK (zero / overflow) can fire. */
            *(u16*)(pos + 0) = (u16)(cx + (u32)((s32)((u32)dx * (u32)r) / d));
            *(u16*)(pos + 4) = (u16)(cz + (u32)((s32)((u32)dz * (u32)r) / d));
            return 1;
        }
    }
    return 0;
}

/* func_800A4CF8.s: register every D_800D2FD0 circle (x, z, h) with slot
 * index at func_800AA650(index) + h. */
void func_800A4CF8(u32 index) {
    u8* t = M68_GP(*(u32*)D_800D2FD0);
    s32 i;

    for (i = 0; i < *(s16*)D_800D2FC8; i++) {
        s16 x = *(s16*)(t + 0);
        s16 z = *(s16*)(t + 2);
        s16 h = *(s16*)(t + 4);

        t += 6;
        func_800B10EC(index, x, z, func_800AA650(index) + h);
    }
}

/* func_800A4DB8.s: build and link the battle ground/horizon effect for
 * buffer idx: sky/haze strips placed from projected points in front of the
 * eye, then the 8x8 textured ground grid (9x9 SVECTORs at +0x54) drawn with
 * RTPT+NCLIP back-face rejection and scrolling UVs (+0x0/+0x2 advanced by
 * +0x4/+0x6), then two more strips. */
void func_800A4DB8(u8* s, SVECTOR* eye, SVECTOR* at, MATRIX* view, u32* ot,
                   s32 idx) {
    VECTOR dir;
    SVECTOR n;
    SVECTOR p;
    s16 a[2];      /* sp70: x, y */
    s16 b[2];      /* sp78: x, y */
    u32 sxy;
    s32 t;
    s32 k;
    VECTOR v60;
    SVECTOR r58;
    MATRIX m38;
    MATRIX m18;
    s32 dist;
    s32 ang;
    s32 tilt;
    s32 t0;
    s32 t1;
    s32 half;
    s32 uBase;
    s32 vBase;
    s32 row;
    s32 col;
    s32 prim;
    u8* vtx;

    if (s == NULL) {
        return;
    }
    m68_add_prim(ot, s + 0x3C + idx * 12);

    dir.vx = at->vx - eye->vx;
    dir.vy = 0;
    dir.vz = at->vz - eye->vz;
    VectorNormalS(&dir, &n);
    t = n.vx * *(s16*)(s + 0x18);
    p.vx = (s16)(t / 4096 + (u16)at->vx);
    p.vy = *(s16*)(s + 0x16);
    t = n.vz * *(s16*)(s + 0x18);
    p.vz = (s16)(t / 4096 + (u16)at->vz);
    SetRotMatrix(view);
    SetTransMatrix(view);
    MTC2(*(u32*)&p.vx, 0);
    MTC2(*(u32*)&p.vz, 1);
    doCOP2(0x0180001); /* RTPS */
    sxy = MFC2(14);
    a[1] = (s16)(sxy >> 16);
    a[0] = a[1];
    if (a[1] > 0xF0) {
        a[1] = 0xF0;
    }

    k = *(s16*)(s + 0xE);
    t = (n.vx * *(s16*)(s + 0x18)) / 4096;
    p.vx = (s16)((t * k) / 256 + (u16)at->vx);
    p.vy = (s16)((*(s16*)(s + 0x16) * k) / 256);
    t = (n.vz * *(s16*)(s + 0x18)) / 4096;
    p.vz = (s16)((t * k) / 256 + (u16)at->vz);
    MTC2(*(u32*)&p.vx, 0);
    MTC2(*(u32*)&p.vz, 1);
    doCOP2(0x0180001);
    sxy = MFC2(14);
    b[0] = (s16)sxy;
    b[1] = (s16)(sxy >> 16);
    if (b[1] >= 0) {
        if (a[1] < 0xF0) {
            u8* q = s + (idx + 2) * 36;

            *(u16*)(q + 0x1746) = (u16)a[1];
            *(u16*)(q + 0x174E) = (u16)a[1];
            *(u16*)(q + 0x1756) = (u16)b[1];
            *(u16*)(q + 0x175E) = (u16)b[1];
            m68_add_prim(ot, s + 0x1784 + idx * 36);
        }
    } else {
        b[1] = 0;
    }
    if (b[1] < 0xF0) {
        u8* q = s + (idx + 2) * 24;

        *(u16*)(q + 0x16E6) = (u16)b[1];
        *(u16*)(q + 0x16EA) = (u16)b[1];
        m68_add_prim(ot, s + 0x170C + idx * 24);
    }

    k = *(s16*)(s + 0xC);
    t = (n.vx * *(s16*)(s + 0x18)) / 4096;
    p.vx = (s16)((t * k) / 256 + (u16)at->vx);
    p.vy = (s16)((*(s16*)(s + 0x16) * k) / 256);
    t = (n.vz * *(s16*)(s + 0x18)) / 4096;
    p.vz = (s16)((t * k) / 256 + (u16)at->vz);
    MTC2(*(u32*)&p.vx, 0);
    MTC2(*(u32*)&p.vz, 1);
    doCOP2(0x0180001);
    sxy = MFC2(14);
    b[0] = (s16)sxy;
    b[1] = (s16)(a[1] * 2 - (u16)(sxy >> 16) - 8);
    if ((u16)a[0] < 0x1E0 && b[1] < 0xF0) {
        u8* q = s + idx * 36;

        *(u16*)(q + 0x1746) = (u16)a[0];
        *(u16*)(q + 0x174E) = (u16)a[0];
        *(u16*)(q + 0x1756) = (u16)b[1];
        *(u16*)(q + 0x175E) = (u16)b[1];
        m68_add_prim(ot, s + 0x173C + idx * 36);
    }

    v60.vx = at->vx - eye->vx;
    v60.vy = 0;
    v60.vz = at->vz - eye->vz;
    dist = SquareRoot0((s32)((u32)v60.vx * (u32)v60.vx + (u32)v60.vz * (u32)v60.vz));
    ang = ratan2(at->vy - eye->vy, dist);
    t = ((ang - 0x100) * *(s16*)(s + 0x12)) / 512;
    tilt = (t * (0x400 - (ang < 0 ? -ang : ang))) / 1024;
    SetGeomScreen(*(s16*)(s + 0x10));
    r58.vx = 0;
    r58.vy = (s16)-ratan2(v60.vx, v60.vz);
    r58.vz = 0;
    RotMatrix(&r58, &m38);
    m38.t[0] = 0;
    m38.t[1] = 0;
    m38.t[2] = 0;
    r58.vx = (s16)tilt;
    r58.vy = (s16)-(u16)r58.vy;
    RotMatrixYXZ(&r58, &m18);
    VectorNormalS(&v60, &r58);
    v60.vx = eye->vx + r58.vx * 2;
    v60.vy = eye->vy / 4 - *(s16*)(s + 0xA);
    v60.vz = eye->vz + r58.vz * 2;
    m18.t[0] = v60.vx;
    m18.t[1] = v60.vy;
    m18.t[2] = v60.vz;
    CompMatrix(&m18, &m38, &m18);
    CompMatrix(view, &m18, &m18);
    SetRotMatrix(&m18);
    SetTransMatrix(&m18);

    *(u16*)(s + 0x0) = (u16)(*(u16*)(s + 0x0) + *(u16*)(s + 0x4));
    *(u16*)(s + 0x2) = (u16)(*(u16*)(s + 0x2) + *(u16*)(s + 0x6));
    t1 = (s32)*(u16*)(s + 0x8) - 1;
    t0 = (*(s16*)(s + 0x0) / 16 + v60.vx / 12) & t1;
    half = *(s16*)(s + 0x8) / 2;
    vBase = (*(s16*)(s + 0x2) / 16 + v60.vz / 12) & t1;
    uBase = t0;
    vtx = s + 0x54;
    prim = idx * 64;
    for (row = 0; row < 8; row++) {
        s32 v0 = (row & 1) * half + vBase;
        s32 v1 = v0 + half - 1;

        for (col = 0; col < 8; col++, prim++, vtx += 8) {
            u8* pr = s + 0x2DC + prim * 40;
            s32 u0;

            MTC2(*(u32*)(vtx + 0x00), 0);
            MTC2(*(u32*)(vtx + 0x04), 1);
            MTC2(*(u32*)(vtx + 0x08), 2);
            MTC2(*(u32*)(vtx + 0x0C), 3);
            MTC2(*(u32*)(vtx + 0x48), 4);
            MTC2(*(u32*)(vtx + 0x4C), 5);
            doCOP2(0x0280030); /* RTPT */
            doCOP2(0x1400006); /* NCLIP */
            if ((s32)MFC2(24) < 0) {
                continue;
            }
            *(u32*)(pr + 0x08) = MFC2(12);
            *(u32*)(pr + 0x10) = MFC2(13);
            *(u32*)(pr + 0x18) = MFC2(14);
            MTC2(*(u32*)(vtx + 0x50), 0);
            MTC2(*(u32*)(vtx + 0x54), 1);
            doCOP2(0x0180001);
            *(u32*)(pr + 0x20) = MFC2(14);
            u0 = (col & 1) * half + uBase;
            pr[0x0D] = (u8)v0;
            pr[0x15] = (u8)v0;
            pr[0x1D] = (u8)v1;
            pr[0x25] = (u8)v1;
            pr[0x0C] = (u8)u0;
            pr[0x1C] = (u8)u0;
            pr[0x14] = (u8)(u0 + half - 1);
            pr[0x24] = (u8)(u0 + half - 1);
            m68_add_prim(ot, pr);
        }
        vtx += 8;
    }
    SetGeomScreen(0x200);
    if (a[1] >= 0) {
        u8* q = s + idx * 24;

        *(u16*)(q + 0x16EE) = (u16)a[1];
        *(u16*)(q + 0x16F2) = (u16)a[1];
        m68_add_prim(ot, s + 0x16DC + idx * 24);
    }
    m68_add_prim(ot, s + 0x24 + idx * 12);
}
#endif /* XENO_PC_PORT */
