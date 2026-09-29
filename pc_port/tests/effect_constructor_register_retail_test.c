/* Retail provenance test, not native constructor parity or scene proof. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "battle_mips_adapter.h"
static uint8_t ram[0x200000];
static unsigned owner_calls;
static unsigned allocation_calls;
static int rd(void* unused,uint32_t a,unsigned w,uint32_t*v) {
    (void)unused;
    if(a<0x80000000u || (uint64_t)a+w>0x80200000u)return -1;
    *v=0;for(unsigned i=0;i<w;++i)*v|=(uint32_t)ram[(a&0x1fffff)+i]<<(8*i);
    return 0;
}
static int wr(void* unused,uint32_t a,unsigned w,uint32_t v) {
    (void)unused;
    if(a<0x80000000u || (uint64_t)a+w>0x80200000u)return -1;
    for(unsigned i=0;i<w;++i)ram[(a&0x1fffff)+i]=(uint8_t)(v>>(8*i));
    return 0;
}
static int bridge(void* unused,PcPortMipsCpu*cpu,uint32_t target) {
    (void)unused;
    if(target==0x80031bdc){assert(cpu->gpr[4]==2 && cpu->gpr[5]==0);++allocation_calls;cpu->gpr[2]=0x80051000;return 1;}
    if(target!=0x80032498)return 0;
    assert(cpu->gpr[4]==4 && cpu->gpr[5]==0);++owner_calls;
    return 1;
}
int main(int argc,char**argv) {
    FILE*f=fopen("disc/disc1.bin","rb");assert(f);
    for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));
    if(argc==2 && !strcmp(argv[1],"zero-carry")) {
        /* Replace the no-op delay slot with OR s1,zero,zero in RAM only. */
        wr(NULL,0x801e1008,4,0x00008825);
    }
    const int offsets[]={-3,-2,-1,0,1,2};unsigned cases=0;
    for(unsigned seed=0;seed<65536;++seed)for(unsigned row=0;row<6;++row) {
        uint32_t object=0x80010000,destination=0x80011000;
        memset(ram+0x10000,0xa5,0x1100);
        wr(NULL,object+4,4,destination);wr(NULL,object+0x1a,2,0);
        /* Arguments 4..18: source; two triples; output XY; WH; timing; callback. */
        uint32_t args[15]={0,0x1234,0x5678,0x9abc,0x1357,0x2468,0xbeef,
            0,(uint32_t)offsets[row],1,1,3,4,5,0};
        for(unsigned i=0;i<15;++i)wr(NULL,0x801ff010+i*4,4,args[i]);
        PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};
        PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
        cpu.gpr[4]=object;cpu.gpr[5]=0;cpu.gpr[6]=4;cpu.gpr[7]=2;
        cpu.gpr[17]=0xa55a0000u|seed;cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
        owner_calls=0;
        int status=PcPortMipsRun(&cpu,0x801e0a00,0xfffffffcu,2000);
        uint32_t got;rd(NULL,destination,2,&got);
        int remainder=offsets[row]%3;
        uint32_t expected=remainder<0?seed:args[1+(unsigned)remainder];
        if(status!=PC_PORT_MIPS_HALTED || owner_calls!=1 || cpu.gpr[2]!=object || got!=expected || cpu.gpr[17]!=(0xa55a0000u|seed)) {
            fprintf(stderr,"EFFECT REGISTER FAIL seed=%u row=%d got=%04x expected=%04x status=%d %s\n",seed,offsets[row],got,expected,status,cpu.error);return 1;
        }
        ++cases;
    }
    for(unsigned index=0;index<256;++index) {
        uint32_t object=0x80010000+index*4,stream=0x80040000,effect=0x80050000;
        memset(ram+0x10000,0,0x1000);memset(ram+0x40000,0,0x100);
        memset(ram+0x50000,0,0x2000);
        wr(NULL,object+0x9a,2,0xffff);wr(NULL,object+0x9e,2,1);
        wr(NULL,object+0xa0,4,stream);wr(NULL,object+0x10e,1,1);wr(NULL,object+0x118,4,effect);
        wr(NULL,stream+2,1,9);wr(NULL,stream+4,1,1);wr(NULL,stream+5,1,255);wr(NULL,stream+6,1,4);
        wr(NULL,stream+8,2,0x1234);wr(NULL,stream+0xa,2,0xffff);
        wr(NULL,stream+0x12,1,2);wr(NULL,stream+0x13,1,1);wr(NULL,stream+0x14,1,1);
        wr(NULL,0x801e8644,4,0x80052000);
        PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
        cpu.gpr[4]=object;cpu.gpr[5]=0x80053000;cpu.gpr[6]=1;cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
        owner_calls=allocation_calls=0;
        int status=PcPortMipsRun(&cpu,0x801e5d44,0xfffffffcu,4000);
        uint32_t got;rd(NULL,0x80051000,2,&got);
        if(status!=PC_PORT_MIPS_HALTED || owner_calls!=1 || allocation_calls!=1 || got!=(object&0xffff)) {
            fprintf(stderr,"EFFECT REGISTER FAIL caller index=%u got=%04x status=%d %s\n",index,got,status,cpu.error);return 1;
        }
    }
    printf("EFFECT REGISTER PASS %u direct cases and 256 real timed-command calls: caller object supplies s1 carry\n",cases);
}
