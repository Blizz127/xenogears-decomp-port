/* Test-only retail instruction oracle, synthetic actor state and spies. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "battle_mips_adapter.h"
#include "psx_memory.h"
uint8_t g_PsxRam[PSX_RAM_SIZE],g_PsxScratchpad[4096];
void func_80071B94(uint32_t);
typedef struct {uint32_t target,args[4];} Call;
static Call calls[128],expected_calls[128];static unsigned count;static uint8_t expected[0x1fe000];
static uint32_t invoke(uint32_t target,const uint32_t *args){assert(count<128);calls[count].target=target;memcpy(calls[count++].args,args,16);return 0;}
int PcPort_BattleMipsCallJal(uint32_t target,const uint32_t *args,unsigned argc,uint32_t *v0,uint32_t *v1){assert(argc==20);*v0=invoke(target,args);*v1=0;return 0;}
static uint8_t *resolve(uint32_t a,unsigned w){uintptr_t b=(uintptr_t)g_PsxRam;if((uintptr_t)a>=b&&(uintptr_t)a+w<=b+0x200000)return(uint8_t*)(uintptr_t)a;unsigned o=a&0x1fffff;if(o+w>0x200000)return NULL;return g_PsxRam+o;}
static int readmem(void*p,uint32_t a,unsigned w,uint32_t*v){(void)p;uint8_t*q=resolve(a,w);if(!q)return -1;*v=0;memcpy(v,q,w);return 0;}
static int writemem(void*p,uint32_t a,unsigned w,uint32_t v){(void)p;uint8_t*q=resolve(a,w);if(!q)return -1;memcpy(q,&v,w);return 0;}
static int bridge(void*p,PcPortMipsCpu*c,uint32_t t){(void)p;if(t==0x80033728u || t==0x80034eacu || t==0x80070eb0u || t==0x800718bcu || t==0x80071964u || t==0x80071a08u || t==0x80071a38u || t==0x80071a8cu || t==0x80071ae0u || t==0x80072270u || t==0x8007252cu || t==0x800769e8u || t==0x80079778u || t==0x800799c8u || t==0x80079c24u || t==0x80080160u || t==0x80080ae4u || t==0x80080c94u || t==0x800841e0u || t==0x80085310u || t==0x80085350u || t==0x80085b58u || t==0x80089c9cu || t==0x80099890u || t==0x800ba4e0u || t==0x800bcd98u || t==0x800bfe48u){c->gpr[2]=invoke(t,&c->gpr[4]);c->gpr[3]=0;return 1;}return 0;}
static void seed(unsigned k,const uint8_t*code){
 memset(g_PsxRam,0,sizeof(g_PsxRam));memcpy(PSX_ADDR(0x80071b94u),code,1756);
 *(uint32_t *)PSX_ADDR(0x800c3eacu)=0x800d5000u;
 *(uint32_t *)PSX_ADDR(0x800c3ea4u)=0x800d8000u;
 *(uint32_t *)PSX_ADDR(0x800d2d28u)=0x800da000u;
 *(uint8_t *)PSX_ADDR(0x800d52d3u)=k==0?0:k==1?1:4;
}

int main(int argc,char**argv){FILE*f;uint8_t code[1756];assert(argc==2);f=fopen(argv[1],"rb");assert(f);assert(fseek(f,0x20a4,SEEK_SET)==0);assert(fread(code,1,sizeof(code),f)==sizeof(code));fclose(f);
 for(unsigned k=0;k<3;k++){PcPortMipsCpu c;PcPortMipsBus bus={.read=readmem,.write=writemem,.bridge=bridge};seed(k,code);count=0;PcPortMipsCpuInit(&c,&bus);c.gpr[4]=k==2?1u:0u;c.gpr[29]=0x801ff000;c.gpr[31]=0x8000deacu;
  assert(PcPortMipsRun(&c,0x80071b94u,0x8000deacu,50000)==PC_PORT_MIPS_HALTED);unsigned n=count;memcpy(expected,g_PsxRam,sizeof(expected));memcpy(expected_calls,calls,sizeof(calls));
  assert((k==0 && n==0) || (k!=0 && n>1));
  assert(*(uint8_t*)PSX_ADDR(0x800c3e8cu)==1 && *(uint8_t*)PSX_ADDR(0x800d2cafu)==0);
  if(k!=0){assert(calls[0].target==0x80085350u && calls[1].target==0x80071a08u);assert(*(uint8_t*)PSX_ADDR(0x800d52d3u)==(k==1?0:3));}
  {static const uint8_t zero[0x1000];assert(memcmp(g_PsxRam,zero,sizeof(zero))==0);}
  seed(k,code);count=0;func_80071B94(k==2?1u:0u);assert(count==n && memcmp(calls,expected_calls,n*sizeof(Call))==0);assert(memcmp(g_PsxRam,expected,sizeof(expected))==0);
 }
 puts("71B94 actor-action comparison PASS (three synthetic cases; all services spied)");return 0;}
