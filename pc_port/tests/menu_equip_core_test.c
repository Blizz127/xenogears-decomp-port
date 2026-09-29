#include "common.h"
#include "system/menu.h"
#include "battle_mips_adapter.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned char ram[0x200000],work[0x3300];
static SystemMenu menu;static MenuManager manager;static MenuPointerCursors cursors;static MenuUnk6 resources;
SystemMenu*g_Menu=&menu;u16 D_801E9DBC[16];
u8 D_801EA730[0x190];
u8 stateStorage[0x4600] __attribute__((aligned(8)));
__asm__(".globl g_GameState\n.set g_GameState, stateStorage\n");
static u32 events[4096][11],expected_events[4096][11];
static unsigned ec,frames,seed,script[96],coverage[618];
static u8*MenuRawPointer(u32 off){if(off!=0x35c)abort();return work;}
static u32 event(u32 pc,u32*a,unsigned n){
 if(ec>=4096){fprintf(stderr,"FAIL event overflow\n");exit(1);}events[ec][0]=pc;events[ec][1]=n;
 for(unsigned i=0;i<n;i++)events[ec][2+i]=a[i];ec++;
 if(pc==0x801c7bf4){if(frames>160){fprintf(stderr,"FAIL frame overflow\n");exit(1);}menu.input=frames<96?script[frames]:5;frames++;if(menu.transitionEffectState)menu.transitionEffectState--;}
 if(pc==0x801d29a8 && (seed&1))menu.transitionEffectState=2;
 if(pc==0x801de5cc){static u32 counts[]={0,1,7,20,100};return counts[seed%5];}
 if(pc==0x801d9704)return (seed&2)?(a[0]+1)%3:a[0];
 if(pc==0x801df0d4)return seed&4?1:0;
 return 0;
}
void func_801C7BF4(void){event(0x801c7bf4,NULL,0);}
void func_801E36D4(void*p,u8 id){if(p!=&resources)abort();u32 a[]={0x80110000,id};event(0x801e36d4,a,2);}
void func_801E3A80(void*p,u8 id){if(p!=&resources)abort();u32 a[]={0x80110000,id};event(0x801e3a80,a,2);}
s32 func_801DE2C8(s32 a0){u32 a[]={ a0 };return event(0x801de2c8,a,1);}
s32 func_801DF5D0(s32 a0, s32 a1){u32 a[]={ a0,a1 };return event(0x801df5d0,a,2);}
s32 func_801D8EA4(s32 a0, s32 a1, s32 a2, s32 a3){u32 a[]={ a0,a1,a2,a3 };return event(0x801d8ea4,a,4);}
s32 func_801DE474(s32 a0, s32 a1){u32 a[]={ a0,a1 };return event(0x801de474,a,2);}
s32 func_801DE5CC(s32 a0, s32 a1, s32 a2, s32 a3, s32 a4){u32 a[]={ a0,a1,a2,a3,a4 };return event(0x801de5cc,a,5);}
s32 func_801D3344(s32 a0, s32 a1, s32 a2){u32 a[]={ a0,a1,a2 };return event(0x801d3344,a,3);}
s32 func_801DB0A8(s32 a0, s32 a1, s32 a2, s32 a3){u32 a[]={ a0,a1,a2,a3 };return event(0x801db0a8,a,4);}
s32 func_801DFB68(s32 a0, s32 a1, s32 a2, s32 a3, s32 a4, s32 a5){u32 a[]={ a0,a1,a2,a3,a4,a5 };return event(0x801dfb68,a,6);}
s32 func_801DFE2C(s32 a0){u32 a[]={ a0 };return event(0x801dfe2c,a,1);}
s32 func_801DFF5C(s32 a0, s32 a1, s32 a2, s32 a3, s32 a4, s32 a5, s32 a6){u32 a[]={ a0,a1,a2,a3,a4,a5,a6 };return event(0x801dff5c,a,7);}
s32 func_801D8DE4(s32 a0, s32 a1, s32 a2, s32 a3){u32 a[]={ a0,a1,a2,a3 };return event(0x801d8de4,a,4);}
s32 func_801D397C(s32 a0, s32 a1, s32 a2, s32 a3, s32 a4, s32 a5, s32 a6, s32 a7, s32 a8){u32 a[]={ a0,a1,a2,a3,a4,a5,a6,a7,a8 };return event(0x801d397c,a,9);}
s32 func_801D1E80(void){u32 a[]={ 0 };return event(0x801d1e80,a,0);}
s32 func_801D29A8(s32 a0, s32 a1){u32 a[]={ a0,a1 };return event(0x801d29a8,a,2);}
s32 func_801D9704(s32 a0, s32 a1, s32 a2){u32 a[]={ a0,a1,a2 };return event(0x801d9704,a,3);}
s32 func_801DF890(s32 a0, s32 a1){u32 a[]={ a0,a1 };return event(0x801df890,a,2);}
s32 func_801DF0D4(s32 a0, s32 a1, s32 a2, s32 a3){u32 a[]={ a0,a1,a2,a3 };return event(0x801df0d4,a,4);}
s32 func_801E0434(s32 a0, s32 a1){u32 a[]={ a0,a1 };return event(0x801e0434,a,2);}
s32 func_801D2484(void){u32 a[]={ 0 };return event(0x801d2484,a,0);}
s32 func_801DB340(s32 a0){u32 a[]={ a0 };return event(0x801db340,a,1);}
#include "equip_core_body.inc"
static int memory(u32 a,unsigned w,u32*v,int write){
 struct {u32 a;void*p;unsigned n;} ranges[]={
 {0x80100325,&menu.input,1},{0x80100329,&menu.transitionEffectState,1},
 {0x80101000,&manager,sizeof manager},{0x80102000,&cursors,sizeof cursors},{0x80103000,work,sizeof work}};
 if(!write&&w==4){
  switch(a){case 0x800625a0:*v=0x80100000;return 0;case 0x80100330:*v=0x80110000;return 0;case 0x8010033c:*v=0x80101000;return 0;case 0x80100428:*v=0x80102000;return 0;case 0x8010035c:*v=0x80103000;return 0;}
 }
 unsigned char*p=NULL;
 for(unsigned i=0;i<sizeof ranges/sizeof ranges[0];i++)if(a>=ranges[i].a&&(uint64_t)a+w<=ranges[i].a+ranges[i].n)p=(unsigned char*)ranges[i].p+a-ranges[i].a;
 if(!p&&a>=0x80000000&&(uint64_t)a+w<=0x80200000)p=ram+(a&0x1fffff);
 if(!p)return -1;
 if(write){for(unsigned i=0;i<w;i++)p[i]=*v>>(8*i);}else{*v=0;for(unsigned i=0;i<w;i++)*v|=(u32)p[i]<<(8*i);if(a>=0x801e05d0&&a<0x801e0f78&&w==4)coverage[(a-0x801e05d0)/4]=1;}
 return 0;
}
static int rd(void*u,u32 a,unsigned w,u32*v){(void)u;return memory(a,w,v,0);}
static int wr(void*u,u32 a,unsigned w,u32 v){(void)u;return memory(a,w,&v,1);}
static int bridge(void*u,PcPortMipsCpu*c,u32 pc){(void)u;unsigned n;
 switch(pc){case 0x801c7bf4:n=0;break;case 0x801e36d4:case 0x801e3a80:n=2;break;
case 0x801de2c8:n=1;break;
case 0x801df5d0:n=2;break;
case 0x801d8ea4:n=4;break;
case 0x801de474:n=2;break;
case 0x801de5cc:n=5;break;
case 0x801d3344:n=3;break;
case 0x801db0a8:n=4;break;
case 0x801dfb68:n=6;break;
case 0x801dfe2c:n=1;break;
case 0x801dff5c:n=7;break;
case 0x801d8de4:n=4;break;
case 0x801d397c:n=9;break;
case 0x801d1e80:n=0;break;
case 0x801d29a8:n=2;break;
case 0x801d9704:n=3;break;
case 0x801df890:n=2;break;
case 0x801df0d4:n=4;break;
case 0x801e0434:n=2;break;
case 0x801d2484:n=0;break;
case 0x801db340:n=1;break;
default:return 0;}
 u32 a[9]={0};for(unsigned i=0;i<n;i++){if(i<4)a[i]=c->gpr[4+i];else if(rd(NULL,c->gpr[29]+16+(i-4)*4,4,&a[i]))return -1;}
 c->gpr[2]=event(pc,a,n);return 1;
}
static void init(void){
 memset(&menu,0,sizeof menu);memset(&manager,0xA5,sizeof manager);memset(&cursors,0x5A,sizeof cursors);memset(work,0x3C,sizeof work);
 menu.pManager=&manager;menu.pCursors=&cursors;menu.unk330=&resources;menu.transitionEffectState=0;
 manager.currentCharacterIDs[0]=seed&8?4:0;manager.currentCharacterIDs[1]=1;manager.currentCharacterIDs[2]=2;cursors.renderContexts[0]=seed&1;
 frames=ec=0;memset(events,0,sizeof events);memset(ram+0x1fe000,seed,0x1000);
}
int main(void){FILE*f=fopen("disc/menu.bin","rb");if(!f)return 2;fseek(f,0,SEEK_END);long size=ftell(f);rewind(f);if(size>0x3b000||fread(ram+0x1c5000,1,size,f)!=(size_t)size)return 2;fclose(f);memcpy(D_801E9DBC,ram+0x1e9dbc,32);
 for(seed=0;seed<400;seed++){
  u32 rng=seed+1;for(unsigned j=0;j<96;j++){rng=rng*1664525+1013904223;script[j]=(rng>>16)%12;}
  /* Reach listing before exercising arbitrary navigation; final cancels terminate both states. */
  script[0]=4;script[1]=1;script[2]=3;
  if(seed>=320){for(unsigned j=1;j<36;j++)script[j]=1;for(unsigned j=36;j<70;j++)script[j]=3;for(unsigned j=70;j<96;j++)script[j]=5;}
  init();PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};PcPortMipsCpu c;PcPortMipsCpuInit(&c,&bus);c.gpr[4]=0;c.gpr[5]=(seed>>4)&1;c.gpr[6]=(seed>>5)&1;c.gpr[29]=0x801ff000;c.gpr[31]=0xfffffffc;
  int rc=PcPortMipsRun(&c,0x801e05d0,0xfffffffc,500000);
  if(rc){fprintf(stderr,"FAIL retail seed=%u %s\n",seed,c.error);return 1;}
  MenuManager em=manager;MenuPointerCursors ep=cursors;unsigned char ew[sizeof work];memcpy(ew,work,sizeof work);unsigned expected_count=ec;memcpy(expected_events,events,sizeof events);u8 ei=menu.input,et=menu.transitionEffectState;
  init();func_801E05D0(0,(seed>>4)&1,(seed>>5)&1);
  if(ec!=expected_count||memcmp(events,expected_events,sizeof events)||memcmp(&manager,&em,sizeof em)||memcmp(&cursors,&ep,sizeof ep)||memcmp(work,ew,sizeof ew)||menu.input!=ei||menu.transitionEffectState!=et){
   fprintf(stderr,"FAIL differential seed=%u events=%u/%u manager=%d cursors=%d work=%d\n",seed,ec,expected_count,memcmp(&manager,&em,sizeof em),memcmp(&cursors,&ep,sizeof ep),memcmp(work,ew,sizeof ew));
   for(unsigned i=0;i<ec&&i<expected_count;i++)if(memcmp(events[i],expected_events[i],sizeof events[i])){fprintf(stderr,"event%u native%08x retail%08x\n",i,events[i][0],expected_events[i][0]);for(unsigned j=0;j<11;j++)fprintf(stderr,"%08x/%08x ",events[i][j],expected_events[i][j]);fputc('\n',stderr);break;}return 1;
  }
 }
 unsigned covered=0;for(unsigned i=0;i<618;i++)covered+=coverage[i]!=0;
 if(covered!=618){fprintf(stderr,"FAIL coverage %u/618\n",covered);return 1;}
 printf("EQUIP CORE PASS cases=400 instruction_coverage=%u/618\n",covered);return 0;}
