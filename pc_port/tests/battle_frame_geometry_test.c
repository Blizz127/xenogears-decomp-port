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
static unsigned calls;static uint32_t trace[8][33],saved_trace[8][33];
static int bridge(void*p,PcPortMipsCpu*c,uint32_t t){(void)p;if(t!=0x80043b48u)return 0;assert(calls<8);trace[calls][0]=t;memcpy(trace[calls++]+1,c->gpr,sizeof(c->gpr));c->gpr[2]=0x1234;return 1;}
static void battle_phase_call(BattleEntryCall *e,uint32_t t,uint32_t site){PcPortMipsCpu c=*e->cpu;c.gpr[31]=site+8;assert(PcPortMipsRun(&c,t,site+8,5000)==PC_PORT_MIPS_HALTED);memcpy(e->cpu->gpr,c.gpr,sizeof(c.gpr));e->cpu->hi=c.hi;e->cpu->lo=c.lo;}
#include "battle_frame_geometry_body.inc"
static void seed(uint8_t *code,unsigned k){memset(g_PsxRam,0,sizeof(g_PsxRam));memcpy(PSX_ADDR(0x800765c4u),code,0x14c);assert(!wr(NULL,0x800d2d28u,4,0x80100000u));assert(!wr(NULL,0x800d3280u,1,0));assert(!wr(NULL,0x800d32a1u+k*8u,1,k!=0));assert(!wr(NULL,0x800d2d24u+k,1,k==2?7:2));assert(!wr(NULL,0x800c3254u+k*2u,2,0x18+k));assert(!wr(NULL,0x80100090u+k,1,3));}

int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);uint8_t code[0x14c];assert(!fseek(f,0x6ad4,SEEK_SET));assert(fread(code,1,sizeof(code),f)==sizeof(code));fclose(f);PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};
for(unsigned k=0;k<3;k++){PcPortMipsCpu c,want;seed(code,k);calls=0;PcPortMipsCpuInit(&c,&bus);for(unsigned i=1;i<32;i++)c.gpr[i]=0x31400000u+i;c.gpr[4]=k;c.gpr[29]=0x801ff000;c.gpr[31]=0x8000deacu;c.hi=0x24681357u;c.lo=0xabcdef01u;want=c;assert(PcPortMipsRun(&c,0x800765c4u,0x8000deacu,5000)==PC_PORT_MIPS_HALTED);unsigned n=calls;assert(n==0);memcpy(expected,g_PsxRam,sizeof(expected));memcpy(saved_trace,trace,sizeof(trace));seed(code,k);calls=0;BattleEntryCall e={.cpu=&want};battle_geometry_run(&e);assert(calls==n);assert(!memcmp(saved_trace,trace,n*sizeof(trace[0])));assert(!memcmp(expected,g_PsxRam,sizeof(expected)));assert(want.hi==c.hi&&want.lo==c.lo);for(unsigned i=1;i<32;i++)if(want.gpr[i]!=c.gpr[i]){fprintf(stderr,"case %u register %u got %08x expected %08x\n",k,i,want.gpr[i],c.gpr[i]);return 1;}}
uint32_t faults[]={0x800d3280u,0x800c3256u,0x80100054u,0x80100091u,0x80100064u};
for(unsigned k=0;k<5;k++){PcPortMipsCpu c,want;read_fault=write_fault=0;seed(code,1);calls=0;PcPortMipsCpuInit(&c,&bus);for(unsigned i=1;i<32;i++)c.gpr[i]=0x31400000u+i;c.gpr[4]=1;c.gpr[29]=0x801ff000;c.gpr[31]=0x8000deacu;c.hi=0x24681357u;c.lo=0xabcdef01u;want=c;if(k==4)write_fault=faults[k];else read_fault=faults[k];assert(PcPortMipsRun(&c,0x800765c4u,0x8000deacu,5000)==PC_PORT_MIPS_FAULT);memcpy(expected,g_PsxRam,sizeof(expected));read_fault=write_fault=0;seed(code,1);if(k==4)write_fault=faults[k];else read_fault=faults[k];BattleEntryCall e={.cpu=&want};if(!setjmp(failure)){battle_geometry_run(&e);assert(0);}assert(!memcmp(expected,g_PsxRam,sizeof(expected)));assert(want.hi==c.hi&&want.lo==c.lo);for(unsigned i=1;i<32;i++)if(want.gpr[i]!=c.gpr[i]){fprintf(stderr,"fault %u register %u got %08x expected %08x\n",k,i,want.gpr[i],c.gpr[i]);return 1;}}
puts("765C4 geometry draft PASS (three mode/actor cases and five fixed read/write faults)");return 0;}
