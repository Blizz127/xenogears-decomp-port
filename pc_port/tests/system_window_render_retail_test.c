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

/* High-level C API differential, not a CPU-register bridge proof.
 * HeapFree, text rendering and LoadImage are explicit boundary spies.
 * Original31798/AddPrim instructions execute in the oracle; actual full
 * temp2.c and guest_prim_link.c bodies execute in the native leg. Wrappers
 * observe arguments and inject the same explicit callback changes as the oracle.
 * This window suite uses native compact pointers; the separate leaf suite
 * covers both native and guest OT domains.
 * Allocator policy, text pixels, VRAM/GPU execution and bus faults are excluded.
 * Valid windows/row allocations are disjoint, padded, four-byte aligned.
 * Context is 0 or 1; live row-array changes occur at service boundaries.
 */
#define BASE 0x80000000u
#define RAM_SIZE 0x200000u
#define ARENA 0x80100000u
#define SIZE 0x10000u
#define WINDOW (ARENA+0x100u)
#define ROWS (ARENA+0x1000u)
#define ROWS2 (ARENA+0x4000u)
#define NODE (ARENA+0x8000u)
#define DATA (ARENA+0x8100u)
#define DATA2 (ARENA+0x8200u)
#define OT (ARENA+0x9000u)
#define STACK 0x801ff000u
#define HALT 0xfffffffcu

extern void func_80034888(void *,void *,s32);
s16 g_SystemPalette1, g_SystemPalette2;
_Alignas(16) uint8_t g_PsxRam[PSX_RAM_SIZE];
extern void WindowBody_LinkPrim(void *,void *);
extern void WindowBody_AddPrim(void *,void *);
static int actual_links;
static u8 ram[RAM_SIZE], initial[SIZE], *native_arena;
static unsigned char coverage[1572/4];
enum { FREE=1,TEXT,UPLOAD,LINK,ADD };
typedef struct { u32 kind,a,b,words[6]; } Event;
typedef struct { Event events[128]; unsigned count,links; int native,hook; } Context;
static Context host,guest;

static u8 *ptr(Context *c,u32 a,unsigned size)
{
    if(a<0x200000u) a|=BASE;
    assert(a>=BASE && (uint64_t)a+size<=BASE+RAM_SIZE);
    if(c->native) {
        assert(a>=ARENA && (uint64_t)a+size<=ARENA+SIZE);
        return native_arena+(a-ARENA);
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
static void service(Context *c,u32 kind,u32 a,u32 b)
{
    event(c,kind,a,b);
    if(kind==FREE && c->hook==3) {
        put(c,WINDOW+0x6a,0xd3,1);
        put(c,WINDOW+0x82,0x8000,2);
        put(c,WINDOW+0x10,0x102,2);
        put(c,WINDOW+0x28,ROWS2,4);
    }
    if(kind==TEXT) {
        /* Controlled text-boundary side effects, compared in both legs. */
        put(c,DATA+4,0x76543210u,4);
        if(c->hook==4) {
            put(c,WINDOW+0x28,ROWS2,4);
            put(c,WINDOW+2,2,2);
            put(c,WINDOW+0x2c,DATA2,4);
            put(c,WINDOW+0x84,2,2);
        }
    }
}
u_int HeapFree(void *p) { service(&host,FREE,(u32)(uintptr_t)p,0);return 0; }
void func_80033DF0(void *p) { service(&host,TEXT,(u32)(uintptr_t)p,0); }
int LoadImage(RECT *r,u_long *p)
{
    service(&host,UPLOAD,(u32)(uintptr_t)r,(u32)(uintptr_t)p);return 0;
}
void func_80031798(void *ot,void *p)
{
    u32 a=(u32)(uintptr_t)ot,b=(u32)(uintptr_t)p;
    event(&host,LINK,a,b);link_hook(&host);
    actual_links++;WindowBody_LinkPrim(ot,p);
}
void PcPort_AddPrimDomainAware(void *ot,void *p)
{
    u32 a=(u32)(uintptr_t)ot,b=(u32)(uintptr_t)p;
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
    switch(pc) {
    case 0x800320e8u: service(c,FREE,a,0);cpu->gpr[2]=0;return 1;
    case 0x80033df0u: service(c,TEXT,a,0);return 1;
    case 0x80044894u: service(c,UPLOAD,a,b);cpu->gpr[2]=0;return 1;
    case 0x80031798u: event(c,LINK,a,b);link_hook(c);return 0;
    case 0x80043b48u: event(c,ADD,a,b);add_hook(c,b);return 0;
    default: return 0;
    }
}
typedef struct {
    const char *name;
    int count,cursor,width,flags,ctx,t84,t86,t88,queued,hook,x,y,step,selected;
} Case;
static uint64_t oracle_steps;
static unsigned total,failures;

static void run_case(Case c,unsigned salt)
{
    guest=(Context){.hook=c.hook};host=(Context){.native=1,.hook=c.hook};
    for(unsigned i=0;i<SIZE;i++)initial[i]=(u8)(salt+i*37u+(i>>8)*13u);
    memcpy(ram+ARENA-BASE,initial,SIZE);
    put(&guest,WINDOW+0xc,(u16)c.count,2);
    put(&guest,WINDOW+0x16,(u16)c.cursor,2);
    put(&guest,WINDOW+0x10,(u16)c.flags,2);
    put(&guest,WINDOW+0x28,ROWS,4);put(&guest,WINDOW+0x2c,DATA,4);
    put(&guest,WINDOW+0x8c,NODE,4);put(&guest,NODE,0x80108040,4);
    put(&guest,NODE+4,DATA2,4);put(&guest,WINDOW+0x82,(u16)c.queued,2);
    put(&guest,WINDOW+0x84,(u16)c.t84,2);put(&guest,WINDOW+0x86,(u16)c.t86,2);
    put(&guest,WINDOW+0x88,(u16)c.t88,2);
    put(&guest,WINDOW+2,1,2);put(&guest,WINDOW+4,(u16)c.x,2);
    put(&guest,WINDOW+6,(u16)c.y,2);put(&guest,WINDOW+0xa,(u16)c.width,2);
    put(&guest,WINDOW+0x14,(u16)c.step,2);
    put(&guest,WINDOW+0x6e,(u8)c.selected,1);
    for(int bank=0;bank<2;bank++)for(int row=-1;row<34;row++) {
        u32 a=(bank?ROWS2:ROWS)+row*0x60;
        put(&guest,a+0x58,(u16)c.width,2);
        put(&guest,a+0x5c,(u8)(row*17+bank*23+salt),1);
        put(&guest,a+0x5e,(u16)(row*193+bank*321+salt),2);
    }
    g_SystemPalette1=(s16)(0xc53a^salt);
    put(&guest,0x800595d4,(u16)g_SystemPalette1,2);
    memcpy(native_arena,ram+ARENA-BASE,SIZE);
    PcPortMipsCpu cpu;PcPortMipsBus bus={.opaque=&guest,.read=read_bus,.write=write_bus,.bridge=bridge};
    PcPortMipsCpuInit(&cpu,&bus);
    cpu.gpr[4]=WINDOW;cpu.gpr[5]=OT;cpu.gpr[6]=c.ctx;
    cpu.gpr[29]=STACK;cpu.gpr[31]=HALT;
    for(unsigned i=16;i<=23;i++)cpu.gpr[i]=0x43210000u+i;
    setenv("XENO_DISABLE_CPU_INTERPRETER","0",1);
    int rc=PcPortMipsRun(&cpu,0x80034888,HALT,20000);oracle_steps+=cpu.steps;
    if(rc!=PC_PORT_MIPS_HALTED) {
        fprintf(stderr,"WINDOW ORACLE ERROR case=%s rc=%d pc=%08x %s\n",c.name,rc,cpu.pc,cpu.error);exit(2);
    }
    assert(cpu.gpr[29]==STACK);
    for(unsigned i=16;i<=23;i++)assert(cpu.gpr[i]==0x43210000u+i);
    setenv("XENO_DISABLE_CPU_INTERPRETER","1",1);
    assert(PcPortMipsInterpreterDisabled());
    PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
    actual_links=0;PcPort_PrimLinkReset();
    func_80034888((void*)(uintptr_t)WINDOW,(void*)(uintptr_t)OT,c.ctx);
    assert(PcPort_PrimLinkNativeCount()==actual_links);
    assert(PcPort_PrimLinkGuestCount()==0 && PcPort_PrimLinkRejectCount()==0);
    PcPortMipsExecutionStats after=PcPortMipsExecutionStatsGet();
    assert(!memcmp(&before,&after,sizeof(before)));
    unsigned changed=0,first=0;
    for(unsigned i=0;i<SIZE;i++)if(native_arena[i]!=ram[ARENA-BASE+i]) {
        if(!changed)first=i;changed++;
    }
    int bad=changed || host.count!=guest.count || memcmp(host.events,guest.events,sizeof(host.events));
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
    if(!strcmp(c.name,"row-wrap-witness")) {
        unsigned n=0;const int order[]={2,0,1,2,0,1};
        for(unsigned i=0;i<guest.count;i++)if(guest.events[i].kind==LINK) {
            u32 expected=ROWS+(u32)order[n]*0x60+(u32)c.ctx*0x28+(n<3?0x14:0);
            assert(guest.events[i].b==expected);n++;
        }
        assert(n==6);
    }
}
static void load(FILE *f,u32 pc,unsigned size)
{
    assert(fseek(f,(long)(pc-0x80010000u+0x800),SEEK_SET)==0);
    assert(fread(ram+pc-BASE,1,size,f)==size);
}
int main(int argc,char **argv)
{
    assert(argc==2 || argc==3);
    _Static_assert(sizeof(RECT)==8,"compact RECT");
    native_arena=mmap((void*)(uintptr_t)ARENA,SIZE,PROT_READ|PROT_WRITE,
        MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0);
    if(native_arena==MAP_FAILED){perror("exclusive test arena");return 2;}
    assert((uintptr_t)native_arena==ARENA);
    FILE *f=fopen(argv[1],"rb");assert(f);
    load(f,0x80034888,1572);load(f,0x80031798,36);load(f,0x80043b48,60);
    assert(fclose(f)==0);
    setenv("XENO_CPU_EXECUTION_STATS","1",1);PcPortMipsExecutionStatsInit();
    Case base={.name="row-wrap-witness",.count=3,.cursor=2,.width=65,.flags=14,.x=28,.y=24,.step=14,.selected=1};
    for(int ctx=0;ctx<2;ctx++){Case c=base;c.ctx=ctx;run_case(c,33);}
    if(argc==2) {
        const int counts[]={-32768,-1,0,1,2,3,16};
        const int widths[]={-32768,-1,0,1,64,65,66,32767};
        for(unsigned n=0;n<7;n++)for(unsigned w=0;w<8;w++)for(int cur=0;cur<5;cur++)for(int ctx=0;ctx<2;ctx++) {
            Case c=base;c.name="row-matrix";c.count=counts[n];c.width=widths[w];c.ctx=ctx;
            c.cursor=cur==0?0:cur==1?c.count-1:cur==2?c.count:cur==3?32767:-1;
            if(c.count<0)c.cursor=0;
            c.selected=(int)((w+n)%5);run_case(c,91);
        }
        for(int flags=0;flags<512;flags++)for(int ctx=0;ctx<2;ctx++) {
            Case c=base;c.name="flags";c.flags=flags;c.ctx=ctx;c.queued=flags&1;
            c.x=-32768;c.y=-32768;c.step=-32768;run_case(c,170);
        }
        const int timers[]={-32768,-1,0,1,2,32767};
        for(unsigned a=0;a<6;a++)for(unsigned b=0;b<6;b++) {
            Case c=base;c.name="timers";c.flags=4;c.t84=timers[a];c.t86=timers[b];c.t88=timers[(a+b)%6];
            c.x=-1;c.y=0;c.width=-32768;run_case(c,239);
        }
        for(int h=1;h<=4;h++)for(int ctx=0;ctx<2;ctx++) {
            Case c=base;c.name="service-rebind";c.hook=h;c.ctx=ctx;
            if(h==3){c.flags=0;c.queued=1;}if(h==4)c.flags=4;
            run_case(c,53);
        }
        const int coords[]={-32768,-1,0,6,7,32767};
        for(unsigned x=0;x<6;x++)for(unsigned y=0;y<6;y++) {
            Case c=base;c.name="background";c.flags=12;c.x=coords[x];c.y=coords[y];
            c.width=widths[(x+y)%8];c.step=coords[(x+y)%6];run_case(c,83);
        }
    }
    unsigned covered=0;for(unsigned i=0;i<sizeof(coverage);i++)covered+=coverage[i]!=0;
    printf("WINDOW %s cases=%u failures=%u original_body_instructions=%u/393 oracle_steps=%llu native_adapter_delta=0 boundary_spies=HeapFree,text,LoadImage actual_native_packet_links=1\n",
        failures?"FAIL":"PASS",total,failures,covered,(unsigned long long)oracle_steps);
    printf("WINDOW UNEXECUTED");
    for(unsigned i=0;i<sizeof(coverage);i++)if(!coverage[i])printf(" %08x",0x80034888u+4*i);
    putchar('\n');
    assert(munmap(native_arena,SIZE)==0);
    return failures?1:0;
}
