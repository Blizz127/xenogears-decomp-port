/* Actual BC2F0 + its BBEE0 callback. Main-executable unlink/free bodies mocked. */
#include "battle_mips_adapter.h"
#ifdef XBT_CLEANUP_NATIVE
#include "battle_target_setup.h"
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t ram[0x200000];
static unsigned roots, gate, owned, initial_count, return_mode;
static unsigned freed, phase, callback_entries, setup_entries;
static unsigned nested_callbacks;
static unsigned stack_writes;
static uint32_t current_task, callback_sp, vector_snapshot[6];
#ifdef XBT_CLEANUP_NATIVE
static unsigned native_active;
static uint8_t expected_ram[sizeof(ram)];
static int guest_invoke(void *opaque,PcPortMipsCpu *cpu,uint32_t target) {
    (void)opaque;
    PcPortMipsCpu child=*cpu;
    int result=PcPortMipsRun(&child,target,cpu->gpr[31],1000);
    memcpy(cpu->gpr,child.gpr,sizeof(cpu->gpr));
    return result? -1:0;
}
#endif
static uint32_t get(unsigned at,unsigned width) {
    uint32_t v=0;
    for (unsigned i=0;i<width;i++) v|=(uint32_t)ram[at+i]<<(8*i);
    return v;
}
static void put(unsigned at,unsigned width,uint32_t v) {
    for (unsigned i=0;i<width;i++) ram[at+i]=(uint8_t)(v>>(8*i));
}
static int read_bus(void *o,uint32_t a,unsigned w,uint32_t *v) {
    (void)o; assert(a>=0x80000000u && a<=0x80200000u-w);
    *v=get(a-0x80000000u,w); return 0;
}
static int write_bus(void *o,uint32_t a,unsigned w,uint32_t v) {
    (void)o; assert(a>=0x80000000u && a<=0x80200000u-w);
    unsigned at=a-0x80000000u;
    unsigned stack=at==0x1FEFF8 || at==0x1FEFFC || at==0x1FEFE0 || at==0x1FEFE4 ||
        at==0x1FEFC8 || at==0x1FEFCC || at==0x1FEFB0 || at==0x1FEFB4;
    if (stack) stack_writes++;
    assert((stack && w==4) ||
        (at>=0xC3680 && at<=0xC3684 && w==4) ||
        ((at==0xC3CC0 || at==0xC3CBC) && w==4) ||
        (at==0xC3CC4 && w==1) ||
        (at>=0xD30A0 && at<=0xD30AC && w==2) ||
        (at>=0x6F99C && at<=0x6F9B4 && w==4));
    put(at,w,v); return 0;
}
static int bridge(void *o,PcPortMipsCpu *cpu,uint32_t target) {
    (void)o;
    if (target==0x800BC2F0) {
        assert(cpu->gpr[4]==(setup_entries?return_mode:1));
        if (setup_entries) {
            assert(phase==0 && freed==initial_count);
            assert(get(0xC3CC4,1)==0);
            assert(cpu->gpr[29]==callback_sp);
            for (unsigned i=0;i<6;i++)
                vector_snapshot[i]=get(0xD30A0+(i/3)*8+(i%3)*2,2)<<16;
        }
        setup_entries++;
        assert(setup_entries<=2);
#ifdef XBT_CLEANUP_NATIVE
        if (native_active) {
            assert(PcPortBattleTargetSetupFrame(cpu,guest_invoke,NULL)==0);
            return 1;
        }
#endif
        return 0;
    }
    if (target==0x800BBEE0) {
        assert(!phase);
        unsigned index=(roots&1) && !freed ? 0:1;
        current_task=0x80100000+index*0x100;
        assert(cpu->gpr[4]==current_task);
        assert(get(0xC3680+index*4,4)==current_task);
        callback_sp=cpu->gpr[29]-0x18;
        unsigned nested=setup_entries==2 && (return_mode==0 || return_mode==1);
        assert(callback_sp==(nested?0x801FEFA0u:0x801FEFD0u));
        nested_callbacks+=nested;
        callback_entries++;
        return 0;
    }
    if (target!=0x8001CE74 && target!=0x8001CD94 && target!=0x8001CB48 && target!=0x800320E8)
        return 0;
    unsigned index=(current_task-0x80100000)/0x100;
    const uint32_t order[]={0x8001CE74,0x8001CD94,0x8001CB48,0x800320E8};
    if (!phase && !owned) phase=1;
    assert(phase<4 && target==order[phase]);
    assert(cpu->gpr[29]==callback_sp);
    assert(cpu->gpr[4]==current_task+(phase==2?0x1Cu:0));
    assert(get(0xC3680+index*4,4)==0);
    for (unsigned i=0;i<3;i++)
        assert(get(0xD30A0+index*8+i*2,2)==(gate?0x1234u+i:0x8000u+index*16+i));
    phase++;
    if (phase==4) { phase=0; freed++; }
    for (unsigned r=2;r<=15;r++) cpu->gpr[r]=0xBAD00000+r;
    return 1;
}
static void run(unsigned r,unsigned g,unsigned own,unsigned count,unsigned mode) {
    roots=r; gate=g; owned=own; initial_count=count; return_mode=mode;
    freed=phase=callback_entries=setup_entries=stack_writes=0;
    for (unsigned i=0;i<2;i++) {
        unsigned task=0x100000+i*0x100;
        memset(ram+task,0xC3,0x100);
        put(task+4,4,0x80000000u+task+0x38);
        put(task+12,4,0x800BBEE0);
        put(task+0x78,4,(10+i)<<13);
        put(task+0xE4,4,own<<5);
        put(0xC3680+i*4,4,(r&(1u<<i))?0x80000000u+task:0);
        for (unsigned j=0;j<3;j++) {
            put(0xC3CCC+i*8+j*2,2,0x8000+i*16+j);
            put(0xD30A0+i*8+j*2,2,0x1234+j);
        }
    }
    put(0xC37C8,1,g); put(0xC3CC4,1,count); put(0xC367C,4,mode);
    put(0xC3CC0,4,0xDEADBEEF); put(0xC3CBC,4,0xABCD1234);
    memset(ram+0x6F99C,0xA5,32);
    memset(ram+0x1FEF80,0xA5,0x80);
    PcPortMipsCpu cpu;
    PcPortMipsBus bus={.read=read_bus,.write=write_bus,.bridge=bridge};
    PcPortMipsCpuInit(&cpu,&bus);
    cpu.gpr[4]=1; cpu.gpr[29]=0x801FF000; cpu.gpr[31]=0xFFFFFFFC;
    for (unsigned i=16;i<=23;i++) cpu.gpr[i]=0xCAFE0000+i;
    assert(PcPortMipsRun(&cpu,0x800BC2F0,0xFFFFFFFC,1000)==0);
    unsigned n=!!(r&1)+!!(r&2);
    unsigned reentered=count>=1 && count<=n;
    assert(stack_writes==2+2*n+2*reentered);
    for (unsigned at=0x1FEF80;at<0x1FF000;at++) {
        unsigned used=at>=0x1FEFF8 || (n && at>=0x1FEFE0 && at<0x1FEFE8) ||
            (reentered && at>=0x1FEFC8 && at<0x1FEFD0) ||
            (r==3 && count==1 && mode<2 && at>=0x1FEFB0 && at<0x1FEFB8);
        if (!used) assert(ram[at]==0xA5);
    }
    assert(freed==n && callback_entries==n && setup_entries==1+reentered && !phase);
    assert(get(0xC3CC4,1)==((initial_count-n)&255));
    assert(get(0xC3680,4)==0 && get(0xC3684,4)==0);
    assert(get(0xC3CC0,4)==(reentered?mode:1));
    assert(get(0xC3CBC,4)==(reentered && mode==4?5u:1u));
    for (unsigned i=0;i<6;i++)
        assert(get(0x6F99C+(i/3)*16+(i%3)*4,4)==
            (reentered && mode==2?vector_snapshot[i]:0xA5A5A5A5u));
    assert(get(0x6F9A8,4)==0xA5A5A5A5u && get(0x6F9B8,4)==0xA5A5A5A5u);
    assert(cpu.gpr[29]==0x801FF000 && cpu.gpr[31]==0xFFFFFFFC);
    for (unsigned i=16;i<=23;i++) assert(cpu.gpr[i]==0xCAFE0000+i);
}
int main(void) {
    FILE *f=fopen("disc/battle.bin","rb"); assert(f);
    assert(!fseek(f,0x4C3F0,SEEK_SET));
    assert(fread(ram+0xBBEE0,1,0x518,f)==0x518); fclose(f);
#ifdef XBT_CLEANUP_NO_REENTRY
    put(0xBBFFC,4,0);
#endif
#ifdef XBT_CLEANUP_NO_ROOT_CLEAR
    put(0xBBF30,4,0); put(0xBBF74,4,0);
#endif
#ifdef XBT_CLEANUP_EARLY_REENTRY
    /* Replace HeapFree call with setup(2), before free/count decrement. */
    put(0xBBFD0,4,0x0C02F0BC); put(0xBBFD4,4,0x24040002);
    run(1,0,0,1,2);
#endif
    const unsigned gates[]={0,1,255}, modes[]={0,1,2,4};
    unsigned cases=0;
    for (unsigned r=0;r<4;r++) for (unsigned g=0;g<3;g++)
    for (unsigned own=0;own<2;own++) for (unsigned count=0;count<256;count++)
    for (unsigned mode=0;mode<4;mode++) {
        run(r,gates[g],own,count,modes[mode]); cases++;
#ifdef XBT_CLEANUP_NATIVE
        memcpy(expected_ram,ram,sizeof(ram));
        native_active=1;
        run(r,gates[g],own,count,modes[mode]); cases++;
        assert(!memcmp(expected_ram,ram,sizeof(ram)));
        native_active=0;
#endif
    }
#ifdef XBT_CLEANUP_NATIVE
    assert(cases==49152 && nested_callbacks==24);
#else
    assert(cases==24576);
    assert(nested_callbacks==12);
#endif
    printf("TARGET CLEANUP PASS %u executions; native=%u, real free callback/reentry, simulated unlink/heap\n",cases,
#ifdef XBT_CLEANUP_NATIVE
           1u
#else
           0u
#endif
    );
}
