#include "../src/battle_mips_runtime.c"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];
int g_GPUDisabledState;
unsigned g_PcPortPresentedFrames;
void PcPort_PadVblankPump(void) {}
char PsyX_BeginScene(void){abort();} void ClearSplits(void){abort();}
void DrawAllSplits(void){abort();} void ParsePrimitivesLinkedList(u_long*p,int s){(void)p;(void)s;abort();}
unsigned MFC2(int r){(void)r;abort();} unsigned CFC2(int r){(void)r;abort();}
void MTC2(unsigned v,int r){(void)v;(void)r;abort();} void CTC2(unsigned v,int r){(void)v;(void)r;abort();}
int doCOP2(int o){(void)o;abort();}
void PcPort_GodModeBeforeGuest(PcPortMipsCpu*cpu,uint32_t target){(void)cpu;(void)target;}

extern void func_80025258(uint8_t *entry);
extern void func_80025224(void *task,int type);
uint8_t D_800C3664; int32_t D_80050100; MATRIX D_8004FBB8; u_long *g_GfxCurOT;

typedef struct {uint32_t target,a0,a1;int16_t x,y,z;} Call;
static Call calls[16];static unsigned call_count;static long test_flag;static int test_otz;
static int mutate_flags;static uint32_t mutated_flags;
static void note(uint32_t t,uintptr_t a0,uintptr_t a1){Call*c=&calls[call_count++];memset(c,0,sizeof *c);c->target=t;c->a0=(uint32_t)a0;c->a1=(uint32_t)a1;}
void SetRotMatrix(MATRIX*m){note(0x80049efc,(uintptr_t)m,0);}
void SetTransMatrix(MATRIX*m){note(0x80049f8c,(uintptr_t)m,0);}
int RotTransPers(SVECTOR*v,int*p,long*q,long*f){uint32_t flag=(uint32_t)test_flag;note(0x8004a64c,(uintptr_t)v,(uintptr_t)p);calls[call_count-1].x=v->vx;calls[call_count-1].y=v->vy;calls[call_count-1].z=v->vz;memcpy(q,&flag,4);memcpy(f,&flag,4);return test_otz;}
MATRIX *TransMatrix(MATRIX*m,VECTOR*v){note(0x80049d9c,(uintptr_t)m,(uintptr_t)v);calls[call_count-1].x=(int16_t)v->vx;calls[call_count-1].y=(int16_t)v->vy;calls[call_count-1].z=(int16_t)v->vz;return m;}
void func_80022038(void*p){note(0x80022038,(uintptr_t)p,0);if(mutate_flags)memcpy((uint8_t*)p+0x3c,&mutated_flags,4);}
void func_8001E3D8(void*p,void*ot){note(0x8001e3d8,(uintptr_t)p,(uintptr_t)ot);}
void func_8001E298(void*p,void*ot){note(0x8001e298,(uintptr_t)p,(uintptr_t)ot);}
void func_80025710(void*p){(void)p;} void func_80025718(void*p){(void)p;}
void func_8002541C(void*p){(void)p;} void func_80025544(void*p){(void)p;} void func_800257F0(void*p){(void)p;}
static void(*registered)(void*);
void WorkListSetTaskCallback(void*task,void(*cb)(void*)){assert(task==PSX_ADDR(0x80170000));registered=cb;}

static int leaf_bridge(void*opaque,PcPortMipsCpu*cpu,uint32_t target)
{
 BattleMipsRuntime*r=opaque;PcPortMipsCpu*old=r->bridge_cpu;r->bridge_cpu=cpu;int handled=1;
 if(target==0x80049efcu)SetRotMatrix((MATRIX*)resolve_memory(r,cpu->gpr[4],sizeof(MATRIX),1));
 else if(target==0x80049f8cu)SetTransMatrix((MATRIX*)resolve_memory(r,cpu->gpr[4],sizeof(MATRIX),1));
 else if(target==0x8004a64cu){SVECTOR*v=(SVECTOR*)resolve_memory(r,cpu->gpr[4],sizeof(SVECTOR),1);note(target,(uintptr_t)v,(uintptr_t)resolve_memory(r,cpu->gpr[5],4,1));calls[call_count-1].x=v->vx;calls[call_count-1].y=v->vy;calls[call_count-1].z=v->vz;runtime_write(r,cpu->gpr[6],4,(uint32_t)test_flag);runtime_write(r,cpu->gpr[7],4,(uint32_t)test_flag);cpu->gpr[2]=(uint32_t)test_otz;}
 else if(target==0x80022038u)func_80022038(resolve_memory(r,cpu->gpr[4],0xb4,1));
 else if(target==0x80049d9cu)TransMatrix((MATRIX*)resolve_memory(r,cpu->gpr[4],sizeof(MATRIX),1),(VECTOR*)resolve_memory(r,cpu->gpr[5],sizeof(VECTOR),1));
 else if(target==0x8001e3d8u)func_8001E3D8(resolve_memory(r,cpu->gpr[4],0xb4,1),resolve_memory(r,cpu->gpr[5],4,1));
 else if(target==0x8001e298u)func_8001E298(resolve_memory(r,cpu->gpr[4],0xb4,1),resolve_memory(r,cpu->gpr[5],4,1));
 else if(target>=0x80025258u&&target<0x8002541cu)handled=0;
 else handled=runtime_bridge_call(r,cpu,target);
 r->bridge_cpu=old;return handled;
}

static int oracle(BattleMipsRuntime*r,uint32_t entry){PcPortMipsCpu c;initialize_cpu(&c,r);c.bus.bridge=leaf_bridge;c.gpr[4]=entry;c.gpr[29]=0x801ff000;c.gpr[31]=BATTLE_HALT_PC;return PcPortMipsRun(&c,0x80025258,BATTLE_HALT_PC,2000);}
typedef struct {const char*name;uint32_t flags_b0,flags3c,mutated;int global,mutate,otz,gte_flag,base_depth,sub_depth,kseg1;} Case;
static int guards_ok(uint32_t a,unsigned n){for(unsigned i=0;i<n;i++)if(*(uint8_t*)PSX_ADDR(a+i)!=0xcc)return 0;return 1;}
static unsigned one_case(BattleMipsRuntime*r,const Case*c)
{
 const uint32_t alias=c->kseg1?0x20000000u:0u,entry=0x80170000u|alias,sprite=0x80170100u|alias,model=0x80172000u|alias,sub=0x80173000u|alias,ot=0x80174000u|alias;
 uint8_t initial[0xc0],expected[0xc0];Call trace[16];unsigned n;
 memset(initial,0,sizeof initial);store_le(initial+2,2,11);store_le(initial+6,2,22);store_le(initial+10,2,33);
 store_le(initial+0x20,4,model);store_le(initial+0x30,2,(uint16_t)c->base_depth);store_le(initial+0x3c,4,c->flags3c);
 store_le(initial+0x70,4,sub);store_le(initial+0xb0,4,c->flags_b0);
 memset(PSX_ADDR(entry-16),0xcc,48);store_le(PSX_ADDR(entry+4),4,sprite); /* +8..+15 stay poisoned: packed pointer is 32-bit. */
 memset(PSX_ADDR(sprite-16),0xcc,0xf0);memcpy(PSX_ADDR(sprite),initial,sizeof initial);
 memset(PSX_ADDR(model-16),0xcc,80);memset(PSX_ADDR(model),0,48);
 memset(PSX_ADDR(sub-16),0xcc,80);memset(PSX_ADDR(sub),0,48);store_le(PSX_ADDR(sub+0x2e),2,(uint16_t)c->sub_depth);
 memset(PSX_ADDR(ot-16),0xcc,16);memset(PSX_ADDR(ot+4096u*4u),0xcc,16);
 *(uint8_t*)PSX_ADDR(0x800c3664u)=(uint8_t)c->global;D_800C3664=(uint8_t)!c->global;
 D_80050100=0;g_GfxCurOT=PSX_ADDR(ot);test_otz=c->otz;test_flag=c->gte_flag;mutate_flags=c->mutate;mutated_flags=c->mutated;
 call_count=0;if(oracle(r,entry)!=PC_PORT_MIPS_HALTED)return 100;
 memcpy(expected,PSX_ADDR(sprite),sizeof expected);n=call_count;memcpy(trace,calls,sizeof trace);
 memcpy(PSX_ADDR(sprite),initial,sizeof initial);*(uint8_t*)PSX_ADDR(0x800c3664u)=(uint8_t)c->global;D_800C3664=(uint8_t)!c->global;call_count=0;
 func_80025258(PSX_ADDR(entry));
 if(!guards_ok(entry-16,16)||!guards_ok(entry+8,24)||!guards_ok(sprite-16,16)||!guards_ok(sprite+0xc0,32)||!guards_ok(model-16,16)||!guards_ok(model+48,16)||!guards_ok(sub-16,16)||!guards_ok(sub+48,16)||!guards_ok(ot-16,16)||!guards_ok(ot+4096u*4u,16))return 101;
 if(memcmp(expected,PSX_ADDR(sprite),sizeof expected)||call_count!=n)return 1;
 for(unsigned i=0;i<n;i++){
  if(calls[i].target!=trace[i].target||calls[i].x!=trace[i].x||calls[i].y!=trace[i].y||calls[i].z!=trace[i].z)return 2;
  if(calls[i].target!=0x8004a64cu&&calls[i].a0!=trace[i].a0)return 2;
  if(calls[i].target==0x8001e3d8u||calls[i].target==0x8001e298u){
   if(calls[i].a0!=(uint32_t)(uintptr_t)PSX_ADDR(sprite)||trace[i].a0!=(uint32_t)(uintptr_t)PSX_ADDR(sprite)||calls[i].a1!=trace[i].a1)return 3;
   uintptr_t off=(uintptr_t)calls[i].a1-(uintptr_t)PSX_ADDR(ot);if(off>4095u*4u||off%4u)return 4;
  }
 }
 return 0;
}
int main(void)
{
 static const Case cases[]={
  {"normal-depth-1",0,0,0,0,0,1,0,0,1,0},
  {"normal-depth-4095",0,0,0,0,0,4094,0,1,1,0},
  {"normal-depth-0-reject",0,0,0,0,0,0,0,0,1,0},
  {"normal-depth-4096-reject",0,0,0,0,0,4095,0,1,1,0},
  {"gte-rejection",0,0,0,0,0,55,0x8000,7,1,0},
  {"suppressed",0x100,0,0,1,0,55,0,7,1,0},
  {"suppression-global-clear",0x100,0,0,0,0,1,0,0,1,0},
  {"suppression-bit-clear",0,0,0,1,0,1,0,0,1,0},
  {"matrix-base-depth-1",0,0x01000000,0,0,0,99,0,1,1,0},
  {"matrix-base-depth-4095",0,0x01000000,0,0,0,99,0,4095,1,0},
  {"matrix-base-depth-0",0,0x01000000,0,0,0,99,0,0,1,0},
  {"matrix-base-depth-4096",0,0x01000000,0,0,0,99,0,4096,1,0},
  {"matrix-bit25-depth4095",0,0x03000000,0,0,0,99,0,7,1,0},
  {"matrix-reloaded-bit25",0,0x01000000,0x03000000,0,1,99,0,7,1,0},
  {"sub-depth-1",0,0x20000000,0,0,0,99,0,7,1,0},
  {"sub-depth-4095",0,0x20000000,0,0,0,99,0,7,4095,0},
  {"sub-depth-0",0,0x20000000,0,0,0,99,0,7,0,0},
  {"sub-depth-4096",0,0x20000000,0,0,0,99,0,7,4096,0},
  {"flag24-wins-over29",0,0x21000000,0,0,0,99,0,7,1,0},
  {"kseg1-packed-alias",0,0,0,0,0,4,0,1,1,1},
 };
 FILE*f=fopen("disc/SLUS_006.64","rb");assert(f);assert(!fseek(f,0x80025258u-0x8000f800u,SEEK_SET));assert(fread(PSX_ADDR(0x80025258),1,0x1c4,f)==0x1c4);fclose(f);
 initialize_runtime(&g_BattleRuntime);unsigned fail=0;void(*expected[16])(void*)={(void(*)(void*))func_80025258,func_80025710,func_80025718,0,0,(void(*)(void*))func_80025258,(void(*)(void*))func_80025258,func_80025718,func_8002541C,func_80025544,0,0,0,0,(void(*)(void*))func_80025258,(void(*)(void*))func_800257F0};
 for(int i=0;i<16;i++){registered=(void*)1;func_80025224(PSX_ADDR(0x80170000),i);if(registered!=expected[i])fail++;}
 for(unsigned i=0;i<sizeof cases/sizeof cases[0];i++){unsigned rc=one_case(&g_BattleRuntime,&cases[i]);if(rc){fprintf(stderr,"BATTLE CHILD BILLBOARD case %s failed rc=%u\n",cases[i].name,rc);fail++;}}
 if(fail){fprintf(stderr,"BATTLE CHILD BILLBOARD RED failures=%u\n",fail);return 1;}puts("BATTLE CHILD BILLBOARD PASS bindings, suppression, flags, depths 1..4095, OT byte stride, KSEG aliases");return 0;
}
