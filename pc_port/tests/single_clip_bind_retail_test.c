#include <stdio.h>
#include <string.h>
#ifndef OBJECT_OVERLAY_SOURCE
#define OBJECT_OVERLAY_SOURCE "../src/field_object_overlay.c"
#endif
#include OBJECT_OVERLAY_SOURCE
#include "battle_mips_adapter.h"
/* Old provisional entry retains this dependency; this fixture must never
 * execute its geometry path. Keep the guard to detect accidental fallback. */
__attribute__((weak))
VECTOR* ApplyMatrix(MATRIX*m,SVECTOR*v,VECTOR*out){(void)m;(void)v;(void)out;assert(!"SINGLE CLIP unexpected provisional geometry");return NULL;}
/* Heap-user tag written by the clip prelude; unobserved by this test. */
void HeapChangeCurrentUser(u32 user,char**data){(void)user;(void)data;}
extern void func_801E8330(s32,s32,s32) __attribute__((weak));
extern void func_801E35D0(u8*,u8*,void*,s32) __attribute__((weak));
static u8 ram[0x200000];
u8 g_PsxScratchpad[4096]; /* overlay matrix scratch (address-only use) */
u8 g_PsxRam[3 * 1024 * 1024]; /* GTE RAM window (PSX_RAM_SIZE; BSS-zeroed) */
static struct {u32 objects[2][128],table[512],aux[4];} fixture,initial,expected;
static struct {u32 count,args[5],snapshot[128];} call,expectedCall;
#ifndef XENO_TEST_MISSING_VM
void func_801E39F0(u8*obj,void*context,s32 limit,s32 ticks,s32 mode){++call.count;call.args[0]=(u32)(uintptr_t)obj;call.args[1]=(u32)(uintptr_t)context;call.args[2]=limit;call.args[3]=ticks;call.args[4]=mode;memcpy(call.snapshot,obj,sizeof(call.snapshot));}
#endif
static u8* address(u32 a,unsigned w){if(a>=0x801e8670u && (uint64_t)a+w<=0x801e8698u)return (u8*)D_801E8670+(a-0x801e8670u);if(a==0x801e863c && w==2)return (u8*)&D_801E863C;if(a==0x801e86b0 && w==2)return (u8*)&D_801E86B0;if(a>=0x80000000u&&(uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);uintptr_t lo=(uintptr_t)&fixture;if(a>=lo&&(uint64_t)a+w<=lo+sizeof(fixture))return(u8*)(uintptr_t)a;return NULL;}
static int rd(void*u,u32 a,unsigned w,u32*v){(void)u;u8*p=address(a,w);if(!p)return-1;*v=0;for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(8*i);return 0;}
static int wr(void*u,u32 a,unsigned w,u32 v){(void)u;u8*p=address(a,w);if(!p)return-1;for(unsigned i=0;i<w;++i)p[i]=(u8)(v>>(8*i));return 0;}
static int bridge(void*u,PcPortMipsCpu*c,u32 t){(void)u;if(t!=0x801e39f0)return 0;u32 mode;assert(!rd(NULL,c->gpr[29]+16,4,&mode));assert(c->gpr[5]==0x801e86a8);func_801E39F0((u8*)(uintptr_t)c->gpr[4],D_801E86A8,c->gpr[6],c->gpr[7],mode);return 1;}
static int oracle(u32 pc,u32 a,u32 b,u32 c,u32 d){PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[4]=a;cpu.gpr[5]=b;cpu.gpr[6]=c;cpu.gpr[7]=d;cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;if(PcPortMipsRun(&cpu,pc,0xfffffffcu,2000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"CLIP BIND FAIL oracle %s\n",cpu.error);return 0;}return 1;}
static void prepare(unsigned seed){for(unsigned i=0;i<sizeof(fixture);++i)((u8*)&fixture)[i]=(u8)(i*37+seed*63);fixture.objects[0][2]=fixture.objects[1][2]=(u32)(uintptr_t)(fixture.table+128);fixture.objects[0][3]=fixture.objects[1][3]=(u32)(uintptr_t)fixture.aux;fixture.aux[1]=(u32)(uintptr_t)(fixture.table+128);for(unsigned i=0;i<512;++i)fixture.table[i]=(i*199+seed*313)&0xffff;}
int main(void){
#ifdef XENO_TEST_MISSING_VM
 prepare(0);((u8*)fixture.objects[0])[0x2b]=0;
 D_801E8670[0]=(u32)(uintptr_t)fixture.objects[0];
 func_801E8330(0,1,0);
 fputs("SINGLE CLIP FAIL missing interpreter returned\n",stderr);return 1;
#endif
 if(!func_801E8330){fputs("SINGLE CLIP FAIL missing native entry\n",stderr);return 1;}
 FILE*f=fopen("disc/disc1.bin","rb");assert(f);for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));
 unsigned cases=0;const s32 indices[]={-128,-1,0,79,80,81,255};const unsigned queues[]={0,1,5,255};
 for(unsigned caseIndex=0;caseIndex<65536+2520;++caseIndex){
  unsigned phase=caseIndex,slot=phase%10,queue=queues[(phase/10)%4],animIndex=(phase/40)%7;
  if(caseIndex>=65536){unsigned n=caseIndex-65536;slot=n%10;n/=10;phase=n%9==8?0:1u<<(n%9);n/=9;queue=queues[n%4];animIndex=n/4;}
  prepare(phase);u8*obj=(u8*)fixture.objects[0];obj[0x2b]=queue;
  ((u8*)fixture.objects[1])[0x20]^=0x5a;
  fixture.objects[1][2]=(u32)(uintptr_t)(fixture.table+160);
  for(unsigned i=0;i<10;++i)D_801E8670[i]=(u32)(uintptr_t)obj;
  D_801E863C=0x1234;D_801E86B0=0x5678;
  initial=fixture;memset(&call,0,sizeof(call));
  s32 slotArg=(s32)(0xa55a0000u|slot),maskArg=(s32)(0x12340000u|phase),anim=indices[animIndex];
  assert(oracle(0x801e8330,slotArg,maskArg,anim,0));
  expected=fixture;expectedCall=call;s16 expectedMask=D_801E863C,expectedSlot=D_801E86B0;
  fixture=initial;memset(&call,0,sizeof(call));D_801E863C=0x1234;D_801E86B0=0x5678;
  func_801E8330(slotArg,maskArg,anim);
  if(memcmp(&fixture,&expected,sizeof(fixture))||memcmp(&call,&expectedCall,sizeof(call))||D_801E863C!=expectedMask||D_801E86B0!=expectedSlot){
   fprintf(stderr,"SINGLE CLIP FAIL phase=%u slot=%u queue=%u anim=%d\n",phase,slot,queue,anim);return 1;
  }++cases;
 }
 printf("SINGLE CLIP PASS %u memory/global/call comparisons; actual retail entry and binder, controlled VM\n",cases);
}
