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
/* Not yet byte-matching: the matching build assembles retail bytes for
 * this function from its own asm segment (see config/battle.yaml).  The
 * body below is kept for the port and the differential harnesses. */
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
