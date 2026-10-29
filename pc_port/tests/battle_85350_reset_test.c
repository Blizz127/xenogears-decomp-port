/* Focused R3000 comparison with synthetic guest RAM; no user data. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "battle_mips_adapter.h"
#include "psx_memory.h"
uint8_t g_PsxRam[PSX_RAM_SIZE],g_PsxScratchpad[4096];static uint8_t expected[0x200000];
void func_80085350(void);
static int rd(void*p,uint32_t a,unsigned w,uint32_t*v){(void)p;unsigned o=a&0x1fffff;if(o+w>0x200000)return -1;*v=0;memcpy(v,g_PsxRam+o,w);return 0;}
static int wr(void*p,uint32_t a,unsigned w,uint32_t v){(void)p;unsigned o=a&0x1fffff;if(o+w>0x200000)return -1;memcpy(g_PsxRam+o,&v,w);return 0;}
static void seed(unsigned k,const uint8_t*code){memset(g_PsxRam,k?0xa5:0x5a,0x200000);memcpy(PSX_ADDR(0x80085350u),code,56);}
int main(int argc,char**argv){FILE*f;uint8_t code[56];assert(argc==2);f=fopen(argv[1],"rb");assert(f);assert(fseek(f,0x15860,SEEK_SET)==0);assert(fread(code,1,56,f)==56);fclose(f);
 for(unsigned k=0;k<2;k++){PcPortMipsCpu c;PcPortMipsBus bus={.read=rd,.write=wr};seed(k,code);PcPortMipsCpuInit(&c,&bus);c.gpr[29]=0x801ff000;c.gpr[31]=0x8000deacu;assert(PcPortMipsRun(&c,0x80085350u,0x8000deacu,1000)==PC_PORT_MIPS_HALTED);memcpy(expected,g_PsxRam,sizeof(expected));for(unsigned i=0;i<11;i++){assert(*(uint8_t*)PSX_ADDR(0x800d2d5cu+i)==0xff);assert(*(uint16_t*)PSX_ADDR(0x800d2d70u+2*i)==0);}seed(k,code);func_80085350();assert(memcmp(g_PsxRam,expected,sizeof(expected))==0);}
 puts("85350 target reset comparison PASS (two synthetic patterns)");return 0;}
