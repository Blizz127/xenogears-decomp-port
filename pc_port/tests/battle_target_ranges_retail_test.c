/* Retail contracts for84548/84750. No native body or callee replacements. */
#if defined(XBT_EXPECT_CURSOR_FAULT) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif
#include "battle_mips_adapter.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#ifdef XBT_EXPECT_CURSOR_FAULT
#include <signal.h>
#include <ucontext.h>
#include <unistd.h>
#ifndef __x86_64__
#error Cursor fault instruction control requires Linux x86-64
#endif
#endif
static uint8_t ram[0x200000];
struct Write { unsigned address, width, value; };
static struct Write writes[64];
static unsigned nwrites, cases;
#ifdef XBT_NATIVE_ACTOR_RANGE
uint8_t D_800D2DCC[256], D_800C3EB7[256][28], D_800D32A1[256][8];
uint8_t D_800C3EB4[256][28], *D_800D3364;
uint16_t D_800CCD64[256][184], D_800CCE08[256][184];
uint8_t D_800D3274, D_800C3E90[12];
uint16_t D_800C3D64, D_800C3448[256];
static uint8_t native_matrix[0x340];
extern uint32_t func_80084750(uint32_t);
#ifdef XBT_EXPECT_CURSOR_FAULT
static uintptr_t expected_fault_address;
static uintptr_t expected_fault_pc;
static volatile sig_atomic_t cursor_armed;
static void cursor_fault(int signal_number, siginfo_t *info, void *context) {
    const ucontext_t *state=context;
    if (cursor_armed && signal_number==SIGSEGV && info->si_code>0 &&
        (uintptr_t)info->si_addr==expected_fault_address &&
        (uintptr_t)state->uc_mcontext.gregs[REG_RIP]==expected_fault_pc &&
        (state->uc_mcontext.gregs[REG_ERR]&0x12)==2) {
        static const char message[]="CURSOR WIDTH expected truncated-address store rejected\n";
        (void)write(STDOUT_FILENO,message,sizeof(message)-1);
        _exit(79);
    }
    _exit(80);
}
#endif
#endif
static uint32_t get(unsigned at, unsigned width) {
    uint32_t v=0; for (unsigned i=0; i<width; i++) v|=(uint32_t)ram[at+i]<<(8*i); return v;
}
static void put(unsigned at, unsigned width, uint32_t v) {
    for (unsigned i=0; i<width; i++) ram[at+i]=v>>(8*i);
}
static int read_bus(void *p, uint32_t address, unsigned width, uint32_t *v) {
    (void)p; unsigned at=address&0x1FFFFFFF;
    if ((uint64_t)at+width>sizeof(ram)) return -1;
    *v=get(at,width); return 0;
}
static int write_bus(void *p, uint32_t address, unsigned width, uint32_t v) {
    (void)p; unsigned at=address&0x1FFFFFFF;
    if ((uint64_t)at+width>sizeof(ram)) return -1;
    if (!(at>=0x1FEFC8 && at<0x1FF000)) {
        assert(nwrites<64);
        writes[nwrites++]=(struct Write){at,width,v& (width==1 ? 255u : 65535u)};
    }
    put(at,width,v); return 0;
}
static void expect_write(unsigned i, unsigned address, unsigned width, unsigned value) {
    assert(i<nwrites && writes[i].address==address && writes[i].width==width && writes[i].value==value);
}
static unsigned run_at(uint32_t entry, unsigned mode, unsigned flag, unsigned order, unsigned inherited_count) {
    PcPortMipsBus bus={.read=read_bus,.write=write_bus};
    PcPortMipsCpu cpu; PcPortMipsCpuInit(&cpu,&bus);
    for (unsigned r=16; r<=23; r++) cpu.gpr[r]=0x12340000u+r;
    cpu.gpr[4]=mode; cpu.gpr[5]=flag; cpu.gpr[6]=order;
    cpu.gpr[7]=3; cpu.gpr[20]=inherited_count;
    cpu.gpr[29]=0x801FF000; cpu.gpr[31]=0xFFFFFFFC; nwrites=0;
    assert(PcPortMipsRun(&cpu,entry,0xFFFFFFFC,10000)==0);
    assert(cpu.gpr[29]==0x801FF000 && cpu.gpr[31]==0xFFFFFFFC);
    assert(cpu.gpr[20]==inherited_count);
    for (unsigned r=16; r<=23; r++)
        if (r!=20) assert(cpu.gpr[r]==0x12340000u+r);
    assert(cpu.gpr[2]==ram[0xC3E90]);
    return ram[0xD3274];
}
static unsigned run(unsigned mode, unsigned flag, unsigned order, unsigned inherited_count) {
    return run_at(0x80084548,mode,flag,order,inherited_count);
}
static void test_actor_range(void) {
    unsigned actor_cases=0;
    const unsigned actors[]={0,2,3,255,0xABCDE001u};
    for (unsigned ai=0; ai<5; ai++) for (unsigned bits=0; bits<256; bits++)
    for (unsigned mode=0; mode<8; mode++) {
        unsigned actor=actors[ai]&255, count=0, mask=0;
        uint8_t expected[12]; memset(expected,255,sizeof(expected));
        /* Distinct groups exercise actor/target matrix orientation. */
        for (unsigned i=0; i<256; i++) ram[0xC3EB4+i*28]=(i*3+1)%8;
        put(0xD3364,4,0x80120000u);
        for (unsigned a=0; a<8; a++) for (unsigned t=0; t<8; t++)
            ram[0x120140+a*64+t*8]=((a+2*t+mode)%3)==0;
        for (unsigned i=0; i<11; i++) {
            unsigned present=i<3 ? 1 : (bits>>(i-3))&1;
            unsigned blocked=(mode&1) && i%3==0;
            unsigned alternate=(mode&2) && (i&1);
            unsigned status=(mode&4) && i%3==1 ? 0x8000 : 0;
            ram[0xD2DCC+i]=present;
            ram[0xC3EB7+i*28]=blocked;
            ram[0xD32A1+i*8]=alternate;
            put(0xCCD64+i*0x170,2,alternate ? 1 : status);
            put(0xCCE08+i*0x170,2,alternate ? status : 1);
            put(0xC3448+i*2,2,(0x8001u>>i)|(1u<<i));
            unsigned matrix=ram[0x120140+ram[0xC3EB4+actor*28]*64+
                                ram[0xC3EB4+i*28]*8];
            if (i>=3 && present && !blocked && !status && (alternate || !matrix))
                expected[count++]=i;
        }
        ram[0xD3274]=0xA5; put(0xC3D64,2,0x5AA5);
#ifdef XBT_NATIVE_ACTOR_RANGE
        for (unsigned i=0; i<256; i++) {
            D_800D2DCC[i]=ram[0xD2DCC+i];
            D_800C3EB7[i][0]=ram[0xC3EB7+i*28];
            D_800D32A1[i][0]=ram[0xD32A1+i*8];
            D_800C3EB4[i][0]=ram[0xC3EB4+i*28];
            D_800CCD64[i][0]=get(0xCCD64+i*0x170,2);
            D_800CCE08[i][0]=get(0xCCE08+i*0x170,2);
            D_800C3448[i]=get(0xC3448+i*2,2);
        }
        memcpy(native_matrix,ram+0x120000,sizeof(native_matrix));
        D_800D3364=native_matrix;
        D_800D3274=0xA5; D_800C3D64=0x5AA5;
        memset(D_800C3E90,0xA5,sizeof(D_800C3E90));
#endif
        assert(run_at(0x80084750,actors[ai],0xFEDCBA98u,0x87654321u,0x1357)==count);
        assert(memcmp(ram+0xC3E90,expected,12)==0);
        for (unsigned i=0; i<12; i++) expect_write(i,0xC3E9B-i,1,255);
        expect_write(12,0xD3274,1,0); expect_write(13,0xC3D64,2,0);
        for (unsigned i=0; i<count; i++) {
            mask|=get(0xC3448+expected[i]*2,2);
            expect_write(14+i*3,0xC3E90+i,1,expected[i]);
            expect_write(15+i*3,0xC3D64,2,mask);
            expect_write(16+i*3,0xD3274,1,i+1);
        }
        assert(nwrites==14+count*3 && get(0xC3D64,2)==mask);
#ifdef XBT_NATIVE_ACTOR_RANGE
#ifdef XBT_EXPECT_CURSOR_FAULT
        cursor_armed=1;
#endif
        uint32_t native_result=func_80084750(actors[ai]);
#ifdef XBT_EXPECT_CURSOR_FAULT
        cursor_armed=0;
#endif
        assert(native_result==ram[0xC3E90]);
        assert(D_800D3274==count && D_800C3D64==mask);
        assert(memcmp(D_800C3E90,expected,12)==0);
#endif
        actor_cases++;
    }
    printf("TARGET ACTOR RANGE retail contract PASS %u cases\n",actor_cases);
#ifdef XBT_NATIVE_ACTOR_RANGE
    printf("TARGET ACTOR RANGE native differential PASS %u cases\n",actor_cases);
#endif
}
int main(void) {
#ifdef XBT_REQUIRE_HIGH_LIST
    assert(sizeof(uintptr_t)>4 && (uintptr_t)D_800C3E90>UINT32_MAX);
#endif
#ifdef XBT_EXPECT_CURSOR_FAULT
    struct sigaction action={0};
    action.sa_sigaction=cursor_fault;
    action.sa_flags=SA_SIGINFO;
    sigemptyset(&action.sa_mask);
    expected_fault_address=(uint32_t)(uintptr_t)&D_800C3E90[11];
    expected_fault_pc=(uintptr_t)func_80084750+0x20;
    /* Pinned control instruction: movb $0xff,(%eax), not production code. */
    assert(memcmp((const void *)expected_fault_pc,"\x67\xc6\x00\xff",4)==0);
    assert(sigaction(SIGSEGV,&action,NULL)==0);
#endif
    FILE *f=fopen("disc/battle.bin","rb"); assert(f);
    const unsigned offsets[]={0x14A58,0x14618,0x1A118,0x14C60,0x14504};
    const unsigned addresses[]={0x84548,0x84108,0x89C08,0x84750,0x83FF4};
    const unsigned sizes[]={0x208,0xD8,0x1C,0x104,0x114};
    for (unsigned i=0; i<5; i++) {
        assert(!fseek(f,offsets[i],SEEK_SET));
        assert(fread(ram+addresses[i],1,sizes[i],f)==sizes[i]);
    }
    fclose(f);
    const unsigned flags[]={0,1,0x100,0x101};
    for (unsigned mode=0; mode<3; mode++) for (unsigned order=0; order<2; order++)
    for (unsigned bits=0; bits<2048; bits++) for (unsigned fi=0; fi<4; fi++) {
        uint8_t expected[12]; memset(expected,255,sizeof(expected));
        unsigned count=0, mask=0, status[11];
        for (unsigned i=0; i<11; i++) {
            ram[0xD2DCC+i]=(bits>>i)&1;
            ram[0xC3EB7+i*28]=0;
            ram[0xD32A1+i*8]=i&1;
            status[i]=(i%3)==0 ? 0x8000 : 0;
            put(0xCCD64+i*0x170,2,(i&1) ? 1 : status[i]);
            put(0xCCE08+i*0x170,2,(i&1) ? status[i] : 1);
            put(0xC3448+i*2,2,(0x8001u>>i)|(1u<<i));
        }
        unsigned first=mode==0 || (mode==2 && !order) ? 3 : 0;
        unsigned length=first ? 8 : 3;
        unsigned second=first ? 0 : 3;
        for (unsigned pass=0; pass<(mode==2 ? 2u : 1u); pass++) {
            unsigned start=pass ? second : first, n=pass ? 11-length : length;
            for (unsigned i=start; i<start+n; i++)
                if (((bits>>i)&1) && ((flags[fi]&255) || !(status[i]&0xC001)))
                    expected[count++]=i;
        }
        ram[0xD3274]=0xA5; put(0xC3D64,2,0x5AA5);
        assert(run(mode|0xABCDE000u,flags[fi],order|0x100u,0x1357)==count);
        assert(memcmp(ram+0xC3E90,expected,12)==0);
        for (unsigned i=0; i<12; i++) expect_write(i,0xC3E9B-i,1,255);
        expect_write(12,0xD3274,1,0); expect_write(13,0xC3D64,2,0);
        for (unsigned i=0; i<count; i++) {
            mask|=get(0xC3448+expected[i]*2,2);
            expect_write(14+i*3,0xC3E90+i,1,expected[i]);
            expect_write(15+i*3,0xC3D64,2,mask);
            expect_write(16+i*3,0xD3274,1,i+1);
        }
        assert(nwrites==14+count*3 && get(0xC3D64,2)==mask); cases++;
    }
    memset(ram+0xD2DCC,1,11);
    assert(run(3,1,0,0)==0);
    assert(run(3,1,0,1)==1 && ram[0xC3E90]==3);
    printf("TARGET RANGES retail contract PASS %u supported cases\n",cases);
    puts("TARGET RANGES mode3 inherited-register dependence PROVEN (counts0/1)");
    test_actor_range();
#ifdef XBT_EXPECT_CURSOR_FAULT
    assert(!"cursor narrowing mutant unexpectedly survived");
#endif
}
