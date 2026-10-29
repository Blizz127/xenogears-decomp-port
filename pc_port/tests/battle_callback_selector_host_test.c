/* Complete original selector and actual native call/consumer composition.
 * The CPU interpreter is used only for the independent original-code leg. */
#define main callback_selector_model_main
#include "battle_effects_render_host_test.c"
#undef main
static const uint32_t selector_target=0x800aa820u;
static unsigned selector_cases;
extern int32_t selector_consume(uint32_t,int32_t,int32_t,int32_t);
static int selector_read(void *opaque,uint32_t address,unsigned width,uint32_t *value)
{
 (void)opaque;
 if(address-selector_target>=120u||width!=4)fail("unexpected selector memory read",address);
 *value=load_le(battle+(address-0x8006faf0u),4);return 0;
}
static void selector_cpu(PcPortMipsCpu *cpu,uint32_t value)
{
 initialize_cpu(cpu,&rt);
 for(unsigned i=1;i<32;i++)cpu->gpr[i]=0x45670000u+selector_cases*128u+i;
 for(unsigned i=0;i<32;i++)cpu->cp0[i]=0xa8760000u+selector_cases*32u+i;
 cpu->gpr[4]=value;cpu->gpr[29]=0x1e000001u;cpu->gpr[31]=BATTLE_HALT_PC;
 cpu->hi=0x12345678u+selector_cases;cpu->lo=0xfedcba98u-selector_cases;
}
static void selector_case(uint32_t value)
{
 PcPortMipsCpu original,native;selector_cpu(&original,value);original.bus.bridge=NULL;original.bus.read=selector_read;
 int rc=PcPortMipsRun(&original,selector_target,BATTLE_HALT_PC,100);
 if(rc!=PC_PORT_MIPS_HALTED||!original.steps||original.load_valid)fail("original selector result",original.pc);
 selector_cpu(&native,value);PcPortMipsExecutionStats stats=PcPortMipsExecutionStatsGet();
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
 rc=PcPortMipsRun(&native,selector_target,BATTLE_HALT_PC,100);
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
 if(rc!=PC_PORT_MIPS_HALTED||memcmp(original.gpr,native.gpr,sizeof(native.gpr))||memcmp(original.cp0,native.cp0,sizeof(native.cp0))||original.hi!=native.hi||original.lo!=native.lo||native.load_valid||native.steps||rt.bridge_cpu||accesses||!memory_equal()||PcPortMipsExecutionStatsGet().interpreted_instructions!=stats.interpreted_instructions)fail("selector complete state",value);
 /* A second witness also detects returning a process address whose callback
  * would happen to work when passed to the dual-spelling consumer. */
 static const uint32_t guest[]={0x800a3490u,0x800a3514u,0x800a3578u,0x800a35c8u};
 if(native.gpr[2]!=guest[value<=3u?value:0])fail("selector exact guest address",value);
 selector_cases++;
}
static int selector_attempt(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
 PcPortMipsCpu before=*cpu;int rc=battle_callback_selector_try_host(runtime,cpu,target);
 if(!rc&&memcmp(cpu,&before,sizeof(*cpu)))fail("selector rejected architecture",target);
 return rc;
}
static unsigned selector_controls(void)
{
 defaults();PcPortMipsCpu cpu;setup(&cpu);observing=0;rt.bridge_cpu=&cpu;PcPortMipsCpu old=cpu;snapshot();
 uint8_t *code=PSX_ADDR(selector_target);unsigned total=0;
 for(unsigned i=0;i<3;i++){unsigned off=i==0?0:i==1?60:119;code[off]^=1;if(selector_attempt(&rt,&cpu,selector_target))fail("selector full identity",off);code[off]^=1;total++;}
 /* Preserve the first digest byte while changing the body, so a shortened
  * digest comparison cannot pass merely by rejecting three easy corruptions. */
 uint8_t original_digest[32],changed_digest[32],saved[2]={code[58],code[59]};
 assert(SHA256(code,120,original_digest));unsigned found=0;
 for(unsigned value=0;value<65536u;value++){
  code[58]=(uint8_t)value;code[59]=(uint8_t)(value>>8);
  if(code[58]==saved[0]&&code[59]==saved[1])continue;
  assert(SHA256(code,120,changed_digest));
  if(changed_digest[0]==original_digest[0]&&memcmp(changed_digest,original_digest,32)){found=1;break;}
 }
 if(!found||selector_attempt(&rt,&cpu,selector_target))fail("selector digest-prefix collision",0);
 code[58]=saved[0];code[59]=saved[1];total++;
 cpu.load_valid=1;cpu.load_reg=4;if(selector_attempt(&rt,&cpu,selector_target))fail("selector pending entry",0);cpu=old;total++;
 rt.force_interpret=1;if(selector_attempt(&rt,&cpu,selector_target))fail("selector forced entry",0);rt.force_interpret=0;total++;
 rt.initialized=0;if(selector_attempt(&rt,&cpu,selector_target))fail("selector initialization",0);rt.initialized=1;total++;
 g_ActiveBattleRuntime=NULL;if(selector_attempt(&rt,&cpu,selector_target))fail("selector active runtime",0);g_ActiveBattleRuntime=&rt;total++;
 rt.bridge_cpu=NULL;if(selector_attempt(&rt,&cpu,selector_target))fail("selector bridge context",0);rt.bridge_cpu=&cpu;total++;
 if(selector_attempt(&rt,&cpu,selector_target+4u))fail("selector wrong target",0);
 total++;
 assert(setenv("XENO_BATTLE_NATIVE_CALLBACK_SELECTOR","0",1)==0);if(selector_attempt(&rt,&cpu,selector_target))fail("selector opt out",0);assert(unsetenv("XENO_BATTLE_NATIVE_CALLBACK_SELECTOR")==0);total++;
 if(memcmp(&cpu,&old,sizeof(cpu))||!memory_equal())fail("selector gates changed state",0);
 return total;
}
static unsigned selector_boundaries(void)
{
 unsigned total=0;
 for(unsigned cached=0;cached<2;cached++)for(unsigned disabled=0;disabled<2;disabled++)for(unsigned reject=0;reject<3;reject++){
  defaults();PcPortMipsCpu cpu;setup(&cpu);observing=0;cpu.bus.bridge=runtime_bridge;uint8_t *code=PSX_ADDR(selector_target);
  rt.functions[0]=(ResolvedFunction){selector_target,(void *)legacy_probe,"selector_scale"};rt.function_count=1;
  if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,selector_target);slot->target=selector_target;slot->state=1;slot->entry=rt.functions[0];}
  if(reject==1)code[0]^=1;
  if(reject==2)assert(setenv("XENO_BATTLE_NATIVE_CALLBACK_SELECTOR","0",1)==0);
  if(disabled)assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  PcPortMipsCpu old=cpu;PcPortMipsExecutionStats stats=PcPortMipsExecutionStatsGet();snapshot();
  int rc=reject&&!disabled?runtime_bridge(&rt,&cpu,selector_target):PcPortMipsRun(&cpu,selector_target,BATTLE_HALT_PC,100);
  if(reject==1)code[0]^=1;
  if(reject==2)assert(unsetenv("XENO_BATTLE_NATIVE_CALLBACK_SELECTOR")==0);
  if(disabled)assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
  /* Restore the identity byte in the memory snapshot before comparison. */
  if(reject==1)want_ram[selector_target&(PSX_RAM_SIZE-1u)]^=1;
  if(rc!=(reject?(disabled?PC_PORT_MIPS_INTERPRETER_DISABLED:0):PC_PORT_MIPS_HALTED)||legacy_calls||cpu.steps||rt.bridge_cpu||!memory_equal()||PcPortMipsExecutionStatsGet().interpreted_instructions!=stats.interpreted_instructions)fail("selector canonical cache boundary",(uint32_t)rc);
  if(reject&&memcmp(cpu.gpr,old.gpr,sizeof(cpu.gpr)))fail("selector rejected state",reject);
  total++;
 }
 return total;
}

static int composition_read(void *opaque,uint32_t address,unsigned width,uint32_t *value)
{
 (void)opaque;
 if(!((address>=0x800a3490u&&(uint64_t)address+width<=0x800a3640u)||
      (address>=0x8003f8b0u&&(uint64_t)address+width<=0x8003f8e8u)||
      (address>=0x800523f0u&&(uint64_t)address+width<=0x800563f0u)||
      (address>=0x801f0000u&&(uint64_t)address+width<=0x80200000u)))fail("callback oracle unexpected read",address);
 *value=load_le(PSX_ADDR(address),width);return 0;
}
static int composition_write(void *opaque,uint32_t address,unsigned width,uint32_t value)
{
 (void)opaque;
 if(address<0x801f0000u||(uint64_t)address+width>0x80200000u)fail("callback oracle unexpected write",address);
 store_le(PSX_ADDR(address),width,value);return 0;
}
static unsigned selector_composition(void)
{
 defaults();PcPortMipsCpu caller;setup(&caller);observing=0;caller.bus.bridge=runtime_bridge;
 FILE *f=fopen("disc/SLUS_006.64","rb");assert(f);
 assert(!fseek(f,0x8003f8b0u-0x8000f800u,SEEK_SET));assert(fread(PSX_ADDR(0x8003f8b0u),1,0x38,f)==0x38);
 assert(!fseek(f,0x800523f0u-0x8000f800u,SEEK_SET));assert(fread(PSX_ADDR(0x800523f0u),1,0x4000,f)==0x4000);assert(!fclose(f));
 static const uint32_t selectors[]={0,1,2,3,4,0xffffffffu,0x80000000u,0x7fffffffu,0x10001u,0x80000002u,0xffff0003u,0xa0000001u};
 static const uint32_t phases[]={0,1,31,32,33,0x7fffu,0x8000u,0xffffu,0x80008001u};
 static const uint32_t divisors[]={1,2,7,31,0x7fffu,0xffffu,0x8000u,0x80000001u};
 static const uint32_t bases[]={0,32,33,0x7fffu,0x8000u,0xffffu,0x80000001u,0xffffffe0u};
 unsigned total=0;
 rt.bridge_cpu=&caller;PcPortMipsCpu before=caller;
 for(unsigned s=0;s<12;s++)for(unsigned a=0;a<9;a++)for(unsigned b=0;b<8;b++){
  uint32_t selected=0,v1=0,args[]={selectors[s]};PcPortMipsExecutionStats stats=PcPortMipsExecutionStatsGet();snapshot();
  assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  int rc=PcPort_BattleMipsCallJal(selector_target,args,1,&selected,&v1);
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
  if(rc||v1!=caller.gpr[3]||rt.bridge_cpu!=&caller||memcmp(&caller,&before,sizeof(caller))||!memory_equal()||PcPortMipsExecutionStatsGet().interpreted_instructions!=stats.interpreted_instructions)fail("selector actual native jal boundary",selectors[s]);
  PcPortMipsCpu oracle;initialize_cpu(&oracle,&rt);oracle.bus.read=composition_read;oracle.bus.write=composition_write;oracle.bus.bridge=NULL;
  oracle.gpr[4]=phases[a];oracle.gpr[5]=divisors[b];oracle.gpr[6]=bases[b];oracle.gpr[29]=0x801ff000u;
  rc=PcPortMipsRun(&oracle,selected,BATTLE_HALT_PC,256);
  if(rc!=PC_PORT_MIPS_HALTED)fail("selected original callback",selected);
  snapshot();stats=PcPortMipsExecutionStatsGet();
  assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  uint32_t actual=(uint32_t)selector_consume(selected,(int32_t)phases[a],(int32_t)divisors[b],(int32_t)bases[b]);
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
  if(actual!=oracle.gpr[2]||!memory_equal()||PcPortMipsExecutionStatsGet().interpreted_instructions!=stats.interpreted_instructions)fail("selector actual callback consumer",selected);
  total++;
 }
 rt.bridge_cpu=NULL;return total;
}
int main(void)
{
 load_inputs();defaults();PcPortMipsCpu cpu;setup(&cpu);observing=1;snapshot();
 /* Exact cases, near misses and high-bit values that would become cases if
  * mistakenly narrowed to a byte/halfword or translated as data pointers. */
 for(unsigned hi=0;hi<16;hi++)for(unsigned lo=0;lo<16;lo++)selector_case((hi<<28)|lo);
 for(unsigned hi=0;hi<16;hi++)for(unsigned lo=0;lo<16;lo++)selector_case((hi<<16)|lo);
 static const uint32_t edges[]={0x7fffffffu,0x80000000u,0x80000001u,0xfffffffeu,0xffffffffu,0x100u,0x101u,0x102u,0x103u,0xff01u,0xff02u,0xff03u};
 for(unsigned i=0;i<sizeof(edges)/sizeof(*edges);i++)selector_case(edges[i]);
 uint32_t random=0x19711224u;
 for(unsigned i=0;i<2048;i++){random=random*1664525u+1013904223u;selector_case(random);}
 printf("CALLBACK SELECTOR PASS %u full original/native comparisons, exact guest addresses; no memory/GTE operations, native interpreted instructions=0\n",selector_cases);
 printf("CALLBACK SELECTOR GATES PASS %u cases\n",selector_controls());
 printf("CALLBACK SELECTOR CACHE PASS %u cases\n",selector_boundaries());
 printf("CALLBACK SELECTOR COMPOSITION PASS %u actual native-JAL/consumer/callback cases; actual SDK trig, return-value proof, nonzero divisors\n",selector_composition());return 0;
}
