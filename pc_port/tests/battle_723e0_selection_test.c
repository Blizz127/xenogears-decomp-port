/* Test-only retail instruction oracle, synthetic actor state and spies. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "battle_mips_adapter.h"
#include "psx_memory.h"
uint8_t g_PsxRam[PSX_RAM_SIZE],g_PsxScratchpad[4096];
void func_800723E0(void);
typedef struct {uint32_t target,args[4];} Call;
static Call calls[4],expected_calls[4];static unsigned count;static uint8_t expected[0x1fe000];
static uint32_t invoke(uint32_t target,const uint32_t *args){assert(count<4);calls[count].target=target;memcpy(calls[count++].args,args,16);return 0;}
int PcPort_BattleMipsCallJal(uint32_t target,const uint32_t *args,unsigned argc,uint32_t *v0,uint32_t *v1){assert(argc==20);*v0=invoke(target,args);*v1=0;return 0;}
static uint8_t *resolve(uint32_t a,unsigned w){uintptr_t b=(uintptr_t)g_PsxRam;if((uintptr_t)a>=b&&(uintptr_t)a+w<=b+0x200000)return(uint8_t*)(uintptr_t)a;unsigned o=a&0x1fffff;if(o+w>0x200000)return NULL;return g_PsxRam+o;}
static int readmem(void*p,uint32_t a,unsigned w,uint32_t*v){(void)p;uint8_t*q=resolve(a,w);if(!q)return -1;*v=0;memcpy(v,q,w);return 0;}
static int writemem(void*p,uint32_t a,unsigned w,uint32_t v){(void)p;uint8_t*q=resolve(a,w);if(!q)return -1;memcpy(q,&v,w);return 0;}
static int bridge(void*p,PcPortMipsCpu*c,uint32_t t){(void)p;if(t==0x80071b94u||t==0x80072324u){c->gpr[2]=invoke(t,&c->gpr[4]);c->gpr[3]=0;return 1;}return 0;}
static void seed(unsigned k,const uint8_t*code){
 memset(g_PsxRam,0,sizeof(g_PsxRam));memcpy(PSX_ADDR(0x800723e0u),code,332);
 uint8_t *record=PSX_ADDR(0x800d5000u);assert((uintptr_t)record<=UINT32_MAX);
 *(uint32_t *)PSX_ADDR(0x800c3eacu)=k==1?(uint32_t)(uintptr_t)record:0x800d5000u;
 *(uint16_t *)PSX_ADDR(0x800d39e0u)=k==2?1:0;
 *(uint8_t *)PSX_ADDR(0x800d2dc0u)=k==0?2:0;
 *(uint8_t *)PSX_ADDR(0x800d2dd7u)=k==1?10:0;
 if(k==1){*(uint8_t *)PSX_ADDR(0x800d2de7u)=1;*(uint8_t *)PSX_ADDR(0x800d2de2u)=3;}
}
int main(int argc,char**argv){FILE*f;uint8_t code[332];assert(argc==2);f=fopen(argv[1],"rb");assert(f);assert(fseek(f,0x28f0,SEEK_SET)==0);assert(fread(code,1,sizeof(code),f)==sizeof(code));fclose(f);
 for(unsigned k=0;k<4;k++){PcPortMipsCpu c;PcPortMipsBus bus={.read=readmem,.write=writemem,.bridge=bridge};seed(k,code);count=0;PcPortMipsCpuInit(&c,&bus);c.gpr[29]=0x801ff000;c.gpr[31]=0x8000deacu;
  assert(PcPortMipsRun(&c,0x800723e0u,0x8000deacu,5000)==PC_PORT_MIPS_HALTED);unsigned n=count;memcpy(expected,g_PsxRam,sizeof(expected));memcpy(expected_calls,calls,sizeof(calls));
  assert(n==1 && calls[0].target==(k==2?0x80072324u:0x80071b94u));
  if(k==0)assert(*(uint8_t*)PSX_ADDR(0x800d52d3u)==2 && *(uint8_t*)PSX_ADDR(0x800d2dc0u)==0);
  if(k==1)assert(*(uint8_t*)PSX_ADDR(0x800d52d3u)==4 && *(uint8_t*)PSX_ADDR(0x800d2dd7u)==0);
  seed(k,code);count=0;func_800723E0();assert(count==n && memcmp(calls,expected_calls,n*sizeof(Call))==0);assert(memcmp(g_PsxRam,expected,sizeof(expected))==0);
 }
 puts("723E0 actor-selection comparison PASS (four synthetic cases; service spies)");return 0;}
