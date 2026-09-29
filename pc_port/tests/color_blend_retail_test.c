#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#define uid_t psyq_test_uid_t
#define gid_t psyq_test_gid_t
#include "common.h"
#undef uid_t
#undef gid_t
extern void func_80026FE8(s32,s32,u16*,const u16*,const u16*) __attribute__((weak));
#include "psyq/libgte.h"
extern void MTC2(unsigned int,int);
extern unsigned int MFC2(int);
extern unsigned int CFC2(int);
extern void CTC2(unsigned int,int);
extern int doCOP2(int);
#include "battle_mips_adapter.h"
#include "psx/gtereg.h"
static u8 ram[0x200000];
static struct {u16 a[16],b[16],destination[16];} fixture,initial,expected;
static u8* address(u32 a,unsigned w){
    if(a>=0x80000000u&&(uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);
    uintptr_t lo=(uintptr_t)&fixture;if(a>=lo&&(uint64_t)a+w<=lo+sizeof(fixture))return(u8*)(uintptr_t)a;
    return NULL;
}
static int rd(void*u,u32 a,unsigned w,u32*v){(void)u;u8*p=address(a,w);if(!p)return-1;*v=0;for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(i*8);return 0;}
static int wr(void*u,u32 a,unsigned w,u32 v){(void)u;u8*p=address(a,w);if(!p)return-1;for(unsigned i=0;i<w;++i)p[i]=(u8)(v>>(i*8));return 0;}
static u32 copRead(void*u,int control,unsigned reg){(void)u;return control?CFC2(reg):MFC2(reg);}
static void copWrite(void*u,int control,unsigned reg,u32 value){(void)u;if(control)CTC2(value,reg);else MTC2(value,reg);}
static int copCommand(void*u,u32 instruction){(void)u;doCOP2(instruction&0x1ffffffu);return 0;}
int main(void){
    if(!func_80026FE8){fputs("COLOR BLEND FAIL missing native owner\n",stderr);return 1;}
    FILE*f=fopen("disc/SLUS_006.64","rb");assert(f);assert(!fseek(f,0x177e8,SEEK_SET));assert(fread(ram+0x26fe8,1,180,f)==180);assert(!fclose(f));
    const s32 factors[]={(-2147483647-1),-32768,-1,0,1,16,31,32,33,0x7fffffff};unsigned cases=0;
    for(unsigned count=0;count<=8;++count)for(unsigned pixel=0;pixel<65536;pixel+=257)for(unsigned factor=0;factor<10;++factor)for(unsigned alias=0;alias<5;++alias){
        for(unsigned i=0;i<16;++i){fixture.a[i]=(u16)(pixel+i*1357);fixture.b[i]=(u16)(65535-pixel-i*311);fixture.destination[i]=0xa5a5;}
        initial=fixture;u16*destination=alias==0?fixture.destination:alias<=2?fixture.a+alias-1:fixture.b+alias-3;
        memset(&gteRegs,0,sizeof(gteRegs));MTC2(0x1234,9);CTC2(0xabcdef,31);
        PcPortMipsBus bus={.read=rd,.write=wr,.cop2_read=copRead,.cop2_write=copWrite,.cop2_command=copCommand};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
        cpu.gpr[4]=count;cpu.gpr[5]=(u32)factors[factor];cpu.gpr[6]=(u32)(uintptr_t)destination;cpu.gpr[7]=(u32)(uintptr_t)fixture.a;cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
        wr(NULL,0x801ff010,4,(u32)(uintptr_t)fixture.b);
        if(PcPortMipsRun(&cpu,0x80026fe8,0xfffffffcu,10000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"COLOR BLEND FAIL oracle %s\n",cpu.error);return 1;}
        expected=fixture;GTERegisters expectedGte=gteRegs;fixture=initial;
        memset(&gteRegs,0,sizeof(gteRegs));MTC2(0x1234,9);CTC2(0xabcdef,31);
        func_80026FE8(count,factors[factor],destination,fixture.a,fixture.b);
        if(memcmp(&fixture,&expected,sizeof(fixture))||memcmp(&gteRegs,&expectedGte,sizeof(gteRegs))){fprintf(stderr,"COLOR BLEND FAIL count=%u pixel=%u factor=%d alias=%u\n",count,pixel,factors[factor],alias);return 1;}
        ++cases;
    }
    puts("COLOR BLEND shared PsyCross backend, not independent hardware validation");
    printf("COLOR BLEND PASS %u cases: full retail body, complete fixture and GTE state\n",cases);
    PcPortMipsBus bus={.read=rd,.write=wr,.cop2_read=copRead,.cop2_write=copWrite,.cop2_command=copCommand};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
    cpu.gpr[4]=0;cpu.gpr[5]=0;cpu.gpr[6]=(u32)(uintptr_t)fixture.destination;cpu.gpr[7]=(u32)(uintptr_t)fixture.a;cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
    wr(NULL,0x801ff010,4,0);
    if(PcPortMipsRun(&cpu,0x80026fe8,0xfffffffcu,1000)!=PC_PORT_MIPS_FAULT){fputs("COLOR BLEND FAIL missing retail trailing-read fault\n",stderr);return 1;}
    fflush(stdout);pid_t child=fork();assert(child>=0);
    if(!child){struct rlimit limit={0,0};assert(!setrlimit(RLIMIT_CORE,&limit));func_80026FE8(0,0,fixture.destination,fixture.a,NULL);_exit(0);}
    int status;assert(waitpid(child,&status,0)==child);
#ifdef COLOR_BLEND_UBSAN
    if(!WIFEXITED(status)||WEXITSTATUS(status)!=1){fputs("COLOR BLEND FAIL expected sanitizer rejection of null read\n",stderr);return 1;}
    puts("COLOR BLEND trailing-read fault PASS: retail bus fault / expected UBSan null-read rejection");
#else
    if(!WIFSIGNALED(status)||WTERMSIG(status)!=SIGSEGV){fputs("COLOR BLEND FAIL native trailing read omitted\n",stderr);return 1;}
    puts("COLOR BLEND trailing-read fault PASS: retail bus fault / native SIGSEGV");
#endif
}
