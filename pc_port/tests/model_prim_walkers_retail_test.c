/*
 * Table-driven retail differential for every D_8004FE50 model-prim walker.
 *
 * For each distinct retail walker entry dispatched by the model-prim table
 * (read from disc/SLUS_006.64 .sdata at 0x8004FE50), the raw retail MIPS
 * bytes run on the project interpreter (battle_mips_adapter.c / PcPortMipsRun)
 * as the oracle, and the port body actually wired into the live port table
 * D_8004FE50 (pc_port/src/game_overrides.c, temp2.c, model_prim_ed20.c) runs
 * natively on the identical fixture: same guest-visible pointer values, same
 * GTE register file, same seeded packet/OT/records memory.  Compared
 * byte-for-byte afterwards:
 *
 *   - the return value and the walker-owned globals (output cursor
 *     D_80059424, emitted counter D_80059578, records cursor D_80059498),
 *   - the whole packet output window including guard bytes before/after,
 *   - the full ordering table,
 *   - the vertex/command/records input windows (retail is the oracle: if
 *     retail writes them, the port must write the same bytes),
 *   - the complete GTE register file (catches reordered/omitted GTE ops,
 *     e.g. DPCS in a branch-delay slot).
 *
 * Both sides share one PsyCross GTE, so this is NOT a PS1 hardware oracle;
 * it proves the port body's control flow and stores equal the retail bytes'.
 * Slots where retail dispatches a walker but the port table holds NULL are
 * reported as informational SLOT lines (prim_table_retail_prod_test.c pins
 * the row set; this reports the per-variant holes this differential cannot
 * reach through the live table).
 *
 * Style/model: model_prim_f2e0_retail_test.c / model_prim_fae8_retail_test.c
 * (which cover func_8002F2E0 and func_8002FAE8/func_8002F4B4 individually).
 */
#include "common.h"
#include "psyq/libgte.h"
#include "psx/inline_c.h"
#include "../src/battle_mips_runtime.c"
#include "psx/gtereg.h"
#include <sys/mman.h>

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];
int g_GPUDisabledState;
unsigned g_PcPortPresentedFrames;
/* compat.o's real pump is the sole definition; with a frozen vblank count it
 * early-returns before touching pad state (a conflicting no-op stub here
 * breaks the link with multiple-definition). */
int PsyX_Sys_GetVBlankCount(void) { return 0; }
void PsyX_UpdateInput(void) {}
void ControllerPoll(void) {}
void ControllerPushState(void) {}
char PsyX_BeginScene(void) { abort(); }
void ClearSplits(void) { abort(); }
void DrawAllSplits(void) { abort(); }
void ParsePrimitivesLinkedList(u_long *p, int s) { (void)p; (void)s; abort(); }
/* libgpu helpers referenced by temp2.c's row-0x10 buildProc func_8002DAFC
 * (the walkers never call a buildProc; link-only, abort if reached). */
void SetShadeTex(void *p, int tge) { (void)p; (void)tge; abort(); }
unsigned short GetTPage(int tp, int abr, int x, int y)
{ (void)tp; (void)abr; (void)x; (void)y; abort(); }
unsigned short GetClut(int x, int y) { (void)x; (void)y; abort(); }

/* Walker-owned runtime state.  D_800500F8/FC/50100 are DEFINED by the
 * production game_overrides.c TU (initialized renderer bounds), so they are
 * only referenced here; everything else this TU owns. */
u8 *D_80059424;
u32 D_8005953C, D_80059498, D_80059568;
s32 D_80059578;
u8 D_80059598, D_80059599, D_8005959A; /* depth-cue model base RGB */
u32 D_8005952C; /* draw-state shadowed by func_8002C700/E688's callers */
/* Builder-proc state (temp2.c buildProcs referenced from the dispatch
 * table's buildProc slots; auto-stubbed in the port build). */
s32 D_80050108, D_8005010C, D_80059308, D_8005930C, D_80059310, D_80059314;
extern s32 D_80050100;
extern s32 D_800500F8;
extern s32 D_800500FC;

/* Link-only storage for game_overrides.c's cull-cam telemetry externs.
 * Never read at run time here: CullCamOn() is env-gated off, so every
 * diagnostic early-returns before touching them; they exist so the strict
 * link resolves.  No other TU defines them, and sizes are arbitrary. */
u8 g_CameraEye2[64], g_CameraAt2[64], g_CameraEye[64], g_CameraAt[64];
u8 g_CamInterpolation[64], g_FieldActors[64];
u8 g_PlayerActorIndex[64], g_C1ButtonStateReleased[64];
u8 g_Scene[64];

/* The live port dispatch table (pc_port/src/game_overrides.c). */
typedef s32 (*ModelPrimProc)(u8 *pCmd, s32 count);
typedef s32 (*ModelPrimBuildProc)(u32 pSrc, u32 pCmd, s32 shade);
typedef struct ModelPrimDesc {
    ModelPrimProc proc[6];
    ModelPrimBuildProc buildProc;
    u32 cmdStride;
    u32 packetStride;
    u32 outputStride;
} ModelPrimDesc;
extern ModelPrimDesc D_8004FE50[17];

#define VERT        0x00100000u
#define CMD         0x00180000u
#define OUT_MAP     0x00200000u
#define OUT         (OUT_MAP + 0x20u)
#define NORMAL      0x00280000u
#define OT          0x00300000u
#define REGION_SIZE 0x80000u
#define STACK       0x801ff000u
#define OT_ENTRIES  4096
#define OUT_WINDOW  512
#define SRC_WINDOW  128
#define MAXCMDS     6
#define RUN_BUDGET  20000
#define NROWS       17

/* Retail table vaddr; the exe image is loaded to 0x80010000 first. */
#define RETAIL_TABLE 0x8004FE50u

typedef struct WalkerSpec {
    const char *name;   /* retail entry hex (report id) */
    u32 retail;         /* retail entry point vaddr */
    u32 row, variant;   /* live port slot that dispatches this body */
    u32 packetStep;     /* retail packet stride (seed layout) */
    const char *portFn; /* informational: expected body */
} WalkerSpec;

/* Every distinct retail entry reachable through the port table today.
 * (func_8002F2E0/FAE8/F4B4 also have their own dedicated differentials;
 * they are swept here again for free through the live table.) */
static const WalkerSpec walkers[] = {
    {"8002E010", 0x8002E010u, 0x03, 0, 0x28, "ModelPrimTriGT3Variant0"},
    {"8002E024", 0x8002E024u, 0x02, 0, 0x1C, "ModelPrimTriMediumAverageVariant0"},
    {"8002E038", 0x8002E038u, 0x00, 0, 0x14, "ModelPrimTriSmallAverageVariant0"},
    {"8002E04C", 0x8002E04Cu, 0x01, 0, 0x20, "ModelPrimTriAverageVariant0"},
    {"8002E240", 0x8002E240u, 0x0A, 0, 0x24, "ModelPrimQuadG4Variant0"},
    {"8002E254", 0x8002E254u, 0x08, 0, 0x18, "ModelPrimQuadF4Variant0"},
    {"8002E268", 0x8002E268u, 0x09, 0, 0x28, "ModelPrimQuadFT4Variant0"},
    {"8002E22C", 0x8002E22Cu, 0x0B, 0, 0x34, "ModelPrimQuadGT4Variant0"},
    {"8002E64C", 0x8002E64Cu, 0x0B, 2, 0x34, "ModelPrimQuadGT4MaxSZVariant2"},
    {"8002E660", 0x8002E660u, 0x0A, 2, 0x24, "ModelPrimQuadG4MaxSZVariant2"},
    {"8002EAB8", 0x8002EAB8u, 0x0B, 3, 0x34, "ModelPrimQuadGT4MinSZVariant3"},
    {"8002EACC", 0x8002EACCu, 0x0A, 3, 0x24, "ModelPrimQuadG4MinSZVariant3"},
    {"8002EAE0", 0x8002EAE0u, 0x08, 3, 0x18, "ModelPrimQuadF4MinSZVariant3"},
    {"8002EAF4", 0x8002EAF4u, 0x09, 3, 0x28, "ModelPrimQuadFT4MinSZVariant3"},
    {"8002F8D0", 0x8002F8D0u, 0x08, 1, 0x18, "ModelPrimQuadF4LitVariant1"},
    {"8002F6B4", 0x8002F6B4u, 0x02, 1, 0x1C, "ModelPrimTriMediumLitVariant1"},
    {"8002E470", 0x8002E470u, 0x00, 2, 0x14, "ModelPrimTriSmallMaxSZVariant2"},
    {"8002E484", 0x8002E484u, 0x05, 2, 0x20, "ModelPrimTriMaxSZVariant2"},
    {"8002E448", 0x8002E448u, 0x03, 2, 0x28, "ModelPrimTriGT3MaxSZVariant2"},
    {"8002E45C", 0x8002E45Cu, 0x02, 2, 0x1C, "ModelPrimTriMediumMaxSZVariant2"},
    {"8002E674", 0x8002E674u, 0x0C, 2, 0x18, "ModelPrimQuadF4MaxSZVariant2"},
    {"8002E688", 0x8002E688u, 0x09, 2, 0x28, "func_8002E688(temp2)"},
    {"8002ED20", 0x8002ED20u, 0x00, 1, 0x14, "func_8002ED20(ed20)"},
    {"8002EF0C", 0x8002EF0Cu, 0x01, 4, 0x20, "ModelPrimTriDepthCueVariant4"},
    {"8002F0E4", 0x8002F0E4u, 0x01, 5, 0x20, "ModelPrimTriDepthCueMaxSZVariant5"},
    {"8002FCFC", 0x8002FCFCu, 0x09, 4, 0x28, "ModelPrimQuadFT4DepthCueVariant4"},
    {"8002FF0C", 0x8002FF0Cu, 0x09, 5, 0x28, "ModelPrimQuadFT4DepthCueMaxSZVariant5"},
    {"8002E8B4", 0x8002E8B4u, 0x03, 3, 0x28, "ModelPrimTriGT3MinSZVariant3"},
    {"8002E8C8", 0x8002E8C8u, 0x02, 3, 0x1C, "ModelPrimTriMediumMinSZVariant3"},
    {"8002E8DC", 0x8002E8DCu, 0x00, 3, 0x14, "ModelPrimTriSmallMinSZVariant3"},
    {"8002E8F0", 0x8002E8F0u, 0x01, 3, 0x20, "ModelPrimTriMinSZVariant3"},
    {"80030750", 0x80030750u, 0x10, 0, 0x20, "func_80030750(temp2)"},
    {"8002F2E0", 0x8002F2E0u, 0x01, 1, 0x20, "func_8002F2E0(temp2)"},
    {"8002F4B4", 0x8002F4B4u, 0x03, 1, 0x20, "func_8002F4B4(temp2)"},
    {"8002FAE8", 0x8002FAE8u, 0x09, 1, 0x28, "func_8002FAE8(temp2)"},
};

typedef struct Case {
    const char *name;
    s32 count;
    s16 xyz[6][3];        /* vertex slots 1..6 */
    u16 idx[MAXCMDS][4];  /* per command v0,v1,v2(,v3 quad) */
    u32 color;            /* packet colour/code word seed at out+4 */
    u32 recColor;         /* ED20 lighting record: colour word */
    s16 recNormal[3];     /* ED20 lighting record: face normal */
    u32 rgbc;             /* depth-cue base colour (D_80059598..) */
    s32 shift;
    u32 xmax;
    u32 ymaxPacked;
    s32 zsf3;
    int expectEmit;       /* sanity: retail must link the OT here */
    int expectNoEmit;     /* sanity: retail must NOT link the OT here */
} Case;

/* Case values follow model_prim_f2e0_retail_test.c: identity rotation,
 * OFX/OFY 160/112, ZSF3 1365 (0 in the zero-depth case).  v1 indices stay
 * even so (cmd>>13)&0xfff8 (ED20) and (cmd>>16)<<3 agree except in the
 * packed case, which deliberately separates them. */
static Case case_list[] = {
    {"count0", 0,
     {{-30, -20, 800}, {30, -20, 800}, {0, 30, 800}, {20, 25, 900}, {8, 4, 950}, {0, 0, 0}},
     {{1, 2, 3, 4}}, 0x20d04020u, 0x30c06030u, {1024, 0, 3072}, 0x20c06030u,
     2, 320, 224u << 16, 1365, 0, 0},
    {"emit-front", 1,
     {{-30, -20, 800}, {30, -20, 800}, {0, 30, 800}, {20, 25, 900}, {8, 4, 950}, {0, 0, 0}},
     {{1, 2, 3, 4}}, 0x20c06030u, 0x30c06030u, {1024, 0, 3072}, 0x20c06030u,
     2, 320, 224u << 16, 1365, 1, 0},
    {"backface", 1,
     {{-30, -20, 800}, {0, 30, 800}, {30, -20, 800}, {20, 25, 900}, {8, 4, 950}, {0, 0, 0}},
     {{1, 2, 3, 4}}, 0x2080a040u, 0x30c06030u, {1024, 0, 3072}, 0x20c06030u,
     2, 320, 224u << 16, 1365, 0, 1},
    {"zero-depth", 1,
     {{-30, -20, 800}, {30, -20, 800}, {0, 30, 800}, {20, 25, 900}, {8, 4, 950}, {0, 0, 0}},
     {{1, 2, 3, 4}}, 0x20f02010u, 0x30c06030u, {1024, 0, 3072}, 0x20c06030u,
     2, 320, 224u << 16, 0, 0, 0},
    {"screen-reject", 1,
     {{-640, -20, 800}, {640, -20, 800}, {0, 30, 800}, {20, 25, 900}, {8, 4, 950}, {0, 0, 0}},
     {{1, 2, 3, 4}}, 0x202060a0u, 0x30c06030u, {1024, 0, 3072}, 0x20c06030u,
     1, 160, 112u << 16, 1365, 0, 1},
    {"mixed-depths", 4,
     {{-30, -20, 700}, {30, -20, 900}, {0, 30, 1100}, {-30, 20, 700}, {30, 20, 900}, {0, 34, 1100}},
     {{1, 2, 3, 4}, {1, 2, 4, 5}, {1, 3, 4, 5}, {2, 3, 5, 4}, {1, 2, 3, 4}, {1, 2, 3, 4}},
     0x20e09020u, 0x30c06030u, {1024, 0, 3072}, 0x20c06030u,
     3, 320, 224u << 16, 1365, 1, 0},
    {"packed-index-v1", 2,
     {{-40, -25, 900}, {0, 35, 900}, {40, -25, 900}, {20, 25, 950}, {8, 4, 950}, {0, 0, 0}},
     {{3, 0xe002, 2, 4}, {1, 2, 3, 4}, {1, 2, 3, 4}, {1, 2, 3, 4}, {1, 2, 3, 4}, {1, 2, 3, 4}},
     0x2040b070u, 0x30c06030u, {1024, 0, 3072}, 0x20c06030u,
     3, 320, 224u << 16, 1365, 0, 0},
};

static void map_native(uintptr_t address)
{
    void *p = mmap((void *)address, REGION_SIZE, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
    if (p != (void *)address) {
        perror("mmap fixed low native fixture");
        exit(1);
    }
    assert(address < 0x01000000u && address + REGION_SIZE <= 0x01000000u);
}

static const u32 bases[] = {VERT, CMD, OUT_MAP, NORMAL, OT};
static int fixture_address(u32 address, unsigned width)
{
    for (unsigned i = 0; i < sizeof(bases) / sizeof(bases[0]); i++)
        if (address >= bases[i] && (uint64_t)address + width <= bases[i] + REGION_SIZE)
            return 1;
    return 0;
}
static int fixture_read(void *opaque, u32 address, unsigned width, u32 *value)
{
    BattleMipsRuntime *runtime = opaque;
    if (!fixture_address(address, width)) return runtime_read(runtime, address, width, value);
    *value = load_le((u8 *)(uintptr_t)address, width);
    return 0;
}
static int fixture_write(void *opaque, u32 address, unsigned width, u32 value)
{
    BattleMipsRuntime *runtime = opaque;
    if (!fixture_address(address, width)) return runtime_write(runtime, address, width, value);
    store_le((u8 *)(uintptr_t)address, width, value);
    return 0;
}

static void put_vertex(u8 *base, unsigned index, const s16 xyz[3])
{
    memcpy(base + index * 8 + 0, &xyz[0], 2);
    memcpy(base + index * 8 + 2, &xyz[1], 2);
    memcpy(base + index * 8 + 4, &xyz[2], 2);
    store_le(base + index * 8 + 6, 2, 0x5aa5);
}

static void prepare_regions(const Case *c, u32 packetStep)
{
    /* Vertex and record regions get a POSITION-DEPENDENT fill: walkers that
     * decode a vertex index differently (retail (cmd>>13)&0xFFF8 vs a native
     * (cmd>>16)<<3) read different slots, and only distinct bytes per slot
     * make that visible in the compared output. */
    for (unsigned i = 0; i < REGION_SIZE; i++) {
        ((u8 *)VERT)[i] = (u8)(0x39 + i * 7);
        ((u8 *)NORMAL)[i] = (u8)(0x6c + i * 5);
    }
    memset((void *)CMD, 0x5a, REGION_SIZE);
    memset((void *)OUT_MAP, 0xa5, REGION_SIZE);
    memset((void *)OT, 0, REGION_SIZE);
    for (unsigned i = 0; i < 6; i++) put_vertex((u8 *)VERT, i + 1, c->xyz[i]);
    for (unsigned i = 0; i <= (unsigned)c->count && i < MAXCMDS; i++) {
        store_le((u8 *)CMD + i * 8, 4, (u32)c->idx[i][0] | ((u32)c->idx[i][1] << 16));
        store_le((u8 *)CMD + i * 8 + 4, 2, c->idx[i][2]);
        store_le((u8 *)CMD + i * 8 + 6, 2, c->idx[i][3]);
        /* Lighting record i (12 bytes): colour, normal lo, normal hi. */
        store_le((u8 *)NORMAL + i * 12, 4, c->recColor ^ (i * 0x0003070bu));
        store_le((u8 *)NORMAL + i * 12 + 4, 4,
                 (u32)(u16)(c->recNormal[0] + i * 97) |
                     ((u32)(u16)(c->recNormal[1] - i * 53) << 16));
        store_le((u8 *)NORMAL + i * 12 + 8, 4,
                 (u32)(u16)(c->recNormal[2] + i * 31) | ((0x7e00u + i) << 16));
        /* Packet i: colour/code word at +4 (code byte = top byte). */
        store_le((u8 *)OUT + i * packetStep + 4, 4, c->color ^ (i * 0x0003070bu));
    }
    for (unsigned i = 0; i < OT_ENTRIES; i++)
        store_le((u8 *)OT + i * 4, 4, 0x00c00000u + i * 4);
}

static void reset_gte(const Case *c)
{
    memset(&gteRegs, 0, sizeof(gteRegs));
    CTC2(4096, 0); CTC2(4096, 2); CTC2(4096, 4);
    CTC2(1024, 7);
    CTC2(4096, 8); CTC2(4096, 10); CTC2(4096, 12);
    CTC2(4096, 16); CTC2(4096, 18); CTC2(4096, 20);
    CTC2(0x00302010, 21); CTC2(0x00403020, 22); CTC2(0x00504030, 23);
    CTC2(160u << 16, 24); CTC2(112u << 16, 25); CTC2(512, 26);
    CTC2(0x100, 27); CTC2(0x200000, 28); CTC2((u32)c->zsf3, 29);
    /* Zero-depth fixtures zero BOTH ZSF3 and DQB (model_prim_fae8_retail_
     * test.c fixture 3): with DQB still 1024 some depth-cue walkers emit. */
    CTC2(c->zsf3 ? 1024 : 0, 30);
}

static void set_guest_globals(const Case *c)
{
    store_le(PSX_ADDR(0x8005953cu), 4, VERT);
    store_le(PSX_ADDR(0x80059424u), 4, OUT);
    store_le(PSX_ADDR(0x80059568u), 4, OT);
    store_le(PSX_ADDR(0x80059578u), 4, 7);
    store_le(PSX_ADDR(0x80059498u), 4, NORMAL);
    store_le(PSX_ADDR(0x80050100u), 4, (u32)c->shift);
    store_le(PSX_ADDR(0x800500f8u), 4, c->xmax);
    store_le(PSX_ADDR(0x800500fcu), 4, c->ymaxPacked);
    /* F2E0-family lit walkers (F4B4/FAE8) read vertex-indexed normals
     * through D_8005952C, not the records cursor. */
    store_le(PSX_ADDR(0x8005952cu), 4, NORMAL);
    ((u8 *)PSX_ADDR(0x80059598u))[0] = (u8)c->rgbc;
    ((u8 *)PSX_ADDR(0x80059598u))[1] = (u8)(c->rgbc >> 8);
    ((u8 *)PSX_ADDR(0x80059598u))[2] = (u8)(c->rgbc >> 16);
}

static void set_host_globals(const Case *c)
{
    D_8005953C = VERT;
    D_80059424 = (u8 *)OUT;
    D_80059568 = OT;
    D_80059578 = 7;
    D_80059498 = NORMAL;
    D_80050100 = c->shift;
    D_800500F8 = (s32)c->xmax;
    D_800500FC = (s32)c->ymaxPacked;
    D_8005952C = NORMAL;
    D_80059598 = (u8)c->rgbc;
    D_80059599 = (u8)(c->rgbc >> 8);
    D_8005959A = (u8)(c->rgbc >> 16);
}

static int ot_is_initial(const u8 *ot)
{
    for (unsigned i = 0; i < OT_ENTRIES; i++)
        if (load_le(ot + i * 4, 4) != 0x00c00000u + i * 4) return 0;
    return 1;
}

static int first_diff(const u8 *actual, const u8 *expected, size_t size, size_t *at)
{
    for (size_t i = 0; i < size; i++)
        if (actual[i] != expected[i]) { *at = i; return 1; }
    return 0;
}

static void fail_bytes(const WalkerSpec *w, const Case *c, const char *what,
                       size_t at, const u8 *nativeBase, const u8 *retailBase)
{
    fprintf(stderr, "MODEL PRIM WALKERS FAIL %s %s %s byte=%zu native=%02x retail=%02x\n",
            w->name, c->name, what, at, nativeBase[at], retailBase[at]);
}

static int run_case(const WalkerSpec *w, const Case *c, ModelPrimProc port)
{
    PcPortMipsCpu cpu;
    BattleMipsRuntime runtime = {0};
    u8 expectedOut[OUT_WINDOW], expectedVert[SRC_WINDOW];
    u8 expectedCmd[SRC_WINDOW], expectedRec[SRC_WINDOW];
    static u8 expectedOt[REGION_SIZE];
    u32 expectedCursor, expectedRecords, expectedRet;
    s32 expectedEmitted;
    GTERegisters expectedGte;
    size_t at;

    prepare_regions(c, w->packetStep);
    set_guest_globals(c);
    memset(PSX_ADDR(STACK - 64), 0xd3, 128);
    reset_gte(c);
    initialize_cpu(&cpu, &runtime);
    cpu.bus.read = fixture_read;
    cpu.bus.write = fixture_write;
    cpu.bus.bridge = NULL;
    cpu.gpr[4] = CMD;
    cpu.gpr[5] = (u32)c->count;
    cpu.gpr[29] = STACK;
    for (unsigned i = 16; i <= 23; i++) cpu.gpr[i] = 0x6f000000u + i;
    if (PcPortMipsRun(&cpu, w->retail, BATTLE_HALT_PC, RUN_BUDGET) != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "MODEL PRIM WALKERS FAIL %s %s retail-run: %s\n",
                w->name, c->name, cpu.error);
        return 0;
    }
    /* The shared .L8002E1F4 epilogue restores $s7 with an lw in the jr $ra
     * delay slot.  The interpreter halts at the return address before that
     * load-delayed value is committed; the caller's first instruction would
     * see it, so commit it here (first visible with F6B4, which uses $s7). */
    if (cpu.load_valid && cpu.load_reg != 0)
        cpu.gpr[cpu.load_reg] =
            (cpu.gpr[cpu.load_reg] & cpu.load_mask) | cpu.load_value;
    if (cpu.gpr[29] != STACK) {
        fprintf(stderr, "MODEL PRIM WALKERS FAIL %s %s retail stack ptr %08x\n",
                w->name, c->name, cpu.gpr[29]);
        return 0;
    }
    for (unsigned i = 16; i <= 23; i++)
        if (cpu.gpr[i] != 0x6f000000u + i) {
            fprintf(stderr, "MODEL PRIM WALKERS FAIL %s %s retail s-reg %u clobbered\n",
                    w->name, c->name, i);
            return 0;
        }
    expectedRet = cpu.gpr[2];
    expectedCursor = load_le(PSX_ADDR(0x80059424u), 4);
    expectedRecords = load_le(PSX_ADDR(0x80059498u), 4);
    expectedEmitted = (s32)load_le(PSX_ADDR(0x80059578u), 4);
    memcpy(expectedOut, (void *)(OUT - 32), OUT_WINDOW);
    memcpy(expectedVert, (void *)VERT, SRC_WINDOW);
    memcpy(expectedCmd, (void *)CMD, SRC_WINDOW);
    memcpy(expectedRec, (void *)NORMAL, SRC_WINDOW);
    memcpy(expectedOt, (void *)OT, REGION_SIZE);
    expectedGte = gteRegs;

    /* Fixture sanity against the oracle itself: the emit case must link the
     * OT, the reject cases must not.  If these flip, the fixtures (not the
     * port) are wrong for this walker. */
    if (c->expectEmit && ot_is_initial(expectedOt)) {
        fprintf(stderr, "MODEL PRIM WALKERS FAIL %s %s fixture never emits\n",
                w->name, c->name);
        return 0;
    }
    if (c->expectNoEmit && !ot_is_initial(expectedOt)) {
        fprintf(stderr, "MODEL PRIM WALKERS FAIL %s %s fixture emits on reject case\n",
                w->name, c->name);
        return 0;
    }

    prepare_regions(c, w->packetStep);
    set_host_globals(c);
    reset_gte(c);
    u32 actualRet = (u32)port((u8 *)CMD, c->count);

    /* Substantive comparisons first (packets/OT/inputs/GTE), the cosmetic
     * return register last, so one divergence does not mask deeper ones. */
    if (first_diff((u8 *)OUT - 32, expectedOut, OUT_WINDOW, &at)) {
        fail_bytes(w, c, "packet/guards", at, (u8 *)OUT - 32, expectedOut);
        return 0;
    }
    if (first_diff((u8 *)OT, expectedOt, REGION_SIZE, &at)) {
        fprintf(stderr, "MODEL PRIM WALKERS FAIL %s %s ot byte=%zu word=%08x retail=%08x\n",
                w->name, c->name, at, load_le((u8 *)OT + (at & ~3u), 4),
                load_le(expectedOt + (at & ~3u), 4));
        return 0;
    }
    if (first_diff((u8 *)VERT, expectedVert, SRC_WINDOW, &at)) {
        fail_bytes(w, c, "vertices", at, (u8 *)VERT, expectedVert);
        return 0;
    }
    if (first_diff((u8 *)CMD, expectedCmd, SRC_WINDOW, &at)) {
        fail_bytes(w, c, "commands", at, (u8 *)CMD, expectedCmd);
        return 0;
    }
    if (first_diff((u8 *)NORMAL, expectedRec, SRC_WINDOW, &at)) {
        fail_bytes(w, c, "records", at, (u8 *)NORMAL, expectedRec);
        return 0;
    }
    if (memcmp(&gteRegs, &expectedGte, sizeof(gteRegs))) {
        fprintf(stderr, "MODEL PRIM WALKERS FAIL %s %s gte-registers DIFFER\n",
                w->name, c->name);
        return 0;
    }
    if ((u32)(uintptr_t)D_80059424 != expectedCursor) {
        fprintf(stderr, "MODEL PRIM WALKERS FAIL %s %s cursor native=%08x retail=%08x\n",
                w->name, c->name, (u32)(uintptr_t)D_80059424, expectedCursor);
        return 0;
    }
    if (D_80059498 != expectedRecords) {
        fprintf(stderr, "MODEL PRIM WALKERS FAIL %s %s records native=%08x retail=%08x\n",
                w->name, c->name, D_80059498, expectedRecords);
        return 0;
    }
    if (D_80059578 != expectedEmitted) {
        fprintf(stderr, "MODEL PRIM WALKERS FAIL %s %s emitted native=%d retail=%d\n",
                w->name, c->name, D_80059578, expectedEmitted);
        return 0;
    }
    if (actualRet != expectedRet) {
        fprintf(stderr, "MODEL PRIM WALKERS FAIL %s %s ret native=%08x retail=%08x\n",
                w->name, c->name, actualRet, expectedRet);
        return 0;
    }
    return 1;
}

int main(void)
{
    FILE *disc = fopen("disc/SLUS_006.64", "rb");
    u32 retailTable[NROWS][6];
    unsigned passCount = 0, failCount = 0, nullSlots = 0;

    assert(disc && fseek(disc, 0x800, SEEK_SET) == 0);
    assert(fread(PSX_ADDR(0x80010000u), 1, 0x476e4, disc) == 0x476e4);
    assert(fclose(disc) == 0);
    /* Rows are 0x28 bytes: six procs, buildProc, then the three strides. */
    for (unsigned row = 0; row < NROWS; row++)
        for (unsigned var = 0; var < 6; var++)
            retailTable[row][var] =
                load_le(PSX_ADDR(RETAIL_TABLE + row * 0x28u + var * 4u), 4);

    map_native(VERT); map_native(CMD); map_native(OUT_MAP);
    map_native(NORMAL); map_native(OT);

    /* Where retail dispatches a walker the port table holds NULL, say so:
     * informational SLOT lines (prim_table_retail_prod_test.c pins the row
     * set; this reports the per-variant holes this differential cannot run). */
    for (unsigned row = 0; row < NROWS; row++)
        for (unsigned var = 0; var < 6; var++)
            if (retailTable[row][var] != 0 && D_8004FE50[row].proc[var] == NULL) {
                printf("MODEL PRIM WALKERS SLOT row=%02X var=%u retail=%08X PORT-NULL\n",
                       row, var, retailTable[row][var]);
                nullSlots++;
            }

    for (unsigned w = 0; w < sizeof(walkers) / sizeof(walkers[0]); w++) {
        const WalkerSpec *spec = &walkers[w];
        ModelPrimProc port = D_8004FE50[spec->row].proc[spec->variant];
        if (port == NULL) {
            printf("MODEL PRIM WALKERS UNTESTABLE %s port-slot row=%02X var=%u NULL (%s)\n",
                   spec->name, spec->row, spec->variant, spec->portFn);
            failCount++;
            continue;
        }
        if (retailTable[spec->row][spec->variant] != spec->retail) {
            printf("MODEL PRIM WALKERS FAIL %s slot row=%02X var=%u holds retail %08X "
                   "(spec mismatch)\n",
                   spec->name, spec->row, spec->variant, retailTable[spec->row][spec->variant]);
            failCount++;
            continue;
        }
        unsigned failedCases = 0;
        for (unsigned i = 0; i < sizeof(case_list) / sizeof(case_list[0]); i++)
            if (!run_case(spec, &case_list[i], port)) failedCases++;
        if (failedCases == 0) {
            printf("MODEL PRIM WALKERS PASS %s cases=%zu %s "
                   "packet/guards/OT/globals/cursors/GTE/return\n",
                   spec->name, sizeof(case_list) / sizeof(case_list[0]), spec->portFn);
            passCount++;
        } else {
            printf("MODEL PRIM WALKERS DIVERGES %s failed-cases=%u/%zu (%s)\n",
                   spec->name, failedCases,
                   sizeof(case_list) / sizeof(case_list[0]), spec->portFn);
            failCount++;
        }
    }

    printf("MODEL PRIM WALKERS RESULT pass=%u fail=%u retail-slots-without-port-body=%u\n",
           passCount, failCount, nullSlots);
    return failCount ? 1 : 0;
}
