#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "battle_mips_adapter.h"

#define ENTRY 0x80022B2Cu
#define END 0x80022D44u
#define BASE 0x100000u
#define SIZE 0x10000u
#define HALT 0xFFFFFFFCu
extern void func_80022CDC(void *);
static uint8_t ram[0x200000], code[END-ENTRY];
static unsigned seen[(END-ENTRY)/4], cases;
static void check(int ok, const char *s) {
    if (!ok) { fprintf(stderr,"SPRITE MOTION FAIL case=%u %s\n",cases,s); exit(1); }
}
static int read_bus(void *o,uint32_t a,unsigned w,uint32_t *v) {
    (void)o;
    if (a >= ENTRY && a+w <= END) {
        *v=0; memcpy(v,code+a-ENTRY,w); seen[(a-ENTRY)/4]=1; return 0;
    }
    a &= 0x1FFFFFu;
    if (a+w>sizeof(ram)) return -1;
    *v=0; memcpy(v,ram+a,w); return 0;
}
static int write_bus(void *o,uint32_t a,unsigned w,uint32_t v) {
    (void)o; a &= 0x1FFFFFu;
    if (a+w>sizeof(ram)) return -1;
    memcpy(ram+a,&v,w); return 0;
}
static unsigned native_calls,oracle_calls;
static uint8_t native_query[256],oracle_query[256];
static int16_t query_floor;
static void* expected_native_sprite;
static void floor_query(uint8_t*p,uint8_t*snapshot,unsigned*count){
 (*count)++; memcpy(snapshot,p,256);memcpy(p+0x84,&query_floor,2);
}
#ifdef MATCHING_SPRITE_OWNER
void func_800BA8F4(void*p){
 check(p==expected_native_sprite,"native floor argument");floor_query(p,native_query,&native_calls);
}
#else
int PcPort_BattleMipsDispatchCallback(uint32_t target,void*p){
 check(target==0x800ba8f4u,"native callback target");floor_query(p,native_query,&native_calls);return 1;
}
#endif
static int bridge(void*o,PcPortMipsCpu*c,uint32_t target){
 (void)o;if(target!=0x800ba8f4u)return 0;
 check(c->gpr[4]==BASE,"oracle callback argument");floor_query(ram+BASE,oracle_query,&oracle_calls);return 1;
}
static void put(uint8_t*p,unsigned off,uint32_t v){memcpy(p+off,&v,4);}
int main(int argc,char**argv){
 check(argc==2,"disc argument");FILE*f=fopen(argv[1],"rb");check(f!=NULL,"disc");
 fseek(f,ENTRY-0x8000f800u,SEEK_SET);check(fread(code,1,sizeof(code),f)==sizeof(code),"code");fclose(f);
 uint32_t storage[64];uint8_t*native=(uint8_t*)storage;
 const uint32_t v[]={0,1,15,16,0xffffffffu,0xfffffff0u,0x7fffffffu,0x80000000u,0x7fff0000u,0x80010000u,0x10000u,0xffff0000u};
 const int16_t floors[]={-32768,-1,0,1,32767};
 const unsigned timers[]={0,1,1024,65535};
 const unsigned bounce[]={0,1,255,1023};
 for(unsigned y=0;y<12;y++)for(unsigned vy=0;vy<12;vy++)for(unsigned g=0;g<12;g++)
 for(unsigned fl=0;fl<5;fl++)for(unsigned t=0;t<4;t++)for(unsigned b=0;b<4;b++)for(unsigned simple=0;simple<2;simple++){
  cases++;memset(native,0xa5,256);put(native,0,v[(y+3)%12]);put(native,4,v[y]);put(native,8,v[(y+7)%12]);
  put(native,12,v[(vy+2)%12]);put(native,16,v[vy]);put(native,20,v[(vy+5)%12]);put(native,28,v[g]);
  uint16_t timer=timers[t];memcpy(native+0x3a,&timer,2);put(native,0x3c,simple<<26);put(native,0xa8,bounce[b]<<1);
  query_floor=floors[fl];int16_t initial_floor=simple?query_floor:(int16_t)~query_floor;memcpy(native+0x84,&initial_floor,2);
  native_calls=oracle_calls=0;memset(native_query,0,256);memset(oracle_query,0,256);memcpy(ram+BASE,native,256);
  PcPortMipsBus bus={0};bus.read=read_bus;bus.write=write_bus;bus.bridge=bridge;PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
  cpu.gpr[4]=BASE;cpu.gpr[29]=0x801ff000;cpu.gpr[31]=HALT;
  int rc=PcPortMipsRun(&cpu,0x80022cdcu,HALT,300);if(rc!=PC_PORT_MIPS_HALTED)fprintf(stderr,"%s\n",cpu.error);check(rc==PC_PORT_MIPS_HALTED,"oracle");
  expected_native_sprite=native;func_80022CDC(native);
  if(memcmp(native,ram+BASE,256)){fprintf(stderr,"y=%08x vy=%08x gravity=%08x floor=%d timer=%u bounce=%u simple=%u\n",v[y],v[vy],v[g],query_floor,timers[t],bounce[b],simple);check(0,"sprite bytes");}
  check(native_calls==oracle_calls,"callback count");check(!memcmp(native_query,oracle_query,256),"callback order/state");
 }
 unsigned covered=0;for(unsigned i=0;i<sizeof(seen)/sizeof(seen[0]);i++)covered+=seen[i]!=0;
 /* 80022C08 negates gravity after this branch required gravity > 0.
  * No intervening instruction or callback writes gravity in this fixture. */
 for(unsigned i=0;i<sizeof(seen)/sizeof(seen[0]);i++)
  check((seen[i]!=0)==(ENTRY+i*4!=0x80022c08u),"reachable instruction coverage");
 printf("SPRITE MOTION PASS cases=%u instructions=%u/%zu\n",cases,covered,sizeof(seen)/sizeof(seen[0]));return 0;
}
