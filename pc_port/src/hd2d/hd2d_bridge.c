/* Landed hook integration. New master OFF never touches game or GL state. */
#include "hd2d_integration.h"
#include "hd2d_scene_buffers.h"
#include "hd2d_sprite.h"
#include "port_dev_menu.h"
#include "quick_checkpoint.h"
#include <GL/gl.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

extern int g_GameSceneMapNum;
/* Numeric controls use the port menu API; the weak reference also supports
 * standalone fixtures and hosts that expose only the toggle registry. */
extern void PcPort_DevMenuRegisterEnhancementValue(const char*,int(*)(void),
    void(*)(int),int,int,int,const char*) __attribute__((weak));
extern void PcPort_Hd2dSpritePresentDone(void) __attribute__((weak));
extern void PcPort_Hd2dSpriteReset(void) __attribute__((weak));
/* Optional passive test observer; absent in ordinary game launches. */
extern void PcPort_Hd2dSceneObserve(int phase,int path,int width,int height,
    const XgHd2dFrame *frame,int result) __attribute__((weak));
static XgHd2dConfig config;
static int initialized;
extern void PcPort_Hd2dWorldReset(void) __attribute__((weak));
static uint64_t scene_epoch=1;
static int scene_observed,scene_map,scene_field,scene_scope;

static void scene_advance(void)
{
    if(++scene_epoch==0)scene_epoch=1;
}
static void scene_reset(void)
{
    scene_advance();
    scene_observed=0;
    PcPort_Hd2dSceneBuffersReset();
    if(PcPort_Hd2dWorldReset)PcPort_Hd2dWorldReset();
}

static void initialize(void)
{
    if (initialized) return;
    initialized=1;
    XgHd2d_DefaultConfig(&config);
    /* Reviewed field-model scope; map4's establishing scene and map2 are
     * reachable on native normal boot through the same non-scripted path.
     * Maps14/1 retain their initial scope while native battle work continues. */
    config.maps[0]=14; config.maps[1]=1; config.maps[2]=2; config.maps[3]=4; config.map_count=4;
    /* First coverage producer supports only the reviewed grading/warm pass. */
    config.effect[XG_HD2D_GRADE].enabled=1;
    config.effect[XG_HD2D_LIGHT].enabled=1;
    /* Diagnostic scope override; positive provenance has its own map gate. */
    const char *maps=getenv("XENO_HD2D_OPENING_MAPS");
    if (maps && *maps) {
        int parsed[XG_HD2D_MAX_MAPS]; unsigned count=0; const char *p=maps;
        while (*p && count<XG_HD2D_MAX_MAPS) {
            char *end; long v=strtol(p,&end,10);
            if (end==p || v<0 || v>=1024 || (*end && *end!=',')) { count=0; break; }
            parsed[count++]=(int)v; p=*end ? end+1 : end;
            if (*end && !*p) { count=0; break; }
        }
        if (!*p && count) PcPort_Hd2dSetOpeningMaps(parsed,count);
    }
    const char *on=getenv("XENO_HD2D_OCTOPATH");
    config.enabled=on && !strcmp(on,"1");
}
int PcPort_Hd2dEnabled(void) { initialize(); return config.enabled; }
void PcPort_Hd2dSetEnabled(int enabled)
{
    initialize(); enabled=!!enabled;
    if(config.enabled==enabled)return;
    config.enabled=enabled;
    scene_reset();
    if(PcPort_Hd2dSpriteReset)
        PcPort_Hd2dSpriteReset(); /* CPU-only packet lifetime invalidation. */
}
const XgHd2dConfig *PcPort_Hd2dConfiguration(void) { initialize(); return &config; }
int PcPort_Hd2dSetIntensity(int e,float value)
{
    initialize();
    if (e<0 || e>=XG_HD2D_EFFECT_COUNT || !isfinite(value) || value<0 || value>1) return 0;
    config.effect[e].intensity=value; return 1;
}
int PcPort_Hd2dSetOpeningMaps(const int *maps,unsigned count)
{
    initialize();
    if (!maps || !count || count>XG_HD2D_MAX_MAPS) return 0;
    for (unsigned i=0;i<count;++i) {
        if (maps[i]<0 || maps[i]>=1024) return 0;
        for (unsigned j=0;j<i;++j) if (maps[i]==maps[j]) return 0;
    }
    if(config.map_count==count && !memcmp(config.maps,maps,count*sizeof(*maps)))return 1;
    memcpy(config.maps,maps,count*sizeof(*maps)); config.map_count=count;
    scene_reset();
    return 1;
}
int PcPort_Hd2dCurrentMap(void) { return g_GameSceneMapNum & 0x3fff; }
uint64_t PcPort_Hd2dSceneEpoch(void)
{
    initialize();
    if(!config.enabled)return scene_epoch;
    int map=PcPort_Hd2dCurrentMap();
    int field=!!PcPort_QuickCheckpointFieldIsActive();
    int scope=0;
    if(field)for(unsigned i=0;i<config.map_count;++i)
        if(config.maps[i]==map){scope=1;break;}
    if(scene_observed &&
       (scene_map!=map||scene_field!=field||scene_scope!=scope))scene_advance();
    scene_observed=1;scene_map=map;scene_field=field;scene_scope=scope;
    /* No reset callbacks here: the registry synchronizes this generation at
     * its own boundary, without recursively entering the epoch getter. */
    return scene_epoch;
}
int PcPort_Hd2dOpeningActive(void)
{
    if(!PcPort_Hd2dEnabled())return 0;
    (void)PcPort_Hd2dSceneEpoch();
    return scene_scope;
}
static void toggle_master(void) { PcPort_Hd2dSetEnabled(!PcPort_Hd2dEnabled()); }
#define CONTROL(NAME,INDEX) \
    static int get_##NAME(void) { initialize(); return config.effect[INDEX].enabled; } \
    static void toggle_##NAME(void) { initialize(); config.effect[INDEX].enabled^=1; } \
    static int get_##NAME##_percent(void) { initialize(); return (int)(config.effect[INDEX].intensity*100.0f+0.5f); } \
    static void set_##NAME##_percent(int value) { if(value>=0 && value<=100) PcPort_Hd2dSetIntensity(INDEX,value/100.0f); }
CONTROL(dof,XG_HD2D_DOF)
CONTROL(bloom,XG_HD2D_BLOOM)
CONTROL(light,XG_HD2D_LIGHT)
CONTROL(shadow,XG_HD2D_SHADOW)
CONTROL(vignette,XG_HD2D_VIGNETTE)
CONTROL(fog,XG_HD2D_FOG)
CONTROL(particles,XG_HD2D_PARTICLES)
CONTROL(grade,XG_HD2D_GRADE)
__attribute__((constructor)) static void register_menu(void)
{
    initialize();
    PcPort_DevMenuRegisterEnhancement("HD-2D (Octopath style)",PcPort_Hd2dEnabled,toggle_master);
    PcPort_DevMenuRegisterEnhancement("Depth of field / tilt shift",get_dof,toggle_dof);
    PcPort_DevMenuRegisterEnhancement("Bloom / glow",get_bloom,toggle_bloom);
    PcPort_DevMenuRegisterEnhancement("Warm lighting",get_light,toggle_light);
    PcPort_DevMenuRegisterEnhancement("Sprite shadows",get_shadow,toggle_shadow);
    PcPort_DevMenuRegisterEnhancement("Vignette",get_vignette,toggle_vignette);
    PcPort_DevMenuRegisterEnhancement("Fog",get_fog,toggle_fog);
    PcPort_DevMenuRegisterEnhancement("Particles",get_particles,toggle_particles);
    PcPort_DevMenuRegisterEnhancement("Colour grading",get_grade,toggle_grade);
    if(PcPort_DevMenuRegisterEnhancementValue) {
#define VALUE(LABEL,NAME) PcPort_DevMenuRegisterEnhancementValue(LABEL,get_##NAME##_percent,set_##NAME##_percent,0,100,5,"%")
        VALUE("Depth of field intensity",dof);
        VALUE("Bloom intensity",bloom);
        VALUE("Warm lighting intensity",light);
        VALUE("Sprite shadow intensity",shadow);
        VALUE("Vignette intensity",vignette);
        VALUE("Fog intensity",fog);
        VALUE("Particle intensity",particles);
        VALUE("Colour grading intensity",grade);
#undef VALUE
    }
}
void PcPort_Hd2dPostProcess(int path,int width,int height)
{
    if(!PcPort_Hd2dEnabled())return;
    XgHd2dFrame frame={0};
    frame.field_active=PcPort_Hd2dOpeningActive();
    frame.field_id=PcPort_Hd2dCurrentMap();
    GLint viewport[4]={0};
    if(frame.field_active&&width>0&&height>0)
        glGetIntegerv(GL_VIEWPORT,viewport);
    frame.x=viewport[0];frame.y=viewport[1];
    frame.width=viewport[2];frame.height=viewport[3];
    /* Every ON invocation expires its offer before any rejection, including
     * path1, out-of-scope fields, invalid drawable sizes and bad viewports. */
    PcPort_Hd2dSceneBuffersConsume(path,width,height,&frame);
    if(PcPort_Hd2dSpritePresentDone)PcPort_Hd2dSpritePresentDone();
    if(!frame.field_active||width<=0||height<=0)return;
    if(frame.x<0||frame.y<0||frame.width<=0||frame.height<=0||
       frame.width>width||frame.height>height||
       frame.x>width-frame.width||frame.y>height-frame.height)return;
    if(PcPort_Hd2dSceneObserve)PcPort_Hd2dSceneObserve(0,path,width,height,&frame,0);
    int result=XgHd2d_Present(&config,&frame);
    if(PcPort_Hd2dSceneObserve)PcPort_Hd2dSceneObserve(1,path,width,height,&frame,result);
}
int PcPort_Hd2dTextureOverride(int tpage,int clut,int u0,int v0,int u1,int v1,
    unsigned *texture,float uv[4])
{
    (void)tpage; (void)clut; (void)u0; (void)v0; (void)u1; (void)v1;
    (void)texture; (void)uv;
    /* No retail-derived area pack or guessed UV association is supplied. */
    return 0;
}
