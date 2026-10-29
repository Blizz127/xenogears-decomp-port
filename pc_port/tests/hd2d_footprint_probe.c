/* In-game production sprite footprint probe. Never accepts a declined part. */
#include "common.h"
#include "psyq/libgpu.h"
#include "hd2d_sprite.h"
#include "hd2d_integration.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hd2d_footprint_alpha.h"
extern unsigned g_PcPortPresentedFrames;
extern int g_PlayerActorIndex;
static FILE *parts;
static unsigned accepted;
static unsigned long long accepted_mask;
static int expected_map=14, expected_actor=-1, expected_frame=190, expected_parts=6;
static unsigned expected_fnv=0xdb7d0105u;
static int configured;
static long parameter(const char *name,long fallback,long min,long max) {
    const char *value=getenv(name); char *end; long n;
    if(!value) return fallback;
    n=strtol(value,&end,0);
    if(!*value || *end || n<min || n>max) {
        fprintf(stderr,"[hd2d-footprint] FAIL invalid parameter %s\n",name);
        return fallback;
    }
    return n;
}
static void configure(void) {
    if(configured) return;
    configured=1;
    expected_map=parameter("XENO_HD2D_FOOTPRINT_MAP",14,0,1023);
    expected_actor=parameter("XENO_HD2D_FOOTPRINT_ACTOR",-1,-1,65535);
    expected_frame=parameter("XENO_HD2D_FOOTPRINT_FRAME",190,0,32767);
    expected_parts=parameter("XENO_HD2D_FOOTPRINT_PARTS",6,1,64);
    expected_fnv=(unsigned)parameter("XENO_HD2D_FOOTPRINT_FNV",0xdb7d0105u,0,0xfffffffful);
}
static float alpha_box[4];
static int mode(void) { return ((g_PcPortPresentedFrames+1)/8)%3; }
static void fail(const char *message) {
    fprintf(stderr,"[hd2d-footprint] FAIL %s present=%u\n",message,g_PcPortPresentedFrames+1);
    if(parts) fflush(parts);
    /* Never exit while the renderer holds graphics locks. Driver checks log. */
}
static int target(const PcPortHd2dSpriteInfo *i) {
    configure();
    return mode()==1 && PcPort_Hd2dOpeningActive() &&
        PcPort_Hd2dCurrentMap()==expected_map &&
        i->actor==(expected_actor<0 ? g_PlayerActorIndex : expected_actor) &&
        i->identity==expected_fnv && i->frame==expected_frame;
}
extern int __real_PcPort_Hd2dSpriteBegin(const PcPortHd2dSpriteInfo*);
int __wrap_PcPort_Hd2dSpriteBegin(const PcPortHd2dSpriteInfo *i) {
    PcPort_Hd2dSetEnabled(mode()==1);
    int r=__real_PcPort_Hd2dSpriteBegin(i);
    if(target(i)) {
        /* The b84 retail/hidden cycle is player-specific. A nonplayer
         * target needs a separate hide producer and must never pass here. */
        if(i->actor!=g_PlayerActorIndex) fail("target actor is not cycle player");
        accepted=0; accepted_mask=0;
        alpha_box[0]=alpha_box[1]=1e9f; alpha_box[2]=alpha_box[3]=-1e9f;
        if(!r) fail("production Begin declined active mapped player");
    }
    return r;
}
extern int __real_PcPort_Hd2dSpriteDraw(const PcPortHd2dSpriteInfo*,int,const PcPortHd2dRetailQuad*);
int __wrap_PcPort_Hd2dSpriteDraw(const PcPortHd2dSpriteInfo *i,int part,const PcPortHd2dRetailQuad *q) {
    POLY_FT4 before=*(POLY_FT4*)q->poly;
    PcPortHd2dRetailQuad original=*q;
    int r=__real_PcPort_Hd2dSpriteDraw(i,part,q);
    if(!target(i)) return r;
    if(!r) { fail("production Draw declined active mapped player"); return 0; }
    AddSilhouette(&before,alpha_box);
    POLY_FT4 after=*(POLY_FT4*)q->poly;
    before.tag=after.tag;
    int exact=!memcmp(&before,&after,sizeof before) && !memcmp(&original,q,sizeof original);
    if(!exact) fail("retail packet or OT argument changed");
    if(!parts) { const char *path=getenv("XENO_HD2D_FOOTPRINT_PART_LOG"); if(!path || !(parts=fopen(path,"w"))) { fail("cannot open lane part log"); return r; } }
    fprintf(parts,"present=%u mode=1 cell=0 part=%d ot=%lx quad=%d,%d,%d,%d,%d,%d,%d,%d fnv=%08x frame=%d actor=%d accepted=1 packet_exact=%d\n",
        g_PcPortPresentedFrames+1,part,(unsigned long)q->ot,q->x[0],q->y[0],q->x[1],q->y[1],q->x[2],q->y[2],q->x[3],q->y[3],i->identity,i->frame,i->actor,exact);
    if(part<0 || part>=expected_parts) fail("accepted part outside exact requested set");
    else {
        unsigned long long bit=1ull<<part;
        if(accepted_mask&bit) fail("duplicate accepted part");
        accepted_mask|=bit;
    }
    accepted++;
    return r;
}
extern void __real_PcPort_Hd2dSpriteEnd(const PcPortHd2dSpriteInfo*);
void __wrap_PcPort_Hd2dSpriteEnd(const PcPortHd2dSpriteInfo *i) {
    __real_PcPort_Hd2dSpriteEnd(i);
    if(target(i)) {
        if(accepted!=(unsigned)expected_parts || accepted_mask!=(expected_parts==64 ? ~0ull : (1ull<<expected_parts)-1)) fail("unexpected production part set");
        if(parts) {
            if(alpha_box[2]<=alpha_box[0] || alpha_box[3]<=alpha_box[1]) fail("missing runtime retail alpha bounds");
            else fprintf(parts,"present=%u box=%.3f,%.3f,%.3f,%.3f source=runtime_retail_vram\n",g_PcPortPresentedFrames+1,alpha_box[0],alpha_box[1],alpha_box[2],alpha_box[3]);
            fflush(parts);
        }
    }
}
extern void __real_PcPort_Hd2dPostProcess(int,int,int);
void __wrap_PcPort_Hd2dPostProcess(int p,int w,int h) {
    __real_PcPort_Hd2dPostProcess(p,w,h);
}
extern int __real_XgHd2d_Present(const XgHd2dConfig*,const XgHd2dFrame*);
int __wrap_XgHd2d_Present(const XgHd2dConfig *config,const XgHd2dFrame *frame) {
    XgHd2dConfig isolated=*config;
    for(int i=0;i<XG_HD2D_EFFECT_COUNT;++i) isolated.effect[i].enabled=0;
    return __real_XgHd2d_Present(&isolated,frame);
}

extern int __real_PcPort_Hd2dSpriteTextureOverride(const void*,int,int,int,int,int,int,unsigned*,float*);
extern void PcPort_Hd2dHeadCropGpuObserved(const void*,int,unsigned) __attribute__((weak));
int __wrap_PcPort_Hd2dSpriteTextureOverride(const void *packet,int tp,int cl,int u0,int v0,int u1,int v1,unsigned *tex,float *uv) {
    int r=__real_PcPort_Hd2dSpriteTextureOverride(packet,tp,cl,u0,v0,u1,v1,tex,uv);
    if(PcPort_Hd2dHeadCropGpuObserved) PcPort_Hd2dHeadCropGpuObserved(packet,r,r ? *tex : 0);
    if(parts && r) fprintf(parts,"present=%u gpu_accepted=1 mode=%d texture=%u\n",g_PcPortPresentedFrames+1,mode(),*tex);
    return r;
}
