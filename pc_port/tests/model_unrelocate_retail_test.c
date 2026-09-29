#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "battle_mips_adapter.h"

#define ENTRY 0x8002C4BCu
#define END 0x8002C59Cu
#define BASE 0x100000u
#define SIZE 0x10000u
#define HALT 0xFFFFFFFCu
extern int func_8002C4BC(uint8_t *);
static uint8_t ram[0x200000], code[END-ENTRY];
static unsigned seen[(END-ENTRY)/4], cases;
static void check(int ok, const char *s) {
    if (!ok) { fprintf(stderr,"MODEL UNRELOCATE FAIL case=%u %s\n",cases,s); exit(1); }
}
static int read_bus(void *o,uint32_t a,unsigned w,uint32_t *v) {
    (void)o;
    if (a >= ENTRY && a+w <= END) {
        *v=0; memcpy(v,code+a-ENTRY,w); seen[(a-ENTRY)/4]=1; return 0;
    }
    a &= 0x1FFFFFu;
    if (a+w>sizeof(ram)) return -1;
    *v=0; memcpy(v,ram+a,w); return 0;
}
static int write_bus(void *o,uint32_t a,unsigned w,uint32_t v) {
    (void)o; a &= 0x1FFFFFu;
    if (a+w>sizeof(ram)) return -1;
    memcpy(ram+a,&v,w); return 0;
}
static void put(uint8_t *p,unsigned off,uint32_t v) { memcpy(p+off,&v,4); }
int main(int argc,char **argv) {
    check(argc==2,"disc argument");
    FILE *f=fopen(argv[1],"rb"); check(f!=NULL,"open retail executable");
    check(fseek(f,ENTRY-0x8000F800u,SEEK_SET)==0,"seek");
    check(fread(code,1,sizeof(code),f)==sizeof(code),"retail read"); fclose(f);
    uint8_t *native=mmap((void*)(uintptr_t)BASE,SIZE,PROT_READ|PROT_WRITE,
                         MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0);
    check(native==(void*)(uintptr_t)BASE,"map native physical-RAM alias");
    const int counts[]={1,0,-1,3,8};
    const uint32_t flags[]={1,0,3,0xFFFFFFFDu,0xFFFFFFFEu};
    for(unsigned c=0;c<5;c++) for(unsigned fl=0;fl<5;fl++)
    for(unsigned sub=0;sub<4;sub++) for(unsigned seed=0;seed<16;seed++) {
        cases++;
        for(unsigned i=0;i<SIZE;i++) native[i]=(uint8_t)(i*37+seed*19);
        put(native,0,(uint32_t)counts[c]); put(native,4,flags[fl]);
        for(int i=0;i<counts[c];i++) {
            unsigned entry=0x10+i*0x38, table=0x1000+i*0x100;
            for(unsigned j=0;j<4;j++) put(native,entry+8+j*4,BASE+0x800+seed*4+j*16+i*64);
            put(native,entry+0x1C,sub ? BASE+table : 0);
            if(sub) {
                int n=sub==1?-1:sub==2?0:2;
                put(native,table,(uint32_t)n);
                for(int k=0;k<=n;k++) {
                    put(native,table+4+k*12+4,BASE+0x4000+k*8+seed*4);
                    put(native,table+4+k*12+8,BASE+0x5000+k*8+seed*4);
                }
            }
        }
        memcpy(ram+BASE,native,SIZE);
        PcPortMipsBus bus={0}; bus.read=read_bus; bus.write=write_bus;
        PcPortMipsCpu cpu; PcPortMipsCpuInit(&cpu,&bus);
        cpu.gpr[4]=BASE; cpu.gpr[29]=0x801FF000u; cpu.gpr[31]=HALT;
        int rc=PcPortMipsRun(&cpu,ENTRY,HALT,10000);
        if(rc!=PC_PORT_MIPS_HALTED) fprintf(stderr,"oracle rc=%d %s\n",rc,cpu.error);
        check(rc==PC_PORT_MIPS_HALTED,"retail execution");
        int result=func_8002C4BC(native);
        check((uint32_t)result==cpu.gpr[2],"return value");
        check(memcmp(native,ram+BASE,SIZE)==0,"full model and guard bytes differ from retail");
    }
    unsigned coverage=0;
    for(unsigned i=0;i<sizeof(seen)/sizeof(seen[0]);i++) coverage+=seen[i]!=0;
    check(coverage==sizeof(seen)/sizeof(seen[0]),"all retail instructions exercised");
    printf("MODEL UNRELOCATE PASS cases=%u instructions=%u/%zu\n",cases,coverage,sizeof(seen)/sizeof(seen[0]));
    munmap(native,SIZE); return 0;
}
