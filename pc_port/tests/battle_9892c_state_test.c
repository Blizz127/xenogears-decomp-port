/* Test-only retail instruction oracle; synthetic game state, no user saves. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "battle_mips_adapter.h"
#include "psx_memory.h"
#define GS_ADDR 0x8006d634u
#define GS_SIZE 0x2358u
uint8_t g_PsxRam[PSX_RAM_SIZE],g_PsxScratchpad[4096];
static uint8_t gs_storage[GS_SIZE+16], expected_gs[GS_SIZE+16], expected_ram[0x200000];
void func_8009892C(void);
uint8_t *PcPort_BattleSharedData(void *context,uint32_t address,unsigned width) {
 (void)context;if(address>=GS_ADDR && address-GS_ADDR+width<=GS_SIZE)return gs_storage+8+(address-GS_ADDR);return NULL;
}
static uint8_t *resolve(uint32_t addr,unsigned width){
 uint8_t *p=PcPort_BattleSharedData(NULL,addr,width);if(p)return p;
 uintptr_t base=(uintptr_t)g_PsxRam;
 if ((uintptr_t)addr>=base && (uintptr_t)addr+width<=base+0x200000u) return (uint8_t *)(uintptr_t)addr;
 unsigned off=addr&0x1fffff;if(off+width>0x200000)return NULL;return g_PsxRam+off;
}
static int readmem(void*p,uint32_t a,unsigned w,uint32_t*v){(void)p;uint8_t*q=resolve(a,w);if(!q)return -1;*v=0;memcpy(v,q,w);return 0;}
static int writemem(void*p,uint32_t a,unsigned w,uint32_t v){(void)p;uint8_t*q=resolve(a,w);if(!q)return -1;memcpy(q,&v,w);return 0;}
static void set16(uint8_t*p,unsigned off,uint16_t v){memcpy(p+off,&v,2);}
static void set32(uint8_t*p,unsigned off,uint32_t v){memcpy(p+off,&v,4);}
static void seed(unsigned c,const uint8_t*code){
 memset(g_PsxRam,0,sizeof(g_PsxRam));memset(gs_storage,0x5a,sizeof(gs_storage));memcpy(PSX_ADDR(0x8009892c),code,460);
 uint8_t *slots=PSX_ADDR(0x800d2d24),*changes=PSX_ADDR(0x800d2d10),*base=PSX_ADDR(0x800d5000),*gs=gs_storage+8;
 assert((uintptr_t)base<=UINT32_MAX);
 set32(PSX_ADDR(0x800c34b0),0,c==3 ? (uint32_t)(uintptr_t)base : 0x800d5000);slots[0]=c==0?0x7f:1;slots[1]=c==1?2:0x7f;slots[2]=c==1?3:0x7f;
 for(unsigned j=0;j<4;j++) changes[j]=c?j+1:0;
 for(unsigned i=0;i<3;i++){uint8_t*r=base+i*0x170;set16(r,0x7a,0x220+i);r[0xa4+0x98]=12+i;for(unsigned j=0;j<4;j++)r[0xa4+0x50+j]=j+4;}
 set16(gs,0x17da,c?0x2000:0);set16(gs,0x17c2,0x10);set16(gs,0x1930,c==2?0xbb:0xba);
}
int main(int argc,char **argv){
 FILE *f;uint8_t code[460];assert(argc==2);f=fopen(argv[1],"rb");assert(f);assert(fseek(f,0x28e3c,SEEK_SET)==0);assert(fread(code,1,sizeof(code),f)==sizeof(code));fclose(f);
 for(unsigned c=0;c<4;c++){
  PcPortMipsCpu cpu;PcPortMipsBus bus={.read=readmem,.write=writemem};seed(c,code);PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0x8000deacu;
  assert(PcPortMipsRun(&cpu,0x8009892cu,0x8000deacu,5000)==PC_PORT_MIPS_HALTED);
  memcpy(expected_ram,g_PsxRam,sizeof(expected_ram));memcpy(expected_gs,gs_storage,sizeof(expected_gs));
  seed(c,code);func_8009892C();assert(memcmp(g_PsxRam,expected_ram,sizeof(expected_ram))==0);assert(memcmp(gs_storage,expected_gs,sizeof(expected_gs))==0);
  assert(gs_storage[8+0x7e6]==7);assert(gs_storage[0]==0x5a && gs_storage[GS_SIZE+15]==0x5a);
 }
 puts("9892C synthetic game-state comparison PASS (four cases, including host pointer; no real saves)");return 0;
}
