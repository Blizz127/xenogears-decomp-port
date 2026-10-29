#include <assert.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "battle_mips_adapter.h"
#include "psx_memory.h"
uint8_t g_PsxRam[PSX_RAM_SIZE],g_PsxScratchpad[4096],expected[PSX_RAM_SIZE];
static uint32_t read_fault,control_writes;static jmp_buf failure;
typedef struct{PcPortMipsCpu *cpu;}BattleEntryCall;
static int rd(void*p,uint32_t a,unsigned w,uint32_t*v){(void)p;if(a==read_fault)return -1;uint32_t o=a&0x1fffffu;if(o+w>PSX_RAM_SIZE)return -1;*v=0;memcpy(v,g_PsxRam+o,w);return 0;}
static int wr(void*p,uint32_t a,unsigned w,uint32_t v){(void)p;uint32_t o=a&0x1fffffu;if(o+w>PSX_RAM_SIZE)return -1;memcpy(g_PsxRam+o,&v,w);return 0;}
static uint32_t battle_entry_read(BattleEntryCall*e,uint32_t a,unsigned w){uint32_t v;(void)e;if(rd(NULL,a,w,&v))longjmp(failure,1);return v;}
static void battle_entry_fail(BattleEntryCall*e,const char*m,uint32_t a){(void)e;(void)m;(void)a;longjmp(failure,1);}
static void CTC2(uint32_t v,int i){(void)v;(void)i;control_writes++;}
static void copwrite(void*p,int control,unsigned reg,uint32_t v){(void)p;assert(control);CTC2(v,(int)reg);}
#include "battle_gte_matrix_body.inc"
static void seed(uint8_t*rot,uint8_t*trans){memset(g_PsxRam,0,sizeof(g_PsxRam));memcpy(PSX_ADDR(0x80049efcu),rot,48);memcpy(PSX_ADDR(0x80049f8cu),trans,32);for(unsigned i=0;i<32;i++)((uint8_t*)PSX_ADDR(0x80100000u))[i]=(uint8_t)(i*13u);}
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);uint8_t rot[48],trans[32];assert(!fseek(f,0x3a6fc,SEEK_SET));assert(fread(rot,1,48,f)==48);assert(!fseek(f,0x3a78c,SEEK_SET));assert(fread(trans,1,32,f)==32);fclose(f);PcPortMipsBus bus={.read=rd,.write=wr,.cop2_write=copwrite};uint32_t faults[]={0x80100004u,0x80100008u,0x8010000cu,0x80100010u,0x80100018u,0x8010001cu};
for(unsigned k=0;k<6;k++){PcPortMipsCpu c,want;read_fault=0;seed(rot,trans);control_writes=0;PcPortMipsCpuInit(&c,&bus);for(unsigned i=1;i<32;i++)c.gpr[i]=0x31400000u+i;c.gpr[4]=0x80100000u;c.gpr[29]=0x801ff000;c.gpr[31]=0x8000deacu;c.hi=0x24681357u;c.lo=0xabcdef01u;want=c;read_fault=faults[k];uint32_t target=k<4?0x80049efcu:0x80049f8cu;assert(PcPortMipsRun(&c,target,0x8000deacu,1000)==PC_PORT_MIPS_FAULT);assert(control_writes==0);memcpy(expected,g_PsxRam,sizeof(expected));read_fault=0;seed(rot,trans);control_writes=0;read_fault=faults[k];BattleEntryCall e={.cpu=&want};if(!setjmp(failure)){battle_set_matrix_run(&e,target);assert(0);}assert(control_writes==0);assert(!memcmp(expected,g_PsxRam,sizeof(expected)));for(unsigned i=1;i<32;i++)if(want.gpr[i]!=c.gpr[i]){fprintf(stderr,"fault %u register %u original=%08x host=%08x\n",k,i,c.gpr[i],want.gpr[i]);return 1;}assert(want.hi==c.hi&&want.lo==c.lo);}
puts("GTE matrix setters PASS six delayed-load faults");return 0;}
