#define _POSIX_C_SOURCE 200809L
#include "hd2d_sprite_parts.h"
#include "hd2d_head_crop_variants.h"
#include "hd2d_map1_head_crop_variants.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
unsigned g_PcPortPresentedFrames;
static int enabled,calls;
static PcPortHd2dPartMapping received;
static const PcPortHd2dRetailQuad *received_quad;
int PcPort_Hd2dEnabled(void) { return enabled; }
int __real_PcPort_Hd2dSpriteSubmitPart(const PcPortHd2dPartProfile *p,
    const PcPortHd2dPartMapping *m,const PcPortHd2dRetailQuad *q)
{ (void)p;calls++;received=*m;received_quad=q;return 1; }
int __wrap_PcPort_Hd2dSpriteSubmitPart(const PcPortHd2dPartProfile*,const PcPortHd2dPartMapping*,const PcPortHd2dRetailQuad*);
void PcPort_Hd2dHeadCropGpuObserved(const void*,int,unsigned);
int main(void)
{
    PcPortHd2dPartMapping m={0xdb7d0105u,14,1,190,5,26,14353,
        {4,15,4,15},{34,34,53,53},100,37,78,120},original=m;
    PcPortHd2dPartProfile p={1254,1254,&m,1,"pc_port/assets/hd2d/fei-octopath-v2.rgba"};
    PcPortHd2dRetailQuad q={{149,161,149,161},{85,85,105,105},
        {4,15,4,15},{34,34,53,53},26,14353,(void*)1,(void*)2},original_quad=q;
    assert(!__wrap_PcPort_Hd2dSpriteSubmitPart((void*)1,(void*)1,(void*)1));assert(!calls);
    assert(!setenv("XENO_HD2D_HEAD_CROP_VARIANTS","1",1));enabled=1;
    for(unsigned i=0;i<HD2D_HEAD_CROP_VARIANT_COUNT;++i) {
        const Hd2dHeadCropVariant *v=hd2d_head_crop_variants+i;
        assert(v->x+v->width<=313 && v->y+v->height<=348);
        g_PcPortPresentedFrames=i*24+8-1;
        assert(Hd2dHeadCropVariantIndex(g_PcPortPresentedFrames+1)==i);
        assert(__wrap_PcPort_Hd2dSpriteSubmitPart(&p,&m,&q));
        assert(received.x==v->x && received.y==v->y && received.width==v->width && received.height==v->height);
        received.x=m.x;received.y=m.y;received.width=m.width;received.height=m.height;
        assert(!memcmp(&received,&m,sizeof m));assert(received_quad==&q);
        assert(!memcmp(&m,&original,sizeof m) && !memcmp(&q,&original_quad,sizeof q));
    }
    PcPort_Hd2dHeadCropGpuObserved((void*)3,1,18); /* unrelated pointer */
    PcPort_Hd2dHeadCropGpuObserved(q.poly,0,18);
    PcPort_Hd2dHeadCropGpuObserved(q.poly,1,0);
    PcPort_Hd2dHeadCropGpuObserved(q.poly,1,18); /* exact head consumed */
    PcPort_Hd2dHeadCropGpuObserved(q.poly,1,18); /* duplicate does not recount */
    m.map=1;
    for(unsigned i=0;i<8;i++) for(int part=2;part<=5;part+=3) {
        const unsigned short *rect=part==2 ? hd2d_map1_head_variants[i].p2 : hd2d_map1_head_variants[i].p5;
        m.part=part; original=m;
        g_PcPortPresentedFrames=i*24+7;
        assert(__wrap_PcPort_Hd2dSpriteSubmitPart(&p,&m,&q));
        assert(received.x==rect[0] && received.y==rect[1] && received.width==rect[2] && received.height==rect[3]);
        assert(rect[0]+rect[2]<=313 && rect[1]+rect[3]<=348);
        assert(!memcmp(&m,&original,sizeof m) && !memcmp(&q,&original_quad,sizeof q));
    }
    m.part=3;assert(__wrap_PcPort_Hd2dSpriteSubmitPart(&p,&m,&q));assert(!memcmp(&received,&m,sizeof m));
    m.frame=191;assert(__wrap_PcPort_Hd2dSpriteSubmitPart(&p,&m,&q));assert(!memcmp(&received,&m,sizeof m));
    return 0;
}
