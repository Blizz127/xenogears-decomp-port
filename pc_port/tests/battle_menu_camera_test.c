#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "battle_mips_adapter.h"
#include "psx_memory.h"
uint8_t g_PsxRam[PSX_RAM_SIZE],g_PsxScratchpad[4096],initial[PSX_RAM_SIZE],expected[PSX_RAM_SIZE];
typedef struct{PcPortMipsCpu *cpu;} BattleEntryCall;
static int rd(void*p,uint32_t a,unsigned w,uint32_t*v){(void)p;*v=0;memcpy(v,PSX_ADDR(a),w);return 0;}
static int wr(void*p,uint32_t a,unsigned w,uint32_t v){(void)p;memcpy(PSX_ADDR(a),&v,w);return 0;}
static uint32_t battle_entry_read(BattleEntryCall*e,uint32_t a,unsigned w){uint32_t v;(void)e;assert(!rd(NULL,a,w,&v));return v;}
static void battle_entry_write(BattleEntryCall*e,uint32_t a,unsigned w,uint32_t v){(void)e;assert(!wr(NULL,a,w,v));}
static uint32_t trace[2][33],saved[2][33];static unsigned calls;
static int bridge(void*p,PcPortMipsCpu*c,uint32_t t){(void)p;if(t!=0x800bc2f0u&&t!=0x800bc460u)return 0;assert(calls<2);trace[calls][0]=t;memcpy(trace[calls++]+1,c->gpr,sizeof c->gpr);c->gpr[2]=0x1234;c->gpr[3]=0x5678;return 1;}
static void battle_target_camera_call(BattleEntryCall*e,uint32_t t,uint32_t site){PcPortMipsCpu c=*e->cpu;c.gpr[31]=site+8;assert(PcPortMipsRun(&c,t,site+8,1000)==PC_PORT_MIPS_HALTED);memcpy(e->cpu->gpr,c.gpr,sizeof c.gpr);e->cpu->hi=c.hi;e->cpu->lo=c.lo;}
#include "body.inc"
int main(void){FILE*f=fopen("disc/battle.bin","rb");assert(f);assert(!fseek(f,0x4c914,SEEK_SET));assert(fread(PSX_ADDR(0x800bc404u),1,80,f)==80);fclose(f);PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};
for(unsigned k=0;k<2;k++){wr(NULL,0x800c37c8u,1,k);wr(NULL,0x800c3cdcu,2,0xa55a);memcpy(initial,g_PsxRam,sizeof initial);PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);for(unsigned i=1;i<32;i++)cpu.gpr[i]=0x31400000u+i;cpu.gpr[4]=0x1234u;cpu.gpr[29]=0x801ff000u;cpu.gpr[31]=0x80080644u;PcPortMipsCpu host=cpu;calls=0;assert(PcPortMipsRun(&cpu,0x800bc404u,0x80080644u,1000)==PC_PORT_MIPS_HALTED);unsigned n=calls;assert(n==(k?0u:2u));memcpy(saved,trace,sizeof trace);memcpy(expected,g_PsxRam,sizeof expected);memcpy(g_PsxRam,initial,sizeof initial);calls=0;BattleEntryCall entry={.cpu=&host};battle_target_camera_wrapper(&entry);assert(calls==n&&!memcmp(saved,trace,n*sizeof trace[0]));for(unsigned i=1;i<32;i++)if(cpu.gpr[i]!=host.gpr[i]){fprintf(stderr,"case %u r%u cpu %08x host %08x\n",k,i,cpu.gpr[i],host.gpr[i]);return 1;}assert(cpu.hi==host.hi&&cpu.lo==host.lo&&!memcmp(expected,g_PsxRam,sizeof expected));}puts("camera wrapper PASS two fixed branches, child services simulated");return 0;}
