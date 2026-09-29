#define PcPortMipsRun BattleSpriteTestMipsRun
#include "../src/battle_mips_runtime.c"
#undef PcPortMipsRun
extern int PcPortMipsRun(PcPortMipsCpu *, uint32_t, uint32_t, uint64_t);

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];
int g_GPUDisabledState;
/* Runtime diagnostics/pad-pump hooks referenced by battle_mips_runtime.c.
 * Inert here: the hooks path presents no frames and reads no pads. */
unsigned g_PcPortPresentedFrames;
void PcPort_PadVblankPump(void) {}
char PsyX_BeginScene(void){abort();} void ClearSplits(void){abort();}
void DrawAllSplits(void){abort();}
void ParsePrimitivesLinkedList(u_long *p,int s){(void)p;(void)s;abort();}
unsigned MFC2(int r){(void)r;abort();} unsigned CFC2(int r){(void)r;abort();}
void MTC2(unsigned v,int r){(void)v;(void)r;abort();}
void CTC2(unsigned v,int r){(void)v;(void)r;abort();} int doCOP2(int o){(void)o;abort();}

typedef struct { uint32_t target, a0, a1, a2, sp; int16_t x,y,z; } Call;
static Call calls[16]; static unsigned call_count;
static int primary_floor, fallback_floor; static int use_fallback;
static uint32_t callback_marker;
static void guard_fill(uint32_t address,unsigned size){memset(PSX_ADDR(address),0xd3,size);}
static int guard_ok(uint32_t address,unsigned size){for(unsigned i=0;i<size;i++)if(*(uint8_t*)PSX_ADDR(address+i)!=0xd3)return 0;return 1;}

static void note(uint32_t target, uintptr_t a0, uintptr_t a1, uintptr_t a2)
{
    Call *c=&calls[call_count++]; memset(c,0,sizeof(*c));c->target=target;c->a0=(uint32_t)a0;
    c->a1=(uint32_t)a1; c->a2=(uint32_t)a2;
    c->sp=g_BattleRuntime.bridge_cpu ? g_BattleRuntime.bridge_cpu->gpr[29] : 0;
}
static void note_xyz(const int16_t *pos)
{ Call *c=&calls[call_count-1];c->x=pos[0];c->y=pos[1];c->z=pos[2]; }
int func_800A5914(int16_t *pos,int previous,int mode)
{ note(0x800a5914u,(uintptr_t)pos,previous,mode);note_xyz(pos);return use_fallback?-1:primary_floor; }
int func_800A579C(int16_t *pos)
{ note(0x800a579cu,(uintptr_t)pos,0,0);note_xyz(pos);return fallback_floor; }
void func_800A5870(int16_t *pos,int floor,void *out)
{ note(0x800a5870u,(uintptr_t)pos,floor,(uintptr_t)out);note_xyz(pos);pos[1]=(int16_t)floor; }
void WorkListTaskSetOnFreeCallback(void *w,uintptr_t cb)
{ note(0x8001cd74u,(uintptr_t)w,cb,0); }
void TimerWorkListSetTaskCallback(void *w,uintptr_t cb)
{ note(0x8001cd6cu,(uintptr_t)w,cb,0); }
void func_800BC2F0(int n){ note(0x800bc2f0u,n,0,0); }
uintptr_t func_80011100(uintptr_t arg)
{ note(0x80011100u,arg,0,0); callback_marker=0xcafebabeu; return 0; }

static int controlled_bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
    BattleMipsRuntime *r=opaque; PcPortMipsCpu *previous=r->bridge_cpu;r->bridge_cpu=cpu;
    int handled=1;
    if(target==0x800a5914u)cpu->gpr[2]=(uint32_t)func_800A5914(
        (int16_t*)resolve_memory(r,cpu->gpr[4],6,1),(int)cpu->gpr[5],(int)cpu->gpr[6]);
    else if(target==0x800a579cu)cpu->gpr[2]=(uint32_t)func_800A579C(
        (int16_t*)resolve_memory(r,cpu->gpr[4],6,1));
    else if(target==0x800a5870u)func_800A5870(
        (int16_t*)resolve_memory(r,cpu->gpr[4],6,1),(int)cpu->gpr[5],
        resolve_memory(r,cpu->gpr[6],8,1));
    else if(target==0x800bc2f0u)func_800BC2F0((int)cpu->gpr[4]);
    else handled=runtime_bridge_call(r,cpu,target);
    r->bridge_cpu=previous;return handled;
}

/* The production dispatcher creates a fresh CPU. Keep its retail leaf boundary
 * identical to the direct retail oracle without changing any guest instruction. */
int BattleSpriteTestMipsRun(PcPortMipsCpu *cpu,uint32_t pc,uint32_t halt,uint64_t limit)
{ cpu->bus.bridge=controlled_bridge;return PcPortMipsRun(cpu,pc,halt,limit); }

extern void func_800BA8F4(void *sprite);
extern void func_800BC158(void *wrapper);

static void load_range(FILE *f,uint32_t start,uint32_t end)
{ assert(!fseek(f,start-BATTLE_BASE,SEEK_SET)); assert(fread(PSX_ADDR(start),1,end-start,f)==end-start); }
static void callback_code(void)
{
    const uint32_t code[]={0x27bdffd0,0xafbf002c,0x0c004440,0,0x8fbf002c,0,
                           0x27bd0030,0x03e00008,0};
    memcpy(PSX_ADDR(0x80080000u),code,sizeof(code));
}
static int run_guest(BattleMipsRuntime *r,uint32_t pc,uint32_t arg)
{ PcPortMipsCpu c; initialize_cpu(&c,r); c.gpr[4]=arg;c.gpr[29]=0x801ff000u;c.gpr[31]=BATTLE_HALT_PC;
  c.bus.bridge=controlled_bridge;
  return PcPortMipsRun(&c,pc,BATTLE_HALT_PC,1000); }
static int run_native(BattleMipsRuntime *r,uint32_t pc,uint32_t arg)
{ PcPortMipsCpu parent; initialize_cpu(&parent,r);parent.gpr[29]=0x801fed00u;
  r->bridge_cpu=&parent;g_ActiveBattleRuntime=r;
  if(pc==0x800ba8f4u)func_800BA8F4(PSX_ADDR(arg));else if(pc==0x800bc158u)func_800BC158(PSX_ADDR(arg));else abort();
  g_ActiveBattleRuntime=NULL;r->bridge_cpu=NULL;return 1; }

static unsigned gravity_case(BattleMipsRuntime *r,int fallback)
{
    uint8_t oracle[0xc0],native[0xc0]; memset(oracle,0xa5,sizeof oracle);
    *(int16_t*)(oracle+2)=11;*(int16_t*)(oracle+6)=22;*(int16_t*)(oracle+10)=33;
    *(uint32_t*)(oracle+0x78)=0x10203040; use_fallback=fallback;
    primary_floor=77;fallback_floor=91;call_count=0;
    guard_fill(0x8017fff0u,16);guard_fill(0x801800c0u,16);memcpy(PSX_ADDR(0x80180000u),oracle,sizeof oracle);
    if(run_guest(r,0x800ba8f4u,0x80180000u)!=PC_PORT_MIPS_HALTED)return 100;
    if(!guard_ok(0x8017fff0u,16)||!guard_ok(0x801800c0u,16))return 102;
    memcpy(oracle,PSX_ADDR(0x80180000u),sizeof oracle);
    unsigned oracle_calls=call_count;Call trace[16];memcpy(trace,calls,sizeof trace);
    memset(native,0xa5,sizeof native);*(int16_t*)(native+2)=11;*(int16_t*)(native+6)=22;
    *(int16_t*)(native+10)=33;*(uint32_t*)(native+0x78)=0x10203040;call_count=0;
    uint32_t guest=0x80181000u;guard_fill(guest-16,16);guard_fill(guest+sizeof native,16);memcpy(PSX_ADDR(guest),native,sizeof native);
    if(run_native(r,0x800ba8f4u,guest)!=1)return 101;
    if(!guard_ok(guest-16,16)||!guard_ok(guest+sizeof native,16))return 102;
    memcpy(native,PSX_ADDR(guest),sizeof native);
    if(memcmp(native,oracle,sizeof native)||call_count!=oracle_calls)return 1;
    for(unsigned i=0;i<oracle_calls;i++)
        if(calls[i].target!=trace[i].target||calls[i].a1!=trace[i].a1||
           (calls[i].target==0x800a5914u&&calls[i].a2!=trace[i].a2)||
           ((calls[i].target==0x800a5914u||calls[i].target==0x800a579cu||calls[i].target==0x800a5870u)&&
            (calls[i].x!=trace[i].x||calls[i].y!=trace[i].y||calls[i].z!=trace[i].z)))return 1;
    return 0;
}

static unsigned child_case(BattleMipsRuntime *r,int type,int occupied,int old_frame,int new_frame,int mirror)
{
    const uint32_t wrapper=0x80182000u,old=0x80183000u;
    uint8_t initial[0x100];memset(initial,0x5a,sizeof initial);
    uint8_t *sprite=initial+0x38;*(uint32_t*)(sprite+0x40)=(uint32_t)type<<13;
    *(uint16_t*)(sprite+0x34)=(uint16_t)new_frame;*(uint32_t*)(sprite+0xac)=mirror?4u:0u;
    memset(PSX_ADDR(0x800c3cc0u),0,32);
    store_le(PSX_ADDR(0x800c3680u),4,0);store_le(PSX_ADDR(0x800c3684u),4,0);
    guard_fill(wrapper-16,16);guard_fill(wrapper+sizeof initial,16);memset(PSX_ADDR(wrapper),0x5a,sizeof initial);memcpy(PSX_ADDR(wrapper),initial,sizeof initial);
    uint32_t slot=type==10?0x800c3680u:0x800c3684u;store_le(PSX_ADDR(slot),4,occupied?old:0);
    if(occupied){memset(PSX_ADDR(old),0,0x80);store_le(PSX_ADDR(old+0x6c),2,(uint16_t)old_frame);
      store_le(PSX_ADDR(old+0xc),4,0x80080000u);}
    store_le(PSX_ADDR(type==10?0x800d30a0u:0x800d30a8u),2,0x1111);
    store_le(PSX_ADDR((type==10?0x800d30a0u:0x800d30a8u)+2),2,0x2222);
    store_le(PSX_ADDR((type==10?0x800d30a0u:0x800d30a8u)+4),2,0x3333);
    call_count=0;callback_marker=0;uint32_t parent_sp=0x801ff000u;
    if(run_guest(r,0x800bc158u,wrapper)!=PC_PORT_MIPS_HALTED)return 100;
    if(!guard_ok(wrapper-16,16)||!guard_ok(wrapper+sizeof initial,16))return 102;
    uint8_t expected[0x100];memcpy(expected,PSX_ADDR(wrapper),sizeof expected);
    uint8_t globals[32];memcpy(globals,PSX_ADDR(0x800c3cc0u),sizeof globals);
    uint32_t expected_slot=(uint32_t)load_le(PSX_ADDR(slot),4);
    unsigned expected_calls=call_count;Call trace[16];memcpy(trace,calls,sizeof trace);
    memset(PSX_ADDR(wrapper),0x5a,sizeof initial);memcpy(PSX_ADDR(wrapper),initial,sizeof initial);
    store_le(PSX_ADDR(0x800c3680u),4,0);store_le(PSX_ADDR(0x800c3684u),4,0);
    store_le(PSX_ADDR(slot),4,occupied?old:0);memset(PSX_ADDR(0x800c3cc0u),0,32);
    call_count=0;callback_marker=0;
    if(run_native(r,0x800bc158u,wrapper)!=1)return 101;
    if(!guard_ok(wrapper-16,16)||!guard_ok(wrapper+sizeof initial,16))return 102;
    if(memcmp(PSX_ADDR(wrapper),expected,sizeof expected)||
       memcmp(PSX_ADDR(0x800c3cc0u),globals,sizeof globals)||
       call_count!=expected_calls)return 11;
    if(expected_slot==wrapper&&load_le(PSX_ADDR(slot),4)!=(uint32_t)(uintptr_t)PSX_ADDR(wrapper))return 12;
    if(expected_slot!=wrapper&&load_le(PSX_ADDR(slot),4)!=expected_slot)return 13;
    for(unsigned i=0;i<expected_calls;i++){
      if(calls[i].target!=trace[i].target)return 14;
      if((trace[i].target==0x8001cd74u||trace[i].target==0x8001cd6cu)&&
         (trace[i].a0!=(uint32_t)(uintptr_t)PSX_ADDR(wrapper)||
          calls[i].a0!=(uint32_t)(uintptr_t)PSX_ADDR(wrapper)))return 15;
      if(trace[i].target==0x80011100u&&
         (trace[i].a0!=(uint32_t)(uintptr_t)PSX_ADDR(old)||calls[i].a0!=(uint32_t)(uintptr_t)PSX_ADDR(old)))return 16;
      if((trace[i].target==0x8001cd74u&&calls[i].a1!=0x800bbee0u)||
         (trace[i].target==0x8001cd6cu&&calls[i].a1!=0x800bc018u)||
         (trace[i].target==0x800bc2f0u&&calls[i].a0!=2))return 17;
    }
    if(occupied&&old_frame!=1&&new_frame==1&&callback_marker!=0xcafebabeu)return 2;
    if((!occupied||old_frame==1||new_frame!=1)&&callback_marker!=0)return 2;
    for(unsigned i=0;i<expected_calls;i++)if(trace[i].target==0x80011100u&&trace[i].sp!=parent_sp-0x50u){fprintf(stderr,"oracle nested SP got=%08x\n",trace[i].sp);return 3;}
    for(unsigned i=0;i<expected_calls;i++)if(calls[i].target==0x80011100u&&calls[i].sp!=0x801fed00u-0x50u){fprintf(stderr,"native nested SP got=%08x\n",calls[i].sp);return 3;}
    return 0;
}

int main(int argc,char **argv)
{
 if(argc==2&&!strcmp(argv[1],"inactive-gravity")){func_800BA8F4(PSX_ADDR(0x80180000u));return 90;}
 if(argc==2&&!strcmp(argv[1],"inactive-child")){func_800BC158(PSX_ADDR(0x80182000u));return 91;}
 FILE*f=fopen("disc/battle.bin","rb");assert(f);load_range(f,0x800ba8f4u,0x800ba984u);
 load_range(f,0x800bc158u,0x800bc2f0u);assert(!fclose(f));callback_code();
 initialize_runtime(&g_BattleRuntime);unsigned fail=0,rc;
#define RUN_CASE(label,expr) do{rc=(expr);if(rc)fprintf(stderr,"BATTLE SPRITE HOOKS case %s failed rc=%u\n",label,rc);fail+=rc;}while(0)
 RUN_CASE("gravity-primary",gravity_case(&g_BattleRuntime,0));
 RUN_CASE("gravity-fallback",gravity_case(&g_BattleRuntime,1));
 for(int slot=0;slot<2;slot++)for(int occupied=0;occupied<2;occupied++)
  for(int old_frame=0;old_frame<2;old_frame++)for(int new_frame=0;new_frame<2;new_frame++)
   for(int mirror=0;mirror<2;mirror++){
    char label[80];snprintf(label,sizeof label,"slot%d-%s-old%d-new%d-mirror%d",slot,occupied?"occupied":"empty",old_frame,new_frame,mirror);
    RUN_CASE(label,child_case(&g_BattleRuntime,10+slot,occupied,old_frame,new_frame,mirror));
   }
 if(fail){fprintf(stderr,"BATTLE SPRITE HOOKS RED failures=%u\n",fail);return 1;}
 puts("BATTLE SPRITE HOOKS PASS gravity XYZ paths and 64-case child ownership/frame/mirror grid");return 0;
}
