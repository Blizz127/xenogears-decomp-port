#define _GNU_SOURCE
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "common.h"
#include "guest_prim_link.h"
#include "psx_memory.h"
#include "battle_mips_adapter.h"

/* Actual temp2.c leaf and actual domain-link source, no native service spies.
 * Original nine instructions are a test-only oracle. The comparison covers
 * the entire 2 MiB RAM arena plus a 1 MiB guard. Inputs are aligned mapped
 * words; arbitrary invalid host pointers and CPU-register bridging are outside
 * this C API contract. Mixed guest-OT/native-primitive rejection is separately
 * tested as host policy, with no corresponding original-CPU address domain.
 */
#define BASE 0x80000000u
#define RAM_BYTES 0x200000u
#define HALT 0xfffffffcu
_Alignas(16) uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];
static uint8_t oracle[PSX_RAM_SIZE], *native_arena;
static unsigned total,failures,policy_cases;
static uint64_t steps;
extern void func_80031798(void *,void *);

static uint32_t word(const uint8_t *p)
{
    return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}
static void put(uint8_t *p,uint32_t v)
{
    for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(i*8));
}
static int read_bus(void *o,uint32_t a,unsigned w,uint32_t *v)
{
    (void)o;uint32_t p=a&0x1fffffffu;
    if(w!=4 || (p&3u) || p>RAM_BYTES-4u)return -1;
    *v=word(oracle+p);return 0;
}
static int write_bus(void *o,uint32_t a,unsigned w,uint32_t v)
{
    (void)o;uint32_t p=a&0x1fffffffu;
    if(w!=4 || (p&3u) || p>RAM_BYTES-4u)return -1;
    put(oracle+p,v);return 0;
}
static void run_case(unsigned domain,unsigned alias,uint32_t tag,uint32_t low,unsigned edge)
{
    uint32_t ot=edge?RAM_BYTES-4u:0x100040u;
    uint32_t prim=alias?ot:edge?RAM_BYTES-12u:0x101080u;
    uint32_t old=(tag<<24)|(low&0xffffffu);
    put(oracle+prim,0xa1555566u);put(oracle+ot,old);
    uint8_t *host=domain?g_PsxRam:native_arena;
    memcpy(host,oracle,sizeof(oracle));
    PcPortMipsCpu cpu;PcPortMipsBus bus={.read=read_bus,.write=write_bus};
    PcPortMipsCpuInit(&cpu,&bus);
    cpu.gpr[4]=BASE+ot;cpu.gpr[5]=BASE+prim;cpu.gpr[31]=HALT;
    setenv("XENO_DISABLE_CPU_INTERPRETER","0",1);
    int rc=PcPortMipsRun(&cpu,0x80031798u,HALT,32);
    assert(rc==PC_PORT_MIPS_HALTED && cpu.steps==9);steps+=cpu.steps;
    /* Independent expected words, including the original write order when
     * head and primitive are the same word. */
    assert(word(oracle+prim)==(old|0x04000000u));
    assert(word(oracle+ot)==(alias?(old|0x04000000u):prim));
    setenv("XENO_DISABLE_CPU_INTERPRETER","1",1);
    assert(PcPortMipsInterpreterDisabled());
    PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
    PcPort_PrimLinkReset();
    func_80031798(host+ot,host+prim);
    PcPortMipsExecutionStats after=PcPortMipsExecutionStatsGet();
    assert(!memcmp(&before,&after,sizeof(before)));
    int bad=memcmp(oracle,host,sizeof(oracle))!=0;
    bad|=PcPort_PrimLinkGuestCount()!=(int)domain;
    bad|=PcPort_PrimLinkNativeCount()!=(int)!domain;
    bad|=PcPort_PrimLinkRejectCount()!=0;
    total++;
    if(bad) {
        failures++;
        if(failures<=8)printf("TAGGED LINK DIFF domain=%s alias=%u tag=%02x low=%06x edge=%u head=%08x/%08x prim=%08x/%08x\n",
            domain?"guest":"native",alias,tag,low,edge,word(host+ot),word(oracle+ot),word(host+prim),word(oracle+prim));
    }
}
static void rejection_case(void)
{
    uint32_t *ot=(uint32_t *)(void *)(g_PsxRam+0x1000);
    uint32_t *prim=(uint32_t *)(void *)(native_arena+0x2000);
    *ot=0xa1234567u;*prim=0xb7654321u;
    PcPort_PrimLinkReset();
    func_80031798(ot,prim);
    int bad=*ot!=0xa1234567u || *prim!=0xb7654321u ||
        PcPort_PrimLinkRejectCount()!=1 || PcPort_PrimLinkGuestCount()!=0 ||
        PcPort_PrimLinkNativeCount()!=0;
    policy_cases++;
    if(bad){failures++;puts("TAGGED LINK DIFF mixed-domain rejection modified input");}
}
int main(int argc,char **argv)
{
    assert(argc==2 || argc==3);
    native_arena=mmap((void*)(uintptr_t)BASE,PSX_RAM_SIZE,PROT_READ|PROT_WRITE,
        MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0);
    if(native_arena==MAP_FAILED){perror("exclusive native arena");return 2;}
    assert((uintptr_t)native_arena==BASE);
    assert((uintptr_t)g_PsxRam!=BASE);
    memset(oracle,0x5a,sizeof(oracle));
    FILE *f=fopen(argv[1],"rb");assert(f);
    assert(fseek(f,0x21f98,SEEK_SET)==0);
    assert(fread(oracle+0x31798,1,36,f)==36);assert(fclose(f)==0);
    setenv("XENO_CPU_EXECUTION_STATS","1",1);PcPortMipsExecutionStatsInit();
    if(argc==3) {
        for(unsigned d=0;d<2;d++){
            run_case(d,0,0,0x123456,0);
            run_case(d,0,0xa3,0x123456,0);
            run_case(d,1,0,0x123456,0);
        }
    } else {
        for(unsigned tag=0;tag<256;tag++)for(unsigned d=0;d<2;d++)for(unsigned alias=0;alias<2;alias++)
            run_case(d,alias,tag,((tag*0x1234u)^0x00abcdefu)&0xffffffu,0);
        const uint32_t lows[]={0,4,0x1ffffc,0x800000,0xfffffc,0xffffff};
        for(unsigned i=0;i<6;i++)for(unsigned d=0;d<2;d++)for(unsigned alias=0;alias<2;alias++)
            run_case(d,alias,i?0xff:0,lows[i],1);
        rejection_case();
    }
    printf("TAGGED LINK %s cases=%u policy_cases=%u failures=%u original_instructions=9/9 oracle_steps=%llu native_adapter_delta=0 native_service_spies=0\n",
        failures?"FAIL":"PASS",total,policy_cases,failures,(unsigned long long)steps);
    assert(munmap(native_arena,PSX_RAM_SIZE)==0);
    return failures?1:0;
}
