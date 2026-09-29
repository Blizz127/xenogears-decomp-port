#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "psyq/libgte.h"
#include "battle_mips_adapter.h"
#include "field_clip_prelude.h"
#pragma weak PcPort_FieldClipPrelude
static u8 ram[0x200000];
static struct {u32 object[80],node[40],scripts[16];} fixture,initial,expected;
static struct Call {u32 kind,args[4],object[80],node[40];} calls[8],expectedCalls[8];
static unsigned count;static s32 distance;
static void record(u32 k,u32 a,u32 b,u32 c,u32 d){assert(count<8);calls[count]=(struct Call){.kind=k,.args={a,b,c,d}};memcpy(calls[count].object,fixture.object,sizeof(fixture.object));memcpy(calls[count++].node,fixture.node,sizeof(fixture.node));}
void HeapChangeCurrentUser(u32 owner,char**data){record(1,owner,(u32)(uintptr_t)data,0,0);}
VECTOR* ApplyMatrix(MATRIX*m,SVECTOR*v,VECTOR*out){record(2,(u32)(uintptr_t)m,(u16)v->vx,(u16)v->vy,(u16)v->vz);out->vx=(s32)((u32)(s32)v->vx*8191u);out->vy=(s32)((u32)(s32)v->vy*31337u);out->vz=(s32)((u32)(s32)v->vz*65537u);return out;}
s32 func_801E6338(u8*object){record(3,(u32)(uintptr_t)object,0,0,0);return distance;}
void func_801E63A8(u8*object){record(4,(u32)(uintptr_t)object,0,0,0);*(u16*)(object+0x88)=0xabcd;}
static u8* address(u32 a,unsigned w){if(a>=0x80000000u&&(uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);uintptr_t lo=(uintptr_t)&fixture;if(a>=lo&&(uint64_t)a+w<=lo+sizeof(fixture))return(u8*)(uintptr_t)a;return NULL;}
static int rd(void*u,u32 a,unsigned w,u32*v){(void)u;u8*p=address(a,w);if(!p)return-1;*v=0;for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(8*i);return 0;}
static int wr(void*u,u32 a,unsigned w,u32 v){(void)u;u8*p=address(a,w);if(!p)return-1;for(unsigned i=0;i<w;++i)p[i]=(u8)(v>>(8*i));return 0;}
static int bridge(void*u,PcPortMipsCpu*c,u32 t){(void)u;switch(t){
 case 0x80032498:HeapChangeCurrentUser(c->gpr[4],(void*)(uintptr_t)c->gpr[5]);return 1;
 case 0x80049cec:ApplyMatrix((MATRIX*)address(c->gpr[4],32),(SVECTOR*)address(c->gpr[5],8),(VECTOR*)address(c->gpr[6],16));return 1;
 case 0x801e6338:c->gpr[2]=func_801E6338((void*)(uintptr_t)c->gpr[4]);return 1;
 case 0x801e63a8:func_801E63A8((void*)(uintptr_t)c->gpr[4]);return 1;
 default:return t>=0x801e39f0u&&t<0x801e59d4u?0:-1;}}
int main(void){
 if(!PcPort_FieldClipPrelude){fputs("CLIP PRELUDE FAIL missing native body\n",stderr);return 1;}
 FILE*f=fopen("disc/disc1.bin","rb");assert(f);for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));
 const s32 ticks[]={-2,-1,0,1,2,3};unsigned cases=0;
 for(unsigned flags=0;flags<16;++flags)for(unsigned seed=0;seed<32;++seed)for(unsigned tick=0;tick<6;++tick)for(unsigned empty=0;empty<2;++empty){
  for(unsigned i=0;i<sizeof(fixture)/4;++i)((u32*)&fixture)[i]=0x80001234u+seed*0x1001001u+i*8191u;
  u8*obj=(u8*)fixture.object;u8*node=(u8*)fixture.node;u32 script=(u32)(uintptr_t)fixture.scripts;
  *(u32*)(obj+4)=(u32)(uintptr_t)node;*(u32*)(obj+0x10)=empty?0:script;
  *(u32*)(obj+0x4c)=flags&1?script+4:0;*(u32*)(obj+0x54)=flags&2?script+8:0;*(u32*)(obj+0x50)=flags&4?script+12:0;
  *(s16*)(obj+0x58)=flags&8?-1:0;*(s16*)(obj+0x48)=(s16)(seed%4-1);distance=seed%4;
  *(s16*)(obj+0x60)=seed&1?-1:1;*(s32*)(node+0x60)=seed&2?-2:2;
  *(u16*)(obj+0x44)=(u16)(seed*8191);*(u16*)(obj+0x46)=seed&16?(u16)(seed*8191+2):(u16)((seed%4)*0x5555);
  initial=fixture;count=0;memset(calls,0,sizeof(calls));
  PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[4]=(u32)(uintptr_t)obj;cpu.gpr[7]=ticks[tick];cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
  unsigned active=!empty&&ticks[tick]!=0;u32 halt=active?0x801e3d34:0x801e59a0;
  if(PcPortMipsRun(&cpu,0x801e39f0,halt,5000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"CLIP PRELUDE FAIL oracle %s\n",cpu.error);return 1;}
  expected=fixture;memcpy(expectedCalls,calls,sizeof(calls));unsigned expectedCount=count;u32 expectedStream=active?cpu.gpr[19]:0xdeadbeef;
  fixture=initial;count=0;memset(calls,0,sizeof(calls));u32 stream=0xdeadbeef;
  int result=PcPort_FieldClipPrelude(obj,ticks[tick],&stream);
  if(result!=(int)active||stream!=expectedStream||memcmp(&fixture,&expected,sizeof(fixture))||count!=expectedCount||memcmp(calls,expectedCalls,sizeof(calls))){fprintf(stderr,"CLIP PRELUDE FAIL flags=%u seed=%u ticks=%d empty=%u calls=%u/%u\n",flags,seed,ticks[tick],empty,count,expectedCount);return 1;}++cases;
 }
 printf("CLIP PRELUDE PASS %u full-memory/call/selected-stream comparisons; controlled matrix, distance, target and heap boundaries\n",cases);
}
