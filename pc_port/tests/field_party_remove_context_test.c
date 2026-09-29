/* Context and callback contract from retail 8008C180..8008C334.
 * Includes caller/target aliasing and a callback that changes the current
 * ActorData owner, which retail reloads after func_80076AC0 returns. */
#include "common.h"
#include "field/actor.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
s32 D_8005A444[3], g_GamePartyMembers[3], g_GamePartyMemberSkins[3];
void* g_PartyDataBuffers[3];
FieldActor* volatile g_FieldActors;
ActorData* g_FieldScriptVMCurActor;
void* D_800B06B8;
s32 D_800AFD1C, D_800B00C0;
static FieldActor actors[4];
static ActorData data[5], expected[5];
static unsigned char buffer[16];
static int target, caller, redirect, phase, checks;
static u16 old_status;
static void require(int yes, const char* what) {
 if (!yes) { fprintf(stderr,"FAIL %s phase=%d target=%d caller=%d redirect=%d\n",what,phase,target,caller,redirect); exit(1); }
}
void func_80080A74(s32 actor) {
 require(phase==0 && actor==target,"teardown order and argument");
 require(D_800B06B8==&actors[target] && g_FieldScriptVMCurActor==&data[target],"teardown target context");
 require(D_800AFD1C==caller,"teardown retains caller index");
 require((u16)actors[target].status==old_status,"teardown precedes status rewrite");
 phase=1;
}
void func_80076AC0(s32 actor,s32 zero,void* buf,s32 one,s32 z1,s32 z2,s32 final) {
 require(phase==1,"constructor follows teardown");
 require(actor==target && !zero && buf==buffer && one==1 && !z1 && !z2 && final==1,"constructor arguments");
 require(D_800AFD1C==target && D_800B06B8==&actors[target] && g_FieldScriptVMCurActor==&data[target],"constructor target context");
 require((u16)actors[target].status==((old_status&0xf07f)|0x200),"constructor status");
 g_FieldScriptVMCurActor=&data[redirect?4:target];
 g_FieldScriptVMCurActor->scriptInstructionPointer=0x7788;
 phase=2;
}
extern void func_8008C180(s32);
int main(void) {
 g_FieldActors=actors;g_PartyDataBuffers[0]=buffer;
 for(int party=0;party<3;party++) for(int slot=0;slot<4;slot++)
 for(int alias=0;alias<2;alias++) for(int change=0;change<2;change++)
 for(int seed=0;seed<4;seed++) {
  target=slot;caller=alias?slot:(slot+1)%4;redirect=change;phase=0;
  memset(actors,0,sizeof actors);
  for(unsigned i=0;i<sizeof data;i++) ((u8*)data)[i]=(u8)(i*17+seed*53);
  for(int i=0;i<4;i++) actors[i].pActorData=(u32)(uintptr_t)&data[i];
  old_status=(u16)(0x9876+seed);actors[target].status=old_status;
  g_FieldScriptVMCurActor=&data[caller];D_800B06B8=&actors[caller];D_800AFD1C=caller;D_800B00C0=9;
  for(int i=0;i<3;i++) {D_8005A444[i]=i;g_GamePartyMembers[i]=i+10;g_GamePartyMemberSkins[i]=i+20;}
  D_8005A444[party]=slot;
  memcpy(expected,data,sizeof data);
  ActorData* e=&expected[redirect?4:target];
  e->scriptFlags.flags|=0x20001;e->flags|=0x100400;
  e->scriptInstructionPointer=expected[caller].scriptInstructionPointer;
  func_8008C180(party);
  require(phase==2,"both callbacks executed");
  require(g_FieldScriptVMCurActor==&data[caller] && D_800B06B8==&actors[caller] && D_800AFD1C==caller,"caller context restored");
  require(!D_800B00C0,"VM yield cleared");
  require(!memcmp(data,expected,sizeof data),"complete ActorData effects");
  require(D_8005A444[party]==255 && g_GamePartyMembers[party]==255 && g_GamePartyMemberSkins[party]==255,"party slot cleared");
  for(int i=0;i<3;i++) if(i!=party) require(D_8005A444[i]==i && g_GamePartyMembers[i]==i+10 && g_GamePartyMemberSkins[i]==i+20,"other party slots preserved");
  checks++;
 }
 for(int party=0;party<3;party++) {
  phase=0;D_8005A444[party]=255;D_800B00C0=7;
  memcpy(expected,data,sizeof data);ActorData* saved=g_FieldScriptVMCurActor;
  func_8008C180(party);
  require(!phase && D_800B00C0==7 && saved==g_FieldScriptVMCurActor && !memcmp(data,expected,sizeof data),"absent slot has no actor effects");checks++;
 }
 printf("PARTY REMOVE CONTEXT PASS cases=%d\n",checks);
 return 0;
}
