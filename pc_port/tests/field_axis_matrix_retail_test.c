/* Raw 73-instruction retail oracle; SDK math boundaries are explicit spies. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "common.h"
#include "battle_mips_adapter.h"
#define ENTRY 0x800759e4u
#define END 0x80075b08u
#define AXIS 0x80180000u
#define MATRIX_ADDR 0x80180110u
#define STACK 0x801ff000u
#define HALT 0xfffffffcu
static uint8_t ram[0x200000],retail[292],seed[16];
static unsigned native_mode,mutation,cases,checks,seen[73];
static PcPortMipsCpu *active_cpu;
static VECTOR axis;
static struct { uint8_t pre[16]; MATRIX matrix; uint8_t post[16]; } arena;
static const void *roles[4];
typedef struct { uint32_t id,role[3],input[8]; } Event;
typedef struct { Event trace[4]; unsigned count; uint8_t matrix[64],axis[16]; } Result;
static Result result;
extern void func_800759E4(MATRIX *,VECTOR *);
extern const VECTOR D_8006FB70;
_Static_assert(sizeof(VECTOR)==16,"VECTOR width");
_Static_assert(sizeof(MATRIX)==32,"MATRIX width");
_Static_assert(__builtin_offsetof(MATRIX,t)==20,"MATRIX padding");
static void require(int ok,const char *why) { checks++; if(!ok) {fprintf(stderr,"FIELD AXIS FAIL case=%u native=%u mutation=%u %s\n",cases,native_mode,mutation,why);exit(1);} }
static uint8_t *ptr(uint32_t a) { return ram+(a&0x1fffffu); }
static uint32_t role(const void *p) {
 for(unsigned i=0;i<4;i++)if(roles[i]==p)return i;
 require(0,"unknown helper pointer");return 99;
}
static void boundary(unsigned id,const uint32_t *a,const uint32_t *b,uint32_t *out,uint32_t ar,uint32_t br,uint32_t cr) {
 unsigned n=result.count;require(n<4,"helper count");Event *e=&result.trace[n];e->id=id;e->role[0]=ar;e->role[1]=br;e->role[2]=cr;
 memcpy(e->input,a,16);if(b)memcpy(e->input+4,b,16);
 /* Deliberately fill all vector words, including pad; no signed arithmetic. */
 uint32_t v[4];for(unsigned i=0;i<4;i++)v[i]=(a[i]^(0x9e3779b9u*(n+1)))+(b?b[(i+1)%4]:0)+(i*0x1020304u);
 memcpy(out,v,16);result.count++;
 if(mutation) {
  uint32_t values[4];void *ap=native_mode?(void *)&axis:(void *)ptr(AXIS);memcpy(values,ap,16);
  for(unsigned i=0;i<4;i++)values[i]=(values[i]+(n+1)*0x81238127u)^(i*0x65432101u);
  memcpy(ap,values,16);
 }
}
void OuterProduct12(VECTOR *a,VECTOR *b,VECTOR *out) {
 if(result.count==0){roles[0]=a;roles[1]=&axis;roles[2]=out;}
 boundary(1,(const uint32_t *)a,(const uint32_t *)b,(uint32_t *)out,role(a),role(b),role(out));
}
long VectorNormal(VECTOR *a,VECTOR *out) {
 if(result.count==1)roles[3]=out;
 boundary(2,(const uint32_t *)a,NULL,(uint32_t *)out,role(a),99,role(out));return -12345;
}
static int read_bus(void *o,uint32_t a,unsigned width,uint32_t *v) {
 (void)o;if(a<0x80000000u||(uint64_t)a+width>0x80200000u)return -1;
 *v=0;for(unsigned i=0;i<width;i++)*v|=(uint32_t)ptr(a)[i]<<(8*i);
 if(active_cpu&&width==4&&a==active_cpu->pc&&a>=ENTRY&&a<END)seen[(a-ENTRY)/4]++;
 return 0;
}
static int write_bus(void *o,uint32_t a,unsigned width,uint32_t v) {
 (void)o;if(a<0x80000000u||(uint64_t)a+width>0x80200000u)return -1;
 for(unsigned i=0;i<width;i++)ptr(a)[i]=(uint8_t)(v>>(8*i));return 0;
}
static uint32_t rawrole(uint32_t a) {
 if(a==STACK-0x58+0x10)return 0;if(a==AXIS)return 1;
 if(a==STACK-0x58+0x30)return 2;if(a==STACK-0x58+0x20)return 3;
 require(0,"unexpected retail helper pointer");return 99;
}
static int bridge(void *o,PcPortMipsCpu *c,uint32_t target) {
 (void)o;unsigned id;if(target==0x8004a480u)id=1;else if(target==0x80048d7cu)id=2;else return 0;
 uint32_t a[4],b[4],v[4];memcpy(a,ptr(c->gpr[4]),16);if(id==1)memcpy(b,ptr(c->gpr[5]),16);
 uint32_t out=c->gpr[id==1?6:5];
 boundary(id,a,id==1?b:NULL,v,rawrole(c->gpr[4]),id==1?rawrole(c->gpr[5]):99,rawrole(out));
 memcpy(ptr(out),v,16);c->gpr[2]=0xffffcfc7u;return 1;
}
static Result run(unsigned native,const uint32_t input[4],unsigned pattern) {
 native_mode=native;memset(&result,0,sizeof(result));memset(roles,0,sizeof(roles));memset(ram,0x5b,sizeof(ram));
 memset(&arena,pattern,sizeof(arena));memset(ptr(MATRIX_ADDR-16),pattern,sizeof(arena));memcpy(&axis,input,16);memcpy(ptr(AXIS),input,16);
 if(native)func_800759E4(&arena.matrix,&axis);
 else {
  memcpy(ptr(ENTRY),retail,292);memcpy(ptr(0x8006fb70),seed,16);
  PcPortMipsBus bus={0};PcPortMipsCpu cpu;bus.read=read_bus;bus.write=write_bus;bus.bridge=bridge;
  PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[4]=MATRIX_ADDR;cpu.gpr[5]=AXIS;cpu.gpr[29]=STACK;cpu.gpr[31]=HALT;active_cpu=&cpu;
  int rc=PcPortMipsRun(&cpu,ENTRY,HALT,1000);active_cpu=NULL;require(rc==PC_PORT_MIPS_HALTED,"retail run");require(cpu.gpr[29]==STACK,"retail stack restore");
 }
 require(result.count==4,"four SDK boundaries");
 memcpy(result.matrix,native?(void *)&arena:(void *)ptr(MATRIX_ADDR-16),sizeof(arena));memcpy(result.axis,native?(void *)&axis:(void *)ptr(AXIS),16);
 for(unsigned i=0;i<64;i++)if(i<16||i>=34)require(result.matrix[i]==(uint8_t)pattern,"padding/translation/redzone preservation");
 return result;
}
int main(int argc,char **argv) {
 require(argc==2,"retail field module argument");FILE *f=fopen(argv[1],"rb");require(f!=NULL,"retail open");require(fseek(f,0x5ef4,SEEK_SET)==0 && fread(retail,1,292,f)==292,"retail function read");require(fseek(f,0x80,SEEK_SET)==0 && fread(seed,1,16,f)==16,"retail seed read");fclose(f);
 uint32_t values[]={0,1,4096,0x7fff,0x8000,0xffff,0x7fffffff,0x80000000u,0xffffffffu,0x12345678u};
 require(memcmp(&D_8006FB70,seed,16)==0,"production retail seed");
 for(unsigned i=0;i<10;i++)for(unsigned j=0;j<10;j++)for(mutation=0;mutation<2;mutation++) {
  uint32_t v[4]={values[i],values[j],values[(i+j)%10],values[(i*3+j)%10]};
  Result raw=run(0,v,(i+j)%2?0xa5:0x3c),native=run(1,v,(i+j)%2?0xa5:0x3c);
  require(memcmp(&raw,&native,sizeof(raw))==0,"native/raw complete state and helper trace");cases++;
 }
 for(unsigned i=0;i<73;i++)require(seen[i]>0,"raw instruction coverage");
 printf("FIELD AXIS PASS cases=%u checks=%u instructions=73\n",cases,checks);return 0;
}
