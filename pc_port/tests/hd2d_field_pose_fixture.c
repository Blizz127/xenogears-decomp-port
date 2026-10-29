/* Opt-in SAME-INPUT render fixture, linked identically into no-lane and lane.
 * Freeze only the verified independently animated map14 actor10 package pose.
 * This changes test animation inputs, never crops/masks output or skips draws.
 * Required linker options: --wrap=func_8001D2B0 --wrap=func_8001DAE8.
 * XENO_HD2D_FIELD_POSE_FIXTURE=1 enables. No lane hook is defined here.
 * Evidence: TRACE map14 actor10 FNV8ee75047 frame36, retained in private logs.
 */
#include "common.h"
#include "field/actor.h"
#include "hd2d_sprite.h"
#include "quick_checkpoint.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern int g_GameSceneMapNum, g_FieldNumActors;
extern unsigned g_PcPortPresentedFrames;
static int matches(void *sprite) {
    const char *enabled=getenv("XENO_HD2D_FIELD_POSE_FIXTURE");
    return enabled && !strcmp(enabled,"1") &&
        PcPort_QuickCheckpointFieldIsActive() && (g_GameSceneMapNum&0x3fff)==14 &&
        g_FieldActors && g_FieldNumActors>10 &&
        (uintptr_t)sprite==g_FieldActors[10].pSpriteData &&
        PcPort_Hd2dSpriteIdentity(sprite)==0x8ee75047u;
}
static void log_pose(const char *stage,int original) {
    fprintf(stderr,"[hd2d-pose-fixture] stage=%s present=%u map=14 actor=10 fnv=8ee75047 original=%d supplied=36\n",stage,g_PcPortPresentedFrames+1,original);
}
extern void __real_func_8001D2B0(void*,s16);
void __wrap_func_8001D2B0(void *sprite,s16 frame) {
    if(matches(sprite)) { log_pose("setter",frame); frame=36; }
    __real_func_8001D2B0(sprite,frame);
}
extern void __real_func_8001DAE8(void*,u16,u32);
void __wrap_func_8001DAE8(void *sprite,u16 frame,u32 package) {
    if(matches(sprite)) {
        log_pose("decode",frame);
        frame=36;
        /* Keep the metadata observed by renderer hooks truthful to the exact
         * frame that the real retail decoder is about to materialize. */
        *(s16*)((u8*)sprite+0x34)=36;
    }
    __real_func_8001DAE8(sprite,frame,package);
}
