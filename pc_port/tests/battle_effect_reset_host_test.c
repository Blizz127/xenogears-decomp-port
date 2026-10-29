/* Six original effect-pool/reset bodies versus structured native bodies.
 * Real heap and SDK integration are separate from controlled boundary cases. */
#define BATTLE_STAGE3_TEST_MAIN stage3_fixture_main
#include "battle_module_stage3_host_test.c"
extern void *HeapAlloc(uint32_t,uint32_t);
extern void HeapChangeCurrentUser(uint32_t,char **);
extern void allocation_fixture_reset(unsigned);
extern void allocation_fixture_state(uint64_t *);
static const uint32_t effect_entries[]={0x800a8b0c,0x800a2234,0x800a22e8,0x800a2ca4,0x800a2d5c,0x800b00d0};
static const unsigned effect_sizes[]={228,116,72,120,300,36};
static uint8_t effect_code[6][300],effect_sdk_code[0x300];
static unsigned effect_index,heap_calls,user_calls,packet_calls;
static int real_heap,model_children,model_initializers,alias_case,perturb;
static uint32_t descriptor,buffer,initial_count;
static uint64_t expected_heap_state[16];
static const uint32_t edge_counts[]={0,1,2,7,0xffff,0x10000,0x7fffffff,0x80000000,
 0x80010001,0xffffffff,0xfffffffe,0x40000000,0x55555555,0x12345678,0x7ffffffd,0x8000};

static void effect_user(uint32_t user,char **names)
{
 if(user!=4 || names) fail("effect heap user",user);
 user_calls++;if(real_heap) HeapChangeCurrentUser(user,names);
 if(perturb) {
  write_at(rt.bridge_cpu->gpr[29]+0x10u,4,0xabc00000u+user_calls);
  if(user_calls==1) write_at(0x800658c8u,4,0x80141000u);
 }
}
static void *effect_allocate(uint32_t size,uint32_t flags)
{
 if(flags) fail("effect heap flags",flags);
 heap_calls++;
 if(real_heap) return HeapAlloc(size,flags);
 write_at(0x80059470u+(heap_calls-1)*8u,4,size);
 write_at(0x80059474u+(heap_calls-1)*8u,4,flags);
 if((variant&8u) && (heap_calls==1 || (variant&16u))) return NULL;
 return PSX_ADDR(0x80100000u+heap_calls*0x4000u);
}
static void effect_ft4(POLY_FT4 *p) {SetPolyFT4(p);}
static const ResolvedFunction effect_services[]={
 {0x80031bdcu,effect_allocate,"HeapAlloc"},{0x80032498u,effect_user,"HeapChangeCurrentUser"},
 {0x80043a1cu,typed_page,"GetTPage"},{0x80043a58u,unused_clut,"GetClut"},
 {0x80043bfcu,typed_semi,"SetSemiTrans"},{0x80043cb0u,effect_ft4,"SetPolyFT4"}};

static int effect_bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
 if(stop_target && target-stop_target<12u) return 0;
 for(unsigned i=0;i<6;i++) {
  if(target==effect_entries[i]) {
   stage_trace(cpu,target,(i==1 || i==3) ? 2:(i==2 || i==4 ? 1:0));
   int modeled=(model_children && i!=0) || (model_initializers && (i==2 || i==4));
   int rc;
   if(modeled) {
    if(i==1 || i==3) {
     write_at(cpu->gpr[4],4,cpu->gpr[5]);
     if(i==1 && perturb) write_at(0x800658c8u,4,0x80141000u);
    } else if(i==2 || i==4) write_at(cpu->gpr[4]+4u,2,0x1234);
    cpu->gpr[2]=0x91abc000u+i;cpu->gpr[3]=0x62abc000u+i;
    cpu->hi=0xabc00000u+i;cpu->lo=0xdef00000u+i;rc=1;
   } else rc=candidate ? runtime_bridge(opaque,cpu,target):0;
   if(rc==1 && pending_return==target) {cpu->load_valid=1;cpu->load_reg=2;cpu->load_value=1;}
   return rc;
  }
  if(target-effect_entries[i]<effect_sizes[i]) return 0;
 }
 unsigned argc=target==0x80043cb0u ? 1:target==0x80043a1cu ? 4:2;
 stage_trace(cpu,target,argc);
 if(!find_function(&rt,target)) {fail("effect unexpected service",target);return -1;}
 PcPortMipsCpu *saved=rt.bridge_cpu;rt.bridge_cpu=cpu;
 if(candidate) {
  if(runtime_bridge(opaque,cpu,target)!=1) {fail("effect typed service",target);rt.bridge_cpu=saved;return -1;}
 } else if(target==0x80031bdcu) {
  cpu->gpr[2]=PsxMemory_GuestAddr(effect_allocate(cpu->gpr[4],cpu->gpr[5]));cpu->gpr[3]=0;
 } else if(target==0x80032498u) effect_user(cpu->gpr[4],(char **)(uintptr_t)cpu->gpr[5]);
 else if(target==0x80043cb0u) effect_ft4(module_memory_pointer(&rt,cpu->gpr[4],40,1));
 else if(target==0x80043bfcu) typed_semi(module_memory_pointer(&rt,cpu->gpr[4],8,1),(int32_t)cpu->gpr[5]);
 else if(target==0x80043a58u) {cpu->gpr[2]=((cpu->gpr[5]<<6)|((cpu->gpr[4]>>4)&63u))&0xffffu;cpu->gpr[3]=0;}
 else if(target==0x80043a1cu) {cpu->gpr[2]=typed_page(cpu->gpr[4],cpu->gpr[5],cpu->gpr[6],cpu->gpr[7]);cpu->gpr[3]=0;}
 rt.bridge_cpu=saved;
 if(target==0x80043cb0u && ++packet_calls==1 && perturb && effect_index==4)
  write_at(descriptor+4u,2,variant&1u ? 0:3);
 if(pending_return==target) {cpu->load_valid=1;cpu->load_reg=2;cpu->load_value=1;}
 return failed ? -1:1;
}

static void effect_setup(PcPortMipsCpu *cpu)
{
 memset(&rt,0,sizeof(rt));memset(g_PsxRam,0xa5,sizeof(g_PsxRam));
 memset(shared,0x5a,sizeof(shared));memset(g_PsxScratchpad,0xa9,sizeof(g_PsxScratchpad));
 memset(stage_host,0xb6,sizeof(stage_host));
 for(unsigned i=0;i<6;i++) memcpy(PSX_ADDR(effect_entries[i]),effect_code[i],effect_sizes[i]);
 memcpy(PSX_ADDR(0x80043a1cu),effect_sdk_code,sizeof(effect_sdk_code));
 rt.data[0]=(ResolvedData){0x80050000u,0x10000u,shared,"shared-5"};
 rt.data[1]=(ResolvedData){0x80060000u,0x10000u,shared+0x10000u,"shared-6"};rt.data_count=2;
 rt.host_ranges[0]=(HostRange){(uintptr_t)stage_host,(uintptr_t)stage_host+sizeof(stage_host),1};rt.host_range_count=1;
 memcpy(rt.functions,effect_services,sizeof(effect_services));rt.function_count=6;
 rt.initialized=1;g_ActiveBattleRuntime=&rt;initialize_cpu(cpu,&rt);cpu->bus.bridge=effect_bridge;
 for(unsigned i=1;i<29;i++) cpu->gpr[i]=0x12340000u+variant*256u+i;
 cpu->gpr[30]=0x87650000u+variant;cpu->hi=0x98765432u;cpu->lo=0xfedcba98u;
 if(variant&2u) cpu->gpr[29]=0x1f8003f8u;else if(variant&4u) cpu->gpr[29]|=0x20000000u;
 frame_base=cpu->gpr[29]-0x100u;calls=heap_calls=user_calls=packet_calls=0;stop_target=pending_return=0;
 uint32_t descriptors[]={0x80170000u,0xa0170000u,0x1f800b00u,(uint32_t)(uintptr_t)stage_host+0x4000u};
 uint32_t buffers[]={0x80120000u,0xa0120000u,0x1f800400u,(uint32_t)(uintptr_t)stage_host+0x10000u};
 descriptor=descriptors[variant%4];buffer=buffers[(variant/4)%4];
 initial_count=(const uint32_t[]){0,1,2,3,4,7,0xffffffffu,0xfffffffeu}[variant%8];
 if(model_initializers || model_children) initial_count=edge_counts[variant%16];
 if(real_heap) {allocation_fixture_reset(variant);initial_count=variant%8;}
 if(effect_index==2) initial_count=variant%8;
 if(alias_case) {
  buffer=0x80120000u;
  descriptor=effect_index==2 ? buffer-6u:buffer+0x44u;
  if(effect_index==2) descriptor=buffer; /* first byte clears the pointer's low byte; count stays live */
  initial_count=3;
 }
 write_at(descriptor,4,effect_index==2 && (variant&16u) && !alias_case ? 0:buffer);
 write_at(descriptor+4u,2,initial_count);write_at(descriptor+6u,2,initial_count);
 if(alias_case && effect_index==2) {
  initial_count=variant&1u ? 0x8000u:3u;buffer=descriptor+(variant&1u ? 7u:6u);
  write_at(descriptor,4,buffer);write_at(descriptor+6u,2,initial_count);
 }
 for(unsigned bank=0;bank<2;bank++) {
  write_at(0x80140000u+bank*0x1000u+0x348u,2,bank ? 2:initial_count);
  write_at(0x80140000u+bank*0x1000u+0x34au,2,bank ? 4:initial_count);
 }
 write_at(0x800658c8u,4,0x80140000u);
 if(effect_index==0 && alias_case) {
  uint32_t setup=0x800c3e88u-0x348u;
  write_at(setup+0x348u,2,3);write_at(setup+0x34au,2,2);write_at(0x800658c8u,4,setup);
 }
 write_at(0x800568d0u,1,0);
 cpu->gpr[4]=descriptor;cpu->gpr[5]=initial_count;
 running_entry=effect_entries[effect_index];
}

static int effect_case(void)
{
 PcPortMipsCpu oracle,native;effect_setup(&oracle);candidate=compare=0;
 if(PcPortMipsRun(&oracle,running_entry,BATTLE_HALT_PC,200000)!=PC_PORT_MIPS_HALTED) {fail(oracle.error,oracle.pc);return 0;}
 unsigned expected_calls=calls,expected_allocations=heap_calls,expected_users=user_calls;
 stage_snapshot();if(real_heap) allocation_fixture_state(expected_heap_state);
 effect_setup(&native);candidate=compare=1;
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
 PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
 int rc=PcPortMipsRun(&native,running_entry,BATTLE_HALT_PC,200000);
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
 if(rc!=PC_PORT_MIPS_HALTED) {fail(native.error,native.pc);return 0;}
 if(native.steps || rt.bridge_cpu || calls!=expected_calls || heap_calls!=expected_allocations || user_calls!=expected_users ||
  PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions) fail("effect native counters/context",rc);
 for(unsigned i=16;i<32;i++) if(native.gpr[i]!=oracle.gpr[i]) fail("effect preserved register",i);
 if(native.gpr[2]!=oracle.gpr[2] || native.gpr[3]!=oracle.gpr[3] || native.hi!=oracle.hi || native.lo!=oracle.lo) {
  fprintf(stderr,"returns %08x/%08x %08x/%08x\n",native.gpr[2],oracle.gpr[2],native.gpr[3],oracle.gpr[3]);
  fail("effect return/HI-LO",native.gpr[2]);
 }
 if(!stage_equal()) fail("effect final memory",running_entry);
 if(real_heap) {uint64_t state[16];allocation_fixture_state(state);if(memcmp(state,expected_heap_state,sizeof(state))) fail("effect heap metadata",0);}
 return !failed;
}

static int effect_controls(void)
{
 PcPortMipsCpu cpu;real_heap=model_children=model_initializers=alias_case=perturb=0;variant=1;candidate=1;compare=0;
 for(effect_index=0;effect_index<6;effect_index++) {
  effect_setup(&cpu);rt.bridge_cpu=&cpu;stage_snapshot();
  const unsigned offsets[]={0,effect_sizes[effect_index]/2,effect_sizes[effect_index]-1};
  for(unsigned k=0;k<3;k++) {uint8_t *p=PSX_ADDR(running_entry+offsets[k]);*p^=1;
   if(battle_effect_reset_try_host(&rt,&cpu,running_entry)) fail("effect identity",k);
   *p^=1;
  }
  cpu.load_valid=1;cpu.load_reg=4;if(battle_effect_reset_try_host(&rt,&cpu,running_entry)) fail("effect pending entry",0);
  cpu.load_valid=0;rt.force_interpret=1;if(battle_effect_reset_try_host(&rt,&cpu,running_entry)) fail("effect reference",0);
  rt.force_interpret=0;assert(setenv("XENO_BATTLE_NATIVE_EFFECT_RESET","0",1)==0);
  if(battle_effect_reset_try_host(&rt,&cpu,running_entry)) fail("effect environment",0);
  assert(unsetenv("XENO_BATTLE_NATIVE_EFFECT_RESET")==0);g_ActiveBattleRuntime=NULL;
  if(battle_effect_reset_try_host(&rt,&cpu,running_entry)) fail("effect inactive",0);
  g_ActiveBattleRuntime=&rt;rt.bridge_cpu=NULL;if(battle_effect_reset_try_host(&rt,&cpu,running_entry)) fail("effect context",0);
  rt.bridge_cpu=&cpu;rt.initialized=0;if(battle_effect_reset_try_host(&rt,&cpu,running_entry)) fail("effect init",0);
  if(calls || !stage_equal()) fail("effect rejected writes",0);
 }
 const uint32_t targets[]={0x800a2234,0x800a22e8,0x800a2ca4,0x800a2d5c,0x800b00d0,
 0x80031bdc,0x80032498,0x80043a1c,0x80043a58,0x80043bfc,0x80043cb0};
 for(unsigned k=0;k<sizeof(targets)/sizeof(targets[0]);k++) for(unsigned pending=0;pending<2;pending++) {
  effect_index=0;effect_setup(&cpu);if(pending) pending_return=targets[k];else stop_target=targets[k];
  assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
  int rc=PcPortMipsRun(&cpu,running_entry,BATTLE_HALT_PC,200000);
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
  if(rc!=PC_PORT_MIPS_UNRESOLVED_CALL || cpu.steps || rt.bridge_cpu ||
   PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions) fail("effect nested refusal",targets[k]);
 }
 /* Partial frame/global writes are retained at a later invalid access. */
 const unsigned fault_entries[]={0,1,2,3,4,4};
 for(unsigned k=0;k<6;k++) {
  PcPortMipsCpu oracle;effect_index=fault_entries[k];effect_setup(&oracle);candidate=compare=0;
  if(k==0) write_at(0x800658c8u,4,0x1f800d00u);
  if(k==1 || k==3) oracle.gpr[29]=0x1f801008u;
  if(k==2) {write_at(descriptor,4,0x1f800ff4u);write_at(descriptor+6u,2,2);}
  if(k==4) write_at(descriptor,4,0x1f800fe4u);
  if(k==5) write_at(descriptor,4,0);
  if(PcPortMipsRun(&oracle,running_entry,BATTLE_HALT_PC,10000)!=PC_PORT_MIPS_FAULT) {fail("effect original partial fault",k);return 0;}
  stage_snapshot();effect_setup(&cpu);candidate=1;compare=0;
  if(k==0) write_at(0x800658c8u,4,0x1f800d00u);
  if(k==1 || k==3) cpu.gpr[29]=0x1f801008u;
  if(k==2) {write_at(descriptor,4,0x1f800ff4u);write_at(descriptor+6u,2,2);}
  if(k==4) write_at(descriptor,4,0x1f800fe4u);
  if(k==5) write_at(descriptor,4,0);
  assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();int rc=PcPortMipsRun(&cpu,running_entry,BATTLE_HALT_PC,10000);
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
  if(rc!=PC_PORT_MIPS_UNRESOLVED_CALL || cpu.steps || rt.bridge_cpu || !stage_equal() ||
   PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions) fail("effect partial fault retention",k);
 }
 puts("BATTLE EFFECT RESET PASS identities/context/pending/nested refusal and six partial-write faults; native steps=0");return !failed;
}

static int effect_sdk_bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
 return candidate && target==running_entry ? runtime_bridge(opaque,cpu,target):0;
}
static void effect_sdk_setup(PcPortMipsCpu *cpu,unsigned service)
{
 effect_index=4;real_heap=model_children=model_initializers=alias_case=perturb=0;effect_setup(cpu);
 cpu->bus.bridge=effect_sdk_bridge;
 running_entry=(const uint32_t[]){0x80043cb0u,0x80043bfcu,0x80043a58u,0x80043a1cu}[service];
 for(unsigned i=0;i<64;i++) write_at(buffer+i,1,i*17u+variant*29u);
 cpu->gpr[4]=buffer;cpu->gpr[5]=1;
 if(service==2) {cpu->gpr[4]=0;cpu->gpr[5]=0x1cd;}
 if(service==3) {cpu->gpr[4]=0;cpu->gpr[5]=1;cpu->gpr[6]=0x380;cpu->gpr[7]=0;}
}
static int effect_sdk_cases(void)
{
 for(unsigned service=0;service<4;service++) for(variant=0;variant<16;variant++) {
  PcPortMipsCpu oracle,native;uint8_t expected[64],actual[64];
  effect_sdk_setup(&oracle,service);candidate=0;
  if(PcPortMipsRun(&oracle,running_entry,BATTLE_HALT_PC,2000)!=PC_PORT_MIPS_HALTED) {fail(oracle.error,oracle.pc);return 0;}
  for(unsigned i=0;i<64;i++) expected[i]=read_at(buffer+i,1);
  effect_sdk_setup(&native,service);candidate=1;
  assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();int rc=PcPortMipsRun(&native,running_entry,BATTLE_HALT_PC,2000);
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
  for(unsigned i=0;i<64;i++) actual[i]=read_at(buffer+i,1);
  if(rc!=PC_PORT_MIPS_HALTED || native.steps || rt.bridge_cpu ||
   PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions ||
   memcmp(expected,actual,sizeof(expected)) || (service>=2 && native.gpr[2]!=oracle.gpr[2])) {fail("effect original SDK packet/return",service);return 0;}
 }
 puts("BATTLE EFFECT RESET PASS 64 original SDK packet/return cases at effect-pool arguments; native steps=0; service frames/volatile state excluded");return !failed;
}

int main(int argc,char **argv)
{
 setvbuf(stdout,NULL,_IOLBF,0);
 assert(argc==1 || (argc==2 && !strcmp(argv[1],"--controls-only")));
 FILE *f=fopen("disc/battle.bin","rb");assert(f);
 for(unsigned i=0;i<6;i++) {assert(!fseek(f,effect_entries[i]-BATTLE_BASE,SEEK_SET));assert(fread(effect_code[i],1,effect_sizes[i],f)==effect_sizes[i]);}
 assert(!fclose(f));
 f=fopen("disc/SLUS_006.64","rb");assert(f && !fseek(f,0x80043a1cu-0x8000f800u,SEEK_SET));
 assert(fread(effect_sdk_code,1,sizeof(effect_sdk_code),f)==sizeof(effect_sdk_code));assert(!fclose(f));
 assert(setenv("XENO_CPU_EXECUTION_STATS","1",1)==0);
 if(argc==2) return effect_controls() ? 0:1;
 unsigned cases=0;
 for(effect_index=0;effect_index<6;effect_index++) for(variant=0;variant<32;variant++) {
  real_heap=model_children=model_initializers=alias_case=0;perturb=(variant&16u)!=0;
  if(!effect_case()) return 1;
  cases++;
 }
 printf("BATTLE EFFECT RESET PASS %u original-body cases, full native child chain and actual SDK, controlled heap; native steps=0\n",cases);
 cases=0;real_heap=1;perturb=0;
 for(unsigned i=0;i<3;i++) for(variant=0;variant<16;variant++) {effect_index=(const unsigned[]){0,1,3}[i];if(!effect_case()) return 1;
  cases++;}
 printf("BATTLE EFFECT RESET PASS %u actual native heap integrations; native steps=0\n",cases);
 cases=0;real_heap=0;model_children=1;
 for(variant=0;variant<32;variant++) {effect_index=0;perturb=(variant&16u)!=0;if(!effect_case()) return 1;
  cases++;}
 model_children=0;model_initializers=1;
 for(unsigned i=0;i<2;i++) for(variant=0;variant<32;variant++) {effect_index=i ? 3:1;perturb=(variant&16u)!=0;if(!effect_case()) return 1;
  cases++;}
 printf("BATTLE EFFECT RESET PASS %u signed/wrapping caller cases with modeled children/initializers; native steps=0\n",cases);
 model_initializers=0;alias_case=1;perturb=0;cases=0;
 for(unsigned i=0;i<3;i++) for(variant=0;variant<8;variant++) {effect_index=(const unsigned[]){0,2,4}[i];if(!effect_case()) return 1;
  cases++;}
 printf("BATTLE EFFECT RESET PASS %u live count/global alias cases; native steps=0\n",cases);
 return effect_sdk_cases() && effect_controls() ? 0:1;
}
