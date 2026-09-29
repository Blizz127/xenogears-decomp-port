#include <stdint.h>
#include "battle_mips_adapter.h"
static int adoption_run(PcPortMipsCpu *,uint32_t,uint32_t,uint64_t);
#define PcPortMipsRun adoption_run
#ifndef BATTLE_RUNTIME_SOURCE
#define BATTLE_RUNTIME_SOURCE "../src/battle_mips_runtime.c"
#endif
#include BATTLE_RUNTIME_SOURCE
#undef PcPortMipsRun

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];
unsigned int MFC2(int reg) { (void)reg; abort(); }
unsigned int CFC2(int reg) { (void)reg; abort(); }
void MTC2(unsigned int v,int reg) { (void)v;(void)reg;abort(); }
void CTC2(unsigned int v,int reg) { (void)v;(void)reg;abort(); }
int doCOP2(int op) { (void)op;abort(); }
int g_GPUDisabledState;
char PsyX_BeginScene(void) { return 1; }
void ClearSplits(void) { abort(); }
void DrawAllSplits(void) { abort(); }
void ParsePrimitivesLinkedList(u_long *p,int s) { (void)p;(void)s;abort(); }

#include "oracle.inc"
static BattleMipsRuntime test_runtime;
static PcPortMipsCpu test_parent;
static uint8_t raw_state[RAM_SIZE];
static unsigned gate_checks,adopted_calls;
static int boundary_bad,guest_fault,bad_string,rebind_bad,nested_once;
static uint32_t archive_size=MODULE_SIZE;
static int archive_failure,archive_corrupt;
static unsigned rebind_flags;
static int invalidate_on_yield;

#define CHECK(label,condition) do { gate_checks++; if (!(condition)) { \
 fprintf(stderr,"FILE1 ADOPTION FAIL %s line=%d\n",label,__LINE__);return 0; } } while(0)
#define HOST_ARGS uintptr_t a0,uintptr_t a1,uintptr_t a2,uintptr_t a3,uintptr_t a4,uintptr_t a5,uintptr_t a6,uintptr_t a7,uintptr_t a8,uintptr_t a9,uintptr_t a10,uintptr_t a11
#define UNUSED_ARGS (void)a0;(void)a1;(void)a2;(void)a3;(void)a4;(void)a5;(void)a6;(void)a7;(void)a8;(void)a9;(void)a10;(void)a11
static uint32_t pointer_id(uintptr_t p)
{
 uint8_t *r=file1_ram(p,1,1);
 if(!r){boundary_bad=1;return 0;}
 return 0x80000000u+(uint32_t)(r-g_PsxRam);
}
static uintptr_t archive_set(HOST_ARGS)
{ UNUSED_ARGS;w32(guest(0x8004fe14u),3087);return 3087; }
static int32_t archive_decode(uint32_t index)
{ (void)index;return (int32_t)archive_size; }
static uintptr_t archive_load(HOST_ARGS)
{
 UNUSED_ARGS;
 if(archive_failure)return (uintptr_t)-1;
 uint8_t *p=file1_ram(a1,MODULE_SIZE,1);if(!p){boundary_bad=1;return (uintptr_t)-1;}
 memcpy(p,module_bytes,MODULE_SIZE);
 if(archive_corrupt)p[MODULE_SIZE-1]^=1;
 return 0;
}
static uintptr_t resident_ctor(HOST_ARGS)
{
 UNUSED_ARGS;
 /* This exact-12-word bridge spy observes the shared retail7/native8 ABI. */
 if(test_runtime.bridge_cpu->gpr[29]!=STACK-0x50u)boundary_bad=1;
 if(a6!=0 || (intptr_t)a7!=(int16_t)(uint16_t)a7)boundary_bad=1;
 uint32_t a[]={pointer_id(a0),(uint32_t)a1,(uint32_t)a2,(uint32_t)a3,
              (uint32_t)a4,(uint32_t)a5,(uint16_t)a7};
 ev(E_CTOR,a,7);w16(win()+0x10,4);return 0;
}
static uintptr_t resident_reset(HOST_ARGS)
{ UNUSED_ARGS;uint32_t a[]={pointer_id(a0)};ev(E_RESET,a,1);return 0; }
static uintptr_t resident_get(HOST_ARGS)
{
 UNUSED_ARGS;uint32_t a[]={pointer_id(a0),(uint32_t)a1};ev(E_GET_STRING,a,2);
 if(lookup_rebind)w32(guest(0x800d2dac),ALT_WINDOW_ADDR);
 if(rebind_bad)w32(guest(0x800d2dac),0x80200000u);
 return bad_string ? UINT64_C(0x180054000) : (uintptr_t)guest(STRING_ADDR);
}
static uintptr_t resident_queue(HOST_ARGS)
{ UNUSED_ARGS;uint32_t a[]={pointer_id(a0),pointer_id(a1)};ev(E_QUEUE,a,2);return 0; }
static uintptr_t resident_clear(HOST_ARGS)
{ UNUSED_ARGS;uint32_t a[]={pointer_id(a0)};ev(E_CLEAR_WAIT,a,1);w16(win()+0x10,r16(win()+0x10)&(uint16_t)~8);return 0; }
static uintptr_t resident_destroy(HOST_ARGS)
{ UNUSED_ARGS;uint32_t a[]={pointer_id(a0)};ev(E_DESTROY,a,1);return 0; }

static void prepare_runtime(void)
{
 memset(&test_runtime,0,sizeof(test_runtime));test_runtime.initialized=1;
 ResolvedFunction functions[]={
  {0x80028470,(void*)archive_set,"ArchiveSetIndex"},
  {0x800288ec,(void*)archive_decode,"ArchiveDecodeAlignedSize"},
  {0x800295d8,(void*)archive_load,"ArchiveReadFileToBuffer"},
  {0x80032f54,(void*)resident_ctor,"func_80032F54"},
  {0x80033728,(void*)resident_get,"GetStringEntry"},
  {0x800345e0,(void*)resident_clear,"func_800345E0"},
  {0x80034614,(void*)resident_reset,"func_80034614"},
  {0x800346d4,(void*)resident_destroy,"func_800346D4"},
  {0x80034714,(void*)resident_queue,"func_80034714"}
 };
 memcpy(test_runtime.functions,functions,sizeof(functions));test_runtime.function_count=sizeof(functions)/sizeof(*functions);
 initialize_cpu(&test_parent,&test_runtime);test_parent.gpr[29]=STACK;
 test_runtime.bridge_cpu=&test_parent;g_ActiveBattleRuntime=&test_runtime;
 boundary_bad=guest_fault=bad_string=rebind_bad=nested_once=0;rebind_flags=0;invalidate_on_yield=0;
 archive_failure=archive_corrupt=0;archive_size=MODULE_SIZE;
 w32(guest(0x8004fe48),0);w32(guest(0x8004fe14),3087);
}
static int load_module(uint32_t directory,uint32_t index,uint32_t destination)
{
 test_parent.gpr[4]=directory;test_parent.gpr[5]=0;
 if(runtime_bridge(&test_runtime,&test_parent,0x80028470)!=1)return 0;
 test_parent.gpr[4]=index;test_parent.gpr[5]=destination;
 test_parent.gpr[6]=0;test_parent.gpr[7]=0x80;
 return runtime_bridge(&test_runtime,&test_parent,0x800295d8)==1;
}
static void seed_after_load(const Case *c)
{
 w32(guest(0x801e9c1c),0x12345678);
 w32(guest(0x801e9c30),(uint32_t)c->saved_x);w32(guest(0x801e9c34),(uint32_t)c->saved_y);
 test_parent.gpr[4]=c->arg0;test_parent.gpr[5]=c->arg1;test_parent.gpr[6]=c->arg2;
 test_parent.gpr[31]=HALT;
}
static void maybe_rebind(uint32_t target)
{
 if(target!=0x800716d8u || !rebind_flags)return;
 if(rebind_flags&1){memcpy(guest(0x80056000),guest(CONTROL_ADDR),0x828);w32(guest(0x800d3278),0x80056000);}
 if(rebind_flags&2){memcpy(guest(0x80057000),ui(),0x200);w32(guest(0x800d2d28),0x80057000);}
 if(rebind_flags&4){memcpy(guest(ALT_WINDOW_ADDR),win(),0x98);w32(guest(0x800d2dac),ALT_WINDOW_ADDR);}
 rebind_flags=0;
}
static int adoption_bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
 switch(target){
 case 0x8008f8f4u:case 0x800716d8u:case 0x8008fa60u:case 0x801e6750u:case 0x801e5b00u:
  /* Only helper boundaries are stubbed. The CPU/frame came from the actual
   * production native-to-guest service; the controller gate is never stubbed. */
  if(cpu==&test_parent || cpu->gpr[29]!=STACK-0x50u || test_runtime.bridge_cpu!=cpu)boundary_bad=1;
  if(guest_fault)return -1;
  if(invalidate_on_yield && target==0x800716d8u)file1_invalidate(&test_runtime);
  if(nested_once && target==0x800716d8u){
   nested_once=0;PcPortMipsCpu nested=*cpu,saved=*cpu;
   uint8_t phase=guest(CONTROL_ADDR)[0x802];uint16_t flags=r16(win()+0x10);
   guest(CONTROL_ADDR)[0x802]=1;w16(win()+0x10,4);
   nested.gpr[29]-=0x20;
   if(runtime_bridge(&test_runtime,&nested,ENTRY)!=1 || test_runtime.bridge_cpu!=cpu || memcmp(&saved,cpu,sizeof(saved)))boundary_bad=1;
   guest(CONTROL_ADDR)[0x802]=phase;w16(win()+0x10,flags);
  }
  maybe_rebind(target);
  return bridge(opaque,cpu,target);
 default:
  if(target==ENTRY)adopted_calls++;
  return runtime_bridge(opaque,cpu,target);
 }
}
static int adoption_run(PcPortMipsCpu *cpu,uint32_t entry,uint32_t halt,uint64_t budget)
{
 cpu->bus.bridge=adoption_bridge;
 return PcPortMipsRun(cpu,entry,halt,budget);
}
static int oracle_bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{ maybe_rebind(target);return bridge(opaque,cpu,target); }

static int one_variant(const Case *c,unsigned rebind,int nested)
{
 Trace raw={0},native={0};PcPortMipsCpu cpu;PcPortMipsBus bus={0,bus_read,bus_write,oracle_bridge,0,0,0};
 init_common(c);w32(guest(0x8004fe48),0);w32(guest(0x8004fe14),3087);trace=&raw;rebind_flags=rebind&7;
 if(rebind&0x80)for(unsigned i=0;i<5;i++)w16(guest(0x801e9c10)+i*2,(uint16_t)(0xdead+i));
 PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[4]=c->arg0;cpu.gpr[5]=c->arg1;cpu.gpr[6]=c->arg2;
 cpu.gpr[29]=STACK;cpu.gpr[31]=HALT;audit_cpu=&cpu;
 int rr=PcPortMipsRun(&cpu,ENTRY,HALT,4096);audit_cpu=NULL;
 CHECK("oracle halts",rr==PC_PORT_MIPS_HALTED);rr=(int)cpu.gpr[2];memcpy(raw_state,g_PsxRam,RAM_SIZE);
 init_common(c);prepare_runtime();trace=&native;
 CHECK("real load bridge",load_module(0x20,1,MODULE_ADDR));
 CHECK("full load authority",test_runtime.file1.verified);seed_after_load(c);rebind_flags=rebind&7;nested_once=nested;
 if(rebind&0x80)for(unsigned i=0;i<5;i++)w16(guest(0x801e9c10)+i*2,(uint16_t)(0xdead+i));
 PcPortMipsCpu before=test_parent;unsigned seen_before=adopted_calls;
 int nr=adoption_run(&test_parent,ENTRY,HALT,4096);
 CHECK("adopted controller halts",nr==PC_PORT_MIPS_HALTED);
 CHECK("actual bridge entry selected",adopted_calls==seen_before+1);
 CHECK("helper ABI and context",!boundary_bad && test_runtime.bridge_cpu==&test_parent && file1_call==NULL);
 CHECK("return",rr==(int)test_parent.gpr[2]);
 CHECK("callee saved and SP",test_parent.gpr[29]==before.gpr[29] && !memcmp(test_parent.gpr+16,before.gpr+16,8*sizeof(uint32_t)));
 CHECK("complete helper trace",!memcmp(&raw,&native,sizeof(raw)) && !raw.overflow && !native.overflow);
 /* Raw compiler locals/saved registers within the 0x50-byte frame are not
  * native output. Everything else in physical RAM, including redzones and
  * all packed globals/module mutable bytes, is compared byte for byte. */
 size_t frame=(STACK-0x80000000u)-0x50u;
 if(memcmp(raw_state,g_PsxRam,frame) || memcmp(raw_state+frame+0x50,g_PsxRam+frame+0x50,RAM_SIZE-frame-0x50)){
  for(size_t i=0;i<RAM_SIZE;i++)if((i<frame || i>=frame+0x50)&&raw_state[i]!=g_PsxRam[i]){fprintf(stderr,"FILE1 ADOPTION DIFF %s address=%08zx raw=%02x native=%02x\n",c->name,i+0x80000000u,raw_state[i],g_PsxRam[i]);break;}
  CHECK("all nonframe RAM",0);
 }
 if(raw.n>audit_max_events)audit_max_events=raw.n;
 return 1;
}
static int one(const Case *c) { return one_variant(c,0,0); }
static const Case simple={.arg0=1,.arg1=0xff,.arg2=2,.x=28,.y=24,.units=8,.rows=4,.phase=0,.ui_bf=0,.window_flags=4,.name="adoption-gates"};
static int ready(void)
{ init_common(&simple);prepare_runtime();if(!load_module(0x20,1,MODULE_ADDR))return 0;seed_after_load(&simple);return test_runtime.file1.verified; }
static int adoption_gate_tests(void)
{
 Trace t={0};trace=&t;
 CHECK("load full correct payload",ready());
 uint64_t generation=test_runtime.file1.generation;
 CHECK("bad physical pointer cannot be masked",file1_ram(0x80252000u,4,4)==NULL);
 CHECK("correct immutable identity",file1_identity_current(&test_runtime));
 CHECK("bad module directory",load_module(0x21,1,MODULE_ADDR)&&!test_runtime.file1.verified);
 CHECK("unproven identity raw fallback",runtime_bridge(&test_runtime,&test_parent,ENTRY)==0);
 CHECK("ready again",ready());archive_corrupt=1;
 CHECK("mutable payload byte participates in initial identity",load_module(0x20,1,MODULE_ADDR)&&!test_runtime.file1.verified);
 CHECK("ready",ready());archive_failure=1;
 CHECK("failed overlapping reload invalidates",load_module(0x20,1,MODULE_ADDR)&&!test_runtime.file1.verified);
 CHECK("ready",ready());
 CHECK("wrong file identity",load_module(0x20,2,MODULE_ADDR)&&!test_runtime.file1.verified);
 CHECK("ready",ready());archive_size=MODULE_SIZE+4;
 CHECK("wrong size identity",load_module(0x20,1,MODULE_ADDR)&&!test_runtime.file1.verified);
 CHECK("ready",ready());
 CHECK("nonoverlap load preserves authority",load_module(0x20,2,0x80100000)&&test_runtime.file1.verified);
 CHECK("ready",ready());generation=test_runtime.file1.generation;
 CHECK("observed immutable write invalidates",runtime_write(&test_runtime,MODULE_ADDR,1,module_bytes[0])==0 && !test_runtime.file1.verified && test_runtime.file1.generation!=generation);
 CHECK("stale same bytes do not reauthorize",runtime_bridge(&test_runtime,&test_parent,ENTRY)==0);
 CHECK("ready",ready());guest(MODULE_ADDR)[0]^=1;
 CHECK("unobserved native immutable write caught",runtime_bridge(&test_runtime,&test_parent,ENTRY)==0&&!test_runtime.file1.verified);
 CHECK("ready",ready());runtime_write(&test_runtime,0x801e9c10,2,0x1234);
 CHECK("mutable defaults retain authority",file1_identity_current(&test_runtime));
 const uint32_t bad[]={0,0x00052000,0x80252000,0x80352000,0x18052000,0x80052001,0x801fff80};
 for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);i++){
  CHECK("ready",ready());w32(guest(0x800d2dac),bad[i]);PcPortMipsCpu before=test_parent;t=(Trace){0};
  CHECK("bad pointer stops adopted path",runtime_bridge(&test_runtime,&test_parent,ENTRY)==-1);
  CHECK("failure context and CPU retained",test_runtime.bridge_cpu==&test_parent&&file1_call==NULL&&!memcmp(&before,&test_parent,sizeof(before)));
 }
 CHECK("host high bits cannot alias RAM",file1_ram(UINT64_C(0x180052000),4,4)==NULL);
 CHECK("physical KSEG1 alias accepted",file1_ram(0xa0052000,0x98,4)==guest(WINDOW_ADDR));
 CHECK("native RAM pointer accepted",file1_ram((uintptr_t)guest(WINDOW_ADDR),0x98,4)==guest(WINDOW_ADDR));
 CHECK("ready",ready());guest_fault=1;t=(Trace){0};PcPortMipsCpu before=test_parent;
 CHECK("guest failure is hard stop",runtime_bridge(&test_runtime,&test_parent,ENTRY)==-1);
 CHECK("guest fault restoration",test_runtime.bridge_cpu==&test_parent&&file1_call==NULL&&!memcmp(&before,&test_parent,sizeof(before)));
 CHECK("ready",ready());invalidate_on_yield=1;t=(Trace){0};before=test_parent;
 CHECK("generation change in helper hard stop",runtime_bridge(&test_runtime,&test_parent,ENTRY)==-1);
 CHECK("generation failure restores caller context",test_runtime.bridge_cpu==&test_parent&&file1_call==NULL&&!memcmp(&before,&test_parent,sizeof(before)));
 CHECK("ready",ready());bad_string=1;t=(Trace){0};
 CHECK("high host result hard stop",runtime_bridge(&test_runtime,&test_parent,ENTRY)==-1);
 CHECK("ready",ready());rebind_bad=1;t=(Trace){0};
 CHECK("fresh bad window after lookup hard stop",runtime_bridge(&test_runtime,&test_parent,ENTRY)==-1);
 CHECK("ready",ready());g_ActiveBattleRuntime=NULL;
 CHECK("inactive adoption rejected",runtime_bridge(&test_runtime,&test_parent,ENTRY)==-1);
 CHECK("ready",ready());test_parent.gpr[29]=STACK+4;t=(Trace){0};
 CHECK("unaligned outgoing SP rejected",runtime_bridge(&test_runtime,&test_parent,ENTRY)==-1);
 CHECK("ready",ready());file1_invalidate(&test_runtime);
 CHECK("explicit lifecycle invalidation",runtime_bridge(&test_runtime,&test_parent,ENTRY)==0);
 return 1;
}
static int adoption_extra_cases(void)
{
 for(unsigned bits=1;bits<8;bits++)if(!one_variant(&simple,bits,0))return 0;
 if(!one_variant(&simple,0,1))return 0;
 Case closed=simple;closed.phase=1;closed.window_flags=0;closed.name="mutated-retail-defaults";
 if(!one_variant(&closed,0x80,0))return 0;
 Case wrapped=simple;wrapped.phase=1;wrapped.window_flags=12;wrapped.saved_x=(int32_t)0x80000000u;wrapped.saved_y=(int32_t)0xa0000000u;wrapped.name="pointer-shaped-guest-scalars";
 if(!one_variant(&wrapped,0,0))return 0;
 return 1;
}
#include "corpus.inc"
