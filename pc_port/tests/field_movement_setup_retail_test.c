/* Extracted helper boundary; trigonometric calls are stubs on both sides. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "battle_mips_adapter.h"
#define DATA 0x80150000u
#define FIELD 0x80151000u
static uint8_t ram[0x200000];
_Alignas(4) static uint8_t actor[0x120],field[0x5c];
void *g_FieldScriptVMCurActor=actor,*D_800B06B8=field;
static int sine,cosine;
static unsigned cases,ncalls,rcalls;
static uint32_t ntrace[4],rtrace[4];
static void check(int ok,const char *why){if(!ok){fprintf(stderr,"MOVEMENT SETUP FAIL case=%u %s\n",cases,why);exit(1);}}
static uint32_t get(uint32_t a,unsigned w){a&=0x1fffffffu;check(a<=sizeof(ram)-w,"read range");uint32_t v=0;for(unsigned i=0;i<w;i++)v|=(uint32_t)ram[a+i]<<(8*i);return v;}
static void put(uint32_t a,unsigned w,uint32_t v){a&=0x1fffffffu;check(a<=sizeof(ram)-w,"write range");for(unsigned i=0;i<w;i++)ram[a+i]=(uint8_t)(v>>(8*i));}
static int rd(void *p,uint32_t a,unsigned w,uint32_t *v){(void)p;*v=get(a,w);return 0;}
static int wr(void *p,uint32_t a,unsigned w,uint32_t v){(void)p;check((a>=DATA && a+w<=DATA+sizeof(actor))||(a>=0x801fe000u && a+w<=0x80200000u),"write destination");put(a,w,v);return 0;}
static int bridge(void *p,PcPortMipsCpu *c,uint32_t t){
    (void)p;if(t!=0x8003f8ccu && t!=0x8003f8b0u)return 0;
    check(rcalls<2,"retail call count");check(t==(rcalls?0x8003f8b0u:0x8003f8ccu),"retail call order");
    rtrace[rcalls*2]=c->gpr[4];rtrace[rcalls*2+1]=get(DATA+0x60,2);
    c->gpr[2]=(uint32_t)(rcalls?cosine:sine);rcalls++;return 1;
}
int rsin(int a){check(ncalls==0,"native sine order");ntrace[0]=(uint32_t)a;ntrace[1]=*(uint16_t*)(actor+0x60);ncalls++;return sine;}
int rcos(int a){check(ncalls==1,"native cosine order");ntrace[2]=(uint32_t)a;ntrace[3]=*(uint16_t*)(actor+0x60);ncalls++;return cosine;}
void func_80092808(void);
static void run(int value,unsigned variant){
    cases++;sine=value;cosine=-value;ncalls=rcalls=0;
    memset(actor,0xa5,sizeof(actor));memset(field,0x39,sizeof(field));
    *(uint16_t*)(field+0x52)=(uint16_t)(variant?0x8001:0x1234);
    *(uint32_t*)(actor+4)=variant?0xffffffffu:0x12340000u;
    *(uint16_t*)(actor+0xcc)=(uint16_t)(variant?0xffff:0x1234);
    memcpy(ram+(DATA&0x1fffffffu),actor,sizeof(actor));memcpy(ram+(FIELD&0x1fffffffu),field,sizeof(field));
    put(0x800b0078u,4,DATA);put(0x800b06b8u,4,FIELD);
    PcPortMipsBus bus={0};bus.read=rd;bus.write=wr;bus.bridge=bridge;PcPortMipsCpu cpu;
    PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[29]=0x801ff000u;cpu.gpr[31]=0x80001000u;
    int result=PcPortMipsRun(&cpu,0x80092808u,0x80001000u,1000);
    if(result!=PC_PORT_MIPS_HALTED)fprintf(stderr,"oracle %d %s\n",result,cpu.error);
    check(result==PC_PORT_MIPS_HALTED,"oracle completion");func_80092808();
    check(ncalls==2 && rcalls==2,"call counts");check(!memcmp(ntrace,rtrace,sizeof(ntrace)),"call arguments and intermediate X");
    check(!memcmp(actor,ram+(DATA&0x1fffffffu),sizeof(actor)),"final actor bytes");
    check(!memcmp(field,ram+(FIELD&0x1fffffffu),sizeof(field)),"field actor unchanged");
}
int main(void){
    FILE *f=fopen("disc/field.bin","rb");check(f!=NULL,"field image");size_t n=fread(ram+0x6faf0,1,260862,f);check(n==260862 && fgetc(f)==EOF,"image size");fclose(f);
    for(int v=-4096;v<=4096;v++)for(unsigned k=0;k<2;k++)run(v,k);
    printf("MOVEMENT SETUP PASS cases=%u (extracted helper; trig stubs)\n",cases);return 0;
}
