#include <stdio.h>
#include <string.h>
#include "common.h"
#include "psyq/libgte.h"
#include "psx/inline_c.h"
#include "psx/gtereg.h"
#include "battle_mips_adapter.h"
extern void func_801E6F64(void*) __attribute__((weak));
u8 g_PsxScratchpad[4096];
static u8 ram[0x200000],guestScratch[4096];
static struct {u32 wrapper[160],object[80],nodes[6][31];} fixture,initial,expected;
static struct Call {u32 kind;u8 a[32],b[32];} calls[8],expectedCalls[8];
static unsigned nCalls;
static struct Call* record(u32 kind){assert(nCalls<8);struct Call*r=&calls[nCalls++];r->kind=kind;return r;}
/* Controlled composition boundary: compare exact input bytes and use real
 * GTE register transfers/commands after composition. Not SDK matrix proof. */
MATRIX* CompMatrix(MATRIX*a,MATRIX*b,MATRIX*out){
    struct Call*r=record(1);memcpy(r->a,a,32);memcpy(r->b,b,32);
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)out->m[i][j]=(s16)((u16)a->m[i][j]+(u16)b->m[j][i]);
    for(unsigned i=0;i<3;++i)out->t[i]=(u32)a->t[i]+(u32)b->t[i];
    return out;
}
void SetRotMatrix(MATRIX*m){memcpy(record(2)->a,m,32);for(unsigned i=0;i<5;++i){u32 v;memcpy(&v,(u8*)m+i*4,4);CTC2(v,i);}}
void SetTransMatrix(MATRIX*m){memcpy(record(3)->a,m,32);for(unsigned i=0;i<3;++i)CTC2(m->t[i],5+i);}
static u8* address(u32 a,unsigned w){
    if(a>=0x80000000u&&(uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);
    if(a>=0x1f800000u&&(uint64_t)a+w<=0x1f801000u)return guestScratch+a-0x1f800000u;
    uintptr_t lo=(uintptr_t)&fixture;if(a>=lo&&(uint64_t)a+w<=lo+sizeof(fixture))return(u8*)(uintptr_t)a;
    return NULL;
}
static int rd(void*u,u32 a,unsigned w,u32*v){(void)u;u8*p=address(a,w);if(!p)return-1;*v=0;for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(i*8);return 0;}
static int wr(void*u,u32 a,unsigned w,u32 v){(void)u;u8*p=address(a,w);if(!p)return-1;for(unsigned i=0;i<w;++i)p[i]=(u8)(v>>(i*8));return 0;}
static void savedCallback(void*wrapper){struct Call*r=record(4);memcpy(r->a,(u8*)wrapper+0x30,32);*(u32*)((u8*)wrapper+0x44)^=0x55aa1234u;}
int PcPort_BattleMipsDispatchCallback(u32 callback,void*argument){if(callback!=0x800bc018u)return 0;savedCallback(argument);return 1;}
static int bridge(void*u,PcPortMipsCpu*c,u32 t){(void)u;
    if(t==0x8004931c){CompMatrix((MATRIX*)address(c->gpr[4],32),(MATRIX*)address(c->gpr[5],32),(MATRIX*)address(c->gpr[6],32));c->gpr[2]=c->gpr[6];}
    else if(t==0x80049efc)SetRotMatrix((MATRIX*)address(c->gpr[4],32));
    else if(t==0x80049f8c)SetTransMatrix((MATRIX*)address(c->gpr[4],32));
    else if(t==(u32)(uintptr_t)savedCallback||t==0x800bc018u)savedCallback((void*)(uintptr_t)c->gpr[4]);
    else return 0;
    return 1;
}
static u32 copRead(void*u,int control,unsigned reg){(void)u;return control?CFC2(reg):MFC2(reg);}
static void copWrite(void*u,int control,unsigned reg,u32 value){(void)u;if(control)CTC2(value,reg);else MTC2(value,reg);}
static int copCommand(void*u,u32 ins){(void)u;doCOP2(ins&0x1ffffffu);return 0;}
int main(void){
 if(!func_801E6F64){fputs("SPRITE ATTACH CALLBACK FAIL missing native body\n",stderr);return 1;}
 FILE*f=fopen("disc/disc1.bin","rb");assert(f);for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));
 unsigned cases=0;
 for(int child=-1;child<4;++child)for(unsigned floor=0;floor<3;++floor)for(unsigned layout=0;layout<2;++layout)for(unsigned guest=0;guest<2;++guest)for(unsigned seed=0;seed<64;++seed){
  for(unsigned i=0;i<sizeof(fixture)/4;++i)((u32*)&fixture)[i]=0x12481248u*(i+seed);
  u8*w=(u8*)fixture.wrapper+128; s16 offset=layout?0x100:-16;u8*recordData=w+offset;
  *(s16*)(w+0xbe)=offset;*(u32*)(recordData+4)=guest?0x800bc018u:(u32)(uintptr_t)savedCallback;*(u32*)(recordData+8)=(u32)(uintptr_t)fixture.object;*(s16*)(recordData+0xc)=child;*(u16*)(recordData+0xe)=floor==0?0:floor==1?1:0xffff;fixture.object[1]=(u32)(uintptr_t)fixture.nodes[1];
  initial=fixture;memset(g_PsxScratchpad,0x36,4096);memcpy(guestScratch,g_PsxScratchpad,4096);memset(&gteRegs,0,sizeof(gteRegs));MTC2(0xfefefefe,0);CTC2(0x9753,31);GTERegisters beforeGte=gteRegs;
  nCalls=0;memset(calls,0,sizeof(calls));PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge,.cop2_read=copRead,.cop2_write=copWrite,.cop2_command=copCommand};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[4]=(u32)(uintptr_t)w;cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
  if(PcPortMipsRun(&cpu,0x801e6f64,0xfffffffcu,1000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"SPRITE ATTACH CALLBACK FAIL oracle %s\n",cpu.error);return 1;}
  expected=fixture;memcpy(expectedCalls,calls,sizeof(calls));unsigned expectedCount=nCalls;GTERegisters expectedGte=gteRegs;fixture=initial;gteRegs=beforeGte;nCalls=0;memset(calls,0,sizeof(calls));func_801E6F64(w);
  if(memcmp(&expected,&fixture,sizeof(fixture))||memcmp(calls,expectedCalls,sizeof(calls))||nCalls!=expectedCount||memcmp(guestScratch,g_PsxScratchpad,4096)||memcmp(&gteRegs,&expectedGte,sizeof(gteRegs))){fprintf(stderr,"SPRITE ATTACH CALLBACK FAIL child=%d floor=%u layout=%u guest=%u seed=%u\n",child,floor,layout,guest,seed);return 1;}++cases;
 }
 printf("SPRITE ATTACH CALLBACK PASS %u memory/scratch/GTE/call comparisons; controlled composition and saved callback\n",cases);
}
