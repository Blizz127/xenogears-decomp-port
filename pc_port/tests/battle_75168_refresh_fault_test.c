#include <assert.h>
#include <stdint.h>
#include <setjmp.h>
static uint32_t fault_addr;static jmp_buf fault_jmp;
#include <stdio.h>
#include <string.h>
#include "battle_mips_adapter.h"
#include "psx_memory.h"
uint8_t g_PsxRam[PSX_RAM_SIZE],g_PsxScratchpad[4096],expected[PSX_RAM_SIZE];
typedef struct{PcPortMipsCpu *cpu;}BattleEntryCall;
static int rd(void*p,uint32_t a,unsigned w,uint32_t*v){(void)p;if(a==fault_addr)return -1;uint32_t o=a&0x1fffffu;if(o+w>PSX_RAM_SIZE)return -1;*v=0;memcpy(v,g_PsxRam+o,w);return 0;}
static int wr(void*p,uint32_t a,unsigned w,uint32_t v){(void)p;uint32_t o=a&0x1fffffu;if(o+w>PSX_RAM_SIZE)return -1;memcpy(g_PsxRam+o,&v,w);return 0;}
static uint32_t battle_entry_read(BattleEntryCall*e,uint32_t a,unsigned w){uint32_t v;(void)e;if(rd(NULL,a,w,&v))longjmp(fault_jmp,1);return v;}
static void battle_entry_write(BattleEntryCall*e,uint32_t a,unsigned w,uint32_t v){(void)e;assert(!wr(NULL,a,w,v));}
#include "battle_gauge_refresh_body.inc"
static void seed(uint8_t *code,unsigned k){memset(g_PsxRam,0,sizeof(g_PsxRam));memcpy(PSX_ADDR(0x80075168u),code,0x7d0);assert(!wr(NULL,0x800c3ea4u,4,0x80100000u));assert(!wr(NULL,0x8010853du,1,k));assert(!wr(NULL,0x800ccdc4u,2,25));assert(!wr(NULL,0x800ccdc6u,2,100));}
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);uint8_t code[0x7d0];assert(!fseek(f,0x5678,SEEK_SET));assert(fread(code,1,0x7d0,f)==0x7d0);fclose(f);uint32_t values[]={0u,1u,2u};PcPortMipsBus bus={.read=rd,.write=wr};
for(unsigned k=0;k<3;k++){PcPortMipsCpu c,want;seed(code,k);PcPortMipsCpuInit(&c,&bus);for(unsigned i=1;i<32;i++)c.gpr[i]=0x31400000u+i;c.gpr[4]=values[k];c.gpr[29]=0x801ff000;c.gpr[31]=0x8000deacu;c.hi=0x24681357u;c.lo=0xabcdef01u;want=c;assert(PcPortMipsRun(&c,0x80075168u,0x8000deacu,5000)==PC_PORT_MIPS_HALTED);memcpy(expected,g_PsxRam,sizeof(expected));seed(code,k);BattleEntryCall e={.cpu=&want};battle_gauge_refresh_run(&e);assert(!memcmp(expected,g_PsxRam,sizeof(expected)));assert(want.hi==c.hi&&want.lo==c.lo);for(unsigned i=1;i<32;i++)if(want.gpr[i]!=c.gpr[i]){fprintf(stderr,"case %u register %u got %08x expected %08x\n",k,i,want.gpr[i],c.gpr[i]);return 1;}}
{PcPortMipsCpu c,want;fault_addr=0;seed(code,1);PcPortMipsCpuInit(&c,&bus);for(unsigned i=1;i<32;i++)c.gpr[i]=0x31400000u+i;c.gpr[29]=0x801ff000;c.gpr[31]=0x8000deacu;c.hi=0x24681357u;c.lo=0xabcdef01u;want=c;fault_addr=0x800d32a0u;assert(PcPortMipsRun(&c,0x80075168u,0x8000deacu,5000)==PC_PORT_MIPS_FAULT);memcpy(expected,g_PsxRam,sizeof(expected));fault_addr=0;seed(code,1);fault_addr=0x800d32a0u;BattleEntryCall e={.cpu=&want};if(!setjmp(fault_jmp)){battle_gauge_refresh_run(&e);assert(0);}assert(!memcmp(expected,g_PsxRam,sizeof(expected)));for(unsigned i=1;i<32;i++)if(want.gpr[i]!=c.gpr[i]){fprintf(stderr,"fault register %u got %08x expected %08x\n",i,want.gpr[i],c.gpr[i]);return 1;}}puts("75168 refresh normal and delayed-load fault comparison PASS");return 0;}
