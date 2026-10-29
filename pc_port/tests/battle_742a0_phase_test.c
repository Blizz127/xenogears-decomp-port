/* Test-only retail instruction oracle, synthetic actor state and spies. */
#include <assert.h>
#include <stdint.h>
#include <setjmp.h>
static uint32_t fault_addr; static jmp_buf fault_jmp;
#include <stdio.h>
#include <string.h>
#include "battle_mips_adapter.h"
#include "psx_memory.h"
uint8_t g_PsxRam[PSX_RAM_SIZE],g_PsxScratchpad[4096];

typedef struct {uint32_t target,args[4];} Call;
static Call calls[4],expected_calls[4];static unsigned count;static uint8_t expected[0x200000];
static uint32_t invoke(uint32_t target,const uint32_t *args){assert(count<4);calls[count].target=target;memcpy(calls[count++].args,args,16);return target==0x80072f38u?0x1234u:0;}
int PcPort_BattleMipsCallJal(uint32_t target,const uint32_t *args,unsigned argc,uint32_t *v0,uint32_t *v1){assert(argc==20);*v0=invoke(target,args);*v1=0;return 0;}
static uint8_t *resolve(uint32_t a,unsigned w){uintptr_t b=(uintptr_t)g_PsxRam;if((uintptr_t)a>=b&&(uintptr_t)a+w<=b+0x200000)return(uint8_t*)(uintptr_t)a;unsigned o=a&0x1fffff;if(o+w>0x200000)return NULL;return g_PsxRam+o;}
static int readmem(void*p,uint32_t a,unsigned w,uint32_t*v){(void)p;if(a==fault_addr)return -1;uint8_t*q=resolve(a,w);if(!q)return -1;*v=0;memcpy(v,q,w);return 0;}
static int writemem(void*p,uint32_t a,unsigned w,uint32_t v){(void)p;uint8_t*q=resolve(a,w);if(!q)return -1;memcpy(q,&v,w);return 0;}
static int bridge(void*p,PcPortMipsCpu*c,uint32_t t){(void)p;if(t==0x80072a9cu || t==0x80072da8u || t==0x80072f38u || t==0x80073380u){c->gpr[2]=invoke(t,&c->gpr[4]);c->gpr[3]=0;return 1;}return 0;}
static void seed(unsigned k,const uint8_t*code){
 memset(g_PsxRam,0,sizeof(g_PsxRam));memcpy(PSX_ADDR(0x800742a0u),code,260);
 uint8_t *record=PSX_ADDR(0x800d5000u);assert((uintptr_t)record<=UINT32_MAX);
 *(uint32_t *)PSX_ADDR(0x800d2d28u)=k==3?(uint32_t)(uintptr_t)record:0x800d5000u;
 record[0x7c]=k?1:0;*(uint32_t *)(record+0xe0)=0x11111111;*(uint32_t *)(record+0xec)=0x22222222;
 *(uint8_t *)PSX_ADDR(0x800c3eb6u)=k==1?0x7f:0;
 *(uint8_t *)PSX_ADDR(0x800d32a1u)=k==3?7:0;
 *(uint8_t *)PSX_ADDR(0x800ccb34u)=5;
}

typedef struct { PcPortMipsCpu *cpu; } BattleEntryCall;
static uint32_t battle_entry_read(BattleEntryCall *e,uint32_t a,unsigned w){uint32_t v;if(readmem(NULL,a,w,&v)) longjmp(fault_jmp,1);return v;}
static void battle_entry_write(BattleEntryCall *e,uint32_t a,unsigned w,uint32_t v){(void)e;assert(writemem(NULL,a,w,v)==0);}
static void battle_phase_call(BattleEntryCall *e,uint32_t target,uint32_t site){PcPortMipsCpu child=*e->cpu;child.gpr[31]=site+8;assert(PcPortMipsRun(&child,target,site+8,5000)==PC_PORT_MIPS_HALTED);memcpy(e->cpu->gpr,child.gpr,sizeof(child.gpr));e->cpu->hi=child.hi;e->cpu->lo=child.lo;}
#include "battle_target_process_body.inc"
int main(int argc,char **argv){FILE *f;uint8_t code[260];assert(argc==2);f=fopen(argv[1],"rb");assert(f);assert(!fseek(f,0x47b0,SEEK_SET));assert(fread(code,1,260,f)==260);fclose(f);
for(unsigned k=0;k<4;k++){PcPortMipsCpu c,want;PcPortMipsBus bus={.read=readmem,.write=writemem,.bridge=bridge};seed(k,code);count=0;PcPortMipsCpuInit(&c,&bus);for(unsigned i=1;i<32;i++)c.gpr[i]=0x31400000u+i;c.gpr[29]=0x801ff000;c.gpr[31]=0x8000deacu;want=c;assert(PcPortMipsRun(&c,0x800742a0u,0x8000deacu,5000)==PC_PORT_MIPS_HALTED);memcpy(expected,g_PsxRam,sizeof(expected));unsigned n=count;memcpy(expected_calls,calls,sizeof(calls));PcPortMipsCpu result=c;seed(k,code);count=0;BattleEntryCall e={.cpu=&want};battle_target_process_run(&e);assert(count==n && !memcmp(calls,expected_calls,n*sizeof(Call)));assert(!memcmp(g_PsxRam,expected,sizeof(expected)));for(unsigned i=1;i<32;i++)if(want.gpr[i]!=result.gpr[i]){fprintf(stderr,"case %u register %u got %08x want %08x\n",k,i,want.gpr[i],result.gpr[i]);return 1;}}
for(unsigned k=0;k<4;k++){PcPortMipsCpu c,want;PcPortMipsBus bus={.read=readmem,.write=writemem,.bridge=bridge};seed(0,code);PcPortMipsCpuInit(&c,&bus);for(unsigned i=1;i<32;i++)c.gpr[i]=0x31400000u+i;c.gpr[29]=0x801ff000;c.gpr[31]=0x8000deacu;want=c;fault_addr=0x801fefe0u+0x1cu-k*4;assert(PcPortMipsRun(&c,0x800742a0u,0x8000deacu,5000)==PC_PORT_MIPS_FAULT);memcpy(expected,g_PsxRam,sizeof(expected));seed(0,code);BattleEntryCall e={.cpu=&want};if(!setjmp(fault_jmp)){battle_target_process_run(&e);assert(0);}assert(!memcmp(g_PsxRam,expected,sizeof(expected)));for(unsigned i=1;i<32;i++)if(want.gpr[i]!=c.gpr[i]){fprintf(stderr,"fault %u register %u got %08x want %08x\n",k,i,want.gpr[i],c.gpr[i]);return 1;}}puts("742A0 phase draft normal and return-fault comparisons PASS (service spies)");return 0;}
