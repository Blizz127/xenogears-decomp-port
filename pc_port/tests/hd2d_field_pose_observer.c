/* Read-only upstream transform evidence for the SAME-INPUT A/B fixture.
 * Link identically into BOTH binaries with --wrap=PcPort_FeiHd2dBegin.
 * XENO_HD2D_FIELD_POSE_FIXTURE=1 enables actor10-only observations.
 * Logs numeric state/hashes, never retail textures or frame-table payloads. */
#include "common.h"
#include "field/actor.h"
#include "fei_hd2d.h"
#include "quick_checkpoint.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern int g_GameSceneMapNum,g_FieldNumActors;
extern unsigned g_PcPortPresentedFrames;
static unsigned hash(const void *data,size_t bytes) {
    const unsigned char *p=data; unsigned h=2166136261u;
    for(size_t i=0;i<bytes;i++) h=(h^p[i])*16777619u;
    return h;
}
extern void __real_PcPort_FeiHd2dBegin(PcPortFeiHd2dBatch*,void*,int);
void __wrap_PcPort_FeiHd2dBegin(PcPortFeiHd2dBatch *batch,void *sprite,int parts) {
    const char *enabled=getenv("XENO_HD2D_FIELD_POSE_FIXTURE");
    if(enabled && !strcmp(enabled,"1") && PcPort_QuickCheckpointFieldIsActive() &&
       (g_GameSceneMapNum&0x3fff)==14 && g_FieldActors && g_FieldNumActors>10 &&
       (uintptr_t)sprite==g_FieldActors[10].pSpriteData && PcPort_Hd2dSpriteIdentity(sprite)==0x8ee75047u) {
        u8 *s=sprite,*b=(u8*)(uintptr_t)*(u32*)(s+0x20);
        u8 *prim=(u8*)(uintptr_t)*(u32*)(b+0x30),*dir=(u8*)(uintptr_t)*(u32*)(b+0x34);
        s16 *rot=(s16*)(b+0x0c);
        fprintf(stderr,"[hd2d-pose-state] present=%u actor=10 frame=%d parts=%d xyz=%d,%d,%d scale=%d angle=%u flags=%08x,%08x,%08x base_offset=%d,%d rotation=%d,%d,%d,%d,%d,%d,%d,%d,%d translation=%d,%d,%d prim_hash=%08x direction_hash=%08x",
            g_PcPortPresentedFrames+1,*(s16*)(s+0x34),parts,*(s32*)s,*(s32*)(s+4),*(s32*)(s+8),*(s16*)(s+0x2c),*(u16*)(s+0x80),
            *(u32*)(s+0x3c),*(u32*)(s+0x40),*(u32*)(s+0xac),(s8)b[0x3c],(s8)b[0x3d],
            rot[0],rot[1],rot[2],rot[3],rot[4],rot[5],rot[6],rot[7],rot[8],
            *(s32*)(b+0x20),*(s32*)(b+0x24),*(s32*)(b+0x28),
            prim && parts>=0 && parts<=63 ? hash(prim,(size_t)parts*0x18):0,dir?hash(dir,64):0);
        fprintf(stderr," flip=%u,%u,%u direction=%d direction_values=",
            (*(u32*)(s+0x3c)>>3)&1,(*(u32*)(s+0x3c)>>4)&1,(*(u32*)(s+0xac)>>2)&1,*(s16*)(s+0x32));
        if(dir) for(int i=0;i<32;i++) fprintf(stderr,"%s%d",i?",":"",((s16*)dir)[i]);
        else fprintf(stderr,"absent");
        FieldActor *actor=&g_FieldActors[10];
        u8 *ad=(u8*)(uintptr_t)actor->pActorData;
        fprintf(stderr," actor_rotation=%d,%d,%d actor_flags=%d actor_status=%d",
            actor->rotation.x,actor->rotation.y,actor->rotation.z,actor->flags,actor->status);
        if(ad) fprintf(stderr," actor_xyz=%d,%d,%d actor_step=%d,%d,%d",
            *(s32*)(ad+0x20),*(s32*)(ad+0x24),*(s32*)(ad+0x28),
            *(s32*)(ad+0x30),*(s32*)(ad+0x34),*(s32*)(ad+0x38));
        fprintf(stderr,"\n");
    }
    __real_PcPort_FeiHd2dBegin(batch,sprite,parts);
}
