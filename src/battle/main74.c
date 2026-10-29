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

extern void func_800A23E8__s(s32 ctx, u8* trk) asm("func_800A23E8");
extern s16 func_800B0B14(s32 a0);
extern s32 func_800B0FF4(s16* a, s16* b);
extern s32 rcos(s32 a);
extern s32 rsin(s32 a);
extern u8* D_800D3368__p[] asm("D_800D3368");
extern u8 D_800C3B84;
extern u8 D_800C3B88;
extern u8 D_800C3B8C;
extern s16 D_800C3B90;
extern s16 D_800C3B94;
extern s16 D_800C3B98;
extern u16 D_800C3B9C;
extern s16 D_800C3BA0;
extern s16 D_800C3BA4;
extern u16 D_800C3BA8;
extern u8* D_800C3BAC__p[] asm("D_800C3BAC");
extern u8 D_800C3EBE[];
extern u8 D_800C3EC0[];
extern u8 D_800C3EC2[];
extern s16 D_800D3354[];
extern s16 D_800D335C[];

/* func_800B026C.s: step the nine tracks D_800C3BAC[0..8] `count` times.  A
 * track has a mode byte (+2), owner id (+3), current x/y/z (+4/+6/+8), a
 * source (+0xA/+0xC/+0xE), a frame (+0x10) and a duration (+0x12).  With
 * mode & 0xF < 2 the goal is actor +0xA's position (D_800D3368, else its
 * D_800C3EB4 record) with y less +0xC, averaged with actor +0xE when that is
 * not negative; otherwise the goal is the literal +0xA/+0xC/+0xE.  Track 7
 * moves the goal by -D_800C3BA4 along angle D_800C3BA0 and takes D_800C3BA8
 * off its y; track 8 adds the offset built from D_800C3B90/94/98/9C, takes
 * x/z back from func_800B0FF4 (against D_800D335C) when it returns nonzero,
 * and caps y at func_800B0B14(a3).  Mode bit 0 selects interpolation over
 * the duration or a per-step move.  Finished tracks 0-6 go back through
 * func_800A23E8, tracks 7 and 8 hold their last frame, and D_800C3B88 notes
 * whether owner D_800C3B84's track moved (1) or finished (2).  The results
 * land in D_800C3B90..D_800C3BA8 and D_800D335C / D_800D3354 through the
 * jtbl_8007079C switch.  `mode & 1` is a switch, so nx/ny/nz may be unset on
 * some path: retail keeps nx in s7 and spills ny/nz for that reason. */
void func_800B026C(s32 ctx, s32 count, s32 a2, s32 a3) {
    s16 pos[3];
    s32 iter;
    s32 k;
    u8* trk;
    u8* cur;
    u8* node;
    s16 nx;
    s16 ny;
    s16 nz;
    u8 mode;
    s16 x;
    s16 y;
    s16 z;
    s16 idx;
    s32 c;
    s32 s;
    s16 h;
    s16 t;
    s16 dur;
    s16 dx;
    s16 dy;
    s16 dz;

    D_800C3B88 = 0;
    for (iter = 0; iter < count; iter++) {
        for (k = 0; k < 9; k++) {
            if (D_800C3BAC__p[k] == NULL) {
                continue;
            }
            trk = D_800C3BAC__p[k];
            cur = trk;
            mode = trk[2];
            if ((mode & 0xF) < 2) {
                idx = *(s16*)(trk + 0xA);
                if (D_800D3368__p[idx] != NULL) {
                    x = *(u16*)(*(u8**)(D_800D3368__p[idx] + 4) + 0x5C);
                    z = *(u16*)(*(u8**)(D_800D3368__p[idx] + 4) + 0x64);
                    y = *(u16*)(*(u8**)(D_800D3368__p[idx] + 4) + 0x60) - *(u16*)(trk + 0xC);
                } else {
                    x = *(u16*)(D_800C3EBE + idx * 0x1C);
                    z = *(u16*)(D_800C3EC0 + idx * 0x1C);
                    y = *(u16*)(D_800C3EC2 + idx * 0x1C) - *(u16*)(trk + 0xC);
                }
                idx = *(s16*)(cur + 0xE);
                if (idx >= 0) {
                    if (D_800D3368__p[idx] != NULL) {
                        node = *(u8**)(D_800D3368__p[idx] + 4);
                        x = (x + *(s32*)(node + 0x5C)) / 2;
                        y = (y + *(s32*)(node + 0x60) - *(s16*)(cur + 0xC)) / 2;
                        z = (z + *(s32*)(node + 0x64)) / 2;
                    } else {
                        x = (x + *(u16*)(D_800C3EBE + idx * 0x1C)) / 2;
                        y = (y + *(u16*)(D_800C3EC2 + idx * 0x1C) - *(s16*)(cur + 0xC)) / 2;
                        z = (z + *(u16*)(D_800C3EC0 + idx * 0x1C)) / 2;
                    }
                }
            } else {
                x = *(u16*)(trk + 0xA);
                y = *(u16*)(trk + 0xC);
                z = *(u16*)(trk + 0xE);
            }
            if (k == 7) {
                y -= D_800C3BA8;
                x += (-D_800C3BA4 * rcos(D_800C3BA0)) / 4096;
                z += (-D_800C3BA4 * rsin(D_800C3BA0)) / 4096;
            }
            if (k == 8) {
                c = D_800C3B98 * rcos(D_800C3B94) / 4096;
                s = D_800C3B98 * rsin(D_800C3B94);
                if (s < 0) {
                    s += 0xFFF;
                }
                s >>= 12;
                x += (-s * rcos(D_800C3B90)) / 4096;
                t = y + (-c - D_800C3B9C);
                y = t;
                z += (-s * rsin(D_800C3B90)) / 4096;
                pos[0] = x;
                pos[1] = y;
                pos[2] = z;
                if (func_800B0FF4(D_800D335C, pos) != 0) {
                    x = pos[0];
                    z = pos[2];
                }
                h = func_800B0B14(a3);
                if (t > h) {
                    y = h;
                }
            }
            if (D_800C3B8C != 0 && k >= 7) {
                *(s16*)(cur + 4) = x;
                *(s16*)(cur + 6) = y;
                *(s16*)(cur + 8) = z;
            }
            switch (mode & 1) {
            case 0:
                t = *(s16*)(trk + 0x10) + 1;
                nx = *(s16*)(cur + 4) + (x - *(s16*)(cur + 4)) * t / *(s16*)(trk + 0x12);
                ny = *(s16*)(cur + 6) + (y - *(s16*)(cur + 6)) * t / *(s16*)(trk + 0x12);
                nz = *(s16*)(cur + 8) + (z - *(s16*)(cur + 8)) * t / *(s16*)(trk + 0x12);
                break;
            case 1:
                dur = *(s16*)(trk + 0x12);
                dx = (x - *(s16*)(cur + 4)) / dur;
                dy = (y - *(s16*)(cur + 6)) / dur;
                dz = (z - *(s16*)(cur + 8)) / dur;
                nx = *(s16*)(cur + 4);
                ny = *(s16*)(cur + 6);
                nz = *(s16*)(cur + 8);
                if (dx == 0 && dy == 0 && dz == 0) {
                    *(s16*)(trk + 0x10) = dur;
                } else {
                    nx += dx;
                    ny += dy;
                    nz += dz;
                    *(s16*)(trk + 0x10) = 0;
                }
                *(s16*)(cur + 4) = nx;
                *(s16*)(cur + 6) = ny;
                *(s16*)(cur + 8) = nz;
                break;
            }
            if (++*(s16*)(trk + 0x10) >= *(s16*)(trk + 0x12)) {
                if (k >= 7) {
                    *(s16*)(trk + 0x10) = *(s16*)(trk + 0x12) - 1;
                } else {
                    if (trk[3] == D_800C3B84) {
                        D_800C3B88 |= 2;
                    }
                    func_800A23E8__s(ctx, trk);
                    D_800C3BAC__p[k] = NULL;
                }
            } else if (trk[3] == D_800C3B84) {
                D_800C3B88 |= 1;
            }
            switch (k) {
            case 0:
                D_800C3B90 = nx;
                break;
            case 1:
                D_800C3BA0 = nx;
                break;
            case 2:
                D_800C3B94 = nx;
                break;
            case 3:
                D_800C3B98 = nx;
                break;
            case 4:
                D_800C3BA4 = nx;
                break;
            case 5:
                D_800C3BA8 = nx;
                break;
            case 6:
                D_800C3B9C = nx;
                break;
            case 7:
                D_800D335C[0] = nx;
                D_800D335C[1] = ny;
                D_800D335C[2] = nz;
                break;
            case 8:
                D_800D3354[0] = nx;
                D_800D3354[1] = ny;
                D_800D3354[2] = nz;
                break;
            }
        }
    }
    D_800C3B8C = 0;
}
extern u8* D_800658C8;

s32 func_800B0AB4(void *arg0) {
    register s32 var_v0 asm("$2");
    s16 temp_a0;
    s16 temp_a2;

    temp_a2 = *(s16 *)((s8*)(arg0) + 0);
    var_v0 = 0;
    if ((*(s16 *)((s8*)(D_800658C8) + 0x34E) < temp_a2) && (temp_a2 < *(s16 *)((s8*)(D_800658C8) + 0x34C))) {
        temp_a0 = *(s16 *)((s8*)(arg0) + 4);
        if (temp_a0 > *(s16 *)((s8*)(D_800658C8) + 0x350)) {
            var_v0 = temp_a0 < *(s16 *)((s8*)(D_800658C8) + 0x352);
        }
    }
    return var_v0;
}

extern s32 SquareRoot0(s32 a);
extern s16 D_800C3542;
extern s16 D_800C3544;
extern s16 D_800C3546;
extern s32 func_800A5914(s16* pos, s32 tri, s32 tries);
extern s32 func_800A579C(s16* pos);
extern s32 func_800A5870(s16* pos, s32 tri, s32* out);

/* func_800B0B14.s, byte-exact: probe the walk mesh 512 units from
 * D_800D3354 toward D_800D335C (d = D_800D335C - D_800D3354 with d.y / 4,
 * p = D_800D3354 + d * 512 / (|d| + 1)).  When func_800B0AB4 accepts p
 * (tested as an s16, as retail does) and `id` differs from the cached
 * D_800C3546: cache id and find p's triangle into D_800C3542, first with
 * func_800A5914 from the cached triangle (5 tries), else with
 * func_800A579C's full search.  With a triangle, func_800A5870(p,
 * D_800C3542, out) runs and D_800C3544 caches p.y; otherwise p.y takes
 * the cached D_800C3544.  Returns p.y.  Matches under gcc 2.6.x with the
 * TU's expand-div preset. */
s16 func_800B0B14(s32 id) {
    s32 out[4];
    s16 p[4];
    s16 dx;
    s32 dy;
    s16 dz;
    s32 len;
    s32 x;
    s32 y;
    s32 z;
    s16 tri;

    dx = D_800D335C[0] - D_800D3354[0];
    dy = (D_800D335C[1] - D_800D3354[1]) / 4;
    dz = D_800D335C[2] - D_800D3354[2];
    len = SquareRoot0(dx * dx + dy * dy + dz * dz) + 1;
    x = (dx << 9) / len;
    y = (dy << 9) / len;
    z = (dz << 9) / len;
    tri = -1;
    p[0] = D_800D3354[0] + x;
    p[1] = D_800D3354[1] + y;
    p[2] = D_800D3354[2] + z;
    if ((s16)func_800B0AB4(p) != 0 && id != D_800C3546) {
        D_800C3546 = id;
        tri = func_800A5914(p, D_800C3542, 5);
        if (tri < 0) {
            tri = func_800A579C(p);
        }
        if (tri >= 0) {
            D_800C3542 = tri;
        }
    }
    if (tri >= 0) {
        func_800A5870(p, D_800C3542, out);
        D_800C3544 = p[1];
    } else {
        p[1] = D_800C3544;
    }
    return p[1];
}
extern u8* D_800D3368[];
extern s32 func_800AA600(s32 slot);
extern s32 func_800AA650(s32 slot);

/* func_800B0D70.s, byte-exact: find the actor whose circle holds pos.  For
 * each i < 11 with a D_800D3368 entry whose +0x34 is set: reach =
 * func_800AA650(i) + 0x80; skip unless the actor's +0x60 height minus
 * func_800AA600(i) is below pos[1] and reach beats the best so far.  The
 * circle's centre is the actor's +0x5C/+0x64 position minus dir / 6, and
 * pos must lie within reach of it (len = |pos - centre| + 1).  The best
 * actor's rim point (centre + offset * reach / len) goes to pos[0] /
 * pos[2]; returns its index + 1, or 0 when none qualifies.  Codegen:
 * reach is declared first (retail's spill-slot order), the best-reach
 * comparison is written reach-first, and the func_800AA600 result has its
 * own local.  Matches under gcc 2.6.x with the TU's expand-div preset. */
s32 func_800B0D70(s16* dir, s16* pos) {
    s16 reach;
    s16 best = -1;
    s16 bestReach;
    s16 bestX;
    s16 bestZ;
    s32 i;
    s32 y;
    u8* body;
    s16 cx;
    s16 cz;
    s16 ox;
    s16 oz;
    s32 r;
    s32 len;

    for (i = 0; i < 11; i++) {
        reach = func_800AA650(i) + 0x80;
        if (D_800D3368[i] == NULL || D_800D3368[i][0x34] == 0) {
            continue;
        }
        y = pos[1];
        r = func_800AA600(i);
        body = *(u8**)(D_800D3368[i] + 4);
        if (*(s32*)(body + 0x60) - r >= y) {
            continue;
        }
        if (best >= 0 && reach <= bestReach) {
            continue;
        }
        cx = *(u16*)(body + 0x5C) - dir[0] / 6;
        ox = pos[0] - cx;
        cz = *(u16*)(body + 0x64) - dir[2] / 6;
        oz = pos[2] - cz;
        len = SquareRoot0(ox * ox + oz * oz) + 1;
        if (len >= reach) {
            continue;
        }
        best = i;
        bestReach = reach;
        bestX = cx + ox * reach / len;
        bestZ = cz + oz * reach / len;
    }
    if (best >= 0) {
        pos[0] = bestX;
        pos[2] = bestZ;
        return best + 1;
    }
    return 0;
}
extern s32 func_800B0D70(s16* d, s16* b);

/* func_800B0FF4.s, byte-exact: d = (a - b).xz scaled to 512 / (|d| + 1),
 * then func_800B0D70(d, b), whose result is returned.  Halfwords 1 and 3 of
 * d are never written (func_800B0D70 reads no others). */
s32 func_800B0FF4(s16* a, s16* b) {
    s16 d[4];
    s32 len;

    d[0] = a[0] - b[0];
    d[2] = a[2] - b[2];
    len = SquareRoot0(d[0] * d[0] + d[2] * d[2]) + 1;
    d[0] = (d[0] << 9) / len;
    d[2] = (d[2] << 9) / len;
    return func_800B0D70(d, b);
}
extern s32 SquareRoot0(s32 a);
extern s32 ratan2(s32 y, s32 x);
extern s32 rcos(s32 a);
extern s32 rsin(s32 a);

/* If (x, z) lies within `radius` of actor `index`'s (D_800D3368) +0x5C/+0x64
 * position, push the actor out onto the circle of that radius around
 * (x, z): back along the offset, then sideways by rcos/rsin of the ratan2
 * heading (flipped by 0x800 when the actor's +0x56 facing points the other
 * way) scaled by the overlap. */
void func_800B10EC(s32 index, s32 x, s32 z, s32 radius) {
    s32 dx = x - *(s32*)(*(u8**)(D_800D3368[index] + 4) + 0x5C);
    s32 dz = z - *(s32*)(*(u8**)(D_800D3368[index] + 4) + 0x64);
    s32 dist = SquareRoot0(dx * dx + dz * dz) + 1;
    s32 angle;
    s32 rem;

    if (dist < radius) {
        angle = ratan2(dz, -dx);
        if ((u32)(((*(s16*)(*(u8**)(D_800D3368[index] + 4) + 0x56) - angle) & 0xFFF) - 0x401) < 0x7FF) {
            angle += 0x800;
        }
        rem = radius - dist;
        *(s32*)(*(u8**)(D_800D3368[index] + 4) + 0x5C) = x - dx * radius / dist - rcos(angle) * rem / 4096;
        *(s32*)(*(u8**)(D_800D3368[index] + 4) + 0x64) = z - dz * radius / dist - rsin(angle) * rem / 4096;
    }
}
extern s32 D_800C360C;
typedef struct BattleRec48 {
    u8 b[0x48];
} BattleRec48;
extern BattleRec48 D_800C4000__r[] asm("D_800C4000");

/* func_800B12D0.s, byte-exact: 1 when the command-mask bit for byte `index`
 * of the current actor's (D_800C360C - 1) 0x48-byte record at D_800C4000 is
 * clear in `mask` (kinds 0/1 -> bit 0, 5 -> bit 1, 4 -> bit 2, 2/3 -> bit 3,
 * in retail's case order), or when mask's low byte is 0; otherwise 0.  The
 * switch table is this object's .rodata (jtbl_800707C4, placed by
 * config/battle.yaml).  The record is indexed as a struct array: the flat
 * `[(D_800C360C - 1) * 0x48 + index]` lets gcc fold the -0x48 into the
 * base address (one instruction short). */
u32 func_800B12D0(u32 index, u32 mask) {
    u32 result = 0;

    switch (D_800C4000__r[D_800C360C - 1].b[index]) {
    case 0:
    case 1:
        if (!(mask & 1)) {
            result = 1;
        }
        break;
    case 5:
        if (!(mask & 2)) {
            result = 1;
        }
        break;
    case 4:
        if (!(mask & 4)) {
            result = 1;
        }
        break;
    case 2:
    case 3:
        if (!(mask & 8)) {
            result = 1;
        }
        break;
    }
    if (!(mask & 0xFF)) {
        result = 1;
    }
    return result;
}
extern u32 func_800BF6F8(void);
extern void func_800BE790(void);
extern void func_800B0060(u8* p);
extern void func_8002A498(int channel);
extern int ArchiveDataSync(void);

/* Run battle frames (func_800BE790) until no actor has its +0x38 busy byte
 * set and func_800BF6F8 reports idle; then, if any actor had a +0xC
 * resource, stop archive channel 0 and, once ArchiveDataSync reports no
 * pending read, free every such resource with func_800B0060 (running
 * frames while the read is pending). */
void func_800B136C(void) {
    s32 loaded;
    s32 busy;
    s32 i;
    u8* ent;

    loaded = 0;
    for (;;) {
        busy = 0;
        for (i = 0; i < 11; i++) {
            ent = D_800D3368[i];
            if (ent != NULL) {
                if (ent[0x38] != 0) {
                    busy = 1;
                }
                if (*(s32*)(ent + 0xC) != 0) {
                    loaded = 1;
                }
            }
        }
        if (func_800BF6F8() != 0) {
            busy = 1;
        }
        if (busy == 0) {
            break;
        }
        func_800BE790();
    }
    if (loaded == 0) {
        return;
    }
    func_8002A498(0);
    busy = 1;
    for (;;) {
        if (ArchiveDataSync() == 0) {
            for (i = 0; i < 11; i++) {
                u8* actor = D_800D3368[i];

                if (actor != NULL && *(s32*)(actor + 0xC) != 0) {
                    func_800B0060(actor);
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
