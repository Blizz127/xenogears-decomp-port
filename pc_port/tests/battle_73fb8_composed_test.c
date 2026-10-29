#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
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
#include "addprim-body.c"
static void battle_optional_strips_link(BattleEntryCall *);
static uint8_t strip[128],add[60];static int native;static unsigned calls;static uint32_t trace[128][33],saved_trace[128][33];
static int bridge(void*p,PcPortMipsCpu*c,uint32_t t){(void)p;if(t!=0x80043b48u && t!=0x800728b8u)return 0;assert(calls<128);trace[calls][0]=t;memcpy(trace[calls++]+1,c->gpr,sizeof(c->gpr));if(!native)return 0;BattleEntryCall e={.cpu=c};if(t==0x80043b48u)battle_fade_draw_sdk_run(&e,t);else battle_optional_strips_link(&e);return 1;}
static void battle_phase_call(BattleEntryCall *e,uint32_t t,uint32_t site){PcPortMipsCpu c=*e->cpu;c.gpr[31]=site+8;assert(PcPortMipsRun(&c,t,site+8,5000)==PC_PORT_MIPS_HALTED);memcpy(e->cpu->gpr,c.gpr,sizeof(c.gpr));e->cpu->hi=c.hi;e->cpu->lo=c.lo;}
#include "link.c"
#include "body.c"
static void seed(uint8_t *code,unsigned k){memset(g_PsxRam,0,sizeof(g_PsxRam));memcpy(PSX_ADDR(0x80073fb8u),code,744);memcpy(PSX_ADDR(0x800728b8u),strip,128);memcpy(PSX_ADDR(0x80043b48u),add,60);assert(!wr(NULL,0x80190004u,4,0x00ffffffu));assert(!wr(NULL,0x800d2d28u,4,0x80100000u));assert(!wr(NULL,0x800c3ea4u,4,0x80120000u));assert(!wr(NULL,0x800ccb04u,4,0x80190000u));assert(!wr(NULL,0x800ccb34u,4,0));assert(!wr(NULL,0x800c3e4cu,1,k==2));if(k){assert(!wr(NULL,0x8010007fu,1,1));assert(!wr(NULL,0x80100084u,1,1));assert(!wr(NULL,0x801000b0u,1,1));assert(!wr(NULL,0x800d2e38u,4,0x80140000u));assert(!wr(NULL,0x801405a0u,4,1));assert(!wr(NULL,0x801405a4u,1,1));}}

int main(int argc,char**argv){assert(argc==3);FILE*f=fopen(argv[1],"rb");assert(f);uint8_t code[744];assert(!fseek(f,0x44c8,SEEK_SET));assert(fread(code,1,sizeof(code),f)==sizeof(code));assert(!fseek(f,0x2dc8,SEEK_SET));assert(fread(strip,1,128,f)==128);fclose(f);f=fopen(argv[2],"rb");assert(f);assert(!fseek(f,0x34348,SEEK_SET));assert(fread(add,1,60,f)==60);fclose(f);PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};
for(unsigned k=0;k<3;k++){PcPortMipsCpu c,want;seed(code,k);calls=0;native=0;PcPortMipsCpuInit(&c,&bus);for(unsigned i=1;i<32;i++)c.gpr[i]=0x31400000u+i;c.gpr[29]=0x801ff000;c.gpr[31]=0x8000deacu;c.hi=0x24681357u;c.lo=0xabcdef01u;want=c;assert(PcPortMipsRun(&c,0x80073fb8u,0x8000deacu,5000)==PC_PORT_MIPS_HALTED);unsigned n=calls;assert(n>=2);memcpy(expected,g_PsxRam,sizeof(expected));memcpy(saved_trace,trace,sizeof(trace));seed(code,k);calls=0;native=1;BattleEntryCall e={.cpu=&want};battle_status_packets_run(&e);assert(calls==n);assert(!memcmp(saved_trace,trace,n*sizeof(trace[0])));assert(!memcmp(expected,g_PsxRam,sizeof(expected)));assert(want.hi==c.hi&&want.lo==c.lo);for(unsigned i=1;i<32;i++)if(want.gpr[i]!=c.gpr[i]){fprintf(stderr,"case %u register %u got %08x expected %08x\n",k,i,want.gpr[i],c.gpr[i]);return 1;}}
puts("73FB8 existing host comparison PASS (three cases, host and original child bodies)");return 0;}
