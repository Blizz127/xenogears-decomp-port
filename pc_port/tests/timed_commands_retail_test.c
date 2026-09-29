#include <assert.h>
#include <stdio.h>
#include <string.h>
#ifndef TIMED_SOURCE
#define TIMED_SOURCE "../src/field_timed_commands.c"
#endif
#include TIMED_SOURCE
#include "battle_mips_adapter.h"
u32 D_801E8670[10];u16 D_801E8648[20];void* D_801E8644;s16 D_801E86B0,D_801E863C;
static u8 ram[0x200000];
static struct {u32 object[80],stream[64],nodes[124],trails[56],effects[24],source[32];} fixture,initial,expected;
static u32 calls[64][24],expectedCalls[64][24];static unsigned count;
static void record(u32 kind,const u32*args,unsigned n){assert(count<64);calls[count][0]=kind;memcpy(calls[count]+1,args,n*4);++count;}
s32 func_801E0844(u8*p,s32 frame){u32 a[]={(u32)(uintptr_t)p,(u32)frame};record(1,a,2);*(u16*)p=0xffff;return -1;}
void func_801E8330(s32 slot,s32 mask,s32 anim){u32 a[]={slot,mask,anim};record(2,a,3);D_801E86B0=7;D_801E863C=0x55;}
void func_801E8394(u8*obj,s32 slot,s32 mask,s32 anim){u32 a[]={(u32)(uintptr_t)obj,slot,mask,anim};record(3,a,4);D_801E86B0=6;D_801E863C=0xaa;}
u32 func_801E34BC(s32 selector){u32 a[]={selector};record(4,a,1);return 0x801e0850u+(u32)selector*4;}
void func_801E165C(u8*p){u32 a[]={(u32)(uintptr_t)p};record(5,a,1);*(u16*)(p+0x1a)=0;}
u8* PcPort_FieldEffectConstructWithS1(u8*e,u8*p,s32 t,s32 f,u8*s,s32 ax,s32 ay,s32 az,s32 bx,s32 by,s32 bz,s32 x,s32 y,s32 w,s32 h,s32 period,s32 p0,s32 p1,u32 cb,u32 carry){
 u32 a[]={(u32)(uintptr_t)e,(u32)(uintptr_t)p,t,f,(u32)(uintptr_t)s,ax,ay,az,bx,by,bz,x,y,w,h,period,p0,p1,cb,carry};record(6,a,20);*(u16*)(e+0x1a)=1;return e;}
static u8* address(u32 a,unsigned w){
 if(a>=0x801e8670u&&(uint64_t)a+w<=0x801e8698u)return(u8*)D_801E8670+a-0x801e8670u;
 if(a>=0x801e8648u&&(uint64_t)a+w<=0x801e8670u)return(u8*)D_801E8648+a-0x801e8648u;
 if(a==0x801e8644&&w==4)return(u8*)&D_801E8644;
 if(a==0x801e863c&&w==2)return(u8*)&D_801E863C;
 if(a==0x801e86b0&&w==2)return(u8*)&D_801E86B0;
 if(a>=0x80000000u&&(uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);
 uintptr_t lo=(uintptr_t)&fixture;if(a>=lo&&(uint64_t)a+w<=lo+sizeof(fixture))return(u8*)(uintptr_t)a;return NULL;
}
static int rd(void*u,u32 a,unsigned w,u32*v){(void)u;u8*p=address(a,w);if(!p)return-1;*v=0;for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(8*i);return 0;}
static int wr(void*u,u32 a,unsigned w,u32 v){(void)u;u8*p=address(a,w);if(!p)return-1;for(unsigned i=0;i<w;++i)p[i]=(u8)(v>>(8*i));return 0;}
static int bridge(void*u,PcPortMipsCpu*c,u32 t){(void)u;u32*a=c->gpr+4;switch(t){
 case 0x801e0844:c->gpr[2]=func_801E0844((void*)(uintptr_t)a[0],a[1]);return 1;
 case 0x801e8330:func_801E8330(a[0],a[1],a[2]);return 1;
 case 0x801e8394:func_801E8394((void*)(uintptr_t)a[0],a[1],a[2],a[3]);return 1;
 case 0x801e34bc:c->gpr[2]=func_801E34BC(a[0]);return 1;
 case 0x801e165c:func_801E165C((void*)(uintptr_t)a[0]);return 1;
 case 0x801e0a00:{u32 x[15];for(unsigned i=0;i<15;++i)assert(!rd(NULL,c->gpr[29]+16+i*4,4,x+i));c->gpr[2]=(u32)(uintptr_t)PcPort_FieldEffectConstructWithS1((void*)(uintptr_t)a[0],(void*)(uintptr_t)a[1],a[2],a[3],(void*)(uintptr_t)x[0],x[1],x[2],x[3],x[4],x[5],x[6],x[7],x[8],x[9],x[10],x[11],x[12],x[13],x[14],c->gpr[17]);return 1;}
 default:return 0;}}
int main(void){
 FILE*f=fopen("disc/disc1.bin","rb");assert(f);for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));
 unsigned cases=0;
 for(unsigned command=0;command<256;++command)for(unsigned seed=0;seed<256;++seed)for(unsigned stack=0;stack<2;++stack){
  for(unsigned i=0;i<sizeof(fixture)/4;++i)((u32*)&fixture)[i]=i*8191u+seed*0x12345u;
  u8*obj=(u8*)fixture.object;u8*p=(u8*)fixture.stream;
  *(u32*)(obj+4)=(u32)(uintptr_t)fixture.nodes;*(u32*)(obj+0x110)=(u32)(uintptr_t)fixture.trails;*(u32*)(obj+0x118)=(u32)(uintptr_t)fixture.effects;
  *(s16*)(obj+0x98)=seed&8?-1:seed%3;*(s16*)(obj+0x9a)=seed&16?-1:seed%3+(seed&128?1:0);*(u16*)(obj+0x9c)=seed%2;*(u16*)(obj+0x9e)=1+seed%3;
  *(u32*)(obj+0xa0)=(u32)(uintptr_t)p;*(u32*)(obj+0xa4)=(u32)(uintptr_t)(p+128);
  *(u16*)(obj+0x10a)=(u16)(seed*37);obj[0x10e]=seed%3;*(s16*)(obj+0x90)=seed&32?-1:1;
  *(s16*)p=seed&4?3:*(s16*)(obj+0x98);p[2]=command;p[3]=seed%2;p[4]=seed%2;p[5]=seed&2?255:1;p[6]=(u8)((seed&64?0x80:0)|seed%6);p[7]=seed%4;
  p[0x12]=(seed%3)<<4|seed%3;p[0x13]=1;p[0x14]=1;
  for(unsigned i=0;i<10;++i)D_801E8670[i]=(seed+i)%3?(u32)(uintptr_t)obj:0;
  for(unsigned i=0;i<20;++i)D_801E8648[i]=(u16)(seed*199+i);
  u16 beforeRecords[20],afterRecords[20];memcpy(beforeRecords,D_801E8648,40);
  D_801E8644=fixture.source;D_801E86B0=(s16)(seed-16);D_801E863C=0xa55a;
  initial=fixture;memset(calls,0,sizeof(calls));count=0;
  PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[4]=(u32)(uintptr_t)obj;cpu.gpr[5]=0x12345678;cpu.gpr[6]=seed;cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;wr(NULL,0x801fefc8,2,stack?0xffff:0);
  if(PcPortMipsRun(&cpu,0x801e5d44,0xfffffffcu,10000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"TIMED COMMAND FAIL oracle command=%u seed=%u %s\n",command,seed,cpu.error);return 1;}
  expected=fixture;memcpy(expectedCalls,calls,sizeof(calls));unsigned expectedCount=count;memcpy(afterRecords,D_801E8648,40);s16 slot=D_801E86B0,mask=D_801E863C;
  fixture=initial;memset(calls,0,sizeof(calls));count=0;memcpy(D_801E8648,beforeRecords,40);D_801E86B0=(s16)(seed-16);D_801E863C=0xa55a;
  PcPort_FieldTimedCommandsWithContext(obj,(void*)0x12345678,seed,(u32)(uintptr_t)obj,stack?0xffff:0);
  if(memcmp(&fixture,&expected,sizeof(fixture))||memcmp(calls,expectedCalls,sizeof(calls))||count!=expectedCount||memcmp(D_801E8648,afterRecords,40)||slot!=D_801E86B0||mask!=D_801E863C){fprintf(stderr,"TIMED COMMAND FAIL command=%u seed=%u stack=%u calls=%u/%u\n",command,seed,stack,count,expectedCount);return 1;}++cases;
 }
 printf("TIMED COMMAND PASS %u complete-memory/global/call comparisons; six controlled callee boundaries\n",cases);
}
