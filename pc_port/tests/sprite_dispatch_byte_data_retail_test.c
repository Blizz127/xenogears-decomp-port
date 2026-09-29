#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "battle_mips_adapter.h"
extern void func_8001FBE4(void*,uint32_t,void*);
static uint8_t ram[0x200000],sprite[0x100],ops[2];
static int rd(void*u,uint32_t a,unsigned w,uint32_t*v){(void)u;uint8_t*p;if(a>=0x80100000u&&a+w<=0x80100100u)p=sprite+(a-0x80100000u);else if(a>=0x80100100u&&a+w<=0x80100102u)p=ops+(a-0x80100100u);else if(a>=0x80000000u&&a+w<=0x80200000u)p=ram+(a&0x1fffff);else return-1;*v=0;for(unsigned i=0;i<w;++i)*v|=(uint32_t)p[i]<<(8*i);return 0;}
static int wr(void*u,uint32_t a,unsigned w,uint32_t v){(void)u;uint8_t*p;if(a>=0x80100000u&&a+w<=0x80100100u)p=sprite+(a-0x80100000u);else if(a>=0x80000000u&&a+w<=0x80200000u)p=ram+(a&0x1fffff);else return-1;for(unsigned i=0;i<w;++i)p[i]=(uint8_t)(v>>(8*i));return 0;}
static void run(unsigned opcode,unsigned value){memset(sprite,0xa5,sizeof sprite);ops[0]=value;PcPortMipsBus b={.read=rd,.write=wr};PcPortMipsCpu c;PcPortMipsCpuInit(&c,&b);c.gpr[4]=0x80100000;c.gpr[5]=opcode;c.gpr[6]=0x80100100;c.gpr[29]=0x801fff00;c.gpr[31]=0xfffffffcu;uint8_t before[sizeof sprite];memcpy(before,sprite,sizeof sprite);int rc=PcPortMipsRun(&c,0x8001fbe4,0xfffffffcu,1000);if(rc!=PC_PORT_MIPS_HALTED){fprintf(stderr,"SPRITE BYTE FAIL cpu opcode=%x value=%x rc=%d err=%s pc=%x\n",opcode,value,rc,c.error,c.pc);assert(0);}uint8_t expected=sprite[opcode==0xbf?0x36:0x3d],expected_hi=sprite[0x37];memcpy(sprite,before,sizeof sprite);func_8001FBE4(sprite,opcode,ops);if(sprite[opcode==0xbf?0x36:0x3d]!=expected||sprite[0x37]!=expected_hi||memcmp(sprite+0x40,before+0x40,sizeof sprite-0x40)){fprintf(stderr,"SPRITE BYTE FAIL opcode=%x value=%x\n",opcode,value);assert(0);}}
int main(void){FILE*f=fopen("disc/SLUS_006.64","rb");assert(f);unsigned off=0x800183d8u-0x8000f800u;assert(!fseek(f,off,SEEK_SET));assert(fread(ram+0x183d8,1,0x5000,f)==0x5000);off=0x8001fbe4u-0x8000f800u;assert(!fseek(f,off,SEEK_SET));assert(fread(ram+0x1fbe4,1,0x4000,f)==0x4000);assert(!fclose(f));for(unsigned v=0;v<256;++v){run(0xbf,v);run(0xa2,v);}puts("SPRITE BYTE DATA PASS 512 retail/native cases");}
