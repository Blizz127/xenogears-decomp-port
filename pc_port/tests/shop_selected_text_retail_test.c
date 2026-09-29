/* Execute the disc MIPS and compare every string record after native mapping. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "system/menu.h"
#include "battle_mips_adapter.h"
#define ENTRY 0x801CBCF0u
#define END 0x801CC024u
#define BASE 0x80000000u
#define MENU 0x80070000u
#define STR 0x80080000u
#define XS 0x80090000u
#define FLAGS 0x80091000u
#define HALT 0xFFFFFFFCu
static u8 ram[0x200000], code[END-ENTRY];
static SystemMenu menu;
SystemMenu* g_Menu = &menu;
s32 D_801D2194[16], D_801D21B0[16];
static unsigned cases, checks, calls;
static unsigned seen[(END-ENTRY)/4];
static PcPortMipsCpu* active_cpu;
static MenuString strings[8];
static u8 flags[8];
static s32 xs[8];
extern void func_801CBCF0(s32, MenuString*, void*, s32*, u8*, u8, u8, u8);
static void require(int ok, const char* why) {
 ++checks;
 if (!ok) { fprintf(stderr,"SHOP SELECTED TEXT FAIL case=%u %s\n",cases,why); exit(1); }
}
static u8* ptr(uint32_t a) { return ram+(a-BASE); }
static void put(uint32_t a, uint32_t v) { memcpy(ptr(a),&v,4); }
static int read_bus(void* o,uint32_t a,unsigned w,uint32_t* v) {
 (void)o; if(a<BASE || (uint64_t)a+w>BASE+sizeof(ram)) return -1;
 if (active_cpu && w == 4 && a == active_cpu->pc && a >= ENTRY && a < END) ++seen[(a-ENTRY)/4];
 *v=0; for(unsigned i=0;i<w;i++) *v|=(uint32_t)ptr(a)[i]<<(8*i); return 0;
}
static int write_bus(void* o,uint32_t a,unsigned w,uint32_t v) {
 (void)o; if(a<BASE || (uint64_t)a+w>BASE+sizeof(ram)) return -1;
 for(unsigned i=0;i<w;i++) { ptr(a)[i]=(u8)(v>>(8*i)); } return 0;
}
static int bridge(void* o,PcPortMipsCpu* c,uint32_t target) {
 (void)o;
 if(target!=0x801CBC88u) return 0;
 require(c->gpr[4]==0 && c->gpr[6]==STR,"retail clear arguments");
 memset(ptr(FLAGS),0,c->gpr[5]); ++calls; c->pc=c->gpr[31]; return 1;
}
void func_801CBC88(u8 mode,u8 count,void* out,void* ids,u8* active) {
 (void)ids; require(mode==0 && out==strings && active==flags,"native clear arguments");
 memset(active,0,count); ++calls;
}
int main(int argc,char** argv) {
 require(argc==2,"disc argument"); FILE* f=fopen(argv[1],"rb");require(f!=NULL,"disc open");
 fseek(f,ENTRY-0x801C5000u,SEEK_SET);require(fread(code,1,sizeof(code),f)==sizeof(code),"disc read");fclose(f);
 const unsigned modes[]={0,1,2,255}; const s32 counts[]={0,4,8,0x104};
 for(unsigned seed=0;seed<8;seed++) for(unsigned mode=0;mode<4;mode++)
 for(unsigned selected=0;selected<8;selected++) for(unsigned offset=0;offset<7;offset++)
 for(unsigned ctx=0;ctx<2;ctx++) for(unsigned n=0;n<4;n++) {
  ++cases; memset(ram,0,sizeof(ram)); memcpy(ptr(ENTRY),code,sizeof(code));
  memset(strings,0xA5,sizeof(strings));memset(flags,0xA5,sizeof(flags));
  memset(ptr(STR),0xA5,8*0x80);memset(ptr(FLAGS),0xA5,8);
  for(unsigned i=0;i<16;i++) {
   D_801D2194[i]=(s32)(0xFFFF0000u+seed*10001u+i*777u);
   D_801D21B0[i]=(s32)(0x80000000u+seed*16001u+i*999u);
   put(0x801D2194u+i*4,D_801D2194[i]);put(0x801D21B0u+i*4,D_801D21B0[i]);
  }
  /* The retail tables are adjacent; mirror their shared address image. */
  memcpy(D_801D2194,ptr(0x801D2194u),sizeof(D_801D2194));
  memcpy(D_801D21B0,ptr(0x801D21B0u),sizeof(D_801D21B0));
  for(unsigned i=0;i<8;i++) {
   xs[i]=(s32)(0xFFFF0000u+seed*12345u+i*831u);put(XS+4*i,xs[i]);
   strings[i].width=(u8)(seed*37+i*31);ptr(STR+i*0x80)[0x7E]=strings[i].width;
  }
  menu.renderContext=ctx;put(0x800625A0u,MENU);put(MENU+0x308,ctx);
  PcPortMipsBus bus={0};PcPortMipsCpu cpu;bus.read=read_bus;bus.write=write_bus;bus.bridge=bridge;
  PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[4]=counts[n];cpu.gpr[5]=STR;cpu.gpr[6]=XS;cpu.gpr[7]=XS;
  cpu.gpr[29]=0x801FF000u;cpu.gpr[31]=HALT;
  put(cpu.gpr[29]+16,FLAGS);put(cpu.gpr[29]+20,selected);put(cpu.gpr[29]+24,offset);put(cpu.gpr[29]+28,modes[mode]);
  calls=0;active_cpu=&cpu;require(PcPortMipsRun(&cpu,ENTRY,HALT,10000)==PC_PORT_MIPS_HALTED,"MIPS execution");
  active_cpu=NULL;require(cpu.gpr[29]==0x801FF000u,"retail stack restored");
  unsigned retail_calls=calls;calls=0;
  func_801CBCF0(counts[n],strings,xs,xs,flags,selected,offset,modes[mode]);
  require(calls==retail_calls,"call count");require(memcmp(flags,ptr(FLAGS),8)==0,"active flags");
  for(unsigned i=0;i<8;i++) {
   require(memcmp(strings[i].polys,ptr(STR+i*0x80),0x50)==0,"polygon bytes");
   require(memcmp(strings[i].vertices,ptr(STR+i*0x80+0x50),0x20)==0,"vertices preserved");
   require(memcmp(&strings[i].vramDest,ptr(STR+i*0x80+0x70),8)==0,"rectangle preserved");
   require(strings[i].renderContext==ptr(STR+i*0x80)[0x7D],"record context");
   require(strings[i].width==ptr(STR+i*0x80)[0x7E],"width preserved");
  }
 }
 for(unsigned i=0;i<sizeof(seen)/sizeof(seen[0]);i++) require(seen[i]!=0,"full instruction coverage");
 printf("SHOP SELECTED TEXT PASS cases=%u checks=%u\n",cases,checks);return 0;
}
