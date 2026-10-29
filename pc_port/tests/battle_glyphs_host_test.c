/* Original glyph wrappers/renderer and stage-three callers versus native.
 * Actual atlas, style and packet services; no modeled game callee outputs. */
#define BATTLE_STAGE3_TEST_MAIN stage3_fixture_main
#include "battle_module_stage3_host_test.c"

extern void func_80076C34(uint8_t *);
static uint8_t glyph_code[184],renderer_code[688],ft4_code[20];
static uint32_t glyph_table,glyph_descriptor;
static unsigned perturb,invalid_descriptor;
static void glyph_ft4(POLY_FT4 *p) {SetPolyFT4(p);}
static void glyph_style(uint8_t *p) {func_80076C34(p);}
static const ResolvedFunction glyph_services[]={
 {0x80026338u,func_80026338,"func_80026338"},
 {0x80043cb0u,glyph_ft4,"SetPolyFT4"},{0x80043cd8u,typed_gt4,"SetPolyGT4"},
 {0x80043cc4u,typed_g4,"SetPolyG4"},{0x80043d78u,typed_line,"SetLineF2"},
 {0x80043bfcu,typed_semi,"SetSemiTrans"},{0x80043c24u,typed_shade,"SetShadeTex"},
 {0x80043a1cu,typed_page,"GetTPage"},{0x80043a58u,unused_clut,"GetClut"},
 {0x80076c34u,glyph_style,"func_80076C34"}};
static const uint32_t glyph_entries[]={BATTLE_GLYPH_RENDER_ADDRESS,BATTLE_GLYPH_FULL_ADDRESS,
 BATTLE_GLYPH_HALF_ADDRESS,BATTLE_STAGE3_GLYPHS_ADDRESS,BATTLE_STAGE3_FIRST_ADDRESS};
static const unsigned glyph_counts[]={128,64,64,16,16};

static int glyph_bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
 if(stop_target && target-stop_target<12u) return 0;
 const uint32_t entries[]={BATTLE_GLYPH_RENDER_ADDRESS,BATTLE_GLYPH_FULL_ADDRESS,
  BATTLE_GLYPH_HALF_ADDRESS,BATTLE_STAGE3_UI_ADDRESS,BATTLE_STAGE3_GLYPHS_ADDRESS,BATTLE_STAGE3_FIRST_ADDRESS};
 const unsigned sizes[]={688,92,92,1032,936,40};
 for(unsigned i=0;i<6;i++) {
  if(target==entries[i]) {
   stage_trace(cpu,target,i==0 ? 7:(i<3 ? 4:0));
   int rc=candidate ? runtime_bridge(opaque,cpu,target):0;
   if(rc==1 && pending_return==target) {cpu->load_valid=1;cpu->load_reg=2;cpu->load_value=1;cpu->load_mask=0;}
   return rc;
  }
  if(target-entries[i]<sizes[i]) return 0;
 }
 unsigned argc;
 switch(target) {
 case 0x80026338:argc=8;break;
 case 0x80043a1c:argc=4;break;
 case 0x80043bfcu:case 0x80043c24:case 0x80043a58:argc=2;break;
 case 0x80043cb0:case 0x80043cd8:case 0x80043cc4:case 0x80043d78:case 0x80076c34:argc=1;break;
 default:fail("glyph unexpected service",target);return -1;
 }
 stage_trace(cpu,target,argc);service_calls++;
 if(runtime_bridge(opaque,cpu,target)!=1) {fail("glyph actual service",target);return -1;}
 if(perturb && service_calls==1) {
  /* Controlled bus writes after an actual SDK call exercise live frame and
   * count reads. They are test perturbations, not game-service substitutes. */
  write_at(cpu->gpr[29]+0x18u,2,0xfff3);
  write_at(cpu->gpr[29]+0x5cu,4,1);
  write_at(glyph_descriptor,2,2);
 }
 if(invalid_descriptor && service_calls==1) write_at(cpu->gpr[29]+0x20u,4,0x1e000000u);
 if(pending_return==target) {cpu->load_valid=1;cpu->load_reg=2;cpu->load_value=1;cpu->load_mask=0;}
 return failed ? -1:1;
}

static void glyph_setup(PcPortMipsCpu *cpu)
{
 memset(&rt,0,sizeof(rt));memset(g_PsxRam,0xa5,sizeof(g_PsxRam));
 memset(shared,0x5a,sizeof(shared));memset(g_PsxScratchpad,0xa9,sizeof(g_PsxScratchpad));
 memset(stage_host,0xb6,sizeof(stage_host));
 memcpy(PSX_ADDR(0x801e4000u),module,sizeof(module));
 memcpy(PSX_ADDR(BATTLE_GLYPH_FULL_ADDRESS),glyph_code,sizeof(glyph_code));
 memcpy(PSX_ADDR(BATTLE_GLYPH_RENDER_ADDRESS),renderer_code,sizeof(renderer_code));
 memcpy(PSX_ADDR(0x80043cb0u),ft4_code,sizeof(ft4_code));
 rt.data[0]=(ResolvedData){0x80050000u,0x10000u,shared,"shared-5"};
 rt.data[1]=(ResolvedData){0x80060000u,0x10000u,shared+0x10000u,"shared-6"};rt.data_count=2;
 rt.host_ranges[0]=(HostRange){(uintptr_t)stage_host,(uintptr_t)stage_host+sizeof(stage_host),1};rt.host_range_count=1;
 rt.initialized=1;g_ActiveBattleRuntime=&rt;
 memcpy(rt.functions,glyph_services,sizeof(glyph_services));rt.function_count=sizeof(glyph_services)/sizeof(glyph_services[0]);
 qsort(rt.functions,rt.function_count,sizeof(rt.functions[0]),function_compare);
 initialize_cpu(cpu,&rt);cpu->bus.bridge=glyph_bridge;
 for(unsigned i=1;i<29;i++) cpu->gpr[i]=0x12340000u+variant*256u+i;
 cpu->gpr[30]=0x87650000u+variant;
 if(variant&2u) cpu->gpr[29]=0x1f8003f8u;
 else if(variant&4u) cpu->gpr[29]|=0x20000000u;
 cpu->hi=0x98765432u;cpu->lo=0xfedcba98u;
 frame_base=cpu->gpr[29]-0x100u;calls=service_calls=0;stop_target=pending_return=0;invalid_descriptor=0;
 uint32_t owner=menu_owner(0),state=state_owner(0);
 memset(resolve_memory(&rt,owner,0xb000,1),0x6d,0xb000);
 memset(resolve_memory(&rt,state,0x110,1),0,0x110);
 glyph_table=variant%3==2 ? (uint32_t)(uintptr_t)stage_host+0x25000u :
             ((variant%3==1 ? 0xa0160000u:0x80160000u));
 write_at(0x800c3ea4u,4,owner);write_at(0x800d2d28u,4,state);
 write_at(0x800d2f5cu,4,glyph_table);
 const uint32_t slots[]={0,1,2,0xffffffffu};
 uint32_t slot=slots[variant%4];
 int caller=running_entry==BATTLE_STAGE3_GLYPHS_ADDRESS || running_entry==BATTLE_STAGE3_FIRST_ADDRESS;
 if(caller) slot=variant&1u;
 int alias=!caller && (variant&32u);
 if(alias) slot=0;
 perturb=!caller && (variant&64u);if(perturb) slot=0;
 write_at(0x800ccb34u,4,slot);write_at(0x800d3280u,1,(variant*37u)&255u);
 const uint16_t values[]={0,1,0x7fff,0x8000,0xffff,255,256,0xffef};
 for(unsigned i=0;i<768;i++) write_at(0x800c3254u+i*2u,2,values[(i+variant)%8]);
 for(unsigned i=0;i<3;i++) write_at(0x800c3eb6u+i*28u,1,(variant&(1u<<i)) ? (i==2?255:i):127);
 for(unsigned index=0;index<=0x91;index++) {
  uint32_t offset=0x400u+index*0x80u,descriptor=glyph_table+offset;
  write_at(glyph_table+4u+index*2u,2,offset);
  unsigned count=caller ? 1+variant%3 : variant%4;
  if(alias) count=1;
  if(perturb) count=3;
  write_at(descriptor,2,count);write_at(descriptor+2u,2,0);
  for(unsigned item=0;item<3;item++) {
   uint32_t p=descriptor+4u+item*0x1cu;
   for(unsigned field=0;field<13;field++) write_at(p+field*2u,2,values[(variant+field*3u+item)%8]);
   write_at(p+0x10u,2,(variant+item)%4); /* Native graph type zero tpage encoding. */
   write_at(p+0x1au,1,(variant>>1)&1u);write_at(p+0x1bu,1,(variant>>2)&1u);
  }
 }
 glyph_descriptor=glyph_table+0x400u+0x52u*0x80u;
 uint32_t output=alias ? glyph_descriptor+4u : owner+0x2000u+40u;
 if(!caller && (variant&16u) && !alias && !perturb) {
  output=cpu->gpr[29]+0x20u;slot=variant&1u;
  write_at(0x800ccb34u,4,slot);
 }
 const uint32_t coordinates[]={0,1,0x7fff,0x8000,0xffff,0xffffffffu,0x80000000u,0x7fffffffu};
 uint32_t x=coordinates[variant%8],y=coordinates[(variant/8)%8];
 const uint32_t scales[]={0,1,0x7ff,0x800,0xfff,0x1000,0x7fff,0x8000,0xffff,0xffff8000u,0x8000ffffu};
 if(running_entry==BATTLE_GLYPH_RENDER_ADDRESS) {
  cpu->gpr[4]=glyph_table;cpu->gpr[5]=0x52;cpu->gpr[6]=output;cpu->gpr[7]=slot;
  write_at(cpu->gpr[29]+0x10u,4,x);write_at(cpu->gpr[29]+0x14u,4,y);
  write_at(cpu->gpr[29]+0x18u,4,scales[variant%11]);
 } else {
  cpu->gpr[4]=0x52;cpu->gpr[5]=output;cpu->gpr[6]=x;cpu->gpr[7]=y;
 }
}

static int glyph_case(uint32_t entry)
{
 running_entry=entry;PcPortMipsCpu oracle,native;glyph_setup(&oracle);candidate=compare=0;
 if(PcPortMipsRun(&oracle,entry,BATTLE_HALT_PC,1000000)!=PC_PORT_MIPS_HALTED) {fail(oracle.error,oracle.pc);return 0;}
 unsigned expected_calls=calls;stage_snapshot();glyph_setup(&native);candidate=compare=1;
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
 PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
 int rc=PcPortMipsRun(&native,entry,BATTLE_HALT_PC,1000000);
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
 if(rc!=PC_PORT_MIPS_HALTED) {fail(native.error,native.pc);return 0;}
 if(native.steps || rt.bridge_cpu || calls!=expected_calls || PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)
  fail("glyph native instructions/context/calls",calls);
 for(unsigned i=16;i<32;i++) if(native.gpr[i]!=oracle.gpr[i]) fail("glyph preserved register",i);
 if(native.gpr[2]!=oracle.gpr[2] || native.gpr[3]!=oracle.gpr[3] || native.hi!=oracle.hi || native.lo!=oracle.lo) {
  fail("glyph return/HI-LO",entry);
  fprintf(stderr,"native V0/V1=%08x/%08x HI/LO=%08x/%08x original=%08x/%08x %08x/%08x\n",native.gpr[2],native.gpr[3],native.hi,native.lo,oracle.gpr[2],oracle.gpr[3],oracle.hi,oracle.lo);
 }
 if(!stage_equal()) fail("glyph final memory",entry);
 return !failed;
}

static int glyph_controls(void)
{
 PcPortMipsCpu cpu;variant=3;candidate=1;compare=0;
 for(unsigned i=0;i<3;i++) {
  running_entry=glyph_entries[i];glyph_setup(&cpu);rt.bridge_cpu=&cpu;stage_snapshot();
  unsigned bytes=i==0 ? 688:92;unsigned offsets[]={0,bytes/2,bytes-1};
  for(unsigned k=0;k<3;k++) {
   uint8_t *code=i==0 ? g_XenoRetailFiles[XENO_RD_SLUS].bytes+XENO_RD_SLUS_OFF(running_entry) : PSX_ADDR(running_entry);
   code[offsets[k]]^=1;
   if(battle_glyphs_try_host(&rt,&cpu,running_entry)) fail("glyph identity",k);
   code[offsets[k]]^=1;
  }
  cpu.load_valid=1;cpu.load_reg=4;if(battle_glyphs_try_host(&rt,&cpu,running_entry)) fail("glyph pending entry",i);
  cpu.load_valid=0;rt.force_interpret=1;if(battle_glyphs_try_host(&rt,&cpu,running_entry)) fail("glyph force reference",i);
  rt.force_interpret=0;assert(setenv("XENO_BATTLE_NATIVE_GLYPHS","0",1)==0);
  if(battle_glyphs_try_host(&rt,&cpu,running_entry)) fail("glyph environment",i);
  assert(unsetenv("XENO_BATTLE_NATIVE_GLYPHS")==0);g_ActiveBattleRuntime=NULL;
  if(battle_glyphs_try_host(&rt,&cpu,running_entry)) fail("glyph inactive",i);
  g_ActiveBattleRuntime=&rt;rt.bridge_cpu=NULL;
  if(battle_glyphs_try_host(&rt,&cpu,running_entry)) fail("glyph CPU context",i);
  rt.bridge_cpu=&cpu;rt.initialized=0;if(battle_glyphs_try_host(&rt,&cpu,running_entry)) fail("glyph init",i);
  rt.initialized=1;rt.bridge_cpu=NULL;if(calls || !stage_equal()) fail("glyph rejection writes",i);
 }
 const uint32_t targets[]={BATTLE_GLYPH_HALF_ADDRESS,BATTLE_GLYPH_RENDER_ADDRESS,0x80043cb0,0x80043bfc,0x80043c24,0x80043a1c,0x80043a58,0x80076c34};
 for(unsigned i=0;i<sizeof(targets)/sizeof(targets[0]);i++) for(unsigned pending=0;pending<2;pending++) {
  running_entry=BATTLE_STAGE3_FIRST_ADDRESS;variant=7;glyph_setup(&cpu);
  if(pending) pending_return=targets[i];else stop_target=targets[i];
  assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
  int rc=PcPortMipsRun(&cpu,running_entry,BATTLE_HALT_PC,1000000);
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
  if(rc!=PC_PORT_MIPS_UNRESOLVED_CALL || cpu.steps || rt.bridge_cpu || PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)
   fail("glyph nested refusal",targets[i]);
 }
 /* Both paths must retain the completed first packet before a later invalid
  * descriptor read. The native path reports structured refusal, never retries. */
 running_entry=BATTLE_GLYPH_RENDER_ADDRESS;variant=3;PcPortMipsCpu oracle;
 glyph_setup(&oracle);candidate=compare=0;invalid_descriptor=1;
 int reference_rc=PcPortMipsRun(&oracle,running_entry,BATTLE_HALT_PC,1000000);
 if(reference_rc!=PC_PORT_MIPS_FAULT || service_calls!=5) fail("glyph original partial fault",reference_rc);
 unsigned expected_calls=calls;stage_snapshot();glyph_setup(&cpu);candidate=compare=1;invalid_descriptor=1;
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
 PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
 int native_rc=PcPortMipsRun(&cpu,running_entry,BATTLE_HALT_PC,1000000);
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
 if(native_rc!=PC_PORT_MIPS_UNRESOLVED_CALL || cpu.steps || rt.bridge_cpu || calls!=expected_calls ||
    PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions || !stage_equal())
  fail("glyph partial writes/refusal",native_rc);
 return !failed;
}

static int ft4_bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
 return candidate && target==0x80043cb0u ? runtime_bridge(opaque,cpu,target):0;
}
static int ft4_checks(void)
{
 running_entry=0x80043cb0u;
 for(variant=0;variant<64;variant++) {
  PcPortMipsCpu oracle,native;glyph_setup(&oracle);perturb=0;
  uint32_t packets[]={0x80100000,0xa0100000,0x1f800800,(uint32_t)(uintptr_t)stage_host};
  uint32_t p=packets[variant%4];oracle.gpr[4]=p;oracle.bus.bridge=ft4_bridge;candidate=0;
  assert(PcPortMipsRun(&oracle,running_entry,BATTLE_HALT_PC,100)==PC_PORT_MIPS_HALTED);
  uint8_t expected[80];memcpy(expected,resolve_memory(&rt,p,sizeof(expected),0),sizeof(expected));
  glyph_setup(&native);native.gpr[4]=p;native.bus.bridge=ft4_bridge;candidate=1;
  assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  assert(PcPortMipsRun(&native,running_entry,BATTLE_HALT_PC,100)==PC_PORT_MIPS_HALTED);
  assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
  if(native.steps || rt.bridge_cpu || memcmp(expected,resolve_memory(&rt,p,sizeof(expected),0),sizeof(expected))) fail("glyph original FT4 packet",variant);
 }
 puts("BATTLE GLYPHS PASS 64 original SetPolyFT4 packet cases; service CPU frames/volatile state excluded; native steps=0");return !failed;
}

int main(int argc,char **argv)
{
 assert(argc==2 || argc==3 || argc==4);assert(setvbuf(stdout,NULL,_IOLBF,0)==0);
 FILE *f=fopen(argv[1],"rb");assert(f && fread(module,1,sizeof(module),f)==sizeof(module));assert(!fclose(f));
 f=fopen("disc/battle.bin","rb");assert(f && !fseek(f,0x80076a10u-0x8006faf0u,SEEK_SET));
 assert(fread(glyph_code,1,sizeof(glyph_code),f)==sizeof(glyph_code));assert(!fclose(f));
 f=fopen("disc/SLUS_006.64","rb");assert(f && !fseek(f,0x8002675cu-0x8000f800u,SEEK_SET));
 assert(fread(renderer_code,1,sizeof(renderer_code),f)==sizeof(renderer_code));assert(!fseek(f,0x80043cb0u-0x8000f800u,SEEK_SET));
 assert(fread(ft4_code,1,sizeof(ft4_code),f)==sizeof(ft4_code));assert(!fclose(f));
 assert(XenoRetailData_File(XENO_RD_SLUS));assert(setenv("XENO_CPU_EXECUTION_STATS","1",1)==0);
 if(argc==3 && !strcmp(argv[2],"--controls-only")) return glyph_controls()?0:1;
 if(argc==3 && !strcmp(argv[2],"--ft4-only")) return ft4_checks()?0:1;
 uint32_t selected=argc==4 && !strcmp(argv[2],"--entry") ? (uint32_t)strtoul(argv[3],NULL,0):0;
 for(unsigned entry=0;entry<5;entry++) {
  if(selected && selected!=glyph_entries[entry]) continue;
  for(variant=0;variant<glyph_counts[entry];variant++) if(!glyph_case(glyph_entries[entry])) return 1;
  printf("BATTLE GLYPHS completed entry=%08x cases=%u\n",glyph_entries[entry],glyph_counts[entry]);
 }
 if(selected) return failed?1:0;
 if(!glyph_controls() || !ft4_checks()) return 1;
 puts("BATTLE GLYPHS PASS 288 original game-body cases; actual atlas/style/SDK services; full memory/frames/HI-LO; live aliases/perturbations/identity/refusal; native steps=0");
 return 0;
}
