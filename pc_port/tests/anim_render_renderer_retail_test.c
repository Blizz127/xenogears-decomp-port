/* Animation-render renderer index 15 (func_800B1F6C / func_800B1F0C)
 * differential.
 *
 * The retail battle-overlay slice is loaded whole from disc/battle.bin at its
 * real base 0x8006FAF0, so the recorded jump table, D_800C3BF8 draw-env packet
 * and func_800B1F0C all execute as retail.  The interpreter COP2 bus is wired
 * to the same PsyCross GTE the native body uses; RotAverageNclip4, NormalColor,
 * NormalColor3, SetDrawTPage and AddPrim are bridged to the same host functions
 * the native body calls.  Native and retail must leave identical packet bytes,
 * OT chains, work-buffer state and full visible GTE state. */
#include <assert.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "battle_mips_adapter.h"
#include "psx_memory.h"
#include "guest_prim_link.h"
#include "psyq/libgte.h"
#include "psyq/libgpu.h"
#include "psx/gtereg.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[1024];

typedef uint8_t u8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;

extern unsigned MFC2(int);
extern unsigned CFC2(int);
extern void MTC2(unsigned, int);
extern void CTC2(unsigned, int);
extern int doCOP2(int);

s32 D_80050100 = 2;
u32 g_GfxCurWorkBuffer;
u32 g_GfxCurWorkBufferEnd;

/* PsyCross hosts SetDrawTPage in LIBGPU.C with a large dependency tail; the
 * renderer test supplies the identical inline-macro body. */
void SetDrawTPage(DR_TPAGE* p, int dfe, int dtd, int tpage)
{
    setDrawTPage(p, dfe, dtd, tpage);
}

#define HEADER   0x80100000u
#define RECORDS  0x80100100u
#define VERTS    0x80100800u
#define NORMALS  0x80100C00u
#define PACKET   0x80108000u
#define PACKET_SZ 0x4000u
#define WORK     0x80170000u
#define WORK_SZ  0x100u
#define OTADDR   0x80190000u
#define OT_SZ    0x4000u
#define STACK    0x801ff000u
#define HALT     0xfffffffcu
#define BATTLE_BASE 0x8006FAF0u
#define BATTLE_SZ   0x53F80u
#define REC_SZ   0x40u

static uint8_t battle[BATTLE_SZ];

static uint8_t* bus_ptr(uint32_t v)
{
    uintptr_t base = (uintptr_t)g_PsxRam;
    if (v >= base && v < base + PSX_RAM_SIZE)
        return (uint8_t*)(uintptr_t)v;
    if ((v & 0xFFE00000u) == 0x80000000u || (v & 0xFFE00000u) == 0xA0000000u)
        return (uint8_t*)PSX_ADDR(v);
    assert(!"pointer outside guest RAM");
    return NULL;
}

static uint8_t* gaddr(uint32_t a) { return (uint8_t*)PSX_ADDR(a & 0x1FFFFFu); }
static void w16v(uint32_t a, uint16_t v) { memcpy(gaddr(a), &v, 2); }
static void w32v(uint32_t a, uint32_t v) { memcpy(gaddr(a), &v, 4); }
static uint32_t r32v(uint32_t a) { uint32_t v; memcpy(&v, gaddr(a), 4); return v; }

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
    case 0x8004A83Cu: {
        uint32_t sp = cpu->gpr[29];
        uint32_t outp[7];
        long xy[4] = {0, 0, 0, 0};
        long p = 0, otz = 0, flag = 0;
        long result;
        for (unsigned i = 0; i < 7; ++i)
            memcpy(&outp[i], bus_ptr(sp + 0x10 + i * 4), 4);
        result = RotAverageNclip4((SVECTOR*)bus_ptr(cpu->gpr[4]),
                                  (SVECTOR*)bus_ptr(cpu->gpr[5]),
                                  (SVECTOR*)bus_ptr(cpu->gpr[6]),
                                  (SVECTOR*)bus_ptr(cpu->gpr[7]),
                                  &xy[0], &xy[1], &xy[2], &xy[3],
                                  &p, &otz, &flag);
        for (unsigned i = 0; i < 4; ++i)
            memcpy(bus_ptr(outp[i]), &xy[i], 4);
        memcpy(bus_ptr(outp[4]), &p, 4);
        memcpy(bus_ptr(outp[5]), &otz, 4);
        memcpy(bus_ptr(outp[6]), &flag, 4);
        cpu->gpr[2] = (uint32_t)result;
        return 1;
    }
    case 0x8004A19Cu:
        NormalColor((SVECTOR*)bus_ptr(cpu->gpr[4]), (CVECTOR*)bus_ptr(cpu->gpr[5]));
        return 1;
    case 0x8004A1B8u: {
        uint32_t sp = cpu->gpr[29];
        uint32_t o1, o2;
        memcpy(&o1, bus_ptr(sp + 0x10), 4);
        memcpy(&o2, bus_ptr(sp + 0x14), 4);
        NormalColor3((SVECTOR*)bus_ptr(cpu->gpr[4]),
                     (SVECTOR*)bus_ptr(cpu->gpr[5]),
                     (SVECTOR*)bus_ptr(cpu->gpr[6]),
                     (CVECTOR*)bus_ptr(cpu->gpr[7]),
                     (CVECTOR*)bus_ptr(o1), (CVECTOR*)bus_ptr(o2));
        return 1;
    }
    case 0x80043E20u:
        SetDrawTPage((DR_TPAGE*)bus_ptr(cpu->gpr[4]), (int)cpu->gpr[5],
                     (int)cpu->gpr[6], (int)cpu->gpr[7]);
        return 1;
    case 0x80043B48u:
        PcPort_AddPrimDomainAware(bus_ptr(cpu->gpr[4]), bus_ptr(cpu->gpr[5]));
        return 1;
    default:
        return 0;
    }
}

static uint32_t cop_read(void* p, int c, unsigned r)
{
    (void)p;
    return c ? CFC2((int)r) : MFC2((int)r);
}

static void cop_write(void* p, int c, unsigned r, uint32_t v)
{
    (void)p;
    if (c)
        CTC2(v, (int)r);
    else
        MTC2(v, (int)r);
}

static int cop_command(void* p, uint32_t i)
{
    (void)p;
    doCOP2((int)i);
    return 0;
}

/* -- Fixture ------------------------------------------------------------- */

typedef struct {
    const char* name;
    unsigned shape;     /* record[3] & 0x1C */
    unsigned key_hi;    /* 0 or 1: record[2] selects key bit 8 = (rec[2]^1)&1 */
    unsigned count;
    unsigned domain;    /* 0 KSEG0, 1 KSEG1, 2 host pass-through */
    int32_t shift;
    unsigned tpage;
    int32_t bias;
    int32_t dqa;
    int32_t dqb;
    unsigned winding;   /* 0 front, 1 back */
    unsigned work_full; /* 0 room, 1 too short, 2 exactly one packet left */
} RCase;

/* Vertex coordinates for indices 0..5: 0/1/2 form a front-facing triangle. */
static const int16_t vertex_coords[8][3] = {
    {0, -100, 0x400}, {100, 100, 0x400}, {-100, 100, 0x400},
    {0, 100, 0x300}, {150, -50, 0x500}, {-150, -50, 0x500},
    {0, 0, 0x7FFF}, {0, 0, 0}, /* overflow / behind-camera */
};

static void put_vertex(unsigned index, unsigned variant)
{
    uint32_t a = VERTS + index * 8u;
    int16_t x = vertex_coords[index][0];
    int16_t y = vertex_coords[index][1];
    int16_t z = vertex_coords[index][2];
    if (variant == 2) { z = (int16_t)0x7FFF; x = (int16_t)0x7FFF; }
    if (variant == 3) { z = -0x200; }
    w16v(a + 0, (uint16_t)x);
    w16v(a + 2, (uint16_t)y);
    w16v(a + 4, (uint16_t)z);
    w16v(a + 6, 0);
}

static void tri_offsets(unsigned key, unsigned off[3])
{
    switch (key) {
    case 0x000: off[0]=0x08; off[1]=0x0A; off[2]=0x0C; break;
    case 0x004: off[0]=0x14; off[1]=0x16; off[2]=0x18; break;
    case 0x010: off[0]=0x10; off[1]=0x12; off[2]=0x14; break;
    case 0x014: off[0]=0x1C; off[1]=0x1E; off[2]=0x20; break;
    case 0x100: off[0]=0x0A; off[1]=0x0C; off[2]=0x0E; break;
    case 0x104: off[0]=0x12; off[1]=0x14; off[2]=0x16; break;
    case 0x110: off[0]=0x10; off[1]=0x16; off[2]=0x1A; break;
    case 0x114: off[0]=0x12; off[1]=0x16; off[2]=0x1A; break;
    default: off[0]=off[1]=off[2]=0x08; break;
    }
}

static void quad_offsets(unsigned key, unsigned off[4])
{
    switch (key) {
    case 0x008: off[0]=0x08; off[1]=0x0A; off[2]=0x0C; off[3]=0x0E; break;
    case 0x00C: off[0]=0x18; off[1]=0x1A; off[2]=0x1C; off[3]=0x1E; break;
    case 0x018: off[0]=0x14; off[1]=0x16; off[2]=0x18; off[3]=0x1A; break;
    case 0x01C: off[0]=0x24; off[1]=0x26; off[2]=0x28; off[3]=0x2A; break;
    case 0x108: off[0]=0x0A; off[1]=0x0C; off[2]=0x0E; off[3]=0x10; break;
    case 0x10C: off[0]=0x16; off[1]=0x18; off[2]=0x1A; off[3]=0x1C; break;
    case 0x118: off[0]=0x16; off[1]=0x1A; off[2]=0x1E; off[3]=0x22; break;
    case 0x11C: off[0]=0x16; off[1]=0x1A; off[2]=0x1E; off[3]=0x22; break;
    default: off[0]=off[1]=off[2]=off[3]=0x08; break;
    }
}

static uint32_t dom(uint32_t guest, unsigned d)
{
    if (d == 1)
        return 0xA0000000u | (guest & 0x1FFFFFu);
    if (d == 2)
        return (uint32_t)(uintptr_t)PSX_ADDR(guest);
    return guest;
}

static void gte_reset(int32_t dqa, int32_t dqb)
{
    memset(&gteRegs, 0, sizeof(gteRegs));
    /* Identity rotation, no translation. */
    CTC2(0x00001000u, 0);
    CTC2(0x00001000u, 2);
    CTC2(0x00001000u, 4);
    CTC2(0, 1); CTC2(0, 3); CTC2(0, 5);
    CTC2(0, 6); CTC2(0, 7);
    for (unsigned i = 8; i < 24; ++i)
        CTC2(0, (int)i);
    CTC2(160u << 16, 24);
    CTC2(112u << 16, 25);
    CTC2(512, 26);
    CTC2((uint32_t)dqa, 27);
    CTC2((uint32_t)dqb, 28);
    CTC2(0x155, 29);
    CTC2(0x100, 30);
}

static void setup(const RCase* t)
{
    memset(g_PsxRam, 0x9b, sizeof(g_PsxRam));
    memcpy(gaddr(BATTLE_BASE), battle, BATTLE_SZ);

    /* Geometry header. */
    w32v(HEADER + 0x00, VERTS - HEADER);
    w32v(HEADER + 0x08, NORMALS - HEADER);
    w32v(HEADER + 0x10, RECORDS - HEADER);
    w32v(HEADER + 0x14, t->count);

    for (unsigned i = 0; i < 8; ++i)
        put_vertex(i, 0);

    /* Normal table: 0x100 SVECTORs. */
    for (unsigned i = 0; i < 0x100; ++i) {
        w16v(NORMALS + i * 8u + 0, (uint16_t)(i * 5u));
        w16v(NORMALS + i * 8u + 2, (uint16_t)(i * 3u));
        w16v(NORMALS + i * 8u + 4, (uint16_t)(0x800u + i));
        w16v(NORMALS + i * 8u + 6, 0);
    }

    unsigned key = t->shape | (t->key_hi ? 0x100u : 0u);
    int is_quad = (t->shape == 0x08 || t->shape == 0x0C ||
                   t->shape == 0x18 || t->shape == 0x1C);
    for (unsigned r = 0; r < t->count; ++r) {
        uint32_t rec = RECORDS + r * REC_SZ;
        memset(gaddr(rec), 0x11, REC_SZ);
        memset(gaddr(rec), 0, 4);
        gaddr(rec)[0] = REC_SZ / 4 - 1;
        gaddr(rec)[1] = REC_SZ / 4 - 1;
        gaddr(rec)[2] = t->key_hi ? 0x00 : 0x01;
        gaddr(rec)[3] = (uint8_t)t->shape;
        if (is_quad) {
            unsigned off[4];
            quad_offsets(key, off);
            unsigned order[4] = {0, 1, 2, 3};
            if (t->winding && r == 0) { order[1] = 2; order[2] = 1; }
            for (unsigned k = 0; k < 4; ++k)
                w16v(rec + off[k], (uint16_t)order[k]);
        } else {
            unsigned off[3];
            tri_offsets(key, off);
            unsigned order[3] = {0, 1, 2};
            if (t->winding && r == 0) { order[1] = 2; order[2] = 1; }
            for (unsigned k = 0; k < 3; ++k)
                w16v(rec + off[k], (uint16_t)order[k]);
        }
        if (t->shape == 0x00 && r == 0 && t->winding == 3) {
            /* Force the behind-camera/overflow vertex. */
            unsigned off[3];
            tri_offsets(key, off);
            w16v(rec + off[1], 6);
        }
    }
    /* Guard word past the descriptor stream. */
    w32v(RECORDS + t->count * REC_SZ, 0xDEADBEEFu);

    /* Output packet arena. */
    for (unsigned i = 0; i < PACKET_SZ; i += 4)
        w32v(PACKET + i, 0x01000000u | (i & 0xFFFFFFu));
    for (unsigned i = 0; i < OT_SZ; i += 4)
        w32v(OTADDR + i, 0x02000000u | (i & 0xFFFFFFu));
    for (unsigned i = 0; i < WORK_SZ; i += 4)
        w32v(WORK + i, 0x03000000u | (i & 0xFFFFFFu));

    /* Draw-env source packet. */
    memset(gaddr(0x800C3BF8u), 0, 8);

    /* Work-buffer head/end.  Retail reads the guest-mapped word; the native
     * host global is a real host pointer (the port keeps it packed 32-bit). */
    uint32_t workEndGuest =
        t->work_full == 1 ? WORK + 4 : (t->work_full == 2 ? WORK + 8 : WORK + WORK_SZ);
    w32v(0x80059580u, dom(WORK, t->domain));
    w32v(0x80059534u, dom(workEndGuest, t->domain));
    g_GfxCurWorkBuffer = (u32)(uintptr_t)PSX_ADDR(WORK);
    g_GfxCurWorkBufferEnd = (u32)(uintptr_t)PSX_ADDR(workEndGuest);

    w32v(0x80050100u, (uint32_t)t->shift);
    D_80050100 = t->shift;

    gte_reset(t->dqa, t->dqb);
    PcPort_PrimLinkReset();
}

/* -- Snapshot ------------------------------------------------------------ */

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
    uint8_t head[0x800];
    uint8_t packet[PACKET_SZ];
    uint8_t ot[OT_SZ];
    uint8_t work[WORK_SZ];
    uint8_t tpage[8];
    uint32_t cursor;
    uint32_t gte[64];
} RSnap;

static void rsnapshot(RSnap* s, int native)
{
    memset(s, 0, sizeof(*s));
    memcpy(s->head, gaddr(HEADER), sizeof(s->head));
    memcpy(s->packet, gaddr(PACKET), sizeof(s->packet));
    memcpy(s->ot, gaddr(OTADDR), sizeof(s->ot));
    memcpy(s->work, gaddr(WORK), sizeof(s->work));
    memcpy(s->tpage, gaddr(0x800C3BF8u), sizeof(s->tpage));
    /* Native keeps a host pointer; retail keeps the guest word.  Compare the
     * canonically mappable value. */
    s->cursor = canon(native ? g_GfxCurWorkBuffer : r32v(0x80059580u));
    for (unsigned i = 0; i < 32; ++i) {
        s->gte[i] = MFC2((int)i);
        s->gte[32 + i] = CFC2((int)i);
    }
}

static int rfail(const RCase* t, const char* what, unsigned i)
{
    fprintf(stderr, "ANIMRENDER FAIL case=%s %s index=%u\n", t->name, what, i);
    return 0;
}

static int rcompare(const RCase* t, const RSnap* n, const RSnap* o)
{
#define RCMP(f) \
    if (memcmp(&n->f, &o->f, sizeof(n->f)) != 0) { \
        unsigned i = 0; \
        while (i < sizeof(n->f) && \
               ((const uint8_t*)&n->f)[i] == ((const uint8_t*)&o->f)[i]) ++i; \
        return rfail(t, #f, i); \
    }
    RCMP(head);
    RCMP(packet);
    RCMP(ot);
    RCMP(work);
    RCMP(tpage);
    RCMP(cursor);
    RCMP(gte);
#undef RCMP
    return 1;
}

/* -- Driver -------------------------------------------------------------- */

extern void func_800B1F6C(void*, void*, void*, s32, s32, s32);

static unsigned emitted;

static void run_retail(const RCase* t)
{
    PcPortMipsBus bus = {.read = bus_read, .write = bus_write, .bridge = bridge,
                         .cop2_read = cop_read, .cop2_write = cop_write,
                         .cop2_command = cop_command};
    PcPortMipsCpu cpu;
    setup(t);
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = dom(HEADER, t->domain);
    cpu.gpr[5] = dom(PACKET, t->domain);
    cpu.gpr[6] = dom(OTADDR, t->domain);
    cpu.gpr[7] = 0;
    cpu.gpr[28] = 0x80059170u;
    cpu.gpr[29] = STACK;
    cpu.gpr[31] = HALT;
    memcpy(bus_ptr(STACK + 0x10), &t->bias, 4);
    uint32_t tp = t->tpage;
    memcpy(bus_ptr(STACK + 0x14), &tp, 4);
    if (PcPortMipsRun(&cpu, 0x800B1F6Cu, HALT, 200000) != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "ANIMRENDER FAIL case=%s oracle %s pc=%08x\n", t->name,
                cpu.error, cpu.pc);
        exit(2);
    }
    /* Any OT word that no longer holds its sentinel proves a link happened. */
    for (unsigned i = 0; i < OT_SZ; i += 4) {
        if (r32v(OTADDR + i) != (0x02000000u | (i & 0xFFFFFFu))) {
            ++emitted;
            break;
        }
    }
}

static volatile sig_atomic_t native_active;

static void run_native(const RCase* t)
{
    setup(t);
    native_active = 1;
    func_800B1F6C((void*)(uintptr_t)dom(HEADER, t->domain),
                  (void*)(uintptr_t)dom(PACKET, t->domain),
                  (void*)(uintptr_t)dom(OTADDR, t->domain), 0,
                  t->bias, (s32)t->tpage);
    native_active = 0;
}

static void pointer_fault(int sig)
{
    (void)sig;
    static const char marker[] =
        "ANIMRENDER POINTER_FAULT SIGSEGV during native case\n";
    if (native_active) {
        if (write(2, marker, sizeof(marker) - 1) < 0) { /* best effort from a signal handler */ }
        _exit(90);
    }
    _exit(91);
}

int main(void)
{
    static const RCase cases[] = {
        {"count-zero",        0x00, 0, 0, 0, 2, 0,  16, 0, 0, 0, 0},
        {"tri-front",         0x00, 0, 1, 0, 2, 0,  16, 0, 0, 0, 0},
        {"tri-back",          0x00, 0, 1, 0, 2, 0,  16, 0, 0, 1, 0},
        {"tri-hi",            0x00, 1, 1, 0, 2, 0,  16, 0, 0, 0, 0},
        {"tri-2",             0x00, 0, 2, 0, 2, 0,  16, 0, 0, 0, 0},
        {"tri-3",             0x00, 0, 3, 0, 2, 0,  16, 0, 0, 0, 0},
        {"shape04",           0x04, 0, 2, 0, 2, 0,  16, 0, 0, 0, 0},
        {"shape04-hi",        0x04, 1, 2, 0, 2, 0,  16, 0, 0, 0, 0},
        {"shape08",           0x08, 0, 1, 0, 2, 0,  16, 0, 0, 0, 0},
        {"shape08-hi",        0x08, 1, 1, 0, 2, 0,  16, 0, 0, 0, 0},
        {"shape0C",           0x0C, 0, 2, 0, 2, 0,  16, 0, 0, 0, 0},
        {"shape0C-hi",        0x0C, 1, 2, 0, 2, 0,  16, 0, 0, 0, 0},
        {"shape10",           0x10, 0, 1, 0, 2, 0,  16, 0, 0, 0, 0},
        {"shape10-hi",        0x10, 1, 1, 0, 2, 0,  16, 0, 0, 0, 0},
        {"shape14",           0x14, 0, 2, 0, 2, 0,  16, 0, 0, 0, 0},
        {"shape14-hi",        0x14, 1, 2, 0, 2, 0,  16, 0, 0, 0, 0},
        {"shape18",           0x18, 0, 1, 0, 2, 0,  16, 0, 0, 0, 0},
        {"shape18-hi",        0x18, 1, 1, 0, 2, 0,  16, 0, 0, 0, 0},
        {"shape1C",           0x1C, 0, 2, 0, 2, 0,  16, 0, 0, 0, 0},
        {"shape1C-hi",        0x1C, 1, 2, 0, 2, 0,  16, 0, 0, 0, 0},
        {"tpage-one",         0x08, 0, 1, 0, 2, 1,  16, 0, 0, 0, 0},
        {"tpage-four",        0x00, 0, 1, 0, 2, 4,  16, 0, 0, 0, 0},
        {"tpage-four-shape1C",0x1C, 0, 1, 0, 2, 4,  16, 0, 0, 0, 0},
        {"shift-zero",        0x00, 0, 1, 0, 0, 0,  16, 0, 0x1000, 0, 0},
        {"shift-one",         0x00, 0, 1, 0, 1, 0,  16, 0, 0x1000, 0, 0},
        {"shift-two",         0x00, 0, 1, 0, 2, 0,  16, 0, 0x1000, 0, 0},
        {"shift-mask",        0x00, 0, 1, 0, 32, 0, 16, 0, 0x1000, 0, 0},
        {"depth-reject",      0x00, 0, 1, 0, 2, 0,  0x1000, 0, 0, 0, 0},
        {"depth-clamp",       0x00, 0, 1, 0, 31, 0, 4, 0, 0, 0, 0},
        {"depth-negative",    0x00, 0, 1, 0, 2, 0,  -4096, 0, 0, 0, 0},
        {"dqb-depth",         0x00, 0, 1, 0, 0, 0,  0, 0, 0x1000, 0, 0},
        {"dqb-depth-two",     0x00, 0, 1, 0, 1, 0,  0, 0, 0x1000, 0, 0},
        {"work-short",        0x00, 0, 1, 0, 2, 1,  16, 0, 0, 0, 1},
        {"work-exact",        0x00, 0, 1, 0, 2, 1,  16, 0, 0, 0, 2},
        {"work-room",         0x00, 0, 1, 0, 2, 1,  16, 0, 0, 0, 0},
        {"kseg1",             0x08, 0, 1, 1, 2, 1,  16, 0, 0, 0, 0},
        {"kseg1-tri-shape0",  0x00, 0, 1, 1, 2, 1,  16, 0, 0, 0, 0},
        {"kseg1-quad-shape1C",0x1C, 0, 2, 1, 2, 4,  16, 0, 0x1000, 0, 0},
        {"behind",            0x00, 0, 1, 0, 2, 0,  16, 0, 0, 0, 3},
        {"dqa-negative",      0x08, 0, 1, 0, 2, 0,  16, (int32_t)0xfffffff0, 0x100000, 0, 0},
    };
    FILE* f = fopen("disc/battle.bin", "rb");
    assert(f != NULL);
    assert(fread(battle, 1, BATTLE_SZ, f) == BATTLE_SZ);
    assert(!fclose(f));

    signal(SIGSEGV, pointer_fault);

    unsigned cases_run = 0;
    for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        const RCase* t = &cases[i];
        RSnap retail, native;
        run_retail(t);
        rsnapshot(&retail, 0);
        run_native(t);
        rsnapshot(&native, 1);
        if (!rcompare(t, &native, &retail)) {
            fprintf(stderr, "ANIMRENDER retail/native mismatch: %s\n", t->name);
            return 1;
        }
        ++cases_run;
    }
    if (emitted == 0) {
        fputs("ANIMRENDER FAIL no case emitted a primitive; fixture is inert\n",
              stderr);
        return 1;
    }
    printf("ANIMRENDER PASS %u cases (%u emitted): all 16 keys, 8 shapes, "
           "counts, tpage, depth clamp/reject, shift mask, domains, work-buffer "
           "capacity and full GTE state\n", cases_run, emitted);
    return 0;
}
