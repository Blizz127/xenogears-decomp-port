#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include "battle_mips_adapter.h"

typedef int32_t (*Callback)(int32_t,int32_t,int32_t);
extern int32_t func_801E0850(int32_t,int32_t,int32_t) __attribute__((weak));
extern int32_t func_801E08D4(int32_t,int32_t,int32_t) __attribute__((weak));
extern int32_t func_801E0938(int32_t,int32_t,int32_t) __attribute__((weak));
extern int32_t func_801E0988(int32_t,int32_t,int32_t) __attribute__((weak));
extern uint32_t func_801E34BC(int32_t) __attribute__((weak));
static uint8_t ram[0x200000];
static unsigned trigCalls;
static int trigArg, trigValue;
/* The game header maps retail rsin/8003F8CC to the host rcos symbol. */
int rcos(int angle) { ++trigCalls; trigArg=angle; return trigValue; }
static int rd(void *u,uint32_t a,unsigned w,uint32_t *v) {
    (void)u;if(a<0x80000000u||(uint64_t)a+w>0x80200000u)return -1;
    *v=0;for(unsigned i=0;i<w;++i)*v|=(uint32_t)ram[(a&0x1fffff)+i]<<(8*i);return 0;
}
static int wr(void *u,uint32_t a,unsigned w,uint32_t v) {
    (void)u;if(a<0x80000000u||(uint64_t)a+w>0x80200000u)return -1;
    for(unsigned i=0;i<w;++i)ram[(a&0x1fffff)+i]=(uint8_t)(v>>(8*i));
    return 0;
}
static int bridge(void *u,PcPortMipsCpu *c,uint32_t target) {
    (void)u;if(target!=0x8003f8cc)return 0;
    c->gpr[2]=(uint32_t)rcos((int)c->gpr[4]);return 1;
}
static int oracle(PcPortMipsCpu *c,uint32_t entry,int32_t a,int32_t b,int32_t d) {
    PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};PcPortMipsCpuInit(c,&bus);
    c->gpr[4]=(uint32_t)a;c->gpr[5]=(uint32_t)b;c->gpr[6]=(uint32_t)d;
    c->gpr[29]=0x801ff000;c->gpr[31]=0xfffffffcu;
    return PcPortMipsRun(c,entry,0xfffffffcu,1000);
}
int main(void) {
    Callback callbacks[]={func_801E0850,func_801E08D4,func_801E0938,func_801E0988};
    const uint32_t entries[]={0x801e0850,0x801e08d4,0x801e0938,0x801e0988};
    if(!func_801E34BC||!callbacks[0]||!callbacks[1]||!callbacks[2]||!callbacks[3]) {
        fputs("EFFECT CALLBACK FAIL missing native owners\n",stderr);return 1;
    }
    FILE *f=fopen("disc/disc1.bin","rb");assert(f);
    for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));
    unsigned cases=0;PcPortMipsCpu cpu;
    const int32_t divisors[]={1,-1,2,-3,31,32767,-32768,65537};
    const int32_t offsets[]={0,1,-1,32,33,32767,-32768,65535};
    for(unsigned kind=0;kind<4;++kind)for(unsigned phase=0;phase<65536;++phase)for(unsigned v=0;v<8;++v){
        int32_t a=(int32_t)(phase|((v&1)?0xabcd0000u:0));
        trigValue=(int)(phase%8193)-4096;trigCalls=0;trigArg=0;
        assert(oracle(&cpu,entries[kind],a,divisors[v],offsets[v])==PC_PORT_MIPS_HALTED);
        unsigned calls=trigCalls;int arg=trigArg;trigCalls=0;trigArg=0;
        int32_t result=callbacks[kind](a,divisors[v],offsets[v]);
        if((uint32_t)result!=cpu.gpr[2]||calls!=trigCalls||arg!=trigArg){
            fprintf(stderr,"EFFECT CALLBACK FAIL kind=%u phase=%u variant=%u\n",kind,phase,v);return 1;
        }++cases;
    }
    for(int32_t selector=-32768;selector<65536;++selector){
        assert(oracle(&cpu,0x801e34bc,selector,0,0)==PC_PORT_MIPS_HALTED);
        if(func_801E34BC(selector)!=cpu.gpr[2]){fputs("EFFECT CALLBACK FAIL selector\n",stderr);return 1;}
    }
    const uint32_t breaks[]={0x801e088c,0x801e08f0,0x801e0954,0x801e09a4};
    for(unsigned kind=0;kind<4;++kind){
        trigValue=0;int r=oracle(&cpu,entries[kind],17,65536,0);char error[64];
        snprintf(error,sizeof(error),"break at pc=0x%08x",breaks[kind]);
        if(r!=PC_PORT_MIPS_UNSUPPORTED||strcmp(cpu.error,error)){fputs("EFFECT CALLBACK FAIL retail exception\n",stderr);return 1;}
        pid_t child=fork();assert(child>=0);
        if(!child){struct rlimit limit={0,0};assert(!setrlimit(RLIMIT_CORE,&limit));callbacks[kind](17,65536,0);_exit(0);}
        int status;assert(waitpid(child,&status,0)==child);
        if(!WIFSIGNALED(status)||WTERMSIG(status)!=SIGILL){fputs("EFFECT CALLBACK FAIL native exception\n",stderr);return 1;}
    }
    printf("EFFECT CALLBACK PASS %u arithmetic cases, 98304 selectors, four divide traps; controlled trig boundary\n",cases);
}
