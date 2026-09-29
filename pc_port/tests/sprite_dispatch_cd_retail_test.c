/* Opcode CD checked against the pinned retail dispatcher, including aliasing. */
#include "battle_mips_adapter.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern void func_8001FBE4(void *, uint32_t, void *);
static uint8_t ram[0x200000];
static struct { uint8_t sprite[256], model[128], dirs[80], operands[8]; } f, initial, expected;
static unsigned cases;
static void put32(void *p,uint32_t v){memcpy(p,&v,4);}
static uint8_t *addr(uint32_t a, unsigned w) {
  uintptr_t b = (uintptr_t)&f;
  if (a >= b && (uint64_t)a + w <= b + sizeof(f))
    return (uint8_t *)(uintptr_t)a;
  if (a >= 0x80000000 && (uint64_t)a + w <= 0x80200000)
    return ram + (a & 0x1fffff);
  return NULL;
}
static int rd(void *u, uint32_t a, unsigned w, uint32_t *v) {
  (void)u;
  uint8_t *p = addr(a, w);
  if (!p)
    return -1;
  *v = 0;
  for (unsigned i = 0; i < w; i++)
    *v |= (uint32_t)p[i] << (8 * i);
  return 0;
}
static int wr(void *u, uint32_t a, unsigned w, uint32_t v) {
  (void)u;
  uint8_t *p = addr(a, w);
  if (!p)
    return -1;
  for (unsigned i = 0; i < w; i++)
    p[i] = v >> (8 * i);
  return 0;
}
static void run(unsigned value,unsigned gate,unsigned variant,unsigned seed){
 for(unsigned i=0;i<sizeof(f);i++)((uint8_t*)&f)[i]=(i*31+seed)&255;
 uint8_t *model=variant==1?f.sprite+0x3c:f.model;
 put32(f.sprite+0x20,gate==0?0:(uintptr_t)model);
 put32(model+0x34,gate==1?0:(uintptr_t)f.dirs);
 uint8_t *op=variant==2?model:variant==3?f.dirs+2:f.operands;
 op[0]=value;op[1]=value>>8;
 initial=f;
 PcPortMipsBus bus={.read=rd,.write=wr};PcPortMipsCpu c;PcPortMipsCpuInit(&c,&bus);
 c.gpr[4]=(uintptr_t)f.sprite;c.gpr[5]=0xa50000cd;c.gpr[6]=(uintptr_t)op;c.gpr[29]=0x801fff00;c.gpr[31]=0xfffffffc;
 if(PcPortMipsRun(&c,0x8001fbe4,0xfffffffc,1000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"oracle %s pc%x\n",c.error,c.pc);exit(2);}
 expected=f;f=initial;func_8001FBE4(f.sprite,0xa50000cd,op);
 if(memcmp(&f,&expected,sizeof(f))){fprintf(stderr,"SPRITE CD FAIL case=%u value=%x gate=%u variant=%u seed=%u\n",cases,value,gate,variant,seed);exit(1);}
 cases++;
}
int main(void){
 FILE *p=fopen("disc/SLUS_006.64","rb");if(!p)return 2;fseek(p,0x800,SEEK_SET);size_t n=fread(ram+0x10000,1,sizeof(ram)-0x10000,p);fclose(p);if(n<0x134e0)return 2;
 for(unsigned value=0;value<65536;value++)for(unsigned gate=0;gate<3;gate++)for(unsigned variant=0;variant<4;variant++)run(value,gate,variant,value*17);
 for(unsigned seed=0;seed<65536;seed++)run(0x1ff,2,0,seed);
 printf("SPRITE CD PASS cases=%u complete fixture comparison\n",cases);
}
