#include <stdio.h>
#include <string.h>
#ifndef ACTOR_RENDER_SOURCE
#define ACTOR_RENDER_SOURCE "../../src/field/main/misc2.c"
#endif
#include ACTOR_RENDER_SOURCE
#include "battle_mips_adapter.h"
s16 D_800B218E;
s32 D_80050100;
static u8 ram[0x200000];
static struct {u32 actor[0x150/4],sprite[0xb0/4],ot[256];} fixture,initial,expected;
static struct Event {u32 call,a,b,c,mask;} events[8],expectedEvents[8];
static unsigned nEvents;
static unsigned callbackMutation;
static int depth;
static void record(u32 call,u32 a,u32 b,u32 c) {
    assert(nEvents<8);
    events[nEvents++]=(struct Event){call,a,b,c,((u8*)fixture.sprite)[0x3d]};
}
void SpriteSetColor(void* sprite,u8 r,u8 g,u8 b) {
    assert(sprite==fixture.sprite);record(0x80021b98,r,g,b);
}
void func_8001E298(void* sprite,void* ot) {
    assert(sprite==fixture.sprite);record(0x8001e298,(u32)(uintptr_t)ot,0,0);
}
void func_8001E2F8(void* sprite,void* ot,s16 y) {
    assert(sprite==fixture.sprite);record(0x8001e2f8,(u32)(uintptr_t)ot,(s32)y,0);
    if(callbackMutation==1)fixture.actor[0x134/4]|=0x40;
    if(callbackMutation==2)fixture.actor[0x134/4]&=~0x40u;
}
void func_8001E368(void* sprite,void* ot,s16 y) {
    assert(sprite==fixture.sprite);record(0x8001e368,(u32)(uintptr_t)ot,(s32)y,0);
}
int RotTransPers(SVECTOR* v,int* xy,long* p,long* flag) {
    record(0x8004a64c,(s32)v->vx,(s32)v->vy,(s32)v->vz);
    *xy=0;*p=0;*flag=0;return depth;
}
static u8* address(u32 a,unsigned w) {
    if(a>=0x80000000u && (uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);
    uintptr_t lo=(uintptr_t)&fixture;
    if(a>=lo && (uint64_t)a+w<=lo+sizeof(fixture))return (u8*)(uintptr_t)a;
    return NULL;
}
static int rd(void* u,u32 a,unsigned w,u32* v) {
    (void)u;u8* p=address(a,w);if(!p)return -1;*v=0;
    for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(8*i);return 0;
}
static int wr(void* u,u32 a,unsigned w,u32 v) {
    (void)u;u8* p=address(a,w);if(!p)return -1;
    for(unsigned i=0;i<w;++i)p[i]=(u8)(v>>(8*i));return 0;
}
static int bridge(void* u,PcPortMipsCpu* cpu,u32 t) {
    (void)u;u32* a=cpu->gpr+4;
    switch(t) {
    case 0x80021b98:SpriteSetColor((void*)(uintptr_t)a[0],a[1],a[2],a[3]);break;
    case 0x8001e298:func_8001E298((void*)(uintptr_t)a[0],(void*)(uintptr_t)a[1]);break;
    case 0x8001e2f8:func_8001E2F8((void*)(uintptr_t)a[0],(void*)(uintptr_t)a[1],a[2]);break;
    case 0x8001e368:func_8001E368((void*)(uintptr_t)a[0],(void*)(uintptr_t)a[1],a[2]);break;
    case 0x8004a64c: {
        SVECTOR v;memcpy(&v,address(a[0],8),8);
        record(t,(s32)v.vx,(s32)v.vy,(s32)v.vz);
        assert(!wr(NULL,a[1],4,0));assert(!wr(NULL,a[2],4,0));assert(!wr(NULL,a[3],4,0));
        cpu->gpr[2]=depth;break;
    }
    default:return 0;
    }
    return 1;
}
int main(void) {
    FILE* f=fopen("disc/field.bin","rb");assert(f);
    assert(fread(ram+0x6faf0,1,260862,f)==260862);assert(!fclose(f));
    static const u16 states[]={0,0xffdd,0xffde,0xffdf,0xffe0};
    static const s16 heights[]={-32768,-13,0,17,32767};
    static const int depths[]={-17,0,1,2,7,128};
    static const int shifts[]={0,1,2,31,32,-1};
    unsigned cases=0;
    for(unsigned st=0;st<5;++st) for(unsigned flags=0;flags<4;++flags)
    for(unsigned hidden=0;hidden<2;++hidden) for(unsigned fog=0;fog<2;++fog)
    for(unsigned h=0;h<5;++h) for(unsigned di=0;di<6;++di)
    for(unsigned shift=0;shift<6;++shift)
    for(callbackMutation=0;callbackMutation<3;++callbackMutation) {
        memset(&fixture,0xa5,sizeof(fixture));
        u8* actor=(u8*)fixture.actor;
        *(u16*)(actor+0xe8)=states[st];*(s16*)(actor+0xee)=heights[h];
        fixture.actor[1]=hidden?0x02000000:0;
        fixture.actor[0x134/4]=flags<<5;
        for(unsigned i=0;i<6;++i)actor[0xfc+i]=17+i*31;
        D_800B218E=fog;D_80050100=shifts[shift];depth=depths[di];
        s32 sceneDip=heights[(h+1)%5];s32 index=depths[(di+1)%6];
        void* ot=fixture.ot+128;
        initial=fixture;nEvents=0;memset(events,0,sizeof(events));
        assert(!wr(NULL,0x800b218e,2,fog));assert(!wr(NULL,0x80050100,4,shifts[shift]));
        PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};
        PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
        cpu.gpr[17]=(u32)(uintptr_t)actor;cpu.gpr[18]=(u32)(uintptr_t)fixture.sprite;
        cpu.gpr[21]=(u32)(uintptr_t)ot;cpu.gpr[29]=0x801ff000;
        assert(!wr(NULL,cpu.gpr[29]+0xa4,4,index));
        assert(!wr(NULL,cpu.gpr[29]+0xa8,4,sceneDip));
        if(PcPortMipsRun(&cpu,0x80076118,0x80076468,5000)!=PC_PORT_MIPS_HALTED) {
            fprintf(stderr,"ACTOR TAIL FAIL oracle %s\n",cpu.error);return 1;
        }
        expected=fixture;memcpy(expectedEvents,events,sizeof(events));unsigned count=nEvents;
        fixture=initial;nEvents=0;memset(events,0,sizeof(events));
        FieldRenderActorSpriteTail(fixture.sprite,(u8*)fixture.actor,ot,index,sceneDip);
        if(memcmp(&fixture,&expected,sizeof(fixture)) || count!=nEvents ||
           memcmp(events,expectedEvents,sizeof(events))) {
            fprintf(stderr,"ACTOR TAIL FAIL state=%x flags=%u hidden=%u fog=%u height=%d depth=%d shift=%u events=%u/%u\n",
                    states[st],flags,hidden,fog,heights[h],depth,shift,nEvents,count);return 1;
        }
        ++cases;
    }
    printf("ACTOR TAIL PASS %u cases: flags/color/projection/ordered draws/sprite mask\n",cases);
}
