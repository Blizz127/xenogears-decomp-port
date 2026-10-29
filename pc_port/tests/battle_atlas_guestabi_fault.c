#include <assert.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "battle_mips_adapter.h"
#include "psx_memory.h"
uint8_t g_PsxRam[PSX_RAM_SIZE],g_PsxScratchpad[4096],expected[PSX_RAM_SIZE];
static uint32_t read_fault,write_fault;static jmp_buf failure;
typedef struct{PcPortMipsCpu *cpu;}BattleEntryCall;
static int rd(void*p,uint32_t a,unsigned w,uint32_t*v){(void)p;if(a==read_fault)return -1;uint32_t o=a&0x1fffffu;if(o+w>PSX_RAM_SIZE)return -1;*v=0;memcpy(v,g_PsxRam+o,w);return 0;}
static int wr(void*p,uint32_t a,unsigned w,uint32_t v){(void)p;if(a==write_fault)return -1;uint32_t o=a&0x1fffffu;if(o+w>PSX_RAM_SIZE)return -1;memcpy(g_PsxRam+o,&v,w);return 0;}
static uint32_t battle_entry_read(BattleEntryCall*e,uint32_t a,unsigned w){uint32_t v;(void)e;if(rd(NULL,a,w,&v))longjmp(failure,1);return v;}
static void battle_entry_write(BattleEntryCall*e,uint32_t a,unsigned w,uint32_t v){(void)e;if(wr(NULL,a,w,v))longjmp(failure,1);}
static void battle_phase_call(BattleEntryCall*e,uint32_t t,uint32_t site){(void)e;(void)t;(void)site;assert(0);}
#include "battle_atlas_guestabi_body.inc"
static void seed(uint8_t*code){memset(g_PsxRam,0,sizeof(g_PsxRam));memcpy(PSX_ADDR(0x80025fa8u),code,0x390);for(unsigned i=0;i<32;i++)((uint8_t*)PSX_ADDR(0x800188ccu))[i]=(uint8_t)(i+1);for(unsigned i=0;i<5;i++)assert(!wr(NULL,0x801ff010u+4*i,4,100+i));}
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);uint8_t code[0x390];assert(!fseek(f,0x167a8,SEEK_SET));assert(fread(code,1,sizeof(code),f)==sizeof(code));fclose(f);PcPortMipsBus bus={.read=rd,.write=wr};uint32_t faults[]={0x800188d0u,0x800188d4u,0x801ff010u,0x801fef88u,0x801feffcu};
for(unsigned k=0;k<5;k++){PcPortMipsCpu c,want;read_fault=write_fault=0;seed(code);PcPortMipsCpuInit(&c,&bus);for(unsigned i=1;i<32;i++)c.gpr[i]=0x31400000u+i;c.gpr[4]=0x80180000u;c.gpr[5]=0x52;c.gpr[6]=0x80190000u;c.gpr[7]=0;c.gpr[29]=0x801ff000u;c.gpr[31]=0x8000deacu;c.hi=0x24681357u;c.lo=0xabcdef01u;want=c;if(k>=3)write_fault=faults[k];else read_fault=faults[k];assert(PcPortMipsRun(&c,0x80025fa8u,0x8000deacu,5000)==PC_PORT_MIPS_FAULT);memcpy(expected,g_PsxRam,sizeof(expected));read_fault=write_fault=0;seed(code);if(k>=3)write_fault=faults[k];else read_fault=faults[k];BattleEntryCall e={.cpu=&want};if(!setjmp(failure)){battle_atlas_render_run(&e);assert(0);}assert(!memcmp(expected,g_PsxRam,sizeof(expected)));for(unsigned i=1;i<32;i++)if(want.gpr[i]!=c.gpr[i]){fprintf(stderr,"fault %u register %u original=%08x host=%08x\n",k,i,c.gpr[i],want.gpr[i]);return 1;}assert(want.hi==c.hi&&want.lo==c.lo);}
puts("25FA8 draft PASS five selected pre-SDK read/write faults");return 0;}
