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
typedef struct {const uint8_t *bytes;size_t size;} XenoRetailFileSlot;
static XenoRetailFileSlot image;
#define XENO_RD_SLUS 1
#define XENO_RD_SLUS_OFF(a) ((size_t)((a)-0x8000f800u))
static const XenoRetailFileSlot *XenoRetailData_File(int id){(void)id;return &image;}
static void battle_phase_call(BattleEntryCall*e,uint32_t t,uint32_t site){(void)t;(void)site;longjmp(e->failure,1);}
#include "battle_atlas_guestabi_host.inc"
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);assert(!fseek(f,0,SEEK_END));long size=ftell(f);assert(size>0);rewind(f);uint8_t*bytes=malloc((size_t)size);assert(bytes&&fread(bytes,1,(size_t)size,f)==(size_t)size);fclose(f);image.bytes=bytes;image.size=(size_t)size;PcPortMipsCpu cpu={0};BattleMipsRuntime rt={.initialized=1,.bridge_cpu=&cpu};g_ActiveBattleRuntime=&rt;cpu.gpr[31]=0x80076814u;cpu.gpr[29]=0x801ff000u;PcPortMipsCpu before=cpu;
assert(!battle_atlas_render_try_host(&rt,&cpu,0x80025fa8u));assert(!setenv("XENO_BATTLE_NATIVE_ATLAS_GUESTABI_EXPERIMENT","1",1));size_t off=XENO_RD_SLUS_OFF(0x80025fa8u);bytes[off+7]^=1;assert(!battle_atlas_render_try_host(&rt,&cpu,0x80025fa8u));bytes[off+7]^=1;cpu.load_valid=1;cpu.load_reg=4;assert(!battle_atlas_render_try_host(&rt,&cpu,0x80025fa8u));cpu=before;cpu.gpr[31]=0x80076818u;assert(!battle_atlas_render_try_host(&rt,&cpu,0x80025fa8u));cpu=before;assert(battle_atlas_render_try_host(&rt,&cpu,0x80025fa8u)==-1);cpu=before;cpu.gpr[31]=0x8007688cu;assert(battle_atlas_render_try_host(&rt,&cpu,0x80025fa8u)==-1);free(bytes);puts("25FA8 guest ABI gate PASS (SDK child refuses)");return 0;}
