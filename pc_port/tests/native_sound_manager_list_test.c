/* Fixed synthetic host-vs-retail manager list comparisons. Event and voice
 * helpers are spies; no live manager lifetime or concurrent writer is proven. */
#include "slus_oracle.h"
extern void SoundAddAudioManagerToList(void *);
extern int32_t SoundRemoveAudioManagerFromList(void *);
void *g_SoundAudioManagerListHead;
uint32_t g_unk_SoundEvent;
static const Glob globals[]={
 {"manager head",&g_SoundAudioManagerListHead,0x80059564u,8,1},
 {"sound event",&g_unk_SoundEvent,0x800595bcu,4,0},
};
int DisableEvent(uint32_t event) {spy(0x800404b4u,event,0,0,0,0,0);return 1;}
int EnableEvent(uint32_t event) {spy(0x800404a4u,event,0,0,0,0,0);return 1;}
void SoundHandleError(int32_t error) {spy(0x8003f6b0u,(uint32_t)error,0,0,0,0,0);}
void SoundReleaseAllVoices(void *manager) {spy(0x8003b060u,(uint32_t)(uintptr_t)manager,0,0,0,0,0);}
static int service_bridge(PcPortMipsCpu *cpu,uint32_t target)
{
 if(target!=0x800404b4u && target!=0x800404a4u && target!=0x8003f6b0u && target!=0x8003b060u)return 0;
 spy(target,cpu->gpr[4],0,0,0,0,0);cpu->gpr[2]=target==0x800404b4u||target==0x800404a4u?1u:0u;return 1;
}
static void setup_add(uint32_t which)
{
 memset(lo(0x100),0,0x100);memset(lo(0x300),0,0x100);
 g_SoundAudioManagerListHead=which==1?NULL:lo(0x300);g_unk_SoundEvent=7;
 A[0]=lo_addr(0x100);
}
static uint32_t native_add(void) {SoundAddAudioManagerToList((void *)(uintptr_t)A[0]);return 0;}
static void setup_remove(uint32_t which)
{
 memset(lo(0x100),0,0x100);memset(lo(0x300),0,0x100);memset(lo(0x500),0,0x100);
 put32(lo(0x100),lo_addr(0x300));g_SoundAudioManagerListHead=lo(0x100);g_unk_SoundEvent=7;
 A[0]=which==1?lo_addr(0x100):which==2?lo_addr(0x300):which==3?lo_addr(0x500):lo_addr(0x100);
 if(which==4)put16(lo(0x110),0x8000u);
}
static uint32_t native_remove(void) {return (uint32_t)SoundRemoveAudioManagerFromList((void *)(uintptr_t)A[0]);}
int main(void)
{
 const Fn add={"SoundAddAudioManagerToList",0x8003b9e4u,0,setup_add,native_add};
 const Fn remove={"SoundRemoveAudioManagerFromList",0x8003ba38u,1,setup_remove,native_remove};
 cur_what="NATIVE SOUND MANAGER LIST";map_low();load_slus(cur_what);globs=globals;nglobs=sizeof(globals)/sizeof(globals[0]);user_bridge=service_bridge;
 for(unsigned i=1;i<=2;i++) {
  if(run_case(&add,i,0))return 1;
  assert(ntrace==2 && trace[0].id==0x800404b4u && trace[1].id==0x800404a4u);
  assert((uintptr_t)g_SoundAudioManagerListHead==lo_addr(0x100));
  assert(*(uint32_t *)lo(0x100)==(i==1?0u:lo_addr(0x300)));
 }
 for(unsigned i=1;i<=4;i++) {
  if(run_case(&remove,i,0))return 1;
  if(i==3)assert(ntrace==1 && trace[0].id==0x8003f6b0u);
  else if(i==4)assert(ntrace==1 && trace[0].id==0x8003b060u);
  else assert(ntrace==0);
  assert((uintptr_t)g_SoundAudioManagerListHead==(i==1||i==4?lo_addr(0x300):lo_addr(0x100)));
  if(i==2)assert(*(uint32_t *)lo(0x100)==0);
  if(i==4)assert(*(uint16_t *)lo(0x110)==0);
 }
 puts("native sound manager list: fixed synthetic CPU comparisons PASS; helpers simulated");
 return 0;
}
