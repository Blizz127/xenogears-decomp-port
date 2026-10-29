/* Test-only retail CPU oracle stops before TransMatrix. Native uses the same
 * observed boundary; no GTE, draw call or later matrix-branch parity claim. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include "battle_mips_adapter.h"
#include "sprite_translation_boundary_fixture.h"
#define ENTRY 0x80140000u
#define SPRITE 0x80150000u
#define SUB 0x80160000u
#define STACK 0x801f0000u
static uint8_t ram[0x200000];
_Alignas(8) static u8 entry[16],sprite[0x80],sub[0x60];
static jmp_buf boundary;
static unsigned cases,native_update,guest_update,native_trans;
static s32 expected[3];
MATRIX D_8004FBB8;
s32 g_GfxCurContext;
u_long *g_GfxCurOT;
static void check(int ok,const char *why) {
    if(!ok){fprintf(stderr,"SPRITE TRANSLATION FAIL case=%u %s\n",cases,why);exit(1);}
}
static int rd(void *ctx,uint32_t addr,unsigned width,uint32_t *value) {
    (void)ctx;addr&=0x1fffffffu;check(addr<=sizeof(ram)-width,"read range");*value=0;
    for(unsigned i=0;i<width;i++)*value|=(uint32_t)ram[addr+i]<<(8*i);
    return 0;
}
static void put(uint32_t addr,unsigned width,uint32_t value) {
    addr&=0x1fffffffu;check(addr<=sizeof(ram)-width,"write range");
    for(unsigned i=0;i<width;i++)ram[addr+i]=(uint8_t)(value>>(8*i));
}
static int wr(void *ctx,uint32_t addr,unsigned width,uint32_t value) {
    (void)ctx;check(addr>=STACK-0x48 && addr+width<=STACK,"oracle stack write");
    put(addr,width,value);return 0;
}
static int bridge(void *ctx,PcPortMipsCpu *cpu,uint32_t target) {
    (void)ctx;if(target!=0x80022038u)return 0;
    check(cpu->gpr[4]==SPRITE && ++guest_update==1,"guest updater");return 1;
}
void func_80022038(void *p) {check(p==sprite && ++native_update==1,"native updater");}
MATRIX *TransMatrix(MATRIX *m,VECTOR *v) {
    check(++native_trans==1 && native_update==1,"translation call order");
    check((void*)m==sub+0xc,"translation matrix pointer");
    for(unsigned i=0;i<3;i++) {
        s32 actual;memcpy(&actual,(u8*)v+4*i,4);
        check(actual==expected[i],"sign-extended coordinate");
    }
    longjmp(boundary,1);
}
MATRIX *CompMatrix(MATRIX *a,MATRIX *b,MATRIX *c){(void)a;(void)b;(void)c;check(0,"past boundary");return NULL;}
void SetRotMatrix(MATRIX *m){(void)m;check(0,"past boundary");}
void SetTransMatrix(MATRIX *m){(void)m;check(0,"past boundary");}
void func_8002C700(void *a,void *b,u_long *c,s32 d){(void)a;(void)b;(void)c;(void)d;check(0,"past boundary");}
static void run(s16 x,s16 y,s16 z,unsigned present,unsigned flag) {
    cases++;native_update=guest_update=native_trans=0;
    memset(sprite,0,sizeof(sprite));memset(sub,0,sizeof(sub));
    u8 *p=sprite;memcpy(entry+8,&p,sizeof(p));p=sub;memcpy(sprite+0x20,&p,sizeof(p));
    memcpy(sprite+2,&x,2);memcpy(sprite+6,&y,2);memcpy(sprite+10,&z,2);
    memcpy(sub+0x34,&present,4);sprite[0x3f]=(u8)flag;
    put(ENTRY+4,4,SPRITE);put(SPRITE+0x20,4,SUB);put(SUB+0x34,4,present);
    put(SPRITE+2,2,(uint16_t)x);put(SPRITE+6,2,(uint16_t)y);put(SPRITE+10,2,(uint16_t)z);put(SPRITE+0x3f,1,flag);
    PcPortMipsBus bus={0};bus.read=rd;bus.write=wr;bus.bridge=bridge;
    PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
    cpu.gpr[4]=ENTRY;cpu.gpr[29]=STACK;cpu.gpr[31]=0x80001000u;
    int rc=PcPortMipsRun(&cpu,0x80025718u,present?0x80049d9cu:0x80001000u,100);
    check(rc==PC_PORT_MIPS_HALTED && guest_update==1,"retail boundary reached");
    if(present) {
        check(cpu.gpr[4]==SUB+0xc,"retail matrix pointer");
        for(unsigned i=0;i<3;i++){uint32_t value;rd(NULL,cpu.gpr[5]+i*4,4,&value);memcpy(expected+i,&value,4);}
        check(expected[0]==x && expected[1]==y && expected[2]==z,"retail coordinate contract");
    }
    if(setjmp(boundary)==0)func_80025718(entry+4);
    check(native_update==1 && native_trans==present,"native boundary/early return");
}
int main(void) {
    FILE *f=fopen("disc/SLUS_006.64","rb");check(f!=NULL,"image open");
    check(fread(ram+0xf800,1,303104,f)==303104 && fgetc(f)==EOF,"image size");fclose(f);
    run(1,2,3,1,0);
    const s16 v[]={0,1,-1,127,-128,INT16_MIN,INT16_MAX};
    for(unsigned x=0;x<7;x++)for(unsigned y=0;y<7;y++)for(unsigned z=0;z<7;z++)
    for(unsigned p=0;p<2;p++)for(unsigned flag=0;flag<2;flag++)run(v[x],v[y],v[z],p,flag);
    printf("SPRITE TRANSLATION PASS cases=%u (extracted helper; stop before TransMatrix)\n",cases);return 0;
}
