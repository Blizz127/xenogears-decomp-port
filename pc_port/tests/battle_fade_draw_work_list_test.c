/* Real native producer -> packed work-list pump -> fade renderer -> cleanup. */
#define COLOR_FADE_LIST_MAIN prior_color_fade_list_main
#include "battle_color_fade_integration_test.c"
extern void WorkListUpdate(void);
static unsigned rendered_packets,render_pumps;
static void render(PcPortMipsCpu *outer,unsigned capacity)
{
 const uint32_t start=0x80160000u,table=0x80170000u;
 uint8_t colors[2][3],blend[2];unsigned entries=0;
 for(uint8_t *record=(uint8_t *)g_WorkList;record;record=(uint8_t *)(uintptr_t)load_le(record+0x18u,4)){
  assert(entries<2&&load_le(record+8u,4)==0x800b3878u);
  uint32_t owner=load_le(record+4u,4);
  for(unsigned i=0;i<3;i++){uint32_t value;assert(model_test_raw_read(&rt,owner+0x48u+i,1,&value)==0);colors[entries][i]=(uint8_t)value;}
  uint32_t value;assert(model_test_raw_read(&rt,owner+0x41u,1,&value)==0);blend[entries++]=(uint8_t)value;
 }
 unsigned count=entries<capacity?entries:capacity;
 fixture_store(0x80059580u,4,start);fixture_store(0x80059534u,4,start+0x50u+(capacity?1u+0x24u*(capacity-1u):0u));
 fixture_store(0x8005956cu,4,table);fixture_store(table+8u,4,0x8d112233u);
 memset(PSX_ADDR(start),0xb7,0x100);
 PcPortMipsCpu saved=*outer;PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();rt.bridge_cpu=outer;
 WorkListUpdate();assert(rt.bridge_cpu==outer&&!memcmp(outer,&saved,sizeof(saved)));rt.bridge_cpu=NULL;
 assert(PcPortMipsExecutionStatsGet().interpreted_instructions==before.interpreted_instructions);
 assert(load_word(0x80059580u)==start+0x24u*count);
 uint32_t previous=0x112233u;
 for(unsigned packet=0;packet<count;packet++){
  uint32_t p=start+packet*0x24u;
  assert(load_word(p)==(0x05000000u|previous));assert(load_word(p+0x18u)==(0x02000000u|(p&0xffffffu)));
  assert(load_word(p+0x1cu)==(0xe1000000u|((blend[packet]&3u)<<5)));assert(!load_word(p+0x20u));
  assert(!memcmp(PSX_ADDR(p+4u),colors[packet],3)&&*((uint8_t *)PSX_ADDR(p+7u))==0x2a);
  const int16_t xy[]={-32,-32,320,-32,-32,240,320,240};assert(!memcmp(PSX_ADDR(p+8u),xy,sizeof(xy)));
  previous=(p+0x18u)&0xffffffu;
 }
 assert(load_word(table+8u)==(0x8d000000u|previous));rendered_packets+=count;render_pumps++;
}
static void composition(unsigned flag,unsigned capacity)
{
 start_environment(flag);PcPortMipsCpu outer;initialize_cpu(&outer,&rt);
 outer.gpr[29]=flag&1u?(uint32_t)(uintptr_t)(host+0x70000u):0x801c0000u;
 setup_fade(2,1,255);verify_record(0x800c3c00u);setup_fade(2,0,127);verify_record(0x800c3c50u);
 assert(g_NumTimerWorkListEntries==2&&g_NumWorkListEntries==2);
 for(unsigned i=0;i<4;i++){render(&outer,capacity);pump(&outer);}
 assert(PcPort_BattleMipsDispatchCallback(0x800b383cu,PSX_ADDR(0x800c3c00u))==1);
 assert(g_NumTimerWorkListEntries==1&&g_NumWorkListEntries==1);render(&outer,capacity);
 assert(PcPort_BattleMipsDispatchCallback(0x800b383cu,PSX_ADDR(0x800c3c50u))==1);
 assert(!g_TimerWorkList&&!g_WorkList&&!g_NumTimerWorkListEntries&&!g_NumWorkListEntries&&D_80059464==7);
 render(&outer,capacity);
}
int main(void)
{
 load_inputs();assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
 PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();unsigned total=0;
 for(unsigned flag=0;flag<256;flag++)for(unsigned capacity=0;capacity<3;capacity++){composition(flag,capacity);total++;}
 assert(!native_gte_commands&&PcPortMipsExecutionStatsGet().interpreted_instructions==before.interpreted_instructions);
 printf("FADE DRAW WORK LIST PASS %u producer/render/timer/cleanup compositions, %u real pumps, %u verified packet pairs; original packed callbacks and live colors, two parent stack domains, capacity boundaries, middle removal, native interpreted instructions=0\n",total,render_pumps,rendered_packets);
 return 0;
}
