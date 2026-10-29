/* Original allocation wrapper versus native, with controlled heap boundaries
 * and actual native heap integration. String/render callees remain modeled in
 * the separate text-caller leg; this is not allocator-policy or text parity. */
#define BATTLE_STAGE3_TEST_MAIN stage3_fixture_main
#include "battle_module_stage3_host_test.c"
extern void *HeapAlloc(uint32_t,uint32_t);
extern void HeapChangeCurrentUser(uint32_t,char **);
extern void allocation_fixture_reset(unsigned);
extern void allocation_fixture_state(uint64_t *);
static uint8_t allocation_code[80];
static int real_heap,caller_leg;
static unsigned heap_calls,user_calls;
static uint32_t requested_size,planned_pointer;
static const uint32_t allocation_sizes[]={0,1,4,0x1b,0x1e,0x39,0xfffffffdu,0xfffffffeu,
 0xffffffffu,0x7fffffffu,0x80000000u,0x80100000u,0x55555555u,0x12345678u,0xdeadbeefu,0x7ffffffdu};
static const uint32_t pointer_results[]={0,0x80000000u,0x80000004u,0x80100000u,0x801fffffu,0x801f34a8u};
static uint64_t expected_heap_state[16];

static void allocation_user(uint32_t user,char **names)
{
 if(user!=2 || names!=NULL) fail("allocation user arguments",user);
 user_calls++;
 if(real_heap) HeapChangeCurrentUser(user,names);
 else {
  write_at(0x80059470u,4,user_calls);
  if(variant&16u) write_at(rt.bridge_cpu->gpr[29]+0x10u,4,0xaabbcc00u+variant);
 }
}
static void *allocation_heap(uint32_t size,uint32_t flags)
{
 heap_calls++;
 if(size!=requested_size || flags) fail("allocation scalar bits",size);
 if(real_heap) return HeapAlloc(size,flags);
 write_at(0x80059474u,4,size);write_at(0x80059478u,4,heap_calls);
 return planned_pointer ? PSX_ADDR(planned_pointer):NULL;
}
static const ResolvedFunction allocation_services[]={
 {0x80031bdcu,allocation_heap,"HeapAlloc"},{0x80032498u,allocation_user,"HeapChangeCurrentUser"}};

static int allocation_bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
 if(stop_target && target-stop_target<12u) return 0;
 const uint32_t entries[]={BATTLE_TEXT_ALLOCATION_ADDRESS,BATTLE_STAGE3_TEXT_ADDRESS};
 const unsigned sizes[]={80,112};
 for(unsigned i=0;i<2;i++) {
  if(target==entries[i]) {
   stage_trace(cpu,target,i ? 0:1);
   int rc=candidate ? runtime_bridge(opaque,cpu,target):0;
   if(rc==1 && pending_return==target) {cpu->load_valid=1;cpu->load_reg=2;cpu->load_value=1;}
   return rc;
  }
  if(target-entries[i]<sizes[i]) return 0;
 }
 if(target==0x800338d8u || target==0x80034eacu) {
  stage_trace(cpu,target,target==0x800338d8u ? 1:4);
  if(target==0x800338d8u) cpu->gpr[2]=0x80170000u+cpu->gpr[4]*4u;
  else {write_at(cpu->gpr[5],4,read_at(cpu->gpr[4],4));cpu->gpr[2]=8;}
  cpu->gpr[3]=0;
 } else {
  if(target!=0x80031bdcu && target!=0x80032498u) {fail("allocation unexpected service",target);return -1;}
  stage_trace(cpu,target,2);
  if(candidate) {
   if(runtime_bridge(opaque,cpu,target)!=1) {fail("allocation typed bridge",target);return -1;}
  } else {
   PcPortMipsCpu *previous=rt.bridge_cpu;rt.bridge_cpu=cpu;
   if(target==0x80032498u) allocation_user(cpu->gpr[4],(char **)(uintptr_t)cpu->gpr[5]);
   else {cpu->gpr[2]=PsxMemory_GuestAddr(allocation_heap(cpu->gpr[4],cpu->gpr[5]));cpu->gpr[3]=0;}
   rt.bridge_cpu=previous;
  }
 }
 if(pending_return==target) {cpu->load_valid=1;cpu->load_reg=2;cpu->load_value=1;}
 return failed ? -1:1;
}

static void allocation_setup(PcPortMipsCpu *cpu)
{
 memset(&rt,0,sizeof(rt));memset(g_PsxRam,0xa5,sizeof(g_PsxRam));
 memset(shared,0x5a,sizeof(shared));memset(g_PsxScratchpad,0xa9,sizeof(g_PsxScratchpad));
 memset(stage_host,0xb6,sizeof(stage_host));
 memcpy(PSX_ADDR(0x801e4000u),module,sizeof(module));
 memcpy(PSX_ADDR(BATTLE_TEXT_ALLOCATION_ADDRESS),allocation_code,sizeof(allocation_code));
 rt.data[0]=(ResolvedData){0x80050000u,0x10000u,shared,"shared-5"};
 rt.data[1]=(ResolvedData){0x80060000u,0x10000u,shared+0x10000u,"shared-6"};rt.data_count=2;
 rt.host_ranges[0]=(HostRange){(uintptr_t)stage_host,(uintptr_t)stage_host+sizeof(stage_host),1};rt.host_range_count=1;
 memcpy(rt.functions,allocation_services,sizeof(allocation_services));rt.function_count=2;
 rt.initialized=1;g_ActiveBattleRuntime=&rt;initialize_cpu(cpu,&rt);cpu->bus.bridge=allocation_bridge;
 for(unsigned i=1;i<29;i++) cpu->gpr[i]=0x12340000u+variant*256u+i;
 cpu->gpr[30]=0x87650000u+variant;cpu->hi=0x98765432u;cpu->lo=0xfedcba98u;
 if(variant&2u) cpu->gpr[29]=0x1f8003f8u;
 else if(variant&4u) cpu->gpr[29]|=0x20000000u;
 frame_base=cpu->gpr[29]-0x100u;calls=heap_calls=user_calls=0;stop_target=pending_return=0;
 cpu->gpr[4]=real_heap ? (const uint32_t[]){0,1,4,0x1b,0x1e,0x39,2,3}[variant%8] : allocation_sizes[variant%16];
 requested_size=(caller_leg ? 4u:cpu->gpr[4])+3u;requested_size*=26u;
 planned_pointer=pointer_results[(variant/16)%6];
 if(real_heap) allocation_fixture_reset(variant);
 for(unsigned i=0;i<10;i++) write_at(0x80170000u+i*4u,4,0xabc00000u+i);
}

static int allocation_case(void)
{
 PcPortMipsCpu oracle,native;allocation_setup(&oracle);candidate=compare=0;
 if(PcPortMipsRun(&oracle,running_entry,BATTLE_HALT_PC,100000)!=PC_PORT_MIPS_HALTED) {fail(oracle.error,oracle.pc);return 0;}
 unsigned expected_calls=calls,expected_allocations=heap_calls,expected_users=user_calls;
 stage_snapshot();if(real_heap) allocation_fixture_state(expected_heap_state);
 allocation_setup(&native);candidate=compare=1;
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
 PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
 int rc=PcPortMipsRun(&native,running_entry,BATTLE_HALT_PC,100000);
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
 if(rc!=PC_PORT_MIPS_HALTED) {fail(native.error,native.pc);return 0;}
 if(native.steps || rt.bridge_cpu || calls!=expected_calls || heap_calls!=expected_allocations || user_calls!=expected_users ||
    PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions) fail("allocation native context/counters",rc);
 for(unsigned i=16;i<32;i++) if(native.gpr[i]!=oracle.gpr[i]) fail("allocation preserved register",i);
 if(native.gpr[2]!=oracle.gpr[2] || native.gpr[3]!=oracle.gpr[3] || native.hi!=oracle.hi || native.lo!=oracle.lo) fail("allocation return/HI-LO",native.gpr[2]);
 if(!stage_equal()) fail("allocation final memory",running_entry);
 if(real_heap) {
  uint64_t state[16];allocation_fixture_state(state);
  if(memcmp(state,expected_heap_state,sizeof(state))) fail("allocation heap metadata",0);
 }
 return !failed;
}

static int allocation_controls(void)
{
 PcPortMipsCpu cpu;variant=1;real_heap=caller_leg=0;candidate=1;compare=0;running_entry=BATTLE_TEXT_ALLOCATION_ADDRESS;
 allocation_setup(&cpu);rt.bridge_cpu=&cpu;stage_snapshot();
 const unsigned offsets[]={0,40,79};
 for(unsigned i=0;i<3;i++) {
  uint8_t *p=PSX_ADDR(running_entry+offsets[i]);*p^=1;
  if(battle_text_allocation_try_host(&rt,&cpu,running_entry)) fail("allocation identity",i);
  *p^=1;
 }
 cpu.load_valid=1;cpu.load_reg=4;if(battle_text_allocation_try_host(&rt,&cpu,running_entry)) fail("allocation pending entry",0);
 cpu.load_valid=0;rt.force_interpret=1;if(battle_text_allocation_try_host(&rt,&cpu,running_entry)) fail("allocation force reference",0);
 rt.force_interpret=0;assert(setenv("XENO_BATTLE_NATIVE_TEXT_ALLOCATION","0",1)==0);
 if(battle_text_allocation_try_host(&rt,&cpu,running_entry)) fail("allocation environment",0);
 assert(unsetenv("XENO_BATTLE_NATIVE_TEXT_ALLOCATION")==0);g_ActiveBattleRuntime=NULL;
 if(battle_text_allocation_try_host(&rt,&cpu,running_entry)) fail("allocation inactive",0);
 g_ActiveBattleRuntime=&rt;rt.bridge_cpu=NULL;if(battle_text_allocation_try_host(&rt,&cpu,running_entry)) fail("allocation context",0);
 rt.bridge_cpu=&cpu;rt.initialized=0;if(battle_text_allocation_try_host(&rt,&cpu,running_entry)) fail("allocation init",0);
 rt.bridge_cpu=NULL;rt.initialized=1;if(calls || !stage_equal()) fail("allocation rejection writes",0);
 const uint32_t targets[]={BATTLE_TEXT_ALLOCATION_ADDRESS,0x80032498u,0x80031bdcu,0x800338d8u,0x80034eacu};
 for(unsigned i=0;i<5;i++) for(unsigned pending=0;pending<2;pending++) {
  running_entry=BATTLE_STAGE3_TEXT_ADDRESS;caller_leg=real_heap=1;variant=1;allocation_setup(&cpu);
  if(pending) pending_return=targets[i];else stop_target=targets[i];
  assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
  int rc=PcPortMipsRun(&cpu,running_entry,BATTLE_HALT_PC,100000);
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
  if(rc!=PC_PORT_MIPS_UNRESOLVED_CALL || cpu.steps || rt.bridge_cpu || PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions) fail("allocation nested refusal",targets[i]);
 }
 /* A valid first frame write survives the following out-of-range RA store;
  * no heap service has run at that point. */
 running_entry=BATTLE_TEXT_ALLOCATION_ADDRESS;caller_leg=0;real_heap=1;variant=1;
 PcPortMipsCpu oracle;allocation_setup(&oracle);candidate=compare=0;
 oracle.gpr[29]=0x1f801004u;
 if(PcPortMipsRun(&oracle,running_entry,BATTLE_HALT_PC,10000)!=PC_PORT_MIPS_FAULT) fail("allocation original frame fault",0);
 stage_snapshot();allocation_setup(&cpu);candidate=compare=1;cpu.gpr[29]=0x1f801004u;
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
 PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
 int rc=PcPortMipsRun(&cpu,running_entry,BATTLE_HALT_PC,10000);
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
 if(rc!=PC_PORT_MIPS_UNRESOLVED_CALL || cpu.steps || heap_calls || user_calls || rt.bridge_cpu ||
    PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions || !stage_equal()) fail("allocation partial frame write",rc);
 return !failed;
}

int main(int argc,char **argv)
{
 assert(argc==2 || argc==3);assert(setvbuf(stdout,NULL,_IOLBF,0)==0);
 FILE *f=fopen(argv[1],"rb");assert(f && fread(module,1,sizeof(module),f)==sizeof(module));assert(!fclose(f));
 f=fopen("disc/battle.bin","rb");assert(f && !fseek(f,BATTLE_TEXT_ALLOCATION_ADDRESS-0x8006faf0u,SEEK_SET));
 assert(fread(allocation_code,1,sizeof(allocation_code),f)==sizeof(allocation_code));assert(!fclose(f));
 assert(setenv("XENO_CPU_EXECUTION_STATS","1",1)==0);
 if(argc==3 && !strcmp(argv[2],"--controls-only")) return allocation_controls()?0:1;
 caller_leg=real_heap=0;running_entry=BATTLE_TEXT_ALLOCATION_ADDRESS;
 for(variant=0;variant<96;variant++) if(!allocation_case()) return 1;
 puts("BATTLE TEXT ALLOCATION PASS 96 original wrapper cases with controlled heap services; native steps=0");
 real_heap=1;
 for(variant=0;variant<32;variant++) if(!allocation_case()) return 1;
 puts("BATTLE TEXT ALLOCATION PASS 32 wrapper cases with actual native heap services; native steps=0");
 caller_leg=1;running_entry=BATTLE_STAGE3_TEXT_ADDRESS;
 for(variant=0;variant<8;variant++) if(!allocation_case()) return 1;
 puts("BATTLE TEXT ALLOCATION PASS 8 original text-caller cases with actual heap; string/render outputs modeled; native steps=0");
 if(!allocation_controls()) return 1;
 puts("BATTLE TEXT ALLOCATION PASS identity/context/pending/refusal/frame controls; allocator policy and text rendering not accepted");
 return 0;
}
