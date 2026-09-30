#include "common.h"
#ifndef BATTLE_TU_SUB
#define BATTLE_TU_SUB 0
#endif
/* Selects one run of byte-exact C bodies when a retail-asm gap splits them;
 * 0 (host/port/tests) compiles every run. */
#define BATTLE_SUB(n) (BATTLE_TU_SUB == 0 || BATTLE_TU_SUB == (n))
/* Retail TU split: gcc 2.7.2 cc1 emits every file-scope INCLUDE_ASM block
 * before the first compiled body, so a mixed TU whose retail layout has a C
 * function ahead of an assembly function cannot be one object.  Each
 * contiguous [asm run][C run] of the retail layout is therefore its own
 * matching TU: src/battle/main35_p<N>.c defines BATTLE_TU_PART=<N> and
 * includes this file.  Host/port builds compile this file whole (part 0). */
#ifndef BATTLE_TU_PART
#define BATTLE_TU_PART 0
#endif
#define BATTLE_PART(n) (BATTLE_TU_PART == 0 || BATTLE_TU_PART == (n))
#ifdef XENO_PC_PORT
#include <stdint.h>
#endif


#if BATTLE_PART(1)
#endif /* BATTLE_PART(1) */
/* Retail actor stride and the fields read by this handler. Unknown bytes
 * remain opaque; this view does not describe the context allocation size. */
typedef struct {
    u8 pad26[0x26];
    u16 right;
    u16 pad28;
    u16 up_alt;
    u16 pad2c;
    u16 pad2e;
    u16 up;
    u16 down;
    u8 pad34[8];
    u8 target;
    u8 pad3d[3];
} BattleCommandActor;
struct BattleCommandContext {
    /* GCC's zero-length tail keeps the retail 64-byte row addressing without
     * inventing an allocation extent; callers own the valid actor range. */
    BattleCommandActor rows[0];
};
STATIC_ASSERT_SIZEOF(BattleCommandActor, 0x40);
#ifndef XENO_PC_PORT
extern struct BattleCommandContext *D_800C3EAC;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D3014, D_800D366C, D_800C3E29;
#endif
extern void func_80087A38(u32);
extern void func_80084A7C(u32);
extern void func_80077698(void);
extern void func_8008AA74(u32);

#if BATTLE_PART(1)
/* Retail 8008115C..80081314: state-1 (Attack) command ring. Event5 is
 * deliberately a no-op; 4/6/7 share acceptance. Reload the context after
 * calls, as retail does, rather than retaining a possibly replaced owner. */
void func_8008115C(u32 arg0) {
    u32 actor;
    u8 *row;
    switch (D_800D3014) {
    case 4:
    case 6:
    case 7:
        actor = arg0 & 0xFF;
        if (D_800C3EAC->rows[actor].target == 0xFF) {
            arg0 = 0x4F;
            goto reject;
        }
        D_800D366C = 0;
        func_80087A38(actor);
        func_80084A7C(actor);
        func_80077698();
        ((u8 *)D_800C3EAC)[0x2DD] = 5;
        return;
    case 1:
        if (D_800C3EAC->rows[arg0 & 0xFF].down) {
            arg0 = 0x4F;
            goto reject;
        }
        ((u8 *)D_800C3EAC)[0x2DD] = 2;
        return;
    case 2:
        ((u8 *)D_800C3EAC)[0x2DD] = 3;
        return;
    case 3:
        row = (u8 *)D_800C3EAC + (arg0 & 0xFF) * 64;
        if (!*(u16 *)(row + 0x30)) {
            ((u8 *)D_800C3EAC)[0x2DD] = 4;
            return;
        }
        if (((u8 *)D_800C3EAC)[0x2F6] && D_800C3E29 == 3) {
            if (*(u16 *)(row + 0x2A)) func_8008AA74(0x4F);
            else ((u8 *)D_800C3EAC)[0x2DD] = 10;
            ((u8 *)D_800C3EAC)[0x2F6] = 0;
            return;
        }
        arg0 = 0x4F;
        ((u8 *)D_800C3EAC)[0x2F6] = 1;
        goto reject;
    case 0:
        if (D_800C3EAC->rows[arg0 & 0xFF].right) {
            arg0 = 0x4F;
            goto reject;
        }
        ((u8 *)D_800C3EAC)[0x2DD] = 7;
        return;
    default:
        return;
    }
reject:
    func_8008AA74(arg0);
}
#endif /* BATTLE_PART(1) */
#if BATTLE_PART(2)
#endif /* BATTLE_PART(2) */
#ifndef XENO_PC_PORT
extern u8 D_800D2DCC[], D_800C3EB7[][28], D_800D32A1[][8];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3EB4[][28], *D_800D3364;
#endif
#ifndef XENO_PC_PORT
extern u16 D_800CCD64[][184], D_800CCE08[][184];
#endif
#if BATTLE_PART(2)
/* func_80083FF4.s: alternate status bypasses the group matrix entirely.
 * Keep the early exits so no irrelevant matrix pointer is dereferenced. */
#include "target_eligibility_impl.inc"
/* Retail 84108: flag low byte bypasses status only, not presence/block. */
u32 func_80084108(u32 arg0, u32 arg1) {
    u8 result = 0;
    u32 target = arg0 & 0xFF;
    if (D_800D2DCC[target] != 0 && D_800C3EB7[target][0] == 0) {
        if (D_800D32A1[target][0] == 0) {
            result = 1;
            if ((arg1 & 0xFF) == 0)
                result = (D_800CCD64[target][0] & 0xC001) == 0;
        } else {
            if ((arg1 & 0xFF) != 0 || (D_800CCE08[target][0] & 0xC001) == 0)
                result = 1;
        }
    }
    return result;
}
#endif /* BATTLE_PART(2) */
#ifndef XENO_PC_PORT
extern u8 D_800D3274;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3E90[], D_800C3EB4[][28];
#endif
#ifndef XENO_PC_PORT
extern u16 D_800CCD34[][184];
#endif
extern u32 func_80083FF4(u32, u32);
#if BATTLE_PART(2)
/* func_800841E0.s: stable group partition followed by slot-zero swaps,
 * not a full sort. Rank is an unsigned halfword at stride0x170. */
/* Not yet byte-matching: the matching build assembles retail bytes for
 * this function from its own asm segment (see config/battle.yaml).  The
 * body below is kept for the port and the differential harnesses. */
#ifdef XENO_PC_PORT
#include "target_list_impl.inc"
#endif /* XENO_PC_PORT */
#endif /* BATTLE_PART(2) */
#if BATTLE_PART(3)
#endif /* BATTLE_PART(3) */
#ifndef XENO_PC_PORT
extern u16 D_800C3D64;
#endif
extern u16 func_80089C08(u8);
#if BATTLE_PART(3)
/* Retail 84750: actor-filtered range, table mask, no group/rank sorting. */
/* Not yet byte-matching: the matching build assembles retail bytes for
 * this function from its own asm segment (see config/battle.yaml).  The
 * body below is kept for the port and the differential harnesses. */
#ifdef XENO_PC_PORT
u32 func_80084750(u32 arg0) {
    s32 remaining = 8;
    s32 fill = 0xFF;
    s32 i = 11;
    s32 output;
    /* Retail decrements once after the final store. Keep that unused address
     * as an integer, rather than forming a pointer before the host array. */
    uintptr_t clear = (uintptr_t)&D_800C3E90[11];
    for (; i >= 0; i--) {
        *(u8 *)clear = fill;
        clear--;
    }
    output = 0;
    remaining--;
    D_800D3274 = 0;
    D_800C3D64 = 0;
    for (i = 3; remaining >= 0; remaining--) {
        u8 target = i;
        u32 eligible = func_80083FF4(arg0 & 0xFF, target);
        if (eligible & 0xFF) {
            u32 mask;
            D_800C3E90[output] = target;
            mask = func_80089C08(target);
            output++;
            D_800C3D64 |= mask;
            D_800D3274++;
        }
        i++;
    }
    return D_800C3E90[0];
}
#endif /* XENO_PC_PORT */
#endif /* BATTLE_PART(3) */
typedef struct {
    u16 v;
    u8 pad[26];
} Row310;
#ifndef XENO_PC_PORT
extern Row310 D_800C3EBE[], D_800C3EC0[];
#endif
#ifdef XENO_PC_PORT
extern int ratan2(int, int); /* PsyCross's native API uses 32-bit int. */
#else
extern long ratan2(long, long);
#endif
#if BATTLE_PART(3)
/* Retail84854: eleven slots, angular sector, then signed wrapped distance. */
/* Not yet byte-matching: the matching build assembles retail bytes for
 * this function from its own asm segment (see config/battle.yaml).  The
 * body below is kept for the port and the differential harnesses. */
#ifdef XENO_PC_PORT
u32 func_80084854(u32 arg0, u32 arg1) {
    s32 best = 0xFFFFFF;
    u32 selected = arg0;
    u32 actor = arg0 & 0xFF;
    u32 actor_offset = actor * sizeof(Row310);
    s32 direction = arg1 & 0xFF;
    u8 *entry = D_800C3E90;
    do {
        if (*entry != 0xFF && *entry != actor) {
            s32 accepted = 0;
            u32 angle = (u32)ratan2(
                (s32)D_800C3EC0[*entry].v - ((Row310 *)((u8 *)D_800C3EC0 + actor_offset))->v,
                (s32)D_800C3EBE[*entry].v - ((Row310 *)((u8 *)D_800C3EBE + actor_offset))->v);
            switch (direction) {
            case 0:
                if ((u16)(angle + 0x200) < 0x400) accepted = 1;
                break;
            case 1:
                if ((u16)(angle + 0x600) < 0x400) accepted = 1;
                break;
            case 2:
                if ((u16)(angle + 0x800) < 0x200) accepted = 1;
                if ((u16)(angle - 0x600) <= 0x200) accepted = 1;
                break;
            case 3:
                if ((u16)(angle - 0x200) < 0x400) accepted = 1;
                break;
            }
            if (accepted) {
                s32 delta = (s32)D_800C3EC0[*entry].v - ((Row310 *)((u8 *)D_800C3EC0 + actor_offset))->v;
                u32 distance;
                /* Unsigned products and sum retain low32 bits without UB. */
                if (delta < 0) {
                    s32 reverse = ((Row310 *)((u8 *)D_800C3EC0 + actor_offset))->v - (s32)D_800C3EC0[*entry].v;
                    distance = (u32)reverse * (u32)reverse;
                } else {
                    distance = (u32)delta * (u32)delta;
                }
                delta = (s32)D_800C3EBE[*entry].v - ((Row310 *)((u8 *)D_800C3EBE + actor_offset))->v;
                if (delta < 0) {
                    s32 reverse = ((Row310 *)((u8 *)D_800C3EBE + actor_offset))->v - (s32)D_800C3EBE[*entry].v;
                    distance += (u32)reverse * (u32)reverse;
                } else {
                    distance += (u32)delta * (u32)delta;
                }
                /* PSX and supported host compilers interpret these bits as s32. */
                if ((s32)distance < best) {
                    best = (s32)distance;
                    selected = *entry;
                }
            }
        }
        entry++;
    } while ((intptr_t)entry < (intptr_t)&D_800C3E90[11]);
    return selected & 0xFF;
}
#endif /* XENO_PC_PORT */
#endif /* BATTLE_PART(3) */
#ifndef XENO_PC_PORT
extern u8 D_800D3274;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3E90[];
#endif
#if BATTLE_PART(3)
extern u32 func_800841E0(u32 arg0);
/* func_80084A7C.s: list-building may replace the context or actor target.
 * A successful search preserves the pre-call selection; even an empty
 * list falls back to entry zero when there is no match. */
#if BATTLE_SUB(2)
#include "target_selection_impl.inc"
#endif /* BATTLE_SUB(2) */
#endif /* BATTLE_PART(3) */
extern void func_800BC404(u32);
extern void func_800BCD98(u32);
extern void func_800716D8(void);
#if BATTLE_PART(3)
/* Not yet byte-matching: the matching build assembles retail bytes for
 * this function from its own asm segment (see config/battle.yaml).  The
 * body below is kept for the port and the differential harnesses. */
#ifdef XENO_PC_PORT
#include "target_ui_impl.inc"
#endif /* XENO_PC_PORT */
#endif /* BATTLE_PART(3) */
#if BATTLE_PART(4)
#endif /* BATTLE_PART(4) */


#if BATTLE_PART(4)
/* func_80085310.s */
#ifndef XENO_PC_PORT
extern u8 D_800D2D5C[];
#endif
#ifndef XENO_PC_PORT
extern u16 D_800D2D70[];
#endif
s32 func_80085310(u32 a0, u32 a1) {
    return (u32)D_800C3EBE[a0 & 0xFF].v > (u32)D_800C3EBE[a1 & 0xFF].v;
}
/* func_80085350.s */
void func_80085350(void) {
    s32 i = 0;
    u8 v = 0xFF;
    u16* p = D_800D2D70;

    for (; i < 0xB; i++) {
        D_800D2D5C[i] = v;
        *p = 0;
        p++;
    }
}
#endif /* BATTLE_PART(4) */
