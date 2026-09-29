/* Complete BC460 + SDK machine code versus staged native bus implementation.
 * Shared PsyCross GTE backend; no independent PS1 hardware accuracy claim. */
#ifdef XBT_CAMERA_RUNTIME
#include "../src/battle_mips_runtime.c"
#endif
#include "battle_target_camera.h"
#include <libgte.h>
#include <inline_c.h>
#include <psx/gtereg.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern int matrixLevel;
extern MATRIX stack[];
extern MATRIX *currentMatrix;
uint8_t g_PsxRam[0x200000]; /* Actual SquareRoot0 owner reads its retail table here. */
#define ram g_PsxRam
static uint8_t initial[0x200000],expected[0x200000];
#ifdef XBT_CAMERA_RUNTIME
uint8_t g_PsxScratchpad[4096];
_Alignas(4) uint8_t g_GameState[0x2358];
_Alignas(4) uint8_t D_800D30A0[16];
uint32_t D_800C3CDC,D_800C3678;
static _Alignas(4) uint8_t host_targets[11][0x40];
static uint8_t saved_shared[sizeof(g_GameState)],saved_host[sizeof(host_targets)];
static unsigned target_domain,native_phase,peripheral_calls;
int g_GPUDisabledState;
unsigned g_PcPortPresentedFrames;
void PcPort_ServiceVblank(void) { peripheral_calls++; }
void ControllerPushState(void) { abort(); }
void ControllerPoll(void) { abort(); }
void PsyX_UpdateInput(void) { abort(); }
int PsyX_Sys_GetVBlankCount(void) { abort(); }
char PsyX_BeginScene(void) { abort(); }
void ClearSplits(void) { abort(); }
void DrawAllSplits(void) { abort(); }
void ParsePrimitivesLinkedList(u_long *p,int s) { (void)p; (void)s; abort(); }
#endif
static unsigned cases,small_cases,large_cases,empty_cases,second_points;
static unsigned seen_small,seen_large,seen_empty,seen_second;
static unsigned raw_padding_differences;
static unsigned fixture_screen=256;
static unsigned fixture_height,near_threshold;
typedef struct Access { uint32_t address,value; unsigned width,write; } Access;
static Access tape[1024];
static unsigned trace_mode,trace_count,fail_at,failure_cases;
static GTERegisters fault_gte;
static int trace(uint32_t a,unsigned w,uint32_t v,unsigned write) {
    if(!trace_mode) return 0;
    unsigned n=trace_count++;
    assert(n<1024);
    if(trace_mode==1) tape[n]=(Access){a,v,w,write};
    else {
        assert(a==tape[n].address && w==tape[n].width && write==tape[n].write);
        if(write) assert(v==tape[n].value);
        if(n==fail_at) { fault_gte=gteRegs; return -1; }
    }
    return 0;
}
enum { STACK=0x1ff000, FRAME_BYTES=0x120+0x68 }; /* BC460 + nested BB844 */
static uint32_t get(unsigned a,unsigned w) {
    uint32_t v=0; for(unsigned i=0;i<w;i++) v|=(uint32_t)ram[a+i]<<(i*8); return v;
}
static void put(unsigned a,unsigned w,uint32_t v) {
    for(unsigned i=0;i<w;i++) ram[a+i]=(uint8_t)(v>>(i*8));
}
static int rd(void *p,uint32_t a,unsigned w,uint32_t *v) {
    (void)p;
    if(trace(a,w,0,0)) return -1;
#ifdef XBT_CAMERA_RUNTIME
#ifdef XBT_CAMERA_RAW_SHARED
    uint32_t canonical=(a&0xff800000u)==0xa0000000u?a-0x20000000u:a;
    if(native_phase && canonical>=0x8006d634u && canonical<0x8006f98cu) {
        *v=get(canonical-0x80000000u,w); return 0;
    }
#endif
    return runtime_read(&g_BattleRuntime,a,w,v);
#else
    if(a<0x80000000u || a>0x80200000u-w) return -1;
    *v=get(a-0x80000000u,w); return 0;
#endif
}
static int wr(void *p,uint32_t a,unsigned w,uint32_t v) {
    (void)p;
    if(trace(a,w,v,1)) return -1;
#ifdef XBT_CAMERA_RUNTIME
#ifdef XBT_CAMERA_SHADOW_WRITE
    if(native_phase && a==0x800c3678u) { D_800C3678=v; return 0; }
#endif
    return runtime_write(&g_BattleRuntime,a,w,v);
#else
    if(a<0x80000000u || a>0x80200000u-w) return -1;
    put(a-0x80000000u,w,v); return 0;
#endif
}
static uint32_t cr(void *p,int c,unsigned r) { (void)p; return c?CFC2(r):MFC2(r); }
static void cw(void *p,int c,unsigned r,uint32_t v) { (void)p; if(c) CTC2(v,r); else MTC2(v,r); }
static int op(void *p,uint32_t ins) { (void)p; doCOP2(ins); return 0; }
static int observe(void *p,PcPortMipsCpu *cpu,uint32_t pc) {
    (void)p;
    if(pc==0x800bc930) seen_small++;
    if(pc==0x800bc994) seen_large++;
    if(pc==0x800bc550 && cpu->gpr[6]==0) seen_empty++;
    if(pc==0x800bc894) seen_second++;
    if(pc==0x800bc924 && cpu->gpr[21]>=100 && cpu->gpr[21]<120) near_threshold++;
    return 0; /* Observe only; never service any guest instruction/callee. */
}
static void check(int ok,const char *what) {
    if(!ok) { fprintf(stderr,"TARGET CAMERA FAIL case=%u %s\n",cases,what); exit(1); }
}
static void configure(uint32_t mask,unsigned profile,unsigned geometry,unsigned height_gate) {
    memset(ram+0xc3eb0,0,0x1c*11);
    memset(ram+0xe0000,0,0x40*11);
    for(unsigned i=0;i<11;i++) {
        put(0xccb3c+i*4,4,profile==2 && i%3==0?0:0x800e0000u+i*0x40);
        put(0xc3eb0+i*0x1c+7,1,profile==1 && i%3==0?255:0);
        put(0xc3eb0+i*0x1c+8,1,i%2?255:0);
        int x=geometry==0?0:(int)i*900-4500;
        int y=geometry==0?0:(int)i*50-250;
        int z=geometry==0?0:(int)i*200-1000;
        put(0xe0000+i*0x40,4,(uint32_t)x*65536u+3);
        put(0xe0004+i*0x40,4,(uint32_t)y*65536u+0xfffe);
        put(0xe0008+i*0x40,4,(uint32_t)z*65536u+1);
        put(0xe0036+i*0x40,2,geometry==0?fixture_height:800+i*17);
    }
    put(0xc3688,1,height_gate);
    put(0xc3740,2,geometry==2?113:0);
    put(0xc3742,2,geometry==2?271:0);
    put(0xc3744,2,geometry==2?509:0);
    put(0xc3730,2,0); put(0xc3732,2,4096); put(0xc3734,2,0);
    memset(ram+0xd309c,0xa5,24); memset(ram+0xc3cd8,0x96,12);
    put(0xc3678,4,~mask);
    memset(ram+0x56d30,0x96,640); put(0x56d2c,4,0);
    memset(ram+STACK-0x300,(uint8_t)(mask^(mask>>8)),0x300);
    memset(&gteRegs,0,sizeof(gteRegs));
    for(unsigned r=0;r<32;r++) {
        CTC2(0x713da905u+r*0x1521u,r);
        MTC2(0x971cd063u+r*0x1753u,r);
    }
    CTC2(4096,0); CTC2(4096,2); CTC2(4096,4);
    CTC2(160u<<16,24); CTC2(164u<<16,25); CTC2(fixture_screen,26);
    CTC2(0x100,27); CTC2(0x100000,28);
#ifdef XBT_CAMERA_RUNTIME
    memset(g_GameState,0x39,sizeof(g_GameState));
    memset(host_targets,0x71,sizeof(host_targets));
    memset(D_800D30A0,0xcc,sizeof(D_800D30A0));
    D_800C3CDC=D_800C3678=0xccccccccu;
    /* Raw RAM beneath the shared symbol is intentionally different and safe
     * zero-vector data. Populate host records directly, not via the resolver
     * whose behavior this fixture is meant to verify. */
    memset(ram+0x6d634,0,sizeof(g_GameState));
    for(unsigned i=0;i<11;i++) {
        memcpy(host_targets[i],ram+0xe0000+i*0x40,0x40);
        memcpy(g_GameState+0x400+i*0x40,ram+0xe0000+i*0x40,0x40);
        if(!get(0xccb3c+i*4,4)) continue;
        unsigned domain=target_domain==5?i%5:target_domain;
        uint32_t handle=domain==0?0x800e0000u+i*0x40:
                        domain==1?0xa00e0000u+i*0x40:
                        domain==2?(uint32_t)(uintptr_t)host_targets[i]:
                        (domain==3?0x8006d634u:0xa006d634u)+0x400+i*0x40;
        put(0xccb3c+i*4,4,handle);
    }
    memcpy(saved_shared,g_GameState,sizeof(saved_shared));
    memcpy(saved_host,host_targets,sizeof(saved_host));
#endif
}
static void run(uint32_t mask,unsigned profile,unsigned geometry,unsigned height_gate) {
    configure(mask,profile,geometry,height_gate);
    memcpy(initial,ram,sizeof(ram)); GTERegisters start=gteRegs;
    PcPortMipsBus bus={.read=rd,.write=wr,.bridge=observe,.cop2_read=cr,.cop2_write=cw,.cop2_command=op};
    PcPortMipsCpu cpu; PcPortMipsCpuInit(&cpu,&bus);
    cpu.gpr[4]=mask; cpu.gpr[29]=0x80000000u+STACK; cpu.gpr[31]=0x801ffffc;
    for(unsigned i=16;i<=23;i++) cpu.gpr[i]=0xace00000u+i;
    cpu.gpr[30]=0x13abcdef;
    seen_small=seen_large=seen_empty=seen_second=0;
    int rc=PcPortMipsRun(&cpu,0x800bc460,0x801ffffc,50000);
    if(rc) {
        fprintf(stderr,"retail error: %s\n",cpu.error);
#ifdef XBT_CAMERA_RUNTIME
        ResolvedData *binding=find_data(&g_BattleRuntime,cpu.pc,4);
        if(binding) fprintf(stderr,"instruction data binding: %s at %08x size=%x\n",
                            binding->name,binding->address,binding->size);
#endif
    }
    check(rc==0,"retail execution");
    check(seen_small+seen_large+seen_empty==1,"retail branch census");
    small_cases+=seen_small; large_cases+=seen_large; empty_cases+=seen_empty; second_points+=seen_second;
    check(cpu.gpr[29]==0x80000000u+STACK && cpu.gpr[31]==0x801ffffc,"SP/RA");
    for(unsigned i=16;i<=23;i++) check(cpu.gpr[i]==0xace00000u+i,"callee registers");
    check(cpu.gpr[30]==0x13abcdef,"frame pointer");
#ifdef XBT_CAMERA_RUNTIME
    check(!memcmp(saved_shared,g_GameState,sizeof(saved_shared)) &&
          !memcmp(saved_host,host_targets,sizeof(saved_host)),"retail resolved inputs immutable");
#endif
    memcpy(expected,ram,sizeof(ram)); GTERegisters end=gteRegs;
    memcpy(ram,initial,sizeof(ram)); gteRegs=start;
    matrixLevel=0; currentMatrix=stack; memset(stack,0x96,640);
#ifdef XBT_CAMERA_RUNTIME
    PcPortMipsCpu parent;
    initialize_cpu(&parent,&g_BattleRuntime);
    parent.gpr[29]=0x801ff000;
    PcPortMipsCpu saved_parent=parent;
    g_BattleRuntime.bridge_cpu=&parent;
    native_phase=1;
#endif
    check(PcPortBattleUpdateTargetCamera(&bus,mask)==0,"native execution");
#ifdef XBT_CAMERA_RUNTIME
    native_phase=0;
    check(g_BattleRuntime.bridge_cpu==&parent && !memcmp(&parent,&saved_parent,sizeof(parent)),"parent CPU unchanged");
    g_BattleRuntime.bridge_cpu=NULL;
    check(!memcmp(saved_shared,g_GameState,sizeof(saved_shared)) &&
          !memcmp(saved_host,host_targets,sizeof(saved_host)),"native resolved inputs immutable");
    check(D_800C3CDC==0xccccccccu && D_800C3678==0xccccccccu,"overlay placeholders untouched");
    for(unsigned i=0;i<sizeof(D_800D30A0);i++) check(D_800D30A0[i]==0xcc,"overlay vector placeholder untouched");
    check(!peripheral_calls,"no runtime callbacks serviced");
#endif
#if XBT_CAMERA_OBSERVATION == 1
    ram[0xd30a4]^=1;
#elif XBT_CAMERA_OBSERVATION == 2
    ((uint8_t *)stack)[0]^=1;
#elif XBT_CAMERA_OBSERVATION == 3
    gteRegs.CP2D.r[9]^=1;
#elif XBT_CAMERA_OBSERVATION == 4
    gteRegs.CP2C.r[0]^=1;
#elif XBT_CAMERA_OBSERVATION == 5
    gteRegs.CP2D.r[1]^=1; /* Low VZ0 must not be hidden by normalization. */
#elif XBT_CAMERA_OBSERVATION == 6
    ram[STACK-0x400]^=1;
#endif
    /* Only the unmapped guest stack frame and SDK's host-mapped stack storage
     * are excluded here; compare the latter separately, including padding. */
    check(!memcmp(ram,expected,0x56d30),"RAM before SDK stack");
    check(!memcmp(ram+0x56fb0,expected+0x56fb0,STACK-FRAME_BYTES-0x56fb0),"public RAM/state");
    check(!memcmp(ram+STACK,expected+STACK,sizeof(ram)-STACK),"RAM above guest stack");
    check(!memcmp(stack,expected+0x56d30,640),"SDK stack bytes");
    check(matrixLevel==0 && currentMatrix==stack,"SDK stack balance");
    /* VZ0's upper half is uninitialized retail SVECTOR frame padding.
     * INLINE_C.C MFC2(1) sign-extends the low half; PsyX_GTE.cpp VZ(n)
     * also reads only sw.l. Do not plant retail frame garbage in native
     * locals or claim raw backing-store identity for these unused bits. */
    if(gteRegs.CP2D.r[1]!=end.CP2D.r[1]) raw_padding_differences++;
    GTERegisters native_end=gteRegs;
    uint32_t native_vz=MFC2(1);
    gteRegs=end;
    uint32_t retail_vz=MFC2(1);
    check(native_vz==retail_vz,"architectural VZ0");
    end.CP2D.r[1]=gteRegs.CP2D.r[1];
    gteRegs=native_end;
    gteRegs.CP2D.r[1]=native_vz;
    if(memcmp(&gteRegs.CP2D,&end.CP2D,sizeof(end.CP2D))) {
        uint32_t wanted[32],actual[32];
        memcpy(wanted,&end.CP2D,sizeof(wanted)); memcpy(actual,&gteRegs.CP2D,sizeof(actual));
        for(unsigned i=0;i<32;i++) if(wanted[i]!=actual[i])
            fprintf(stderr,"CP2D[%u] retail=%08x native=%08x mask=%x profile=%u geometry=%u gate=%u\n",i,wanted[i],actual[i],mask,profile,geometry,height_gate);
    }
    check(!memcmp(&gteRegs.CP2D,&end.CP2D,sizeof(end.CP2D)),"GTE data");
    check(!memcmp(&gteRegs.CP2C,&end.CP2C,sizeof(end.CP2C)),"GTE control");
    gteRegs=native_end;
    check(get(0xc3678,4)==mask,"full mask publication");
    if(!seen_empty && geometry==0 && fixture_screen==256 && fixture_height==0) {
        check(get(0xc3cdc,4)==512 && get(0xd30a0,2)==0 && get(0xd30a2,2)==0 &&
              get(0xd30a4,2)==(uint16_t)-512 && get(0xd30a8,2)==0 &&
              get(0xd30aa,2)==0 && get(0xd30ac,2)==0,"coincident-point camera witness");
    }
    cases++;
}
static void failures(unsigned geometry,uint32_t mask) {
    configure(mask,0,geometry,0);
    PcPortMipsBus bus={.read=rd,.write=wr};
    memcpy(initial,ram,sizeof(ram)); GTERegisters start=gteRegs;
    matrixLevel=0; currentMatrix=stack; memset(stack,0x96,640);
    trace_mode=1; trace_count=0;
    check(PcPortBattleUpdateTargetCamera(&bus,mask)==0,"failure tape baseline");
    unsigned length=trace_count;
    for(unsigned f=0;f<length;f++) {
        memcpy(ram,initial,sizeof(ram)); memcpy(expected,initial,sizeof(ram));
        gteRegs=start; matrixLevel=0; currentMatrix=stack; memset(stack,0x96,640);
        for(unsigned i=0;i<f;i++) if(tape[i].write) {
            unsigned a=tape[i].address-0x80000000u;
            for(unsigned j=0;j<tape[i].width;j++) expected[a+j]=(uint8_t)(tape[i].value>>(j*8));
        }
        trace_mode=2; trace_count=0; fail_at=f;
        check(PcPortBattleUpdateTargetCamera(&bus,mask)==-1,"bus failure return");
        check(trace_count==f+1,"bus failure stops immediately");
        check(!memcmp(ram,expected,sizeof(ram)),"bus failure public-write prefix");
        check(!memcmp(&gteRegs,&fault_gte,sizeof(gteRegs)),"bus failure retains GTE effects");
        failure_cases++;
    }
    trace_mode=0; trace_count=0;
    check(PcPortBattleUpdateTargetCamera(NULL,mask)==-1,"NULL API");
    bus.read=NULL; check(PcPortBattleUpdateTargetCamera(&bus,mask)==-1,"missing reader");
    bus.read=rd; bus.write=NULL;
    check(PcPortBattleUpdateTargetCamera(&bus,mask)==-1,"missing writer");
}
int main(void) {
#ifdef XBT_CAMERA_RUNTIME
    check((uintptr_t)host_targets+sizeof(host_targets)<=UINT32_MAX,"low native target addresses");
    initialize_runtime(&g_BattleRuntime);
    g_ActiveBattleRuntime=&g_BattleRuntime;
    check(resolve_memory(&g_BattleRuntime,0x8006da34,0x40,0)==g_GameState+0x400,"shared KSEG0 resolution");
    check(resolve_memory(&g_BattleRuntime,0xa006da34,0x40,0)==g_GameState+0x400,"shared KSEG1 resolution");
    check(resolve_memory(&g_BattleRuntime,(uint32_t)(uintptr_t)host_targets,0x40,0)==host_targets[0],"native range resolution");
    check(resolve_memory(&g_BattleRuntime,0x800d30a0,16,1)==ram+0xd30a0 &&
          resolve_memory(&g_BattleRuntime,0xa00c3cdc,4,1)==ram+0xc3cdc &&
          resolve_memory(&g_BattleRuntime,0x800c3678,4,1)==ram+0xc3678,"overlay data stays guest-owned");
#endif
    FILE *f=fopen("disc/SLUS_006.64","rb"); assert(f);
    assert(!fseek(f,0x800,SEEK_SET)); assert(fread(ram+0x10000,1,0x49800,f)==0x49800); fclose(f);
    f=fopen("disc/battle.bin","rb"); assert(f);
    assert(!fseek(f,0x4bd54,SEEK_SET)); assert(fread(ram+0xbb844,1,0x190,f)==0x190);
    assert(!fseek(f,0x4c970,SEEK_SET)); assert(fread(ram+0xbc460,1,0x644,f)==0x644); fclose(f);
#ifdef XBT_CAMERA_RUNTIME
    for(target_domain=0;target_domain<6;target_domain++)
    for(unsigned mask=0;mask<2048;mask+=17) for(unsigned geometry=0;geometry<3;geometry++)
        run(mask|0x80000800u,mask%3,geometry,(mask&1)?255:0);
    target_domain=5;
#else
    for(unsigned mask=0;mask<2048;mask++) for(unsigned geometry=0;geometry<3;geometry++)
        run(mask|0x80000800u,mask%3,geometry,(mask&1)?255:0);
#endif
    const unsigned screens[]={0,1,4095,4096,8191,8192,16383,32767,32768,65535};
    for(unsigned i=0;i<sizeof(screens)/sizeof(screens[0]);i++) for(unsigned g=0;g<3;g++) {
        fixture_screen=screens[i]; run(0xffffffffu,0,g,0);
    }
    fixture_screen=256;
    const unsigned heights[]={196,200,204,220,236,240,244,65535,32768};
    for(unsigned i=0;i<sizeof(heights)/sizeof(heights[0]);i++) {
        fixture_height=heights[i]; run(2,0,0,0);
    }
    fixture_height=0;
    failures(0,0); failures(0,0x7ff); failures(2,0x7ff);
    check(cases==
#ifdef XBT_CAMERA_RUNTIME
          2217
#else
          6183
#endif
          && small_cases && large_cases && empty_cases && second_points && near_threshold,"coverage");
    printf("TARGET CAMERA native/retail PASS %u cases small=%u large=%u empty=%u second_points=%u raw_VZ0_padding_differences=%u; full SDK, shared GTE\n",
           cases,small_cases,large_cases,empty_cases,second_points,raw_padding_differences);
    printf("TARGET CAMERA failure prefixes PASS %u bus failures and 9 invalid API checks\n",failure_cases);
#ifdef XBT_CAMERA_RUNTIME
    puts("TARGET CAMERA runtime resolver PASS 6 domains; shared and host inputs, guest-owned outputs, unchanged parent CPU");
#endif
}
