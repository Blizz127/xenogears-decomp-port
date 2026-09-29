#include "battle_mips_adapter.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern int32_t func_800263E4(uint8_t*,int32_t,void*,int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
static uint8_t ram[0x200000], captured[32];
static struct { uint8_t table[512], packets[512]; } f,initial,expected;
static uint8_t *address(uint32_t a,unsigned n) {
 uintptr_t base=(uintptr_t)&f;
 if(a>=base && (uint64_t)a+n<=base+sizeof(f))return (uint8_t*)(uintptr_t)a;
 if(a>=0x80000000 && (uint64_t)a+n<=0x80200000)return ram+(a&0x1fffff);
 return NULL;
}
static int rd(void*u,uint32_t a,unsigned n,uint32_t*v){(void)u;uint8_t*p=address(a,n);if(!p)return -1;*v=0;for(unsigned i=0;i<n;i++)*v|=(uint32_t)p[i]<<(i*8);return 0;}
static int wr(void*u,uint32_t a,unsigned n,uint32_t v){(void)u;uint8_t*p=address(a,n);if(!p)return -1;for(unsigned i=0;i<n;i++)p[i]=v>>(i*8);return 0;}
static void put16(uint8_t*p,uint16_t v){memcpy(p,&v,2);}
static uint32_t seed=0x263e4,cases;
static uint32_t next(void){seed=seed*1664525u+1013904223u;return seed;}
static void compare(unsigned trial,unsigned mode){
 for(unsigned i=0;i<sizeof(f);i++)((uint8_t*)&f)[i]=next()>>24;
 unsigned count=trial%5,buffer=(trial/5)%2,index=trial%96;
 int32_t x=(int32_t)next(),y=(int32_t)next(),scale=trial,fx=(int32_t)next(),fy=(int32_t)next();
 if(mode){fx=(trial>>1)&1;fy=trial&1;scale=0x1000;x=32;y=204;}
 if(mode==2){fx=0x100|(trial&255);fy=0x100|((trial>>8)&255);}
 put16(f.table+index*2+4,256);put16(f.table+256,count);
 for(unsigned i=0;i<count;i++){
  uint8_t*p=f.table+260+i*28;
  if(mode){put16(p,0);put16(p+2,0);put16(p+4,8);put16(p+6,8);put16(p+8,(uint16_t)-4);put16(p+10,(uint16_t)-8);}
  /* Hardware texture/CLUT coordinates; geometry retains full signed range. */
  put16(p+16,trial%3);put16(p+18,next()%1024);put16(p+20,next()%512);put16(p+22,next()%1024);put16(p+24,next()%512);
  p[26]=(trial>>2)&1;p[27]=(trial>>3)&1;
 }
 if(mode==3){count=1;buffer=0;index=100;x=32;y=204;scale=0x1000;fx=0;fy=1;put16(f.table+204,256);memcpy(f.table+256,captured,32);}
 initial=f;
 PcPortMipsBus bus={.read=rd,.write=wr};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
 cpu.gpr[4]=(uintptr_t)f.table;cpu.gpr[5]=index;cpu.gpr[6]=(uintptr_t)f.packets;cpu.gpr[7]=buffer;cpu.gpr[29]=0x801fff00;cpu.gpr[31]=0xfffffffc;
 const uint32_t args[]={x,y,scale,fx,fy};for(unsigned i=0;i<5;i++)wr(NULL,cpu.gpr[29]+16+i*4,4,args[i]);
 if(PcPortMipsRun(&cpu,0x800263e4,0xfffffffc,10000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"ATLAS FLIP ORACLE FAIL %s pc=%x\n",cpu.error,cpu.pc);exit(2);}
 expected=f;f=initial;
 int result=func_800263E4(f.table,index,f.packets,buffer,x,y,scale,fx,fy);
 if((uint32_t)result!=cpu.gpr[2] || memcmp(&f,&expected,sizeof(f))){fprintf(stderr,"ATLAS FLIP FAIL case=%u trial=%u mode=%u\n",cases,trial,mode);for(unsigned i=0;i<sizeof(f);i++)if(((uint8_t*)&f)[i]!=((uint8_t*)&expected)[i]){fprintf(stderr,"byte=%u native=%02x retail=%02x\n",i,((uint8_t*)&f)[i],((uint8_t*)&expected)[i]);break;}exit(1);}
 cases++;
}
int main(int argc,char**argv){FILE*p=fopen("disc/SLUS_006.64","rb");if(!p)return 2;fseek(p,0x800,SEEK_SET);size_t n=fread(ram+0x10000,1,sizeof(ram)-0x10000,p);fclose(p);if(n<0x40000)return 2;
 if(argc==2){FILE*q=fopen(argv[1],"rb");if(!q)return 2;fseek(q,0x2400,SEEK_SET);if(fread(captured,1,32,q)!=32)return 2;fclose(q);compare(0,3);puts("ATLAS FLIP CAPTURED ENTRY PASS");return 0;}
 for(unsigned i=0;i<65536;i++)compare(i,0);
 for(unsigned i=0;i<1280;i++)compare(i,1);
 for(unsigned i=0;i<512;i++)compare(i,2);
 printf("ATLAS FLIP PASS cases=%u full fixture and return value\n",cases);
}
