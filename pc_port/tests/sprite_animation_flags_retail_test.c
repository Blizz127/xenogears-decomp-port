/* Extracted native setup vs pinned retail, with an observed/stubbed callee.
 * This does not execute animation progression, rendering, or a playable route. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "battle_mips_adapter.h"
#define SPRITE 0x80150000u
#define STACK 0x801f0000u
#define SIZE 0xc0u
static uint8_t ram[0x200000], at_call[SIZE];
_Alignas(4) static uint8_t sprite[SIZE];
static unsigned cases, guest_calls, native_calls;
static uint32_t call_frame;
void func_80023170(void*, int16_t, int32_t, int32_t);
static void check(int ok, const char *why) {
    if (!ok) { fprintf(stderr,"SPRITE FLAGS FAIL case=%u %s\n",cases,why); exit(1); }
}
static int rd(void *ctx,uint32_t addr,unsigned width,uint32_t *value) {
    (void)ctx; addr &= 0x1fffffffu;
    check(addr <= sizeof(ram)-width,"read range"); *value=0;
    for(unsigned i=0;i<width;i++) *value |= (uint32_t)ram[addr+i]<<(8*i);
    return 0;
}
static int wr(void *ctx,uint32_t addr,unsigned width,uint32_t value) {
    (void)ctx;
    check((addr>=SPRITE && addr+width<=SPRITE+SIZE) ||
          (addr>=STACK-24 && addr+width<=STACK),"write range");
    addr &= 0x1fffffffu;
    for(unsigned i=0;i<width;i++) ram[addr+i]=(uint8_t)(value>>(8*i));
    return 0;
}
static int bridge(void *ctx,PcPortMipsCpu *cpu,uint32_t target) {
    (void)ctx;
    if(target!=0x8001d2b0u) return 0;
    check(++guest_calls==1,"one guest call");
    check(cpu->gpr[4]==SPRITE,"guest sprite forwarding");
    call_frame=cpu->gpr[5];
    memcpy(at_call,ram+(SPRITE&0x1fffffffu),SIZE);
    /* A visible callee effect must survive the caller's return. */
    return wr(NULL,SPRITE+0x9e,1,0xa7)==0 ? 1 : -1;
}
void func_8001D2B0(void *p,int16_t frame) {
    check(++native_calls==1,"one native call");
    check(p==sprite,"native sprite forwarding");
    check((uint32_t)(int32_t)frame==call_frame,"frame forwarding");
    check(!memcmp(sprite,at_call,SIZE),"state at callee entry");
    sprite[0x9e]=0xa7;
}
static void run(uint32_t flags,int16_t frame,int32_t a,int32_t b) {
    cases++;guest_calls=native_calls=0;
    for(unsigned i=0;i<SIZE;i++) sprite[i]=(uint8_t)(i*37+cases*13);
    memcpy(sprite+0x3c,&flags,4);
    memcpy(ram+(SPRITE&0x1fffffffu),sprite,SIZE);
    PcPortMipsBus bus={0};bus.read=rd;bus.write=wr;bus.bridge=bridge;
    PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
    cpu.gpr[4]=SPRITE;cpu.gpr[5]=(uint32_t)(int32_t)frame;
    cpu.gpr[6]=(uint32_t)a;cpu.gpr[7]=(uint32_t)b;
    cpu.gpr[29]=STACK;cpu.gpr[31]=0x80001000u;
    int rc=PcPortMipsRun(&cpu,0x80023170u,0x80001000u,100);
    check(rc==PC_PORT_MIPS_HALTED,"oracle returned");
    check(guest_calls==1 && cpu.gpr[29]==STACK,"guest call and stack restored");
    func_80023170(sprite,frame,a,b);
    check(native_calls==1,"native call count");
    check(!memcmp(sprite,ram+(SPRITE&0x1fffffffu),SIZE),"state after return");
}
int main(void) {
    FILE *f=fopen("disc/SLUS_006.64","rb");check(f!=NULL,"image open");
    check(fread(ram+0xf800,1,303104,f)==303104 && fgetc(f)==EOF,"image size");fclose(f);
    const int32_t inputs[]={0,1,2,-1,INT32_MIN,INT32_MAX};
    const int16_t frames[]={0,1,-1,INT16_MIN,INT16_MAX};
    const uint32_t upper[]={0,0xffffffc0u,0x55555540u,0xaaaaaa80u};
    for(unsigned h=0;h<4;h++)for(unsigned low=0;low<64;low++)
    for(unsigned a=0;a<6;a++)for(unsigned b=0;b<6;b++)for(unsigned f=0;f<5;f++)
        run(upper[h]|low,frames[f],inputs[a],inputs[b]);
    printf("SPRITE FLAGS PASS cases=%u (extracted native vs pinned retail; callee stubbed)\n",cases);
    return 0;
}
