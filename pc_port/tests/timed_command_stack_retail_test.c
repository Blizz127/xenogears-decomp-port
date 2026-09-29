/* Retail-only provenance experiment. This does not certify a native VM. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "battle_mips_adapter.h"
static uint8_t ram[0x200000];
static uint32_t target_seen, arguments[4];
static unsigned calls, reads;
static int read_mem(void* unused,uint32_t a,unsigned w,uint32_t*v) {
    (void)unused;
    if(a<0x80000000u || (uint64_t)a+w>0x80200000u)return -1;
    if(a==0x801fefc8u && w==2)++reads;
    *v=0;for(unsigned i=0;i<w;++i)*v|=(uint32_t)ram[(a&0x1fffff)+i]<<(8*i);
    return 0;
}
static int write_mem(void* unused,uint32_t a,unsigned w,uint32_t v) {
    (void)unused;
    if(a<0x80000000u || (uint64_t)a+w>0x80200000u)return -1;
    for(unsigned i=0;i<w;++i)ram[(a&0x1fffff)+i]=(uint8_t)(v>>(8*i));
    return 0;
}
static int bridge(void* unused,PcPortMipsCpu*cpu,uint32_t target) {
    (void)unused;
    if(target!=0x801e8330u && target!=0x801e8394u)return 0;
    ++calls;target_seen=target;memcpy(arguments,cpu->gpr+4,sizeof(arguments));
    return 1;
}
int main(int argc,char**argv) {
    FILE*f=fopen("disc/disc1.bin","rb");assert(f);
    for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));
    if(argc==2) {
        if(!strcmp(argv[1],"zero-stack"))write_mem(NULL,0x801e6014,4,0x340f0000);
        else if(!strcmp(argv[1],"invert-branch")) {
            uint32_t instruction;read_mem(NULL,0x801e606c,4,&instruction);
            write_mem(NULL,0x801e606c,4,instruction^0x04000000u);
        } else return 2;
    }
    for(unsigned seed=0;seed<65536;++seed) {
        const uint32_t object=0x80010000,stream=0x80010400;
        memset(ram+0x10000,0,0x500);
        write_mem(NULL,object+0x9a,2,0xffff);write_mem(NULL,object+0x9e,2,1);
        write_mem(NULL,object+0xa0,4,stream);write_mem(NULL,object+0x10a,2,1);
        write_mem(NULL,stream+2,1,8);write_mem(NULL,stream+5,1,7);
        write_mem(NULL,0x801e8670,4,object);write_mem(NULL,0x801e86b0,2,3);
        write_mem(NULL,0x801e863c,2,0xa55a);
        /* Only the incoming, untouched stack halfword differs per case. */
        write_mem(NULL,0x801fefc8,2,seed);calls=reads=0;target_seen=0;
        PcPortMipsBus bus={.read=read_mem,.write=write_mem,.bridge=bridge};
        PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
        cpu.gpr[4]=object;cpu.gpr[5]=0x80010800;cpu.gpr[6]=1;
        cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
        int status=PcPortMipsRun(&cpu,0x801e5d44,0xfffffffcu,2000);
        if(status!=PC_PORT_MIPS_HALTED || calls!=1 || reads!=1 ||
           target_seen!=(seed?0x801e8394u:0x801e8330u) ||
           arguments[0]!=(seed?object:0) || arguments[1]!=(seed?0:8) ||
           arguments[2]!=(seed?8:7) || (seed && arguments[3]!=7)) {
            fprintf(stderr,"TIMED STACK FAIL seed=%u calls=%u reads=%u target=%08x status=%d %s\n",seed,calls,reads,target_seen,status,cpu.error);return 1;
        }
        uint32_t value;read_mem(NULL,object+0xa0,4,&value);assert(value==stream+10);
        read_mem(NULL,object+0x98,2,&value);assert(value==1);
        read_mem(NULL,object+0x9c,2,&value);assert(value==1);
    }
    puts("TIMED STACK PASS 65536 retail inputs: zero selects 8330, nonzero selects 8394; native ownership unresolved");
}
