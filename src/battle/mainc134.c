#include "common.h"


#ifndef XENO_PC_PORT
extern u8 D_8006BE10[];
extern u8 D_800591AC[];
extern u8* func_80023B84(void* pSpriteData, void* pScript, void* pAnimPackage);
extern u32 func_800BFC80(u8* sprite, u32 index, u32 a2);

/* when the sprite is live (+0x48) and has no child of kind `index` yet
 * (func_800BFC80), spawn child script `index` from the shared package
 * D_8006BE10 (table pointer at +0x10, halfword offsets from +2) with the
 * D_800591AC timer flag suppressed, then tag the child (+0xAF = index,
 * +0x74 = parent, +0xB0 |= 0x100). */
void func_800BFDA8(u8* sprite, u32 index) {
    u8* table;
    u8* child;
    u8 saved;
    u8* pkg;
    u8* script;
    u32 flags;

    if (*(u32*)(sprite + 0x48) == 0) {
        return;
    }
    if (func_800BFC80(sprite, index, 0) != 0) {
        return;
    }
    pkg = D_8006BE10;
    table = *(u8**)(pkg + 0x10);
    script = (u8*)(*(u16*)(index * 2 + table + 2) + (u32)table);
    saved = D_800591AC[0];
    D_800591AC[0] = 0;
    child = func_80023B84(sprite, script, pkg);
    flags = *(u32*)(child + 0xB0);
    *(u8*)(child + 0xAF) = index;
    *(u8**)(child + 0x74) = sprite;
    D_800591AC[0] = saved;
    *(u32*)(child + 0xB0) = flags | 0x100;
}

INCLUDE_ASM("asm/battle/nonmatchings/mainc134", func_800BFE48);
INCLUDE_ASM("asm/battle/nonmatchings/mainc134", func_800C0314);
INCLUDE_ASM("asm/battle/nonmatchings/mainc134", func_800C0564);
#include "psyq/libgte.h"

/* distance between two word vectors (x, y, z) */
s32 func_800C06E4(VECTOR* a, VECTOR* b) {
    VECTOR v;

    v.vx = a->vx - b->vx;
    v.vy = a->vy - b->vy;
    v.vz = a->vz - b->vz;
    Square0(&v, &v);
    return SquareRoot0(v.vy + v.vz + v.vx);
}

#include "psyq/libgte.h"

/* distance between two halfword vectors (x, y, z) */
s32 func_800C0758(SVECTOR* a, SVECTOR* b) {
    VECTOR v;

    v.vx = a->vx - b->vx;
    v.vy = a->vy - b->vy;
    v.vz = a->vz - b->vz;
    Square0(&v, &v);
    return SquareRoot0(v.vy + v.vz + v.vx);
}

#include "psyq/libgte.h"

typedef struct {
    s16 x;
    s16 z;
} Pos2;

s32 func_800C07CC(Pos2 a, Pos2 b) {
    VECTOR v;

    v.vx = a.x - b.x;
    v.vz = a.z - b.z;
    Square0(&v, &v);
    return SquareRoot0(v.vx + v.vz);
}

#include "psyq/libgte.h"

/* rotation from halfword point `to` towards `from`:
 * out = (0, ratan2(dz, dx), ratan2(dy, planar length)). */
void func_800C0828(SVECTOR* from, SVECTOR* to, SVECTOR* out) {
    MATRIX unused;
    VECTOR v;
    VECTOR sq;
    s32 len;

    v.vx = from->vx - to->vx;
    v.vy = from->vy - to->vy;
    v.vz = from->vz - to->vz;
    Square0(&v, &sq);
    len = SquareRoot0(sq.vx + sq.vz);
    out->vy = ratan2(v.vz, v.vx);
    out->vz = ratan2(v.vy, len);
    out->vx = 0;
}

INCLUDE_ASM("asm/battle/nonmatchings/mainc134", func_800C08CC);
INCLUDE_ASM("asm/battle/nonmatchings/mainc134", func_800C0D18);
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
extern void func_800245D8(void* pSpriteData, s16 animIndex);
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


/* func_800C0F70.s */
#ifndef XENO_PC_PORT
extern u32 D_800C3A6C[];
#endif
extern void SoundFreeWdsEntry(u32 p);
void func_800C0F70(void) {
    if (D_800C3A6C[0] != 0) {
        SoundFreeWdsEntry(D_800C3A6C[0]);
    }
    D_800C3A6C[0] = 0;
}


#ifdef XENO_PC_PORT
#include "battle_host_guest.h"
/* Native main-executable globals and routines (not in guest RAM). */
extern u8 D_800591AC;
extern u8 D_8006BE10[];
extern void* func_80023B84(void* pSpriteData, void* pScript, void* pAnimPackage);
extern void* Square0(void* in, void* out);
extern int SquareRoot0(int a);
extern int ratan2(int y, int x);
/* Battle overlay callees. */
extern u32 func_800BFC80(u32 a0, u32 a1, u32 a2);
extern u32 func_8009A0DC(u32 index);
extern u32 func_8009A1AC(u32 index);
extern void func_800BE790(void);
extern void func_800BF9EC(void);
extern void func_800B136C(void);
extern void func_800BEE2C(u32 a0, u32 a1, u32 a2);
extern u32 func_800BEEB4(u32 mask, u32* out, u32 value);
extern void func_800BAEB8(s32 a0);
extern void func_800BFD88(u32 a0, u32 a1);
void func_800C0564(void);
u32 func_800C0314(void);
void func_800BFDA8(u8* sprite, u32 index);

/* Slot index packed into a record's +0xA8 (bits 30-31) and +0xAC (bits 0-1). */
static u32 bat4_c134_slot(u32 rec) {
    return ((BG_U32(rec + 0xAC) & 3) << 2) | (BG_U32(rec + 0xA8) >> 30);
}

/* func_800BFDA8.s: when the sprite is live (+0x48) and has no child of kind
 * `index` yet (func_800BFC80), spawn child script `index` from the shared
 * package D_8006BE10 (table pointer at +0x10, halfword offsets from +2) with
 * the D_800591AC timer flag suppressed, then tag the child (+0xAF = index,
 * +0x74 = parent, +0xB0 |= 0x100). */
void func_800BFDA8(u8* sprite, u32 index) {
    u32 s = BG_ADDR(sprite);
    u32 table;
    u32 child;
    u32 flags;
    u8 saved;

    if (BG_U32(s + 0x48) == 0) {
        return;
    }
    if (func_800BFC80(s, index, 0) != 0) {
        return;
    }
    table = *(u32*)(D_8006BE10 + 0x10);
    saved = D_800591AC;
    D_800591AC = 0;
    child = (u32)(uintptr_t)func_80023B84(
        BG_PTR(s), BG_PTR(table + BG_U16(index * 2 + table + 2)), D_8006BE10);
    flags = BG_U32(child + 0xB0);
    BG_U8(child + 0xAF) = (u8)index;
    BG_U32(child + 0x74) = s;
    D_800591AC = saved;
    BG_U32(child + 0xB0) = flags | 0x100;
}

/* func_800C06E4.s: distance between two word vectors (x, y, z). */
s32 func_800C06E4(s32* a, s32* b) {
    s32 v[4];
    s32 sq[4];

    v[0] = (s32)((u32)a[0] - (u32)b[0]);
    v[1] = (s32)((u32)a[1] - (u32)b[1]);
    v[2] = (s32)((u32)a[2] - (u32)b[2]);
    v[3] = 0;
    Square0(v, sq);
    return SquareRoot0((s32)((u32)sq[1] + (u32)sq[2] + (u32)sq[0]));
}

/* func_800C0758.s: distance between two halfword vectors (x, y, z). */
s32 func_800C0758(s16* a, s16* b) {
    s32 v[4];
    s32 sq[4];

    v[0] = a[0] - b[0];
    v[1] = a[1] - b[1];
    v[2] = a[2] - b[2];
    v[3] = 0;
    Square0(v, sq);
    return SquareRoot0((s32)((u32)sq[1] + (u32)sq[2] + (u32)sq[0]));
}

/* func_800C07CC.s: planar distance between two packed (lo, hi) halfword
 * pairs passed by value; lo is x, hi is z.  Retail leaves the y lane of the
 * Square0 input uninitialised and never reads its square. */
s32 func_800C07CC(u32 a, u32 b) {
    s32 v[4];

    v[0] = (s16)a - (s16)b;
    v[1] = 0;
    v[2] = (s16)(a >> 16) - (s16)(b >> 16);
    v[3] = 0;
    Square0(v, v);
    return SquareRoot0((s32)((u32)v[0] + (u32)v[2]));
}

/* func_800C0828.s: rotation from halfword point `to` towards `from`:
 * out = (0, ratan2(dz, dx), ratan2(dy, planar length)). */
void func_800C0828(s16* from, s16* to, s16* out) {
    s32 v[4];
    s32 sq[4];
    s32 len;

    v[0] = from[0] - to[0];
    v[1] = from[1] - to[1];
    v[2] = from[2] - to[2];
    v[3] = 0;
    Square0(v, sq);
    len = SquareRoot0((s32)((u32)sq[0] + (u32)sq[2]));
    out[1] = (s16)ratan2(v[2], v[0]);
    out[2] = (s16)ratan2(v[1], len);
    out[0] = 0;
}

/* func_800C0564.s: wait (func_800BE790 frames) until no party slot is
 * still settling: an inactive (+8 set) slot whose record +0x64 is busy, or
 * an active slot (func_8009A0DC != 8, record state +0xAF outside the
 * jtbl_80070BB0 idle set, +7 clear) whose state is neither the D_800C37D4
 * target nor its +0xB0 fallback. */
void func_800C0564(void) {
    u32 busy = 0;
    u32 i;

    for (;;) {
        u32 row = 0x800C3EB0; /* D_800C3EB0 */
        u32 ent = 0x800C3EB0;

        for (i = 0; i != 11; i++) {
            u32 rec = BG_U32(ent + 0x8C8C);

            if (rec != 0) {
                if (BG_U8(row + 8) != 0) {
                    if (BG_U32(rec + 0x64) != 0) {
                        busy = 1;
                    }
                } else if (func_8009A0DC(i) != 8) {
                    s32 state = BG_S8(rec + 0xAF);
                    u32 check;

                    switch ((u32)state) {
                    case 0: case 1: case 5: case 7: case 14: case 15:
                    case 20: case 21: case 22: case 24:
                        check = 0;
                        break;
                    default:
                        check = 1;
                        break;
                    }
                    if (check && BG_U8(row + 7) == 0) {
                        s32 s0 = BG_S8(rec + 0xAF);
                        u32 k = func_8009A0DC(i);

                        if ((u32)s0 != BG_U16(0x800C37D4 + k * 2) /* D_800C37D4 */ &&
                            BG_S8(rec + 0xAF) != BG_S8(rec + 0xB0)) {
                            busy = 1;
                        }
                    }
                }
            }
            row += 0x1C;
            ent += 4;
        }
        if ((busy & 0xFF) == 0) {
            break;
        }
        busy = 0;
        func_800BE790();
    }
}

/* func_800C0314.s: collect the slots newly set in D_800C3EB0+0xA38 versus
 * D_800D2E54 (func_800BEEB4, tagging them with D_800C3E1C); drop those
 * masked by D_800C3608, and for the rest clear +0x9E, then either flag the
 * D_800D3368 entry (+0x38) and run func_800BEE2C(slot, slot, 0x15) for
 * inactive slots or set state 0x15 via func_800245D8, and mark the slot in
 * D_800D2E54.  Then wait frames while any collected live record has +0x9E
 * set, and run func_800B136C if an inactive slot was handled.  Returns the
 * number handled. */
u32 func_800C0314(void) {
    u32 list[12];
    s32 count = 0;
    u32 flag = 0;
    s32 i;
    u32 any;
    u32 a = BG_U16(0x800C3EB0 + 0xA38); /* D_800C3EB0 */
    u32 b = BG_U16(0x800D2E54);         /* D_800D2E54 */
    u32 n;

    n = func_800BEEB4(a & (a ^ b), list, BG_U32(0x800C3E1C) /* D_800C3E1C */);
    if (n != 0) {
        for (i = (s32)n - 1; i >= 0; i--) {
            u32 rec = list[i];
            u32 slot = bat4_c134_slot(rec);

            if (((u32)BG_U16(0x800C3608) >> slot) & 1) { /* D_800C3608 */
                list[i] = 0;
                continue;
            }
            BG_U16(rec + 0x9E) = 0;
            slot = bat4_c134_slot(rec);
            if (BG_U8(0x800C3EB0 + slot * 0x1C + 8) != 0) {
                flag = 1;
                BG_U8(BG_U32(0x800D3368 + slot * 4) /* D_800D3368 */ + 0x38) = 1;
                slot = bat4_c134_slot(rec);
                func_800BEE2C(slot, slot, 0x15);
                count++;
            } else {
                if (BG_S8(rec + 0xAF) != 0x15) {
                    func_800245D8(BG_PTR(rec), 0x15);
                }
                count++;
            }
            slot = bat4_c134_slot(rec);
            BG_U16(0x800D2E54) = (u16)(BG_U16(0x800D2E54) | (1u << (slot & 31)));
        }
    }
    any = 0;
    for (;;) {
        if (count != 0) {
            s32 j;

            for (j = 0; j != count; j++) {
                u32 e = list[j];

                if (e != 0 && BG_U32(e + 0x48) != 0 && BG_S16(e + 0x9E) != 0) {
                    any = 1;
                }
            }
        }
        if ((any & 0xFF) == 0) {
            break;
        }
        any = 0;
        if (count == 0) {
            continue;
        }
        func_800BF9EC();
        if (count == 0) {
            continue;
        }
        func_800BE790();
    }
    if ((flag & 0xFF) != 0) {
        func_800B136C();
    }
    return (u32)count;
}

/* func_800BFE48.s: battle-start party pose pass.  (1) Active slots whose
 * state (+0xAF - 5) is in the jtbl_80070B08 set and differs from their
 * D_800C37D4 target go to animation 0x10 and leave D_800D2E54.  (2) Active
 * slots away (>= 9 on x or z) from their row's +0xE/+0x10 home get +0xAC
 * bit 0x40, the home at +0xA0..+0xA4 and animation 3.  (3) Clear bit 0x40 on
 * every record.  (4) Per active slot: func_800BAEB8, func_800C0314, restore
 * the target pose, then diff func_8009A1AC flags against the record's
 * +0x7C->+0xC and spawn (func_800BFDA8) / remove (func_800BFD88) child
 * effects 9..11 for flag bits 13..15. */
void func_800BFE48(void) {
    u32 base = 0x800C3EB0; /* D_800C3EB0 */
    u32 i;

    for (i = 0; i != 11; i++) {
        u32 row = base + i * 0x1C;
        u32 rec;
        s32 kind;

        if (BG_U8(row + 8) != 0) {
            continue;
        }
        rec = BG_U32(base + i * 4 + 0x8C8C);
        if (rec == 0) {
            continue;
        }
        if (func_8009A0DC(i) == 8) {
            continue;
        }
        kind = (s8)(u8)(BG_U8(rec + 0xAF) - 5);
        switch ((u32)kind) {
        case 0: case 2: case 9: case 10: case 16: {
            s32 s0 = BG_S8(rec + 0xAF);
            u32 k = func_8009A0DC(i);
            u32 slot;

            if ((u32)s0 == BG_U16(0x800C37D4 + k * 2)) { /* D_800C37D4 */
                break;
            }
            func_800245D8(BG_PTR(rec), 0x10);
            slot = bat4_c134_slot(rec);
            BG_U16(0x800D2E54) = (u16)(BG_U16(0x800D2E54) & ~(1u << (slot & 31)));
            break;
        }
        default:
            break;
        }
    }
    func_800C0564();

    for (i = 0; i != 11; i++) {
        u32 row = base + i * 0x1C;
        u32 rec;
        s32 d;
        u32 near;

        if (BG_U8(row + 8) != 0) {
            continue;
        }
        rec = BG_U32(base + i * 4 + 0x8C8C);
        if (rec == 0) {
            continue;
        }
        if (func_8009A0DC(i) == 8) {
            continue;
        }
        if (BG_S8(rec + 0xAF) == 0x15) {
            continue;
        }
        d = BG_S16(rec + 2) - (s32)BG_U16(row + 0xE);
        near = d >= 0 ? d < 9 : ((s32)BG_U16(row + 0xE) - BG_S16(rec + 2)) < 9;
        if (near) {
            d = BG_S16(rec + 0xA) - (s32)BG_U16(row + 0x10);
            near = d >= 0 ? d < 9 : ((s32)BG_U16(row + 0x10) - BG_S16(rec + 0xA)) < 9;
            if (near) {
                continue;
            }
        }
        {
            u16 x;
            u16 z;

            BG_U32(rec + 0xAC) = BG_U32(rec + 0xAC) | 0x40;
            x = BG_U16(row + 0xE);
            BG_U16(rec + 0xA0) = x;
            z = BG_U16(row + 0x10);
            BG_U16(rec + 0xA2) = 0;
            BG_U16(rec + 0xA4) = z;
            func_800245D8(BG_PTR(rec), 3);
        }
    }
    func_800C0564();

    for (i = 0; i != 11; i++) {
        u32 rec = BG_U32(base + i * 4 + 0x8C8C);

        if (rec != 0) {
            BG_U32(rec + 0xAC) = BG_U32(rec + 0xAC) & 0xFFFFFFBFu;
        }
    }

    for (i = 0; i != 11; i++) {
        u32 row = base + i * 0x1C;
        u32 rec;
        u32 flags;
        u32 model;
        u32 old;
        u32 added;
        u32 removed;
        u32 bit;

        if (BG_U8(row + 8) != 0) {
            continue;
        }
        rec = BG_U32(base + i * 4 + 0x8C8C);
        if (rec == 0) {
            continue;
        }
        if (BG_S8(rec + 0xAF) != 0x15) {
            func_800BAEB8((s32)i);
        }
        func_800C0314();
        if (func_8009A0DC(i) != 8) {
            u32 k = func_8009A0DC(i);
            u32 target = BG_U16(0x800C37D4 + k * 2); /* D_800C37D4 */

            if (BG_U8(row + 8) == 0) {
                u32 go = 1;

                if (target == 0x15) {
                    u32 slot = bat4_c134_slot(rec);

                    if ((((u32)BG_U16(0x800C3608) >> slot) & 1) == 0) { /* D_800C3608 */
                        go = 0;
                    }
                }
                if (go) {
                    if (target == 1) {
                        target = (u32)(s32)BG_S8(rec + 0xB0);
                    }
                    if (BG_U8(row + 7) == 0 && (u32)(s32)BG_S8(rec + 0xAF) != target) {
                        func_800245D8(BG_PTR(rec), (s16)target);
                    }
                }
            }
        }
        flags = func_8009A1AC(i);
        model = BG_U32(rec + 0x7C);
        old = BG_U16(model + 0xC);
        BG_U16(model + 0xC) = (u16)flags;
        removed = old & ~flags;
        added = flags & ~old;
        for (bit = 0; bit != 16; bit++) {
            if (added & 1) {
                switch (bit) {
                case 13:
                    func_800BFDA8(BG_PTR(rec), 9);
                    break;
                case 14:
                    func_800BFDA8(BG_PTR(rec), 10);
                    break;
                case 15:
                    func_800BFDA8(BG_PTR(rec), 11);
                    break;
                }
            }
            added = (added & 0xFFFF) >> 1;
        }
        for (bit = 0; bit != 16; bit++) {
            if (removed & 1) {
                switch (bit) {
                case 13:
                    func_800BFD88(rec, 9);
                    break;
                case 14:
                    func_800BFD88(rec, 10);
                    break;
                case 15:
                    func_800BFD88(rec, 11);
                    break;
                }
            }
            removed = (removed & 0xFFFF) >> 1;
        }
    }
}
#endif
