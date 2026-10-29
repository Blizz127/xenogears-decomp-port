/* Concrete five-word void motion-updater bridge checks.
 * This fixture includes the unmodified production runtime source. */
#include <stdint.h>
static int runtime_read(void *,uint32_t,unsigned,uint32_t *);
static int runtime_write(void *,uint32_t,unsigned,uint32_t);
#ifndef BATTLE_RUNTIME_SOURCE
#error "Use the test runner to produce the bus-observed runtime snapshot"
#endif
#include BATTLE_RUNTIME_SOURCE
#include <openssl/sha.h>
uint8_t g_PsxRam[PSX_RAM_SIZE],g_PsxScratchpad[4096];
int g_GPUDisabledState;unsigned g_PcPortPresentedFrames;
static uint32_t cop_data[32],cop_control[32];
unsigned int MFC2(int r){return cop_data[r&31];}
unsigned int CFC2(int r){return cop_control[r&31];}
void MTC2(unsigned int v,int r){cop_data[r&31]=v;}
void CTC2(unsigned int v,int r){cop_control[r&31]=v;}
int doCOP2(int op){(void)op;abort();}
void ControllerPushState(void){}
void ControllerPoll(void){}
void PsyX_UpdateInput(void){}
int PsyX_Sys_GetVBlankCount(void){return 0;}
int PcPort_QuickCheckpointFieldIsActive(void){return 0;}
char PsyX_BeginScene(void){abort();}
void ClearSplits(void){abort();}
void DrawAllSplits(void){abort();}
void ParsePrimitivesLinkedList(u_long *p,int s){(void)p;(void)s;abort();}
static BattleMipsRuntime rt;
static uint32_t want_args[5];
static unsigned called;
static void motion_probe(uint32_t a0,uint32_t a1,uint32_t a2,uint32_t a3,uint32_t a4)
{
 uint32_t got[]={a0,a1,a2,a3,a4};called++;
 if(memcmp(got,want_args,sizeof(got))||!rt.bridge_cpu||g_ActiveBattleRuntime!=&rt){fputs("MOTION ABI FAIL raw arguments/context\n",stderr);exit(1);}
}
static void fail(const char *why)
{fprintf(stderr,"MOTION ABI FAIL %s\n",why);exit(1);}
int main(void)
{
 const uint32_t values[]={0,1,UINT32_MAX,0x800c3d0cu,0xa0101234u,0x1f800020u,0x80310000u,0x12345678u};unsigned total=0;
 for(unsigned cached=0;cached<2;cached++)for(unsigned value=0;value<8;value++)for(unsigned invalid=0;invalid<4;invalid++){
  memset(&rt,0,sizeof(rt));memset(g_PsxRam,0xa5,sizeof(g_PsxRam));rt.initialized=1;g_ActiveBattleRuntime=&rt;
  rt.functions[0]=(ResolvedFunction){0x800aad54u,motion_probe,"func_800AAD54"};rt.function_count=1;
  if(cached){BridgeCallCacheSlot *p=bridge_cache_slot(&rt,0x800aad54u);p->target=0x800aad54u;p->state=1;p->entry=rt.functions[0];}
  PcPortMipsCpu cpu;initialize_cpu(&cpu,&rt);
  for(unsigned i=1;i<32;i++)cpu.gpr[i]=0x12340000u+i;
  cpu.gpr[29]=0x801fff00u;cpu.gpr[31]=BATTLE_HALT_PC;cpu.hi=0xabcdef01u;cpu.lo=0x23456789u;
  for(unsigned i=0;i<5;i++){want_args[i]=values[(i+value)%8];if(i<4)cpu.gpr[4+i]=want_args[i];}
  assert(runtime_write(&rt,cpu.gpr[29]+0x10u,4,want_args[4])==0);
  if(invalid==1){cpu.load_valid=1;cpu.load_reg=5;cpu.load_value=0xabcdefu;}
  if(invalid==2)cpu.gpr[29]=0x1e000000u;
  if(invalid==3)cpu.gpr[29]++;
  called=0;PcPortMipsCpu before=cpu;PcPortMipsExecutionStats stats=PcPortMipsExecutionStatsGet();
  int rc=runtime_bridge(&rt,&cpu,0x800aad54u);
  if(invalid){if(rc>=0||called||memcmp(cpu.gpr,before.gpr,sizeof(cpu.gpr)))fail("invalid shape entered callee or changed registers");}
  else{before.gpr[3]=0;if(rc!=1||called!=1||memcmp(cpu.gpr,before.gpr,sizeof(cpu.gpr)))fail("void result/register preservation");}
  if(cpu.steps||rt.bridge_cpu||cpu.hi!=before.hi||cpu.lo!=before.lo||memcmp(cpu.cp0,before.cp0,sizeof(cpu.cp0))||PcPortMipsExecutionStatsGet().interpreted_instructions!=stats.interpreted_instructions)fail("CPU state/fallback");
  total++;
 }
 printf("MOTION ABI PASS %u resolved/cached five-word void calls and pending/malformed-frame refusals; native instructions=0\n",total);
 return 0;
}
