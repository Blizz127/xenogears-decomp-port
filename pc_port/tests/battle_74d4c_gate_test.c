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
#define CPU_METER_KIND_GENERIC 0
static void cpu_meter_note_kind(int k){(void)k;}
#include "battle_indicator_inactive_host.inc"
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);assert(!fseek(f,0x525c,SEEK_SET));assert(fread(PSX_ADDR(0x80074d4cu),1,0x1a0,f)==0x1a0);fclose(f);*(uint32_t*)PSX_ADDR(0x800d2d28u)=0x80150000u;PcPortMipsCpu cpu={0};BattleMipsRuntime rt={.initialized=1,.bridge_cpu=&cpu};g_ActiveBattleRuntime=&rt;cpu.gpr[31]=0x8007648cu;cpu.gpr[29]=0x801ff000u;PcPortMipsCpu before=cpu;
assert(!battle_indicator_inactive_try_host(&rt,&cpu,0x80074d4cu));assert(!setenv("XENO_BATTLE_NATIVE_INACTIVE_INDICATOR_EXPERIMENT","1",1));for(unsigned state=1;state<=5;state++){*(uint8_t*)PSX_ADDR(0x801500c6u)=state;memcpy(snapshot,g_PsxRam,sizeof(snapshot));assert(!battle_indicator_inactive_try_host(&rt,&cpu,0x80074d4cu));assert(!memcmp(&cpu,&before,sizeof(cpu)));assert(!memcmp(snapshot,g_PsxRam,sizeof(snapshot)));}
*(uint8_t*)PSX_ADDR(0x801500c6u)=0;uint8_t*code=PSX_ADDR(0x80074d4cu);code[73]^=1;assert(!battle_indicator_inactive_try_host(&rt,&cpu,0x80074d4cu));code[73]^=1;cpu.load_valid=1;cpu.load_reg=4;assert(!battle_indicator_inactive_try_host(&rt,&cpu,0x80074d4cu));cpu=before;assert(battle_indicator_inactive_try_host(&rt,&cpu,0x80074d4cu)==1);puts("74D4C inactive-only gate PASS (active states decline)");return 0;}
