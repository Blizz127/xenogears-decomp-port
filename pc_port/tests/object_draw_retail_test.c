/*
 * Differential regression test for func_801DCEC8 (archive 6B9 object draw).
 *
 * The raw retail bytes of [801DCEC8,801DDBF8) run on the project's MIPS
 * interpreter against the native body from pc_port/src/field_object_overlay.c.
 * Both sides share one PsyCross GTE, one set of matrix/trig spies (recorded
 * with their arguments), one func_8002C700 spy and the retail inline OT
 * link semantics, so the comparison covers: the whole object/node fixture
 * (ground-quad packet, shade byte, OT links), the ordered SDK/GTE boundary
 * record (which matrices reach SetLightMatrix / SetRotMatrix / SetTransMatrix
 * before every mesh dispatch, and with which draw variant), the scratchpad
 * (0x1F800000 on the guest, g_PsxScratchpad natively) and the final GTE
 * register file.
 *
 * Attached-effect lists (obj+0x10C/10D/10E) are held at zero: their owners
 * are separate translation units with their own tests.
 */
#include <stdio.h>
#include <string.h>
#ifndef OBJECT_OVERLAY_SOURCE
#define OBJECT_OVERLAY_SOURCE "../src/field_object_overlay.c"
#endif
#include OBJECT_OVERLAY_SOURCE
#include "battle_mips_adapter.h"
#include "psx/inline_c.h"
#include "psx/gtereg.h"

extern void func_801DCEC8(u8*, MATRIX*, MATRIX*, s32, s32, u32*, s32) __attribute__((weak));

u8 g_PsxScratchpad[4096];
s32 D_80050100;
s32 D_80050104;
s32 D_80059578;

enum { NODES = 5, OT_N = 0x1002, MODELS = 6, REC_MAX = 64 };

static u8 ram[0x200000], guestScratch[4096];
static struct {
    u32 obj[0x134 / 4];
    u32 nodes[NODES * 0x7C / 4];
    u32 ptrs[MODELS];
    u32 models[MODELS][0x40 / 4];
    u32 ot[OT_N];
    MATRIX view;
    MATRIX light;
    OvlyPtrTab tab;
} fixture, initial, expected;

static struct Record { u32 kind; u8 a[32], b[32]; } records[REC_MAX], expectedRecords[REC_MAX];
static unsigned nRecords, overflow;

static void record(u32 kind, const void* a, unsigned na, const void* b, unsigned nb) {
    struct Record* r;
    if (nRecords >= REC_MAX) { overflow = 1; return; }
    r = &records[nRecords++];
    r->kind = kind;
    memset(r->a, 0, 32); memset(r->b, 0, 32);
    if (a) memcpy(r->a, a, na);
    if (b) memcpy(r->b, b, nb);
}

/* Deterministic stand-ins for the matrix SDK: the output depends on both
 * inputs and on the call, so a swapped or missing operand changes every
 * later matrix that reaches the GTE. */
static MATRIX* matrixOutput(u32 kind, MATRIX* a, MATRIX* b, MATRIX* out) {
    MATRIX x = *a, y = *b;
    unsigned i, j;
    record(kind, a, 32, b, 32);
    for (i = 0; i < 3; ++i)
        for (j = 0; j < 3; ++j)
            out->m[i][j] = (s16)((u16)x.m[i][j] + (u16)y.m[j][i] * 3 + i * 7 + j);
    if (kind == 0x8004931c)
        for (i = 0; i < 3; ++i) out->t[i] = (s32)((u32)x.t[i] + (u32)y.t[i] * 5 + i);
    if (kind == 0x80049acc)
        for (i = 0; i < 3; ++i) out->t[i] = x.t[i];
    return out;
}
MATRIX* MulMatrix0(MATRIX* a, MATRIX* b, MATRIX* out) { return matrixOutput(0x8004920c, a, b, out); }
MATRIX* CompMatrix(MATRIX* a, MATRIX* b, MATRIX* out) { return matrixOutput(0x8004931c, a, b, out); }
MATRIX* MulMatrix(MATRIX* a, MATRIX* b) { return matrixOutput(0x80049acc, a, b, a); }
MATRIX* RotMatrix(SVECTOR* r, MATRIX* m) {
    unsigned i, j;
    record(0x8003f738, r, 8, NULL, 0);
    for (i = 0; i < 3; ++i)
        for (j = 0; j < 3; ++j)
            m->m[i][j] = (s16)(r->vx * (s32)(i + 1) + r->vy * (s32)(j + 2) + r->vz * 3 + (s32)(i * j));
    return m;
}
void SetRotMatrix(MATRIX* m) {
    unsigned i;
    record(0x80049efc, m, 32, NULL, 0);
    for (i = 0; i < 5; ++i) CTC2(((u32*)m)[i], i);
}
void SetTransMatrix(MATRIX* m) {
    unsigned i;
    record(0x80049f8c, m, 32, NULL, 0);
    for (i = 0; i < 3; ++i) CTC2((u32)m->t[i], 5 + i);
}
void SetLightMatrix(MATRIX* m) {
    unsigned i;
    record(0x80049f2c, m, 32, NULL, 0);
    for (i = 0; i < 5; ++i) CTC2(((u32*)m)[i], 8 + i);
}
int ratan2(int y, int x) {
    s32 in[2] = { (s32)y, (s32)x };
    record(0x8004b32c, in, 8, NULL, 0);
    return ((y * 7 + x * 13) & 0xFFF) - 0x800;
}
/* The include shim maps source-level rsin/rcos onto the C symbols rcos/rsin
 * (retail 0x8003F8CC / 0x8003F8B0); define the C symbols themselves. */
#undef rcos
#undef rsin
int rcos(int a) { record(0x8003f8cc, &a, 4, NULL, 0); return (a * 3) & 0xFFF; }
int rsin(int a) { record(0x8003f8b0, &a, 4, NULL, 0); return (a * 5) & 0xFFF; }
s32 func_8002C700(void* model, void* packet, void* ot, s32 variant) {
    u32 in[4] = { (u32)(uintptr_t)model, (u32)(uintptr_t)packet, (u32)(uintptr_t)ot, (u32)variant };
    record(0x8002c700, in, 16, NULL, 0);
    return 1;
}
/* Link-time dependencies of the attached-effect owners that the included
 * translation unit carries. They are never reached with the effect counts
 * held at zero; reaching one is itself a failure. */
static void unreachable(const char* who) {
    fprintf(stderr, "OBJECT DRAW FAIL unexpected call to %s\n", who);
    abort();
}
void SetSemiTrans(void* p, int abe) { (void)p; (void)abe; unreachable("SetSemiTrans"); }
int LoadImage(RECT16* rect, u_long* p) { (void)rect; (void)p; unreachable("LoadImage"); return 0; }
u_int HeapFree(void* p) { (void)p; unreachable("HeapFree"); return 0; }
void func_80026F44(s32 a, s32 b, u16* c, const u16* d) { (void)a; (void)b; (void)c; (void)d; unreachable("func_80026F44"); }
void func_80026FE8(s32 a, s32 b, u16* c, const u16* d, const u16* e) { (void)a; (void)b; (void)c; (void)d; (void)e; unreachable("func_80026FE8"); }
int SquareRoot0(int a) { (void)a; unreachable("SquareRoot0"); return 0; }
long VectorNormal(VECTOR* a, VECTOR* b) { (void)a; (void)b; unreachable("VectorNormal"); return 0; }

/* Retail's inline OT link, as the port's adapter performs it. */
void PcPort_LinkModelPrim(u32* ot, s32 index, void* packet, u32 tagLength) {
    u32* tag = (u32*)packet;
    *tag = (*tag & 0x00FFFFFFu) | (tagLength & 0xFF000000u);
    *tag = (*tag & 0xFF000000u) | (ot[index] & 0x00FFFFFFu);
    ot[index] = (ot[index] & 0xFF000000u) | ((u32)(uintptr_t)packet & 0x00FFFFFFu);
}

static u8* address(u32 a, unsigned w) {
    uintptr_t lo = (uintptr_t)&fixture;
    if (a >= 0x80000000u && (uint64_t)a + w <= 0x80200000u) return ram + (a & 0x1fffff);
    if (a >= 0x1f800000u && (uint64_t)a + w <= 0x1f801000u) return guestScratch + (a - 0x1f800000u);
    if (a >= lo && (uint64_t)a + w <= lo + sizeof(fixture)) return (u8*)(uintptr_t)a;
    return NULL;
}
static int rd(void* u, u32 a, unsigned w, u32* v) {
    unsigned i; u8* p = address(a, w); (void)u;
    if (!p) return -1;
    *v = 0;
    for (i = 0; i < w; ++i) *v |= (u32)p[i] << (8 * i);
    return 0;
}
static int wr(void* u, u32 a, unsigned w, u32 v) {
    unsigned i; u8* p = address(a, w); (void)u;
    if (!p) return -1;
    for (i = 0; i < w; ++i) p[i] = (u8)(v >> (8 * i));
    return 0;
}
static u32 c2read(void* u, int control, unsigned r) { (void)u; return control ? CFC2(r) : MFC2(r); }
static void c2write(void* u, int control, unsigned r, u32 v) { (void)u; if (control) CTC2(v, r); else MTC2(v, r); }
static int c2command(void* u, u32 op) { (void)u; doCOP2(op); return 0; }
static u32 unexpectedTarget;
static int bridge(void* u, PcPortMipsCpu* c, u32 t) {
    u32* a = c->gpr + 4;
    MATRIX* ma = (MATRIX*)address(a[0], 32);
    MATRIX* mb = (MATRIX*)address(a[1], 32);
    MATRIX* out = (MATRIX*)address(a[2], 32);
    (void)u;
    switch (t) {
    case 0x8004920c: MulMatrix0(ma, mb, out); c->gpr[2] = a[2]; break;
    case 0x8004931c: CompMatrix(ma, mb, out); c->gpr[2] = a[2]; break;
    case 0x80049acc: MulMatrix(ma, mb); c->gpr[2] = a[0]; break;
    case 0x80049efc: SetRotMatrix(ma); break;
    case 0x80049f8c: SetTransMatrix(ma); break;
    case 0x80049f2c: SetLightMatrix(ma); break;
    case 0x8003f738: RotMatrix((SVECTOR*)address(a[0], 8), mb); c->gpr[2] = a[1]; break;
    case 0x8004b32c: c->gpr[2] = (u32)ratan2((int)(s32)a[0], (int)(s32)a[1]); break;
    case 0x8003f8cc: c->gpr[2] = (u32)rcos((s32)a[0]); break;
    case 0x8003f8b0: c->gpr[2] = (u32)rsin((s32)a[0]); break;
    case 0x8002c700: {
        u32 v;
        c->gpr[2] = (u32)func_8002C700((void*)(uintptr_t)a[0], (void*)(uintptr_t)a[1],
                                       (void*)(uintptr_t)a[2], (s32)a[3]);
        (void)v;
        break;
    }
    case 0x801e22f8: case 0x801e1258: case 0x801e0248:
        unexpectedTarget = t; return 0;
    default: return 0;
    }
    return 1;
}

static u32 seed;
static u32 next(void) { seed = seed * 1103515245u + 12345u; return seed >> 8; }
static s16 pick16(void) { return (s16)(next() & 0xffff); }

int main(void) {
    static const s16 scales[] = { 0, 320, 644, 4096 };
    static const s16 heights[] = { -3000, -1, 0, 1, 4500 };
    static const s16 extents[] = { 0, 100, 2496, -1952 };
    static const s32 shifts[] = { 2, 4 };
    FILE* f;
    unsigned i, cases = 0, cfg;

    if (!func_801DCEC8) { fputs("OBJECT DRAW FAIL missing native owner\n", stderr); return 1; }
    f = fopen("disc/disc1.bin", "rb"); assert(f);
    for (i = 0; i < 25; ++i) {
        assert(!fseek(f, (231361 + i) * 2352L + 24, SEEK_SET));
        assert(fread(ram + 0x1dc000 + i * 2048, 1, 2048, f) == 2048);
    }
    assert(!fclose(f));

    for (cfg = 0; cfg < 1536; ++cfg) {
        unsigned visible = cfg & 1, noQuad = (cfg >> 1) & 1, ctx = (cfg >> 2) & 1;
        unsigned scaleIx = (cfg >> 3) & 3, heightIx = (cfg >> 5) % 5, rootHeightIx = (cfg >> 5) / 5 % 5;
        unsigned extIx = (cfg >> 7) & 3, shiftIx = (cfg >> 9) & 1, drawArg = 1 + ((cfg >> 10) & 1);
        unsigned count = 2 + (cfg % 4), n;
        u8* obj = (u8*)fixture.obj;
        u8* root = (u8*)fixture.nodes;
        GTERegisters expectedGte;
        unsigned expectedCount;
        PcPortMipsBus bus = { .read = rd, .write = wr, .bridge = bridge,
                              .cop2_read = c2read, .cop2_write = c2write, .cop2_command = c2command };
        PcPortMipsCpu cpu;

        seed = 0x9e3779b9u ^ (cfg * 2654435761u);
        memset(&fixture, 0, sizeof(fixture));
        for (i = 0; i < sizeof(fixture.nodes) / 4; ++i) ((u32*)fixture.nodes)[i] = next();
        for (i = 0; i < sizeof(fixture.obj) / 4; ++i) fixture.obj[i] = next();
        for (i = 0; i < OT_N; ++i) fixture.ot[i] = (0x0Au << 24) | ((i * 0x1010u + 0x123u) & 0xFFFFFFu);
        for (i = 0; i < 9; ++i) {
            ((s16*)fixture.view.m)[i] = (s16)(pick16() / 4);
            ((s16*)fixture.light.m)[i] = (s16)(pick16() / 4);
        }
        for (i = 0; i < 3; ++i) { fixture.view.t[i] = pick16() / 2; fixture.light.t[i] = pick16() / 2; }
        for (i = 0; i < MODELS; ++i) fixture.ptrs[i] = (u32)(uintptr_t)fixture.models[i];
        fixture.tab.ptrs = fixture.ptrs;
        fixture.tab.count = MODELS;

        *(u32*)(obj + 0x00) = (u32)(uintptr_t)&fixture.tab;
        *(u32*)(obj + 0x04) = (u32)(uintptr_t)root;
        *(u16*)(obj + 0x1C) = (u16)scales[scaleIx];
        *(u16*)(obj + 0x26) = (u16)extents[extIx];
        *(u16*)(obj + 0x28) = (u16)extents[(extIx + 1) & 3];
        obj[0x34] = (u8)visible;
        *(u16*)(obj + 0x4A) = (u16)((next() & 0xFFFE) | noQuad);
        *(s16*)(obj + 0x60) = heights[heightIx];
        obj[0x10C] = obj[0x10D] = obj[0x10E] = 0;
        *(u16*)(root + 0x0A) = (u16)count;
        *(s32*)(root + 0x60) = heights[rootHeightIx] * 3;
        for (n = 0; n < NODES; ++n) {
            u8* node = root + n * 0x7C;
            unsigned r = next();
            *(u16*)(node + 0x08) = (r & 4) ? 0xFFFF : (u16)(r % MODELS);
            node[7] = (u8)((r >> 3) & 1);
            *(s16*)(node + 0x52) = (s16)((r >> 4) % 3);
            *(u32*)(node + 0x68) = 0x800C0000u + n * 0x400u;
            *(u32*)(node + 0x6C) = 0x800D0000u + n * 0x400u;
            for (i = 0; i < 9; ++i) ((s16*)(node + 0x0C))[i] = (s16)(pick16() / 4);
            for (i = 0; i < 9; ++i) ((s16*)(node + 0x2C))[i] = (s16)(pick16() / 4);
        }
        D_80050100 = shifts[shiftIx];
        *(u32*)(ram + 0x50100) = (u32)shifts[shiftIx];
        initial = fixture;

        memset(g_PsxScratchpad, 0x36, sizeof(g_PsxScratchpad));
        memcpy(guestScratch, g_PsxScratchpad, sizeof(guestScratch));
        memset(&gteRegs, 0, sizeof(gteRegs));
        nRecords = 0; overflow = 0; unexpectedTarget = 0;
        memset(records, 0, sizeof(records));
        PcPortMipsCpuInit(&cpu, &bus);
        cpu.gpr[4] = (u32)(uintptr_t)obj;
        cpu.gpr[5] = (u32)(uintptr_t)&fixture.view;
        cpu.gpr[6] = (u32)(uintptr_t)&fixture.light;
        cpu.gpr[7] = drawArg;
        cpu.gpr[29] = 0x801ff000;
        cpu.gpr[31] = 0xfffffffcu;
        *(u32*)(ram + 0x1ff010) = 1;
        *(u32*)(ram + 0x1ff014) = (u32)(uintptr_t)fixture.ot;
        *(u32*)(ram + 0x1ff018) = ctx;
        if (PcPortMipsRun(&cpu, 0x801dcec8, 0xfffffffcu, 200000) != PC_PORT_MIPS_HALTED) {
            fprintf(stderr, "OBJECT DRAW FAIL oracle cfg=%u %s\n", cfg, cpu.error);
            return 1;
        }
        if (unexpectedTarget || overflow) {
            fprintf(stderr, "OBJECT DRAW FAIL oracle cfg=%u target=%08x overflow=%u\n",
                    cfg, unexpectedTarget, overflow);
            return 1;
        }
        expected = fixture;
        memcpy(expectedRecords, records, sizeof(records));
        expectedCount = nRecords;
        expectedGte = gteRegs;

        fixture = initial;
        memset(&gteRegs, 0, sizeof(gteRegs));
        nRecords = 0; overflow = 0;
        memset(records, 0, sizeof(records));
        func_801DCEC8(obj, &fixture.view, &fixture.light, (s32)drawArg, 1, fixture.ot, (s32)ctx);

        if (memcmp(&fixture, &expected, sizeof(fixture)) || nRecords != expectedCount ||
            memcmp(records, expectedRecords, sizeof(records)) ||
            memcmp(guestScratch, g_PsxScratchpad, sizeof(guestScratch)) ||
            memcmp(&gteRegs, &expectedGte, sizeof(gteRegs))) {
            fprintf(stderr,
                    "OBJECT DRAW FAIL cfg=%u visible=%u noQuad=%u ctx=%u scale=%d count=%u "
                    "records=%u/%u\n",
                    cfg, visible, noQuad, ctx, scales[scaleIx], count, nRecords, expectedCount);
            return 1;
        }
        ++cases;
    }
    printf("OBJECT DRAW PASS %u cases: ground quad/OT link/shade byte, per-node light+view "
           "matrices, draw variant, scratch, GTE\n", cases);
    return 0;
}
