#define _GNU_SOURCE
#include <sys/mman.h>
#ifndef MODEL_RELOCATION_SOURCE
#define MODEL_RELOCATION_SOURCE "../src/field_object_overlay.c"
#endif
#include MODEL_RELOCATION_SOURCE
#include "battle_mips_adapter.h"
#define B 0x100000u
#define OLD (B+0x1000)
#define NEW (B+0x3000)
#define OT (B+0x5000)
#define NT (B+0x6000)
#define OBJ (B+0x7000)
#define TAB (B+0x8000)
static u8 ram[0x200000];
static unsigned count,size,allocs,n,mode,cases;
typedef struct {u32 id,a,b;} Event;
static Event events[32],expected[32];
static void ck(int ok,const char*s){if(!ok){fprintf(stderr,"MODEL RELOCATION FAIL case=%u mode=%u %s\n",cases,mode,s);exit(1);}}
static void ev(u32 id,u32 a,u32 b){ck(n<32,"event limit");events[n++]=(Event){id,a,b};}
static u32 addr(void*p){return (u32)(uintptr_t)p;}
static void put(u8*p,u32 a,u32 v){memcpy(p+a,&v,4);}
static u32 get(u8*p,u32 a){u32 v;memcpy(&v,p+a,4);return v;}
s32 func_8002C644(u8*p){ev(1,addr(p),0);return 0;}
s32 func_8002C4BC(u8*p){ev(2,addr(p),0);return count;}
void* HeapGetNextBlockHeader(HeapBlock*p){ev(3,addr(p),0);return(void*)(uintptr_t)size;}
void* HeapAlloc(u32 s,u32 f){ev(4,s,f);return(void*)(uintptr_t)(allocs++?NT:NEW);}
u_int HeapFree(void*p){ev(5,addr(p),0);return 0;}
void HeapChangeCurrentUser(u32 u,char**p){ev(6,u,addr(p));}
int func_8002C3E8(u8*p){ev(7,addr(p),0);return count;}
static int rd(void*o,u32 a,unsigned w,u32*v){(void)o;a&=0x1fffff;*v=0;memcpy(v,ram+a,w);return 0;}
static int wr(void*o,u32 a,unsigned w,u32 v){(void)o;a&=0x1fffff;memcpy(ram+a,&v,w);return 0;}
static int bridge(void*o,PcPortMipsCpu*c,u32 t){
 (void)o;u32 a=c->gpr[4],b=c->gpr[5];
 switch(t){
 case 0x8002c644:ev(1,a,0);c->gpr[2]=0;break;
 case 0x8002c4bc:ev(2,a,0);c->gpr[2]=count;break;
 case 0x80031894:ev(3,a,0);c->gpr[2]=size;break;
 case 0x80031bdc:ev(4,a,b);c->gpr[2]=allocs++?NT:NEW;break;
 case 0x800320e8:ev(5,a,0);c->gpr[2]=0;break;
 case 0x80032498:ev(6,a,b);break;
 case 0x8002c3e8:ev(7,a,0);c->gpr[2]=count;break;
 case 0x8003f968:memcpy(ram+(a&0x1fffff),ram+(b&0x1fffff),c->gpr[6]);break;
 default:return 0;
 }return 1;
}
int main(int argc,char**argv){
 ck(argc==2,"overlay argument");FILE*f=fopen(argv[1],"rb");ck(f!=NULL,"open overlay");ck(fread(ram+0x1dc000,1,50868,f)==50868,"read overlay");fclose(f);
 u8*host=mmap((void*)(uintptr_t)B,0x10000,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0);ck(host==(void*)(uintptr_t)B,"native mapping");
 for(unsigned flags=0;flags<8;flags++)for(count=0;count<9;count++)for(unsigned seed=0;seed<8;seed++)for(unsigned nulltab=0;nulltab<2;nulltab++){
  cases++;size=512+seed*16;memset(host,0x5a,0x10000);for(unsigned i=0;i<size;i++)host[OLD-B+i]=(u8)(i*29+seed);
  put(host,OBJ-B,TAB);put(host,OBJ-B+0xa8,0x12345678);put(host,TAB-B,nulltab?0:OT);put(host,TAB-B+4,count);
  memcpy(ram+B,host,0x10000);put(ram,0x1e8638,OLD);put(ram,0x1ff058,flags);
  PcPortMipsBus bus={0};bus.read=rd;bus.write=wr;bus.bridge=bridge;PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[29]=0x801ff000;cpu.gpr[19]=OBJ;
  mode=0;n=allocs=0;int rc=PcPortMipsRun(&cpu,0x801e7c40,0x801e7ce0,10000);if(rc)fprintf(stderr,"oracle %d %s\n",rc,cpu.error);ck(rc==0,"retail execution");unsigned en=n;memcpy(expected,events,sizeof(events));
  OvlyPtrTab*tab=(OvlyPtrTab*)(uintptr_t)TAB;tab->ptrs=(u32*)(uintptr_t)(nulltab?0:OT);tab->count=count;
  mode=1;n=allocs=0;OvlyFinalizeModelCopy((u8*)(uintptr_t)OBJ,(u8*)(uintptr_t)OLD,flags);
  ck(n==en && memcmp(events,expected,n*sizeof(Event))==0,"allocation/helper order");ck(get(host,OBJ-B+0xa8)==get(ram,OBJ+0xa8),"model owner");
  ck(memcmp(host+NEW-B,ram+NEW,size)==0,"copied model bytes");
  if(!(flags&2)){ck(addr(tab->ptrs)==get(ram,TAB),"registry pointer");ck((u32)tab->count==get(ram,TAB+4),"registry count");ck(memcmp((void*)(uintptr_t)NT,ram+NT,count*4)==0,"rebuilt model pointers");}
 }
 printf("MODEL RELOCATION PASS cases=%u\n",cases);return 0;
}
