/* Full retail routine vs verbatim native body. Math/rectangle/script lookup
 * boundaries are controlled equally; this is not a geometry or SDK oracle. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "battle_mips_adapter.h"
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;
typedef int16_t s16; typedef int32_t s32;
typedef struct { s32 vx,vy,vz,pad; } VECTOR;
static struct { u8 actors[3][0x5c], data[3][0x1b8], sprite[0x80]; } fixture, initial;
static u8 ram[0x200000];
static void *g_FieldActors=fixture.actors;
static s32 D_800ADBFC=3,D_800ADF64,D_80285988,g_FieldSystemMode;
static u16 D_800C2694; static s16 D_800B2174;
static unsigned cases,native_mode; static int latch_initial; static unsigned seen[495]; static PcPortMipsCpu *active_cpu; static int angle_result,rect_result;
static struct { unsigned n; s32 events[32][5]; } trace;
static void fail(const char *s) { fprintf(stderr,"INTERACTION FAIL case=%u native=%u %s\n",cases,native_mode,s);exit(1); }
static void event(s32 id,s32 a,s32 b,s32 c,s32 d) { if(trace.n>=32)fail("event overflow"); s32 e[]={id,a,b,c,d};memcpy(trace.events[trace.n++],e,sizeof(e)); }
static s32 ratan2(s32 z,s32 x) {event(1,z,x,0,0);return angle_result;}
static s32 func_8008237C(s32 x,s32 z,void *p,s32 extra) {event(2,x,z,(u8*)p-fixture.data[0],extra);return rect_result;}
static u16 FieldScriptGetBytecodeOffset(int a,int b) {event(3,a,b,0,0);return 0x120+a*32+b;}
static void Square0(VECTOR *a,VECTOR *b) {for(int i=0;i<3;i++){s32 v=(s16)((s32*)a)[i];((s32*)b)[i]=v*v;}}
#include INTERACTION_BODY
static u8 *address(u32 a,unsigned w) {
 uintptr_t lo=(uintptr_t)&fixture;
 if(a>=lo&&(uint64_t)a+w<=lo+sizeof(fixture))return (u8*)(uintptr_t)a;
 if(a>=0x80000000u&&(uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);
 if(a>=0x80285988u&&(uint64_t)a+w<=0x8028598cu)return (u8*)&D_80285988+(a-0x80285988u);
 return NULL;
}
static void put16(void *p,u16 v){memcpy(p,&v,2);} static void put32(void *p,u32 v){memcpy(p,&v,4);}
static int rd(void *o,u32 a,unsigned w,u32 *v){(void)o;u8*p=address(a,w);if(!p)return -1;if(active_cpu && w==4 && a==active_cpu->pc && a>=0x8008399c && a<0x80084158)seen[(a-0x8008399c)/4]++;*v=0;for(unsigned i=0;i<w;i++)*v|=(u32)p[i]<<(i*8);return 0;}
static int wr(void *o,u32 a,unsigned w,u32 v){(void)o;u8*p=address(a,w);if(!p)return -1;for(unsigned i=0;i<w;i++)p[i]=v>>(i*8);return 0;}
static u32 cop[32];
static u32 cr(void*o,int ctl,unsigned r){(void)o;if(ctl)fail("cop control read");return cop[r];}
static void cw(void*o,int ctl,unsigned r,u32 v){(void)o;if(ctl)fail("cop control write");cop[r]=v;}
static int cc(void*o,u32 v){(void)o;if((v&0x1ffffff)!=0xa00428)fail("unexpected GTE");for(unsigned i=0;i<3;i++){s32 n=(s16)cop[9+i];cop[25+i]=n*n;}return 0;}
static int bridge(void*o,PcPortMipsCpu*c,u32 target){(void)o;
 if(target==0x8004a414){u8*a=address(c->gpr[4],12),*b=address(c->gpr[5],12);if(!a||!b)return -1;Square0((VECTOR*)a,(VECTOR*)b);return 1;}
 if(target==0x8004b32c){c->gpr[2]=ratan2(c->gpr[4],c->gpr[5]);return 1;}
 if(target==0x8008237c){c->gpr[2]=func_8008237C(c->gpr[4],c->gpr[5],address(c->gpr[6],0x1b8),c->gpr[7]);return 1;}
 if(target==0x800a3090){c->gpr[2]=FieldScriptGetBytecodeOffset(c->gpr[4],c->gpr[5]);return 1;}
 return 0;
}
static void run(unsigned native){
 native_mode=native;fixture=initial;memset(&trace,0,sizeof(trace));D_800ADF64=latch_initial;D_80285988=0x1234;
 if(native)func_8008399C(0,fixture.actors[0],fixture.data[0]);
 else {
  put32(ram+0xafb10,(u32)(uintptr_t)fixture.actors);put32(ram+0xadbfc,D_800ADBFC);
  put32(ram+0xadf64,latch_initial);put16(ram+0xc2694,D_800C2694);put16(ram+0xb2174,D_800B2174);put32(ram+0xc268c,g_FieldSystemMode);
  PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge,.cop2_read=cr,.cop2_write=cw,.cop2_command=cc};PcPortMipsCpu cpu;
  PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[4]=0;cpu.gpr[5]=(u32)(uintptr_t)fixture.actors[0];cpu.gpr[6]=(u32)(uintptr_t)fixture.data[0];cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffc;
  active_cpu=&cpu;int rc=PcPortMipsRun(&cpu,0x8008399c,0xfffffffc,10000);active_cpu=NULL;if(rc){fprintf(stderr,"%d %s pc=%x\n",rc,cpu.error,cpu.pc);fail("retail execution");}
  memcpy(&D_800ADF64,ram+0xadf64,4);
 }
}
static void check(void){
 run(0);typeof(fixture) expected=fixture;typeof(trace) expected_trace=trace;s32 latch=D_800ADF64,flag=D_80285988;
 run(1);
 if(memcmp(&fixture,&expected,sizeof(fixture))||memcmp(&trace,&expected_trace,sizeof(trace))||latch!=D_800ADF64||flag!=D_80285988){
  fprintf(stderr,"retail events=%u native=%u flags=%x/%x\n",expected_trace.n,trace.n,flag,D_80285988);
  for(unsigned i=0;i<sizeof(fixture);i++)if(((u8*)&fixture)[i]!=((u8*)&expected)[i]){fprintf(stderr,"first byte %x retail=%02x native=%02x\n",i,((u8*)&expected)[i],((u8*)&fixture)[i]);break;}
  fail("full fixture/global/helper trace mismatch");
 }
 cases++;
}
static void setup(int distance,u32 flags0,u32 flags4,int button,int rotation){
 memset(&initial,0,sizeof(initial));D_800ADBFC=2;D_800B2174=0;g_FieldSystemMode=1;D_800C2694=button?0x20:0;angle_result=0;rect_result=0;latch_initial=7;
 for(unsigned i=0;i<3;i++){
  put32(initial.actors[i]+0x4c,(u32)(uintptr_t)fixture.data[i]);put32(initial.actors[i]+4,(u32)(uintptr_t)fixture.sprite);
  initial.data[i][0x74]=255;put16(initial.data[i]+0x1a,64);put16(initial.data[i]+0x1e,16);put16(initial.data[i]+0x106,rotation);
  for(unsigned j=0;j<8;j++){initial.data[i][0x8f+j*8]=255;put32(initial.data[i]+0x90+j*8,0x3c0000);}
 }
 put32(initial.data[0],1);put16(initial.data[1]+0x22,distance);put32(initial.data[1],flags0);put32(initial.data[1]+4,flags4);
}
int main(int argc,char**argv){
 if(argc!=2)fail("field module argument");FILE*f=fopen(argv[1],"rb");if(!f)fail("open");if(fseek(f,0x13eac,SEEK_SET)||fread(ram+0x8399c,1,0x7bc,f)!=0x7bc)fail("read");fclose(f);
 /* Annulus from the natural doorway, facing-angle math boundary controlled. */
 setup(42,0,0,1,0);check();
 int distances[]={0,1,39,40,41,42,63,64,65,500};u32 f0[]={0,1,0x2000,0x20000,0x200000,0x800000,0x8000000};u32 f4[]={0,0x80,0x100,0x180,0x40000,0x4000000,0x40100};
 for(unsigned d=0;d<10;d++)for(unsigned a=0;a<7;a++)for(unsigned b=0;b<7;b++)for(int btn=0;btn<2;btn++)for(int mode=0;mode<2;mode++)for(int lock=0;lock<2;lock++){
  setup(distances[d],f0[a],f4[b],btn,0);g_FieldSystemMode=mode;D_800B2174=lock;check();
 }
 for(int r=0;r<4096;r++){setup(42,0,0,1,r);check();setup(0,0x2000,0x40000,1,r);check();}

 for(int a=-2048;a<=2048;a++)for(int rect=0;rect<2;rect++){
  setup(25,rect?0x2000:0,0,1,(-a)&4095);angle_result=a;check();
 }
 /* Natural door geometry and flags, sweeping every facing angle. The angle
  * helper remains an explicit boundary, so this is not a live replay. */
 for(int r=0;r<4096;r++){
  setup(0,0x1b0,0x400800,1,r);put16(initial.data[0]+0x22,123);put16(initial.data[0]+0x2a,-119);
  put16(initial.data[1]+0x22,123);put16(initial.data[1]+0x26,-58);put16(initial.data[1]+0x2a,-177);
  put16(initial.data[1]+0x60,25);put16(initial.data[1]+0x64,25);put16(initial.data[1]+0x1a,38);angle_result=-600;check();
 }
 unsigned seed=0x8399c;
 for(unsigned n=0;n<20000;n++){
  seed=seed*1664525u+1013904223u;unsigned v=seed;
  setup(distances[v%10],f0[(v>>4)%7],f4[(v>>8)%7],v&1,(v>>16)&4095);
  D_800ADBFC=3;D_800B2174=(v>>2)&1;g_FieldSystemMode=(v>>3)&1;latch_initial=(v>>4)&1;rect_result=(v>>5)&1;
  angle_result=(int)((v>>8)&4095)-2048;
  put32(initial.data[2],f0[(v>>12)%7]);put32(initial.data[2]+4,f4[(v>>20)%7]);put16(initial.data[2]+0x22,distances[(v>>24)%10]);
  if(v&0x40)initial.data[0][0x74]=1;
  if(v&0x80)put16(initial.data[1]+0x26,(v&0x100)?65:-65);
  for(unsigned j=0;j<8;j++){
   if(v&(1u<<(j+8)))put32(initial.data[1]+0x90+j*8,(v&0x400000)?0x7c0000:0);
  }
  if(v&0x10000)initial.data[1][0x8f+((v>>17)&7)*8]=(v&0x100000)?2:3;
  check();
 }

 for(unsigned r=0;r<65536;r++)for(int button=0;button<2;button++){
  setup(42,0,0,button,0);put16(initial.data[0]+0x1e,r);put16(initial.data[1]+0x1e,65535-r);check();put16(initial.data[1]+0x1e,16);check();
 }
 int coords[]={-32768,-32767,-1,0,1,32766,32767};
 for(unsigned x=0;x<7;x++)for(unsigned z=0;z<7;z++)for(unsigned ox=0;ox<7;ox++)for(unsigned oz=0;oz<7;oz++){
  setup(0,0,0,1,0);put16(initial.data[1]+0x22,coords[x]);put16(initial.data[1]+0x2a,coords[z]);put16(initial.data[1]+0x60,coords[ox]);put16(initial.data[1]+0x64,coords[oz]);check();
 }
 unsigned covered=0;for(unsigned i=0;i<495;i++)covered+=seen[i]!=0;
 if(covered!=495)fail("retail instruction coverage");
 printf("INTERACTION PASS cases=%u instructions=%u/495\n",cases,covered);return 0;
}
