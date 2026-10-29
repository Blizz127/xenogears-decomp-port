#include <assert.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>
#include "battle_mips_adapter.h"
#include "psx_memory.h"
uint8_t g_PsxRam[PSX_RAM_SIZE],g_PsxScratchpad[4096],snapshot[PSX_RAM_SIZE];
typedef struct {int force_interpret,initialized;PcPortMipsCpu *bridge_cpu;} BattleMipsRuntime;
typedef struct {BattleMipsRuntime *runtime;PcPortMipsCpu *cpu;jmp_buf failure;} BattleEntryCall;
static BattleMipsRuntime *g_ActiveBattleRuntime;
static int runtime_read(void*p,uint32_t a,unsigned w,uint32_t*v){(void)p;uint32_t o=a&0x1fffffu;if(o+w>PSX_RAM_SIZE)return -1;*v=0;memcpy(v,g_PsxRam+o,w);return 0;}
static uint32_t battle_entry_read(BattleEntryCall *e,uint32_t a,unsigned w){uint32_t v;if(runtime_read(e->runtime,a,w,&v))longjmp(e->failure,1);return v;}
static void battle_entry_write(BattleEntryCall *e,uint32_t a,unsigned w,uint32_t v){(void)e;assert((a&0x1fffffu)+w<=PSX_RAM_SIZE);memcpy(PSX_ADDR(a),&v,w);}
static void battle_entry_fail(BattleEntryCall *e,const char*m,uint32_t a){(void)m;(void)a;longjmp(e->failure,1);}
static void file1_digest(const uint8_t*p,unsigned n,uint8_t*d){assert(SHA256(p,n,d));}
static char file1_hex_digit(unsigned n){return "0123456789abcdef"[n&15];}
#define BATTLE_HALT_PC 0xfffffffcu
#define CPU_METER_KIND_GENERIC 0
static void cpu_meter_note_kind(int k){(void)k;}
#include "battle_frame_geometry_host.inc"
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);assert(!fseek(f,0x6ad4,SEEK_SET));assert(fread(PSX_ADDR(0x800765c4u),1,0x14c,f)==0x14c);fclose(f);*(uint32_t*)PSX_ADDR(0x800d2d28u)=0x80100000u;PcPortMipsCpu cpu={0};BattleMipsRuntime rt={.initialized=1,.bridge_cpu=&cpu};g_ActiveBattleRuntime=&rt;cpu.gpr[31]=0x800739c4u;cpu.gpr[29]=0x801ff000u;PcPortMipsCpu before=cpu;
assert(!battle_frame_geometry_try_host(&rt,&cpu,0x800765c4u));assert(!setenv("XENO_BATTLE_NATIVE_FRAME_GEOMETRY_EXPERIMENT","1",1));uint8_t*code=PSX_ADDR(0x800765c4u);code[7]^=1;assert(!battle_frame_geometry_try_host(&rt,&cpu,0x800765c4u));code[7]^=1;cpu.load_valid=1;cpu.load_reg=4;assert(!battle_frame_geometry_try_host(&rt,&cpu,0x800765c4u));cpu=before;cpu.gpr[4]=11;assert(!battle_frame_geometry_try_host(&rt,&cpu,0x800765c4u));cpu=before;cpu.gpr[31]=0x800739c8u;assert(!battle_frame_geometry_try_host(&rt,&cpu,0x800765c4u));cpu=before;assert(battle_frame_geometry_try_host(&rt,&cpu,0x800765c4u)==1);puts("765C4 geometry gate PASS");return 0;}
