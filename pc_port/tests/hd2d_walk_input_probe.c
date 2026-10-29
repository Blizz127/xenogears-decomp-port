/* Gate existing field-test input schedule on an actual settled safe field. */
#include "common.h"
#include "quick_checkpoint.h"
#include <stdio.h>
extern int g_GameSceneMapNum;
extern unsigned g_PcPortPresentedFrames;
extern void __real_PcPort_FieldTestInputMerge(u16*,u16*,u16*);
void __wrap_PcPort_FieldTestInputMerge(u16 *held,u16 *pressed,u16 *released) {
    static unsigned first_safe;
    static int ready;
    if((g_GameSceneMapNum&0x3fff)!=1 || !PcPort_QuickCheckpointFieldIsSafe()) return;
    if(!first_safe) first_safe=g_PcPortPresentedFrames+1;
    if(!ready && g_PcPortPresentedFrames+1-first_safe<120) return;
    if(!ready) {
        ready=1;
        fprintf(stderr,"[capture-walk] settled-active map=1 present=%u\n",g_PcPortPresentedFrames+1);
    }
    __real_PcPort_FieldTestInputMerge(held,pressed,released);
}
