#include <stdint.h>
/* Weak only in this test TU: the unimplemented baseline fails semantically. */
extern int PcPort_BattleMipsCallGuest(uint32_t, const uint32_t *, unsigned,
                                     uint32_t *) __attribute__((weak));
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#ifndef BATTLE_RUNTIME_SOURCE
#define BATTLE_RUNTIME_SOURCE "../src/battle_mips_runtime.c"
#endif
#include BATTLE_RUNTIME_SOURCE

/* Added to battle_mips_runtime.c after this test was written; the runtime
 * bridge calls it before every guest invocation, so this test TU (the only
 * definition linked by the runner) must provide it. */
void PcPort_GodModeBeforeGuest(PcPortMipsCpu *cpu, uint32_t target)
{
    (void)cpu;
    (void)target;
}

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];
unsigned int MFC2(int reg) { (void)reg; abort(); }
unsigned int CFC2(int reg) { (void)reg; abort(); }
void MTC2(unsigned int v, int reg) { (void)v; (void)reg; abort(); }
void CTC2(unsigned int v, int reg) { (void)v; (void)reg; abort(); }
int doCOP2(int op) { (void)op; abort(); }
int g_GPUDisabledState;
unsigned g_PcPortPresentedFrames;
void ControllerPushState(void) {}
void ControllerPoll(void) {}
void PsyX_UpdateInput(void) {}
int PsyX_Sys_GetVBlankCount(void) { return 0; }
char PsyX_BeginScene(void) { return 1; }
void ClearSplits(void) { abort(); }
void DrawAllSplits(void) { abort(); }
void ParsePrimitivesLinkedList(u_long *p, int s) { (void)p; (void)s; abort(); }

#define TARGET 0x801d0000u
#define CAPTURE 0x801c0000u
#define PARENT_SP 0x801ff000u
#define FRAME 0x50u
#define HOST_CALL 0x80010080u
#define NESTED 0x801d0200u
#define CALLBACK 0x801d0100u
static BattleMipsRuntime rt;
static PcPortMipsCpu parent;
static unsigned checks;
static int nested_ok;
static unsigned native_depth;
static uint32_t callback_sp;

#define CHECK(label, condition) do { checks++; if (!(condition)) { \
    fprintf(stderr,"BATTLE GUEST CALL FAIL %s line=%d\n",label,__LINE__); return 0; \
} } while (0)
static uint32_t word(uint32_t a) { return load_le(PSX_ADDR(a),4); }
static void emit(uint32_t a, uint32_t w) { store_le(PSX_ADDR(a),4,w); }
static uint32_t ins(unsigned op,unsigned rs,unsigned t,unsigned imm)
{ return op<<26 | rs<<21 | t<<16 | (imm&0xffff); }
static void setup(void)
{
    memset(&rt,0,sizeof(rt)); memset(g_PsxRam,0xA5,sizeof(g_PsxRam));
    rt.initialized=1; initialize_cpu(&parent,&rt);
    for(unsigned i=1;i<32;i++) parent.gpr[i]=0x13570000u+i;
    parent.gpr[28]=CAPTURE+0x100; parent.gpr[29]=PARENT_SP;
    parent.hi=0x76543210;parent.lo=0xfedcba98;parent.cp0[12]=0x12345678;
    parent.load_valid=1;parent.load_reg=16;parent.load_value=0xdeadbeef;
    parent.pc=0x801e6ce8;parent.next_pc=parent.pc+4;
    rt.bridge_cpu=&parent;g_ActiveBattleRuntime=&rt;
    native_depth=0;nested_ok=1;callback_sp=0;
}
static void capture_leaf(uint32_t target)
{
    uint32_t p=target;
    emit(p,ins(15,0,8,CAPTURE>>16));p+=4;
    emit(p,ins(43,8,29,0));p+=4;
    for(unsigned i=0;i<4;i++){emit(p,ins(43,8,4+i,4+i*4));p+=4;}
    for(unsigned i=0;i<3;i++){
        emit(p,ins(35,29,9,0x10+i*4));p+=4;emit(p,0);p+=4;
        emit(p,ins(43,8,9,20+i*4));p+=4;
    }
    emit(p,ins(35,28,10,4));p+=4;emit(p,0);p+=4;
    emit(p,ins(43,8,10,32));p+=4;
    emit(p,ins(43,8,16,36));p+=4;
    emit(p,ins(43,8,25,40));p+=4;
    emit(p,(11u<<11)|0x10);p+=4; /* mfhi t3 */
    emit(p,ins(43,8,11,44));p+=4;
    emit(p,(11u<<11)|0x12);p+=4; /* mflo t3 */
    emit(p,ins(43,8,11,48));p+=4;
    emit(p,0x400b6000u);p+=4; /* mfc0 t3,status */
    emit(p,0);p+=4;emit(p,ins(43,8,11,52));p+=4;
    emit(p,0x3c0289abu);p+=4;emit(p,0x3442cdefu);p+=4;
    emit(p,0x03e00008u);emit(p+4,0);
}
static int matrix(void)
{
    const uint32_t args[7]={0x800d2dac,0xa0011234,0xffffffff,0x12345678,
                            0x80000001,0xffff8000,0xabcdef01};
    for(unsigned argc=0;argc<=7;argc++){
        uint8_t before[0x70],expected[0x70];uint32_t out=0;
        setup();capture_leaf(TARGET);emit(parent.gpr[28]+4,0x5555abcd);
        memcpy(before,PSX_ADDR(PARENT_SP-FRAME-0x10),sizeof(before));
        memcpy(expected,before,sizeof(expected));
        for(unsigned i=4;i<argc;i++)store_le(expected+0x20+(i-4)*4,4,args[i]);
        PcPortMipsCpu saved=parent;
        CHECK("0..7 success",PcPort_BattleMipsCallGuest(TARGET,args,argc,&out)==0);
        CHECK("v0 result",out==0x89abcdef);
        CHECK("retail frame",word(CAPTURE)==PARENT_SP-FRAME);
        for(unsigned i=0;i<4;i++)CHECK("register raw args",word(CAPTURE+4+i*4)==(i<argc?args[i]:0));
        for(unsigned i=4;i<7;i++)CHECK("o32 stack raw args",word(CAPTURE+20+(i-4)*4)==(i<argc?args[i]:0xa5a5a5a5));
        CHECK("GP relative load",word(CAPTURE+32)==0x5555abcd);
        CHECK("callee saved inheritance",word(CAPTURE+36)==saved.gpr[16]);
        CHECK("t9 inheritance",word(CAPTURE+40)==saved.gpr[25]);
        CHECK("HI LO CP0 inheritance",word(CAPTURE+44)==saved.hi && word(CAPTURE+48)==saved.lo && word(CAPTURE+52)==saved.cp0[12]);
        CHECK("caller CPU unchanged",memcmp(&saved,&parent,sizeof(saved))==0);
        CHECK("bridge restored",rt.bridge_cpu==&parent);
        CHECK("frame and redzones preserved",memcmp(expected,PSX_ADDR(PARENT_SP-FRAME-0x10),sizeof(expected))==0);
    }
    setup();emit(TARGET,0x03e00008);emit(TARGET+4,0);
    CHECK("null array and optional result argc0",PcPort_BattleMipsCallGuest(TARGET,NULL,0,NULL)==0);
    return 1;
}
static int aliasing(void)
{
    setup();capture_leaf(TARGET);emit(parent.gpr[28]+4,0);
    uint32_t sp=PARENT_SP-FRAME;
    uint32_t *args=(uint32_t*)PSX_ADDR(sp-4);
    uint32_t original[7];uint8_t expected[0x70];
    for(unsigned i=0;i<7;i++) args[i]=original[i]=0xabc00000+i;
    memcpy(expected,PSX_ADDR(sp-0x10),sizeof(expected));
    for(unsigned i=4;i<7;i++)store_le(expected+0x20+(i-4)*4,4,original[i]);
    store_le(expected+0x24,4,0x89abcdef); /* result aliases outgoing arg5 */
    CHECK("aliased argument snapshot",PcPort_BattleMipsCallGuest(TARGET,args,7,(uint32_t*)PSX_ADDR(sp+0x14))==0);
    for(unsigned i=0;i<7;i++)CHECK("alias arguments intact",word(CAPTURE+4+i*4)==original[i]);
    CHECK("aliased result and retained bytes",memcmp(expected,PSX_ADDR(sp-0x10),sizeof(expected))==0);
    return 1;
}
static int rejection(void)
{
    const uint32_t targets[]={0,0x00401000,0x80010080,0x8006faec,0x800c3a6c,
      0x801cffff,0x801d0001,0x80200000,0x80300000,0xa01d0000,0xfffffffC};
    const uint32_t stacks[]={0,0x40,0x80000048,0x8000004f,0x801ff004,
      BATTLE_STACK_TOP+8,0x80200000,0x803ff000,0xa01ff000,0xfffffff8};
    uint32_t out=0xcafebabe,arg=1,args[7]={1,2,3,4,5,6,7};uint8_t bytes[0x80];
    setup();emit(0x80000000,0x03e00008);emit(0x80000004,0);
    memcpy(bytes,PSX_ADDR(PARENT_SP-0x60),sizeof(bytes));
    PcPortMipsCpu saved=parent;
    for(unsigned i=0;i<sizeof(targets)/sizeof(*targets);i++)
      CHECK("invalid target rejected",PcPort_BattleMipsCallGuest(targets[i],args,7,&out)!=0);
    CHECK("argc8 rejected",PcPort_BattleMipsCallGuest(TARGET,&arg,8,&out)!=0);
    CHECK("argc high rejected",PcPort_BattleMipsCallGuest(TARGET,&arg,~0u,&out)!=0);
    CHECK("null args rejected",PcPort_BattleMipsCallGuest(TARGET,NULL,1,&out)!=0);
    rt.bridge_cpu=NULL;CHECK("no bridge CPU rejected",PcPort_BattleMipsCallGuest(TARGET,&arg,1,&out)!=0);rt.bridge_cpu=&parent;
    rt.initialized=0;CHECK("uninitialized rejected",PcPort_BattleMipsCallGuest(TARGET,&arg,1,&out)!=0);rt.initialized=1;
    g_ActiveBattleRuntime=NULL;CHECK("inactive rejected",PcPort_BattleMipsCallGuest(TARGET,&arg,1,&out)!=0);g_ActiveBattleRuntime=&rt;
    CHECK("rejection CPU intact",memcmp(&saved,&parent,sizeof(saved))==0);
    for(unsigned i=0;i<sizeof(stacks)/sizeof(*stacks);i++){
      parent.gpr[29]=stacks[i];
      CHECK("invalid stack rejected",PcPort_BattleMipsCallGuest(TARGET,args,7,&out)!=0);
      CHECK("invalid SP not rewritten",parent.gpr[29]==stacks[i]);
    }
    CHECK("rejections preserve output",out==0xcafebabe);
    CHECK("rejections preserve frame",memcmp(bytes,PSX_ADDR(PARENT_SP-0x60),sizeof(bytes))==0);
    CHECK("rejections restore context",rt.bridge_cpu==&parent);
    setup();parent.gpr[29]=0x80000050;emit(TARGET,0x03e00008);emit(TARGET+4,0);
    CHECK("lowest complete frame accepted",PcPort_BattleMipsCallGuest(TARGET,NULL,0,NULL)==0);
    parent.gpr[29]=BATTLE_STACK_TOP;
    CHECK("highest initialized stack accepted",PcPort_BattleMipsCallGuest(TARGET,NULL,0,NULL)==0);
    return 1;
}
static int guest_failure(void)
{
    setup();emit(TARGET,ins(15,0,8,CAPTURE>>16));emit(TARGET+4,ins(43,8,4,0));emit(TARGET+8,0xffffffff);
    uint32_t args[5]={0x11223344,2,3,4,5},out=0xdeadbeef;
    PcPortMipsCpu saved=parent;
    CHECK("guest fault propagates",PcPort_BattleMipsCallGuest(TARGET,args,5,&out)!=0);
    CHECK("fault output unchanged",out==0xdeadbeef);
    CHECK("fault keeps performed guest writes",word(CAPTURE)==args[0]);
    CHECK("fault keeps outgoing stack writes",word(PARENT_SP-FRAME+0x10)==args[4]);
    CHECK("fault caller CPU unchanged",memcmp(&saved,&parent,sizeof(saved))==0);
    CHECK("fault context restored",rt.bridge_cpu==&parent);
    return 1;
}
static uintptr_t host_reenter(uintptr_t a0,uintptr_t a1,uintptr_t a2,uintptr_t a3,
 uintptr_t a4,uintptr_t a5,uintptr_t a6,uintptr_t a7,uintptr_t a8,uintptr_t a9,uintptr_t a10,uintptr_t a11)
{
    (void)a0;(void)a1;(void)a2;(void)a3;(void)a4;(void)a5;(void)a6;(void)a7;(void)a8;(void)a9;(void)a10;(void)a11;
    PcPortMipsCpu *current=rt.bridge_cpu;
    if(!current || current==&parent) {nested_ok=0;return 0;}
    uint32_t sp=current->gpr[29];
    if(native_depth++==0){
        if(sp!=PARENT_SP-FRAME-0x20)nested_ok=0;
        if(PcPort_BattleMipsDispatchCallback(CALLBACK,(void*)(uintptr_t)0x12345678)!=1)nested_ok=0;
        if(rt.bridge_cpu!=current)nested_ok=0;
        uint32_t args[7]={7,6,5,4,3,2,1},out=0;
        if(PcPort_BattleMipsCallGuest(NESTED,args,7,&out)!=0 || out!=0x89abcdef)nested_ok=0;
        if(word(CAPTURE)!=sp-FRAME || rt.bridge_cpu!=current)nested_ok=0;
        emit(NESTED,0xffffffff);out=0x55667788;
        if(PcPort_BattleMipsCallGuest(NESTED,args,7,&out)==0 || out!=0x55667788 || rt.bridge_cpu!=current)nested_ok=0;
    } else {
        callback_sp=sp;
        if(sp!=PARENT_SP-FRAME-0x40 || current->gpr[28]!=0 || current->gpr[16]!=0 || current->gpr[4]!=0x12345678)nested_ok=0;
    }
    native_depth--;
    return 0x76543210;
}
static unsigned nonguest_dispatches;
static void host_nonguest(void *argument){(void)argument;nonguest_dispatches++;}
static void calling_guest(uint32_t addr)
{
    const uint32_t code[]={0x27bdffe0,0xafbf001c,0x0c004020,0,
                           0x8fbf001c,0x27bd0020,0x03e00008,0};
    for(unsigned i=0;i<sizeof(code)/sizeof(*code);i++)emit(addr+i*4,code[i]);
}
static int nesting(void)
{
    setup();calling_guest(TARGET);calling_guest(CALLBACK);capture_leaf(NESTED);
    rt.functions[0]=(ResolvedFunction){HOST_CALL,(void*)host_reenter,"guest_call_test_host"};rt.function_count=1;
    PcPortMipsCpu saved=parent;uint32_t out=0;
    CHECK("nested call success",PcPort_BattleMipsCallGuest(TARGET,NULL,0,&out)==0);
    CHECK("nested result",out==0x76543210);
    CHECK("nested callbacks and failure restore",nested_ok && callback_sp==PARENT_SP-FRAME-0x40);
    CHECK("nested caller CPU unchanged",memcmp(&saved,&parent,sizeof(saved))==0);
    CHECK("nested final context",rt.bridge_cpu==&parent);
    g_ActiveBattleRuntime=NULL;
    CHECK("legacy callback inactive unhandled",PcPort_BattleMipsDispatchCallback(CALLBACK,NULL)==0);
    g_ActiveBattleRuntime=&rt;
    /* The dispatcher gained main-executable callback support (commit
     * b7d74b67): a registered non-guest callback is now handled, and only a
     * callback with no bridge entry and no host symbol is unhandled. */
    nonguest_dispatches=0;
    rt.functions[0]=(ResolvedFunction){HOST_CALL,(void*)host_nonguest,"guest_call_test_host"};
    CHECK("legacy callback nonguest dispatched",PcPort_BattleMipsDispatchCallback(HOST_CALL,NULL)==1);
    CHECK("legacy callback nonguest host called once",nonguest_dispatches==1);
    rt.function_count=0;
    CHECK("legacy callback nonguest unhandled",PcPort_BattleMipsDispatchCallback(HOST_CALL,NULL)==0);
    return 1;
}
int main(void)
{
    if(PcPort_BattleMipsCallGuest==NULL){fputs("BATTLE GUEST CALL FAIL service absent\n",stderr);return 1;}
    if(!matrix() || !aliasing() || !rejection() || !guest_failure() || !nesting())return 1;
    printf("BATTLE GUEST CALL PASS checks=%u args0..7/frame50/GP/alias/failure/nesting\n",checks);
    return 0;
}
