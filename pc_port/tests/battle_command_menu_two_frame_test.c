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
static unsigned menu_calls;static unsigned desired;static uint8_t table[404];static unsigned calls;static uint32_t trace[32][33],saved_trace[32][33];
static int bridge(void*p,PcPortMipsCpu*c,uint32_t t){(void)p;switch(t){case 0x800716d8u:case 0x8008aa74u:case 0x8007fec4u:case 0x8007ff14u:case 0x80085e78u:case 0x8009ab00u:case 0x80089c08u:case 0x800bc404u:case 0x800bcd98u:case 0x8007fbe0u:case 0x8008d598u:case 0x800816f8u:case 0x80082f7cu:case 0x8008115cu:case 0x80081b58u:case 0x800baf40u:case 0x8007fce8u:case 0x8007fdecu:case 0x800800e8u:case 0x8007fb70u:break;default:return 0;}assert(calls<32);trace[calls][0]=t;memcpy(trace[calls++]+1,c->gpr,sizeof(c->gpr));if(t==0x800716d8u)assert(!wr(NULL,0x80120090u+c->gpr[16],1,1));if(t==0x8008d598u && desired)assert(!wr(NULL,0x801002ddu,1,desired));if(t==0x800816f8u || t==0x80082f7cu || t==0x8008115cu || t==0x80081b58u){if(++menu_calls==2)assert(!wr(NULL,0x801002deu,1,1));}c->gpr[2]=5;return 1;}
static void battle_phase_call(BattleEntryCall *e,uint32_t t,uint32_t site){PcPortMipsCpu c=*e->cpu;c.gpr[31]=site+8;assert(PcPortMipsRun(&c,t,site+8,5000)==PC_PORT_MIPS_HALTED);memcpy(e->cpu->gpr,c.gpr,sizeof(c.gpr));e->cpu->hi=c.hi;e->cpu->lo=c.lo;}
static void battle_entry_fail(BattleEntryCall*e,const char*m,uint32_t a){(void)e;fprintf(stderr,"%s %08x\n",m,a);assert(0);}
#include "battle_command_menu_body.inc"
static void seed(uint8_t *code,unsigned k){menu_calls=0;desired=k==2?1:k==3?100:0;k &= 1;memset(g_PsxRam,0,sizeof(g_PsxRam));memcpy(PSX_ADDR(0x80080160u),code,0x984);memcpy(PSX_ADDR(0x8006fe7cu),table,404);assert(!wr(NULL,0x800c3eacu,4,0x80100000u));assert(!wr(NULL,0x800d2d28u,4,0x80120000u));assert(!wr(NULL,0x800d32a4u+k*8u,1,7+k));assert(!wr(NULL,0x800d32a1u+k*8u,1,k));assert(!wr(NULL,0x800ccd62u+k*0x170u,2,0xaa55));for(unsigned j=0;j<16;j++)assert(!wr(NULL,0x800c3234u+j*2u,2,0xffffu-j));}

int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);uint8_t code[0x984];assert(!fseek(f,0x10670,SEEK_SET));assert(fread(code,1,sizeof(code),f)==sizeof(code));assert(!fseek(f,0x38c,SEEK_SET));assert(fread(table,1,404,f)==404);fclose(f);PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};
for(unsigned k=0;k<2;k++){PcPortMipsCpu c,want;seed(code,k);calls=0;PcPortMipsCpuInit(&c,&bus);for(unsigned i=1;i<32;i++)c.gpr[i]=0x31400000u+i;c.gpr[4]=k&1u;c.gpr[29]=0x801ff000;c.gpr[31]=0x8000deacu;c.hi=0x24681357u;c.lo=0xabcdef01u;want=c;assert(PcPortMipsRun(&c,0x80080160u,0x8000deacu,5000)==PC_PORT_MIPS_HALTED);unsigned n=calls;assert(n==((k&1u)==0?22u:20u));memcpy(expected,g_PsxRam,sizeof(expected));memcpy(saved_trace,trace,sizeof(trace));seed(code,k);calls=0;BattleEntryCall e={.cpu=&want};battle_command_menu_run(&e);assert(calls==n);assert(!memcmp(saved_trace,trace,n*sizeof(trace[0])));assert(!memcmp(expected,g_PsxRam,sizeof(expected)));assert(want.hi==c.hi&&want.lo==c.lo);for(unsigned i=1;i<32;i++)if(want.gpr[i]!=c.gpr[i]){fprintf(stderr,"case %u register %u got %08x expected %08x\n",k,i,want.gpr[i],c.gpr[i]);return 1;}}
puts("80160 menu-entry prefix PASS (two synthetic actors, two simulated menu states, two frames and cleanup)");return 0;}
