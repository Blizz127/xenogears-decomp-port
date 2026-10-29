#define _GNU_SOURCE
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "common.h"
#include "psyq/libgpu.h"
#include "battle_mips_adapter.h"
#include "psx_memory.h"
#include "guest_prim_link.h"
#include "main/game.h"
#include "system/memory.h"
#include "battle_mips_runtime.c"
static BattleMipsRuntime binding_runtime;
uint8_t g_PsxScratchpad[4096];


/* Active window API differential. Real production system.c, memory.c,
 * temp2.c, domain links and PsyCross LoadImage execute in the native leg.
 * Only GR_CopyVRAM is a modeled renderer boundary. Oracle uses original
 * window/text/glyph/heap/link code, with LoadImage modeled at the SDK boundary.
 * Actual typed runtime dispatch with guest outer tokens and native compact embedded pointers. Valid disjoint objects; malformed-span cases are separate.
 */
#define BASE 0x80000000u
#define RAM_SIZE 0x200000u
#define ARENA 0x80100000u
#define SIZE 0x40000u
#define WINDOW (ARENA+0x100u)
#define ROWS (ARENA+0x1000u)
#define ROWS2 (ARENA+0x4000u)
#define NODE (ARENA+0x8000u)
#define DATA (ARENA+0x8100u)
#define DATA2 (ARENA+0x8200u)
#define OT (ARENA+0x9000u)
#define SCRIPT (ARENA+0xa000u)
#define ENTRIES (ARENA+0xb000u)
#define TABLE (ARENA+0xb100u)
#define FONT (ARENA+0x18000u)
#define STATE (ARENA+0x2b000u)
#define UI (ARENA+0x2c000u)
extern u32 D_8005934C,D_80059350,D_80059354,D_80059358,D_8005935C,D_80059364;
extern void *g_SystemDataEntries;
extern u32 g_HeapNeedsConsolidation;
u16 D_800501D0[11];
GameState g_GameState;
u_char g_ControllerButtonMappings[8];
u8 D_8005A0E4[0x400];
int g_xeno_swap_index;
static u16 host_vram[1024*512], guest_vram[1024*512];
extern u_int WindowBody_HeapFree(void *);
extern int WindowBody_LoadImage(RECT *,u_long *);
void MainLoop(int code) { fprintf(stderr,"unexpected MainLoop %d\n",code);abort(); }

#define STACK 0x801ff000u
#define HALT 0xfffffffcu

extern void func_80034888(void *,void *,s32);
s16 g_SystemPalette1, g_SystemPalette2;
_Alignas(16) uint8_t g_PsxRam[PSX_RAM_SIZE];
extern void WindowBody_LinkPrim(void *,void *);
extern void WindowBody_AddPrim(void *,void *);
unsigned int MFC2(int reg) { (void)reg; abort(); }
unsigned int CFC2(int reg) { (void)reg; abort(); }
void MTC2(unsigned int value, int reg) { (void)value; (void)reg; abort(); }
void CTC2(unsigned int value, int reg) { (void)value; (void)reg; abort(); }
int doCOP2(int op) { (void)op; abort(); }

/* These symbols are reachable from runtime_bridge_call through the generic
 * dispatcher's other ABI branches. The constructor test never exercises them. */
int g_GPUDisabledState;
unsigned g_PcPortPresentedFrames; /* printf-only OT diag counter (runtime) */
/* compat.o's real pad pump (kept via the oracle bridge) early-returns on a
 * frozen vblank count before touching pad state. */
int PsyX_Sys_GetVBlankCount(void) { return 0; }
void PsyX_UpdateInput(void) {}
void ControllerPoll(void) {}
void ControllerPushState(void) {}
char PsyX_BeginScene(void) { return 1; }
void ClearSplits(void) {}
void DrawAllSplits(void) {}
void ParsePrimitivesLinkedList(u_long *packet, int single)
{
    (void)packet;
    (void)single;
}


static int actual_links;
static u8 ram[RAM_SIZE], initial[SIZE], *native_arena;
static unsigned char coverage[1572/4],text_coverage[(0x345e0-0x33df0)/4],glyph_coverage[1696/4];
static unsigned char caller_coverage[39];
static unsigned uploads,glyph_calls,text_calls,free_calls;
enum { FREE=1,TEXT,UPLOAD,LINK,ADD };
typedef struct { u32 kind,a,b,words[6]; } Event;
typedef struct { Event events[128]; unsigned count,links; int native,hook; } Context;
static Context host,guest;

static u32 canonical(u32 address)
{
    uintptr_t base=(uintptr_t)g_PsxRam;
    if(address>=base && address<base+0x200000u)
        return BASE+address-(u32)base;
    return address;
}
static u32 native_word(u32 address)
{
    assert(address>=BASE && address<BASE+0x200000u);
    return (u32)(uintptr_t)(g_PsxRam+(address-BASE));
}
static u8 *ptr(Context *c,u32 a,unsigned size)
{
    if(c->native)a=canonical(a);
    if(a<0x200000u) a|=BASE;
    assert(a>=BASE && (uint64_t)a+size<=BASE+RAM_SIZE);
    if(c->native) {
        return g_PsxRam+(a-BASE);
    }
    return ram+(a-BASE);
}
static u32 get(Context *c,u32 a,unsigned n)
{
    u8 *p=ptr(c,a,n);u32 v=0;
    for(unsigned i=0;i<n;i++) v|=(u32)p[i]<<(8*i);
    return v;
}
static void put(Context *c,u32 a,u32 v,unsigned n)
{
    u8 *p=ptr(c,a,n);for(unsigned i=0;i<n;i++) p[i]=(u8)(v>>(8*i));
}
static void event(Context *c,u32 kind,u32 a,u32 b)
{
    if(c->native){a=canonical(a);b=canonical(b);}
    assert(c->count<128);
    Event *e=&c->events[c->count++];*e=(Event){.kind=kind,.a=a,.b=b};
    if(kind==UPLOAD) {
        e->words[0]=get(c,a,4);e->words[1]=get(c,a+4,4);
        for(unsigned i=0;i<4;i++)e->words[2+i]=get(c,b+4*i,4);
    } else if(kind==LINK || kind==ADD) {
        e->words[0]=get(c,a,4);e->words[1]=get(c,b,4);
        for(unsigned i=0;i<4;i++) e->words[2+i]=get(c,b+4+4*i,4);
    } else {
        e->words[0]=get(c,WINDOW+0x10,2);
        e->words[1]=get(c,WINDOW+0x82,2);
        e->words[2]=get(c,WINDOW+0x8c,4);
        e->words[3]=get(c,WINDOW+0x1c,4);
        if(c->native){e->words[2]=canonical(e->words[2]);e->words[3]=canonical(e->words[3]);}
    }
}
static void link_hook(Context *c)
{
    c->links++;
    if(c->links==1 && c->hook==1) {
        put(c,WINDOW+0x28,ROWS2,4);
        put(c,WINDOW+0xc,4,2);
    }
}
static void add_hook(Context *c,u32 p)
{
    if(p==WINDOW+0x3c && c->hook==2) {
        put(c,WINDOW+0x28,ROWS2,4);
        put(c,WINDOW+0x16,2,2);
        put(c,WINDOW+0xc,4,2);
    }
}
static void upload(Context *c,u32 data,int x,int y,int w,int h)
{
    assert(w>=0 && h>=0 && x>=0 && y>=0 && x+w<=1024 && y+h<=512);
    u16 *v=c->native?host_vram:guest_vram;
    u8 *p=ptr(c,data,(unsigned)(w*h*2));
    for(int row=0;row<h;row++)for(int col=0;col<w;col++)
        v[(y+row)*1024+x+col]=(u16)(p[2*(row*w+col)]|p[2*(row*w+col)+1]<<8);
}
/* Compiled C++ SDK declares its renderer transfer boundary with C linkage. */
void GR_CopyVRAM(unsigned short *p,int sx,int sy,int w,int h,int x,int y)
{
    assert(sx==0 && sy==0);
    upload(&host,(u32)(uintptr_t)p,x,y,w,h);uploads++;
}
u_int HeapFree(void *p)
{
    event(&host,FREE,(u32)(uintptr_t)p,0);return WindowBody_HeapFree(p);
}
int LoadImage(RECT *r,u_long *p)
{
    event(&host,UPLOAD,(u32)(uintptr_t)r,(u32)(uintptr_t)p);
    return WindowBody_LoadImage(r,p);
}
void func_80031798(void *ot,void *p)
{
    u32 a=canonical((u32)(uintptr_t)ot),b=canonical((u32)(uintptr_t)p);
    event(&host,LINK,a,b);link_hook(&host);
    actual_links++;WindowBody_LinkPrim(ot,p);
}
void PcPort_AddPrimDomainAware(void *ot,void *p)
{
    u32 a=canonical((u32)(uintptr_t)ot),b=canonical((u32)(uintptr_t)p);
    /* Background linkage is inlined in retail. It is compared via memory. */
    if(b==WINDOW+0x30 || b==WINDOW+0x3c) event(&host,ADD,a,b);
    add_hook(&host,b);
    actual_links++;WindowBody_AddPrim(ot,p);
}
static int read_bus(void *o,u32 a,unsigned w,u32 *v)
{
    *v=get(o,a,w);return 0;
}
static int write_bus(void *o,u32 a,unsigned w,u32 v)
{
    put(o,a,v,w);return 0;
}
static int bridge(void *o,PcPortMipsCpu *cpu,u32 pc)
{
    Context *c=o;u32 a=cpu->gpr[4],b=cpu->gpr[5];
    if(pc>=0x80034888u && pc<0x80034eacu)coverage[(pc-0x80034888u)/4]=1;
    if(pc>=0x80033df0u && pc<0x800345e0u)text_coverage[(pc-0x80033df0u)/4]=1;
    if(pc>=0x80034ffcu && pc<0x8003569cu)glyph_coverage[(pc-0x80034ffcu)/4]=1;
    if(pc>=0x80074f70u && pc<0x8007500cu)caller_coverage[(pc-0x80074f70u)/4]=1;
    switch(pc) {
    case 0x800320e8u: event(c,FREE,a,0);free_calls++;return 0;
    case 0x80033df0u: text_calls++;return 0;
    case 0x80034ffcu: glyph_calls++;return 0;
    case 0x80044894u:
        event(c,UPLOAD,a,b);
        upload(c,b,(s16)get(c,a,2),(s16)get(c,a+2,2),(s16)get(c,a+4,2),(s16)get(c,a+6,2));
        cpu->gpr[2]=0;return 1;
    case 0x80031798u: event(c,LINK,a,b);link_hook(c);return 0;
    case 0x80043b48u: event(c,ADD,a,b);add_hook(c,b);return 0;
    default: return 0;
    }
}
typedef struct {
    const char *name;
    int count,cursor,width,flags,ctx,t84,t86,t88,queued,hook,x,y,step,selected;
    int glyph_x,row,page,speed,pinned,number,frames,restore,suppress,caller,ui_slot,alias;u8 script[32];
} Case;
static uint64_t oracle_steps;
static unsigned total,failures;
static int native_caller_bridge(void *opaque,PcPortMipsCpu *cpu,u32 pc)
{
    if(pc==0x80043b48u && cpu->gpr[31]==0x80074fc0u)
        event(&host,ADD,cpu->gpr[4],cpu->gpr[5]);
    return runtime_bridge(opaque,cpu,pc);
}

static void run_case(Case c,unsigned salt)
{
    guest=(Context){.hook=c.hook};host=(Context){.native=1,.hook=c.hook};
    for(unsigned i=0;i<SIZE;i++)initial[i]=(u8)(salt+i*37u+(i>>8)*13u);
    memcpy(ram+(ARENA-BASE),initial,SIZE);
    memset(ptr(&guest,WINDOW,0x100),0,0x100);
    put(&guest,WINDOW+0xc,c.count,2);put(&guest,WINDOW+0x16,c.cursor,2);
    put(&guest,WINDOW+0x10,c.flags,2);
    put(&guest,WINDOW+0x28,ROWS,4);put(&guest,WINDOW+0x2c,DATA,4);
    put(&guest,WINDOW+0x8c,NODE,4);put(&guest,NODE,0,4);
    put(&guest,NODE+4,SCRIPT,4);put(&guest,NODE-4,0x61ffffffu|((u32)c.pinned<<25),4);
    put(&guest,WINDOW+0x82,c.queued,2);
    put(&guest,WINDOW+0x84,c.t84,2);put(&guest,WINDOW+0x86,c.t86,2);
    put(&guest,WINDOW+0x88,c.t88,2);
    put(&guest,WINDOW,c.glyph_x,2);put(&guest,WINDOW+2,c.row,2);
    put(&guest,WINDOW+4,c.x,2);put(&guest,WINDOW+6,c.y,2);
    put(&guest,WINDOW+0xa,c.width,2);
    put(&guest,WINDOW+0xe,32,2);put(&guest,WINDOW+0x12,(c.width|1)+3,2);
    put(&guest,WINDOW+0x14,c.step,2);put(&guest,WINDOW+0x18,c.page,2);
    put(&guest,WINDOW+0x1c,SCRIPT,4);put(&guest,WINDOW+0x20,SCRIPT+16,4);
    put(&guest,WINDOW+0x68,c.speed,1);put(&guest,WINDOW+0x69,c.speed,1);
    put(&guest,WINDOW+0x6a,c.restore,1);put(&guest,WINDOW+0x6c,c.suppress,1);
    put(&guest,WINDOW+0x6e,c.selected,1);
    for(unsigned i=0;i<4;i++)put(&guest,WINDOW+0x70+i*4,(u32)c.number,4);
    memcpy(ptr(&guest,SCRIPT,sizeof(c.script)),c.script,sizeof(c.script));
    for(int row=-1;row<34;row++) {
        u32 a=ROWS+row*0x60;
        put(&guest,a+0x50,128,2);put(&guest,a+0x52,32+row*13,2);
        put(&guest,a+0x54,(c.width|1)+3,2);put(&guest,a+0x56,13,2);
        put(&guest,a+0x58,c.width,2);put(&guest,a+0x5a,row&1,1);
        put(&guest,a+0x5c,32+row*13,1);put(&guest,a+0x5e,0xc53a,2);
    }
    g_SystemPalette1=(s16)0xc53a;g_SystemPalette2=(s16)0xda17;
    put(&guest,0x800595d4,(u16)g_SystemPalette1,2);
    put(&guest,0x80059414,(u16)g_SystemPalette2,2);
    D_8005934C=0x80;D_80059350=0x1000;D_80059354=0x20;
    D_80059358=0x40;D_8005935C=FONT;D_80059364=0x20;
    const u32 values[]={D_8005934C,D_80059350,D_80059354,D_80059358,D_8005935C,ENTRIES,D_80059364};
    for(unsigned i=0;i<7;i++)put(&guest,0x8005934c+4*i,values[i],4);
    g_SystemDataEntries=(void*)(uintptr_t)ENTRIES;
    for(int i=0;i<0x100;i+=4)put(&guest,ENTRIES+i,TABLE,4);
    for(int i=0;i<16;i++)put(&guest,TABLE+4+i*2,0x40,2);
    put(&guest,TABLE+0x40,0x002221,4);
    put(&guest,ENTRIES+0x6c,TABLE+0x100,4);
    for(unsigned i=0;i<64;i++)put(&guest,TABLE+0x100+i*2,(0x20u+i)<<8,2);
    memset(&g_GameState,0,sizeof(g_GameState));
    ((u8*)&g_GameState)[0x20]=0x21;((u8*)&g_GameState)[0x21]=0x22;
    ((u8*)&g_GameState)[0x1d34]=1;
    memcpy(ptr(&guest,0x8006d634,sizeof(g_GameState)),&g_GameState,sizeof(g_GameState));
    for(unsigned i=0;i<8;i++)g_ControllerButtonMappings[i]=(u8)i;
    memcpy(ptr(&guest,0x80050238,8),g_ControllerButtonMappings,8);
    memset(D_8005A0E4,0x6a,sizeof(D_8005A0E4));
    memcpy(ptr(&guest,0x8005a0e4,sizeof(D_8005A0E4)),D_8005A0E4,sizeof(D_8005A0E4));
    for(unsigned i=0;i<11;i++)D_800501D0[i]=(u16)(0x9101u+i*31u);
    memcpy(ptr(&guest,0x800501d0,sizeof(D_800501D0)),D_800501D0,sizeof(D_800501D0));
    put(&guest,0x8005932c,0,4);g_HeapNeedsConsolidation=0;
    if(c.caller) {
        put(&guest,0x800d2d28,STATE,4);put(&guest,0x800d3278,UI,4);
        put(&guest,0x800d2dac,WINDOW,4);put(&guest,0x800ccb04,c.alias?0x800d2d24u:OT-4,4);
        put(&guest,0x800ccb34,c.ctx,4);
        put(&guest,STATE+0xc8,(c.caller-1)&1,1);
        put(&guest,STATE+0xc9,((c.caller-1)>>1)&1,1);
        put(&guest,UI+0x7f4,c.ui_slot,1);
        if(c.alias)put(&guest,UI+0x7a4+40*c.ui_slot+0xc9,1,1);
        for(unsigned i=0;i<5;i++) {
            const u32 globals[]={0x800d2d28u,0x800d3278u,0x800d2dacu,0x800ccb04u,0x800ccb34u};
            put(&host,globals[i],get(&guest,globals[i],4),4);
        }
    }
    memset(host_vram,0x5a,sizeof(host_vram));memset(guest_vram,0x5a,sizeof(guest_vram));
    memcpy(native_arena,ram+(ARENA-BASE),SIZE);
    const unsigned pointer_offsets[]={0x1c,0x20,0x28,0x2c,0x8c};
    for(unsigned i=0;i<5;i++){
        u32 value=get(&host,WINDOW+pointer_offsets[i],4);
        if(value)put(&host,WINDOW+pointer_offsets[i],native_word(value),4);
    }
    put(&host,NODE+4,native_word(SCRIPT),4);
    for(unsigned off=0;off<0x100;off+=4)
        put(&host,ENTRIES+off,native_word(get(&host,ENTRIES+off,4)),4);
    D_8005935C=native_word(FONT);
    g_SystemDataEntries=(void*)(uintptr_t)native_word(ENTRIES);

    PcPortMipsCpu cpu;PcPortMipsBus bus={.opaque=&guest,.read=read_bus,.write=write_bus,.bridge=bridge};
    PcPortMipsCpuInit(&cpu,&bus);
    cpu.gpr[4]=WINDOW;cpu.gpr[5]=OT;cpu.gpr[6]=c.ctx;
    cpu.gpr[29]=STACK;cpu.gpr[31]=HALT;cpu.gpr[28]=0x80059170u;
    for(unsigned i=16;i<=23;i++)cpu.gpr[i]=0x43210000u+i;
    setenv("XENO_DISABLE_CPU_INTERPRETER","0",1);
    int rc=0;
    for(int frame=0;frame<(c.frames?c.frames:1);frame++) {
        cpu.gpr[4]=WINDOW;cpu.gpr[5]=OT;cpu.gpr[6]=c.ctx;cpu.gpr[31]=HALT;
        uint64_t steps=cpu.steps;
        rc=PcPortMipsRun(&cpu,c.caller?0x80074f70u:0x80034888u,HALT,1000000);oracle_steps+=cpu.steps-steps;
        if(rc!=PC_PORT_MIPS_HALTED)break;
    }
    if(rc!=PC_PORT_MIPS_HALTED) {
        fprintf(stderr,"WINDOW ORACLE ERROR case=%s rc=%d pc=%08x %s\n",c.name,rc,cpu.pc,cpu.error);exit(2);
    }
    assert(cpu.gpr[29]==STACK);
    for(unsigned i=16;i<=23;i++)assert(cpu.gpr[i]==0x43210000u+i);
    setenv("XENO_DISABLE_CPU_INTERPRETER","1",1);
    assert(PcPortMipsInterpreterDisabled());
    PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
    actual_links=0;PcPort_PrimLinkReset();
    PcPortMipsCpu native_cpu={0};
    if(c.caller)initialize_cpu(&native_cpu,&binding_runtime);
    native_cpu.gpr[4]=WINDOW;native_cpu.gpr[5]=OT;native_cpu.gpr[6]=c.ctx;
    native_cpu.gpr[31]=0x80074ffcu;native_cpu.gpr[29]=STACK;
    for(unsigned i=16;i<=23;i++)native_cpu.gpr[i]=0x43210000u+i;
    if(c.caller) {
        native_cpu.gpr[31]=HALT;
        native_cpu.bus.bridge=native_caller_bridge;
    }
    binding_runtime.bridge_cpu=&native_cpu;
    for(int frame=0;frame<(c.frames?c.frames:1);frame++) {
        int handled;
        if(c.caller) {
            native_cpu.gpr[31]=HALT;
            handled=PcPortMipsRun(&native_cpu,0x80074f70u,HALT,1000000)==PC_PORT_MIPS_HALTED;
        } else handled=runtime_bridge_call(&binding_runtime,&native_cpu,0x80034888u);
        if(handled!=1){fprintf(stderr,"BINDING FAIL case=%s status=%d %s\n",c.name,handled,native_cpu.error);abort();}
        assert(PcPort_SystemWindowGuardSwap(NULL)==NULL);
    }
    binding_runtime.bridge_cpu=NULL;
    assert(PcPort_PrimLinkGuestCount()==actual_links);
    assert(PcPort_PrimLinkNativeCount()==0 && PcPort_PrimLinkRejectCount()==0);
    PcPortMipsExecutionStats after=PcPortMipsExecutionStatsGet();
    if(!c.caller)assert(!memcmp(&before,&after,sizeof(before)));
    else {
        assert(after.interpreted_instructions==before.interpreted_instructions);
        assert(after.disabled_refusals==before.disabled_refusals);
        assert(after.unresolved_bridges==before.unresolved_bridges);
        assert(native_cpu.gpr[29]==STACK && native_cpu.gpr[31]==HALT);
        for(unsigned i=16;i<=23;i++)assert(native_cpu.gpr[i]==0x43210000u+i);
    }
    /* Canonicalize only documented native global-pointer fields for the
     * comparison. Both the text output bytes and all other arena bytes are
     * compared independently; no production address is changed. */
    for(unsigned off=0x1c;off<=0x20;off+=4) {
        u32 value=get(&host,WINDOW+off,4);
        if(value>=(uintptr_t)D_8005A0E4 && value<(uintptr_t)D_8005A0E4+sizeof(D_8005A0E4))
            put(&host,WINDOW+off,0x8005a0e4u+value-(u32)(uintptr_t)D_8005A0E4,4);
        if(value>=(uintptr_t)&g_GameState && value<(uintptr_t)&g_GameState+sizeof(g_GameState))
            put(&host,WINDOW+off,0x8006d634u+value-(u32)(uintptr_t)&g_GameState,4);
    }
    for(unsigned i=0;i<5;i++){
        u32 value=get(&host,WINDOW+pointer_offsets[i],4);
        put(&host,WINDOW+pointer_offsets[i],canonical(value),4);
    }
    put(&host,NODE+4,canonical(get(&host,NODE+4,4)),4);
    for(unsigned off=0;off<0x100;off+=4)
        put(&host,ENTRIES+off,canonical(get(&host,ENTRIES+off,4)),4);
    unsigned changed=0,first=0;
    for(unsigned i=0;i<SIZE;i++)if(native_arena[i]!=ram[ARENA-BASE+i]) {
        if(!changed)first=i;changed++;
    }
    int bad=changed || memcmp(D_8005A0E4,ptr(&guest,0x8005a0e4,sizeof(D_8005A0E4)),sizeof(D_8005A0E4)) || memcmp(host_vram,guest_vram,sizeof(host_vram)) || g_HeapNeedsConsolidation!=get(&guest,0x8005932c,4) || host.count!=guest.count || memcmp(host.events,guest.events,sizeof(host.events));
    if(c.caller) {
        const u32 globals[]={0x800d2d28u,0x800d3278u,0x800d2dacu,0x800ccb04u,0x800ccb34u,STACK-8u};
        for(unsigned i=0;i<6;i++)if(get(&host,globals[i],4)!=get(&guest,globals[i],4))bad=1;
    }
    total++;
    if(bad) {
        failures++;
        if(failures<=8) {
            printf("WINDOW DIFF case=%s count=%d cursor=%d width=%d flags=%x ctx=%d hook=%d salt=%u bytes=%u first=%08x native=%02x retail=%02x calls=%u/%u\n",
                c.name,c.count,c.cursor,c.width,c.flags,c.ctx,c.hook,salt,changed,ARENA+first,
                native_arena[first],ram[ARENA-BASE+first],host.count,guest.count);
            for(unsigned i=0;i<host.count && i<guest.count;i++)if(memcmp(&host.events[i],&guest.events[i],sizeof(Event))) {
                printf(" EVENT %u kind=%u/%u a=%08x/%08x b=%08x/%08x\n",i,
                    host.events[i].kind,guest.events[i].kind,host.events[i].a,guest.events[i].a,host.events[i].b,guest.events[i].b);break;
            }
        }
    }

}
static void check_binding_rejections(void)
{
    const char *names[]={
        "outer-tail","outer-alignment","scalar-high-bit","guest-row-token",
        "row-tail","row-alignment","null-queue-node","queue-header-before-RAM",
        "guest-script-token","two-byte-script-tail","glyph-tail","glyph-alignment",
        "upload-tail","negative-upload-width","button-index","zero-page-divisor"
    };
    static u8 before_arena[SIZE];
    for(unsigned which=0;which<sizeof(names)/sizeof(names[0]);which++) {
        host=(Context){.native=1};actual_links=0;PcPort_PrimLinkReset();
        memset(native_arena,0,SIZE);
        put(&host,WINDOW+0x10,4,2);put(&host,WINDOW+0xc,1,2);
        put(&host,WINDOW+0xa,24,2);put(&host,WINDOW+0x12,28,2);
        put(&host,WINDOW+0x14,14,2);put(&host,WINDOW+0x68,1,1);
        put(&host,WINDOW+0x1c,native_word(SCRIPT),4);
        put(&host,WINDOW+0x28,native_word(ROWS),4);
        put(&host,WINDOW+0x2c,native_word(DATA),4);
        put(&host,ROWS+0x58,1,2);
        put(&host,ROWS+0x50,128,2);put(&host,ROWS+0x52,32,2);
        put(&host,ROWS+0x54,28,2);put(&host,ROWS+0x56,13,2);
        put(&host,OT,0x00ffffffu,4);
        D_8005934C=0x80;D_80059364=0x20;D_8005935C=native_word(FONT);
        PcPortMipsCpu cpu={0};
        cpu.gpr[4]=WINDOW;cpu.gpr[5]=OT;cpu.gpr[6]=0;
        cpu.gpr[31]=0x80074ffcu;cpu.gpr[29]=STACK;
        switch(which) {
        case 0: cpu.gpr[4]=0x801fff80u;break;
        case 1: cpu.gpr[4]=WINDOW+1;break;
        case 2: cpu.gpr[6]=0x80000001u;break;
        case 3: put(&host,WINDOW+0x28,ROWS,4);break;
        case 4: put(&host,WINDOW+0x28,native_word(0x801fffc0u),4);break;
        case 5: put(&host,WINDOW+0x28,native_word(ROWS+2),4);break;
        case 6: case 7:
            put(&host,WINDOW+0x10,0,2);put(&host,WINDOW+0x82,1,2);
            put(&host,WINDOW+0x8c,which==6?0:native_word(BASE+4),4);break;
        case 8: put(&host,WINDOW+0x1c,SCRIPT,4);break;
        case 9:
            put(&host,WINDOW+0x1c,native_word(0x801fffffu),4);
            g_PsxRam[0x1fffff]=0x80;break;
        case 10: case 11:
            put(&host,SCRIPT,0x20,1);
            D_8005935C=which==10?native_word(0x801ffff6u):native_word(FONT+1);break;
        case 12: put(&host,WINDOW+0x2c,native_word(0x801ffffeu),4);break;
        case 13: put(&host,ROWS+0x54,0xffffu,2);break;
        case 14: put(&host,SCRIPT,0x00080f0fu,4);break;
        case 15:
            put(&host,WINDOW+0xc,0xffffu,2);put(&host,WINDOW,25,2);break;
        }
        memcpy(before_arena,native_arena,SIZE);
        unsigned uploads_before=uploads;
        PcPortMipsExecutionStats stats=PcPortMipsExecutionStatsGet();
        binding_runtime.bridge_cpu=&cpu;
        int status=runtime_bridge_call(&binding_runtime,&cpu,0x80034888u);
        binding_runtime.bridge_cpu=NULL;
        assert(PcPort_SystemWindowGuardSwap(NULL)==NULL);
        PcPortMipsExecutionStats after=PcPortMipsExecutionStatsGet();
        assert(!memcmp(&stats,&after,sizeof(stats)));
        int unchanged_required=which<=2 || which==6 || which==7;
        if(status!=-1 || uploads!=uploads_before ||
           (unchanged_required && memcmp(before_arena,native_arena,SIZE))) {
            fprintf(stderr,"BINDING REJECTION FAIL %s status=%d uploads=%u/%u\n",
                names[which],status,uploads,uploads_before);
            failures++;
        } else printf("BINDING rejected %s; guard restored; no CPU fallback\n",names[which]);
    }
}

int main(int argc,char **argv)
{
    assert(argc==2 || argc==3);
    _Static_assert(sizeof(RECT)==8,"compact RECT");
    native_arena=g_PsxRam+(ARENA-BASE);
    assert((uintptr_t)g_PsxRam+PSX_RAM_SIZE<=UINT32_MAX);
    binding_runtime.initialized=1;
    load_host_ranges(&binding_runtime);
    assert(binding_runtime.host_range_count);
    binding_runtime.functions[0]=(ResolvedFunction){0x80034888u,func_80034888,"func_80034888"};
    binding_runtime.function_count=1;g_ActiveBattleRuntime=&binding_runtime;
    setenv("XENO_BATTLE_NATIVE_WINDOW_RENDER","1",1);
    FILE *f=fopen(argv[1],"rb");assert(f);
    /* Include original text jump tables and all called resident bodies. */
    assert(fseek(f,0x800,SEEK_SET)==0);
    size_t loaded=fread(ram+0x10000,1,0x70000,f);assert(loaded==0x49800);
    assert(fclose(f)==0);
    FILE *battle=fopen("disc/battle.bin","rb");assert(battle);
    assert(fseek(battle,0x80074f70u-BATTLE_BASE,SEEK_SET)==0);
    assert(fread(ram+0x74f70u,1,156,battle)==156);assert(fclose(battle)==0);
    memcpy(g_PsxRam+0x74f70u,ram+0x74f70u,156);
    setenv("XENO_CPU_EXECUTION_STATS","1",1);PcPortMipsExecutionStatsInit();
    Case base={.name="active",.count=3,.cursor=2,.width=24,.flags=4,.x=28,.y=24,.step=14,.selected=1,.speed=4,.script={0x21,0x22,0x80,0x23,0x01,0x24,0}};
    for(int ctx=0;ctx<2;ctx++)for(int row=0;row<3;row++)for(int overflow=0;overflow<2;overflow++)for(int speed=0;speed<5;speed++) {
        Case c=base;c.ctx=ctx;c.row=row;c.glyph_x=overflow?25:0;c.speed=speed;
        c.page=row;run_case(c,33u+(unsigned)speed);
    }
    for(int code=0;code<20;code++)for(int ctx=0;ctx<2;ctx++)for(int queued=0;queued<3;queued++) {
        Case c=base;c.ctx=ctx;c.queued=queued!=0;c.pinned=queued==2;
        c.flags=queued?0:4;c.name="controls";
        memset(c.script,0,sizeof(c.script));
        if(code<4)c.script[0]=(u8)code;
        else { c.script[0]=15;c.script[1]=(u8)(code-4);c.script[2]=1;c.script[3]=1;c.script[4]=0; }
        run_case(c,71);
    }
    const int numbers[]={INT32_MIN,-1,0,1,INT32_MAX};
    for(unsigned n=0;n<5;n++)for(int sub=9;sub<=12;sub++) {
        if(sub==11)continue;
        Case c=base;c.name="numbers";c.number=numbers[n];
        memcpy(c.script,(u8[]){15,(u8)sub,1,0,0,0,0},7);run_case(c,43);
    }
    const int pages[]={-32768,-4,-3,-2,-1,0,1,32766,32767};
    for(unsigned p=0;p<9;p++)for(int ctx=0;ctx<2;ctx++) {
        Case c=base;c.name="page-rollover";c.glyph_x=25;c.page=pages[p];c.ctx=ctx;
        run_case(c,91);
    }
    for(int width=64;width<=66;width++)for(int page=0;page<2;page++)for(int ctx=0;ctx<2;ctx++) {
        Case c=base;c.name="wide-active";c.width=width;c.row=page;c.ctx=ctx;
        memcpy(c.script,(u8[]){0xff,0xff,0x81,0x41,0x21,0x22,0},7);run_case(c,51);
    }
    for(int flags=0;flags<512;flags++) {
        Case c=base;c.name="active-flags";c.flags=flags;c.queued=1;
        c.restore=7;c.suppress=flags&1;run_case(c,63);
    }
    const int timers[]={-32768,-1,0,1,2,32767};
    for(unsigned a=0;a<6;a++)for(unsigned b=0;b<6;b++) {
        Case c=base;c.name="active-timing";c.t84=timers[a];c.t86=timers[b];c.t88=1;
        run_case(c,23);
    }
    for(int ctx=0;ctx<2;ctx++)for(int speed=1;speed<=4;speed++) {
        Case c=base;c.name="eight-frame-text";c.ctx=ctx;c.speed=speed;c.frames=8;
        memcpy(c.script,(u8[]){0x21,0x22,1,0x23,0x24,1,0x25,0x26,1,0x27,0},11);
        run_case(c,53);
    }
    const int slots[]={0,1,2,127,255};
    for(int branches=1;branches<=4;branches++)for(unsigned slot=0;slot<5;slot++)for(int ctx=0;ctx<2;ctx++) {
        Case c=base;c.name="full-caller";c.caller=branches;c.ui_slot=slots[slot];c.ctx=ctx;
        run_case(c,37);
    }
    for(unsigned slot=0;slot<5;slot++)for(int ctx=0;ctx<2;ctx++) {
        Case c=base;c.name="caller-reread-alias";c.caller=2;c.ui_slot=slots[slot];c.ctx=ctx;c.alias=1;
        run_case(c,47);
    }
    check_binding_rejections();
    unsigned covered=0;for(unsigned i=0;i<sizeof(coverage);i++)covered+=coverage[i]!=0;
    printf("WINDOW %s cases=%u failures=%u original_body_instructions=%u/393 oracle_steps=%llu native_interpreted_delta=0 boundary_model=GR_CopyVRAM actual_native_services=window,text,glyph,HeapFree,LoadImage,links\n",
        failures?"FAIL":"PASS",total,failures,covered,(unsigned long long)oracle_steps);
    unsigned tc=0,gc=0;
    for(unsigned i=0;i<sizeof(text_coverage);i++)tc+=text_coverage[i]!=0;
    for(unsigned i=0;i<sizeof(glyph_coverage);i++)gc+=glyph_coverage[i]!=0;
    printf("ACTIVE text_calls=%u glyph_calls=%u frees=%u uploads=%u text_instructions=%u/%zu glyph_instructions=%u/%zu\n",text_calls,glyph_calls,free_calls,uploads,tc,sizeof(text_coverage),gc,sizeof(glyph_coverage));
    unsigned cc=0;for(unsigned i=0;i<39;i++)cc+=caller_coverage[i]!=0;
    printf("CALLER original instructions=%u/39; real AddPrim and typed active window; volatile callee registers excluded from API equivalence\n",cc);
    printf("WINDOW UNEXECUTED");
    for(unsigned i=0;i<sizeof(coverage);i++)if(!coverage[i])printf(" %08x",0x80034888u+4*i);
    putchar('\n');
    assert(PcPort_SystemWindowGuardSwap(NULL)==NULL);
    printf("BINDING actual runtime typed dispatch and guarded guest-OT domain %s\n",failures?"FAIL":"PASS");
    return failures?1:0;
}
