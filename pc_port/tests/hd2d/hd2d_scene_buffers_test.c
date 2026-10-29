#include "hd2d_scene_buffers.h"
#include "hd2d_integration.h"
#include "hd2d_sprite.h"
#include <GL/gl.h>
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

int g_GameSceneMapNum=14;
static int field_active=1,field_calls,gl_calls,present_calls;
static int sprite_resets,world_resets,present_done,toggles,values;
static GLint viewport[4]={10,20,320,240};
static XgHd2dFrame last;

int PcPort_QuickCheckpointFieldIsActive(void){field_calls++;return field_active;}
void PcPort_Hd2dSpriteReset(void){sprite_resets++;}
void PcPort_Hd2dWorldReset(void){world_resets++;}
void PcPort_Hd2dSpritePresentDone(void){present_done++;}
void PcPort_DevMenuRegisterEnhancement(const char *label,int(*get)(void),void(*toggle)(void))
{
    assert(label&&get&&toggle);
    if(!toggles)assert(!strcmp(label,"HD-2D (Octopath style)"));
    toggles++;
}
#ifndef HD2D_TEST_NO_VALUE_API
void PcPort_DevMenuRegisterEnhancementValue(const char *label,int(*get)(void),
    void(*set)(int),int min,int max,int step,const char *unit)
{
    assert(label&&get&&set&&min==0&&max==100&&step==5&&!strcmp(unit,"%"));
    assert(get()==25);set(25);values++;
}
#endif
void glGetIntegerv(GLenum key,GLint *out)
{
    assert(key==GL_VIEWPORT);gl_calls++;memcpy(out,viewport,sizeof viewport);
}
int XgHd2d_Present(const XgHd2dConfig *config,const XgHd2dFrame *frame)
{
    assert(config->enabled);present_calls++;last=*frame;
    return XgHd2d_Plan(config,frame).active!=0;
}
/* This obsolete producer must never supply depth or bypass the one-shot API. */
int PcPort_Hd2dSceneInputs(int path,int width,int height,XgHd2dFrame *frame)
{
    (void)path;(void)width;(void)height;(void)frame;assert(0);return 0;
}
static PcPortHd2dSceneBuffers buffers(void)
{
    PcPortHd2dSceneBuffers b={0};
    b.coverage_tex=37;b.drawable_width=640;b.drawable_height=480;
    b.viewport_x=10;b.viewport_y=20;b.viewport_width=320;b.viewport_height=240;
    b.field_id=PcPort_Hd2dCurrentMap();b.epoch=PcPort_Hd2dSceneEpoch();
    b.flags=PC_PORT_HD2D_SCENE_COVERAGE_VALID;
    return b;
}
static XgHd2dFrame frame(void)
{
    XgHd2dFrame f={0};
    f.field_active=1;f.field_id=PcPort_Hd2dCurrentMap();
    f.x=10;f.y=20;f.width=320;f.height=240;
    f.linear_depth_texture=91;f.linear_depth_verified=1;
    f.protection_texture=92;f.protection_verified=1;
    f.depth_mode=XG_HD2D_DEPTH_OT_ORDER;f.order_scale=17;
    f.auxiliary_width=999;f.auxiliary_height=998;
    f.light_count=1;f.lights[0]=(XgHd2dItem){0.2f,0.3f,0.4f,0.5f};
    f.shadow_count=1;f.shadows[0]=(XgHd2dItem){0.6f,0.7f,0.8f,0.9f};
    f.particle_count=1;f.particles[0]=(XgHd2dItem){0.1f,0.2f,2.0f,0.3f};
    return f;
}
static void clear_claims(const XgHd2dFrame *f)
{
    assert(!f->linear_depth_texture&&!f->linear_depth_verified);
    assert(!f->protection_texture&&!f->protection_verified);
    assert(!f->auxiliary_width&&!f->auxiliary_height&&!f->order_scale);
    assert(f->depth_mode==XG_HD2D_DEPTH_LINEAR);
    assert(f->protection_mode==XG_HD2D_PROTECTION_COVERAGE_BITS);
}
static void offer(void)
{
    PcPortHd2dSceneBuffers b=buffers();PcPort_Hd2dSceneBuffersOffer(&b);
}
static void expired(void)
{
    XgHd2dFrame f=frame();
    assert(!PcPort_Hd2dSceneBuffersConsume(0,640,480,&f));clear_claims(&f);
}
static void adapter_cases(void)
{
    XgHd2dFrame f=frame(),before=f;
    PcPortHd2dSceneBuffers b=buffers();PcPort_Hd2dSceneBuffersOffer(&b);
    b.coverage_tex=99; /* Metadata is copied, not retained through this pointer. */
    int old_gl=gl_calls;
    assert(PcPort_Hd2dSceneBuffersConsume(0,640,480,&f));
    assert(gl_calls==old_gl&&f.protection_texture==37&&f.protection_verified);
    assert(!f.linear_depth_texture&&!f.linear_depth_verified&&!f.order_scale);
    assert(f.auxiliary_width==640&&f.auxiliary_height==480);
    assert(f.x==before.x&&f.y==before.y&&f.width==before.width&&f.height==before.height);
    assert(f.field_active==before.field_active&&f.field_id==before.field_id);
    assert(f.light_count==1&&f.shadow_count==1&&f.particle_count==1);
    assert(!memcmp(f.lights,before.lights,sizeof f.lights));
    assert(!memcmp(f.shadows,before.shadows,sizeof f.shadows));
    assert(!memcmp(f.particles,before.particles,sizeof f.particles));
    XgHd2dPlan plan=XgHd2d_Plan(PcPort_Hd2dConfiguration(),&f);
    assert(plan.active==((UINT32_C(1)<<XG_HD2D_GRADE)|(UINT32_C(1)<<XG_HD2D_LIGHT)));
    expired();
    for(unsigned bad=0;bad<28;bad++){
        b=buffers();f=frame();int path=0,width=640,height=480;
        switch(bad){
        case 0:path=1;break;
        case 1:path=-1;break;
        case 2:width=0;break;
        case 3:height=-1;break;
        case 4:b.drawable_width--;break;
        case 5:b.drawable_height--;break;
        case 6:b.viewport_x++;break;
        case 7:b.viewport_y++;break;
        case 8:b.viewport_width--;break;
        case 9:b.viewport_height--;break;
        case 10:b.field_id=1;break;
        case 11:b.epoch--;break;
        case 12:b.epoch=0;break;
        case 13:b.flags=0;break;
        case 14:b.flags|=2;break;
        case 15:b.coverage_tex=0;break;
        case 16:f.field_active=0;break;
        case 17:f.field_id=1;break;
        case 18:f.x=-1;break;
        case 19:f.y=-1;break;
        case 20:f.width=0;break;
        case 21:f.height=0;break;
        case 22:f.width=INT_MAX;break;
        case 23:f.height=INT_MAX;break;
        case 24:f.x=INT_MAX;break;
        case 25:f.y=INT_MAX;break;
        case 26:f.x=321;break;
        default:f.y=241;break;
        }
        PcPort_Hd2dSceneBuffersOffer(&b);
        assert(!PcPort_Hd2dSceneBuffersConsume(path,width,height,&f));
        clear_claims(&f);expired();
    }
    offer();PcPort_Hd2dSceneBuffersOffer(NULL);expired();
    offer();assert(!PcPort_Hd2dSceneBuffersConsume(0,640,480,NULL));expired();
    offer();PcPort_Hd2dSceneBuffersReset();expired();
}
static void epoch_cases(void)
{
    uint64_t epoch=PcPort_Hd2dSceneEpoch();assert(epoch);
    assert(PcPort_Hd2dSceneEpoch()==epoch);
    int world=world_resets,sprite=sprite_resets;
    PcPort_Hd2dSetEnabled(1);assert(PcPort_Hd2dSceneEpoch()==epoch);
    assert(world_resets==world&&sprite_resets==sprite);
    int same[]={14,1,2,4},other[]={14,1,2,4,3},invalid[]={14,14};
    assert(PcPort_Hd2dSetOpeningMaps(same,4));
    assert(PcPort_Hd2dSceneEpoch()==epoch);
    assert(!PcPort_Hd2dSetOpeningMaps(invalid,2));
    assert(!PcPort_Hd2dSetOpeningMaps(NULL,2));
    assert(PcPort_Hd2dSceneEpoch()==epoch);
    offer();assert(PcPort_Hd2dSetOpeningMaps(other,5));
    assert(PcPort_Hd2dSceneEpoch()!=epoch&&world_resets==world+1);expired();
    assert(PcPort_Hd2dSetOpeningMaps(same,4));
    epoch=PcPort_Hd2dSceneEpoch();offer();
    g_GameSceneMapNum=1;assert(PcPort_Hd2dSceneEpoch()!=epoch);
    g_GameSceneMapNum=14;(void)PcPort_Hd2dSceneEpoch();expired();
    epoch=PcPort_Hd2dSceneEpoch();offer();field_active=0;
    assert(!PcPort_Hd2dOpeningActive()&&PcPort_Hd2dSceneEpoch()!=epoch);
    field_active=1;assert(PcPort_Hd2dOpeningActive());expired();
    epoch=PcPort_Hd2dSceneEpoch();offer();PcPort_Hd2dSetEnabled(0);
    assert(PcPort_Hd2dSceneEpoch()!=epoch);
    int fields=field_calls,gl=gl_calls;
    PcPort_Hd2dSceneBuffersOffer((const PcPortHd2dSceneBuffers *)(uintptr_t)1);
    assert(!PcPort_Hd2dSceneBuffersConsume(0,640,480,(XgHd2dFrame *)(uintptr_t)1));
    PcPort_Hd2dPostProcess(0,640,480);
    assert(field_calls==fields&&gl_calls==gl);
    epoch=PcPort_Hd2dSceneEpoch();PcPort_Hd2dSetEnabled(1);
    assert(PcPort_Hd2dSceneEpoch()!=epoch);expired();
}
static void bridge_cases(void)
{
    int done=present_done;
    offer();PcPort_Hd2dPostProcess(0,640,480);
    assert(last.protection_verified&&last.protection_texture==37&&!last.linear_depth_verified);
    PcPort_Hd2dPostProcess(0,640,480);assert(!last.protection_verified);
    offer();PcPort_Hd2dPostProcess(1,640,480);assert(!last.protection_verified);expired();
    offer();int presents=present_calls;viewport[0]=-1;
    PcPort_Hd2dPostProcess(0,640,480);assert(present_calls==presents);
    viewport[0]=10;expired();
    offer();int gl=gl_calls;PcPort_Hd2dPostProcess(0,0,480);
    assert(gl_calls==gl&&present_calls==presents);expired();
    offer();g_GameSceneMapNum=3;PcPort_Hd2dPostProcess(0,640,480);
    assert(gl_calls==gl&&present_calls==presents);
    g_GameSceneMapNum=14;expired();
    offer();field_active=0;PcPort_Hd2dPostProcess(0,640,480);
    assert(gl_calls==gl&&present_calls==presents);
    field_active=1;expired();assert(present_done==done+7);
}
int main(void)
{
    assert(toggles==9);
#ifdef HD2D_TEST_NO_VALUE_API
    assert(!values);
#else
    assert(values==8);
#endif
    assert(!PcPort_Hd2dEnabled());
    const XgHd2dConfig *config=PcPort_Hd2dConfiguration();
    assert(config->map_count==4&&config->maps[0]==14&&config->maps[1]==1&&
           config->maps[2]==2&&config->maps[3]==4);
    for(int e=0;e<XG_HD2D_EFFECT_COUNT;e++)
        assert(config->effect[e].enabled==(e==XG_HD2D_GRADE||e==XG_HD2D_LIGHT));
    assert(!PcPort_Hd2dSetIntensity(-1,0.5f));
    assert(!PcPort_Hd2dSetIntensity(0,NAN));
    uint64_t epoch=PcPort_Hd2dSceneEpoch();assert(epoch&&!field_calls&&!gl_calls);
    PcPort_Hd2dSceneBuffersOffer((const PcPortHd2dSceneBuffers *)(uintptr_t)1);
    assert(!PcPort_Hd2dSceneBuffersConsume(0,640,480,(XgHd2dFrame *)(uintptr_t)1));
    PcPort_Hd2dPostProcess(0,640,480);
    assert(!field_calls&&!gl_calls&&!present_calls&&!present_done);
    PcPort_Hd2dSetEnabled(1);
    assert(PcPort_Hd2dSceneEpoch()!=epoch&&world_resets==1&&sprite_resets==1);
    adapter_cases();epoch_cases();bridge_cases();
    /* Both reachable fields use the same coverage-only, one-shot contract. */
    const int reachable[]={2,4};
    for(unsigned i=0;i<sizeof reachable/sizeof reachable[0];++i){
        g_GameSceneMapNum=reachable[i];assert(PcPort_Hd2dOpeningActive());
        offer();PcPort_Hd2dPostProcess(0,640,480);
        assert(last.field_id==reachable[i]&&last.protection_verified&&last.protection_texture==37);
        assert(!last.linear_depth_verified&&!last.linear_depth_texture);
        PcPort_Hd2dPostProcess(0,640,480);assert(!last.protection_verified);
    }
    /* Removing map4 from configured scope expires its already-issued offer;
     * restoring scope cannot resurrect the old epoch's certificate. */
    const int without_map4[]={14,1,2},defaults[]={14,1,2,4};
    offer();epoch=PcPort_Hd2dSceneEpoch();
    assert(PcPort_Hd2dSetOpeningMaps(without_map4,3));
    assert(!PcPort_Hd2dOpeningActive()&&PcPort_Hd2dSceneEpoch()!=epoch);expired();
    assert(PcPort_Hd2dSetOpeningMaps(defaults,4)&&PcPort_Hd2dOpeningActive());expired();
    offer();PcPort_Hd2dPostProcess(0,640,480);
    assert(last.field_id==4&&last.protection_verified&&!last.linear_depth_verified);
    puts("HD2D SCENE BUFFERS PASS: coverage-only, one-shot expiry, OFF bypass, epoch lifecycle and preserved menus; GL mocked");
    return 0;
}
