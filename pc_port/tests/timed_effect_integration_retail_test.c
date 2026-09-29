#include <stdio.h>
#include <string.h>
#ifndef OBJECT_OVERLAY_SOURCE
#define OBJECT_OVERLAY_SOURCE "../src/field_object_overlay.c"
#endif
#include OBJECT_OVERLAY_SOURCE
#ifndef TIMED_SOURCE
#define TIMED_SOURCE "../src/field_timed_commands.c"
#endif
#include TIMED_SOURCE
#include "field_effect_constructor.h"
#include "battle_mips_adapter.h"
#pragma weak PcPort_FieldEffectConstructWithS1
static u8 ram[0x200000];
u8 g_PsxScratchpad[4096]; /* overlay matrix scratch (address-only use) */
u8 g_PsxRam[3 * 1024 * 1024]; /* GTE RAM window (PSX_RAM_SIZE; BSS-zeroed) */
static struct {u32 effect[12],source[1024],blocks[3][1024],object[80],stream[64];} fixture,initial,expected;
static struct Call {u32 kind,a,b,rect[2],effect[12];} calls[8],expectedCalls[8];
static unsigned count,allocations;
static void record(u32 kind,u32 a,u32 b,void*rect){assert(count<8);calls[count]=(struct Call){.kind=kind,.a=a,.b=b};if(rect)memcpy(calls[count].rect,rect,8);memcpy(calls[count++].effect,fixture.effect,48);}
void* HeapAlloc(u32 size,u32 flags){record(1,size,flags,NULL);assert(allocations<3);return fixture.blocks[allocations++];}
u_int HeapFree(void*p){record(5,(u32)(uintptr_t)p,0,NULL);return 1;}
void HeapChangeCurrentUser(u32 user,char**data){record(2,user,(u32)(uintptr_t)data,NULL);}
int StoreImage(RECT*rect,u_long*destination){record(3,(u32)(uintptr_t)destination,0,rect);/* Controlled hardware readback; preserve its call ordering and output. */
 if(rect->w>0&&rect->h>0)for(int i=0;i<rect->w*rect->h;++i)((u16*)destination)[i]=(u16)(i+rect->x*37+rect->y*79);return 0;}
int DrawSync(int mode){record(4,mode,0,NULL);return 0;}
int LoadImage(RECT*rect,u_long*source){record(6,(u32)(uintptr_t)source,0,rect);return 0;}
static u8* address(u32 a,unsigned w){if(a==0x801e8644 && w==4)return(u8*)&D_801E8644;if(a>=0x80000000u&&(uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);uintptr_t lo=(uintptr_t)&fixture;if(a>=lo&&(uint64_t)a+w<=lo+sizeof(fixture))return(u8*)(uintptr_t)a;return NULL;}
static int rd(void*u,u32 a,unsigned w,u32*v){(void)u;u8*p=address(a,w);if(!p)return-1;*v=0;for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(8*i);return 0;}
static int wr(void*u,u32 a,unsigned w,u32 v){(void)u;u8*p=address(a,w);if(!p)return-1;for(unsigned i=0;i<w;++i)p[i]=(u8)(v>>(8*i));return 0;}
static int bridge(void*u,PcPortMipsCpu*c,u32 t){(void)u;switch(t){
 case 0x80032498:HeapChangeCurrentUser(c->gpr[4],(void*)(uintptr_t)c->gpr[5]);return 1;
 case 0x80031bdc:c->gpr[2]=(u32)(uintptr_t)HeapAlloc(c->gpr[4],c->gpr[5]);return 1;
 case 0x800448f8:c->gpr[2]=StoreImage((RECT*)address(c->gpr[4],8),(void*)(uintptr_t)c->gpr[5]);return 1;
 case 0x800320e8:c->gpr[2]=HeapFree((void*)(uintptr_t)c->gpr[4]);return 1;
 case 0x80044894:c->gpr[2]=LoadImage((RECT*)address(c->gpr[4],8),(void*)(uintptr_t)c->gpr[5]);return 1;
 case 0x800445d0:c->gpr[2]=DrawSync(c->gpr[4]);return 1;
 default:return t>=0x801dc000u && t<0x801e8800u?0:-1;}}
int main(void){
 FILE*f=fopen("disc/disc1.bin","rb");assert(f);for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));
 unsigned cases=0;
 for(unsigned type=0;type<8;++type)for(unsigned mode=0;mode<9;++mode)for(unsigned seed=0;seed<16;++seed)for(unsigned enabled=0;enabled<2;++enabled){
  for(unsigned i=0;i<sizeof(fixture)/4;++i)((u32*)&fixture)[i]=0xa55a1234u+i*8191;
  u8*effect=(u8*)fixture.effect;u8*obj=(u8*)fixture.object;u8*p=(u8*)fixture.stream;
  for(unsigned i=0;i<3;++i)*(u32*)(effect+4+i*4)=(u32)(uintptr_t)fixture.blocks[i];
  effect[0x10]=type;*(u16*)(effect+0x1a)=seed&8?1:0;
  *(s16*)(obj+0x98)=0;*(s16*)(obj+0x9a)=seed&4?1:-1;*(u16*)(obj+0x9c)=0;*(u16*)(obj+0x9e)=1;
  *(u32*)(obj+0xa0)=(u32)(uintptr_t)p;*(u32*)(obj+0xa4)=(u32)(uintptr_t)(p+64);
  *(u32*)(obj+0x118)=(u32)(uintptr_t)effect;obj[0x10e]=1;
  *(s16*)(obj+0x90)=seed&2?-1:1;*(u16*)(obj+0x94)=1;*(u16*)(obj+0x96)=2;
  *(u16*)p=0;p[2]=9;p[3]=0;p[4]=enabled;p[5]=seed&4?0:255;p[6]=type|(seed&1?0x80:0);p[7]=seed;
  *(u16*)(p+8)=1;*(s16*)(p+0xa)=(mode%3==2 && seed&4)?-1:1;
  *(u16*)(p+0xc)=1;*(u16*)(p+0xe)=1;*(u16*)(p+0x10)=0x4567;
  p[0x12]=(mode%3)|((mode/3)<<4);p[0x13]=seed&2?0:3;p[0x14]=1;
  *(u16*)(p+0x16)=0xffff;*(u16*)(p+0x18)=0x8000;*(u16*)(p+0x1a)=0x1234;
  D_801E8644=fixture.source;
  initial=fixture;count=allocations=0;memset(calls,0,sizeof(calls));
  PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
  cpu.gpr[4]=(u32)(uintptr_t)obj;cpu.gpr[5]=0x12345678;cpu.gpr[6]=seed;cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
  if(PcPortMipsRun(&cpu,0x801e5d44,0xfffffffcu,100000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"TIMED EFFECT FAIL oracle type=%u mode=%u seed=%u enabled=%u %s\n",type,mode,seed,enabled,cpu.error);return 1;}
  expected=fixture;memcpy(expectedCalls,calls,sizeof(calls));unsigned expectedCount=count;
  fixture=initial;count=allocations=0;memset(calls,0,sizeof(calls));
  PcPort_FieldTimedCommandsWithContext(obj,(void*)0x12345678,seed,(u32)(uintptr_t)obj,0);
  if(memcmp(&fixture,&expected,sizeof(fixture))||count!=expectedCount||memcmp(calls,expectedCalls,sizeof(calls))){fprintf(stderr,"TIMED EFFECT FAIL type=%u mode=%u seed=%u enabled=%u calls=%u/%u\n",type,mode,seed,enabled,count,expectedCount);return 1;}++cases;
 }
 printf("TIMED EFFECT PASS %u complete-memory/boundary comparisons; real timed commands, selector, constructor and cleanup; controlled heap/GPU\n",cases);
}
