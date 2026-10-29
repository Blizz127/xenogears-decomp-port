/* Test-only fixed render-input replay; link SAME object into A/B:
 *   -Wl,--wrap=func_8001E298
 * Opt in with XENO_HD2D_FIELD_TRANSFORM_FIXTURE=1; use frame36 pose fixture too.
 * Exact observed state: field-transform-observed/A/run.log present2384.
 * The field actor loop calls this external entry directly (verified PLT32
 * relocations in misc2.c.o), before real matrix/GTE setup and sprite drawing.
 * The actor loop has already chosen its OT index; simulation/OT is not frozen. The sprite's simulation state is restored afterward.
 * No pixels/regions/output samples are removed from comparison.
 */
#include "common.h"
#include "field/actor.h"
#include "hd2d_sprite.h"
#include "quick_checkpoint.h"
#include "psx_memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern int g_GameSceneMapNum,g_FieldNumActors;
extern unsigned g_PcPortPresentedFrames;
extern void __real_func_8001E298(void*,void*);
static u8 *address(u32 p) {
    if((p&0xffe00000u)==0x80000000u || (p&0xffe00000u)==0xa0000000u) return PSX_ADDR(p);
    return (u8*)(uintptr_t)p;
}
static unsigned hash(const void *data,size_t n) {
    const u8 *p=data; unsigned h=2166136261u;
    for(size_t i=0;i<n;i++) h=(h^p[i])*16777619u;
    return h;
}
void __wrap_func_8001E298(void *sprite_ptr,void *ot) {
    const char *enabled=getenv("XENO_HD2D_FIELD_TRANSFORM_FIXTURE");
    u8 *sprite=sprite_ptr;
    if(!enabled || strcmp(enabled,"1") || !PcPort_QuickCheckpointFieldIsActive() ||
       (g_GameSceneMapNum&0x3fff)!=14 || !g_FieldActors || g_FieldNumActors<=10 ||
       (uintptr_t)sprite!=g_FieldActors[10].pSpriteData || PcPort_Hd2dSpriteIdentity(sprite)!=0x8ee75047u) {
        __real_func_8001E298(sprite,ot); return;
    }
    u8 *base=address(*(u32*)(sprite+0x20));
    u8 *prim=address(*(u32*)(base+0x30));
    u8 *dirs=address(*(u32*)(base+0x34));
    if(*(s16*)(sprite+0x34)!=36 || ((*(u32*)(sprite+0x40)>>2)&63)!=3 || !prim || hash(prim,3*0x18)!=0x83a10a6fu) {
        fprintf(stderr,"[hd2d-transform-fixture] unapplied present=%u reason=pose-not-verified\n",g_PcPortPresentedFrames+1);
        __real_func_8001E298(sprite,ot); return;
    }
    u8 saved_sprite[0xb4],saved_matrix[32],saved_dirs[64];
    memcpy(saved_sprite,sprite,sizeof saved_sprite);
    memcpy(saved_matrix,base+0x0c,sizeof saved_matrix);
    if(dirs) memcpy(saved_dirs,dirs,sizeof saved_dirs);
    *(s32*)(sprite+0)=19290611; *(s32*)(sprite+4)=0; *(s32*)(sprite+8)=15600162;
    *(s16*)(sprite+0x2c)=3072; *(s16*)(sprite+0x32)=128; *(u16*)(sprite+0x80)=65422;
    *(u32*)(sprite+0x3c)=0x10000001u; *(u32*)(sprite+0x40)=0x0000030cu; *(u32*)(sprite+0xac)=0x01008000u;
    memset(base+0x0c,0,32);
    ((s16*)(base+0x0c))[0]=3072; ((s16*)(base+0x0c))[4]=3072; ((s16*)(base+0x0c))[8]=3072;
    *(s32*)(base+0x20)=-1092; *(s32*)(base+0x24)=-689; *(s32*)(base+0x28)=4775;
    if(dirs) memset(dirs,0,64);
    fprintf(stderr,"[hd2d-transform-fixture] applied present=%u actor=10 source=A2384 frame=36\n",g_PcPortPresentedFrames+1);
    __real_func_8001E298(sprite,ot);
    memcpy(sprite,saved_sprite,sizeof saved_sprite);
    memcpy(base+0x0c,saved_matrix,sizeof saved_matrix);
    if(dirs) memcpy(dirs,saved_dirs,sizeof saved_dirs);
}
