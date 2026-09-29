#include <assert.h>
#include <stdint.h>
#include <stdio.h>
typedef int32_t s32;
typedef int16_t s16;
typedef uint16_t u16;
typedef uint8_t u8;
typedef struct { s32 vx,vy,vz; } Vec;
typedef struct { Vec position,move,moveModified,rotation,curTriNormal; u16 curYPos,walkmeshTriIds[4]; s16 walkmeshId; s32 curWalkmeshTriMaterial; } ActorData;
typedef struct { struct {s32 x,y,z;} rotation; struct {s32 t[3];} transformMatrix; uintptr_t pSpriteData; } FieldActor;
typedef struct {struct {s32 x,y,z;} position;} SpriteData;
static struct {s32 position[3],rotation[3];} s_loaded;
static int s_restoreReady;
static s16 D_800AFB54=3;
static ActorData actor;
static FieldActor field;
static SpriteData sprite;
static int lookups;
static ActorData* player_actor(FieldActor** f) { *f=&field; return &actor; }
static s16 func_8007B1C4(s16 x,s16 z,s32 layer,s16* out,s32* state) {
 assert(x==696 && z==-803); assert(layer==lookups++);
 out[1]=-18; state[0]=layer+10;state[1]=4096;state[2]=layer+20;
 return layer==0?845:100;
}
static s32 func_80080968(u8* p) { assert(p==(u8*)&actor); assert(actor.walkmeshTriIds[0]==845); return 123; }
#include "checkpoint_restore.inc"
int main(void) {
 s_loaded.position[0]=45677210; s_loaded.position[1]=-1179648;s_loaded.position[2]=-52606129;
 actor.walkmeshTriIds[0]=60; actor.walkmeshTriIds[1]=132;
 field.pSpriteData=(uintptr_t)&sprite;s_restoreReady=1;
 PcPort_QuickCheckpointRestorePlayer();
 assert(lookups==2 && actor.walkmeshTriIds[0]==845 && actor.walkmeshTriIds[1]==100);
 assert(actor.curWalkmeshTriMaterial==123 && actor.curTriNormal.vy==4096);
 assert(actor.position.vx==45677210 && actor.position.vy==-1179648 && actor.position.vz==-52606129);
 assert(sprite.position.x==actor.position.vx && field.transformMatrix.t[2]==-803);
 assert(!s_restoreReady); PcPort_QuickCheckpointRestorePlayer(); assert(lookups==2);
 puts("checkpoint collision restore: PASS");
}
