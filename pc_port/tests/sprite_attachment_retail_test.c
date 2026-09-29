#include <stdio.h>
#include <string.h>
#include "common.h"
#include "field/actor.h"
#include "psyq/libgte.h"
#include "battle_mips_adapter.h"
extern void func_801E6E48(u8*,s32,s16*,s32,s32,u8*,u8*) __attribute__((weak));
extern void func_801E6F64(void*);
extern void TimerWorkListSetTaskCallback(void*,void(*)(void*));
extern void (*WorkListTaskGetTaskCallback(void*))(void*);
u8 g_PsxScratchpad[4096];
static u8 ram[0x200000];
static struct {u32 wrapper[160],data[16],package[16],parent[16],position[4];} fixture,initial,expected;
static struct Call {u32 args[5];u8 state[sizeof(fixture)];} calls[4],wantCalls[4];
static unsigned nCalls;
static void record(u32 kind,u32 a,u32 b,u32 c,u32 d){assert(nCalls<4);struct Call*r=&calls[nCalls++];r->args[0]=kind;r->args[1]=a;r->args[2]=b;r->args[3]=c;r->args[4]=d;memcpy(r->state,&fixture,sizeof(fixture));}
u8* func_80023FD8(s32 i,u8*p,s16*v,s32 extra){record(0,i,(u32)(uintptr_t)p,(u32)(uintptr_t)v,extra);return(u8*)fixture.wrapper+128;}
void func_80021FE0(void*p,s16 angle){record(1,(u32)(uintptr_t)p,(s32)angle,0,0);*(u16*)((u8*)p+0x80)=angle;}
void func_800223B0(void*p,s16 angle){record(2,(u32)(uintptr_t)p,(s32)angle,0,0);*(u16*)((u8*)p+0x32)=angle;}
void SpriteSetScale(SpriteData*p,short scale){record(3,(u32)(uintptr_t)p,(s32)scale,0,0);*(u16*)((u8*)p+0x2c)=scale;}
MATRIX* CompMatrix(MATRIX*a,MATRIX*b,MATRIX*c){(void)a;(void)b;assert(0);return c;}
void SetRotMatrix(MATRIX*m){(void)m;assert(0);}
void SetTransMatrix(MATRIX*m){(void)m;assert(0);}
static u8* address(u32 a,unsigned w){if(a>=0x80000000u&&(uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);uintptr_t lo=(uintptr_t)&fixture;if(a>=lo&&(uint64_t)a+w<=lo+sizeof(fixture))return(u8*)(uintptr_t)a;return NULL;}
static int rd(void*u,u32 a,unsigned w,u32*v){(void)u;u8*p=address(a,w);if(!p)return-1;*v=0;for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(8*i);return 0;}
static int wr(void*u,u32 a,unsigned w,u32 v){(void)u;u8*p=address(a,w);if(!p)return-1;for(unsigned i=0;i<w;++i)p[i]=(u8)(v>>(8*i));return 0;}
static int bridge(void*u,PcPortMipsCpu*c,u32 t){(void)u;
 if(t==0x80023fd8)c->gpr[2]=(u32)(uintptr_t)func_80023FD8(c->gpr[4],(u8*)(uintptr_t)c->gpr[5],(s16*)(uintptr_t)c->gpr[6],c->gpr[7]);
 else if(t==0x80021fe0)func_80021FE0((void*)(uintptr_t)c->gpr[4],c->gpr[5]);
 else if(t==0x800223b0)func_800223B0((void*)(uintptr_t)c->gpr[4],c->gpr[5]);
 else if(t==0x80022000)SpriteSetScale((SpriteData*)(uintptr_t)c->gpr[4],c->gpr[5]);
 else return 0;
 return 1;
}
int main(void){
 if(!func_801E6E48){fputs("SPRITE ATTACHMENT FAIL missing native body\n",stderr);return 1;}
 FILE*f=fopen("disc/disc1.bin","rb");assert(f);for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));unsigned cases=0;
 f=fopen("disc/SLUS_006.64","rb");assert(f);assert(!fseek(f,0x8001cd6c-0x8000f800,SEEK_SET));assert(fread(ram+0x1cd6c,1,28,f)==28);assert(!fclose(f));
 for(unsigned flag=0;flag<256;++flag)for(unsigned layout=0;layout<2;++layout)for(unsigned seed=0;seed<16;++seed){
  for(unsigned i=0;i<sizeof(fixture)/4;++i)((u32*)&fixture)[i]=0x12481248u*(i+seed);
  u8*w=(u8*)fixture.wrapper+128;u8*d=(u8*)fixture.data;*(s16*)(w+0xbe)=layout?0x100:-16;*(u32*)(w+8)=0x800bc018u;d[0x13]=flag;d[5]=seed*17;d[0xc]=255-seed*17;
  s32 angle=(s32)(0xa55a0000u+seed*71317u),scale=(s32)(0xffff0000u+seed*13579u);initial=fixture;nCalls=0;memset(calls,0,sizeof(calls));PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[4]=(u32)(uintptr_t)fixture.package;cpu.gpr[5]=seed;cpu.gpr[6]=(u32)(uintptr_t)fixture.position;cpu.gpr[7]=angle;cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;assert(!wr(NULL,0x801ff010,4,scale));assert(!wr(NULL,0x801ff014,4,(u32)(uintptr_t)d));assert(!wr(NULL,0x801ff018,4,(u32)(uintptr_t)fixture.parent));
  if(PcPortMipsRun(&cpu,0x801e6e48,0xfffffffcu,1000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"SPRITE ATTACHMENT FAIL oracle %s\n",cpu.error);return 1;}expected=fixture;memcpy(wantCalls,calls,sizeof(calls));assert(nCalls==4);
  if(flag){assert(*(u32*)((u8*)expected.wrapper+128+8)==0x801e6f64);*(u32*)((u8*)expected.wrapper+128+8)=(u32)(uintptr_t)func_801E6F64;}
  fixture=initial;nCalls=0;memset(calls,0,sizeof(calls));func_801E6E48((u8*)fixture.package,seed,(s16*)fixture.position,angle,scale,d,(u8*)fixture.parent);
  if(nCalls!=4||memcmp(&fixture,&expected,sizeof(fixture))||memcmp(calls,wantCalls,sizeof(calls))){fprintf(stderr,"SPRITE ATTACHMENT FAIL flag=%u layout=%u seed=%u\n",flag,layout,seed);return 1;}++cases;
 }
 printf("SPRITE ATTACHMENT PASS %u memory/call comparisons; real work-list accessors, callback relocation normalized\n",cases);
}
