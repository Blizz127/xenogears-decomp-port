#include "battle_mips_adapter.h"
#include "battle_target_bounds.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
static uint8_t ram[0x200000];
static int32_t positions[11][3];
static PcPortBattleTargetPoint points[11];
static unsigned observed_count, memset_calls;
static uint32_t mask;
enum { FRAME=0x1FEEE0 };
static uint32_t get(unsigned a,unsigned w) {
    uint32_t v=0; for (unsigned i=0;i<w;i++) v|=(uint32_t)ram[a+i]<<(i*8); return v;
}
static void put(unsigned a,unsigned w,uint32_t v) {
    for (unsigned i=0;i<w;i++) ram[a+i]=(uint8_t)(v>>(i*8));
}
static int read_bus(void *o,uint32_t a,unsigned w,uint32_t *v) {
    (void)o; assert(a>=0x80000000u && a<=0x80200000u-w);
    *v=get(a-0x80000000u,w); return 0;
}
static int write_bus(void *o,uint32_t a,unsigned w,uint32_t v) {
    (void)o; unsigned at=a-0x80000000u;
    assert(w==4 && !(at&3));
    assert(at==0xC3678 || (at>=FRAME+0xF8 && at<=FRAME+0x11C) ||
        (at>=FRAME+0x10 && at<=FRAME+0x18));
    put(at,w,v); return 0;
}
static int bridge(void *o,PcPortMipsCpu *cpu,uint32_t target) {
    (void)o;
    if (target==0x800BC550) { observed_count=cpu->gpr[6]; return 0; }
    if (target!=0x8003FA08) return 0;
    assert(!memset_calls++ && cpu->gpr[4]==0x80000000u+FRAME+0x10);
    assert(cpu->gpr[5]==0 && cpu->gpr[6]==16);
    memset(ram+FRAME+0x10,0,16);
    for (unsigned i=2;i<=15;i++) cpu->gpr[i]=0xBAD00000+i;
    return 1;
}
static void run(uint32_t input,unsigned profile,unsigned dataset) {
    mask=input; observed_count=99; memset_calls=0;
    unsigned count=0;
    for (unsigned i=0;i<11;i++) {
        points[i].suppressed=profile==1 && i%3==0?255:0;
        points[i].position=profile==2 && i%2?NULL:positions[i];
        for (unsigned axis=0;axis<3;axis++) {
            int32_t v=0;
            if (dataset==0) v=(int32_t)(i*7+axis*3)-40;
            if (dataset==1) v=(i+axis)%2?INT32_MIN:INT32_MAX;
            if (dataset==2) v=((int32_t)i-5)*0x10001+(int32_t)axis;
            if (dataset==3) v=-1;
            positions[i][axis]=v;
            put(0x100000+i*16+axis*4,4,(uint32_t)v);
        }
        put(0xC3EB0+i*28+7,1,points[i].suppressed);
        put(0xCCB3C+i*4,4,points[i].position?0x80100000+i*16:0);
        if ((input&(1u<<i)) && !points[i].suppressed && points[i].position) count++;
    }
    put(0xC3678,4,0xDEADBEEF);
    memset(ram+FRAME,0xA5,0x120);
    PcPortMipsCpu cpu;
    PcPortMipsBus bus={.read=read_bus,.write=write_bus,.bridge=bridge};
    PcPortMipsCpuInit(&cpu,&bus);
    cpu.gpr[4]=mask; cpu.gpr[29]=0x801FF000; cpu.gpr[31]=0xFFFFFFFC;
    for (unsigned i=16;i<=23;i++) cpu.gpr[i]=0xCAFE0000+i;
    cpu.gpr[30]=0x12345678;
    assert(PcPortMipsRun(&cpu,0x800BC460,count?0x8004ABBCu:0xFFFFFFFCu,1500)==0);
    PcPortBattleTargetBounds result;
    int32_t saved_positions[11][3];
    PcPortBattleTargetPoint saved_points[11];
    memcpy(saved_positions,positions,sizeof(positions));
    memcpy(saved_points,points,sizeof(points));
    memset(&result,0x5A,sizeof(result));
    assert(PcPortBattleComputeTargetBounds(mask,points,&result)==0);
    assert(!memcmp(saved_positions,positions,sizeof(positions)) && !memcmp(saved_points,points,sizeof(points)));
    assert(result.count==count && observed_count==count && memset_calls==1);
    assert(get(0xC3678,4)==mask);
    for (unsigned axis=0;axis<3;axis++) assert((uint32_t)result.center[axis]==get(FRAME+0x10+axis*4,4));
    assert(get(FRAME+0x1C,4)==0);
    if (count) {
        assert(cpu.gpr[29]==0x80000000u+FRAME && cpu.gpr[31]==0x800BC6E4u);
        assert(cpu.gpr[4]==0x800C3740u && cpu.gpr[5]==0x80000000u+FRAME+0x30);
    } else {
        assert(cpu.gpr[29]==0x801FF000u && cpu.gpr[31]==0xFFFFFFFCu && cpu.gpr[30]==0x12345678u);
        for (unsigned i=16;i<=23;i++) assert(cpu.gpr[i]==0xCAFE0000+i);
    }
}
int main(void) {
    FILE *f=fopen("disc/battle.bin","rb"); assert(f);
    assert(!fseek(f,0x4C970,SEEK_SET));
    assert(fread(ram+0xBC460,1,0x644,f)==0x644); fclose(f);
    const uint32_t upper[]={0,0xFFFFF800u,0x80000800u};
    unsigned cases=0;
    for (unsigned low=0;low<2048;low++) for (unsigned u=0;u<3;u++)
    for (unsigned profile=0;profile<3;profile++) for (unsigned dataset=0;dataset<4;dataset++) {
        run(low|upper[u],profile,dataset); cases++;
    }
    assert(cases==73728);
    PcPortBattleTargetBounds result;
    assert(PcPortBattleComputeTargetBounds(0,NULL,&result)==-1);
    assert(PcPortBattleComputeTargetBounds(0,points,NULL)==-1);
    puts("TARGET BOUNDS native/retail PASS 73728 cases; stops before geometry, simulated memset");
}
