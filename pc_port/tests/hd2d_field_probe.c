/* Link-only instrumentation: every wrapper calls the production implementation.
 * No profile injection and no enabling of the legacy Fei test cycle. */
#include "common.h"
#include "psyq/libgpu.h"
#include "fei_hd2d.h"
#include "hd2d_integration.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern unsigned g_PcPortPresentedFrames;
extern int g_PlayerActorIndex;
static FILE *logfile;
static int trace_mode(void) {
    const char *p=getenv("XENO_HD2D_TRACE_PARTS"); return p && !strcmp(p,"1");
}
static FILE *log_output(void) {
    if (!logfile) { const char *p=getenv("XENO_HD2D_FIELD_PROBE"); if(p) logfile=fopen(p,"w"); }
    return logfile;
}
extern void PcPort_Hd2dCaptureWarpTick(void) __attribute__((weak));
extern void __real_PcPort_Hd2dPostProcess(int,int,int);
void __wrap_PcPort_Hd2dPostProcess(int p,int w,int h) {
    __real_PcPort_Hd2dPostProcess(p,w,h);
    if(PcPort_Hd2dCaptureWarpTick) PcPort_Hd2dCaptureWarpTick();
    FILE *f=log_output(); if(f) { fprintf(f,"post present=%u map=%d enabled=%d active=%d\n",g_PcPortPresentedFrames+1,PcPort_Hd2dCurrentMap(),PcPort_Hd2dEnabled(),PcPort_Hd2dOpeningActive()); fflush(f); }
}
extern int __real_PcPort_Hd2dSpriteBegin(const PcPortHd2dSpriteInfo*);
int __wrap_PcPort_Hd2dSpriteBegin(const PcPortHd2dSpriteInfo *i) {
    int r=__real_PcPort_Hd2dSpriteBegin(i); FILE *f=log_output();
    if(f) fprintf(f,"begin present=%u map=%d actor=%d player=%d fnv=%08x frame=%d accepted=%d\n",g_PcPortPresentedFrames+1,PcPort_Hd2dCurrentMap(),i->actor,g_PlayerActorIndex,i->identity,i->frame,r);
    /* Tracing may expose declined parts, but must never activate a valid
     * production profile: its Draw could link packets before returning. */
    if (trace_mode() && r) {
        fprintf(stderr,"HD2D TRACE blocked: production Begin accepted; use production ON evidence\n");
        exit(3);
    }
    return trace_mode() ? 1 : r;
}
extern int __real_PcPort_Hd2dSpriteDraw(const PcPortHd2dSpriteInfo*,int,const PcPortHd2dRetailQuad*);
int __wrap_PcPort_Hd2dSpriteDraw(const PcPortHd2dSpriteInfo *i,int part,const PcPortHd2dRetailQuad *q) {
    POLY_FT4 before=*(POLY_FT4*)q->poly;
    int r=__real_PcPort_Hd2dSpriteDraw(i,part,q);
    int exact=!memcmp(&before,q->poly,sizeof before);
    FILE *f=log_output(); if(f) {
        fprintf(f,"draw present=%u actor=%d fnv=%08x frame=%d part=%d production_accepted=%d trace=%d packet_exact=%d\n",g_PcPortPresentedFrames+1,i->actor,i->identity,i->frame,part,r,trace_mode(),exact);
    }
    if(trace_mode() && (r || !exact)) {
        fprintf(stderr,"HD2D TRACE failed: declined production profile changed or accepted packet\n");
        exit(3);
    }
    return trace_mode() ? 0 : r;
}
extern int __real_PcPort_FeiHd2dCapture(PcPortFeiHd2dBatch*,void*,void*);
int __wrap_PcPort_FeiHd2dCapture(PcPortFeiHd2dBatch *b,void *poly,void *ot) {
    POLY_FT4 before=*(POLY_FT4*)poly; int part=b->part;
    int r=__real_PcPort_FeiHd2dCapture(b,poly,ot); POLY_FT4 after=*(POLY_FT4*)poly;
    /* The tag changes when linked; all remaining packet bytes must be exact. */
    before.tag=after.tag;
    FILE *f=log_output(); if(f && b->hookState) {
        fprintf(f,"part present=%u map=%d actor=%d player=%d fnv=%08x frame=%d part=%d ot=%lx quad=%d,%d,%d,%d,%d,%d,%d,%d uv=%u,%u,%u,%u,%u,%u,%u,%u tpage=%u clut=%u accepted=%d packet_exact=%d\n",
            g_PcPortPresentedFrames+1,PcPort_Hd2dCurrentMap(),b->info.actor,g_PlayerActorIndex,b->info.identity,b->info.frame,part,(unsigned long)ot,
            before.x0,before.y0,before.x1,before.y1,before.x2,before.y2,before.x3,before.y3,
            before.u0,before.v0,before.u1,before.v1,before.u2,before.v2,before.u3,before.v3,before.tpage,before.clut,r,!memcmp(&before,&after,sizeof before));
    } return r;
}

extern int __real_PcPort_Hd2dSpriteTextureOverride(const void*,int,int,int,int,int,int,unsigned*,float*);
int __wrap_PcPort_Hd2dSpriteTextureOverride(const void *packet,int tp,int cl,int u0,int v0,int u1,int v1,unsigned *tex,float *uv) {
    int r=__real_PcPort_Hd2dSpriteTextureOverride(packet,tp,cl,u0,v0,u1,v1,tex,uv);
    FILE *f=log_output();
    if(f && r) fprintf(f,"gpu present=%u map=%d accepted=1 texture=%u\n",g_PcPortPresentedFrames+1,PcPort_Hd2dCurrentMap(),*tex);
    return r;
}
