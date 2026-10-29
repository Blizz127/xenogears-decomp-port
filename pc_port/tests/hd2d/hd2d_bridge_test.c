#include "hd2d_integration.h"
#include "hd2d_scene_buffers.h"
#include "hd2d_sprite.h"
#include <GL/gl.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
int g_GameSceneMapNum=3;
static int field_calls,gl_calls,producer_calls,present_calls,registered;
static int field_active=1,world_reset_calls,present_done;
static GLint viewport[4]={0,0,640,480};
static XgHd2dFrame last;
static int reset_calls,values;
#ifndef HD2D_TEST_NO_VALUE_API
static struct { const char *label; int (*get)(void); void (*set)(int); } controls[XG_HD2D_EFFECT_COUNT];
#endif
void PcPort_Hd2dSpriteReset(void) { ++reset_calls; }
void PcPort_Hd2dWorldReset(void) { ++world_reset_calls; }
void PcPort_Hd2dSpritePresentDone(void) { ++present_done; }
void PcPort_DevMenuRegisterEnhancement(const char *label,int (*get)(void),void (*toggle)(void))
{
    assert(label && get && toggle);
    if (!registered) assert(!strcmp(label,"HD-2D (Octopath style)"));
    ++registered;
}
#ifndef HD2D_TEST_NO_VALUE_API
void PcPort_DevMenuRegisterEnhancementValue(const char *label,int(*get)(void),
    void(*set)(int),int min,int max,int step,const char *unit)
{
    assert(label && get && set && values<XG_HD2D_EFFECT_COUNT);
    assert(min==0 && max==100 && step==5 && !strcmp(unit,"%"));
    for(int i=0;i<values;++i) assert(strcmp(label,controls[i].label));
    controls[values].label=label;controls[values].get=get;controls[values++].set=set;
    registered++;
}
#endif
int PcPort_QuickCheckpointFieldIsActive(void) { ++field_calls; return field_active; }
void glGetIntegerv(GLenum key,GLint *out)
{
    assert(key==GL_VIEWPORT); ++gl_calls;
    memcpy(out,viewport,sizeof viewport);
}
int XgHd2d_Present(const XgHd2dConfig *c,const XgHd2dFrame *f)
{
    assert(c->enabled); ++present_calls; last=*f; return 1;
}
int PcPort_Hd2dSceneInputs(int path,int width,int height,XgHd2dFrame *out)
{
    /* Old fabricated provider must not bypass the one-shot coverage API. */
    (void)path;(void)width;(void)height;(void)out;
    ++producer_calls; assert(0); return 0;
}
static PcPortHd2dSceneBuffers buffers(void)
{
    PcPortHd2dSceneBuffers b={0};
    b.coverage_tex=12;b.drawable_width=640;b.drawable_height=480;
    b.viewport_width=640;b.viewport_height=480;
    b.field_id=PcPort_Hd2dCurrentMap();b.epoch=PcPort_Hd2dSceneEpoch();
    b.flags=PC_PORT_HD2D_SCENE_COVERAGE_VALID;
    return b;
}
static void offer(void)
{
    PcPortHd2dSceneBuffers b=buffers();PcPort_Hd2dSceneBuffersOffer(&b);
}
static void no_attachments(void)
{
    assert(!last.linear_depth_verified && !last.linear_depth_texture);
    assert(!last.protection_verified && !last.protection_texture);
}
int main(void)
{
#ifdef HD2D_TEST_NO_VALUE_API
    assert(registered==9 && !values);
#else
    assert(registered==17 && values==8);
    // Numeric changes neither toggle effects nor enable the master, and only
    // touch their own intensity. Percent rounding supports all integer inputs.
    for(int effect=0;effect<XG_HD2D_EFFECT_COUNT;++effect) {
        assert(controls[effect].get()==25);
        for(int pct=0;pct<=100;++pct) {
            XgHd2dConfig before=*PcPort_Hd2dConfiguration();
            controls[effect].set(pct);
            assert(controls[effect].get()==pct);
            const XgHd2dConfig *after=PcPort_Hd2dConfiguration();
            assert(!after->enabled);
            for(int other=0;other<XG_HD2D_EFFECT_COUNT;++other) {
                assert(after->effect[other].enabled==before.effect[other].enabled);
                if(other!=effect) assert(after->effect[other].intensity==before.effect[other].intensity);
            }
        }
        XgHd2dConfig before=*PcPort_Hd2dConfiguration();
        controls[effect].set(-1);controls[effect].set(101);
        assert(!memcmp(&before,PcPort_Hd2dConfiguration(),sizeof before));
        controls[effect].set(25);
    }
#endif
    assert(!PcPort_Hd2dEnabled());
    assert(PcPort_Hd2dConfiguration()->map_count==4 &&
        PcPort_Hd2dConfiguration()->maps[0]==14 &&
        PcPort_Hd2dConfiguration()->maps[1]==1 &&
        PcPort_Hd2dConfiguration()->maps[2]==2 &&
        PcPort_Hd2dConfiguration()->maps[3]==4);
    for(int effect=0;effect<XG_HD2D_EFFECT_COUNT;++effect)
        assert(PcPort_Hd2dConfiguration()->effect[effect].enabled==
               (effect==XG_HD2D_LIGHT || effect==XG_HD2D_GRADE));
    uint64_t epoch=PcPort_Hd2dSceneEpoch();assert(epoch);
    PcPort_Hd2dSceneBuffersOffer((const PcPortHd2dSceneBuffers *)(uintptr_t)1);
    assert(!PcPort_Hd2dSceneBuffersConsume(0,640,480,(XgHd2dFrame *)(uintptr_t)1));
    PcPort_Hd2dPostProcess(0,640,480);
    assert(!field_calls && !gl_calls && !producer_calls && !present_calls && !present_done);
    assert(!PcPort_Hd2dSetIntensity(-1,0.5f));
    assert(!PcPort_Hd2dSetIntensity(0,NAN));
    assert(!PcPort_Hd2dSetIntensity(0,1.1f));
    assert(PcPort_Hd2dSetIntensity(0,0.75f));
    int maps[]={2,4},duplicates[]={2,2},invalid[]={1024};
    assert(!PcPort_Hd2dSetOpeningMaps(duplicates,2));
    assert(!PcPort_Hd2dSetOpeningMaps(invalid,1));
    PcPort_Hd2dSetEnabled(1);
    assert(reset_calls==1 && world_reset_calls==1 && PcPort_Hd2dSceneEpoch()!=epoch);
    epoch=PcPort_Hd2dSceneEpoch();
    PcPort_Hd2dSetEnabled(1);
    assert(reset_calls==1 && world_reset_calls==1 && PcPort_Hd2dSceneEpoch()==epoch);
    PcPort_Hd2dPostProcess(0,640,480);
    assert(!gl_calls && !present_calls); /* unapproved map */
    assert(PcPort_Hd2dSetOpeningMaps(maps,2));
    assert(world_reset_calls==2 && PcPort_Hd2dSceneEpoch()!=epoch);
    g_GameSceneMapNum=12; PcPort_Hd2dPostProcess(0,640,480);
    assert(!gl_calls && !present_calls);
    g_GameSceneMapNum=2;offer();
    PcPort_Hd2dPostProcess(0,640,480);
    assert(present_calls==1 && !last.linear_depth_verified && !last.linear_depth_texture);
    assert(last.protection_verified && last.protection_texture==12);
    assert(last.protection_mode==XG_HD2D_PROTECTION_COVERAGE_BITS);
    assert(last.auxiliary_width==640 && last.auxiliary_height==480);
    PcPort_Hd2dPostProcess(0,640,480); /* An offer is consumed exactly once. */
    assert(present_calls==2);no_attachments();
    offer();
    PcPort_Hd2dPostProcess(1,640,480);
    assert(present_calls==3 && !producer_calls);no_attachments();
    PcPort_Hd2dPostProcess(0,640,480);no_attachments(); /* path1 expired it. */
    PcPortHd2dSceneBuffers wrong=buffers();wrong.field_id=3;
    PcPort_Hd2dSceneBuffersOffer(&wrong);
    PcPort_Hd2dPostProcess(0,640,480);no_attachments(); /* wrong map */
    offer();epoch=PcPort_Hd2dSceneEpoch();
    g_GameSceneMapNum=4;assert(PcPort_Hd2dSceneEpoch()!=epoch);
    g_GameSceneMapNum=2;PcPort_Hd2dPostProcess(0,640,480);
    no_attachments(); /* Returning to the map cannot resurrect an old epoch. */

    int presents=present_calls,calls=gl_calls;
    offer();field_active=0;PcPort_Hd2dPostProcess(0,640,480);
    assert(present_calls==presents && gl_calls==calls);
    field_active=1;PcPort_Hd2dPostProcess(0,640,480);no_attachments();
    offer();presents=present_calls;viewport[0]=-1;
    PcPort_Hd2dPostProcess(0,640,480);assert(present_calls==presents);
    viewport[0]=0;PcPort_Hd2dPostProcess(0,640,480);no_attachments();
    offer();presents=present_calls;calls=gl_calls;
    PcPort_Hd2dPostProcess(0,0,480);
    assert(present_calls==presents && gl_calls==calls);
    PcPort_Hd2dPostProcess(0,640,480);no_attachments();

    offer();calls=gl_calls;PcPort_Hd2dSetEnabled(0);assert(reset_calls==2);
    PcPort_Hd2dPostProcess(0,640,480); assert(gl_calls==calls);
    PcPort_Hd2dSetEnabled(1);assert(reset_calls==3);
    PcPort_Hd2dPostProcess(0,640,480);no_attachments();
    PcPort_Hd2dSetEnabled(0);assert(reset_calls==4);
    unsigned texture=99; float uv[]={1,2,3,4};
    assert(!PcPort_Hd2dTextureOverride(1,2,3,4,5,6,&texture,uv));
    assert(texture==99 && uv[0]==1 && uv[3]==4);
    puts("HD2D bridge: OFF bypass, scope, menus/intensity, coverage-only one-shot offers and epochs PASS");
    return 0;
}
