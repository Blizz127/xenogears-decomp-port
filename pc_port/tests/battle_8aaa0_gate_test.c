#include <assert.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>
#include "battle_mips_adapter.h"
#include "psx_memory.h"
uint8_t g_PsxRam[PSX_RAM_SIZE],g_PsxScratchpad[4096];
typedef struct {int force_interpret,initialized;PcPortMipsCpu *bridge_cpu;} BattleMipsRuntime;
typedef struct {BattleMipsRuntime *runtime;PcPortMipsCpu *cpu;jmp_buf failure;} BattleEntryCall;
static BattleMipsRuntime *g_ActiveBattleRuntime;
static uint32_t battle_entry_read(BattleEntryCall *e,uint32_t a,unsigned w){(void)e;uint32_t v=0;memcpy(&v,PSX_ADDR(a),w);return v;}
static void battle_entry_write(BattleEntryCall *e,uint32_t a,unsigned w,uint32_t v){(void)e;memcpy(PSX_ADDR(a),&v,w);}
static void file1_digest(const uint8_t*p,unsigned n,uint8_t*d){assert(SHA256(p,n,d));}
static char file1_hex_digit(unsigned n){return "0123456789abcdef"[n&15];}
#define CPU_METER_KIND_GENERIC 0
static void cpu_meter_note_kind(int k){(void)k;}
#include "battle_digits_host.inc"
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);assert(!fseek(f,0x1afb0,SEEK_SET));assert(fread(PSX_ADDR(0x8008aaa0u),1,172,f)==172);fclose(f);PcPortMipsCpu cpu={0};BattleMipsRuntime rt={.initialized=1,.bridge_cpu=&cpu};g_ActiveBattleRuntime=&rt;cpu.gpr[31]=0x80073420u;cpu.gpr[4]=65535;PcPortMipsCpu before=cpu;
assert(!battle_digits_try_host(&rt,&cpu,0x8008aaa0u));assert(!memcmp(&cpu,&before,sizeof(cpu)));
assert(!setenv("XENO_BATTLE_NATIVE_DIGITS_EXPERIMENT","1",1));cpu.gpr[31]++;assert(!battle_digits_try_host(&rt,&cpu,0x8008aaa0u));cpu=before;
uint8_t *code=PSX_ADDR(0x8008aaa0u);code[73]^=1;assert(!battle_digits_try_host(&rt,&cpu,0x8008aaa0u));code[73]^=1;
cpu.load_valid=1;cpu.load_reg=4;assert(!battle_digits_try_host(&rt,&cpu,0x8008aaa0u));cpu=before;
rt.force_interpret=1;assert(!battle_digits_try_host(&rt,&cpu,0x8008aaa0u));rt.force_interpret=0;rt.bridge_cpu=NULL;assert(!battle_digits_try_host(&rt,&cpu,0x8008aaa0u));rt.bridge_cpu=&cpu;assert(!memcmp(&cpu,&before,sizeof(cpu)));
assert(battle_digits_try_host(&rt,&cpu,0x8008aaa0u)==1);assert(*(uint8_t*)PSX_ADDR(0x800c3cf8u)==6 && *(uint8_t*)PSX_ADDR(0x800c3cfcu)==5);puts("8AAA0 identity and context gate PASS (fixture bus)");return 0;}
