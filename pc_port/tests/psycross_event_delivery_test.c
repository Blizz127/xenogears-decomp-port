#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "battle_mips_adapter.h"
struct EXEC;
#include "psx/libapi.h"
extern void PsyX_Sys_InitSoundGate(void);
extern void PsyX_Sys_DispatchCounter2(void);
static uint8_t bios[0x80000],ram[0x200000];
static int native_calls,retail_calls;
static int disable_target;
static long callback(void) { ++native_calls;if(disable_target)DisableEvent(disable_target);return 0; }
static int read_bus(void *u,uint32_t a,unsigned w,uint32_t *v) {
    (void)u;uint8_t*p;
    if(a>=0x1b44 && (uint64_t)a+w<=0x1f88) p=bios+0xfb00+a;
    else if((a&0x1fffffff)+w<=sizeof ram) p=ram+(a&0x1fffffff);
    else return -1;
    *v=0;for(unsigned i=0;i<w;++i)*v|=(uint32_t)p[i]<<(8*i);return 0;
}
static int write_bus(void*u,uint32_t a,unsigned w,uint32_t v) {
    (void)u;a&=0x1fffffff;if((uint64_t)a+w>sizeof ram)return -1;
    for(unsigned i=0;i<w;++i)ram[a+i]=v>>(8*i);return 0;
}
static int bridge(void*u,PcPortMipsCpu*c,uint32_t target) {
    (void)u;(void)c;if(target!=0x80030000)return 0;++retail_calls;return 1;
}
static void word(uint32_t a,uint32_t v) { assert(!write_bus(NULL,a,4,v)); }
static uint32_t run(uint32_t pc,uint32_t a,uint32_t b) {
    PcPortMipsBus bus={.read=read_bus,.write=write_bus,.bridge=bridge};PcPortMipsCpu cpu;
    PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[4]=a;cpu.gpr[5]=b;
    cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffc;
    int status=PcPortMipsRun(&cpu,pc,0xfffffffc,10000);
    if(status)fprintf(stderr,"BIOS event failure %s\n",cpu.error);
    assert(status==0);return cpu.gpr[2];
}
int main(void) {
    FILE*f=fopen("disc/scph5500.bin","rb");assert(f);
    assert(fread(bios,1,sizeof bios,f)==sizeof bios);fclose(f);
    PsyX_Sys_InitSoundGate();
    /* Native handles stay opaque; compare their slots with BIOS F100 IDs. */
    for(int state=0;state<3;++state)for(int mode=0;mode<4;++mode)
    for(int match=0;match<4;++match)for(int has_callback=0;has_callback<2;++has_callback) {
        const int modes[]={0x1000,0x2000,0,0x3000};
        memset(ram,0,sizeof ram);word(0x120,0x80010000);word(0x124,28);
        word(0x10000,0xf4000001);word(0x10004,state==0?0:state==1?0x1000:0x2000);
        word(0x10008,4);word(0x1000c,modes[mode]);word(0x10010,has_callback?0x80030000:0);
        int h=OpenEvent(0xf4000001,4,modes[mode],has_callback?callback:NULL);assert(h>0);
        if(state==0)CloseEvent(h);else if(state==2)EnableEvent(h);
        native_calls=retail_calls=0;
        uint32_t desc=(match&1)?0xf4000002:0xf4000001;int spec=(match&2)?8:4;
        for(int repeat=0;repeat<2;++repeat){DeliverEvent(desc,spec);run(0x1b44,desc,spec);}
        assert(native_calls==retail_calls);
        if(state!=2 || (mode==1 && match==0))
            assert(WaitEvent(h)==(int)run(0x1e44,0xf1000000,0));
        assert(TestEvent(h)==(int)run(0x1ec8,0xf1000000,0));
        assert(TestEvent(h)==(int)run(0x1ec8,0xf1000000,0));
        DeliverEvent(desc,spec);run(0x1b44,desc,spec);
        UnDeliverEvent(desc,spec);run(0x1c5c,desc,spec);
        assert(TestEvent(h)==(int)run(0x1ec8,0xf1000000,0));
        if(state){
            EnableEvent(h);run(0x1f10,0xf1000000,0);
            DeliverEvent(0xf4000001,4);run(0x1b44,0xf4000001,4);
            DisableEvent(h);run(0x1f4c,0xf1000000,0);
            EnableEvent(h);run(0x1f10,0xf1000000,0);
            assert(TestEvent(h)==(int)run(0x1ec8,0xf1000000,0));
        }
        CloseEvent(h);
    }
    /* All matching registrations receive delivery, not only the first. */
    int a=OpenEvent(1,2,0x2000,NULL),b=OpenEvent(1,2,0x2000,NULL);
    EnableEvent(a);EnableEvent(b);DeliverEvent(1,2);
    assert(TestEvent(a)==1 && TestEvent(b)==1);CloseEvent(a);CloseEvent(b);
    a=OpenEvent(7,8,0x1000,callback);b=OpenEvent(7,8,0x2000,NULL);
    EnableEvent(a);EnableEvent(b);disable_target=b;native_calls=0;
    DeliverEvent(7,8);assert(native_calls==1 && TestEvent(b)==0);
    disable_target=0;CloseEvent(a);CloseEvent(b);
    a=OpenEvent(0xf2000002,2,0x1000,callback);
    b=OpenEvent(0xf2000002,2,0x2000,NULL);
    EnableEvent(a);EnableEvent(b);native_calls=0;
    PsyX_Sys_DispatchCounter2();assert(native_calls==1 && TestEvent(b)==1);
    DisableEvent(a);PsyX_Sys_DispatchCounter2();assert(native_calls==1);
    EnableEvent(a);PsyX_Sys_DispatchCounter2();assert(native_calls==2);
    CloseEvent(a);CloseEvent(b);
    assert(TestEvent(0)==0 && TestEvent(0x80000000u)==0 && TestEvent(0xffffffffu)==0);
    puts("PASS BIOS/native event delivery differential: 96 state/mode/match/callback cases and duplicate registrations");
}
