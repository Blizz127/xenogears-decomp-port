#include <stdio.h>
#include <string.h>
#ifndef SPRITE_CONSTRUCTOR_SOURCE
#define SPRITE_CONSTRUCTOR_SOURCE "../src/sprite_constructor.c"
#endif
#include SPRITE_CONSTRUCTOR_SOURCE
#include "battle_mips_adapter.h"
u8 D_800591AD,g_PsxRam[PSX_RAM_SIZE];
s32 D_800591A8;
static u8 ram[0x200000];
static struct {u32 package[8],alternate[8],scripts[32],wrapper[96],parent[64],position[4];} fixture,initial,expected;
static struct Call {u32 kind,args[5];u8 memory[sizeof(fixture)];} calls[5],wantCalls[5];
static unsigned nCalls,typeValue;
static void record(u32 kind,u32 a,u32 b,u32 c,u32 d,u32 e){assert(nCalls<5);struct Call*p=&calls[nCalls++];p->kind=kind;p->args[0]=a;p->args[1]=b;p->args[2]=c;p->args[3]=d;p->args[4]=e;memcpy(p->memory,&fixture,sizeof(fixture));}
s32 func_80023440(void*p){record(0,(u32)(uintptr_t)p,0,0,0,0);return typeValue;}
s32 func_80023468(s32 t){record(1,t,0,0,0,0);return t%3;}
void* func_80023A48(s32 t,s32 m,void*p,s32 extra,void*owner){record(2,t,m,(u32)(uintptr_t)p,extra,(u32)(uintptr_t)owner);write32((u8*)fixture.wrapper+0x5c,(u32)(uintptr_t)fixture.alternate);return fixture.wrapper;}
void func_80023538(void*p,void*s){record(3,(u32)(uintptr_t)p,(u32)(uintptr_t)s,0,0,0);((u8*)p)[0x33]^=0x55;}
void func_80024730(void*p){record(4,(u32)(uintptr_t)p,0,0,0,0);((u8*)p)[0x37]^=0xaa;}
static u8* address(u32 a,unsigned w){if(a==0x800591ad&&w==1)return &D_800591AD;if(a==0x800591a8&&w<=4)return(u8*)&D_800591A8;if(a>=0x80001000u&&(uint64_t)a+w<=0x80001100u)return g_PsxRam+(a&0x1fffff);if(a>=0x80000000u&&(uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);uintptr_t lo=(uintptr_t)&fixture;if(a>=lo&&(uint64_t)a+w<=lo+sizeof(fixture))return(u8*)(uintptr_t)a;return NULL;}
static int rd(void*u,u32 a,unsigned w,u32*v){(void)u;u8*p=address(a,w);if(!p)return-1;*v=0;for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(i*8);return 0;}
static int wr(void*u,u32 a,unsigned w,u32 v){(void)u;u8*p=address(a,w);if(!p)return-1;for(unsigned i=0;i<w;++i)p[i]=(u8)(v>>(i*8));return 0;}
static int bridge(void*u,PcPortMipsCpu*c,u32 t){(void)u;
 if(t==0x80023440)c->gpr[2]=func_80023440((void*)(uintptr_t)c->gpr[4]);
 else if(t==0x80023468)c->gpr[2]=func_80023468(c->gpr[4]);
 else if(t==0x80023a48){u32 owner;assert(!rd(NULL,c->gpr[29]+16,4,&owner));c->gpr[2]=(u32)(uintptr_t)func_80023A48(c->gpr[4],c->gpr[5],(void*)(uintptr_t)c->gpr[6],c->gpr[7],(void*)(uintptr_t)owner);}
 else if(t==0x80023538)func_80023538((void*)(uintptr_t)c->gpr[4],(void*)(uintptr_t)c->gpr[5]);
 else if(t==0x80024730)func_80024730((void*)(uintptr_t)c->gpr[4]);
 else return 0;return 1;
}
int main(void){
 FILE*f=fopen("disc/SLUS_006.64","rb");assert(f);assert(!fseek(f,0x80023fd8-0x8000f800,SEEK_SET));assert(fread(ram+0x23fd8,1,700,f)==700);assert(!fclose(f));unsigned cases=0;const s32 indices[]={-1,0,1,7};
 for(typeValue=0;typeValue<16;++typeValue)for(unsigned parent=0;parent<4;++parent)for(unsigned enabled=0;enabled<2;++enabled)for(unsigned seed=0;seed<8;++seed)for(unsigned idx=0;idx<4;++idx){
  for(unsigned i=0;i<sizeof(fixture)/4;++i)((u32*)&fixture)[i]=0x12481248u*(i+seed);fixture.package[4]=(u32)(uintptr_t)fixture.scripts;
  for(unsigned i=0;i<9;++i)((u16*)fixture.scripts)[i]=32+i*4;
  D_800591AD=enabled?0x80:0;D_800591A8=(s32)(seed*71317u);
  memcpy(g_PsxRam+0x1000,fixture.parent,256);u32 p=parent==0?0:parent==1?(u32)(uintptr_t)fixture.parent:parent==2?(u32)(uintptr_t)((u8*)fixture.wrapper+0x38):0x80001000u;
  write32(PSX_ADDR(0x800c3e1c),p);assert(!wr(NULL,0x800c3e1c,4,p));initial=fixture;memset(calls,0,sizeof(calls));nCalls=0;
  PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[4]=indices[idx];cpu.gpr[5]=(u32)(uintptr_t)fixture.package;cpu.gpr[6]=(u32)(uintptr_t)fixture.position;cpu.gpr[7]=0xa55a0018u+seed;cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
  if(PcPortMipsRun(&cpu,0x80023fd8,0xfffffffcu,1000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"SPRITE CONSTRUCTOR FAIL oracle %s\n",cpu.error);return 1;}expected=fixture;memcpy(wantCalls,calls,sizeof(calls));assert(nCalls==5);fixture=initial;nCalls=0;memset(calls,0,sizeof(calls));u8*got=func_80023FD8(indices[idx],(u8*)fixture.package,(s16*)fixture.position,(s32)(0xa55a0018u+seed));
  if((u32)(uintptr_t)got!=cpu.gpr[2]||nCalls!=5||memcmp(&fixture,&expected,sizeof(fixture))||memcmp(calls,wantCalls,sizeof(calls))){fprintf(stderr,"SPRITE CONSTRUCTOR FAIL type=%u parent=%u enabled=%u seed=%u idx=%u\n",typeValue,parent,enabled,seed,idx);return 1;}++cases;
 }
 printf("SPRITE CONSTRUCTOR PASS %u full-body memory/return/call-snapshot comparisons; controlled callees\n",cases);
}
