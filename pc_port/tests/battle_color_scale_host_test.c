/* Complete 80-byte color helper, using the production runtime and a test-only
 * original-code oracle. No child services or data accesses are modeled. */
#define main color_scale_model_main
#include "battle_effects_render_host_test.c"
#undef main
static const uint32_t color_target=0x800aa514u;
static unsigned color_cases;
static int color_read(void *opaque,uint32_t address,unsigned width,uint32_t *value)
{
 (void)opaque;
 if(address-color_target>=80u||width!=4)fail("unexpected color memory read",address);
 *value=load_le(battle+(address-0x8006faf0u),4);return 0;
}
static void color_cpu(PcPortMipsCpu *cpu,uint32_t a,uint32_t b,uint32_t c)
{
 initialize_cpu(cpu,&rt);
 for(unsigned i=1;i<32;i++)cpu->gpr[i]=0x45670000u+color_cases*128u+i;
 for(unsigned i=0;i<32;i++)cpu->cp0[i]=0xa8760000u+i;
 cpu->gpr[4]=a;cpu->gpr[5]=b;cpu->gpr[6]=c;cpu->gpr[29]=0x1e000001u;cpu->gpr[31]=BATTLE_HALT_PC;
 cpu->hi=0x12345678u;cpu->lo=0xfedcba98u;
}
static void color_case(uint32_t a,uint32_t b,uint32_t c)
{
 PcPortMipsCpu original,native;color_cpu(&original,a,b,c);original.bus.bridge=NULL;original.bus.read=color_read;
 int rc=PcPortMipsRun(&original,color_target,BATTLE_HALT_PC,100);
 if(rc!=PC_PORT_MIPS_HALTED||!original.steps||original.load_valid)fail("original color result",original.pc);
 color_cpu(&native,a,b,c);PcPortMipsExecutionStats stats=PcPortMipsExecutionStatsGet();
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
 rc=PcPortMipsRun(&native,color_target,BATTLE_HALT_PC,100);
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
 if(rc!=PC_PORT_MIPS_HALTED||memcmp(original.gpr,native.gpr,sizeof(native.gpr))||memcmp(original.cp0,native.cp0,sizeof(native.cp0))||original.hi!=native.hi||original.lo!=native.lo||native.load_valid||native.steps||rt.bridge_cpu||accesses||!memory_equal()||PcPortMipsExecutionStatsGet().interpreted_instructions!=stats.interpreted_instructions)fail("color complete state",color_cases);
 /* Independent arithmetic witness uses division, with explicit numeric
  * low-halfword interpretation instead of the production shifts/casts. */
 int64_t x=(a&65535u)>=32768u?(int64_t)(a&65535u)-65536:(a&65535u);
 int64_t y=(b&65535u)>=32768u?(int64_t)(b&65535u)-65536:(b&65535u);
 uint32_t sum=(uint32_t)(x*y/256)+c;
 int32_t low=(sum&65535u)>=32768u?(int32_t)(sum&65535u)-65536:(int32_t)(sum&65535u);
 uint32_t v1=low>=256?255u:sum;
 if(native.gpr[2]!=(v1&255u)||native.gpr[3]!=v1)fail("independent signed Q8/clamp witness",color_cases);
 color_cases++;
}
static unsigned color_controls(void)
{
 defaults();PcPortMipsCpu cpu;setup(&cpu);observing=0;rt.bridge_cpu=&cpu;PcPortMipsCpu old=cpu;snapshot();
 uint8_t *code=PSX_ADDR(color_target);unsigned total=0;
 for(unsigned i=0;i<3;i++){unsigned off=i==0?0:i==1?40:79;code[off]^=1;if(battle_color_scale_try_host(&rt,&cpu,color_target))fail("color full identity",off);code[off]^=1;total++;}
 /* Preserve the first digest byte while changing the body, so a shortened
  * digest comparison cannot pass merely by rejecting three easy corruptions. */
 uint8_t original_digest[32],changed_digest[32],saved[2]={code[38],code[39]};
 assert(SHA256(code,80,original_digest));unsigned found=0;
 for(unsigned value=0;value<65536u;value++){
  code[38]=(uint8_t)value;code[39]=(uint8_t)(value>>8);
  if(code[38]==saved[0]&&code[39]==saved[1])continue;
  assert(SHA256(code,80,changed_digest));
  if(changed_digest[0]==original_digest[0]&&memcmp(changed_digest,original_digest,32)){found=1;break;}
 }
 if(!found||battle_color_scale_try_host(&rt,&cpu,color_target))fail("color digest-prefix collision",0);
 code[38]=saved[0];code[39]=saved[1];total++;
 cpu.load_valid=1;cpu.load_reg=4;if(battle_color_scale_try_host(&rt,&cpu,color_target))fail("color pending entry",0);cpu=old;total++;
 rt.force_interpret=1;if(battle_color_scale_try_host(&rt,&cpu,color_target))fail("color forced entry",0);rt.force_interpret=0;total++;
 rt.initialized=0;if(battle_color_scale_try_host(&rt,&cpu,color_target))fail("color initialization",0);rt.initialized=1;total++;
 g_ActiveBattleRuntime=NULL;if(battle_color_scale_try_host(&rt,&cpu,color_target))fail("color active runtime",0);g_ActiveBattleRuntime=&rt;total++;
 rt.bridge_cpu=NULL;if(battle_color_scale_try_host(&rt,&cpu,color_target))fail("color bridge context",0);rt.bridge_cpu=&cpu;total++;
 if(battle_color_scale_try_host(&rt,&cpu,color_target+4u))fail("color wrong target",0);
 total++;
 assert(setenv("XENO_BATTLE_NATIVE_COLOR_SCALE","0",1)==0);if(battle_color_scale_try_host(&rt,&cpu,color_target))fail("color opt out",0);assert(unsetenv("XENO_BATTLE_NATIVE_COLOR_SCALE")==0);total++;
 if(memcmp(&cpu,&old,sizeof(cpu))||!memory_equal())fail("color gates changed state",0);
 return total;
}
static unsigned color_boundaries(void)
{
 unsigned total=0;
 for(unsigned cached=0;cached<2;cached++)for(unsigned disabled=0;disabled<2;disabled++)for(unsigned reject=0;reject<3;reject++){
  defaults();PcPortMipsCpu cpu;setup(&cpu);observing=0;cpu.bus.bridge=runtime_bridge;uint8_t *code=PSX_ADDR(color_target);
  rt.functions[0]=(ResolvedFunction){color_target,(void *)legacy_probe,"color_scale"};rt.function_count=1;
  if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,color_target);slot->target=color_target;slot->state=1;slot->entry=rt.functions[0];}
  if(reject==1)code[0]^=1;
  if(reject==2)assert(setenv("XENO_BATTLE_NATIVE_COLOR_SCALE","0",1)==0);
  if(disabled)assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
  PcPortMipsCpu old=cpu;PcPortMipsExecutionStats stats=PcPortMipsExecutionStatsGet();snapshot();
  int rc=reject&&!disabled?runtime_bridge(&rt,&cpu,color_target):PcPortMipsRun(&cpu,color_target,BATTLE_HALT_PC,100);
  if(reject==1)code[0]^=1;
  if(reject==2)assert(unsetenv("XENO_BATTLE_NATIVE_COLOR_SCALE")==0);
  if(disabled)assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
  /* Restore the identity byte in the memory snapshot before comparison. */
  if(reject==1)want_ram[color_target&(PSX_RAM_SIZE-1u)]^=1;
  if(rc!=(reject?(disabled?PC_PORT_MIPS_INTERPRETER_DISABLED:0):PC_PORT_MIPS_HALTED)||legacy_calls||cpu.steps||rt.bridge_cpu||!memory_equal()||PcPortMipsExecutionStatsGet().interpreted_instructions!=stats.interpreted_instructions)fail("color canonical cache boundary",(uint32_t)rc);
  if(reject&&memcmp(cpu.gpr,old.gpr,sizeof(cpu.gpr)))fail("color rejected state",reject);
  total++;
 }
 return total;
}
int main(void)
{
 load_inputs();defaults();PcPortMipsCpu cpu;setup(&cpu);observing=1;snapshot();
 static const int16_t values[]={-32768,-32767,-256,-255,-33,-32,-1,0,1,31,32,127,255,256,32766,32767};
 static const uint32_t bases[]={0,255,256,32767,32768,65535,0x7fffffffu,0xffffffffu};
 for(unsigned i=0;i<16;i++)for(unsigned j=0;j<16;j++)for(unsigned k=0;k<8;k++)color_case(0xface0000u|(uint16_t)values[i],0xbeef0000u|(uint16_t)values[j],bases[k]);
 uint32_t random=0x19711224u;
 for(unsigned i=0;i<1024;i++){uint32_t v[3];for(unsigned j=0;j<3;j++){random=random*1664525u+1013904223u;v[j]=random;}color_case(v[0],v[1],v[2]);}
 printf("COLOR SCALE PASS %u full original/native comparisons and independent arithmetic witnesses; no memory/graphics operations, native interpreted instructions=0\n",color_cases);
 printf("COLOR SCALE GATES PASS %u cases\n",color_controls());
 printf("COLOR SCALE CACHE PASS %u cases\n",color_boundaries());return 0;
}
