#include <assert.h>
#include <stdint.h>
#include <setjmp.h>
static uint32_t fault_addr;static unsigned fail_at,hits;static jmp_buf fault_jmp;
#include <stdio.h>
#include <string.h>
#include "battle_mips_adapter.h"
#include "psx_memory.h"
uint8_t g_PsxRam[PSX_RAM_SIZE],g_PsxScratchpad[4096],expected[PSX_RAM_SIZE];
typedef struct{PcPortMipsCpu *cpu;}BattleEntryCall;
static int rd(void*p,uint32_t a,unsigned w,uint32_t*v){(void)p;if(a==fault_addr && ++hits==fail_at)return -1;uint32_t o=a&0x1fffffu;if(o+w>PSX_RAM_SIZE)return -1;*v=0;memcpy(v,g_PsxRam+o,w);return 0;}
static int wr(void*p,uint32_t a,unsigned w,uint32_t v){(void)p;uint32_t o=a&0x1fffffu;if(o+w>PSX_RAM_SIZE)return -1;memcpy(g_PsxRam+o,&v,w);return 0;}
static uint32_t battle_entry_read(BattleEntryCall*e,uint32_t a,unsigned w){uint32_t v;(void)e;if(rd(NULL,a,w,&v))longjmp(fault_jmp,1);return v;}
static void battle_entry_write(BattleEntryCall*e,uint32_t a,unsigned w,uint32_t v){(void)e;assert(!wr(NULL,a,w,v));}
static unsigned calls;static uint32_t trace[4][33],saved_trace[4][33];
static int bridge(void*p,PcPortMipsCpu*c,uint32_t t){(void)p;if(t!=0x80043b48u)return 0;assert(calls<4);trace[calls][0]=t;memcpy(trace[calls++]+1,c->gpr,sizeof(c->gpr));c->gpr[2]=0x1234;return 1;}
static void battle_phase_call(BattleEntryCall *e,uint32_t t,uint32_t site){PcPortMipsCpu c=*e->cpu;c.gpr[31]=site+8;assert(PcPortMipsRun(&c,t,site+8,5000)==PC_PORT_MIPS_HALTED);memcpy(e->cpu->gpr,c.gpr,sizeof(c.gpr));e->cpu->hi=c.hi;e->cpu->lo=c.lo;}
#include "battle_popup_body.inc"
static void seed(uint8_t *code,unsigned k){memset(g_PsxRam,0,sizeof(g_PsxRam));memcpy(PSX_ADDR(0x80073a58u),code,0x10c);assert(!wr(NULL,0x800c3ea4u,4,0x80100000u));assert(!wr(NULL,0x800ccb04u,4,0x80190000u));assert(!wr(NULL,0x80106415u,1,k!=0));assert(!wr(NULL,0x80106414u,1,k==2?1:0));assert(!wr(NULL,0x80106410u,1,0x70));}
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);uint8_t code[0x10c];assert(!fseek(f,0x3f68,SEEK_SET));assert(fread(code,1,sizeof(code),f)==sizeof(code));fclose(f);PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};
for(unsigned k=0;k<3;k++){PcPortMipsCpu c,want;seed(code,k);calls=0;PcPortMipsCpuInit(&c,&bus);for(unsigned i=1;i<32;i++)c.gpr[i]=0x31400000u+i;c.gpr[29]=0x801ff000;c.gpr[31]=0x8000deacu;c.hi=0x24681357u;c.lo=0xabcdef01u;want=c;assert(PcPortMipsRun(&c,0x80073a58u,0x8000deacu,5000)==PC_PORT_MIPS_HALTED);unsigned n=calls;assert(n==(k==0?0u:2u));memcpy(expected,g_PsxRam,sizeof(expected));memcpy(saved_trace,trace,sizeof(trace));seed(code,k);calls=0;BattleEntryCall e={.cpu=&want};battle_popup_run(&e);assert(calls==n);assert(!memcmp(saved_trace,trace,n*sizeof(trace[0])));assert(!memcmp(expected,g_PsxRam,sizeof(expected)));assert(want.hi==c.hi&&want.lo==c.lo);for(unsigned i=1;i<32;i++)if(want.gpr[i]!=c.gpr[i]){fprintf(stderr,"case %u register %u got %08x expected %08x\n",k,i,want.gpr[i],c.gpr[i]);return 1;}}
uint32_t targets[]={0x800ccb04u,0x80106414u,0x800ccb04u};unsigned occurrences[]={1,4,2};for(unsigned k=0;k<3;k++){PcPortMipsCpu c,want;fault_addr=0;seed(code,1);calls=0;PcPortMipsCpuInit(&c,&bus);for(unsigned i=1;i<32;i++)c.gpr[i]=0x31400000u+i;c.gpr[29]=0x801ff000;c.gpr[31]=0x8000deacu;c.hi=0x24681357u;c.lo=0xabcdef01u;want=c;fault_addr=targets[k];fail_at=occurrences[k];hits=0;assert(PcPortMipsRun(&c,0x80073a58u,0x8000deacu,5000)==PC_PORT_MIPS_FAULT);unsigned n=calls;memcpy(expected,g_PsxRam,sizeof(expected));memcpy(saved_trace,trace,sizeof(trace));fault_addr=0;seed(code,1);calls=0;fault_addr=targets[k];hits=0;BattleEntryCall e={.cpu=&want};if(!setjmp(fault_jmp)){battle_popup_run(&e);assert(0);}assert(calls==n && !memcmp(saved_trace,trace,n*sizeof(trace[0])));assert(!memcmp(expected,g_PsxRam,sizeof(expected)));assert(want.hi==c.hi&&want.lo==c.lo);for(unsigned i=1;i<32;i++)if(want.gpr[i]!=c.gpr[i]){fprintf(stderr,"fault %u register %u got %08x expected %08x\n",k,i,want.gpr[i],c.gpr[i]);return 1;}}puts("73A58 popup normal and delayed-load fault comparisons PASS (AddPrim spy)");return 0;}
