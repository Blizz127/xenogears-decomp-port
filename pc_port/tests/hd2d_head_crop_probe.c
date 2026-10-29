/* Extra test link wrapper, compatible with existing footprint probe wrappers.
 * Link with --wrap=PcPort_Hd2dSpriteSubmitPart. Never changes production files. */
#include "hd2d_sprite_parts.h"
#include "hd2d_integration.h"
#include "hd2d_head_crop_variants.h"
#include "hd2d_map1_head_crop_variants.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern unsigned g_PcPortPresentedFrames;
static FILE *variant_log;
static const void *registered_head_packet[2];
static unsigned registered_variant[2];
static int registered_part[2];
/* Call from the existing footprint TextureOverride wrapper AFTER its real
 * result, preserving both wrappers' ownership and avoiding duplicate symbols. */
void PcPort_Hd2dHeadCropGpuObserved(const void *packet,int accepted,unsigned texture)
{
    if (!PcPort_Hd2dEnabled()) return;
    if (!accepted || !texture || !variant_log) return;
    for(int slot=0;slot<2;slot++) {
        if(packet!=registered_head_packet[slot]) continue;
        fprintf(variant_log,"present=%u variant=%u part=%d gpu_consumed=1 texture=%u\n",
            g_PcPortPresentedFrames+1,registered_variant[slot],registered_part[slot],texture);
        fflush(variant_log);
        registered_head_packet[slot]=NULL;
    }
}
extern int __real_PcPort_Hd2dSpriteSubmitPart(const PcPortHd2dPartProfile*,
    const PcPortHd2dPartMapping*,const PcPortHd2dRetailQuad*);
int __wrap_PcPort_Hd2dSpriteSubmitPart(const PcPortHd2dPartProfile *p,
    const PcPortHd2dPartMapping *m,const PcPortHd2dRetailQuad *q)
{
    static int requested=-1;
    PcPortHd2dPartMapping tuned;
    unsigned index, next_present;
    const Hd2dHeadCropVariant *variant;
    int result, map1, slot;
    const char *name;
    if (!PcPort_Hd2dEnabled()) return 0;
    if (requested<0) {
        const char *value=getenv("XENO_HD2D_HEAD_CROP_VARIANTS");
        requested=value && !strcmp(value,"1");
    }
    if (!requested || !p || !m || !q || m->identity!=0xdb7d0105u ||
        m->actor!=1 || m->frame!=190 || m->tpage!=26 || m->clut!=14353 ||
        p->width!=1254 || p->height!=1254)
        return __real_PcPort_Hd2dSpriteSubmitPart(p,m,q);
    map1=m->map==1;
    if(map1) {
        if(m->part!=2 && m->part!=5) return __real_PcPort_Hd2dSpriteSubmitPart(p,m,q);
    } else if(m->map!=14 || m->part!=5 || m->x!=100 || m->y!=37 || m->width!=78 || m->height!=120)
        return __real_PcPort_Hd2dSpriteSubmitPart(p,m,q);
    next_present=g_PcPortPresentedFrames+1;
    index=Hd2dHeadCropVariantIndex(next_present);
    variant=hd2d_head_crop_variants+index;
    tuned=*m; slot=m->part==2 ? 0 : 1;
    if(map1) {
        const Hd2dMap1HeadVariant *pair=hd2d_map1_head_variants+index;
        const unsigned short *rect=m->part==2 ? pair->p2 : pair->p5;
        tuned.x=rect[0]; tuned.y=rect[1]; tuned.width=rect[2]; tuned.height=rect[3];name=pair->name;
    } else {
        tuned.x=variant->x;tuned.y=variant->y;
        tuned.width=variant->width;tuned.height=variant->height;name=variant->name;
    }
    result=__real_PcPort_Hd2dSpriteSubmitPart(p,&tuned,q);
    if (!variant_log) {
        const char *path=getenv("XENO_HD2D_HEAD_CROP_VARIANT_LOG");
        if (path) variant_log=fopen(path,"w");
    }
    if (result) { registered_head_packet[slot]=q->poly; registered_variant[slot]=index; registered_part[slot]=m->part; }
    if (variant_log) {
        fprintf(variant_log,"present=%u variant=%u name=%s part=%d rect=%u,%u,%u,%u registered=%d\n",
            next_present,index,name,m->part,tuned.x,tuned.y,tuned.width,tuned.height,result);
        fflush(variant_log);
    }
    return result;
}
