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
static void battle_entry_fail(BattleEntryCall*e,const char*m,uint32_t a){(void)e;(void)m;(void)a;assert(0);}
static uint32_t trace[32],saved[32];static unsigned calls;
static int bridge(void*p,PcPortMipsCpu*c,uint32_t t){(void)p;if(t!=0x8003b644u)return 0;calls++;memcpy(trace,c->gpr,sizeof trace);c->gpr[2]=0x1234;c->gpr[3]=0x5678;return 1;}
static void battle_phase_call(BattleEntryCall*e,uint32_t t,uint32_t site){PcPortMipsCpu c=*e->cpu;c.gpr[31]=site+8;assert(PcPortMipsRun(&c,t,site+8,1000)==PC_PORT_MIPS_HALTED);memcpy(e->cpu->gpr,c.gpr,sizeof c.gpr);e->cpu->hi=c.hi;e->cpu->lo=c.lo;}
#include "battle_sound_enabled_body.inc"
int main(void){FILE*f=fopen("disc/SLUS_006.64","rb");assert(f);assert(!fseek(f,0x2a5b8,SEEK_SET));assert(fread(PSX_ADDR(0x80039db8u),1,96,f)==96);fclose(f);PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};
for(unsigned k=0;k<3;k++){wr(NULL,0x8005957cu,2,0x800u);wr(NULL,0x80059478u,4,k==0?0:k==1?7:0x1234abcdu);memcpy(initial,g_PsxRam,sizeof initial);PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);for(unsigned i=1;i<32;i++)cpu.gpr[i]=0x31400000u+i;cpu.gpr[29]=0x801ff000u;cpu.gpr[31]=0x8000deacu;PcPortMipsCpu host=cpu;calls=0;assert(PcPortMipsRun(&cpu,0x80039db8u,0x8000deacu,1000)==PC_PORT_MIPS_HALTED);assert(calls==1);memcpy(saved,trace,sizeof trace);memcpy(expected,g_PsxRam,sizeof expected);memcpy(g_PsxRam,initial,sizeof initial);calls=0;BattleEntryCall entry={.cpu=&host};battle_sound_enabled_run(&entry);assert(calls==1);for(unsigned i=0;i<32;i++)if(saved[i]!=trace[i])fprintf(stderr,"call case %u r%u cpu=%08x host=%08x\n",k,i,saved[i],trace[i]);assert(!memcmp(saved,trace,sizeof trace));for(unsigned i=1;i<32;i++)if(cpu.gpr[i]!=host.gpr[i]){fprintf(stderr,"case %u r%u cpu %08x host %08x\n",k,i,cpu.gpr[i],host.gpr[i]);return 1;}assert(cpu.hi==host.hi&&cpu.lo==host.lo&&!memcmp(expected,g_PsxRam,sizeof expected));}puts("enabled sound wrapper PASS three cases, audio service spy");return 0;}
