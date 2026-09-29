#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#ifndef OBJECT_OVERLAY_SOURCE
#define OBJECT_OVERLAY_SOURCE "../src/field_object_overlay.c"
#endif
#define uid_t psyq_test_uid_t
#define gid_t psyq_test_gid_t
#include OBJECT_OVERLAY_SOURCE
#undef uid_t
#undef gid_t
#include "battle_mips_adapter.h"
extern void func_801E1A14(u8*,u16*,s32,s32,s32,s32,s32,s32,s32,s32,s32,s32,s32,s32,s32,s32,s32,s32,s32,s32) __attribute__((weak));
static u8 ram[0x200000];
static struct {u32 owner[9];u16 source[256];u32 blocks[5][2048];} fixture,initial,expected;
static struct Call {u32 kind,a,b,c,d,owner[9];} calls[512],expectedCalls[512];
static unsigned count,allocations,failAt;
static void record(u32 k,u32 a,u32 b,u32 c,u32 d){assert(count<512);calls[count]=(struct Call){k,a,b,c,d,{0}};memcpy(calls[count++].owner,fixture.owner,36);}
void* HeapAlloc(u_int n,u_int a){record(1,n,a,0,0);assert(allocations<5);void*p=fixture.blocks[allocations++];assert(n<=sizeof(fixture.blocks[0]));return allocations==failAt?NULL:p;}
u_int HeapFree(void*p){record(2,(u32)(uintptr_t)p,0,0,0);return 1;}
void HeapChangeCurrentUser(u_int a,char**b){record(3,a,(u32)(uintptr_t)b,0,0);}
static u8* address(u32 a,unsigned w){if(a>=0x80000000u&&(uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);uintptr_t lo=(uintptr_t)&fixture;if(a>=lo&&(uint64_t)a+w<=lo+sizeof(fixture))return(u8*)(uintptr_t)a;return NULL;}
static int rd(void*u,u32 a,unsigned w,u32*v){(void)u;u8*p=address(a,w);if(!p)return-1;*v=0;for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(i*8);return 0;}
static int wr(void*u,u32 a,unsigned w,u32 v){(void)u;u8*p=address(a,w);if(!p)return-1;for(unsigned i=0;i<w;++i)p[i]=(u8)(v>>(i*8));return 0;}
/* Shared retail SDK boundary; this fixture does not certify native LIBGPU. */
static u32 sdk(u32 pc,u32 a,u32 b,u32 c,u32 d){PcPortMipsBus bus={.read=rd,.write=wr};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[4]=a;cpu.gpr[5]=b;cpu.gpr[6]=c;cpu.gpr[7]=d;cpu.gpr[29]=0x801fe000;cpu.gpr[31]=0xfffffffcu;assert(PcPortMipsRun(&cpu,pc,0xfffffffcu,1000)==PC_PORT_MIPS_HALTED);return cpu.gpr[2];}
u_short GetTPage(int a,int b,int c,int d){return sdk(0x80043a1c,a,b,c,d);}
u_short GetClut(int a,int b){return sdk(0x80043a58,a,b,0,0);}
void SetPolyGT3(POLY_GT3*p){sdk(0x80043c88,(u32)(uintptr_t)p,0,0,0);}
static int bridge(void*u,PcPortMipsCpu*c,u32 t){(void)u;switch(t){case 0x80032498:HeapChangeCurrentUser(c->gpr[4],(void*)(uintptr_t)c->gpr[5]);return 1;case 0x80031bdc:c->gpr[2]=(u32)(uintptr_t)HeapAlloc(c->gpr[4],c->gpr[5]);return 1;case 0x800320e8:c->gpr[2]=HeapFree((void*)(uintptr_t)c->gpr[4]);return 1;default:return 0;}}
int main(void){
 if(!func_801E1A14){fputs("GEOMETRY CONSTRUCTOR FAIL missing native owner\n",stderr);return 1;}
 FILE*f=fopen("disc/disc1.bin","rb");assert(f);for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));
 /* Real SDK leaf instructions, rather than a guessed packet initializer. */
 f=fopen("disc/SLUS_006.64","rb");assert(f);assert(!fseek(f,0x80043a1c-0x8000f800,SEEK_SET));assert(fread(ram+0x43a1c,1,0x300,f)==0x300);assert(!fclose(f));
 unsigned cases=0;const s32 scales[]={4096,-4096,8192,0,0x7fffffff};
 for(unsigned chains=2;chains<=4;++chains)for(unsigned seed=0;seed<12;++seed)for(failAt=0;failAt<=5;++failAt)for(unsigned sc=0;sc<5;++sc){
  memset(&fixture,0xa5,sizeof(fixture));u16*p=fixture.source;*p++=chains;u16*faces=p++;unsigned total=0,pairs=0,counts[4];
  for(unsigned i=0;i<chains;++i)for(unsigned j=0;j<3;++j)*p++=(u16)(seed*9001+i*312+j*21111);
  for(unsigned i=0;i<chains;++i){counts[i]=1+(seed+i)%3;total+=counts[i];*p++=counts[i];if(i)pairs+=counts[i]<counts[i-1]?counts[i]:counts[i-1];}
  *faces=pairs;*p++=total;for(unsigned i=0;i<total;++i)*p++=(u16)(seed*8191+i*137);u8*q=(u8*)p;for(unsigned i=0;i<total;++i)*q++=(u8)(seed*37+i);
  s32 args[16]={-32768+(s32)seed,32767,-1,(s32)(seed%4)-1,-65+(s32)seed*73,-257+(s32)seed*97,127,-113,seed*41,seed*13,0x81,0x47,0xee,0xfe,0x12,0x91};
  initial=fixture;count=allocations=0;memset(calls,0,sizeof(calls));PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
  cpu.gpr[4]=(u32)(uintptr_t)fixture.owner;cpu.gpr[5]=(u32)(uintptr_t)fixture.source;cpu.gpr[6]=0x7fff+seed;cpu.gpr[7]=scales[sc];cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
  for(unsigned i=0;i<16;++i)assert(!wr(NULL,cpu.gpr[29]+16+i*4,4,args[i]));
  if(PcPortMipsRun(&cpu,0x801e1a14,0xfffffffcu,100000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"GEOMETRY CONSTRUCTOR FAIL oracle %s pc=%x\n",cpu.error,cpu.pc);return 1;}
  expected=fixture;memcpy(expectedCalls,calls,sizeof(calls));unsigned expectedCount=count;fixture=initial;count=allocations=0;memset(calls,0,sizeof(calls));
  func_801E1A14((u8*)fixture.owner,fixture.source,0x7fff+seed,scales[sc],args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9],args[10],args[11],args[12],args[13],args[14],args[15]);
  if(memcmp(&fixture,&expected,sizeof(fixture))||count!=expectedCount||memcmp(calls,expectedCalls,sizeof(calls))){unsigned at=0;while(at<sizeof(fixture)&&((u8*)&fixture)[at]==((u8*)&expected)[at])++at;fprintf(stderr,"GEOMETRY CONSTRUCTOR FAIL chains=%u seed=%u fail=%u scale=%u byte=%x calls=%u/%u\n",chains,seed,failAt,sc,at,count,expectedCount);return 1;}++cases;
 }
 for(unsigned kind=0;kind<2;++kind){
  memset(&fixture,0,sizeof(fixture));fixture.source[0]=kind?2:1;
  /* All segment counts are zero: one chain faults on horizontal division;
   * two chains reach the zero-minimum vertical division. */
  initial=fixture;failAt=count=allocations=0;
  PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
  cpu.gpr[4]=(u32)(uintptr_t)fixture.owner;cpu.gpr[5]=(u32)(uintptr_t)fixture.source;cpu.gpr[7]=4096;cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffcu;
  for(unsigned i=0;i<16;++i)assert(!wr(NULL,cpu.gpr[29]+16+i*4,4,0));
  int result=PcPortMipsRun(&cpu,0x801e1a14,0xfffffffcu,100000);
  char error[64];snprintf(error,sizeof(error),"break at pc=0x%08x",kind?0x801e1f6c:0x801e1ef0);
  if(result!=PC_PORT_MIPS_UNSUPPORTED||strcmp(cpu.error,error)){fprintf(stderr,"GEOMETRY CONSTRUCTOR FAIL retail divide fault %u: %s\n",kind,cpu.error);return 1;}
  pid_t child=fork();assert(child>=0);
  if(!child){struct rlimit limit={0,0};assert(!setrlimit(RLIMIT_CORE,&limit));fixture=initial;count=allocations=0;func_801E1A14((u8*)fixture.owner,fixture.source,0,4096,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0);_exit(0);}
  int status;assert(waitpid(child,&status,0)==child);
  if(!WIFSIGNALED(status)||WTERMSIG(status)!=SIGILL){fprintf(stderr,"GEOMETRY CONSTRUCTOR FAIL native divide fault %u status=%x\n",kind,status);return 1;}
 }
 printf("GEOMETRY CONSTRUCTOR PASS %u cases and two divide faults; full memory and heap calls; retail SDK leaves, controlled returning heap\n",cases);
}
