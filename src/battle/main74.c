#include "common.h"


#ifndef XENO_PC_PORT
extern void func_800A23E8(s32, s32);
extern void func_800B00D0(void);
extern s32 D_800C3BAC[];

/* Release the nine handles in D_800C3BAC, then func_800B00D0. */
void func_800B00F4(s32 ctx) {
    s32 h;
    s32 i;
    s32* p;

    for (i = 0, p = D_800C3BAC; i < 9; i++) {
        h = *p++;
        if (h != 0) {
            func_800A23E8(ctx, h);
        }
    }
    func_800B00D0();
}

extern u8 D_800C37C8;
extern u8* func_800A2330(u8* pool);

/* Take (once) a pool slot for handle `index` and fill its 20-byte record. */
void func_800B0164(u8* pool, s32 index, u8 a, u8 b, u16 p0, u16 p1, u16 p2, u16 p3, u16 p4, u16 p5, u16 p6) {
    u8* e;

    if (D_800C37C8 == 0) {
        if (D_800C3BAC[index] == 0) {
            D_800C3BAC[index] = (s32)func_800A2330(pool);
        }
        e = (u8*)D_800C3BAC[index];
        if (e != NULL) {
            e[0] = 1;
            e[1] = 0;
            e[2] = a;
            e[3] = b;
            *(u16*)(e + 0x4) = p0;
            *(u16*)(e + 0x6) = p1;
            *(u16*)(e + 0x8) = p2;
            *(u16*)(e + 0xA) = p3;
            *(u16*)(e + 0xC) = p4;
            *(u16*)(e + 0xE) = p5;
            *(u16*)(e + 0x10) = 0;
            *(u16*)(e + 0x12) = p6;
        }
    }
}

INCLUDE_ASM("asm/battle/nonmatchings/main74", func_800B026C);
INCLUDE_ASM("asm/battle/nonmatchings/main74", func_800B0AB4);
INCLUDE_ASM("asm/battle/nonmatchings/main74", func_800B0B14);
INCLUDE_ASM("asm/battle/nonmatchings/main74", func_800B0D70);
INCLUDE_ASM("asm/battle/nonmatchings/main74", func_800B0FF4);
INCLUDE_ASM("asm/battle/nonmatchings/main74", func_800B10EC);
INCLUDE_ASM("asm/battle/nonmatchings/main74", func_800B12D0);
INCLUDE_ASM("asm/battle/nonmatchings/main74", func_800B136C);
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


/* func_800B14B8.s */
void func_800B14B8(void) {
    D_800C3D6C = 1;
}


#ifdef XENO_PC_PORT
#include "battle_host_guest.h"
/* Native owner: src/slus_006.64/system/asset_loader.c stores a host pointer. */
extern u8* D_800658C8;

/* D_800D3368: the eleven battle actor record pointers. */
#define BATTLE_ACTOR_TABLE 0x800D3368u
/* Native psyq (PsyCross psx/libgte.h) and main-executable prototypes. */
extern int SquareRoot0(int a);
extern int ratan2(int y, int x);
extern int rcos(int a);
extern int rsin(int a);
extern void func_8002A498(int channel);
extern int ArchiveDataSync(void);

/* func_800B00F4.s: release each non-zero handle of the nine-word table
 * D_800C3BAC through func_800A23E8, then clear the table (func_800B00D0). */
extern void func_800A23E8(u8* a0, u32 p);
extern void func_800B00D0(void);
void func_800B00F4(u8* ctx) {
    u32 slot = 0x800C3BAC; /* D_800C3BAC */
    s32 i;

    for (i = 0; i < 9; i++, slot += 4) {
        if (BG_U32(slot) != 0) {
            func_800A23E8(ctx, BG_U32(slot));
        }
    }
    func_800B00D0();
}

/* func_800B0164.s: unless D_800C37C8 is set, fill the 0x14-byte record of
 * D_800C3BAC[index] (allocated from `pool` by func_800A2330 when empty):
 * +0 = 1, +1 = 0, +2/+3 = a2/a3 bytes, +4..+0xE = the six stack halfwords,
 * +0x10 = 0, +0x12 = the seventh.  Stack arguments are read with LHU. */
extern u32 func_800A2330(u8* pool);
void func_800B0164(u8* pool, u32 index, u32 a2, u32 a3, u32 h0, u32 h1, u32 h2,
                   u32 h3, u32 h4, u32 h5, u32 h6) {
    u32 slot;
    u32 rec;

    if (BG_U8(0x800C37C8) != 0) { /* D_800C37C8 */
        return;
    }
    slot = 0x800C3BAC + index * 4; /* D_800C3BAC */
    if (BG_U32(slot) == 0) {
        BG_U32(slot) = func_800A2330(pool);
    }
    rec = BG_U32(slot);
    if (rec == 0) {
        return;
    }
    BG_U8(rec + 0) = 1;
    BG_U8(rec + 1) = 0;
    BG_U8(rec + 2) = (u8)a2;
    BG_U8(rec + 3) = (u8)a3;
    BG_U16(rec + 4) = (u16)h0;
    BG_U16(rec + 6) = (u16)h1;
    BG_U16(rec + 8) = (u16)h2;
    BG_U16(rec + 0xA) = (u16)h3;
    BG_U16(rec + 0xC) = (u16)h4;
    BG_U16(rec + 0xE) = (u16)h5;
    BG_U16(rec + 0x10) = 0;
    BG_U16(rec + 0x12) = (u16)h6;
}

/* func_800B0D70.s: find the actor (D_800D3368, +0x34 set) with the largest
 * func_800AA650 + 0x80 reach whose height test against pos[1] passes and
 * whose circle (centre = actor xz minus dir / 6) holds pos; push pos out to
 * that circle's rim.  Returns the chosen index + 1, or 0 if none. */
extern s32 func_800AA600(u32 slot);
extern s32 func_800AA650(u32 slot);
s32 func_800B0D70(s16* dir, s16* pos) {
    s32 best = -1;
    s16 bestReach = 0;
    u16 bestX = 0;
    u16 bestZ = 0;
    s32 i;

    for (i = 0; i < 11; i++) {
        u32 reach = (u32)func_800AA650((u32)i) + 0x80;
        u32 ent = BG_U32(BATTLE_ACTOR_TABLE + (u32)i * 4);
        u32 body;
        s32 height;
        s32 dx6;
        s32 dz6;
        u32 cx;
        u32 cz;
        s32 ox;
        s32 oz;
        s32 r;
        s32 len;

        if (ent == 0 || BG_U8(ent + 0x34) == 0) {
            continue;
        }
        height = pos[1];
        r = func_800AA600((u32)i);
        body = BG_U32(BG_U32(BATTLE_ACTOR_TABLE + (u32)i * 4) + 4);
        if (!((s32)(BG_U32(body + 0x60) - (u32)r) < height)) {
            continue;
        }
        if ((s16)best >= 0 && !(bestReach < (s16)reach)) {
            continue;
        }
        dx6 = (s32)(((s64)dir[0] * 0x2AAAAAAB) >> 32) - (dir[0] >> 31);
        dz6 = (s32)(((s64)dir[2] * 0x2AAAAAAB) >> 32) - (dir[2] >> 31);
        cx = (u32)BG_U16(body + 0x5C) - (u32)dx6;
        ox = (s16)((u32)(u16)pos[0] - cx);
        cz = (u32)BG_U16(body + 0x64) - (u32)dz6;
        oz = (s16)((u32)(u16)pos[2] - cz);
        len = (s32)((u32)SquareRoot0((s32)((u32)(ox * ox) + (u32)(oz * oz))) + 1);
        if (!(len < (s16)reach)) {
            continue;
        }
        best = i;
        bestReach = (s16)reach;
        bestX = (u16)(cx + (u32)bg_div(ox * (s16)reach, len));
        bestZ = (u16)(cz + (u32)bg_div(oz * (s16)reach, len));
    }
    if ((s16)best < 0) {
        return 0;
    }
    pos[0] = (s16)bestX;
    pos[2] = (s16)bestZ;
    return (s16)best + 1;
}

/* func_800B0FF4.s: d = (a - b).xz, scaled to 512 / (|d| + 1), then
 * func_800B0D70(d, b); returns its result.  Only halfwords 0 and 2 of the
 * stack vector are written (func_800B0D70 reads no others). */
s32 func_800B0FF4(s16* a, s16* b) {
    s16 d[4] = { 0, 0, 0, 0 };
    u32 dx = (u32)(u16)a[0] - (u32)(u16)b[0];
    u32 dz = (u32)(u16)a[2] - (u32)(u16)b[2];
    s32 len;

    d[0] = (s16)dx;
    d[2] = (s16)dz;
    len = (s32)((u32)SquareRoot0((s32)((u32)((s16)dx * (s16)dx) + (u32)((s16)dz * (s16)dz))) + 1);
    d[0] = (s16)bg_div((s32)((u32)(s32)d[0] << 9), len);
    d[2] = (s16)bg_div((s32)((u32)(s32)d[2] << 9), len);
    return func_800B0D70(d, b);
}

/* func_800B10EC.s: if (x, z) lies within `radius` of actor `index`
 * (D_800D3368), move the actor's +0x5C/+0x64 position onto the circle of
 * that radius around (x, z), facing away along ratan2 (flipped by 0x800 when
 * the actor's +0x56 heading points the other way). */
void func_800B10EC(u32 index, s32 x, s32 z, s32 radius) {
    u32 slot = BATTLE_ACTOR_TABLE + index * 4;
    u32 body = BG_U32(BG_U32(slot) + 4);
    s32 dx = (s32)((u32)x - BG_U32(body + 0x5C));
    s32 dz = (s32)((u32)z - BG_U32(body + 0x64));
    s32 len = (s32)((u32)SquareRoot0((s32)((u32)dx * (u32)dx + (u32)dz * (u32)dz)) + 1);
    s32 angle;
    s32 q;
    s32 gap;
    s32 t;

    if (!(len < radius)) {
        return;
    }
    angle = ratan2(dz, (s32)(0u - (u32)dx));
    if (((((u32)(s32)BG_S16(BG_U32(BG_U32(slot) + 4) + 0x56) - (u32)angle) & 0xFFF) - 0x401) < 0x7FF) {
        angle = (s32)((u32)angle + 0x800);
    }
    t = bg_rcos(angle);
    q = bg_div((s32)((u32)dx * (u32)radius), len);
    gap = (s32)((u32)radius - (u32)len);
    t = (s32)((u32)t * (u32)gap);
    if (t < 0) {
        t = (s32)((u32)t + 0xFFF);
    }
    BG_U32(BG_U32(BG_U32(slot) + 4) + 0x5C) = (u32)x - (u32)q - (u32)(t >> 12);
    t = bg_rsin(angle);
    q = bg_div((s32)((u32)dz * (u32)radius), len);
    t = (s32)((u32)t * (u32)gap);
    if (t < 0) {
        t = (s32)((u32)t + 0xFFF);
    }
    BG_U32(BG_U32(BG_U32(slot) + 4) + 0x64) = (u32)z - (u32)q - (u32)(t >> 12);
}

/* func_800B136C.s: wait (func_800BE790 per pass) until no actor
 * (D_800D3368) has its +0x38 busy byte set and func_800BF6F8 reports idle;
 * then, if any actor had a +0xC resource, stop archive channel 0 and, once
 * ArchiveDataSync reports no pending read, free every such resource with
 * func_800B0060. */
extern u32 func_800BF6F8(void);
extern void func_800BE790(void);
extern void func_800B0060(u8* p);
void func_800B136C(void) {
    s32 busy = 0;
    s32 loaded = 0;
    s32 i = 0;

    for (;;) {
        u32 slot = BATTLE_ACTOR_TABLE;

        do {
            u32 ent = BG_U32(slot);

            if (ent != 0) {
                if (BG_U8(ent + 0x38) != 0) {
                    busy = 1;
                }
                if (BG_U32(ent + 0xC) != 0) {
                    loaded = 1;
                }
            }
            i++;
            slot += 4;
        } while (i < 11);
        if (func_800BF6F8() != 0) {
            busy = 1;
        }
        if (busy == 0) {
            break;
        }
        func_800BE790();
        busy = 0;
        i = 0;
    }
    if (loaded == 0) {
        return;
    }
    func_8002A498(0);
    busy = 1;
    for (;;) {
        if (ArchiveDataSync() == 0) {
            u32 slot = BATTLE_ACTOR_TABLE;

            for (i = 0; i < 11; i++) {
                u32 ent = BG_U32(slot);

                slot += 4;
                if (ent != 0 && BG_U32(ent + 0xC) != 0) {
                    func_800B0060(BG_PTR(ent));
                }
            }
            busy = 0;
        }
        if (busy == 0) {
            break;
        }
        func_800BE790();
    }
}

/* func_800B0AB4.s: is the (x, _, z) halfword point strictly inside the
 * rectangle at D_800658C8 + 0x34C (x max, x min, z min, z max)? */
u32 func_800B0AB4(s16* point) {
    u8* area = D_800658C8;
    s32 x = point[0];
    s32 z;

    if (!(*(s16*)(area + 0x34E) < x)) {
        return 0;
    }
    if (!(x < *(s16*)(area + 0x34C))) {
        return 0;
    }
    z = point[2];
    if (!(*(s16*)(area + 0x350) < z)) {
        return 0;
    }
    return z < *(s16*)(area + 0x352);
}

/* func_800B12D0.s: 1 when the command-mask bit selected by the current
 * actor's (D_800C360C - 1) 0x48-byte record at D_800C4000, byte `index`, is
 * clear in `mask`; the jtbl_800707C4 cases are 0,1 -> bit 0, 2,3 -> bit 3,
 * 4 -> bit 2, 5 -> bit 1.  Any other kind tests the whole low byte. */
u32 func_800B12D0(u32 index, u32 mask) {
    u32 kind = BG_U8(0x800C4000 + (BG_U32(0x800C360C) - 1) * 0x48 + index);
    u32 bit;

    switch (kind) {
    case 0:
    case 1:
        bit = 1;
        break;
    case 2:
    case 3:
        bit = 8;
        break;
    case 4:
        bit = 4;
        break;
    case 5:
        bit = 2;
        break;
    default:
        return (mask & 0xFF) == 0;
    }
    return (mask & bit) == 0;
}
#endif
