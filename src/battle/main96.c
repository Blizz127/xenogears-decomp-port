#include "common.h"


#ifndef XENO_PC_PORT
#include "battle/shard.h"

extern u8 D_800C3EB0[];
extern SVECTOR D_800C3594[];
extern SVECTOR D_800C35AC[];
extern u32 D_800C3CB4[];
extern void AddPrim(void* ot, void* p);

/* Work callback: draw every live shard (position z >= 0x40) of the burst
 * with the 160x112 / 0x200 projection: its own rotation and translation,
 * then RotTransPers3 of the half's three template vertices (D_800C3594 for
 * the first [14][20] half, D_800C35AC for the second) into the current
 * buffer's triangle, added at OT depth z >> 6 of D_800C3CB4.  The caller's
 * geometry offset and screen distance are restored afterwards. */
void func_800B7160(u8* task) {
    s32 ofx;
    s32 ofy;
    s32 p;
    s32 flag;
    BattleShardSet* set;
    s32 h;
    BattleShard* e;
    u8* base;
    BattleShardPoly* prim;
    SVECTOR* v;
    s32 k;
    s32 j;
    s32 i;
    s32 z;

    set = *(BattleShardSet**)(task + 4);
    ReadGeomOffset(&ofx, &ofy);
    h = ReadGeomScreen();
    SetGeomOffset(0xA0, 0x70);
    SetGeomScreen(0x200);
    for (k = 0; k != 2; k++) {
        for (j = 0; j != 14; j++) {
            for (i = 0; i != 20; i++) {
                e = &set->grid[k][j][i];
                base = (u8*)(u32)D_800C3EB0;
                prim = &e->prims[*(s32*)(base + 0x8000 + 0xC84)];
                if (e->pos.vz >= 0x40) {
                    MATRIX m;

                    RotMatrix(&e->rot, &m);
                    TransMatrix(&m, &e->pos);
                    SetRotMatrix(&m);
                    SetTransMatrix(&m);
                    if (k == 0) {
                        v = D_800C3594;
                    } else {
                        v = D_800C35AC;
                    }
                    z = RotTransPers3(&v[0], &v[1], &v[2], (long*)&prim->x0, (long*)&prim->x1, (long*)&prim->x2, &p, &flag) >> 6;
                    AddPrim((u32*)D_800C3CB4[0] + z, prim);
                }
            }
        }
    }
    SetGeomOffset(ofx, ofy);
    SetGeomScreen(h);
}
#endif


extern void DrawSync(s32 mode);
extern void HeapFree(u32 p);


/* func_800B7330.s */
void func_800B7330(u8* p) {
    DrawSync(0);
    HeapFree((u32)p);
}
/* func_800B7364.s */
void func_800B7364(u32 p) {
    WorkListRemoveTask(p + 0x1C);
    TimerWorkListRemoveTask(p);
    func_80025180(p);
}
