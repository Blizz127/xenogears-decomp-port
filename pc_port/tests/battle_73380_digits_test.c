#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "battle_mips_adapter.h"
#include "psx_memory.h"
uint8_t g_PsxRam[PSX_RAM_SIZE],g_PsxScratchpad[4096],expected[PSX_RAM_SIZE];
typedef struct {PcPortMipsCpu *cpu;} BattleEntryCall;
static int rd(void *p,uint32_t a,unsigned w,uint32_t *v){(void)p;uint32_t o=a&0x1fffffu;if(o+w>PSX_RAM_SIZE)return -1;*v=0;memcpy(v,g_PsxRam+o,w);return 0;}
static int wr(void *p,uint32_t a,unsigned w,uint32_t v){(void)p;uint32_t o=a&0x1fffffu;if(o+w>PSX_RAM_SIZE)return -1;memcpy(g_PsxRam+o,&v,w);return 0;}
static uint32_t battle_entry_read(BattleEntryCall *e,uint32_t a,unsigned w){uint32_t v;(void)e;assert(!rd(0,a,w,&v));return v;}
static void battle_entry_write(BattleEntryCall *e,uint32_t a,unsigned w,uint32_t v){(void)e;assert(!wr(0,a,w,v));}
static void battle_digits_run(BattleEntryCall *);
static int native_digits;
static unsigned calls;static uint32_t trace[8][33],saved_trace[8][33];
static int bridge(void *p,PcPortMipsCpu *c,uint32_t t){(void)p;if(t!=0x8008aaa0u && t!=0x80076a10u)return 0;assert(calls<8);trace[calls][0]=t;memcpy(trace[calls++]+1,c->gpr,sizeof(c->gpr));if(t==0x8008aaa0u){if(!native_digits)return 0; BattleEntryCall e={.cpu=c};battle_digits_run(&e);return 1;}c->gpr[2]=1;return 1;}
static void battle_phase_call(BattleEntryCall *e,uint32_t t,uint32_t site){PcPortMipsCpu c=*e->cpu;c.gpr[31]=site+8;assert(PcPortMipsRun(&c,t,site+8,5000)==PC_PORT_MIPS_HALTED);memcpy(e->cpu->gpr,c.gpr,sizeof(c.gpr));e->cpu->hi=c.hi;e->cpu->lo=c.lo;}
#include "battle_number_body.inc"
#include "battle_digits_body.inc"
static uint8_t digits_code[172];
static void seed(const uint8_t *code,unsigned k){memset(g_PsxRam,0,sizeof(g_PsxRam));memcpy(PSX_ADDR(0x80073380u),code,440);memcpy(PSX_ADDR(0x8008aaa0u),digits_code,172);*(uint32_t*)PSX_ADDR(0x800c3ea4u)=0x800d6000u;*(uint32_t*)PSX_ADDR(0x800d2d28u)=0x800d5000u;*(uint8_t*)PSX_ADDR(0x800de53du)=k?1:0;*(uint16_t*)PSX_ADDR(0x800ccdc4u)=0x120;*(uint8_t*)PSX_ADDR(0x800d32a0u)=23;*(uint16_t*)PSX_ADDR(0x800c3254u)=40;*(uint8_t*)PSX_ADDR(0x800ccb34u)=9;}
int main(int argc,char **argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);uint8_t code[440];assert(!fseek(f,0x3890,SEEK_SET));assert(fread(code,1,440,f)==440);assert(!fseek(f,0x1afb0,SEEK_SET));assert(fread(digits_code,1,172,f)==172);fclose(f);PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};
for(unsigned k=0;k<2;k++){PcPortMipsCpu c,want;native_digits=0;seed(code,k);calls=0;PcPortMipsCpuInit(&c,&bus);for(unsigned i=1;i<32;i++)c.gpr[i]=0x31400000u+i;c.gpr[4]=0;c.gpr[29]=0x801ff000;c.gpr[31]=0x8000deacu;want=c;assert(PcPortMipsRun(&c,0x80073380u,0x8000deacu,5000)==PC_PORT_MIPS_HALTED);unsigned n=calls;assert(n>=2 && n<=5);memcpy(expected,g_PsxRam,sizeof(expected));memcpy(saved_trace,trace,sizeof(trace));native_digits=1;seed(code,k);calls=0;BattleEntryCall entry={.cpu=&want};battle_number_draw_run(&entry);assert(calls==n);assert(!memcmp(saved_trace,trace,n*sizeof(trace[0])));assert(!memcmp(expected,g_PsxRam,sizeof(expected)));for(unsigned i=1;i<32;i++)if(want.gpr[i]!=c.gpr[i]){fprintf(stderr,"case %u register %u got %08x expected %08x\n",k,i,want.gpr[i],c.gpr[i]);return 1;}}
puts("73380 and 8AAA0 composed comparison PASS (two cases, glyph spy)");return 0;}
