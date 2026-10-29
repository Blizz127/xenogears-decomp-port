/* Extracted native helper vs pinned retail CPU oracle. Callee is a capturing
 * stub on both sides; production reachability and playable behavior unproven. */
#include "field_turn_retail_test.h"
#include "battle_mips_adapter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RAM_SIZE 0x200000u
#define BASE 0x8006faf0u
#define DATA 0x80150000u
#define SPRITE 0x80151000u
#define ACTOR 0x80152000u
#define HALT 0x80001000u
static u8 ram[RAM_SIZE];
_Alignas(u32) static u8 data[0x120],sprite[64],actor[128];
static unsigned cases,native_count,retail_count;
static u16 callback_angle;
static int mutate;
typedef struct {u32 sprite,actor,angle,before104,before106;} Call;
static Call nc,rc;
static void require(int ok,const char *what) {
    if(!ok){fprintf(stderr,"FIELD TURN FAIL case=%u %s\n",cases,what);exit(1);}
}
static unsigned offset(u32 a,unsigned w){unsigned o=a&0x1fffffffu;require(o<=RAM_SIZE-w,"bus range");return o;}
static u32 get(u32 a,unsigned w){unsigned o=offset(a,w);u32 v=0;for(unsigned i=0;i<w;i++)v|=(u32)ram[o+i]<<(8*i);return v;}
static void put(u32 a,unsigned w,u32 v){unsigned o=offset(a,w);for(unsigned i=0;i<w;i++)ram[o+i]=(u8)(v>>(8*i));}
static int read_bus(void *p,u32 a,unsigned w,u32 *v){(void)p;*v=get(a,w);return 0;}
static int write_bus(void *p,u32 a,unsigned w,u32 v){
    (void)p;require((a>=DATA && a+w<=DATA+sizeof(data)) || (a>=0x801fe000u && a+w<=0x80200000u),"retail write range");put(a,w,v);return 0;
}
static int bridge(void *p,PcPortMipsCpu *cpu,u32 target){
    (void)p;if(target!=0x80081f80u)return 0;
    require(++retail_count==1,"retail call count");
    rc=(Call){cpu->gpr[4],cpu->gpr[6],cpu->gpr[5],get(DATA+0x104,2),get(DATA+0x106,2)};
    if(mutate)put(DATA+0x106,2,callback_angle);
    return 1;
}
void func_80081F80(void *s,s16 angle,void *a){
    require(++native_count==1,"native call count");
    require(s==sprite,"sprite argument identity");
    require(a==actor,"field-actor argument identity");
    nc=(Call){SPRITE,ACTOR,(u32)(s32)angle,*(u16*)(data+0x104),*(u16*)(data+0x106)};
    if(mutate)*(u16*)(data+0x106)=callback_angle;
}
static void run_case(u16 angle,u32 x,u32 z,s32 flags,int effect){
    PcPortMipsCpu cpu;PcPortMipsBus bus={0};cases++;native_count=retail_count=0;
    memset(&nc,0,sizeof(nc));memset(&rc,0,sizeof(rc));
    memset(data,0xa5,sizeof(data));memset(sprite,0x39,sizeof(sprite));memset(actor,0x72,sizeof(actor));
    *(u32*)(data+0x30)=x;*(u32*)(data+0x38)=z;
    *(u16*)(data+0x104)=0xbeef;*(u16*)(data+0x106)=angle;
    mutate=effect!=0;callback_angle=(u16)(effect==1?0x123:effect==2?0xfedc:0);
    memcpy(ram+offset(DATA,sizeof(data)),data,sizeof(data));
    memcpy(ram+offset(SPRITE,sizeof(sprite)),sprite,sizeof(sprite));
    memcpy(ram+offset(ACTOR,sizeof(actor)),actor,sizeof(actor));
    bus.read=read_bus;bus.write=write_bus;bus.bridge=bridge;
    PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[29]=0x801ff000u;cpu.gpr[31]=HALT;
    cpu.gpr[4]=SPRITE;cpu.gpr[5]=DATA;cpu.gpr[6]=ACTOR;cpu.gpr[7]=(u32)flags;
    int result=PcPortMipsRun(&cpu,0x800831f4u,HALT,1000);
    if(result!=PC_PORT_MIPS_HALTED)fprintf(stderr,"oracle rc=%d %s\n",result,cpu.error);
    require(result==PC_PORT_MIPS_HALTED,"oracle completed");
    func_800831F4(sprite,data,actor,flags);
    require(native_count==retail_count,"call count");require(memcmp(&nc,&rc,sizeof(nc))==0,"call trace / pre-call state");
    require(memcmp(data,ram+offset(DATA,sizeof(data)),sizeof(data))==0,"final actor data");
    for(unsigned i=0;i<sizeof(sprite);i++)require(sprite[i]==0x39,"sprite unchanged");
    for(unsigned i=0;i<sizeof(actor);i++)require(actor[i]==0x72,"field actor unchanged");
}
int main(int argc,char **argv){
    const u16 angles[]={0,1,0x3ff,0x400,0xbff,0xc00,0xfff,0x1000,0x7fff,0x8000,0x8001,0xffff};
    const u32 motion[]={0,1,0xffffffffu,0x80000000u};
    const s32 flags[]={0,1,2,3,-1,0x7ffffffe};
    require((uintptr_t)actor > UINT32_MAX,"64-bit native pointer fixture");
    require(argc==2,"field image argument");FILE *f=fopen(argv[1],"rb");require(f!=NULL,"open field image");
    size_t n=fread(ram+(BASE&0x1fffffffu),1,260862,f);require(n==260862 && fgetc(f)==EOF,"field image size");fclose(f);
    for(unsigned a=0;a<12;a++)for(unsigned x=0;x<4;x++)for(unsigned z=0;z<4;z++)for(unsigned fl=0;fl<6;fl++)for(int effect=0;effect<4;effect++)run_case(angles[a],motion[x],motion[z],flags[fl],effect);
    printf("FIELD TURN PASS cases=%u (extracted native helper vs pinned retail oracle)\n",cases);return 0;
}
