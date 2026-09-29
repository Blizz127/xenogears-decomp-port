#include "common.h"
#include "main/game.h"
#include "field/actor.h"
#include "battle_mips_adapter.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern void func_800AD4D4(s32) __attribute__((weak));
#ifdef TEST_GEAR_EXIT
extern void func_800ACFD0(s32) __attribute__((weak));
#define ENTRY func_800ACFD0
#define RETAIL_ENTRY 0x800acfd0
#define EXPECT_CALLS 5
#define EXPECT_RIDE 0
#else
#define ENTRY func_800AD4D4
#define RETAIL_ENTRY 0x800ad4d4
#define EXPECT_CALLS 4
#define EXPECT_RIDE 1
#endif
struct Fixture {
    FieldActor actors[6];
    u32 data[6][128];
    u32 sprites[6][128];
    u8 state[0x4000];
};
static struct Fixture fixture, expected;
FieldActor* volatile g_FieldActors=fixture.actors;
GameState* g_pGameState=(GameState*)fixture.state;
s32 D_800AFD1C;
s32 D_8005A444[3]={3,4,5};
s32 D_8006F990[3]={0,1,2};
static u8 ram[0x200000];
struct Call { u32 target,a0,a1,a2; };
static struct Call calls[8], expected_calls[8];
static unsigned call_count;
static void record(u32 t,u32 a,u32 b,u32 c) {
    assert(call_count<8);calls[call_count++]=(struct Call){t,a,b,c};
}
void func_800821F4(void* p,s16 anim,void* a) {
    record(0x800821f4,(u32)(uintptr_t)p,(u32)(s32)anim,(u32)(uintptr_t)a);
}
void FieldParticleActorStop(s32 actor,s32 mode) { record(0x800a98e8,actor,mode,0); }
void func_8009FEE4(s32 slot) { record(0x8009fee4,slot,0,0); }
void func_800A0524(s32 source,s32 destination) {
    record(0x800a0524,source,destination,0);
}
static u32 read_word(const u8* p,unsigned w) {
    u32 v=0;for(unsigned i=0;i<w;++i)v|=(u32)p[i]<<(8*i);return v;
}
static void write_word(u8* p,unsigned w,u32 v) {
    for(unsigned i=0;i<w;++i)p[i]=(u8)(v>>(8*i));
}
static u8* address(u32 a,unsigned w) {
    if(a>=0x80000000u && (uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);
    uintptr_t lo=(uintptr_t)&fixture;
    if(a>=lo && (uint64_t)a+w<=lo+sizeof(fixture))return (u8*)(uintptr_t)a;
    return NULL;
}
static int read_bus(void* u,u32 a,unsigned w,u32* v) {
    (void)u;u8* p=address(a,w);if(!p)return -1;*v=read_word(p,w);return 0;
}
static int write_bus(void* u,u32 a,unsigned w,u32 v) {
    (void)u;u8* p=address(a,w);if(!p)return -1;write_word(p,w,v);return 0;
}
static int bridge(void* u,PcPortMipsCpu* cpu,u32 target) {
    (void)u;
    if(target==0x800821f4)record(target,cpu->gpr[4],cpu->gpr[5],cpu->gpr[6]);
    else if(target==0x800a98e8)record(target,cpu->gpr[4],cpu->gpr[5],0);
    else if(target==0x8009fee4)record(target,cpu->gpr[4],0,0);
    else if(target==0x800a0524)record(target,cpu->gpr[4],cpu->gpr[5],0);
    else return 0;
    cpu->gpr[2]=0;return 1;
}
static void prepare(unsigned current,unsigned seed) {
    memset(&fixture,0xa5,sizeof(fixture));
    for(unsigned i=0;i<6;++i) {
        fixture.actors[i].pActorData=(u32)(uintptr_t)fixture.data[i];
        fixture.actors[i].pSpriteData=(u32)(uintptr_t)fixture.sprites[i];
        fixture.actors[i].status=(s16)(0x8140+i);
        fixture.data[i][0]=(0x5ac31fffu ^ (seed*0x31007)) & ~0x200u;
        for(unsigned j=0;j<3;++j) fixture.data[i][8+j]=0x12300000u+i*31+j;
        write_word((u8*)fixture.data[i]+0xe6,2,0xfff0+i);
        write_word((u8*)fixture.data[i]+0x106,2,0x7010+i);
        write_word((u8*)fixture.data[i]+0x108,2,0x8020+i);
#ifdef TEST_GEAR_EXIT
        fixture.actors[i].status=(s16)(0xffffu ^ (seed<<8));
        fixture.data[i][0] &= ~0x400u;
#endif
    }
    D_800AFD1C=current;
    write_word(ram+0xafb10,4,(u32)(uintptr_t)fixture.actors);
    write_word(ram+0x5a39c,4,(u32)(uintptr_t)fixture.state);
    write_word(ram+0xafd1c,4,current);
    for(unsigned i=0;i<3;++i) {
        write_word(ram+0x5a444+i*4,4,D_8005A444[i]);
        write_word(ram+0x6f990+i*4,4,D_8006F990[i]);
    }
    memset(calls,0,sizeof(calls));call_count=0;
}
int main(void) {
    if(!ENTRY) {fputs("GEAR BOARD RETAIL FAIL missing native owner\n",stderr);return 1;}
    assert(sizeof(FieldActor)==0x5c);
    FILE* f=fopen("disc/field.bin","rb");assert(f);
    assert(fread(ram+0x6faf0,1,260862,f)==260862);assert(fclose(f)==0);
    for(unsigned slot=0;slot<3;++slot) for(unsigned current=0;current<6;++current) {
        prepare(current,slot);
        PcPortMipsBus bus={.read=read_bus,.write=write_bus,.bridge=bridge};
        PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
        cpu.gpr[4]=slot;cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
        if(PcPortMipsRun(&cpu,RETAIL_ENTRY,0xfffffffcu,5000)!=PC_PORT_MIPS_HALTED) {
            fprintf(stderr,"GEAR BOARD RETAIL FAIL oracle: %s\n",cpu.error);return 1;
        }
        assert(call_count==EXPECT_CALLS);assert(fixture.state[0x22b1+slot]==EXPECT_RIDE);
        expected=fixture;memcpy(expected_calls,calls,sizeof(calls));
        prepare(current,slot);ENTRY(slot);
        if(memcmp(&fixture,&expected,sizeof(fixture)) || call_count!=EXPECT_CALLS ||
           memcmp(calls,expected_calls,sizeof(calls))) {
            fprintf(stderr,"GEAR BOARD RETAIL FAIL slot=%u current=%u calls=%u\n",slot,current,call_count);
            return 1;
        }
    }
    printf("GEAR RETAIL %08x PASS 18 cases: actor/sprite/state bytes and ordered call arguments\n",RETAIL_ENTRY);
    return 0;
}
