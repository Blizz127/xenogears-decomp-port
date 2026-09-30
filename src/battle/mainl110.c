#include "common.h"
#ifndef XENO_PC_PORT
#include "system/memory.h"
#endif


#ifndef XENO_PC_PORT
void func_800BA59C(void *arg0, s16 arg1);
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/mainl110", func_800B9C00);
INCLUDE_ASM("asm/battle/nonmatchings/mainl110", func_800B9C78);
INCLUDE_ASM("asm/battle/nonmatchings/mainl110", func_800B9F78);
INCLUDE_ASM("asm/battle/nonmatchings/mainl110", func_800BA4E0);

#ifndef XENO_PC_PORT
void func_800BA59C(void *arg0, s16 arg1) {
    s32 temp_s0;

    *(s16 *)((s8*)(arg0) + 0x32) = arg1;
    temp_s0 = (s32) *(s32 *)((s8*)(arg0) + 0x18) >> 3;
    *(s32 *)((s8*)(arg0) + 0xC) = (s32) ((s32) ((rsin(arg1) >> 1) * temp_s0) >> 8);
    *(s32 *)((s8*)(arg0) + 0x14) = (s32) ((s32) -((rcos(*(s16 *)((s8*)(arg0) + 0x32)) >> 1) * temp_s0) >> 8);
}
#endif

INCLUDE_ASM("asm/battle/nonmatchings/mainl110", func_800BA614);
INCLUDE_ASM("asm/battle/nonmatchings/mainl110", func_800BA768);
#endif


typedef struct { long vx, vy; long vz, pad; } VECTOR;
typedef struct { short vx, vy; short vz, pad; } SVECTOR;
extern s32 func_800A5914(SVECTOR* v, u32 arg1, s32 arg2);
extern s32 func_800A579C(SVECTOR* v);
extern void func_800A5870(SVECTOR* v, s32 r, VECTOR* out);


/* func_800BA8F4.s */
void func_800BA8F4(u8* s) {
    SVECTOR v;
    VECTOR out;
    s32 r;
    s32 t;

    t = *(s16*)(s + 2);
    *(s16*)((u8*)&v + 0) = (s16)t;
    t = *(s16*)(s + 6);
    *(s16*)((u8*)&v + 2) = (s16)t;
    t = *(s16*)(s + 0xA);
    *(s16*)((u8*)&v + 4) = (s16)t;
    r = func_800A5914(&v, *(u32*)(s + 0x78), 4);
    if (r < 0) {
        r = func_800A579C(&v);
    }
    func_800A5870(&v, r, &out);
    *(u16*)(s + 0x84) = *(u16*)((u8*)&v + 2);
    *(u32*)(s + 0x78) = (u32)r;
}

#ifdef XENO_PC_PORT
#include "battle_host_guest.h"
/* Native main-executable globals and routines (not in guest RAM). */
extern u8 D_800591AC;
extern s32 D_80059464;
extern void func_800245D8(void* pSpriteData, s16 animIndex);
extern void func_80021B04(void* arg0, s16 arg1, s16 arg2, s16 arg3);
extern void func_80021BF8(void* arg0, s32 arg1);
extern void func_80022B2C(u8* pSpriteData);
extern void* Square0(void* in, void* out);
extern int SquareRoot0(int a);
extern int ratan2(int y, int x);
extern int rsin(int a);
extern int rcos(int a);
/* Battle overlay callees. */
extern void func_800BF0B4(u32 value);
extern void func_800B9B54(u8* a, u8* b);
extern void func_800B136C(void);
extern void func_800B8D7C(void);
extern void func_800BFBA0(void);
extern void func_800BF2B8(u8* task);
extern void func_800BC454(u16 v);
extern void func_800BF9EC(void);
extern void func_800BEFF4(u32 arg0);
extern void func_800BF3E8(u8* rec);
extern void func_800B8354(void);
extern void func_800BEE2C(u32 a0, u32 a1, u32 a2);
extern void func_800AA760(u32 index, u8 value);
extern void func_800AA79C(u32 arg0, u32 arg1);
extern void func_800B9284(u8* rec, u32 kind);
extern void func_800B8048(u32 v);
extern u32 func_800B7E94(void);
extern void func_800BF4F0(u8* a, u8* b);
extern void func_800BE790(void);
extern void func_800BAEB8(s32 a0);
extern u32 func_800BF720(void);
extern void func_800B8EBC(void);
extern void func_800BFA9C(void);
extern void func_800B905C(void);
extern s32 func_800C07CC(u32 a, u32 b);
void func_800B9C00(u8* rec, u8* other);
void func_800B9C78(void);
void func_800BA59C(u8* rec, u32 angle);

/* Slot index packed into a record's +0xA8 (bits 30-31) and +0xAC (bits 0-1). */
static u32 bat4_l110_slot(u32 rec) {
    return ((BG_U32(rec + 0xAC) & 3) << 2) | (BG_U32(rec + 0xA8) >> 30);
}

/* func_800B9C00.s: snap the record's 16.16 x/z (+0/+8) to its target
 * (+0xA0/+0xA4), flag the current command (D_800C3610 +0x48), step the
 * battle phase (func_800BF0B4(4)), play the +0xB0 pose and turn the pair to
 * face each other (func_800B9B54). */
void func_800B9C00(u8* rec, u8* other) {
    s32 x = *(s16*)(rec + 0xA0);
    s32 z = *(s16*)(rec + 0xA4);

    *(u32*)(rec + 0) = (u32)x << 16;
    *(u32*)(rec + 8) = (u32)z << 16;
    BG_U8(BG_U32(0x800C3610) /* D_800C3610 */ + 0x48) = 1;
    func_800BF0B4(4);
    func_800245D8(rec, *(s8*)(rec + 0xB0));
    func_800B9B54(rec, other);
}

/* func_800B9C78.s: command-step driver.  Counts down the D_800C3614 delay;
 * otherwise refreshes the target chain, reads the current command step's
 * kind (D_800C3EB0 + D_800C360C * 0x48 + 0x17F) and dispatches it through
 * jtbl_80070A80 (kinds 0xF3..0xFF), advancing D_800C360C. */
void func_800B9C78(void) {
    u32 base = 0x800C3EB0; /* D_800C3EB0 */
    u32 p;
    u32 idx;
    u32 s1;
    u32 s2;
    u32 s3;
    u32 a1;

    if (BG_S16(0x800C3614) != 0) { /* D_800C3614 */
        BG_U16(0x800C3614) = (u16)(BG_U16(0x800C3614) - 1);
        return;
    }
    func_800BF9EC();
    idx = BG_U32(0x800C360C); /* D_800C360C */
    func_800BEFF4(BG_U8(base + idx * 0x48 + 0x15B));
    func_800BF3E8(BG_PTR(BG_U32(BG_U32(0x800C3610) /* D_800C3610 */ + 4)));
    p = BG_U32(0x800C3610);
    idx = BG_U32(0x800C360C);
    s2 = BG_U32(p + 0x24);
    s1 = BG_U32(base + s2 * 4 + 0x8C8C);
    s3 = BG_U8(base + idx * 0x48 + 0x17F);
    func_800BF0B4(8);
    p = BG_U32(0x800C3610);
    if (BG_U8(p + 0x4A) != 0) {
        BG_U8(base + 0xA73) = 0;
    }
    BG_U8(p + 0x4A) = 0;
    switch (s3 - 0xF3) {
    case 12:
        func_800BF0B4(9);
        return;
    case 11:
        func_800B8354();
        p = BG_U32(0x800C3610);
        idx = BG_U32(0x800C360C);
        func_800BEE2C(BG_U32(p + 0x20), BG_U16(base + idx * 0x48 + 0x14E), 4);
        func_800BF0B4(0xA);
        return;
    case 9:
        idx = BG_U32(0x800C360C);
        func_800AA760(s2, (u8)BG_U16(base + idx * 0x48 + 0x172));
        func_800BF0B4(9);
        BG_U32(0x800C360C) = BG_U32(0x800C360C) + 1;
        return;
    case 10:
        idx = BG_U32(0x800C360C);
        a1 = BG_U16(base + idx * 0x48 + 0x14E);
        BG_U32(0x800C360C) = idx + 1;
        func_800BEE2C(s2, a1, 2);
        return;
    case 0: case 2: case 3: case 4: case 5: case 7:
        func_800B9284(BG_PTR(s1), s3);
        func_800BF0B4(9);
        BG_U32(0x800C360C) = BG_U32(0x800C360C) + 1;
        return;
    case 8:
        func_800BF0B4(9);
        idx = BG_U32(0x800C360C);
        a1 = BG_U16(base + idx * 0x48 + 0x172);
        func_800AA79C(bat4_l110_slot(s1), a1);
        BG_U32(0x800C360C) = BG_U32(0x800C360C) + 1;
        return;
    case 1:
        func_800BF0B4(9);
        idx = BG_U32(0x800C360C);
        BG_U32(0x800C360C) = idx + 1;
        a1 = BG_U16(base + idx * 0x48 + 0x172);
        BG_U16(0x800C3DF0) = (u16)((a1 >> 9) & 0x3F); /* D_800C3DF0 */
        BG_U16(0x800D39E4) = (u16)(a1 & 0x1FF);        /* D_800D39E4 */
        return;
    default:
        BG_U8(BG_U32(0x800C3610) + 0x4A) = 1;
        func_800B8048(s1);
        idx = BG_U32(0x800C360C);
        a1 = BG_U16(base + idx * 0x48 + 0x14E);
        BG_U32(0x800C360C) = idx + 1;
        func_800BEE2C(s2, a1, s3);
        return;
    }
}

/* func_800B9F78.s: per-frame battle command task (re-entrancy guarded by
 * D_800C3660).  Publishes the task as D_800C3610, runs the pending
 * start-of-command pose (D_800C3628), the queued sub-steps (+0x34), the
 * one-shot event in +0x49, then the phase handler selected by +0x1C
 * (jtbl_80070AB8, phases 2..10). */
void func_800B9F78(u8* task) {
    u32 base = 0x800C3EB0; /* D_800C3EB0 */
    u32 s1;
    u32 s3;
    u32 p;
    u32 v;

    if (BG_U32(0x800C3660) != 0) { /* D_800C3660 */
        return;
    }
    BG_U32(0x800C3660) = 1;
    s1 = *(u32*)(task + 4);
    BG_U32(0x800C3610) = BG_ADDR(task); /* D_800C3610 */
    v = BG_U32(0x800C3628);             /* D_800C3628 */
    s3 = *(u32*)(task + 0x4C);
    if (v != 0) {
        BG_U32(0x800C3628) = 0;
        if ((func_800B7E94() & 0xFF) != 0) {
            func_80021BF8(BG_PTR(s1), (s32)0x800B9B30u);
        } else {
            if ((s32)bat4_l110_slot(s1) < 3) {
                func_800245D8(BG_PTR(s1), BG_U8(0x800C3623) != 0 ? 0x13 : 0x12); /* D_800C3623 */
                BG_U8(0x800C3623) = 0;
            }
            if (BG_U32(s1 + 0x48) != 0) {
                func_800BF0B4(7);
            }
        }
    }
    p = BG_U32(0x800C3610);
    if (BG_U32(p + 0x34) != 0) {
        func_800B9C78();
        p = BG_U32(0x800C3610);
        BG_U32(p + 0x34) = BG_U32(p + 0x34) - 1;
    }
    p = BG_U32(0x800C3610);
    v = BG_U8(p + 0x49);
    if (v != 0) {
        switch (v) {
        case 4: {
            u32 s0 = BG_U32(0x800D363C); /* D_800D363C */
            u32 slot = bat4_l110_slot(s0);

            if ((s32)slot < 3 && BG_U8(base + slot * 0x1C + 8) == 0) {
                u32 idx = BG_U32(0x800C360C) - 1; /* D_800C360C */

                if (BG_U8(base + idx * 0x48 + slot + 0x150) == 7) {
                    func_800B8048(s0);
                    func_800245D8(BG_PTR(s0), 0x1B);
                    if (BG_S8(s0 + 0xAF) == 0x1B) {
                        do {
                            func_800BE790();
                        } while (BG_S8(s0 + 0xAF) == 0x1B);
                    }
                }
            }
            break;
        }
        case 6:
            func_800BF4F0(BG_PTR(s1), BG_PTR(BG_U32(s1 + 0x74)));
            break;
        case 2:
            func_800B9C00(BG_PTR(s1), BG_PTR(BG_U32(s1 + 0x74)));
            break;
        case 5: {
            u32 slot = bat4_l110_slot(s1);

            if (BG_U8(base + slot * 0x1C + 7) == 0) {
                func_800245D8(BG_PTR(s1), BG_S8(s1 + 0xB0));
            }
            func_800BAEB8((s32)BG_U32(BG_U32(0x800C3610) + 0x24));
            func_800BF0B4(0xA);
            break;
        }
        default:
            break;
        }
        BG_U8(BG_U32(0x800C3610) + 0x49) = 0;
    }
    switch (BG_U32(BG_U32(0x800C3610) + 0x1C) - 2) {
    case 0:
    case 4: {
        u32 a0 = (u32)(u16)BG_S16(s1 + 2) | ((u32)(u16)BG_S16(s1 + 0xA) << 16);
        u32 a1 = (u32)BG_U16(s1 + 0xA0) | ((u32)BG_U16(s1 + 0xA4) << 16);
        s32 d = func_800C07CC(a0, a1);

        p = BG_U32(0x800C3610);
        if (BG_S32(p + 0x44) < d) {
            if (BG_U32(BG_U32(0x800C3610) + 0x1C) - 2 == 0) {
                func_800B9C00(BG_PTR(s1), BG_PTR(s3));
            } else {
                func_800BF4F0(BG_PTR(s1), BG_PTR(s3));
            }
        } else {
            BG_U32(p + 0x44) = (u32)d;
        }
        break;
    }
    case 2:
        if (BG_U8(BG_U32(0x800C3610) + 0x48) != 0) {
            func_800B905C();
        }
        break;
    case 5:
        if (func_800BF720() == (u32)D_80059464) {
            func_800BF0B4(4);
            BG_U8(BG_U32(0x800C3610) + 0x48) = 1;
            func_800BF9EC();
        }
        break;
    case 6:
        if (BG_U8(0x800C362C) != 0 && func_800BF720() == (u32)D_80059464) { /* D_800C362C */
            if (BG_U8(0x800C3624) != 0) { /* D_800C3624 */
                if (BG_U8(0x800C362C) == 1) {
                    p = BG_U32(0x800C3610);
                    BG_U32(p + 0x34) = BG_U32(p + 0x34) + 1;
                }
                func_800BFA9C();
            }
        }
        break;
    case 7:
        func_800B9C78();
        break;
    case 8:
        if (func_800BF720() == (u32)D_80059464) {
            func_800B8EBC();
        }
        break;
    default:
        break;
    }
    BG_U32(0x800C3660) = 0;
}

/* func_800BA4E0.s: end-of-command cleanup for slot `index`: clear the timer
 * work-list counters, finish the command (func_800B136C), release the
 * slot's resources (func_800B8D7C/func_800BFBA0 for inactive slots,
 * func_800BF2B8 for party slots 0..2, func_800B8D7C otherwise) and queue
 * battle event 0xC0. */
void func_800BA4E0(s32 index) {
    D_80059464 = 0;
    D_800591AC = 0;
    func_800B136C();
    if (BG_U8(0x800C3EB0 + (u32)index * 0x1C + 8) != 0) { /* D_800C3EB0 */
        func_800B8D7C();
        func_800BFBA0();
    } else if (index < 3) {
        u32 rec = BG_U32(0x800C3EB0 + (u32)index * 4 + 0x8C8C);

        if (rec != 0) {
            func_800BF2B8(BG_PTR(rec));
        }
    } else {
        func_800B8D7C();
    }
    func_800BC454(0xC0);
}

/* func_800BA59C.s: set the record's heading (+0x32) and its x/z velocity
 * (+0xC, +0x14) from speed +0x18 / 8: (sin/2 * s) >> 8, -(cos/2 * s) >> 8. */
void func_800BA59C(u8* rec, u32 angle) {
    s32 speed = *(s32*)(rec + 0x18) >> 3;
    s32 t;

    *(u16*)(rec + 0x32) = (u16)angle;
    t = bg_rsin((s16)angle) >> 1;
    *(s32*)(rec + 0xC) = (s32)((u32)t * (u32)speed) >> 8;
    t = bg_rcos(*(s16*)(rec + 0x32)) >> 1;
    *(s32*)(rec + 0x14) = (s32)(0u - (u32)t * (u32)speed) >> 8;
}

/* func_800BA614.s: aim the record at its target (+0xA0..+0xA4): clamp the
 * ground height there (func_80021B04 / func_800A5914 / func_800A5870) to
 * the target y, derive the vertical speed (+0x10) from the planar distance
 * and height difference, then set heading and x/z velocity
 * (func_800BA59C). */
void func_800BA614(u8* rec) {
    SVECTOR v;
    VECTOR out;
    s32 d[4];
    s32 r;
    s32 dy;
    s32 heading;
    s32 len;
    s32 speed;
    s32 a2;
    s32 t;

    func_80021B04(&v, *(s16*)(rec + 0xA0), *(s16*)(rec + 0xA2), *(s16*)(rec + 0xA4));
    r = func_800A5914(&v, *(u32*)(rec + 0x78), 4);
    if (r < 0) {
        r = func_800A579C(&v);
    }
    func_800A5870(&v, r, &out);
    if (*(s16*)(rec + 0xA2) < v.vy) {
        v.vy = (s16)*(u16*)(rec + 0xA2);
    }
    d[0] = *(s16*)(rec + 0xA0) - *(s16*)(rec + 2);
    dy = (s32)(((u32)(s32)v.vy << 16) - *(u32*)(rec + 4)) >> 16;
    d[1] = 0;
    d[2] = *(s16*)(rec + 0xA4) - *(s16*)(rec + 0xA);
    d[3] = 0;
    heading = (s32)(0u - (u32)ratan2(d[2], d[0]));
    Square0(d, d);
    len = SquareRoot0((s32)((u32)d[0] + (u32)d[2]));
    speed = *(s32*)(rec + 0x18);
    t = (s32)((0u - *(u32*)(rec + 0x1C)) * (u32)len);
    a2 = bg_div((s32)((u32)t << 4), speed >> 11);
    t = (s32)((u32)speed * (u32)dy);
    a2 = (s32)((u32)a2 + (u32)bg_div(t, len));
    *(s32*)(rec + 0x10) = a2;
    func_800BA59C(rec, (u32)(s32)(s16)heading);
}

/* func_800BA768.s: start a jump towards the target (+0xA0..+0xA4) taking
 * n = -(2 * vy / g) frames (+0x10, +0x1C): truncate x/y/z to whole units,
 * planar speed +0x18 = (distance << 16) / n, correct the vertical speed for
 * the clamped ground height there, set heading/velocity (func_800BA59C) and
 * refresh the sprite (func_80022B2C). */
void func_800BA768(u8* rec) {
    SVECTOR v;
    VECTOR out;
    s32 d[4];
    s32 r;
    s32 frames;
    s32 heading;
    s32 len;
    s32 t;
    s32 a1;

    t = bg_div((s32)(*(u32*)(rec + 0x10) << 1), *(s32*)(rec + 0x1C));
    *(u32*)(rec + 4) = *(u32*)(rec + 4) & 0xFFFF0000u;
    a1 = *(s16*)(rec + 0xA0);
    *(u32*)(rec + 0) = *(u32*)(rec + 0) & 0xFFFF0000u;
    a1 -= *(s16*)(rec + 2);
    *(u32*)(rec + 8) = *(u32*)(rec + 8) & 0xFFFF0000u;
    d[0] = a1;
    d[1] = 0;
    d[2] = *(s16*)(rec + 0xA4) - *(s16*)(rec + 0xA);
    d[3] = 0;
    frames = (s32)(0u - (u32)t);
    heading = (s32)(0u - (u32)ratan2(d[2], d[0]));
    Square0(d, d);
    len = SquareRoot0((s32)((u32)d[0] + (u32)d[2]));
    if (frames != 0) {
        *(s32*)(rec + 0x18) = bg_div((s32)((u32)len << 16), frames);
    } else {
        *(s32*)(rec + 0x18) = 0;
    }
    func_80021B04(&v, *(s16*)(rec + 0xA0), *(s16*)(rec + 0xA2), *(s16*)(rec + 0xA4));
    r = func_800A5914(&v, *(u32*)(rec + 0x78), 4);
    if (r < 0) {
        r = func_800A579C(&v);
    }
    func_800A5870(&v, r, &out);
    if (*(s16*)(rec + 0xA2) < v.vy) {
        v.vy = (s16)*(u16*)(rec + 0xA2);
    }
    t = (s32)(((u32)(s32)v.vy << 16) - *(u32*)(rec + 4));
    if (frames != 0) {
        *(s32*)(rec + 0x10) = (s32)(*(u32*)(rec + 0x10) + (u32)bg_div(t, frames));
    }
    func_800BA59C(rec, (u32)(s32)(s16)heading);
    func_80022B2C(rec);
}
#endif
