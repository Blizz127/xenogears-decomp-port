#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "battle_mips_adapter.h"
#include "psx_memory.h"
uint8_t g_PsxRam[PSX_RAM_SIZE],g_PsxScratchpad[4096],expected[PSX_RAM_SIZE];
typedef struct{PcPortMipsCpu *cpu;}BattleEntryCall;
static int rd(void*p,uint32_t a,unsigned w,uint32_t*v){(void)p;uint32_t o=a&0x1fffffu;if(o+w>PSX_RAM_SIZE)return -1;*v=0;memcpy(v,g_PsxRam+o,w);return 0;}
static int wr(void*p,uint32_t a,unsigned w,uint32_t v){(void)p;uint32_t o=a&0x1fffffu;if(o+w>PSX_RAM_SIZE)return -1;memcpy(g_PsxRam+o,&v,w);return 0;}
static uint32_t battle_entry_read(BattleEntryCall*e,uint32_t a,unsigned w){uint32_t v;(void)e;assert(!rd(NULL,a,w,&v));return v;}
static void battle_entry_write(BattleEntryCall*e,uint32_t a,unsigned w,uint32_t v){(void)e;assert(!wr(NULL,a,w,v));}
#include "battle_digits_body.inc"
static void seed(uint8_t *code){memset(g_PsxRam,0xa5,sizeof(g_PsxRam));memcpy(PSX_ADDR(0x8008aaa0u),code,172);}
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);uint8_t code[172];assert(!fseek(f,0x1afb0,SEEK_SET));assert(fread(code,1,172,f)==172);fclose(f);uint32_t values[]={0u,65535u,0xffffffffu};PcPortMipsBus bus={.read=rd,.write=wr};
for(unsigned k=0;k<3;k++){PcPortMipsCpu c,want;seed(code);PcPortMipsCpuInit(&c,&bus);for(unsigned i=1;i<32;i++)c.gpr[i]=0x31400000u+i;c.gpr[4]=values[k];c.gpr[29]=0x801ff000;c.gpr[31]=0x8000deacu;c.hi=0x24681357u;c.lo=0xabcdef01u;want=c;assert(PcPortMipsRun(&c,0x8008aaa0u,0x8000deacu,5000)==PC_PORT_MIPS_HALTED);memcpy(expected,g_PsxRam,sizeof(expected));seed(code);BattleEntryCall e={.cpu=&want};battle_digits_run(&e);assert(!memcmp(expected,g_PsxRam,sizeof(expected)));assert(want.hi==c.hi&&want.lo==c.lo);for(unsigned i=1;i<32;i++)if(want.gpr[i]!=c.gpr[i]){fprintf(stderr,"case %u register %u got %08x expected %08x\n",k,i,want.gpr[i],c.gpr[i]);return 1;}}
puts("8AAA0 digit host comparison PASS (three values, no service spies)");return 0;}
