/* M1(b) retail-differential proof: func_8001E3D8 (sprite -> OT draw),
 * func_8001DAE8 (frame builder), func_8001D53C (alt frame builder) and
 * func_8001E9BC (shadow), rendering.c native bodies vs the retail SLUS
 * instructions on the port MIPS interpreter, identical guest memory.
 * See sprite_draw_retail_harness.h for scope. The real rendering.c is
 * #included so its static scratch (quad/shadow work vectors) is compared. */
#ifndef SPRITE_RENDER_SOURCE
#define SPRITE_RENDER_SOURCE "../../src/slus_006.64/system/rendering.c"
#endif
#include SPRITE_RENDER_SOURCE
#include "psx_memory.h"
#include "sprite_draw_retail_harness.h"

/* --- definitions of the externs the production TU expects --- */
uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];
void* g_GfxCurWorkBuffer; void* g_GfxCurWorkBufferEnd;
MATRIX D_8004FBB8;
s16 D_80059194, D_80059196;
u8 D_800591AD, D_800591AE;
void* D_800592E4; s16 D_800592E8, D_800592EA;
FieldActor* volatile g_FieldActors; s32 g_PlayerActorIndex;
void func_80022038(void* p) { (void)p; fputs("SPRITE DRAW FAIL unexpected func_80022038\n", stderr); abort(); }
void func_8002DDE4(void) { fputs("SPRITE DRAW FAIL unexpected func_8002DDE4\n", stderr); abort(); }
u_int HeapFree(void* p) { (void)p; fputs("SPRITE DRAW FAIL unexpected HeapFree\n", stderr); abort(); }
void* HeapAlloc(u_int size, u_int arg) { return (void*)(uintptr_t)svc_heap_alloc((uint32_t)size, (uint32_t)arg); }
void func_800234AC(void* sprite) { svc_note_800234AC((uint32_t)(uintptr_t)sprite); }
void func_800251C8(u_long* a, int x, int y, int w, int h) { svc_note_800251C8((uint32_t)(uintptr_t)a, (uint32_t)x, (uint32_t)y, (uint32_t)w, (uint32_t)h); }
int PcPort_WorldCaptureCurFrame(void) { return 0; }
/* Fei HD-2D presentation hooks are a port enhancement outside the retail
 * diff: pass-through (retail link path). */
void PcPort_FeiHd2dBegin(PcPortFeiHd2dBatch* b, void* s, int n) { (void)s; (void)n; b->active = b->count = b->hookState = 0; }
int PcPort_FeiHd2dCapture(PcPortFeiHd2dBatch* b, void* p, void* o) { (void)b; (void)p; (void)o; return 0; }
void PcPort_FeiHd2dEnd(PcPortFeiHd2dBatch* b) { (void)b; }
int PcPort_FeiHd2dEnabled(void) { return 0; }
void PcPort_FeiHd2dToggle(void) {}
char PsyX_BeginScene(void) { abort(); }

/* ---- fixture layout (offsets inside the low region) ---- */
enum { O_SPR = 0x0000, O_BASE = 0x0100, O_DIRS = 0x0200, O_PRIMS = 0x0400, O_PKG = 0x0800, O_VRAM = 0x0840,
       O_HOLD = 0x0860, O_TEXTBL = 0x0880, O_FRAMES = 0x1000, O_TILES = 0x1800, O_PACK = 0x3000, O_OT = 0x4100,
       O_XFER = 0x4200, O_ENDFIX = 0x4400 };
#define OT_ADDR (lo_addr(O_OT + 0x20))

static const char* label(uint32_t off, char* b)
{
    static const char* ft4[] = {"tag","tag","tag","len","rgb+code","rgb","rgb","code","x0","x0","y0","y0","u0","v0","clut","clut","x1","x1","y1","y1","u1","v1","tpage","tpage","x2","x2","y2","y2","u2","v2","pad","pad","x3","x3","y3","y3","u3","v3","pad","pad"};
    if (off >= O_PACK && off < O_PACK + 0x1000) { uint32_t j = (off - O_PACK) / 0x28, f = (off - O_PACK) % 0x28; sprintf(b, "packet[%u].%s byte %u", j, ft4[f], f); return b; }
    if (off >= O_PRIMS && off < O_PRIMS + 0x300) { uint32_t j = (off - O_PRIMS) / 24, f = (off - O_PRIMS) % 24; sprintf(b, "framePrim[%u]+0x%x", j, f); return b; }
    if (off >= O_SPR && off < O_SPR + 0x100) { sprintf(b, "sprite+0x%x", off - O_SPR); return b; }
    if (off >= O_BASE && off < O_BASE + 0x80) { sprintf(b, "base+0x%x", off - O_BASE); return b; }
    if (off >= O_DIRS && off < O_DIRS + 0x40) { sprintf(b, "dirXform[%u]+%u", (off - O_DIRS) / 8, (off - O_DIRS) % 8); return b; }
    if (off >= O_OT && off < O_OT + 0x40) { sprintf(b, "OT+0x%x", off - O_OT); return b; }
    if (off >= O_VRAM && off < O_VRAM + 0x20) { sprintf(b, "vramData+%u", off - O_VRAM); return b; }
    if (off >= HEAP_BASE_OFF) { sprintf(b, "heap+0x%x", off - HEAP_BASE_OFF); return b; }
    sprintf(b, "other"); return b;
}

/* ---- common sprite fill ---- */
static void fill_noise(void)
{
    for (uint32_t i = 0; i < LOW_SIZE; i++) low[i] = 0xA5;
    for (uint32_t i = O_SPR; i < O_SPR + 0x100; i++) low[i] = (uint8_t)rnd();
    for (uint32_t i = O_BASE; i < O_BASE + 0x80; i++) low[i] = (uint8_t)rnd();
}
static void random_matrix(int16_t m[9], unsigned mode)
{
    memset(m, 0, 18);
    static const int16_t sn[] = {0, 1024, 2048, -1024, 2896, -2896};
    switch (mode) {
    case 0: m[0] = m[4] = m[8] = 4096; break;                           /* identity: east/front */
    case 1: m[0] = -4096; m[4] = m[8] = 4096; break;                    /* mirrored X */
    case 2: m[0] = -4096; m[4] = 4096; m[8] = -4096; break;             /* rotY 180: west-facing */
    case 3: { int16_t s = sn[rn(6)], c = (int16_t)(4096 - (s * s) / 4096 / 2); m[0] = c; m[2] = s; m[4] = 4096; m[6] = (int16_t)-s; m[8] = c; } break;
    case 4: m[0] = m[4] = m[8] = (int16_t)rrange(1024, 6000); break;
    default: for (int i = 0; i < 9; i++) m[i] = (int16_t)rrange(-4096, 4096); break;
    }
}
static GteSetup gte_for_case(void)
{
    GteSetup g; memset(&g, 0, sizeof g);
    g.ofx = rn(2) ? (160 << 16) : (int32_t)rrange(0, 320) << 16; g.ofy = (112 << 16);
    static const uint32_t hs[] = {512, 300, 1000, 256, 0x1000};
    g.h = hs[rn(5)]; g.dqa = 0x100; g.dqb = 0x200000; g.zsf3 = 1365; g.zsf4 = 1024;
    g.rot[0] = g.rot[4] = g.rot[8] = 4096;
    return g;
}

static unsigned cases_by_fn[4];
static unsigned gte_diffs;
static const char* g_fn;
static unsigned g_case;
static char g_desc[160];
static void set_desc(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
#include <stdarg.h>
static void set_desc(const char* fmt, ...) { va_list ap; va_start(ap, fmt); vsnprintf(g_desc, sizeof g_desc, fmt, ap); va_end(ap); }

static void common_prerun(void)
{
    heap_next = lo_addr(HEAP_BASE_OFF); ncalls = 0; memset(calllog, 0, sizeof calllog);
    memcpy(low_init, low, LOW_SIZE);
}
static CallRec retail_calls[256]; static unsigned retail_ncalls; static uint32_t retail_heap;
static int finish_compare(const char* what, int check_gte, GTERegisters* retail_gte, uint32_t native_scratch_ok)
{
    (void)native_scratch_ok;
    if (diff_low(what, g_fn, g_case, g_desc, low, low_retail, label)) return 1;
    if (ncalls != retail_ncalls || memcmp(calllog, retail_calls, sizeof calllog) || heap_next != retail_heap) {
        if (!take_print()) return 1;
        fprintf(stderr, "%s FAIL %s case=%u [%s] boundary calls differ: native n=%u heap=%08x retail n=%u heap=%08x\n", what, g_fn, g_case, g_desc, ncalls, heap_next, retail_ncalls, retail_heap);
        for (unsigned i = 0; i < ncalls && i < retail_ncalls && i < 256; i++) if (memcmp(&calllog[i], &retail_calls[i], sizeof(CallRec)))
            fprintf(stderr, "  call %u native id=%u a=%x,%x,%x,%x,%x retail id=%u a=%x,%x,%x,%x,%x\n", i, calllog[i].id, calllog[i].a[0], calllog[i].a[1], calllog[i].a[2], calllog[i].a[3], calllog[i].a[4], retail_calls[i].id, retail_calls[i].a[0], retail_calls[i].a[1], retail_calls[i].a[2], retail_calls[i].a[3], retail_calls[i].a[4]);
        return 1;
    }
    if (check_gte && memcmp(&gteRegs, retail_gte, sizeof(gteRegs))) {
        gte_diffs++;
        if (gte_diffs <= 2) fprintf(stderr, "note: %s case=%u [%s] GTE register state differs (native vs interpreted libgte); packets equal\n", g_fn, g_case, g_desc);
    }
    return 0;
}

/* ====================== func_8001E3D8 ====================== */
static int e3d8_exec(unsigned id, uint32_t cur, uint32_t end, const GteSetup* gp)
{
    GteSetup g = *gp;
    common_prerun();
    /* retail */
    memset(g_PsxRam + 0x4fb98, 0x37, 32); memset(&s_QuadWork8004FB98, 0x37, sizeof s_QuadWork8004FB98);
    p32(0x80059580u, cur); p32(0x80059534u, end);
    gte_setup(&g);
    retail_call(0x8001e3d8u, lo_addr(O_SPR), OT_ADDR, 0, 0, "SPRITE DRAW");
    memcpy(low_retail, low, LOW_SIZE); GTERegisters rg = gteRegs;
    retail_ncalls = ncalls; memcpy(retail_calls, calllog, sizeof calllog); retail_heap = heap_next;
    uint32_t r_cur = g32(0x80059580u); uint8_t r_scr[32]; memcpy(r_scr, g_PsxRam + 0x4fb98, 32);
    /* native */
    memcpy(low, low_init, LOW_SIZE); heap_next = lo_addr(HEAP_BASE_OFF); ncalls = 0; memset(calllog, 0, sizeof calllog);
    g_GfxCurWorkBuffer = (void*)(uintptr_t)cur; g_GfxCurWorkBufferEnd = (void*)(uintptr_t)end;
    gte_setup(&g);
    func_8001E3D8(low + O_SPR, (void*)(uintptr_t)OT_ADDR);
    if ((uint32_t)(uintptr_t)g_GfxCurWorkBuffer != r_cur) { if (take_print()) fprintf(stderr, "SPRITE DRAW FAIL func_8001E3D8 case=%u [%s] work cursor native=%08x retail=%08x\n", id, g_desc, (uint32_t)(uintptr_t)g_GfxCurWorkBuffer, r_cur); return 1; }
    if (memcmp(&s_QuadWork8004FB98, r_scr, 32)) { if (take_print()) fprintf(stderr, "SPRITE DRAW FAIL func_8001E3D8 case=%u [%s] quad scratch 8004FB98 differs\n", id, g_desc); return 1; }
    return finish_compare("SPRITE DRAW", 1, &rg, 0);
}


static int e3d8_case(unsigned id)
{
    seed_case(id * 7 + 1);
    fill_noise();
    static const unsigned fcs[] = {0, 1, 1, 2, 3, 4, 5, 6, 8, 12, 24};
    unsigned fc = fcs[rn(11)];
    unsigned west = rn(10) < 4;           /* bias toward west/flip cases */
    unsigned shift = rn(4) ? 0 : rn(4);
    if (rn(30) == 0) shift = 4 + rn(3);
    uint32_t f3c = rnd() & 0xA7FFFFE7u;   /* random junk, bits 2,3,4,27 chosen below */
    f3c &= ~((1u << 2) | (1u << 3) | (1u << 4) | (1u << 27));
    unsigned flipx = west ? (rn(4) != 0) : rn(5) == 0, flipy = rn(6) == 0, depth = rn(4) == 0;
    f3c |= flipx << 3 | flipy << 4 | depth << 27 | (rn(2) << 2);
    if (rn(3) == 0) f3c &= ~0xFF00u; else if (rn(2)) f3c = (f3c & ~0xFF00u) | (rn(256) << 8);
    put32(low + O_SPR + 0x3C, f3c);
    put32(low + O_SPR + 0x40, (rnd() & ~0x3FFFu) | (shift << 8) | (fc << 2) | (rnd() & 3));
    put32(low + O_SPR + 0xAC, rnd() & ~4u | (rn(2) << 2));
    static const int16_t scales[] = {4096, 4096, 4096, 2048, 8192, -4096, 0x1800, 1, 0};
    put16(low + O_SPR + 0x2C, rn(5) ? (uint32_t)(uint16_t)scales[rn(9)] : rnd());
    put32(low + O_SPR + 0x20, lo_addr(O_BASE));
    /* model base: matrix (west modes), translation, prim/dir pointers, offsets */
    int16_t m[9]; random_matrix(m, west && rn(2) ? 2 : rn(6));
    for (int i = 0; i < 9; i++) put16(low + O_BASE + 0x0C + 2 * i, (uint16_t)m[i]);
    put32(low + O_BASE + 0x20, (uint32_t)rrange(-300, 300)); put32(low + O_BASE + 0x24, (uint32_t)rrange(-300, 300));
    put32(low + O_BASE + 0x28, rn(8) == 0 ? (uint32_t)rrange(-100, 700) : (uint32_t)rrange(500, 5000));
    put32(low + O_BASE + 0x30, lo_addr(O_PRIMS));
    unsigned dirmode = rn(3); /* 0 NULL, 1 all-zero table, 2 populated */
    put32(low + O_BASE + 0x34, dirmode ? lo_addr(O_DIRS) : 0);
    low[O_BASE + 0x3C] = (uint8_t)(rn(3) ? rrange(-20, 20) : rnd()); low[O_BASE + 0x3D] = (uint8_t)(rn(3) ? rrange(-20, 20) : rnd());
    for (int i = 0; i < 64; i++) low[O_DIRS + i] = dirmode == 1 ? 0 : (uint8_t)(rn(3) ? 0 : rnd());
    if (dirmode == 2) for (int d = 0; d < 8; d++) { if (rn(2)) { put16(low + O_DIRS + d * 8, rn(2) ? rnd() : 0); put16(low + O_DIRS + d * 8 + 2, rn(2) ? rnd() & 0xFFF : 0); put16(low + O_DIRS + d * 8 + 4, rn(2) ? (west ? 0x800 : rnd() & 0xFFF) : 0); put16(low + O_DIRS + d * 8 + 6, rn(2) ? rnd() & 0xFFF : 0); } }
    unsigned curdir = rn(8);
    for (unsigned i = 0; i < 24; i++) {
        uint8_t* p = low + O_PRIMS + i * 24;
        put16(p, (uint32_t)rrange(-90, 90)); put16(p + 2, (uint32_t)rrange(-120, 40));
        p[4] = rn(4) == 0 ? (uint8_t)rn(3) : (uint8_t)rnd(); p[5] = (uint8_t)rnd();
        p[6] = rn(10) == 0 ? (uint8_t)rn(3) : (uint8_t)rrange(1, 64); p[7] = (uint8_t)rrange(1, 64);
        p[8] = (uint8_t)(rn(2) ? 0 : rrange(-12, 12)); p[9] = (uint8_t)(rn(2) ? 0 : rrange(-12, 12));
        put16(p + 10, rnd()); put16(p + 12, rnd()); put32(p + 16, rn(2) ? 0x2C808080u : rnd());
        if (rn(3) == 0) curdir = rn(8);
        put32(p + 20, (rnd() & ~0x37u) | curdir | (rn(4) == 0) << 4 | (rn(5) == 0) << 5);
    }
    put32(low + O_OT + 0x20, rnd());
    uint32_t cur = lo_addr(O_PACK), end = cur + fc * 0x28 + (rn(8) ? 0x2000 : (uint32_t[]){0, (uint32_t)-1, 1}[rn(3)]);
    GteSetup g = gte_for_case();
    set_desc("fc=%u shift=%u f3c=%08x(flipX=%u flipY=%u depth=%u) flagsAC=%08x scale=%d matrix=%d,%d,%d dirmode=%u west=%u", fc, shift, f3c, flipx, flipy, depth, get32(low + O_SPR + 0xAC), (int)(int16_t)(low[O_SPR + 0x2C] | low[O_SPR + 0x2D] << 8), m[0], m[4], m[8], dirmode, west);
    return e3d8_exec(id, cur, end, &g);
}


/* Named, deterministic west/flip probes (owner bug: west-facing Fei). One
 * prim, identity-ish model matrix, only the stated feature enabled. */
static const char* const tgt_names[] = {
    "baseline (no direction transform)", "dirXform vz tilt only", "dirXform xy offset only, scale 3000 (negative operands)",
    "dirXform vx tilt only", "flip UV clamp u0=0 (texU-- branch)", "flip UV u0=5 (texU-- branch)",
    "dirXform vz tilt + xy offset together"
};
#define N_TGT 7
static int e3d8_targeted(unsigned v, unsigned west)
{
    seed_case(9000 + v * 2 + west); fill_noise();
    put32(low + O_SPR + 0x20, lo_addr(O_BASE));
    put32(low + O_SPR + 0x3C, west ? (1u << 3) : 0u);
    put32(low + O_SPR + 0x40, 1u << 2);
    put32(low + O_SPR + 0xAC, 0); put16(low + O_SPR + 0x2C, v == 2 || v == 6 ? 3000 : 4096); low[O_SPR + 0x3D] = 0;
    memset(low + O_BASE + 0x0C, 0, 18); put16(low + O_BASE + 0x0C, 4096); put16(low + O_BASE + 0x14, 4096); put16(low + O_BASE + 0x1C, 4096);
    put32(low + O_BASE + 0x20, 0); put32(low + O_BASE + 0x24, 0); put32(low + O_BASE + 0x28, 2000);
    put32(low + O_BASE + 0x30, lo_addr(O_PRIMS)); put32(low + O_BASE + 0x34, v == 0 || v >= 4 && v != 6 ? 0 : lo_addr(O_DIRS));
    low[O_BASE + 0x3C] = 0; low[O_BASE + 0x3D] = 0;
    memset(low + O_DIRS, 0, 64);
    if (v == 1 || v == 6) put16(low + O_DIRS + 6, 0x100);
    if (v == 2 || v == 6) { low[O_DIRS + 0] = (uint8_t)-5; low[O_DIRS + 1] = (uint8_t)-7; }
    if (v == 3) put16(low + O_DIRS + 2, 0x80);
    uint8_t* p = low + O_PRIMS; memset(p, 0, 24);
    put16(p, (uint32_t)-12); put16(p + 2, (uint32_t)-30); p[4] = v == 4 ? 0 : v == 5 ? 5 : 8; p[5] = 16; p[6] = 24; p[7] = 32;
    put32(p + 16, 0x2C808080u);
    put32(low + O_OT + 0x20, 0);
    GteSetup g; memset(&g, 0, sizeof g); g.ofx = 160 << 16; g.ofy = 112 << 16; g.h = 512; g.dqa = 0x100; g.dqb = 0x200000; g.zsf3 = 1365; g.zsf4 = 1024;
    g.rot[0] = g.rot[4] = g.rot[8] = 4096;
    g_fn = "func_8001E3D8"; g_case = 9000 + v * 2 + west;
    set_desc("TARGET %s, %s", tgt_names[v], west ? "WEST (flipX)" : "east");
    return e3d8_exec(g_case, lo_addr(O_PACK), lo_addr(O_PACK) + 0x2000, &g);
}

/* ====================== frame builders for DAE8 / D53C ====================== */
static uint8_t* sb; /* stream write cursor */
static void build_stream_part(unsigned wide)
{
    unsigned ncmd = rn(3);
    for (unsigned c = 0; c < ncmd; c++) {
        if (rn(2)) { /* direction command */
            unsigned cmd = 0xC0 | (rn(2) ? 0x20 : 0) | (rn(2) ? 0x10 : 0) | rn(8) | (rn(2) ? 0x08 : 0);
            *sb++ = (uint8_t)cmd; if (cmd & 0x20) { *sb++ = (uint8_t)rnd(); *sb++ = (uint8_t)rnd(); } if (cmd & 0x10) *sb++ = (uint8_t)rnd();
        } else {
            unsigned cmd = 0x80 | (rn(2) ? 4 : 0) | (rn(2) ? 1 : 0) | (rn(2) ? 2 : 0) | (rn(2) ? 0x20 : 0) | (rn(2) ? 0x10 : 0);
            *sb++ = (uint8_t)cmd; if (cmd & 1) *sb++ = (uint8_t)rnd(); if (cmd & 2) *sb++ = (uint8_t)rnd();
        }
    }
    *sb++ = (uint8_t)(rn(128) | (rn(2) ? 0x40 : 0));          /* terminator: bit7 clear; bit6=mirror X, abr, clut nibble */
    *sb++ = (uint8_t)rnd(); *sb++ = (uint8_t)rnd();
    if (wide) { *sb++ = (uint8_t)rnd(); *sb++ = (uint8_t)rnd(); }
}
/* returns frame offset within pFrames */
static void sprite_common_dae8(unsigned id, uint32_t* pkg_out, unsigned fmtB)
{
    (void)id; (void)fmtB;
    put32(low + O_SPR + 0x20, lo_addr(O_BASE)); put32(low + O_SPR + 0x24, lo_addr(O_VRAM));
    put32(low + O_SPR + 0x28, rn(2) ? 0x2C808080u : rnd());
    static const int16_t scales[] = {4096, 4096, 2048, 8192, -4096, 1, 0, 0x1234};
    put16(low + O_SPR + 0x2C, (uint16_t)scales[rn(8)]);
    uint32_t f3c = rnd() & ~(1u << 30); if (rn(4) == 0) f3c |= 1u << 30; put32(low + O_SPR + 0x3C, f3c);
    put16(low + O_SPR + 0x3E, rnd());
    uint32_t f40 = rnd(); if (rn(4) == 0) f40 = (f40 & ~(0xFu << 13)) | (0xEu << 13); else if (((f40 >> 13) & 0xF) == 0xE) f40 ^= 1u << 13;
    put32(low + O_SPR + 0x40, f40);
    { uint32_t a8 = rn(2) ? (rnd() | 1u) : (rnd() & ~1u); put32(low + O_SPR + 0xA8, a8);
      /* Retail derefs holder+0x18 whenever a8&1 without a NULL test (native guards it): keep holder valid then. */
      put32(low + O_SPR + 0x7C, ((a8 & 1) || rn(2)) ? lo_addr(O_HOLD) : 0); }
    put32(low + O_HOLD + 0x18, rn(4) ? lo_addr(O_TEXTBL) : 0);
    for (int i = 0; i < 0x40; i++) low[O_TEXTBL + i] = (uint8_t)rnd();
    put32(low + O_BASE + 0x30, lo_addr(O_PRIMS)); put32(low + O_BASE + 0x34, rn(3) ? lo_addr(O_DIRS) : 0);
    for (int i = 0; i < 0x300; i++) low[O_PRIMS + i] = (uint8_t)rnd();
    for (int i = 0; i < 0x40; i++) low[O_DIRS + i] = (uint8_t)rnd();
    put16(low + O_VRAM + 4, rnd() & (rn(3) ? 0x3FF : 0xFFFF)); put16(low + O_VRAM + 6, rnd());
    put32(low + O_PKG + 0, lo_addr(O_FRAMES)); put16(low + O_PKG + 4, rnd()); put16(low + O_PKG + 6, rnd());
    put16(low + O_PKG + 8, rnd()); put16(low + O_PKG + 0xA, rnd() & 0x1FF); put32(low + O_PKG + 0xC, lo_addr(O_XFER));
    put16(low + O_XFER, rn(3) ? rn(5) : 0); for (int i = 2; i < 0x100; i++) low[O_XFER + i] = (uint8_t)rnd();
    D_80059194 = (s16)rrange(0, 2); D_80059196 = (s16)rrange(0, 64);
    put16(g_PsxRam + 0x59194, (uint16_t)D_80059194); put16(g_PsxRam + 0x59196, (uint16_t)D_80059196);
    *pkg_out = lo_addr(O_PKG);
}
/* Build nframes frames; entry0 flagged for the 0x8000 (D53C) route when fmtB==2. */
static unsigned build_frames(unsigned fmtB, unsigned nframes, unsigned wide_all)
{
    uint8_t* fr = low + O_FRAMES; uint8_t* tiles = low + O_TILES; unsigned tile_off = O_TILES - O_FRAMES;
    uint8_t* fcur = fr + ((2 * nframes + 3) / 4) * 4;
    uint16_t tbl[8];
    for (unsigned f = 0; f < nframes; f++) {
        unsigned wide = wide_all ? 1 : rn(2), count = rn(5) ? rrange(1, 6) : (rn(2) ? 0 : 8);
        tbl[f] = (uint16_t)(fcur - fr);
        fcur[0] = (uint8_t)(wide << 7 | count); fcur[1] = (uint8_t)rnd(); fcur[2] = (uint8_t)rnd(); fcur[3] = (uint8_t)rnd(); fcur[4] = (uint8_t)rnd(); fcur[5] = (uint8_t)rnd();
        if (!fmtB) {
            uint8_t* d = fcur + 6; sb = fcur + 6 + count * 4;
            for (unsigned i = 0; i < count; i++) {
                put16(d + 4 * i, tile_off >> 2); put16(d + 4 * i + 2, rnd());
                uint8_t* t = low + O_FRAMES + tile_off; t[0] = (uint8_t)rn(60); t[1] = (uint8_t)rn(60); put16(t + 2, rnd());
                for (int k = 4; k < 12; k++) t[k] = (uint8_t)rnd();
                tile_off += 16; build_stream_part(wide);
            }
        } else {
            uint8_t* d = fcur + 4; sb = fcur + 4 + count * 2;
            for (unsigned i = 0; i < count; i++) {
                put16(d + 2 * i, tile_off);
                uint8_t* t = low + O_FRAMES + tile_off; unsigned ext = rn(2);
                t[0] = (uint8_t)(rnd() & 0xEF) | (ext ? 0x10 : 0); t[1] = (uint8_t)rnd(); t[2] = (uint8_t)rnd(); t[3] = (uint8_t)rn(64); t[4] = (uint8_t)rn(64); t[5] = (uint8_t)rn(64); t[6] = (uint8_t)rnd();
                tile_off += 8; build_stream_part(wide);
            }
        }
        fcur = low + O_FRAMES + ((unsigned)(sb - (low + O_FRAMES)) + 3u) / 4u * 4u; /* frames stay word aligned */
        if ((uint32_t)(fcur - low) > O_TILES - 16) { fprintf(stderr, "fixture overflow frames\n"); exit(2); }
        if ((uint32_t)(tile_off + O_FRAMES) > O_PACK - 64) { fprintf(stderr, "fixture overflow tiles\n"); exit(2); }
    }
    for (unsigned f = 0; f < nframes; f++) put16(fr + 2 * f, tbl[f]);
    if (fmtB == 2) put16(fr, 0x8000u | tbl[0]); /* NOTE: entry0 loses its offset, frame 0 is not callable via DAE8 */
    return nframes;
}

static int frame_case(unsigned id, int which /*0=DAE8 main,1=D53C direct,2=DAE8->D53C route*/)
{
    seed_case(id * 11 + 3 + which * 1000);
    fill_noise();
    uint32_t pkg; unsigned nframes = rrange(2, 4);
    sprite_common_dae8(id, &pkg, which != 0);
    build_frames(which == 0 ? 0 : (which == 1 ? 1 : 2), nframes, 0);
    unsigned frame = which == 2 ? rrange(1, (int)nframes - 1) : rn(nframes);
    if (which == 0 && rn(12) == 0) frame = (low[O_FRAMES] | low[O_FRAMES + 1] << 8) + 1 + rn(3);   /* beyond the (hdr&0x1FF)+1 bound: early return */
    g_fn = which == 0 ? "func_8001DAE8" : which == 1 ? "func_8001D53C" : "func_8001DAE8(->D53C)"; g_case = id;
    set_desc("%s frame=%u/%u f40=%08x f3c=%08x vram=(%04x,%04x) pkg+4=%04x dirNULL=%u", which == 0 ? "DAE8" : which == 1 ? "D53C" : "DAE8->D53C", frame, nframes, get32(low + O_SPR + 0x40), get32(low + O_SPR + 0x3C), low[O_VRAM + 4] | low[O_VRAM + 5] << 8, low[O_VRAM + 6] | low[O_VRAM + 7] << 8, low[O_PKG + 4] | low[O_PKG + 5] << 8, get32(low + O_BASE + 0x34) == 0);
    common_prerun();
    uint16_t sv94 = (uint16_t)D_80059194, sv96 = (uint16_t)D_80059196;
    retail_call(which == 1 ? 0x8001d53cu : 0x8001dae8u, lo_addr(O_SPR), frame, pkg, 0, "SPRITE DRAW");
    memcpy(low_retail, low, LOW_SIZE);
    retail_ncalls = ncalls; memcpy(retail_calls, calllog, sizeof calllog); retail_heap = heap_next;
    uint8_t rg[4]; memcpy(rg, g_PsxRam + 0x59194, 4);
    memcpy(low, low_init, LOW_SIZE); heap_next = lo_addr(HEAP_BASE_OFF); ncalls = 0; memset(calllog, 0, sizeof calllog);
    D_80059194 = (s16)sv94; D_80059196 = (s16)sv96;
    if (which == 1) func_8001D53C(low + O_SPR, (u16)frame, low + O_PKG); else func_8001DAE8(low + O_SPR, (u16)frame, pkg);
    g_fn = which == 0 ? "func_8001DAE8" : which == 1 ? "func_8001D53C" : "func_8001DAE8(->D53C)"; g_case = id;
    uint8_t ng[4]; put16(ng, (uint16_t)D_80059194); put16(ng + 2, (uint16_t)D_80059196);
    if (memcmp(ng, rg, 4)) { if (take_print()) fprintf(stderr, "SPRITE DRAW FAIL %s case=%u [%s] atlas cursor globals D_80059194/6 native=%d,%d retail=%d,%d\n", g_fn, id, g_desc, (int16_t)(ng[0] | ng[1] << 8), (int16_t)(ng[2] | ng[3] << 8), (int16_t)(rg[0] | rg[1] << 8), (int16_t)(rg[2] | rg[3] << 8)); return 1; }
    return finish_compare("SPRITE DRAW", 0, 0, 0);
}

/* ====================== func_8001E9BC ====================== */
static int e9bc_case(unsigned id)
{
    seed_case(id * 13 + 5);
    fill_noise();
    unsigned fc = (unsigned[]){0, 1, 2, 5, 12}[rn(5)]; unsigned shift = rn(3) ? 0 : rn(4);
    put32(low + O_SPR + 0x20, lo_addr(O_BASE)); put32(low + O_BASE + 0x30, lo_addr(O_PRIMS));
    put32(low + O_SPR + 0x3C, (rnd() & ~0xFF18u) | (rn(2) << 3) | (rn(2) << 4) | (rn(3) ? 0 : rn(256) << 8));
    put32(low + O_SPR + 0x40, (rnd() & ~0x3FFFu) | shift << 8 | fc << 2 | (rnd() & 3));
    put16(low + O_SPR + 2, rrange(-300, 300)); put16(low + O_SPR + 6, rrange(-100, 100)); put16(low + O_SPR + 10, rrange(-300, 300));
    put16(low + O_SPR + 0x84, rrange(-200, 200)); put16(low + O_SPR + 0x2C, (uint16_t)(int16_t[]){4096, 2048, 8192, -4096, 1, 0}[rn(6)]);
    for (unsigned i = 0; i < 12; i++) {
        uint8_t* p = low + O_PRIMS + i * 24;
        put16(p, rrange(-60, 60)); put16(p + 2, rrange(-60, 60)); p[4] = (uint8_t)rnd(); p[5] = (uint8_t)rnd(); p[6] = (uint8_t)rrange(1, 40); p[7] = (uint8_t)rrange(1, 40);
        p[8] = (uint8_t)rrange(-8, 8); p[9] = (uint8_t)rrange(-8, 8); put16(p + 10, rnd()); put16(p + 12, rnd());
        put32(p + 20, (rnd() & ~0x37u) | rn(8) | (rn(2) << 4) | (rn(2) << 5));
    }
    put32(low + O_OT + 0x20, rnd());
    int16_t m[9]; random_matrix(m, rn(6)); memset(&D_8004FBB8, 0, sizeof D_8004FBB8);
    for (int i = 0; i < 9; i++) ((int16_t*)D_8004FBB8.m)[i] = m[i];
    D_8004FBB8.t[0] = rrange(-200, 200); D_8004FBB8.t[1] = rrange(-200, 200); D_8004FBB8.t[2] = rrange(500, 4000);
    uint32_t cur = lo_addr(O_PACK), end = cur + fc * 0x28 + (rn(6) ? 0x2000 : (uint32_t[]){0, (uint32_t)-1, 1}[rn(3)]);
    GteSetup g = gte_for_case(); g_fn = "func_8001E9BC"; g_case = id;
    set_desc("fc=%u shift=%u scale=%d mat0=%d t=%d,%d,%d", fc, shift, (int)(int16_t)(low[O_SPR + 0x2C] | low[O_SPR + 0x2D] << 8), m[0], D_8004FBB8.t[0], D_8004FBB8.t[1], D_8004FBB8.t[2]);
    common_prerun();
    memcpy(g_PsxRam + 0x4fbb8, &D_8004FBB8, 32); memset(g_PsxRam + 0x4fad8, 0x27, 32); memset(&s_ShadowQuad8004FAD8, 0x27, 32);
    p32(0x80059580u, cur); p32(0x80059534u, end);
    gte_setup(&g);
    retail_call(0x8001e9bcu, lo_addr(O_SPR), OT_ADDR, 0, 0, "SPRITE DRAW");
    memcpy(low_retail, low, LOW_SIZE); GTERegisters rg = gteRegs;
    retail_ncalls = ncalls; memcpy(retail_calls, calllog, sizeof calllog); retail_heap = heap_next;
    uint32_t r_cur = g32(0x80059580u); uint8_t r_scr[32]; memcpy(r_scr, g_PsxRam + 0x4fad8, 32);
    memcpy(low, low_init, LOW_SIZE);
    g_GfxCurWorkBuffer = (void*)(uintptr_t)cur; g_GfxCurWorkBufferEnd = (void*)(uintptr_t)end;
    gte_setup(&g);
    func_8001E9BC(low + O_SPR, (void*)(uintptr_t)OT_ADDR);
    if ((uint32_t)(uintptr_t)g_GfxCurWorkBuffer != r_cur) { if (take_print()) fprintf(stderr, "SPRITE DRAW FAIL func_8001E9BC case=%u [%s] work cursor native=%08x retail=%08x\n", id, g_desc, (uint32_t)(uintptr_t)g_GfxCurWorkBuffer, r_cur); return 1; }
    if (memcmp(&s_ShadowQuad8004FAD8, r_scr, 32)) { if (take_print()) fprintf(stderr, "SPRITE DRAW FAIL func_8001E9BC case=%u [%s] shadow quad scratch differs\n", id, g_desc); return 1; }
    return finish_compare("SPRITE DRAW", 1, &rg, 0);
}

static const char* ctx_str(void) { static char b[260]; snprintf(b, sizeof b, "%s case=%u [%s]", g_fn, g_case, g_desc); return b; }
int main(int argc, char** argv)
{
    ctx_fn = ctx_str;
    unsigned n = argc > 1 ? (unsigned)atoi(argv[1]) : 3000;
    host_rotmatrix = getenv("XENO_SDR_HOST_ROTMATRIX") && getenv("XENO_SDR_HOST_ROTMATRIX")[0] == '1';
    load_slus("SPRITE DRAW"); map_low();
    unsigned pass[6] = {0}, fail[6] = {0};
    static const char* const names[6] = {"func_8001E3D8", "func_8001E3D8(west/flip targets)", "func_8001DAE8", "func_8001D53C", "func_8001DAE8->D53C route", "func_8001E9BC"};
    prints_left = 2;
    for (unsigned i = 0; i < n; i++) { g_fn = names[0]; g_case = i; if (e3d8_case(i)) fail[0]++; else pass[0]++; }
    prints_left = 14;
    for (unsigned v = 0; v < N_TGT; v++) for (unsigned w = 0; w < 2; w++) { if (e3d8_targeted(v, w)) fail[1]++; else pass[1]++; }
    prints_left = 2;
    for (unsigned i = 0; i < n / 3; i++) { if (frame_case(i, 0)) fail[2]++; else pass[2]++; }
    prints_left = 2;
    for (unsigned i = 0; i < n / 3; i++) { if (frame_case(i, 1)) fail[3]++; else pass[3]++; }
    prints_left = 2;
    for (unsigned i = 0; i < n / 6; i++) { if (frame_case(i, 2)) fail[4]++; else pass[4]++; }
    prints_left = 2;
    for (unsigned i = 0; i < n / 4; i++) { if (e9bc_case(i)) fail[5]++; else pass[5]++; }
    unsigned bad = 0;
    for (unsigned i = 0; i < 6; i++) { printf("SPRITE DRAW %-34s %s pass=%u fail=%u\n", names[i], fail[i] ? "DIVERGES" : "PASS    ", pass[i], fail[i]); bad += fail[i]; }
    printf("SPRITE DRAW %s (retail RotMatrix %s); GTE-register-only diffs=%u (informational)\n", bad ? "RED" : "GREEN", host_rotmatrix ? "replaced by shared host RotMatrix" : "interpreted", gte_diffs);
    return bad ? 1 : 0;
}
