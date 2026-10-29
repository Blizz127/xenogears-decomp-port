/* Native helper vs pinned main-executable oracle; no SPU/audio execution. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "battle_mips_adapter.h"
#define MANAGER 0x80150000u
#define SIZE (0x98+64*0x158+0x100)
_Alignas(4) static uint8_t manager[SIZE];
static uint8_t ram[0x200000];
uint32_t D_800595D8;
int32_t D_80059478;
static unsigned cases;
void func_8003A450(int32_t,int32_t,int32_t);
void func_8003A3B8(int32_t,int32_t,int32_t);
static void check(int ok,const char *why){if(!ok){fprintf(stderr,"PAIR PITCH FAIL case=%u %s\n",cases,why);exit(1);}}
static uint32_t get(uint32_t a,unsigned w){a&=0x1fffffffu;check(a<=sizeof(ram)-w,"read range");uint32_t v=0;for(unsigned i=0;i<w;i++)v|=(uint32_t)ram[a+i]<<(8*i);return v;}
static void put(uint32_t a,unsigned w,uint32_t v){a&=0x1fffffffu;check(a<=sizeof(ram)-w,"write range");for(unsigned i=0;i<w;i++)ram[a+i]=(uint8_t)(v>>(8*i));}
static int rd(void *p,uint32_t a,unsigned w,uint32_t *v){(void)p;*v=get(a,w);return 0;}
static int wr(void *p,uint32_t a,unsigned w,uint32_t v){(void)p;check(a>=MANAGER && a+w<=MANAGER+SIZE,"retail write destination");put(a,w,v);return 0;}
static void run(int channel,int pitch,int duration,int current,unsigned active,int data_mode,int count,unsigned selection){
    cases++;memset(manager,0x39,SIZE);
    for(unsigned i=0;i<64;i++){
        uint8_t *f=manager+0x94+i*0x158;
        *(uint16_t*)f=(uint16_t)(0x8200|((active>>(i&1))&1));
        *(int16_t*)(f+4+0x72)=(int16_t)current;
        *(uint32_t*)(f+8)=(selection==0 || (selection==2 && !(i&1)))?0x12345600u:0x12345604u;
    }
    memcpy(ram+(MANAGER&0x1fffffffu),manager,SIZE);put(0x800595d8u,4,MANAGER);
    D_800595D8=(uint32_t)(uintptr_t)manager;
    D_80059478=count;put(0x80059478u,4,(uint32_t)count);
    PcPortMipsBus bus={0};bus.read=rd;bus.write=wr;PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
    cpu.gpr[4]=data_mode?0x12345600u:(uint32_t)channel;cpu.gpr[5]=(uint32_t)pitch;cpu.gpr[6]=(uint32_t)duration;cpu.gpr[31]=0x80001000;
    int rc=PcPortMipsRun(&cpu,data_mode?0x8003a3b8u:0x8003a450u,0x80001000u,10000);
    if(rc!=PC_PORT_MIPS_HALTED)fprintf(stderr,"oracle=%d %s\n",rc,cpu.error);
    check(rc==PC_PORT_MIPS_HALTED,"oracle completion");
    if(data_mode)func_8003A3B8(0x12345600,pitch,duration);else func_8003A450(channel,pitch,duration);
    if(memcmp(manager,ram+(MANAGER&0x1fffffffu),SIZE)){
        fprintf(stderr,"%s PITCH FAIL case=%u channel=%d pitch=%d duration=%d current=%d active=%u count=%d selection=%u\n",data_mode?"DATA":"PAIR",cases,channel,pitch,duration,current,active,count,selection);exit(1);
    }
    check(D_80059478==count,"record count unchanged");
    check(D_800595D8==(uint32_t)(uintptr_t)manager,"manager pointer unchanged");
}
int main(void){
    const int pitches[]={0,1,127,128,255,-1,-32768,256,32767};
    const int durations[]={0,1,2,-1,-32768,32767};
    const int current[]={0,256,32512,-1,-32768};
    check((uintptr_t)manager<=UINT32_MAX,"packed pointer fixture");
    FILE *f=fopen("disc/SLUS_006.64","rb");check(f!=NULL,"main executable");size_t n=fread(ram+0xf800,1,303104,f);check(n==303104 && fgetc(f)==EOF,"image size");fclose(f);
    for(int ch=0;ch<16;ch++)for(unsigned p=0;p<9;p++)for(unsigned d=0;d<6;d++)for(unsigned c=0;c<5;c++)for(unsigned a=0;a<4;a++)run(ch,pitches[p],durations[d],current[c],a,0,16,0);
    printf("PAIR PITCH PASS cases=%u (native helper vs pinned retail)\n",cases);
    cases=0;
    const int counts[]={1,2,16};
    for(unsigned n=0;n<3;n++)for(unsigned sel=0;sel<3;sel++)for(unsigned p=0;p<9;p++)for(unsigned d=0;d<6;d++)for(unsigned c=0;c<5;c++)for(unsigned a=0;a<4;a++)run(0,pitches[p],durations[d],current[c],a,1,counts[n],sel);
    printf("DATA PITCH PASS cases=%u (native helper vs pinned retail)\n",cases);return 0;
}
