/* Retail differential for the work-list insertion arms:
 *   WorkListAddTask (0x8001CA58)  TimerWorkListAddTask (0x8001CC18)
 * The production body lives in pc_port/src/work_list_port.c (the matching
 * src/slus_006.64/system/work_list.c is a reference-only TU the port link
 * replaces); that TU is linked unmodified.  Retail = pinned disc/SLUS_006.64
 * on PcPortMipsRun (test oracle only).  Both sides see the same 0x1C-byte
 * entries and owner at identical addresses; the only value that legitimately
 * differs by construction is the stored free-callback pointer (host address
 * of WorkListRemoveTask/TimerWorkListRemoveTask vs the retail guest address),
 * which the native wrapper normalises to the retail address before the byte
 * compare.  A NULL owner (battle's idiom) reads RAM address 0x10 on both
 * sides.  usage: main_exe_work_list_retail_test [fn-name] */
#include "slus_oracle.h"

typedef struct WorkListEntry WorkListEntry;
extern void WorkListAddTask(WorkListEntry*, WorkListEntry*);
extern void TimerWorkListAddTask(WorkListEntry*, WorkListEntry*);
extern void WorkListRemoveTask(WorkListEntry*);
extern void TimerWorkListRemoveTask(WorkListEntry*);
extern s32 D_80059184, D_80059464;
extern u8 D_800591AC;

/* externs of work_list_port.c that the production runtime defines elsewhere */
WorkListEntry *g_WorkList, *g_TimerWorkList, *D_800594C0, *D_80059590;
s32 g_NumWorkListEntries, g_NumTimerWorkListEntries, g_WorkListCurTimer, D_80059190;
short D_80059494;
void* HeapAlloc(unsigned s, unsigned f) { (void)s; (void)f; abort(); }
unsigned HeapFree(void* p) { (void)p; abort(); }
void func_8001DAE8(void* a, u16 b, u32 c) { (void)a; (void)b; (void)c; abort(); }
void func_800234AC(void* a) { (void)a; abort(); }
void func_8001F8E8(void* a, u16 b, u32 c) { (void)a; (void)b; (void)c; abort(); }
u8 D_8005A474[16], D_8006BE10[16];
int g_GPUDisabledState;
unsigned g_PcPortPresentedFrames;
uint8_t g_PsxScratchpad[4096];

static const Glob g_wl[] = {
    { "D_80059184", &D_80059184, 0x80059184u, 4, 0 },
    { "g_WorkList", &g_WorkList, 0x80059594u, 8, 1 },
    { "g_TimerWorkList", &g_TimerWorkList, 0x8005958Cu, 8, 1 },
    { "g_NumWorkListEntries", &g_NumWorkListEntries, 0x8005918Cu, 4, 0 },
    { "g_NumTimerWorkListEntries", &g_NumTimerWorkListEntries, 0x80059188u, 4, 0 },
    { "D_80059464", &D_80059464, 0x80059464u, 4, 0 },
    { "D_800591AC", &D_800591AC, 0x800591ACu, 1, 0 },
};
#define O_OWNER 0x0100u
#define O_ENTRY 0x0200u

static void setup(uint32_t id)
{
    for (unsigned i = 0; i < 0x400; i++) low[i] = (u8)rnd();
    for (unsigned i = 0; i < 0x20; i++) g_PsxRam[i] = (u8)rnd(); /* RAM @0x10 for a NULL owner */
    D_80059184 = rnd_val();
    g_WorkList = rn(3) ? (WorkListEntry*)(uintptr_t)lo_addr(0x300 + 0x20 * rn(8)) : NULL;
    g_TimerWorkList = rn(3) ? (WorkListEntry*)(uintptr_t)lo_addr(0x340 + 0x20 * rn(8)) : NULL;
    g_NumWorkListEntries = rnd_val(); g_NumTimerWorkListEntries = rnd_val(); D_80059464 = rnd_val();
    D_800591AC = (u8)(rn(3) ? rn(2) : rnd());
    A[0] = (id % 6 == 5) ? 0 : lo_addr(O_OWNER);
    A[1] = lo_addr(O_ENTRY);
}
static void normalise(void)
{
    u32 *cb = (u32*)lo(O_ENTRY + 0xC);
    if (*cb == (u32)(uintptr_t)WorkListRemoveTask) *cb = 0x8001CB48u;
    else if (*cb == (u32)(uintptr_t)TimerWorkListRemoveTask) *cb = 0x8001CD94u;
}
static uint32_t nat_add(void) { WorkListAddTask((WorkListEntry*)(uintptr_t)A[0], (WorkListEntry*)(uintptr_t)A[1]); normalise(); return 0; }
static uint32_t nat_tadd(void) { TimerWorkListAddTask((WorkListEntry*)(uintptr_t)A[0], (WorkListEntry*)(uintptr_t)A[1]); normalise(); return 0; }

static const Fn fns[] = {
    { "WorkListAddTask", 0x8001CA58u, 0, setup, nat_add },
    { "TimerWorkListAddTask", 0x8001CC18u, 0, setup, nat_tadd },
};

int main(int argc, char **argv)
{
    const char *only = argc > 1 ? argv[1] : NULL;
    unsigned cases = getenv("XENO_CASES") ? (unsigned)atoi(getenv("XENO_CASES")) : 4000;
    int bad = 0, ran = 0;
    cur_what = "MAIN EXE WORK LIST";
    setvbuf(stdout, NULL, _IOLBF, 0);
    map_low();
    load_slus(cur_what);
    globs = g_wl; nglobs = sizeof(g_wl) / sizeof(g_wl[0]);
    for (unsigned k = 0; k < sizeof(fns) / sizeof(fns[0]); k++) {
        const Fn *f = &fns[k];
        unsigned fails = 0;
        if (only && strcmp(only, f->name)) continue;
        prints_left = 1;
        for (unsigned id = 0; id < cases; id++) fails += (unsigned)run_case(f, id + 1, 0);
        printf("MAIN EXE WORK LIST %s %s cases=%u diverged=%u\n", f->name, fails ? "DIVERGES" : "PASS", cases, fails);
        ran++; bad += fails ? 1 : 0;
    }
    if (!ran) { fprintf(stderr, "no such function\n"); return 2; }
    printf("MAIN EXE WORK LIST %s (%d function(s) diverge)\n", bad ? "FAIL" : "ALL PASS", bad);
    return bad ? 1 : 0;
}
