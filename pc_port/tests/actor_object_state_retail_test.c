#include <stdio.h>
#include <string.h>
#ifndef ACTOR_RENDER_SOURCE
#define ACTOR_RENDER_SOURCE "../../src/field/main/misc2.c"
#endif
#include ACTOR_RENDER_SOURCE
#include "battle_mips_adapter.h"
s32 D_8004F380;
u32 D_801E8670[10];
extern u8 g_FieldBss_800B20A8[];
static u8 ram[0x200000];
static struct {
    u32 actor[0x150/4],objects[10][0xb4/4],nodes[10][0x80/4];
    u32 loopActors[3][0x5c/4],loopStates[3][0x150/4];
} fixture,expected;
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
int main(void) {
    FILE* f=fopen("disc/field.bin","rb");assert(f);
    assert(fread(ram+0x6faf0,1,260862,f)==260862);assert(!fclose(f));
    static const s16 values[]={-32768,-1,0,1,4096,32767};
    static const u32 scales[]={0,1,0xffffffffu,0x1000,0x7fffffffu,0x80000000u};
    unsigned cases=0;
    for(unsigned globalSkip=0;globalSkip<2;++globalSkip)
    for(unsigned slot=0;slot<10;++slot)for(unsigned bits=0;bits<64;++bits)
    for(unsigned value=0;value<6;++value) {
        memset(&fixture,0xa5,sizeof(fixture));
        memset(g_FieldBss_800B20A8,0x36,0x424);
        u8* actor=(u8*)fixture.actor;u32 status=(bits&32)?0x60:0x40;
        fixture.actor[0]=(bits&1)?0x10000:0;
        fixture.actor[1]=0x2000|0x200|((bits&2)?0x800:0)|((bits&4)?0x20000:0);
        fixture.actor[5]=((bits&8)?0x200000:0)|((bits&16)?2:0);
        *(s16*)(actor+0xf4)=values[value];
        *(u16*)(actor+0x108)=(u16)values[(value+1)%6];
        *(s16*)(actor+0x22)=values[(value+2)%6];
        *(s16*)(actor+0x26)=values[(value+3)%6];
        *(s16*)(actor+0x2a)=values[(value+4)%6];
        for(unsigned j=0;j<10;++j) {
            fixture.objects[j][1]=(u32)(uintptr_t)fixture.nodes[j];
            *(u16*)((u8*)fixture.nodes[j]+0x56)=(u16)(j*0x1001+values[value]);
            D_801E8670[j]=globalSkip?0:(u32)(uintptr_t)fixture.objects[j];
            /* The packed block starts at retail 0x800B2078 (g_FieldEffects),
             * so the D_800B220C scale table is at +0x194, not +0x164 (+0x164
             * is the 0x800B21DC spriteId table). Map the whole block at its
             * true base so the oracle reads the filled scales. */
            *(u32*)(g_FieldBss_800B20A8+0x194+j*4)=scales[(value+j)%6];
        }
        D_8004F380=globalSkip;
        memcpy(ram+0x1e8670,D_801E8670,40);
        memcpy(ram+0xb2078,g_FieldBss_800B20A8,0x424);
        assert(!wr(NULL,0x8004f380,4,globalSkip));
        PcPortMipsBus bus={.read=rd,.write=wr};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
        cpu.gpr[17]=(u32)(uintptr_t)actor;cpu.gpr[19]=0x801e8670+slot*4;
        cpu.gpr[23]=slot*4;cpu.gpr[8]=status;
        typeof(fixture) initial=fixture;
        if(PcPortMipsRun(&cpu,0x80076300,0x80076468,5000)!=PC_PORT_MIPS_HALTED) {
            fprintf(stderr,"OBJECT STATE FAIL oracle %s\n",cpu.error);return 1;
        }
        expected=fixture;fixture=initial;
        u32 next=FieldUpdateObjectActor((u8*)fixture.actor,status,slot);
        if(memcmp(&fixture,&expected,sizeof(fixture)) || next*4!=cpu.gpr[23] ||
           0x801e8670+next*4!=cpu.gpr[19] ||
           memcmp(ram+0xb2078,g_FieldBss_800B20A8,0x424) ||
           memcmp(ram+0x1e8670,D_801E8670,40)) {
            fprintf(stderr,"OBJECT STATE FAIL skip=%u slot=%u flags=%u value=%u next=%u/%u\n",
                    globalSkip,slot,bits,value,next,cpu.gpr[23]/4);return 1;
        }
        ++cases;
    }
    printf("OBJECT STATE PASS %u cases: flags/rotation/scale/position/slot cursor/guards\n",cases);
}
