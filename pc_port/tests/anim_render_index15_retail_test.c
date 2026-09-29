/* Animation-render callback index 15 (func_800257F0) differential.
 *
 * The retail main-exe slice 800257F0..80025A88 is executed on the MIPS
 * adapter with every external callee bridged to the same host PsyCross GTE
 * functions the native body calls.  func_800B1F6C is bridged to an ordered
 * observer on both sides, so this test isolates the callback seam: packed
 * pointer translation, per-context buffer selection, geom-offset save/restore,
 * D_80050100 save/restore, flag handling and the forwarded argument table.
 *
 * Native and retail must leave identical visible GTE state, identical PSX RAM
 * fixture bytes, identical D_8004FD80 matrix words and identical observed
 * func_800B1F6C argument tuples. */
#include <assert.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "battle_mips_adapter.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "psyq/libgpu.h"
#include "psx/gtereg.h"

typedef uint8_t u8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[1024];

extern unsigned MFC2(int);
extern unsigned CFC2(int);
extern void MTC2(unsigned, int);
extern void CTC2(unsigned, int);

/* Native globals the callback reads or writes. */
s32 D_80050100 = 2;
s32 g_GfxCurContext = 0;
u_long* g_GfxCurOT;
MATRIX D_8004FBB8;

/* Host mirror of a "native low" pointer domain for forwarded fields. */

/* -- Observable callees -------------------------------------------------- */

static unsigned call_80022038;
static unsigned call_800B1F6C;
static uint32_t f6c_args[16][6];

void func_80022038(void* p)
{
    (void)p;
    ++call_80022038;
}

/* Native observer stand-in for the renderer seam.  The extracted callback calls
 * this; the oracle bridge records the same tuple. */
void func_800B1F6C(void* a, void* b, void* c, s32 d, s32 e, s32 f)
{
    assert(call_800B1F6C < 16);
    f6c_args[call_800B1F6C][0] = (uint32_t)(uintptr_t)a;
    f6c_args[call_800B1F6C][1] = (uint32_t)(uintptr_t)b;
    f6c_args[call_800B1F6C][2] = (uint32_t)(uintptr_t)c;
    f6c_args[call_800B1F6C][3] = (uint32_t)d;
    f6c_args[call_800B1F6C][4] = (uint32_t)e;
    f6c_args[call_800B1F6C][5] = (uint32_t)f;
    ++call_800B1F6C;
}

/* Production psyq_compat.c contract, reproduced so the differential does not
 * need the whole port TU. */
void ReadGeomOffset(long* ofx, long* ofy)
{
    *ofx = C2_OFX >> 16;
    *ofy = C2_OFY >> 16;
}

/* -- Fixture ------------------------------------------------------------- */

#define TASK    0x80100000u
#define SPRITE  0x80100100u
#define PDATA   0x80100200u
#define VERTS   0x80100400u
#define OTADDR  0x80190000u
#define STACK   0x801ff000u
#define HALT    0xfffffffcu

#define FIXTURE_BASE TASK
#define FIXTURE_SIZE 0x800u
#define OT_RANGE_START 0x80180000u
#define OT_RANGE_SIZE  0x800u

static uint8_t* gaddr(uint32_t a)
{
    return (uint8_t*)PSX_ADDR(a & 0x1FFFFFu);
}

static void w16v(uint32_t a, uint16_t v) { memcpy(gaddr(a), &v, 2); }
static void w32v(uint32_t a, uint32_t v) { memcpy(gaddr(a), &v, 4); }
static uint32_t r32v(uint32_t a) { uint32_t v; memcpy(&v, gaddr(a), 4); return v; }

/* Bus translation: native host pointers pass through; KSEG0/KSEG1 map into
 * g_PsxRam; anything else is a fixture error. */
static uint8_t* bus_ptr(uint32_t v)
{
    uintptr_t base = (uintptr_t)g_PsxRam;
    if (v >= base && v < base + PSX_RAM_SIZE)
        return (uint8_t*)(uintptr_t)v;
    if ((v & 0xFFE00000u) == 0x80000000u || (v & 0xFFE00000u) == 0xA0000000u)
        return (uint8_t*)PSX_ADDR(v);
    assert(!"bridge pointer outside guest RAM");
    return NULL;
}

static int bus_read(void* o, uint32_t a, unsigned n, uint32_t* v)
{
    (void)o;
    uint8_t* p = bus_ptr(a);
    if (p == NULL)
        return -1;
    *v = 0;
    for (unsigned i = 0; i < n; ++i)
        *v |= (uint32_t)p[i] << (i * 8);
    return 0;
}

static int bus_write(void* o, uint32_t a, unsigned n, uint32_t v)
{
    (void)o;
    uint8_t* p = bus_ptr(a);
    if (p == NULL)
        return -1;
    for (unsigned i = 0; i < n; ++i)
        p[i] = (uint8_t)(v >> (i * 8));
    return 0;
}

/* -- Retail library bridge ----------------------------------------------- */

static int bridge(void* o, PcPortMipsCpu* cpu, uint32_t target)
{
    (void)o;
    switch (target) {
    case 0x80022038u:
        ++call_80022038;
        return 1;
    case 0x800B1F6Cu: {
        uint32_t sp = cpu->gpr[29];
        uint32_t a4, a5;
        assert(call_800B1F6C < 16);
        memcpy(&a4, bus_ptr(sp + 0x10), 4);
        memcpy(&a5, bus_ptr(sp + 0x14), 4);
        f6c_args[call_800B1F6C][0] = cpu->gpr[4];
        f6c_args[call_800B1F6C][1] = cpu->gpr[5];
        f6c_args[call_800B1F6C][2] = cpu->gpr[6];
        f6c_args[call_800B1F6C][3] = cpu->gpr[7];
        f6c_args[call_800B1F6C][4] = a4;
        f6c_args[call_800B1F6C][5] = a5;
        ++call_800B1F6C;
        return 1;
    }
    case 0x8003F738u:
        RotMatrix((SVECTOR*)bus_ptr(cpu->gpr[4]), (MATRIX*)bus_ptr(cpu->gpr[5]));
        return 1;
    case 0x8004920Cu:
        MulMatrix0((MATRIX*)bus_ptr(cpu->gpr[4]), (MATRIX*)bus_ptr(cpu->gpr[5]),
                   (MATRIX*)bus_ptr(cpu->gpr[6]));
        return 1;
    case 0x8004931Cu:
        CompMatrix((MATRIX*)bus_ptr(cpu->gpr[4]), (MATRIX*)bus_ptr(cpu->gpr[5]),
                   (MATRIX*)bus_ptr(cpu->gpr[6]));
        return 1;
    case 0x80049CECu:
        ApplyMatrix((MATRIX*)bus_ptr(cpu->gpr[4]), (SVECTOR*)bus_ptr(cpu->gpr[5]),
                    (VECTOR*)bus_ptr(cpu->gpr[6]));
        return 1;
    case 0x8004960Cu:
        PushMatrix();
        return 1;
    case 0x800496ACu:
        PopMatrix();
        return 1;
    case 0x80049D9Cu:
        TransMatrix((MATRIX*)bus_ptr(cpu->gpr[4]), (VECTOR*)bus_ptr(cpu->gpr[5]));
        return 1;
    case 0x80049EFCu:
        SetRotMatrix((MATRIX*)bus_ptr(cpu->gpr[4]));
        return 1;
    case 0x80049F2Cu:
        SetLightMatrix((MATRIX*)bus_ptr(cpu->gpr[4]));
        return 1;
    case 0x80049F5Cu:
        SetColorMatrix((MATRIX*)bus_ptr(cpu->gpr[4]));
        return 1;
    case 0x80049F8Cu:
        SetTransMatrix((MATRIX*)bus_ptr(cpu->gpr[4]));
        return 1;
    case 0x8004A0BCu: {
        long x = 0, y = 0;
        uint32_t a0 = cpu->gpr[4], a1 = cpu->gpr[5];
        ReadGeomOffset(&x, &y);
        memcpy(bus_ptr(a0), &x, 4);
        memcpy(bus_ptr(a1), &y, 4);
        return 1;
    }
    case 0x8004A0ECu:
        SetBackColor((int)cpu->gpr[4], (int)cpu->gpr[5], (int)cpu->gpr[6]);
        return 1;
    case 0x8004A12Cu:
        SetGeomOffset((int)cpu->gpr[4], (int)cpu->gpr[5]);
        return 1;
    default:
        return 0;
    }
}

/* -- GTE / fixture reset ------------------------------------------------- */

static void gte_reset(void)
{
    memset(&gteRegs, 0, sizeof(gteRegs));
    /* Deliberately different from the (0xA0,0x70) geom override so a missing
     * ReadGeomOffset/SetGeomOffset restore is observable. */
    CTC2(200u << 16, 24);
    CTC2(90u << 16, 25);
    CTC2(512, 26);
    CTC2(0xfffffff0u, 27);
    CTC2(0x100000, 28);
    CTC2(0x155, 29);
    CTC2(0x100, 30);
}

static uint32_t domain_addr(uint32_t guest, unsigned domain)
{
    switch (domain) {
    case 0: return guest;
    case 1: return 0xA0000000u | (guest & 0x1FFFFFu);
    /* Native pass-through: a real host pointer inside g_PsxRam.  It is not a
     * KSEG alias, so BattleAnimRenderAddress must leave it alone. */
    case 2: return (uint32_t)(uintptr_t)PSX_ADDR(guest);
    default: return guest;
    }
}

typedef struct {
    const char* name;
    uint32_t gate;      /* pSprite+0x40 */
    uint32_t flags3C;   /* pSprite+0x3C */
    uint8_t flags3F;    /* pSprite+0x3F */
    uint16_t depth30;   /* pSprite+0x30 */
    unsigned sprite_dom;/* domain of the pSprite pointer stored at pArg+4 */
    unsigned hdr_dom;   /* domain of the geometry header at pData+0x34 */
    unsigned buf_dom;   /* domain of the per-context buffer */
    unsigned ctx;       /* g_GfxCurContext */
    int32_t shift;      /* D_80050100 */
    uint16_t color_words[3];
    unsigned sub_dom;   /* domain of the pSub pointer at pSprite+0x20 */
} Case;

static const Case* active;

static void fill_matrix(uint8_t* p, unsigned variant)
{
    for (unsigned i = 0; i < 32; ++i)
        p[i] = (uint8_t)(i * 7u + variant * 31u + 1u);
    p[0] = 0x00; p[1] = 0x10; /* 0x1000 fixed point diagonal-ish */
    p[10] = 0x00; p[11] = 0x10;
    p[20] = 0x00; p[21] = 0x10;
}

static void setup(const Case* t, int native)
{
    active = t;
    memset(g_PsxRam, 0xc7, sizeof(g_PsxRam));
    memset(&D_8004FBB8, 0, sizeof(D_8004FBB8));
    fill_matrix((uint8_t*)&D_8004FBB8, 3);
    /* Guest camera copy so the oracle's CompMatrix(D_8004FBB8) sees the same
     * bytes as the native host symbol. */
    memcpy(gaddr(0x8004FBB8), &D_8004FBB8, 32);

    /* Light matrix .sdata source (retail bytes, non-zero). */
    fill_matrix(gaddr(0x8004FDA0), 5);
    /* Colour matrix .sdata source. */
    memset(gaddr(0x8004FD80), 0x00, 32);
    w16v(0x8004FD8C, 0x0C00);
    w16v(0x8004FD98, 0x0C00);

    /* Work-list task -> sprite pointer. */
    memset(gaddr(FIXTURE_BASE), 0, FIXTURE_SIZE);
    w32v(TASK + 4, domain_addr(SPRITE, t->sprite_dom));
    w32v(SPRITE + 0x20, domain_addr(PDATA, t->sub_dom));
    w16v(SPRITE + 0x02, 0x1234);
    w16v(SPRITE + 0x06, 0xFFF0);
    w16v(SPRITE + 0x0A, 0x0033);
    w32v(SPRITE + 0x3C, t->flags3C);
    /* +0x3F is the top byte of the +0x3C word: retail 800257F0 reads it with
     * lbu for the raw-matrix bit, so flags3F is encoded in flags3C bits 24+. */
    w16v(SPRITE + 0x30, t->depth30);
    w32v(SPRITE + 0x40, t->gate);

    /* pData fields. */
    fill_matrix(gaddr(PDATA + 0x0C), 9);
    w32v(PDATA + 0x34, t->hdr_dom == 3 ? 0 : domain_addr(VERTS, t->hdr_dom));
    w32v(PDATA + 0x2C, domain_addr(OTADDR, t->buf_dom));
    w32v(PDATA + 0x30, domain_addr(OTADDR + 0x100, t->buf_dom));
    memset(gaddr(PDATA + 0x44), 0, 8);
    w16v(PDATA + 0x44, 0x0400);
    w16v(PDATA + 0x4C, t->color_words[0]);
    w16v(PDATA + 0x4E, t->color_words[1]);
    w16v(PDATA + 0x50, t->color_words[2]);

    /* Vertex table (geometry header target); not dereferenced by the bridged
     * renderer, but keep deterministic bytes. */
    for (unsigned i = 0; i < 0x100; ++i)
        gaddr(VERTS)[i] = (uint8_t)(i * 3u + 1u);

    /* OT region: distinctive words. */
    for (unsigned i = 0; i < OT_RANGE_SIZE; i += 4)
        w32v(OT_RANGE_START + i, 0xd5000000u | (i & 0xFFFFu));

    /* GTE-visible globals. */
    w32v(0x800592F8, t->ctx);
    w32v(0x80050100, (uint32_t)t->shift);
    w32v(0x8005956C, domain_addr(OTADDR, t->buf_dom));
    w32v(0x80059580, 0x80170000u);
    w32v(0x80059534, 0x80170040u);

    g_GfxCurContext = (s32)t->ctx;
    D_80050100 = t->shift;
    g_GfxCurOT = (u_long*)(uintptr_t)domain_addr(OTADDR, t->buf_dom);

    call_80022038 = 0;
    call_800B1F6C = 0;
    memset(f6c_args, 0, sizeof(f6c_args));

    gte_reset();
    (void)native;
}

/* -- Snapshots ----------------------------------------------------------- */

static uint32_t canon(uint32_t v)
{
    uintptr_t base = (uintptr_t)g_PsxRam;
    if (v >= base && v < base + PSX_RAM_SIZE)
        return (uint32_t)(v - base);
    if ((v & 0xFFE00000u) == 0x80000000u || (v & 0xFFE00000u) == 0xA0000000u)
        return v & 0x1FFFFFu;
    return v;
}

typedef struct {
    uint8_t fixture[FIXTURE_SIZE];
    uint8_t ot[OT_RANGE_SIZE];
    uint8_t color[0x40];
    uint8_t light[0x40];
    uint8_t camera[0x20];
    uint32_t ctx;
    uint32_t otptr;
    int32_t shift;
    uint32_t f6c[16][6];
    unsigned n6c;
    unsigned n22038;
    uint32_t gte[64];
} Snap;

static void snapshot(Snap* s, int native)
{
    memset(s, 0, sizeof(*s));
    memcpy(s->fixture, gaddr(FIXTURE_BASE), sizeof(s->fixture));
    memcpy(s->ot, gaddr(OT_RANGE_START), sizeof(s->ot));
    memcpy(s->color, gaddr(0x8004FD80), sizeof(s->color));
    memcpy(s->light, gaddr(0x8004FDA0), sizeof(s->light));
    if (native) {
        s->ctx = (uint32_t)g_GfxCurContext;
        s->otptr = canon((uint32_t)(uintptr_t)g_GfxCurOT);
        s->shift = D_80050100;
        memcpy(s->camera, &D_8004FBB8, sizeof(s->camera));
    } else {
        s->ctx = r32v(0x800592F8);
        s->otptr = canon(r32v(0x8005956C));
        s->shift = (int32_t)r32v(0x80050100);
        memcpy(s->camera, gaddr(0x8004FBB8), sizeof(s->camera));
    }
    for (unsigned i = 0; i < call_800B1F6C; ++i) {
        s->f6c[i][0] = canon(f6c_args[i][0]);
        s->f6c[i][1] = canon(f6c_args[i][1]);
        s->f6c[i][2] = canon(f6c_args[i][2]);
        s->f6c[i][3] = f6c_args[i][3];
        s->f6c[i][4] = f6c_args[i][4];
        s->f6c[i][5] = f6c_args[i][5];
    }
    s->n6c = call_800B1F6C;
    s->n22038 = call_80022038;
    for (unsigned i = 0; i < 32; ++i) {
        s->gte[i] = MFC2((int)i);
        s->gte[32 + i] = CFC2((int)i);
    }
}

static int fail(const char* name, const char* what, unsigned i)
{
    fprintf(stderr, "INDEX15 FAIL case=%s %s index=%u\n", name, what, i);
    return 0;
}

static int compare(const Case* t, const Snap* native, const Snap* retail)
{
#define CMP(field) \
    if (memcmp(&native->field, &retail->field, sizeof(native->field)) != 0) { \
        unsigned i = 0; \
        while (i < sizeof(native->field) && \
               ((const uint8_t*)&native->field)[i] == \
               ((const uint8_t*)&retail->field)[i]) ++i; \
        return fail(t->name, #field, i); \
    }
    CMP(fixture);
    CMP(ot);
    CMP(color);
    CMP(light);
    CMP(camera);
    CMP(ctx);
    CMP(otptr);
    CMP(shift);
    CMP(f6c);
    if (native->n6c != retail->n6c)
        return fail(t->name, "n6c", native->n6c);
    if (native->n22038 != retail->n22038)
        return fail(t->name, "n22038", native->n22038);
    CMP(gte);
#undef CMP
    return 1;
}

/* -- Driver -------------------------------------------------------------- */

static uint8_t image257[0x298];
static uint8_t image25A[0x17C];

static void run_retail(const Case* t, unsigned entry)
{
    PcPortMipsBus bus = {.read = bus_read, .write = bus_write, .bridge = bridge};
    PcPortMipsCpu cpu;
    uint32_t pc = entry ? 0x80025A88u : 0x800257F0u;
    const uint8_t* src = entry ? image25A : image257;
    unsigned size = entry ? sizeof(image25A) : sizeof(image257);

    setup(t, 0);
    for (unsigned i = 0; i < size; ++i)
        gaddr(pc)[i] = src[i];
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = TASK;
    cpu.gpr[28] = 0x80059170u;
    cpu.gpr[29] = STACK;
    cpu.gpr[31] = HALT;
    if (PcPortMipsRun(&cpu, pc, HALT, 20000) != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "INDEX15 FAIL case=%s oracle %s pc=%08x\n", t->name,
                cpu.error, cpu.pc);
        exit(2);
    }
}

extern void func_800257F0(uint8_t* pEntry);
extern void func_80025A88(uint8_t* pEntry);

static volatile sig_atomic_t native_active;

static void pointer_fault(int sig)
{
    (void)sig;
    static const char marker[] =
        "INDEX15 POINTER_FAULT SIGSEGV during native case\n";
    if (native_active) {
        /* best effort from a signal handler; nothing to do on failure */
        if (write(2, marker, sizeof(marker) - 1) < 0)
            _exit(90);
        _exit(90);
    }
    _exit(91);
}

static void run_native(const Case* t, unsigned entry)
{
    setup(t, 1);
    native_active = 1;
    if (entry)
        func_80025A88((uint8_t*)PSX_ADDR(TASK));
    else
        func_800257F0((uint8_t*)PSX_ADDR(TASK));
    native_active = 0;
}

static const Case callback_cases[] = {
    {"early-return", 0, 0, 0, 0, 0, 3, 0, 0, 2, {0x1000, 0x2000, 0x3000}},
    {"plain", 0, 0x00000000, 0, 0x0040, 0, 0, 0, 0, 2, {0x1000, 0x2000, 0x3000}},
    {"color-on", 2, 0x00000000, 0, 0x0040, 0, 0, 0, 0, 2, {0x0C00, 0x0800, 0x0400}},
    {"raw-matrix", 0, 0x01000000u, 1, 0x0040, 0, 0, 0, 0, 2, {0,0,0}},
    {"geom-override", 0, 0x80000000u, 0, 0x0040, 0, 0, 0, 0, 2, {0,0,0}},
    {"halved", 0, 0x02000000u, 0, 0x0040, 0, 0, 0, 0, 2, {0,0,0}},
    {"geom-halved", 2, 0x83000000u, 1, 0xFFF0, 0, 0, 0, 0, 2, {1,2,3}},
    {"ctx-one", 0, 0, 0, 0x0040, 0, 0, 0, 1, 2, {0,0,0}},
    {"ctx-one-halved", 0, 0x02000000u, 0, 0x0040, 0, 0, 0, 1, 2, {0,0,0}},
    {"kseg1-sprite", 0, 0, 0, 0x0040, 1, 0, 0, 0, 2, {0,0,0}},
    {"kseg1-hdr-buf", 0, 0, 0, 0x0040, 0, 1, 1, 0, 2, {0,0,0}},
    {"native-low-hdr", 0, 0, 0, 0x0040, 0, 2, 2, 0, 2, {0,0,0}},
    {"flag-max", 0, 0x000000E0u, 0, 0x0040, 0, 0, 0, 0, 2, {0,0,0}},
    {"flag-and-halved", 0, 0x020000E0u, 0, 0x0040, 0, 0, 0, 0, 2, {0,0,0}},
    {"shift-one", 2, 0, 0, 0x0040, 0, 0, 0, 0, 1, {1,2,3}},
    {"shift-31", 2, 0, 0, 0x0040, 0, 0, 0, 0, 31, {1,2,3}},
    {"shift-large", 2, 0, 0, 0x0040, 0, 0, 0, 0, -4096, {1,2,3}},
    {"gate-bit1-only", 2, 0, 0, 0x0040, 0, 0, 0, 0, 2, {0xFFFF, 0x8000, 0x0001}},
    {"negative-flags", 0, 0x81000080u, 1, 0x8000, 0, 0, 0, 1, 0, {0,0,0}},
};

static const Case sibling_cases[] = {
    {"sibling-early-return", 0, 0, 0, 0, 0, 3, 0, 0, 2, {0,0,0}, 0},
    {"sibling-plain", 0, 0x00000000, 0, 0x0040, 0, 0, 0, 0, 2, {0,0,0}, 0},
    {"sibling-halved", 0, 0x02000000u, 0, 0x0040, 0, 0, 0, 0, 2, {0,0,0}, 0},
    {"sibling-ctx-one", 0, 0, 0, 0x0040, 0, 0, 0, 1, 5, {0,0,0}, 0},
    {"sibling-ctx-one-halved", 0, 0x02000000u, 0, 0x0040, 0, 0, 0, 1, 5, {0,0,0}, 0},
    {"sibling-kseg1-sprite", 0, 0, 0, 0x0040, 1, 0, 0, 0, 5, {0,0,0}, 0},
    {"sibling-kseg1-sub", 0, 0, 0, 0x0040, 0, 0, 0, 0, 5, {0,0,0}, 1},
    {"sibling-native-low-sprite", 0, 0, 0, 0x0040, 2, 2, 2, 1, 5, {0,0,0}, 2},
    {"sibling-native-low-sub", 0, 0, 0, 0x0040, 0, 0, 0, 1, 5, {0,0,0}, 2},
    {"sibling-flag-max", 0, 0x000000E0u, 0, 0x0040, 0, 0, 0, 0, 5, {0,0,0}, 0},
    {"sibling-negative-depth", 0, 0x80000000u, 0, 0xFFF0, 0, 0, 0, 0, 5, {0,0,0}, 0},
};

static int run_suite(const Case* cases, unsigned n, unsigned entry)
{
    unsigned cases_run = 0;
    for (unsigned i = 0; i < n; ++i) {
        const Case* t = &cases[i];
        Snap retail, native;
        run_retail(t, entry);
        snapshot(&retail, 0);
        run_native(t, entry);
        snapshot(&native, 1);
        if (!compare(t, &native, &retail)) {
            fprintf(stderr, "INDEX15 retail/native mismatch: %s\n", t->name);
            return -1;
        }
        ++cases_run;
    }
    return (int)cases_run;
}

int main(void)
{
    FILE* f = fopen("disc/SLUS_006.64", "rb");
    assert(f != NULL);
    assert(fseek(f, 0x800257F0u - 0x8000F800u, SEEK_SET) == 0);
    assert(fread(image257, 1, sizeof(image257), f) == sizeof(image257));
    assert(fseek(f, 0x80025A88u - 0x8000F800u, SEEK_SET) == 0);
    assert(fread(image25A, 1, sizeof(image25A), f) == sizeof(image25A));
    assert(!fclose(f));

    signal(SIGSEGV, pointer_fault);

    int a = run_suite(callback_cases,
                      sizeof(callback_cases) / sizeof(callback_cases[0]), 0);
    if (a < 0)
        return 1;
    int b = run_suite(sibling_cases,
                      sizeof(sibling_cases) / sizeof(sibling_cases[0]), 1);
    if (b < 0)
        return 1;

    printf("INDEX15 CALLBACK PASS %d+%d cases: packed pointer translation, "
           "context stride, geom override/restore, D_80050100 save/restore, "
           "flags, domains and full GTE state\n", a, b);
    return 0;
}
