#include "common.h"


#ifndef XENO_PC_PORT
typedef struct {
    u16 x;
    u16 y;
    u16 w;
    u16 h;
    u16 curW;
    u16 curH;
    u8 kind;
} BattleWindowRect;

extern u8* D_800D2D28;
extern BattleWindowRect* D_800D2D90[];
extern void func_8008F6E4(u8 kind, u16 x, u16 y, u16 w, s32 h);

/* Open animation of the seven battle windows (D_800D2D90): every window
 * that is shown (D_800D2D28 +0xB8 + i) and not yet open (+0xBF + i) grows its
 * current size by 0x20 per frame up to its full size, is marked open once
 * both axes are full, and is redrawn centred (func_8008F6E4). */
void func_8008FAD8(void) {
    s32 i;
    BattleWindowRect* r;
    u8* f;
    u8 done;

    for (i = 0; i < 7; i++) {
        r = D_800D2D90[i];
        if ((D_800D2D28 + i)[0xB8] != 0 && (D_800D2D28 + i)[0xBF] == 0) {
            done = 0;
            if (r->curW + 0x20 >= r->w) {
                r->curW = r->w;
                done = 1;
            } else {
                r->curW += 0x20;
            }
            if (r->curH + 0x20 >= r->h) {
                r->curH = r->h;
                done++;
            } else {
                r->curH += 0x20;
            }
            if (done == 2) {
                (D_800D2D28 + i)[0xBF] = 1;
            }
            func_8008F6E4(r->kind, r->x + r->w / 2 - r->curW / 2, r->y + r->h / 2 - r->curH / 2, r->curW, r->curH);
        }
    }
}
typedef struct {
    u8 pad[0xFC];
    s32 count;
} M50Ctx;

extern u8* D_800C3EA4;
extern u8* D_800D2F5C;
extern s32 D_800CCB34[];
extern s32 func_80076A10(s32 glyph, u8* prims, s16 x, s16 y);
extern s32 func_800263E4(u8* table, s32 index, void* packets, s32 buffer,
                         s32 x, s32 y, s32 scale, s32 flipX, s32 flipY);

/* func_8008FC1C.s, byte-exact.  x/y/w/h are s16 parameters.
 * - PROMOTE_PROTOTYPES makes each s16 parameter copy a deferred conversion
 *   insn, which sched1 can schedule as a once-set insn.  That lets n's lbu
 *   land after the count store, and dbr fills the beqz slot as in retail.
 * - With s32 parameters and (s16) casts at the uses it is 460 vs 456.
 * The later u32 x5 declaration below is port-only. */
void func_8008FC1C(s16 x, s16 y, s16 w, s16 h, u8 n) {
    s32 i;

    ((M50Ctx*)D_800D2D28)->count = 0;
    for (i = 0; i < n; i++) {
        ((M50Ctx*)D_800D2D28)->count +=
            func_80076A10(0x65, D_800C3EA4 + (((M50Ctx*)D_800D2D28)->count * 80 + 0x1E68), x, h + i * 8);
    }
    ((M50Ctx*)D_800D2D28)->count +=
        func_80076A10(0x64, D_800C3EA4 + (((M50Ctx*)D_800D2D28)->count * 80 + 0x1E68), x, y);
    ((M50Ctx*)D_800D2D28)->count +=
        func_800263E4(D_800D2F5C, 0x64, D_800C3EA4 + (((M50Ctx*)D_800D2D28)->count * 80 + 0x1E68),
                      D_800CCB34[0], x, w, 0x1000, 0, 1);
    D_800D2D28[0xA6] = *(u8*)D_800CCB34;
    D_800D2D28[0x9D] = 1;
}

#endif


#ifdef XENO_PC_PORT
/* Port-only: the retail build sees the byte-exact definition above, whose
 * prototype is (s16, s16, s16, s16, u8). */
extern void func_8008FC1C(u32 a0, u32 a1, u32 a2, u32 a3, u32 a4);
#endif


/* func_8008FDE4.s */
void func_8008FDE4(void) {
    func_8008FC1C(0x20, 0x5C, 0xCC, 0x60, 0xE);
}
