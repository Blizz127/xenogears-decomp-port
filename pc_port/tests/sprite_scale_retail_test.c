#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "battle_mips_adapter.h"

#define ENTRY 0x80022CACu
#define END 0x80022CDCu
#define BASE 0x100000u
#define SIZE 0x10000u
#define HALT 0xFFFFFFFCu
extern int func_80022CAC(void *, int);
static uint8_t ram[0x200000], code[END-ENTRY];
static unsigned seen[(END-ENTRY)/4], cases;
static void check(int ok, const char *s) {
    if (!ok) { fprintf(stderr,"SPRITE SCALE FAIL case=%u %s\n",cases,s); exit(1); }
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
int main(int argc,char **argv) {
 check(argc==2,"disc argument"); FILE*f=fopen(argv[1],"rb");check(f!=NULL,"disc");
 fseek(f,ENTRY-0x8000F800u,SEEK_SET);check(fread(code,1,sizeof(code),f)==sizeof(code),"code");fclose(f);
 uint8_t native[256]={0};
 const uint32_t values[]={0,1,0xffffffffu,0x7fffffffu,0x80000000u,0x07ffffffu,0xf8000000u,1023,1024,0xfffffc01u,0xfffffc00u,65535,65536,0xffff0000u};
 for(unsigned t=0;t<65536;t++) for(unsigned j=0;j<sizeof(values)/sizeof(values[0]);j++) {
  cases++; uint16_t timer=t;memcpy(native+0x3a,&timer,2);memcpy(ram+BASE,native,sizeof(native));
  PcPortMipsBus bus={0};bus.read=read_bus;bus.write=write_bus;PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
  cpu.gpr[4]=BASE;cpu.gpr[5]=values[j];cpu.gpr[31]=HALT;
  check(PcPortMipsRun(&cpu,ENTRY,HALT,100)==PC_PORT_MIPS_HALTED,"oracle");
  uint32_t result=func_80022CAC(native,(int32_t)values[j]);
  if(result!=cpu.gpr[2]){fprintf(stderr,"timer=%u value=%08x native=%08x retail=%08x\n",t,values[j],result,cpu.gpr[2]);check(0,"result");}
  check(!memcmp(native,ram+BASE,sizeof(native)),"memory");
 }
 unsigned covered=0;for(unsigned i=0;i<sizeof(seen)/sizeof(seen[0]);i++)covered+=seen[i]!=0;
 check(covered==sizeof(seen)/sizeof(seen[0]),"instruction coverage");
 printf("SPRITE SCALE PASS cases=%u instructions=%u/%zu\n",cases,covered,sizeof(seen)/sizeof(seen[0]));return 0;
}
