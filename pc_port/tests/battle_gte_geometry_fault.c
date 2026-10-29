#include <assert.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "battle_mips_adapter.h"
#include "psx_memory.h"
uint8_t g_PsxRam[PSX_RAM_SIZE],g_PsxScratchpad[4096],expected[PSX_RAM_SIZE];
static uint32_t write_fault;static jmp_buf failure;
typedef struct{PcPortMipsCpu *cpu;}BattleEntryCall;
static int rd(void*p,uint32_t a,unsigned w,uint32_t*v){(void)p;uint32_t o=a&0x1fffffu;if(o+w>PSX_RAM_SIZE)return -1;*v=0;memcpy(v,g_PsxRam+o,w);return 0;}
static int wr(void*p,uint32_t a,unsigned w,uint32_t v){(void)p;if(a==write_fault)return -1;uint32_t o=a&0x1fffffu;if(o+w>PSX_RAM_SIZE)return -1;memcpy(g_PsxRam+o,&v,w);return 0;}
static void battle_entry_write(BattleEntryCall*e,uint32_t a,unsigned w,uint32_t v){(void)e;if(wr(NULL,a,w,v))longjmp(failure,1);}
static void battle_entry_fail(BattleEntryCall*e,const char*m,uint32_t a){(void)e;(void)m;(void)a;longjmp(failure,1);}
static uint32_t CFC2(int i){return i==24?0xfffc0000u:0x01230000u;}
static void CTC2(uint32_t v,int i){(void)v;(void)i;assert(0);}
static uint32_t copread(void*p,int control,unsigned reg){(void)p;assert(control);return CFC2((int)reg);}
#include "battle_gte_geometry_body.inc"
static void seed(uint8_t*code){memset(g_PsxRam,0,sizeof(g_PsxRam));memcpy(PSX_ADDR(0x8004a0bcu),code,32);}
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);uint8_t code[32];assert(!fseek(f,0x3a8bc,SEEK_SET));assert(fread(code,1,32,f)==32);fclose(f);PcPortMipsBus bus={.read=rd,.write=wr,.cop2_read=copread};uint32_t faults[]={0x80100000u,0x80110000u};
for(unsigned k=0;k<2;k++){PcPortMipsCpu c,want;write_fault=0;seed(code);PcPortMipsCpuInit(&c,&bus);for(unsigned i=1;i<32;i++)c.gpr[i]=0x31400000u+i;c.gpr[4]=0x80100000u;c.gpr[5]=0x80110000u;c.gpr[29]=0x801ff000;c.gpr[31]=0x8000deacu;c.hi=0x24681357u;c.lo=0xabcdef01u;want=c;write_fault=faults[k];assert(PcPortMipsRun(&c,0x8004a0bcu,0x8000deacu,1000)==PC_PORT_MIPS_FAULT);memcpy(expected,g_PsxRam,sizeof(expected));write_fault=0;seed(code);write_fault=faults[k];BattleEntryCall e={.cpu=&want};if(!setjmp(failure)){battle_geom_control_run(&e,0x8004a0bcu);assert(0);}assert(!memcmp(expected,g_PsxRam,sizeof(expected)));for(unsigned i=1;i<32;i++)if(want.gpr[i]!=c.gpr[i]){fprintf(stderr,"fault %u register %u original=%08x host=%08x\n",k,i,c.gpr[i],want.gpr[i]);return 1;}assert(want.hi==c.hi&&want.lo==c.lo);}
puts("GTE geometry control PASS two write faults");return 0;}
