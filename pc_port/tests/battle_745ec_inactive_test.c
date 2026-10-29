#include <assert.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "battle_mips_adapter.h"
#include "psx_memory.h"
uint8_t g_PsxRam[PSX_RAM_SIZE],g_PsxScratchpad[4096],expected[PSX_RAM_SIZE];static uint32_t fault_addr;static jmp_buf failure;
typedef struct{PcPortMipsCpu *cpu;}BattleEntryCall;
static int rd(void*p,uint32_t a,unsigned w,uint32_t*v){(void)p;if(a==fault_addr)return -1;uint32_t o=a&0x1fffffu;if(o+w>PSX_RAM_SIZE)return -1;*v=0;memcpy(v,g_PsxRam+o,w);return 0;}
static int wr(void*p,uint32_t a,unsigned w,uint32_t v){(void)p;uint32_t o=a&0x1fffffu;if(o+w>PSX_RAM_SIZE)return -1;memcpy(g_PsxRam+o,&v,w);return 0;}
static uint32_t battle_entry_read(BattleEntryCall*e,uint32_t a,unsigned w){uint32_t v;(void)e;if(rd(NULL,a,w,&v))longjmp(failure,1);return v;}
static void battle_entry_write(BattleEntryCall*e,uint32_t a,unsigned w,uint32_t v){(void)e;assert(!wr(NULL,a,w,v));}
static void battle_entry_fail(BattleEntryCall*e,const char*m,uint32_t a){(void)e;(void)m;(void)a;longjmp(failure,1);}
#include "battle_panel_inactive_body.inc"
static void seed(uint8_t *code,unsigned value){memset(g_PsxRam,0,sizeof(g_PsxRam));memcpy(PSX_ADDR(0x800745ecu),code,0x4cc);assert(!wr(NULL,0x800d2d28u,4,0x80150000u));assert(!wr(NULL,0x801500b7u,1,value));}
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);uint8_t code[0x4cc];assert(!fseek(f,0x4afc,SEEK_SET));assert(fread(code,1,sizeof(code),f)==sizeof(code));fclose(f);PcPortMipsBus bus={.read=rd,.write=wr};uint32_t values[]={0,6,255};
for(unsigned k=0;k<3;k++){PcPortMipsCpu c,want;seed(code,values[k]);PcPortMipsCpuInit(&c,&bus);for(unsigned i=1;i<32;i++)c.gpr[i]=0x31400000u+i;c.gpr[29]=0x801ff000;c.gpr[31]=0x8000deacu;c.hi=0x24681357u;c.lo=0xabcdef01u;want=c;assert(PcPortMipsRun(&c,0x800745ecu,0x8000deacu,5000)==PC_PORT_MIPS_HALTED);memcpy(expected,g_PsxRam,sizeof(expected));seed(code,values[k]);BattleEntryCall e={.cpu=&want};battle_panel_inactive_run(&e);assert(!memcmp(expected,g_PsxRam,sizeof(expected)));assert(want.hi==c.hi&&want.lo==c.lo);for(unsigned i=1;i<32;i++)if(want.gpr[i]!=c.gpr[i]){fprintf(stderr,"case %u register %u got %08x expected %08x\n",k,i,want.gpr[i],c.gpr[i]);return 1;}}
uint32_t targets[]={0x800d2d28u,0x801500b7u,0x801feff8u};for(unsigned k=0;k<3;k++){PcPortMipsCpu c,want;fault_addr=0;seed(code,0);PcPortMipsCpuInit(&c,&bus);for(unsigned i=1;i<32;i++)c.gpr[i]=0x31400000u+i;c.gpr[29]=0x801ff000;c.gpr[31]=0x8000deacu;c.hi=0x24681357u;c.lo=0xabcdef01u;want=c;fault_addr=targets[k];assert(PcPortMipsRun(&c,0x800745ecu,0x8000deacu,5000)==PC_PORT_MIPS_FAULT);memcpy(expected,g_PsxRam,sizeof(expected));fault_addr=0;seed(code,0);fault_addr=targets[k];BattleEntryCall e={.cpu=&want};if(!setjmp(failure)){battle_panel_inactive_run(&e);assert(0);}assert(!memcmp(expected,g_PsxRam,sizeof(expected)));for(unsigned i=1;i<32;i++)if(want.gpr[i]!=c.gpr[i]){fprintf(stderr,"fault %u register %u got %08x expected %08x\n",k,i,want.gpr[i],c.gpr[i]);return 1;}}
puts("745EC inactive panel draft PASS (three states, three read faults)");return 0;}
