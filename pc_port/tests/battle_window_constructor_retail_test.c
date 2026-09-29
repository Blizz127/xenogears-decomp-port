#define _GNU_SOURCE
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <sys/mman.h>
#include "common.h"
#include "psyq/libgpu.h"
#include "battle_mips_adapter.h"

/* Successful allocation POLICY IS STUBBED. Requests, flags, tags, order and
 * outputs are compared. No allocator, desktop, renderer or game is run. */
#define RAM_BASE 0x80000000u
#define RAM_SIZE 0x200000u
#define ARENA 0x80100000u
#define ARENA_SIZE 0xa0000u
#define WINDOW (ARENA + 0x40u)
#define ROWS (ARENA + 0x1040u)
#define ROW_CAP 0x8000u
#define WORK (ARENA + 0x11040u)
#define WORK_CAP 0x80000u
#define STACK 0x801ff000u
#define HALT 0xfffffffcu

static u8 ram[RAM_SIZE], initial[ARENA_SIZE];
static u8 *native_arena;
s16 g_SystemPalette1, g_SystemPalette2;
extern void func_80032F54(void *, s32,s32,s32,s32,s32,s32,s32);
extern u_short GpuBody_GetTPage(int,int,int,int);
extern void GpuBody_SetSemiTrans(void *,int);
extern void GpuBody_SetDrawMode(DR_MODE *,int,int,int,RECT16 *);

enum { TAG=1, ALLOC, SEMI, PAGE, MODE };
typedef struct { u32 kind, a[5]; } Event;
typedef struct {
    Event events[16]; unsigned count, allocations;
    u32 tag, row_size, work_size;
    int boundary, fault;
} Context;
static Context guest, host;
static jmp_buf host_stop;

static Event *event(Context *c, u32 kind, u32 a, u32 b, u32 d, u32 e, u32 f)
{
    assert(c->count < 16);
    Event *v = &c->events[c->count++];
    *v = (Event){kind,{a,b,d,e,f}};
    return v;
}

static u32 allocate(Context *c, u32 size, u32 flags)
{
    unsigned n = c->allocations++;
    u32 p = n == 0 ? ROWS : WORK;
    int bounded = n < 2 && flags == 2 && size <= (n == 0 ? ROW_CAP : WORK_CAP);
    event(c, ALLOC, size, flags, c->tag, bounded ? p : 0, n);
    c->tag = 0x20;
    if (!bounded) { c->boundary = 1; return 0; }
    if (n == 0) c->row_size = size; else c->work_size = size;
    return p;
}

void HeapSetCurrentContentType(u_short tag)
{
    host.tag = tag; event(&host, TAG, tag,0,0,0,0);
}
void *HeapAlloc(u_int size, u_int flags)
{
    u32 p = allocate(&host,size,flags);
    if (host.boundary) longjmp(host_stop,1);
    return (void *)(uintptr_t)p;
}
void SetSemiTrans(void *p, int enabled)
{
    event(&host,SEMI,(u32)(uintptr_t)p,(u32)enabled,0,0,0);
    GpuBody_SetSemiTrans(p,enabled);
}
u_short GetTPage(int a,int b,int x,int y)
{
    event(&host,PAGE,(u32)a,(u32)b,(u32)x,(u32)y,0);
    return GpuBody_GetTPage(a,b,x,y);
}
void SetDrawMode(DR_MODE *p,int dfe,int dtd,int page,RECT16 *tw)
{
    event(&host,MODE,(u32)(uintptr_t)p,(u32)dfe,(u32)dtd,(u32)page,(u32)(uintptr_t)tw);
    GpuBody_SetDrawMode(p,dfe,dtd,page,tw);
}

static int in(u32 a, unsigned w, u32 b, u32 n)
{
    return a >= b && (uint64_t)a+w <= (uint64_t)b+n;
}
static int read_bus(void *opaque,u32 a,unsigned w,u32 *v)
{
    (void)opaque;
    if (w == 0 || w > 4 || !in(a,w,RAM_BASE,RAM_SIZE)) return -1;
    *v=0; for(unsigned i=0;i<w;i++) *v|=(u32)ram[a-RAM_BASE+i]<<(i*8);
    return 0;
}
static int write_bus(void *opaque,u32 a,unsigned w,u32 v)
{
    Context *c=opaque;
    if (!(in(a,w,WINDOW,0x90) || in(a,w,ROWS,c->row_size) ||
          in(a,w,WORK,c->work_size) || in(a,w,STACK-0x200,0x280))) {
        c->fault=1; return -1;
    }
    if (w == 0 || w > 4 || !in(a,w,RAM_BASE,RAM_SIZE)) return -1;
    for(unsigned i=0;i<w;i++) ram[a-RAM_BASE+i]=(u8)(v>>(i*8));
    return 0;
}
static int bridge(void *opaque,PcPortMipsCpu *cpu,u32 pc)
{
    Context *c=opaque; u32 *a=&cpu->gpr[4];
    switch(pc) {
    case 0x800324b8u:
        c->tag=a[0]&0xffff; event(c,TAG,c->tag,0,0,0,0); return 1;
    case 0x80031bdcu:
        cpu->gpr[2]=allocate(c,a[0],a[1]); return c->boundary ? -1 : 1;
    case 0x80043bfcu: event(c,SEMI,a[0],a[1],0,0,0); break;
    case 0x80043a1cu: event(c,PAGE,a[0],a[1],a[2],a[3],0); break;
    case 0x800454dcu: {
        u32 fifth; assert(read_bus(c,cpu->gpr[29]+0x10,4,&fifth)==0);
        event(c,MODE,a[0],a[1],a[2],a[3],fifth); break;
    }
    default: break;
    }
    return 0; /* Every GPU instruction, including internal leaves, executes. */
}
static void put(u32 a,u32 v,unsigned w)
{
    assert(in(a,w,RAM_BASE,RAM_SIZE));
    for(unsigned i=0;i<w;i++) ram[a-RAM_BASE+i]=(u8)(v>>(i*8));
}

typedef struct { const char *name; s32 tx,ty,x,y,width,rows; } Case;
static const Case cases[]={
    {"opening9",896,256,28,24,24,4},
    {"opening10",896,256,50,24,39,8},
    {"opening11",896,256,32,148,48,8},
    {"opening12",896,256,68,148,30,8},
    {"opening13",896,256,92,24,27,8},
    {"opening14",896,256,32,24,48,8},
    {"zero_rows",896,256,28,24,24,0},
    {"one_row",896,256,28,24,24,1},
    {"split63",896,256,28,24,63,3},
    {"split64",896,256,28,24,64,3},
    {"uv_wrap",919,250,28,24,64,9},
    {"background_x0",896,256,0,24,24,4},
    {"background_x6",896,256,6,24,24,4},
    {"background_x7",896,256,7,24,24,4},
    {"x_negative",896,256,-1,24,24,4},
    {"right_x_negative",896,256,-257,24,24,4},
    {"y_negative",896,256,28,-1,24,4},
    {"signed_min_xy",896,256,-32768,-32768,24,4},
    {"signed_max_xy",896,256,32767,32767,24,4},
    {"signed_left_width",896,256,28,24,8192,4},
    {"wide_background",896,256,28,24,16384,4},
    {"negative_width_minus1",896,256,28,24,-1,4},
    {"negative_width_minus3_zero_work",896,256,28,24,-3,4},
    {"high_coordinate_bits",(s32)0x80000380u,(s32)0x80000100u,(s32)0x8000001cu,0x10018,(s32)0x80000018u,4},
    {"high_row_bits",896,256,28,24,24,0x10004},
    {"top_row_bits",896,256,28,24,24,(s32)0x80000004u},
    {"texture_signed_max",0x7fffffff,0x7fffffff,28,24,24,8},
    {"texture_signed_min",(s32)0x80000000u,(s32)0x80000000u,28,24,24,8},
    {"boundary_negative_rows",896,256,28,24,24,-1},
    {"boundary_negative_stride",896,256,28,24,-5,4},
    {"boundary_stride_signed_wrap",896,256,28,24,32767,4},
};

static const char *region(u32 address)
{
    if(in(address,1,WINDOW,0x90)) return "window";
    if(in(address,1,ROWS,ROW_CAP)) return "rows";
    if(in(address,1,WORK,WORK_CAP)) return "work";
    return "redzones/unused";
}
static int compare_case(const Case *c,unsigned salt,unsigned *boundary_cases,uint64_t *steps)
{
    memset(&guest,0,sizeof(guest)); memset(&host,0,sizeof(host));
    for(unsigned i=0;i<ARENA_SIZE;i++) initial[i]=(u8)(salt+i*37u+(i>>8)*13u);
    memcpy(ram+(ARENA-RAM_BASE),initial,ARENA_SIZE);
    memcpy(native_arena,initial,ARENA_SIZE);
    memset(ram+(STACK-RAM_BASE-0x200),0xa5,0x280);
    g_SystemPalette1=(s16)(0x4567u^salt); g_SystemPalette2=(s16)(0x9abcu^salt);
    put(0x800595d4u,(u16)g_SystemPalette1,2); put(0x80059414u,(u16)g_SystemPalette2,2);
    put(0x800568d0u,0,1); /* Normal PSX GPU mode; alternate chip mask excluded. */
    put(STACK+0x10,(u32)c->y,4); put(STACK+0x14,(u32)c->width,4); put(STACK+0x18,(u32)c->rows,4);
    put(STACK+0x1c,0xdeadc0deu,4);
    PcPortMipsCpu cpu;
    PcPortMipsBus bus={.opaque=&guest,.read=read_bus,.write=write_bus,.bridge=bridge};
    PcPortMipsCpuInit(&cpu,&bus);
    cpu.gpr[4]=WINDOW; cpu.gpr[5]=(u32)c->tx; cpu.gpr[6]=(u32)c->ty; cpu.gpr[7]=(u32)c->x;
    cpu.gpr[29]=STACK; cpu.gpr[31]=HALT;
    for(unsigned i=16;i<=23;i++) cpu.gpr[i]=0xa5000000u+i;
    int rc=PcPortMipsRun(&cpu,0x80032f54u,HALT,20000);
    *steps+=cpu.steps;
    int oracle_ok=(rc==PC_PORT_MIPS_HALTED || (guest.boundary && rc==PC_PORT_MIPS_UNRESOLVED_CALL));
    if(!oracle_ok || guest.fault) {
        fprintf(stderr,"ORACLE_ERROR %s salt=%u rc=%d pc=%08x %s\n",c->name,salt,rc,cpu.pc,cpu.error);
        exit(2);
    }
    if(!guest.boundary) {
        assert(cpu.gpr[29]==STACK);
        for(unsigned i=16;i<=23;i++) assert(cpu.gpr[i]==0xa5000000u+i);
    } else (*boundary_cases)++;
    if(setjmp(host_stop)==0)
        func_80032F54((void*)(uintptr_t)WINDOW,c->tx,c->ty,c->x,c->y,c->width,0x5a5a,c->rows);
    int bad=guest.boundary!=host.boundary || guest.count!=host.count ||
        memcmp(guest.events,host.events,sizeof(guest.events))!=0;
    unsigned changed=0, first=0;
    for(unsigned i=0;i<ARENA_SIZE;i++) if(native_arena[i]!=ram[ARENA-RAM_BASE+i]) {
        if(!changed) first=i;
        changed++;
    }
    if(changed) bad=1;
    if(g_SystemPalette1!=(s16)(0x4567u^salt) || g_SystemPalette2!=(s16)(0x9abcu^salt)) bad=1;
    if(bad) {
        printf("DIFF %s salt=%u bytes=%u",c->name,salt,changed);
        if(changed) printf(" first=%s:%08x native=%02x retail=%02x",region(ARENA+first),ARENA+first,native_arena[first],ram[ARENA-RAM_BASE+first]);
        printf(" boundary=%d/%d events=%u/%u\n",host.boundary,guest.boundary,host.count,guest.count);
        unsigned n=host.count<guest.count ? host.count : guest.count;
        for(unsigned i=0;i<n;i++) if(memcmp(&host.events[i],&guest.events[i],sizeof(Event))) {
            printf(" EVENT[%u] native=%u:%08x,%08x,%08x,%08x,%08x retail=%u:%08x,%08x,%08x,%08x,%08x\n",i,
                   host.events[i].kind,host.events[i].a[0],host.events[i].a[1],host.events[i].a[2],host.events[i].a[3],host.events[i].a[4],
                   guest.events[i].kind,guest.events[i].a[0],guest.events[i].a[1],guest.events[i].a[2],guest.events[i].a[3],guest.events[i].a[4]);
            break;
        }
    }
    return bad;
}

static void load(const char *dir,const char *name,u32 addr,size_t count)
{
    char path[1024]; assert(snprintf(path,sizeof(path),"%s/retail.%s.bin",dir,name)>0);
    FILE *f=fopen(path,"rb"); assert(f);
    assert(fread(ram+(addr-RAM_BASE),1,count,f)==count); assert(fgetc(f)==EOF); assert(fclose(f)==0);
}
int main(int argc,char **argv)
{
    assert(argc==2 || argc==3); _Static_assert(sizeof(DR_MODE)==12,"native DR_MODE packing");
    native_arena=mmap((void*)(uintptr_t)ARENA,ARENA_SIZE,PROT_READ|PROT_WRITE,
        MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0);
    if(native_arena==MAP_FAILED) { perror("exclusive scratch arena mmap"); return 2; }
    assert((uintptr_t)native_arena==ARENA);
    load(argv[1],"constructor",0x80032f54u,0x4d8);
    load(argv[1],"gettpage",0x80043a1cu,0x3c);
    load(argv[1],"semitrans",0x80043bfcu,0x28);
    load(argv[1],"setdrawmode",0x800454dcu,0x58);
    load(argv[1],"getmode",0x800459dcu,0x58);
    load(argv[1],"gettw",0x80045c10u,0x84);
    unsigned failures=0,total=0,boundaries=0; uint64_t steps=0;
    const unsigned salts[]={0x21,0x5b,0xaa,0xef};
    for(unsigned s=0;s<sizeof(salts)/sizeof(salts[0]);s++)
        for(unsigned i=0;i<sizeof(cases)/sizeof(cases[0]);i++) {
            if(argc==3 && strcmp(argv[2],cases[i].name)) continue;
            failures+=compare_case(&cases[i],salts[s],&boundaries,&steps); total++;
        }
    assert(total==(argc==3 ? 4u : 124u));
    if(argc==2) assert(boundaries==12);
    printf("CONSTRUCTOR_RETAIL_%s cases=%u complete=%u allocator_boundary_only=%u failures=%u patterns=4 arena_bytes_per_case=%u raw_mips_steps=%llu allocator=BOUNDED_STUB gpu=RETAIL_BYTES\n",
        failures?"FAIL":"PASS",total,total-boundaries,boundaries,failures,ARENA_SIZE,(unsigned long long)steps);
    assert(munmap(native_arena,ARENA_SIZE)==0);
    return failures?1:0;
}
