#include "common.h"


#ifdef XENO_PC_PORT
#include "battle_guest_call.h"
#define BATTLE_SPRITE_TABLE ((u8*)BATTLE_HOST_PTR(BATTLE_G32(0x800D2F5Cu)))
#define BATTLE_RENDER_SLOT BATTLE_G32(0x800CCB34u)
#else
extern u8* D_800D2F5C;
extern u32 D_800CCB34;
#define BATTLE_SPRITE_TABLE D_800D2F5C
#define BATTLE_RENDER_SLOT D_800CCB34
#endif
extern s32 func_8002675C(u8*, s32, u8*, u32, s32, s32, s32);
/* func_80076A10.s: draw sprite-table entry `index` of D_800D2F5C into the
 * POLY_FT4 run at `polys` (slot D_800CCB34) at (x, y), scale 0x1000.  The
 * first argument is an entry index, not a pointer: func_8002675C forwards it
 * as its `index` (see main37.c / main40.c callers). */
s32 func_80076A10(s32 index, u8* polys, s16 x, s16 y) {
    return func_8002675C(BATTLE_SPRITE_TABLE, index, polys, BATTLE_RENDER_SLOT,
                         x, y, 0x1000);
}
/* func_80076A6C.s: as func_80076A10 with scale 0x800. */
s32 func_80076A6C(s32 index, u8* polys, s16 x, s16 y) {
    return func_8002675C(BATTLE_SPRITE_TABLE, index, polys, BATTLE_RENDER_SLOT,
                         x, y, 0x800);
}


extern void SetSemiTrans(void* p, s32 v);
extern void SetShadeTex(void* p, s32 v);


/* func_80076AC8.s */
void func_80076AC8(u8* p) {
    SetSemiTrans(p, 1);
    SetShadeTex(p, 0);
}
