#include <stdio.h>
#include <string.h>
#ifndef OBJECT_OVERLAY_SOURCE
#define OBJECT_OVERLAY_SOURCE "../src/field_object_overlay.c"
#endif
#include OBJECT_OVERLAY_SOURCE
#include "field_effect_constructor.h"
#include "battle_mips_adapter.h"
#pragma weak PcPort_FieldEffectConstructWithS1
static u8 ram[0x200000];
static struct {u32 effect[12],source[1024],blocks[3][1024];} fixture,initial,expected;
static struct Call {u32 kind,a,b,rect[2],effect[12];} calls[8],expectedCalls[8];
static unsigned count,allocations;
static void record(u32 kind,u32 a,u32 b,void*rect){assert(count<8);calls[count]=(struct Call){.kind=kind,.a=a,.b=b};if(rect)memcpy(calls[count].rect,rect,8);memcpy(calls[count++].effect,fixture.effect,48);}
void* HeapAlloc(u32 size,u32 flags){record(1,size,flags,NULL);assert(allocations<3);return fixture.blocks[allocations++];}
void HeapChangeCurrentUser(u32 user,char**data){record(2,user,(u32)(uintptr_t)data,NULL);}
int StoreImage(RECT*rect,u_long*destination){record(3,(u32)(uintptr_t)destination,0,rect);/* Controlled hardware readback; preserve its call ordering and output. */
 if(rect->w>0&&rect->h>0)for(int i=0;i<rect->w*rect->h;++i)((u16*)destination)[i]=(u16)(i+rect->x*37+rect->y*79);return 0;}
int DrawSync(int mode){record(4,mode,0,NULL);return 0;}
static u8* address(u32 a,unsigned w){if(a>=0x80000000u&&(uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);uintptr_t lo=(uintptr_t)&fixture;if(a>=lo&&(uint64_t)a+w<=lo+sizeof(fixture))return(u8*)(uintptr_t)a;return NULL;}
static int rd(void*u,u32 a,unsigned w,u32*v){(void)u;u8*p=address(a,w);if(!p)return-1;*v=0;for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(8*i);return 0;}
static int wr(void*u,u32 a,unsigned w,u32 v){(void)u;u8*p=address(a,w);if(!p)return-1;for(unsigned i=0;i<w;++i)p[i]=(u8)(v>>(8*i));return 0;}
static int bridge(void*u,PcPortMipsCpu*c,u32 t){(void)u;switch(t){
 case 0x80032498:HeapChangeCurrentUser(c->gpr[4],(void*)(uintptr_t)c->gpr[5]);return 1;
 case 0x80031bdc:c->gpr[2]=(u32)(uintptr_t)HeapAlloc(c->gpr[4],c->gpr[5]);return 1;
 case 0x800448f8:c->gpr[2]=StoreImage((RECT*)address(c->gpr[4],8),(void*)(uintptr_t)c->gpr[5]);return 1;
 case 0x800445d0:c->gpr[2]=DrawSync(c->gpr[4]);return 1;
 default:return 0;}}
int main(void){
 if(!PcPort_FieldEffectConstructWithS1){fputs("EFFECT CONSTRUCTOR FAIL missing native body\n",stderr);return 1;}
 FILE*f=fopen("disc/disc1.bin","rb");assert(f);for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));
 const s32 widths[]={1,3,0,-3,3,1,1};unsigned cases=0;
 for(unsigned type=0;type<11;++type)for(unsigned mode=0;mode<9;++mode)for(unsigned allocation=0;allocation<8;++allocation)for(unsigned dimension=0;dimension<7;++dimension)for(unsigned occupied=0;occupied<2;++occupied){
  u32 typeArg=0xa5000000u|(type<8?type:type==8?0x100:type==9?0x101:0xffff);
  for(unsigned i=0;i<sizeof(fixture)/4;++i)((u32*)&fixture)[i]=0xa55a1234u+i*8191;
  u8*effect=(u8*)fixture.effect;
  for(unsigned i=0;i<3;++i)*(u32*)(effect+4+i*4)=(u32)(uintptr_t)fixture.blocks[i];
  *(u16*)(effect+0x1a)=occupied?0xffff:0;
  u32 flags=0xdead0000u|allocation*256u|(mode%3)|((mode/3)<<4);
  s32 args[14]={0x1234,0x5678,0x9abc,1,2,3,0,(dimension==3||dimension==4)?-1:1,widths[dimension],dimension==5?-1:dimension==6?0:1,0xffff,0x8000,0x12345,0x801e0850};
  /* Type 4/5 source-copy coordinates must point into the allocated fixture. */
  if(type>=4 && mode%3==1){args[0]=1;args[1]=1;}
  u32 carry=0xaabb3456u+dimension;
  initial=fixture;count=allocations=0;memset(calls,0,sizeof(calls));
  PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
  cpu.gpr[4]=(u32)(uintptr_t)effect;cpu.gpr[5]=0x12345678;cpu.gpr[6]=typeArg;cpu.gpr[7]=flags;cpu.gpr[17]=carry;cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
  wr(NULL,0x801ff010,4,(u32)(uintptr_t)fixture.source);for(unsigned i=0;i<14;++i)wr(NULL,0x801ff014+i*4,4,args[i]);
  if(PcPortMipsRun(&cpu,0x801e0a00,0xfffffffcu,100000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"EFFECT CONSTRUCTOR FAIL oracle %s\n",cpu.error);return 1;}
  expected=fixture;memcpy(expectedCalls,calls,sizeof(calls));unsigned expectedCount=count;u32 result=cpu.gpr[2];
  fixture=initial;count=allocations=0;memset(calls,0,sizeof(calls));
  u8*got=PcPort_FieldEffectConstructWithS1(effect,(void*)0x12345678,typeArg,flags,(u8*)fixture.source,args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9],args[10],args[11],args[12],args[13],carry);
  if((u32)(uintptr_t)got!=result||memcmp(&fixture,&expected,sizeof(fixture))||count!=expectedCount||memcmp(calls,expectedCalls,sizeof(calls))){fprintf(stderr,"EFFECT CONSTRUCTOR FAIL type=%u mode=%u alloc=%u dimension=%u occupied=%u calls=%u/%u\n",type,mode,allocation,dimension,occupied,count,expectedCount);return 1;}++cases;
 }
 printf("EFFECT CONSTRUCTOR PASS %u complete-memory and boundary-snapshot comparisons; controlled heap and GPU\n",cases);
}
