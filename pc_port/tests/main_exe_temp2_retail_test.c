/* Retail differential for hand-written main-executable arms of
 * src/slus_006.64/system/temp2.c (XENO_PC_PORT):
 *   func_80030228 func_800302D4 func_80030988 func_80030A30 func_80030B14
 *   func_8002DB84 func_8002D244 func_8002D420 func_8002D814 func_8002DA14
 * The production temp2.c TU is linked unmodified (port-owned symbols aside).
 * Retail = the pinned disc/SLUS_006.64 bytes on PcPortMipsRun (test oracle
 * only).  See slus_oracle.h for the comparison contract.
 * usage: main_exe_temp2_retail_test [fn-name]   (one function, for mutants) */
#include "slus_oracle.h"

/* ---- native bodies under test ---- */
extern s32 func_8002D244(u8*, s16*);
extern s32 func_8002D420(u8*, s16*);
extern s32 func_8002D814(u8*, s16*, s32);
extern s32 func_8002DA14(u8*, s16*);
extern void func_8002DB84(void*, void*, void*, void*);
extern void func_80030228(u8*, s16*, s32, s32);
extern void func_800302D4(u8*, s16*, s32, s32);
extern void func_80030988(u16, u16, s16, s16);
extern void func_80030A30(s32, void*);
extern void func_80030B14(void*);

/* ---- libgte helpers: one shared host implementation for both sides ---- */
extern long VectorNormalS(void*, void*);
extern long VectorNormalSS(void*, void*);
extern void OuterProduct0(void*, void*, void*);
extern int SquareRoot0(int);
extern void NormalColor(void*, void*);
extern void NormalColor3(void*, void*, void*, void*, void*, void*);

/* ---- native globals the units read/write ---- */
u8* D_80059424;
u32 D_8005952C, D_8005953C, D_80059498;
u16 D_80059308, D_8005930C;
extern s16 D_800308D0[0x48];
extern u8 D_80059F64[32], D_80059F84[32];

/* func_8002CD64 is a callee spy on both sides (tested with the other code
 * gates); the production definition is weakened by the runner. */
s32 func_8002CD64(u8* p) { return spy(1, (uint32_t)(uintptr_t)p, 0, 0, 0, 0, 0); }

/* link-only stubs (never reached) */
int g_GPUDisabledState;
unsigned g_PcPortPresentedFrames;
int PsyX_Sys_GetVBlankCount(void) { return 0; }
void PsyX_UpdateInput(void) {}
void ControllerPoll(void) {}
void ControllerPushState(void) {}
char PsyX_BeginScene(void) { abort(); }
void ClearSplits(void) { abort(); }
void DrawAllSplits(void) { abort(); }
void ParsePrimitivesLinkedList(unsigned long *p, int s) { (void)p; (void)s; abort(); }
uint8_t g_PsxScratchpad[4096];

/* ---- retail addresses of the bridged libgte helpers ---- */
#define R_SQRT0   0x80048C4Cu
#define R_VNORMS  0x80048D68u
#define R_VNORMSS 0x80048DA8u
#define R_NCOLOR  0x8004A19Cu
#define R_NCOLOR3 0x8004A1B8u
#define R_OUTER0  0x8004A4D8u
#define R_CD64    0x8002CD64u

static int bridge(PcPortMipsCpu *c, uint32_t t)
{
    switch (t) {
    case R_SQRT0: c->gpr[2] = (uint32_t)SquareRoot0((int)c->gpr[4]); return 1;
    case R_VNORMS: c->gpr[2] = (uint32_t)VectorNormalS(hp(c->gpr[4]), hp(c->gpr[5])); return 1;
    case R_VNORMSS: c->gpr[2] = (uint32_t)VectorNormalSS(hp(c->gpr[4]), hp(c->gpr[5])); return 1;
    case R_OUTER0: OuterProduct0(hp(c->gpr[4]), hp(c->gpr[5]), hp(c->gpr[6])); return 1;
    case R_NCOLOR: NormalColor(hp(c->gpr[4]), hp(c->gpr[5])); return 1;
    case R_NCOLOR3:
        NormalColor3(hp(c->gpr[4]), hp(c->gpr[5]), hp(c->gpr[6]), hp(c->gpr[7]),
                     hp(g32(c->gpr[29] + 16)), hp(g32(c->gpr[29] + 20)));
        return 1;
    case R_CD64: c->gpr[2] = (uint32_t)spy(1, c->gpr[4], 0, 0, 0, 0, 0); return 1;
    default: return 0;
    }
}

/* ---- low-region layout ---- */
#define O_VERT   0x0000u   /* 64 SVECTORs (8 bytes) */
#define O_DELTA  0x0400u
#define O_OUT    0x1000u   /* packet window */
#define O_NORM   0x2000u   /* 64 normals (8 bytes) */
#define O_REC    0x2800u   /* record stream */
#define O_COL    0x3000u
#define O_IDX    0x3100u
#define O_LIGHT  0x3200u
#define O_MAT    0x3300u
#define O_OUTN   0x3400u
#define O_V012   0x3500u
#define O_VERT2  0x4000u
#define LOWC(o)  ((void *)(uintptr_t)lo_addr(o))

static void fill_random_low(void)
{
    for (unsigned i = 0; i < LOW_CMP; i++) low[i] = (u8)rnd();
}
static void rand_vertices(unsigned base, unsigned n, int degenerate)
{
    for (unsigned i = 0; i < n; i++) {
        s16 *v = (s16 *)lo(base + i * 8);
        for (int k = 0; k < 4; k++) {
            if (degenerate) v[k] = (s16)rrange(-2, 2);
            else v[k] = (rnd() & 7) ? (s16)rrange(-3000, 3000) : (s16)rnd_val();
        }
    }
}

/* ---- globals tables ---- */
static const Glob g_poly[] = {
    { "D_80059424", &D_80059424, 0x80059424u, 8, 1 },
    { "D_8005952C", &D_8005952C, 0x8005952Cu, 4, 0 },
    { "D_8005953C", &D_8005953C, 0x8005953Cu, 4, 0 },
    { "D_80059498", &D_80059498, 0x80059498u, 4, 0 },
    { "D_80059308", &D_80059308, 0x80059308u, 2, 0 },
    { "D_8005930C", &D_8005930C, 0x8005930Cu, 2, 0 },
};
static const Glob g_patch[] = { { "D_800308D0", D_800308D0, 0x800308D0u, 0x90, 0 } };
static const Glob g_light[] = { { "D_80059F64", D_80059F64, 0x80059F64u, 32, 0 }, { "D_80059F84", D_80059F84, 0x80059F84u, 32, 0 } };
static const Glob g_none[] = { { "-", &rng_s, 0x80000000u + 0x1ff800u, 0, 0 } };

/* ---- setups / natives ---- */
static void setup_poly(uint32_t id)
{
    (void)id;
    fill_random_low();
    D_80059424 = lo(O_OUT + 0x20);
    D_8005952C = lo_addr(O_NORM);
    D_8005953C = lo_addr(O_NORM) + (rnd() & 1 ? 0 : 0x400);
    D_80059498 = lo_addr(O_REC + 8 * rn(32));
    D_80059308 = (u16)rnd(); D_8005930C = (u16)rnd();
    for (int i = 0; i < 4; i++) ((s16 *)lo(O_IDX))[i] = (s16)rn(60);
    feed[0] = (rnd() % 5) != 0; /* func_8002CD64: mostly emit */
    A[0] = lo_addr(O_COL); A[1] = lo_addr(O_IDX); A[2] = rn(8);
}
static uint32_t nat_D244(void) { return (uint32_t)func_8002D244(lo(O_COL), (s16 *)lo(O_IDX)); }
static uint32_t nat_D420(void) { return (uint32_t)func_8002D420(lo(O_COL), (s16 *)lo(O_IDX)); }
static uint32_t nat_D814(void) { return (uint32_t)func_8002D814(lo(O_COL), (s16 *)lo(O_IDX), (s32)A[2]); }
static uint32_t nat_DA14(void) { return (uint32_t)func_8002DA14(lo(O_COL), (s16 *)lo(O_IDX)); }

static void setup_db84(uint32_t id)
{
    int deg = (id % 4) == 0;
    fill_random_low();
    rand_vertices(O_VERT, 8, deg);
    if (deg && (id & 8)) { memcpy(lo(O_VERT + 8), lo(O_VERT), 8); }        /* coincident */
    if (id % 7 == 3) { /* collinear */
        s16 *a = (s16 *)lo(O_VERT), *b = (s16 *)lo(O_VERT + 8), *c = (s16 *)lo(O_VERT + 16);
        for (int k = 0; k < 3; k++) { b[k] = (s16)(a[k] + 5 * (k + 1)); c[k] = (s16)(a[k] + 10 * (k + 1)); }
    }
    A[0] = lo_addr(O_VERT); A[1] = lo_addr(O_VERT + 8); A[2] = lo_addr(O_VERT + 16); A[3] = lo_addr(O_OUTN);
}
static uint32_t nat_db84(void) { func_8002DB84(LOWC(O_VERT), LOWC(O_VERT + 8), LOWC(O_VERT + 16), LOWC(O_OUTN)); return 0; }

static int g_norm_ranges; /* func_800302D4: keep |n|^2 sums inside retail's signed-ADD domain */
static void setup_delta(uint32_t id)
{
    unsigned n = rn(8);
    fill_random_low();
    rand_vertices(O_VERT2, 16, 0);
    if (g_norm_ranges) { /* plausible unit-ish normals (<=4096 scale) so sum of squares never overflows */
        for (unsigned i = 0; i < 16; i++) for (int k = 0; k < 3; k++) ((s16 *)lo(O_VERT2 + i * 8))[k] = (s16)rrange(-4096, 4096);
    }
    for (unsigned i = 0; i < n; i++) {
        s16 *d = (s16 *)lo(O_DELTA + i * 8);
        for (int k = 0; k < 4; k++) d[k] = g_norm_ranges ? (s16)rrange(-1500, 1500) : (rnd() & 3) ? (s16)rrange(-5000, 5000) : (s16)rnd_val();
        /* retail layout: +0 dx, +2 dy, +4 dz, +6 vertex index; every slot is
         * a valid index so a wrong slot reads different data, never out of range */
        d[3] = (s16)rn(16);
        d[1] = (s16)rrange(-1500, 1500); /* also the slot a wrong-layout body would use as index: stays inside the window */
    }
    A[0] = lo_addr(O_VERT2); A[1] = lo_addr(O_DELTA); A[2] = n;
    A[3] = g_norm_ranges ? (id % 5 == 0 ? 0 : (uint32_t)rrange(-0x1200, 0x1200)) : (id % 5 == 0) ? 0 : (uint32_t)((rnd() & 1) ? rnd_val() : rrange(-0x1200, 0x1200));
}
static uint32_t nat_3028(void) { func_80030228(lo(O_VERT2), (s16 *)lo(O_DELTA), (s32)A[2], (s32)A[3]); return 0; }
static void setup_norm(uint32_t id) { g_norm_ranges = 1; setup_delta(id); g_norm_ranges = 0; }
static uint32_t nat_302D(void) { func_800302D4(lo(O_VERT2), (s16 *)lo(O_DELTA), (s32)A[2], (s32)A[3]); return 0; }

static void setup_patch(uint32_t id)
{
    (void)id;
    fill_random_low();
    for (unsigned i = 0; i < 0x90; i++) { D_800308D0[i / 2] = (s16)rnd(); }
    A[0] = rnd() & 0xFFFF; A[1] = rnd() & 0xFFFF;
    A[2] = (uint32_t)(int32_t)(s16)rnd(); A[3] = (uint32_t)(int32_t)(s16)rnd();
}
static uint32_t nat_3098(void) { func_80030988((u16)A[0], (u16)A[1], (s16)A[2], (s16)A[3]); return 0; }

static void setup_lightdir(uint32_t id)
{
    fill_random_low();
    for (int k = 0; k < 3; k++) ((s32 *)lo(O_LIGHT))[k] = (rnd() & 3) ? rrange(-3000, 3000) : rnd_val();
    for (int k = 0; k < 3; k++) ((u16 *)lo(O_LIGHT + 0xC))[k] = (u16)rnd();
    for (int i = 0; i < 32; i++) { D_80059F64[i] = (u8)rnd(); D_80059F84[i] = (u8)rnd(); }
    A[0] = (id % 3) | ((id & 4) ? 0x30000u : 0u); A[1] = lo_addr(O_LIGHT);
}
static uint32_t nat_3A30(void) { func_80030A30((s32)A[0], LOWC(O_LIGHT)); return 0; }

static void setup_lightmat(uint32_t id)
{
    (void)id;
    fill_random_low();
    for (int i = 0; i < 32; i++) D_80059F64[i] = (u8)rnd();
    for (int i = 0; i < 9; i++) ((s16 *)lo(O_MAT))[i] = (rnd() & 3) ? (s16)rrange(-4096, 4096) : (s16)rnd_val();
    if (rnd() & 1) for (int i = 0; i < 9; i++) ((s16 *)D_80059F64)[i] = (s16)rrange(-4096, 4096);
    A[0] = lo_addr(O_MAT);
}
static uint32_t nat_3B14(void) { func_80030B14(LOWC(O_MAT)); return 0; }

static const Fn fns[] = {
    { "func_8002D244", 0x8002D244u, 1, setup_poly, nat_D244 },
    { "func_8002D420", 0x8002D420u, 1, setup_poly, nat_D420 },
    { "func_8002D814", 0x8002D814u, 1, setup_poly, nat_D814 },
    { "func_8002DA14", 0x8002DA14u, 1, setup_poly, nat_DA14 },
    { "func_8002DB84", 0x8002DB84u, 0, setup_db84, nat_db84 },
    { "func_80030228", 0x80030228u, 0, setup_delta, nat_3028 },
    { "func_800302D4", 0x800302D4u, 0, setup_norm, nat_302D },
    { "func_80030988", 0x80030988u, 0, setup_patch, nat_3098 },
    { "func_80030A30", 0x80030A30u, 0, setup_lightdir, nat_3A30 },
    { "func_80030B14", 0x80030B14u, 0, setup_lightmat, nat_3B14 },
};

static void select_globs(const char *fn)
{
    if (!strcmp(fn, "func_80030988")) { globs = g_patch; nglobs = 1; }
    else if (!strcmp(fn, "func_80030A30") || !strcmp(fn, "func_80030B14")) { globs = g_light; nglobs = 2; }
    else if (!strncmp(fn, "func_8002D", 10) && strcmp(fn, "func_8002DB84")) { globs = g_poly; nglobs = 6; }
    else { globs = g_none; nglobs = 0; }
}

int main(int argc, char **argv)
{
    const char *only = argc > 1 ? argv[1] : NULL;
    unsigned cases = getenv("XENO_CASES") ? (unsigned)atoi(getenv("XENO_CASES")) : 1500;
    int bad = 0, ran = 0;
    cur_what = "MAIN EXE TEMP2";
    setvbuf(stdout, NULL, _IOLBF, 0);
    map_low();
    load_slus(cur_what);
    user_bridge = bridge;
    /* the sqrt/trig tables live in guest RAM (retail-data loader); the SLUS
     * image already holds them, which is what the host helpers read. */
    for (unsigned k = 0; k < sizeof(fns) / sizeof(fns[0]); k++) {
        const Fn *f = &fns[k];
        unsigned fails = 0, id;
        if (only && strcmp(only, f->name)) continue;
        select_globs(f->name);
        prints_left = 1;
        for (id = 0; id < cases; id++) fails += (unsigned)run_case(f, id + 1, 1);
        printf("MAIN EXE TEMP2 %s %s cases=%u diverged=%u\n", f->name, fails ? "DIVERGES" : "PASS", cases, fails);
        ran++;
        bad += fails ? 1 : 0;
    }
    if (!ran) { fprintf(stderr, "no such function\n"); return 2; }
    printf("MAIN EXE TEMP2 %s (%d function(s) diverge)\n", bad ? "FAIL" : "ALL PASS", bad);
    return bad ? 1 : 0;
}
