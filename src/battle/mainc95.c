#include "common.h"


#ifndef XENO_PC_PORT
#include "psyq/libgte.h"

extern void func_80021B04(void* v, s16 x, s16 y, s16 z);

/* func_800B6E84.s: rotate the actor's position vector (+0xC, a VECTOR)
 * about z by the signed byte args[0] * 16 (4096 = one turn).
 * func_80021B04 sets the SVECTOR to (0, 0, angle), RotMatrix builds the
 * matrix, and ApplyMatrixLV's result is copied back field by field. */
void func_800B6E84(u8* actor, u8* args) {
    SVECTOR rot;
    MATRIX m;
    VECTOR out;

    func_80021B04(&rot, 0, 0, (s8)args[0] * 16);
    RotMatrix(&rot, &m);
    ApplyMatrixLV(&m, (VECTOR*)(actor + 0xC), &out);
    *(s32*)(actor + 0xC) = out.vx;
    *(s32*)(actor + 0x10) = out.vy;
    *(s32*)(actor + 0x14) = out.vz;
}
#include "battle/shard.h"

extern u8 D_800C3EB0[];
extern s32 func_80021AD8(s32 c, s32 delta);

/* Timer callback: count the frame, then for every shard past its start
 * delay fade the colour (func_80021AD8, -6 per channel), integrate the
 * position by the integer part of the velocity and the rotation by its
 * delta, damp the velocity by 1/16 and add gravity to its y. */
void func_800B6F0C(u8* task) {
    SVECTOR v;
    BattleShardSet* set;
    BattleShard* e;
    BattleShardPoly* prim;
    u8* base;
    s32 k;
    s32 j;
    s32 i;

    set = *(BattleShardSet**)(task + 4);
    set->frame++;
    for (k = 0; k != 2; k++) {
        for (j = 0; j != 14; j++) {
            for (i = 0; i != 20; i++) {
                e = &set->grid[k][j][i];
                if (e->delay != 0) {
                    e->delay--;
                } else {
                    base = (u8*)(u32)D_800C3EB0;
                    prim = &e->prims[*(s32*)(base + 0x8000 + 0xC84)];
                    prim->r0 = func_80021AD8(prim->r0, -6);
                    prim->g0 = func_80021AD8(prim->g0, -6);
                    prim->b0 = func_80021AD8(prim->b0, -6);
                    v.vx = e->vel.vx >> 16;
                    v.vy = e->vel.vy >> 16;
                    v.vz = e->vel.vz >> 16;
                    e->pos.vx += v.vx;
                    e->pos.vy += v.vy;
                    e->pos.vz += v.vz;
                    e->rot.vx += e->drot.vx;
                    e->rot.vy += e->drot.vy;
                    e->rot.vz += e->drot.vz;
                    e->vel.vy -= e->vel.vy / 16;
                    e->vel.vx -= e->vel.vx / 16;
                    e->vel.vz -= e->vel.vz / 16;
                    e->vel.vy += e->gravity;
                }
            }
        }
    }
}
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
extern void func_800245D8(u32 a0, u32 a1);
#ifndef XENO_PC_PORT
extern u8 D_800C3EB0[];
#endif
extern u32 WorkListsAddTasks(u32 a0, u32 a1, void* a2, void* a3, void* a4);
extern void func_800B7424(u32 p);
extern void func_800B7364(void);
extern void func_800B6F0C(u8* task);
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


/* func_800B7134.s */
extern u32 g_GfxCurOT[];
#ifndef XENO_PC_PORT
extern u32 D_800C3CB4[];
#endif
extern void func_800B7160(void);
void func_800B7134(void) {
    D_800C3CB4[0] = g_GfxCurOT[0];
    func_800B7160();
}
