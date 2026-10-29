/* Synthetic CPU comparison for the popup TransMatrix call. */
#include <assert.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>
#include "battle_mips_adapter.h"
#include "psx_memory.h"
uint8_t g_PsxRam[PSX_RAM_SIZE],g_PsxScratchpad[4096],initial[PSX_RAM_SIZE],expected[PSX_RAM_SIZE];
typedef struct {int force_interpret,initialized;PcPortMipsCpu *bridge_cpu;} BattleMipsRuntime;
typedef struct {BattleMipsRuntime *runtime;PcPortMipsCpu *cpu;jmp_buf failure;} BattleEntryCall;
static BattleMipsRuntime *g_ActiveBattleRuntime;
#define CPU_METER_KIND_GENERIC 0
static void cpu_meter_note_kind(int k){(void)k;}
typedef struct {const uint8_t *bytes;size_t size;} XenoRetailFileSlot;
static XenoRetailFileSlot image;
#define XENO_RD_SLUS 1
#define XENO_RD_SLUS_OFF(a) ((size_t)((a)-0x8000f800u))
static const XenoRetailFileSlot *XenoRetailData_File(int id){(void)id;return &image;}
static void file1_digest(const uint8_t*p,unsigned n,uint8_t*d){assert(SHA256(p,n,d));}
static char file1_hex_digit(unsigned n){return "0123456789abcdef"[n&15];}
static uint32_t load_le(const uint8_t*p,unsigned w){uint32_t v=0;memcpy(&v,p,w);return v;}
static uint8_t *resolve_memory(BattleMipsRuntime*r,uint32_t a,unsigned w,int writing)
{
 (void)r;(void)writing;uintptr_t base=(uintptr_t)g_PsxRam;
 if(a>=base && (uint64_t)a+w<=base+PSX_RAM_SIZE)return (uint8_t*)(uintptr_t)a;
 if(a>=0x1f800000u && (uint64_t)a+w<=0x1f800400u)return g_PsxScratchpad+(a-0x1f800000u);
 if(((a&0xffe00000u)==0x80000000u || (a&0xffe00000u)==0xa0000000u) && (a&0x1fffffu)+w<=PSX_RAM_SIZE)return g_PsxRam+(a&0x1fffffu);
 return NULL;
}
static int rd(void *p,uint32_t a,unsigned w,uint32_t*v){uint8_t*q=resolve_memory(p,a,w,0);if(!q)return -1;*v=load_le(q,w);return 0;}
static int wr(void *p,uint32_t a,unsigned w,uint32_t v){uint8_t*q=resolve_memory(p,a,w,1);if(!q)return -1;memcpy(q,&v,w);return 0;}
static void battle_entry_write(BattleEntryCall *e,uint32_t a,unsigned w,uint32_t v){if(wr(e->runtime,a,w,v))longjmp(e->failure,1);}
#include "battle_popup_transmatrix_host.inc"
int main(void)
{
 FILE*f=fopen("disc/SLUS_006.64","rb");assert(f);assert(!fseek(f,0,SEEK_END));long n=ftell(f);assert(n>0);rewind(f);
 uint8_t *bytes=malloc((size_t)n);assert(bytes && fread(bytes,1,(size_t)n,f)==(size_t)n);fclose(f);image.bytes=bytes;image.size=(size_t)n;
 memcpy(PSX_ADDR(0x80049d9cu),bytes+XENO_RD_SLUS_OFF(0x80049d9cu),36u);
 for(unsigned mode=0;mode<4;mode++){
  memset(PSX_ADDR(0x80150000u),0x5a,128u);memset(g_PsxScratchpad,0x33,1024u);
  uint32_t matrix=mode==1?0x1f800100u:0x80150000u;
  uint32_t vector=mode==1?(uint32_t)(uintptr_t)PSX_ADDR(0x80150080u):mode==2?matrix+24u:0x80150080u;
  for(unsigned i=0;i<3;i++)assert(!wr(NULL,vector+4u*i,4,0x10203040u+i));
  BattleMipsRuntime rt={.initialized=1};PcPortMipsBus bus={.read=rd,.write=wr,.opaque=&rt};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
  for(unsigned i=1;i<32;i++)cpu.gpr[i]=0x31400000u+i;cpu.gpr[4]=matrix;cpu.gpr[5]=vector;cpu.gpr[31]=0x800bceccu;
  PcPortMipsCpu host=cpu;rt.bridge_cpu=&host;g_ActiveBattleRuntime=&rt;memcpy(initial,g_PsxRam,sizeof initial);uint8_t pad[4096],wantpad[4096];memcpy(pad,g_PsxScratchpad,sizeof pad);
  assert(PcPortMipsRun(&cpu,0x80049d9cu,0x800bceccu,200)==PC_PORT_MIPS_HALTED);memcpy(expected,g_PsxRam,sizeof expected);memcpy(wantpad,g_PsxScratchpad,sizeof wantpad);
  if(mode==3){memset(initial+0x49d9cu,0,36u);memset(expected+0x49d9cu,0,36u);}
  memcpy(g_PsxRam,initial,sizeof initial);memcpy(g_PsxScratchpad,pad,sizeof pad);
  unsetenv("XENO_DISABLE_CPU_INTERPRETER");unsetenv("XENO_BATTLE_NATIVE_POPUP_TRANS_MATRIX_EXPERIMENT");assert(!popup_transmatrix_try_host(&rt,&host,0x80049d9cu));
  assert(!setenv("XENO_BATTLE_NATIVE_POPUP_TRANS_MATRIX_EXPERIMENT","1",1));
  if(mode==3){assert(!popup_transmatrix_try_host(&rt,&host,0x80049d9cu));assert(!setenv("XENO_DISABLE_CPU_INTERPRETER","1",1));}
  PcPortMipsCpu saved=host;host.gpr[31]^=4u;assert(!popup_transmatrix_try_host(&rt,&host,0x80049d9cu));host=saved;
  host.gpr[5]|=1u;assert(!popup_transmatrix_try_host(&rt,&host,0x80049d9cu));host=saved;
  host.gpr[4]=0x80049d88u;assert(!popup_transmatrix_try_host(&rt,&host,0x80049d9cu));host=saved;
  host.gpr[5]=0x60000000u;assert(!popup_transmatrix_try_host(&rt,&host,0x80049d9cu));host=saved;
  host.load_valid=1;host.load_reg=4;assert(!popup_transmatrix_try_host(&rt,&host,0x80049d9cu));host=saved;
  bytes[XENO_RD_SLUS_OFF(0x80049d9cu)]^=1u;assert(!popup_transmatrix_try_host(&rt,&host,0x80049d9cu));
  ((uint8_t*)PSX_ADDR(0x80049d9cu))[0]^=1u;assert(!popup_transmatrix_try_host(&rt,&host,0x80049d9cu));
  bytes[XENO_RD_SLUS_OFF(0x80049d9cu)]^=1u;((uint8_t*)PSX_ADDR(0x80049d9cu))[0]^=1u;
  assert(!memcmp(&host,&saved,sizeof host));assert(!memcmp(initial,g_PsxRam,sizeof initial));assert(!memcmp(pad,g_PsxScratchpad,sizeof pad));
  assert(popup_transmatrix_try_host(&rt,&host,0x80049d9cu)==1);assert(!memcmp(cpu.gpr,host.gpr,sizeof cpu.gpr) && cpu.hi==host.hi && cpu.lo==host.lo);
  assert(!memcmp(expected,g_PsxRam,sizeof expected) && !memcmp(wantpad,g_PsxScratchpad,sizeof wantpad));
 }
 unsetenv("XENO_DISABLE_CPU_INTERPRETER");free(bytes);puts("TransMatrix popup call PASS CPU comparison, alias and native-only empty-text cases");return 0;
}
