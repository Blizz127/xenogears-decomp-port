/* Retail differential for hand-written menu functions in the production
 * src/menu/main/misc.c TU (XENO_PC_PORT arms).  The production TU is linked
 * unmodified.  For every case both sides start from byte-identical guest
 * memory:
 *   retail: the pinned disc/menu.bin bytes (loaded at 0x801C5000) executed on
 *           PcPortMipsRun;
 *   native: the port C body.  The guest windows are identity-mapped at their
 *           guest addresses; the pointer-inflated host SystemMenu and the
 *           host-layout card buffer (TIM_IMAGE holds pointers, so every card
 *           byte from retail +0xB94 sits 0x10 later) are rebuilt from the
 *           retail image field by field and translated back for comparison.
 * Callees that leave the menu overlay (or whose result is injected) are
 * capturing spies on both sides; call order, arguments and the return feed
 * are compared with every byte of every touched window and the callee-saved
 * registers.  Callees that live inside the TU and image run as real code on
 * both sides.  No retail bytes are stored in the tree. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include "battle_mips_adapter.h"
#include "menu_retail_layout.h"

#define MENU_BASE 0x801C5000u
#define MENU_SIZE 153864u
#define STACK 0x801FF000u
#define HALT 0xFFFFFFFCu

/* ---- guest windows (byte-identical at start, compared at end) ---------- */
#define BUF_BASE 0x80100000u
#define BUF_SIZE 0x60000u
#define HEAP_BASE 0x80152000u /* guest heap handed out by the HeapAlloc stand-in */
#define HEAP_END 0x80160000u
#define SEDST 0x80102800u    /* SoundFile (g_Menu->unk2E4) */
#define GM 0x80100000u      /* retail SystemMenu (g_Menu = 0x800625A0 -> GM) */
#define GM_SIZE 0x1E98u
#define MGR 0x80102000u     /* MenuManager, all u8: same layout both sides */
#define CURS 0x80103000u    /* MenuPointerCursors */
#define GFX 0x80104000u     /* GfxEnvironment (ot at +0x70) */
#define WIN 0x80108000u     /* MenuWindow x7, stride 0x800 */
#define SLOT 0x80110000u    /* title slot buffers x32, stride 0x400 */
#define CARD 0x80120000u    /* MenuUnk2 window */
#define CARDW 0x30000u
#define RC_SIZE 0x5034u     /* retail sizeof(MenuUnk2) */
#define PAYLOAD 0x80150000u /* save payload buffer */
#define NWIN 7u
#define NSLOT 32u
#define CARD_DELTA 0x10u
#define P_GMENU 0x800625A0u

#define RA_BASE 0x80059400u /* D_80059460, D_80059488 */
#define RA_SIZE 0x100u
#define RB_BASE 0x8005A380u /* D_8005A3A0 (u16[16]) */
#define RB_SIZE 0x40u
#define RGS_BASE 0x8006D634u /* g_GameState */
#define RGS_SIZE 0x2300u
#define RC_BASE 0x8006F940u /* D_8006F958 (u16[16]) */
#define RC_SIZE2 0x60u
#define RM_BASE 0x801E9600u /* migrated menu data / bss */
#define RM_SIZE 0x1308u
#define RD_BASE 0x801C50A0u /* overlay rodata: card path prefixes, header template */
#define RD_SIZE 0x100u

#define STR2(x) #x
#define STR(x) STR2(x)
#define REGION(id) uint8_t id[id##_SIZE] __attribute__((aligned(16), used))
#define ALIAS(sym, reg, addr) \
    __asm__(".globl " #sym "\n.set " #sym ", " #reg "+(" STR(addr) "-" STR(reg##_BASE) ")")
#define RC2_SIZE RC_SIZE2
#define RC2_BASE RC_BASE
REGION(RA); REGION(RB); REGION(RGS); REGION(RC2); REGION(RM); REGION(RD);
ALIAS(D_80059460, RA, 0x80059460); ALIAS(D_80059488, RA, 0x80059488);
ALIAS(D_8005A3A0, RB, 0x8005A3A0);
ALIAS(g_GameState, RGS, 0x8006D634);
ALIAS(D_8006F958, RC2, 0x8006F958);
ALIAS(D_801E96A5, RM, 0x801E96A5); ALIAS(D_801E9778, RM, 0x801E9778);
ALIAS(D_801E9779, RM, 0x801E9779); ALIAS(D_801E977A, RM, 0x801E977A);
ALIAS(D_801E981C, RM, 0x801E981C); ALIAS(D_801E9820, RM, 0x801E9820);
ALIAS(D_801C50A8, RD, 0x801C50A8); ALIAS(D_801C50AC, RD, 0x801C50AC); ALIAS(D_801C50B0, RD, 0x801C50B0);
ALIAS(D_801C50B4, RD, 0x801C50B4); ALIAS(D_801C50B8, RD, 0x801C50B8); ALIAS(D_801C50E0, RD, 0x801C50E0);
ALIAS(D_801EA6D0, RM, 0x801EA6D0); ALIAS(D_801EA6F4, RM, 0x801EA6F4);
ALIAS(D_801EA6F8, RM, 0x801EA6F8); ALIAS(D_801EA8FC, RM, 0x801EA8FC);
ALIAS(D_801EA900, RM, 0x801EA900); ALIAS(D_801EA904, RM, 0x801EA904);
uint8_t g_PsxRam[64]; /* only reached through the NULL arm of MENU_GUEST_PTR */
ALIAS(D_801E9768, RM, 0x801E9768); ALIAS(D_801E9894, RM, 0x801E9894); ALIAS(D_801E9914, RM, 0x801E9914);
/* g_Menu is a host pointer natively and a u32 guest word in the retail image. */
void* g_Menu;

typedef struct { uint32_t base, size; uint8_t* nat; int randomize; } Region;
static const Region regions[] = {
    { RA_BASE, RA_SIZE, RA, 1 }, { RB_BASE, RB_SIZE, RB, 1 }, { RGS_BASE, RGS_SIZE, RGS, 1 },
    { RC_BASE, RC_SIZE2, RC2, 1 }, { RM_BASE, RM_SIZE, RM, 0 }, { RD_BASE, RD_SIZE, RD, 0 },
};
#define NREG (sizeof(regions) / sizeof(regions[0]))

/* ---- machinery ---------------------------------------------------------- */
static uint8_t ram[0x200000], *natbuf;
static uint8_t menu_image[MENU_SIZE];
static uint8_t SM[HO_SM_SIZE] __attribute__((aligned(16)));
static unsigned cases, checks;
static const char* cur_name;
static PcPortMipsCpu* active_cpu;
static uint32_t cov_lo, cov_hi;
static uint8_t cov[4096];
static FILE* out;

static void fail(const char* why) {
    fprintf(stderr, "MENU CARD FLOW RETAIL FAIL fn=%s case=%u %s\n", cur_name, cases, why);
    fprintf(out, "MENU CARD FLOW RETAIL FAIL fn=%s case=%u %s\n", cur_name, cases, why);
    fflush(out);
    exit(1);
}
static void require(int ok, const char* why) { checks++; if (!ok) fail(why); }
static uint8_t* gp(uint32_t a) { return ram + (a & 0x1FFFFFu); }
static uint32_t r32(uint32_t a) { uint32_t v; memcpy(&v, gp(a), 4); return v; }
static void w32(uint32_t a, uint32_t v) { memcpy(gp(a), &v, 4); }
static void w8(uint32_t a, uint8_t v) { *gp(a) = v; }

static uint64_t rng_s = 0x9E3779B97F4A7C15ull;
static uint32_t rnd(void) {
    rng_s ^= rng_s << 13; rng_s ^= rng_s >> 7; rng_s ^= rng_s << 17;
    return (uint32_t)(rng_s >> 16);
}
static uint32_t rnd_range(uint32_t n) { return rnd() % n; }
__attribute__((unused)) static int32_t rnd_val(void) {
    static const int32_t spec[] = { 0, 1, -1, 2, 3, 0x7F, 0x80, 0xFF, 0x100, 0x7FFF, 0x8000, 0xFFFF,
                                    0x10000, 0x7FFFFFFF, (int32_t)0x80000000, 0x12345, -0x8000, -0x100, 255, 256 };
    return (rnd() & 1) ? spec[rnd_range(sizeof(spec) / sizeof(spec[0]))] : (int32_t)rnd();
}
static int32_t small(int32_t lo, int32_t hi) { return lo + (int32_t)rnd_range((uint32_t)(hi - lo + 1)); }

/* ---- native/retail address views ---------------------------------------- */
/* Native card byte at retail offset `off`. */
static uint32_t card_noff(uint32_t off) { return off >= 0xB94u ? off + CARD_DELTA : off; }
static uint8_t* cardb_r(uint32_t off) { return gp(CARD + off); }
static uint8_t* cardb_n(uint32_t off) { return natbuf + (CARD - BUF_BASE) + card_noff(off); }
/* Normalise a pointer into a retail-view address (identity for everything but
 * the host-layout card window and the 8-byte-stride ot[] array). */
static uint32_t nrm_n(uint64_t p) {
    uint32_t a = (uint32_t)p;
    if (a >= CARD + 0xBA4u && a < CARD + CARDW) return a - CARD_DELTA;
    if (a >= GFX + 0x70u && a < GFX + 0x70u + 0x80u) return 0xF0000000u | ((a - (GFX + 0x70u)) / (HO_ULONG_SIZE));
    return a;
}
static uint32_t nrm_r(uint32_t a) {
    if (a >= GFX + 0x70u && a < GFX + 0x70u + 0x40u) return 0xF0000000u | ((a - (GFX + 0x70u)) / 4);
    return a;
}

/* ---- boundary spies ----------------------------------------------------- */
enum { S_C7BF4 = 1, S_CLUT, S_DEC, S_ENC, S_E4A28, S_DISC, S_2675C, S_93A8, S_CA750, S_CACF8, S_C8CA4,
       S_C8574, S_D2F4C, S_D32B4, S_C72BC, S_CB28C, S_OPEN, S_READ, S_WRITE, S_CLOSE, S_FORMAT, S_RENAME,
       S_ERASE, S_HALLOC, S_HFREE, S_VSYNC, S_SETPOLY, S_SND, S_90B0, S_E78C8, S_D9B08,
       S_E4D10, S_FIRSTFILE, S_NEXTFILE, S_CARDINFO, S_TESTEV, S_UNDELIV, S_CLOSEEV, S_ENTERCS, S_EXITCS, NSPY };
#define MAXEV 6000
typedef struct { uint32_t id, a[7]; } Event;
static struct { Event ev[MAXEV]; unsigned n; } trace;
static uint32_t feedr[NSPY][64];
static unsigned fidx[NSPY];
/* per-case return-value profile (identical on both sides) */
static struct { unsigned failpct, wfail, ca750cap, yes, readmode, hdr3, thr326; } P;
static uint32_t heap_ptr, heap_last, heap_top;
static uint32_t gs_digest(uint8_t* gs);
#define STACKTAG 0xBADC0DE0u

/* world hook state (func_801C7BF4 stand-in for the card scan) */
static struct { unsigned tick; uint8_t inp[256]; uint8_t f0[256]; uint8_t f1[256]; unsigned n; } world;
static int native_side;

static uint32_t dig(const uint8_t* p, unsigned n) {
    uint32_t h = 2166136261u;
    for (unsigned i = 0; i < n; i++) { h ^= p[i]; h *= 16777619u; }
    return h;
}
static uint32_t sdig(const void* s) { /* NUL-terminated string digest (length folded in) */
    const uint8_t* p = s;
    unsigned n = 0;
    while (n < 0x100 && p[n]) n++;
    return dig(p, n) ^ (n * 0x9E3779B1u);
}
static uint32_t wdig(const void* p, int32_t len) { return (len <= 0 || len > 0x4000) ? 0 : dig(p, (unsigned)len); }
/* normalised pointer views: host stack and guest stack both map to STACKTAG */
static uint32_t pn(const void* q) {
    uint64_t p = (uintptr_t)q;
    if (p >= 0x80000000ull && p < 0x80200000ull) return nrm_n(p);
    return STACKTAG;
}
static uint32_t pr(uint32_t a) {
    if (a >= STACK - 0x2000u && a <= STACK + 0x20u) return STACKTAG;
    return nrm_r(a);
}

static void spy(uint32_t id, uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5, uint32_t a6) {
    require(trace.n < MAXEV, "spy count");
    trace.ev[trace.n++] = (Event){ id, { a0, a1, a2, a3, a4, a5, a6 } };
}
/* Return value injected into the code under test; same stream on both sides. */
static int32_t draw(uint32_t id, uint32_t a0, uint32_t a2) {
    (void)a0;
    unsigned k = fidx[id]++;
    uint32_t r = feedr[id][k & 63];
    unsigned fail = (r % 100) < P.failpct;
    switch (id) {
    case S_93A8: return (r % 9 == 0) ? 1 : (r % 13 == 0) ? (int32_t)(r | 0x100u) : (r % 17 == 0) ? 0x100 : 0;
    case S_CA750: {
        if (k >= P.ca750cap) return 2;
        unsigned v = (r >> 4) % 10;
        return v < 5 ? 1 : v < 7 ? 2 : v == 7 ? 0 : v == 8 ? 3 : (int32_t)((r >> 12) & 7);
    }
    case S_CACF8: if (r % 40 == 0) return (int32_t)((r >> 8) & 0xFF);
                  return ((r >> 4) % 100) < P.yes ? 1 : 0;
    case S_OPEN: return fail ? (((r >> 8) & 3) == 3 ? 0 : -1) : 1 + (int32_t)((r >> 8) % 3);
    case S_READ: case S_WRITE:
        if (id == S_WRITE && (r % 100) < P.wfail) return -1;
        if (!fail) return (int32_t)a2;
        switch ((r >> 8) % 3) { case 0: return -1; case 1: return 0; default: return (int32_t)a2 - 1; }
    case S_RENAME: return fail ? 0 : 1;
    case S_FORMAT: return (fail || ((r >> 16) % 4 == 0)) ? (((r >> 8) & 1) ? 0 : -1) : 1;
    case S_ERASE: case S_CLOSE: return (int32_t)(r & 1);
    case S_FIRSTFILE: return fail ? 0 : 1;
    case S_NEXTFILE: return (r % 5) != 0;
    case S_CARDINFO: return (r % 5 == 0) ? 0 : (r % 7 == 0) ? (int32_t)r : 1;
    case S_TESTEV: return (r % 3) == 0;
    case S_CLUT: return (int32_t)(r & 0xFFFFu);
    case S_DISC: return (int32_t)(r % 4);
    default: return (int32_t)r;
    }
}
static int32_t spy_ret(uint32_t id, uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5, uint32_t a6) {
    spy(id, a0, a1, a2, a3, a4, a5, a6);
    return draw(id, a0, a2);
}

static void world_step(void) {
    unsigned t = world.tick++;
    uint8_t in = t < world.n ? world.inp[t] : 4 /* MENU_INPUT_CONFIRM */;
    uint8_t d326 = t >= P.thr326 ? 1 : 0;
    if (native_side) {
        SM[HO_SystemMenu_input] = in;
        if (d326) SM[HO_SystemMenu_input + 1] = 1; /* unk326 follows input in the byte run */
        if (t < world.n) { *cardb_n(0x4FE4) = world.f0[t]; *cardb_n(0x4FE5) = world.f1[t]; }
    } else {
        w8(GM + 0x325, in);
        if (d326) w8(GM + 0x326, 1);
        if (t < world.n) { *cardb_r(0x4FE4) = world.f0[t]; *cardb_r(0x4FE5) = world.f1[t]; }
    }
}
/* deterministic decode/encode stand-ins (act on the stack buffers) */
static void dec_fill(uint8_t* o, const uint8_t* in, unsigned n) {
    for (unsigned i = 0; i < 0x14; i++) o[i] = (uint8_t)((i < n * 2 ? in[i] : 0x5A) * 7 + i + n);
}
static void enc_fill(uint8_t* o, const uint8_t* in) {
    for (unsigned i = 0; i < 0x14; i++) o[i] = (uint8_t)(in[i] * 5 + 0x33 + i);
}
/* card file read: fills the destination deterministically (zeros keep the
 * Load checksum valid; mode 2 patches the last byte to make random data valid) */
static void read_fill(uint8_t* p, uint32_t ga, int32_t len) {
    if (len <= 0 || len > 0x4000) return;
    uint32_t seed = fidx[S_READ] * 0x9E3779B1u;
    for (int32_t i = 0; i < len; i++)
        p[i] = P.readmode == 0 ? 0 : (uint8_t)((seed + (uint32_t)i * 0x45D9F3Bu) >> 13);
    if (len == 0x200) p[3] = (uint8_t)P.hdr3;
    if (P.readmode == 2 && len == 0x100 && heap_last && ga == heap_last + 0x1F00u) {
        const uint8_t* base = p - 0x1F00;
        uint8_t sum = 0;
        for (unsigned i = 0x100; i < 0x1FFF; i++) sum = (uint8_t)(sum + (i >= 0x1F00 ? p[i - 0x1F00] : base[i]));
        p[0xFF] = sum;
    }
}
static int32_t do_read(uint32_t fd, uint32_t np, uint8_t* host, uint32_t ga, int32_t len) {
    int32_t r = spy_ret(S_READ, fd, np, (uint32_t)len, 0, 0, 0, 0);
    read_fill(host, ga, len);
    return r;
}
static uint32_t do_halloc(uint32_t size, uint32_t flags) {
    spy(S_HALLOC, size, flags, 0, 0, 0, 0, 0);
    uint32_t p = heap_ptr;
    heap_ptr += (size + 7u) & ~7u;
    require(heap_ptr <= HEAP_END, "heap exhausted");
    if (size == 0x2100u) heap_last = p;
    heap_top = p;
    return p;
}
static void do_hfree(uint32_t p, uint32_t np) { /* LIFO release keeps the long-running loops inside the window */
    spy(S_HFREE, np, 0, 0, 0, 0, 0, 0);
    if (p == heap_top && p >= HEAP_BASE) heap_ptr = p;
}

/* native spy implementations (names as the TU declares them) */
/* every menu frame records the card state byte (+0x4FE6) and the menu-sound flag (unk32A) */
void func_801C7BF4(void) { spy(S_C7BF4, *cardb_n(0x4FE6), SM[HO_SystemMenu_unk32A], 0, 0, 0, 0, 0); world_step(); }
unsigned short GetClut(int x, int y) { return (unsigned short)spy_ret(S_CLUT, (uint32_t)x, (uint32_t)y, 0, 0, 0, 0, 0); }
void func_80033B34(void* in, void* o, int n) {
    spy(S_DEC, dig(in, (unsigned)n * 2), (uint32_t)n, 0, 0, 0, 0, 0);
    dec_fill(o, in, (unsigned)n);
}
void func_80033C20(void* in, void* o) {
    spy(S_ENC, dig(in, 0x14), 0, 0, 0, 0, 0, 0);
    enc_fill(o, in);
}
void func_801E4A28(void* buf) {
    spy(S_E4A28, nrm_n((uintptr_t)buf), gs_digest(RGS), 0, 0, 0, 0, 0);
    uint32_t d = gs_digest(RGS);
    memcpy((uint8_t*)buf + 0x30, &d, 4);
}
int ArchiveGetDiscNumber(void) { return spy_ret(S_DISC, 0, 0, 0, 0, 0, 0, 0); }
int func_8002675C(void* a, int b, void* c, int d, int e, int f, int g) {
    return spy_ret(S_2675C, (uint32_t)(uintptr_t)a, (uint32_t)b, nrm_n((uintptr_t)c), (uint32_t)d, (uint32_t)e, (uint32_t)f, (uint32_t)g);
}
void func_80039E60(int packed) { spy(S_SND, (uint32_t)packed, 0, 0, 0, 0, 0, 0); }
void SetPolyF4(void* p) {
    spy(S_SETPOLY, pn(p), 0, 0, 0, 0, 0, 0);
    ((uint8_t*)p)[3] = 4; ((uint8_t*)p)[7] = 0x28;
}
void* HeapAlloc(unsigned size, unsigned flags) { return (void*)(uintptr_t)do_halloc(size, flags); }
void HeapFree(void* p) { do_hfree((uint32_t)(uintptr_t)p, pn(p)); }
int Vsync(int mode) { return spy_ret(S_VSYNC, (uint32_t)mode, 0, 0, 0, 0, 0, 0); }
/* in-overlay callees of the card flows (their own bodies are not under test here) */
#ifndef FLOW_REAL_SCAN /* the scan itself is under test in the second binary */
int func_801C93A8(void) { return spy_ret(S_93A8, 0, 0, 0, 0, 0, 0, 0); }
#endif
#ifndef FLOW_REAL_IO
void func_801C90B0(unsigned char port, unsigned char slot) { spy(S_90B0, port, slot, 0, 0, 0, 0, 0); }
#endif
void func_801E78C8(int index) { spy(S_E78C8, (uint32_t)index, 0, 0, 0, 0, 0, 0); }
void func_801D9B08(void) { spy(S_D9B08, 0, 0, 0, 0, 0, 0, 0); }
int func_801CA750(int dir, int unused, int limit) {
    (void)unused; (void)limit;
    int32_t r = spy_ret(S_CA750, (uint32_t)dir, 0, 0, 0, 0, 0, 0);
    world_step(); /* the cursor reader runs menu frames: pad and card state move on */
    return r;
}
unsigned char func_801CACF8(unsigned char a, unsigned char b, unsigned char c) {
    return (unsigned char)spy_ret(S_CACF8, a, b, c, 0, 0, 0, 0);
}
#ifndef FLOW_REAL_IO /* the card I/O helpers are under test in the third binary */
void func_801C8CA4(unsigned char idx) { spy(S_C8CA4, idx, 0, 0, 0, 0, 0, 0); }
#endif
void func_801C8574(int id) { spy(S_C8574, (uint32_t)id, 0, 0, 0, 0, 0, 0); }
void func_801D2F4C(unsigned char s) { spy(S_D2F4C, s, 0, 0, 0, 0, 0, 0); }
int func_801D32B4(int a) { (void)a; return spy_ret(S_D32B4, 0, 0, 0, 0, 0, 0, 0); }
void func_801C72BC(int mode) { spy(S_C72BC, (uint32_t)mode, 0, 0, 0, 0, 0, 0); }
#ifndef FLOW_REAL_IO
void func_801CB28C(int p) {
    uint32_t ga = (uint32_t)p;
    spy(S_CB28C, pn((const void*)(uintptr_t)ga), dig((const uint8_t*)(uintptr_t)ga, 0x1EFF), 0, 0, 0, 0, 0);
}
#endif
void* func_801E4D10(int a, int b) { spy(S_E4D10, (uint32_t)a, (uint32_t)b, 0, 0, 0, 0, 0); return NULL; }
/* directory enumeration (firstfile/nextfile) and BIOS event/status calls */
static unsigned ff_run;
static const char* const FFN[] = { "BASLUS-00664A1", "BASLUS-00664A2", "BASLUS-00664B", "Q", "", "BASLUS-006640Xeno", "BASLUS-00664A1" };
static void ff_fill(uint8_t* e, uint32_t idx) {
    const char* nm = FFN[(idx * 5u + fidx[S_FIRSTFILE] * 3u) % 7u];
    memset(e, 0, 20);
    memcpy(e, nm, strlen(nm));
}
void* firstfile(char* name, void* e) {
    int32_t r = spy_ret(S_FIRSTFILE, sdig(name), pn(e), 0, 0, 0, 0, 0);
    ff_run = 0;
    if (!r) return NULL;
    ff_fill(e, 0);
    return e;
}
void* nextfile(void* e) {
    int32_t r = spy_ret(S_NEXTFILE, pn(e), 0, 0, 0, 0, 0, 0);
    if (!r || ff_run >= 15) return NULL;
    ff_fill(e, ++ff_run);
    return e;
}
int _card_info(int port) { return spy_ret(S_CARDINFO, (uint32_t)port, 0, 0, 0, 0, 0, 0); }
int TestEvent(unsigned ev) { return spy_ret(S_TESTEV, ev, 0, 0, 0, 0, 0, 0); }
int UnDeliverEvent(unsigned a, unsigned b) { spy(S_UNDELIV, a, b, 0, 0, 0, 0, 0); return 0; }
int CloseEvent(unsigned ev) { spy(S_CLOSEEV, ev, 0, 0, 0, 0, 0, 0); return 0; }
void EnterCriticalSection(void) { spy(S_ENTERCS, 0, 0, 0, 0, 0, 0, 0); }
void ExitCriticalSection(void) { spy(S_EXITCS, 0, 0, 0, 0, 0, 0, 0); }
/* PS1 BIOS card file calls (the port's memory-card layer entry points) */
int PcPortCard_open(const char* name, unsigned mode) { if (getenv("FLOWDBG")) fprintf(stderr, "native open '%s' %x\n", name, mode); return spy_ret(S_OPEN, sdig(name), mode, 0, 0, 0, 0, 0); }
int PcPortCard_read(int fd, void* b, int len) { return do_read((uint32_t)fd, pn(b), b, (uint32_t)(uintptr_t)b, len); }
int PcPortCard_write(int fd, const void* b, int len) {
    return spy_ret(S_WRITE, (uint32_t)fd, pn(b), (uint32_t)len, wdig(b, len), 0, 0, 0);
}
int PcPortCard_close(int fd) { return spy_ret(S_CLOSE, (uint32_t)fd, 0, 0, 0, 0, 0, 0); }
int PcPortCard_rename(const char* a, const char* b) { return spy_ret(S_RENAME, sdig(a), sdig(b), 0, 0, 0, 0, 0); }
int erase(char* name) { return spy_ret(S_ERASE, sdig(name), 0, 0, 0, 0, 0, 0); }
int format(char* name) { return spy_ret(S_FORMAT, sdig(name), 0, 0, 0, 0, 0, 0); }

static uint32_t gs_digest(uint8_t* gs) { return dig(gs, RGS_SIZE); }

typedef struct { uint32_t addr, id, nargs, stackargs, ret; } Target;
static const Target targets[] = {
    { 0x801C7BF4, S_C7BF4, 0, 0, 0 }, { 0x80043A58, S_CLUT, 2, 0, 1 },
    { 0x80033B34, S_DEC, 3, 0, 0 }, { 0x80033C20, S_ENC, 2, 0, 0 }, { 0x801E4A28, S_E4A28, 1, 0, 0 },
    { 0x80028530, S_DISC, 0, 0, 1 }, { 0x8002675C, S_2675C, 4, 3, 1 },
#ifndef FLOW_REAL_SCAN
    { 0x801C93A8, S_93A8, 0, 0, 1 },
#endif
#ifndef FLOW_REAL_IO
    { 0x801CB28C, S_CB28C, 1, 0, 0 },
#endif
    { 0x801E4D10, S_E4D10, 2, 0, 0 },
#ifndef FLOW_REAL_IO
    { 0x801C90B0, S_90B0, 2, 0, 0 }, { 0x801C8CA4, S_C8CA4, 1, 0, 0 },
#endif
    { 0x80040584, S_FIRSTFILE, 2, 0, 1 }, { 0x80040594, S_NEXTFILE, 1, 0, 1 }, { 0x8004E784, S_CARDINFO, 1, 0, 1 },
    { 0x80040494, S_TESTEV, 1, 0, 1 }, { 0x800404C4, S_UNDELIV, 2, 0, 0 }, { 0x80040484, S_CLOSEEV, 1, 0, 0 },
    { 0x800404D4, S_ENTERCS, 0, 0, 0 }, { 0x800404E4, S_EXITCS, 0, 0, 0 },
    { 0x801E78C8, S_E78C8, 1, 0, 0 }, { 0x801D9B08, S_D9B08, 0, 0, 0 },
    { 0x801CA750, S_CA750, 1, 0, 1 }, { 0x801CACF8, S_CACF8, 3, 0, 1 },
    { 0x801C8574, S_C8574, 1, 0, 0 }, { 0x801D2F4C, S_D2F4C, 1, 0, 0 },
    { 0x801D32B4, S_D32B4, 0, 0, 1 }, { 0x801C72BC, S_C72BC, 1, 0, 0 },
    { 0x80040534, S_OPEN, 2, 0, 1 }, { 0x80040544, S_READ, 3, 0, 1 }, { 0x80040554, S_WRITE, 3, 0, 1 },
    { 0x80040564, S_CLOSE, 1, 0, 1 }, { 0x80040574, S_FORMAT, 1, 0, 1 }, { 0x800405A4, S_RENAME, 2, 0, 1 },
    { 0x800405B4, S_ERASE, 1, 0, 1 }, { 0x80031BDC, S_HALLOC, 2, 0, 1 }, { 0x800320E8, S_HFREE, 1, 0, 0 },
    { 0x8004B54C, S_VSYNC, 1, 0, 1 }, { 0x80043C9C, S_SETPOLY, 1, 0, 0 }, { 0x80039E60, S_SND, 1, 0, 0 },
};

static int read_bus(void* o, uint32_t a, unsigned width, uint32_t* v) {
    (void)o;
    if (a < 0x80000000u || (uint64_t)a + width > 0x80200000u) return -1;
    *v = 0;
    for (unsigned i = 0; i < width; i++) *v |= (uint32_t)gp(a)[i] << (8 * i);
    if (active_cpu && width == 4 && a == active_cpu->pc && a >= cov_lo && a < cov_hi && cov[(a - cov_lo) / 4] < 255) cov[(a - cov_lo) / 4]++;
    return 0;
}
static int in_window(uint32_t a, unsigned w) {
    if (a >= 0x801FE000u && (uint64_t)a + w <= 0x80200000u) return 1;
    if (a >= BUF_BASE && (uint64_t)a + w <= BUF_BASE + BUF_SIZE) return 1;
    for (unsigned i = 0; i < NREG; i++)
        if (a >= regions[i].base && (uint64_t)a + w <= (uint64_t)regions[i].base + regions[i].size) return 1;
    return 0;
}
static int write_bus(void* o, uint32_t a, unsigned width, uint32_t v) {
    (void)o;
    if (!in_window(a, width)) { fprintf(stderr, "retail stray write %08x w=%u\n", a, width); return -1; }
    for (unsigned i = 0; i < width; i++) gp(a)[i] = (uint8_t)(v >> (8 * i));
    return 0;
}
static const char* gstr(uint32_t a) { return (const char*)gp(a); }
static int bridge(void* o, PcPortMipsCpu* c, uint32_t t) {
    (void)o;
    uint32_t A0 = c->gpr[4], A1 = c->gpr[5], A2 = c->gpr[6];
    /* libc string/memory helpers of the main executable: real semantics, unrecorded */
    switch (t) {
    case 0x8003F8E8: memset(gp(A0), 0, A1); return 1;                       /* bzero */
    case 0x8003F968: memmove(gp(A0), gp(A1), A2); c->gpr[2] = A0; return 1; /* memcpy */
    case 0x8003FA78: { uint32_t d = A0; while (*gp(d)) d++; for (uint32_t s = A1;; s++, d++) { *gp(d) = *gp(s); if (!*gp(s)) break; } c->gpr[2] = A0; return 1; }
    case 0x8003FB84: { uint32_t d = A0; for (uint32_t s = A1;; s++, d++) { *gp(d) = *gp(s); if (!*gp(s)) break; } c->gpr[2] = A0; return 1; }
    case 0x8003FB20: { int d = strcmp(gstr(A0), gstr(A1)); c->gpr[2] = (uint32_t)(d < 0 ? -1 : d > 0 ? 1 : 0); return 1; }
    default: break;
    }
    for (unsigned i = 0; i < sizeof(targets) / sizeof(targets[0]); i++) {
        if (targets[i].addr != t) continue;
        const Target* g = &targets[i];
        uint32_t a[7] = { 0 };
        for (unsigned k = 0; k < g->nargs && k < 4; k++) a[k] = c->gpr[4 + k];
        for (unsigned k = 0; k < g->stackargs; k++) a[4 + k] = r32(c->gpr[29] + 16 + 4 * k);
        /* normalise pointer-valued arguments exactly as the native spies do */
        switch (g->id) {
        case S_2675C: a[2] = nrm_r(a[2]); break;
        case S_E4A28: {
            uint32_t buf = a[0];
            a[0] = nrm_r(buf); a[1] = gs_digest(gp(RGS_BASE));
            uint32_t d = a[1]; memcpy(gp(buf + 0x30), &d, 4);
            break;
        }
        case S_DEC: {
            uint32_t in = a[0], o2 = a[1], n = a[2];
            a[0] = dig(gp(in), n * 2); a[1] = n; a[2] = 0; a[3] = 0;
            dec_fill(gp(o2), gp(in), n);
            break;
        }
        case S_ENC: {
            uint32_t in = a[0], o2 = a[1];
            a[0] = dig(gp(in), 0x14); a[1] = 0;
            enc_fill(gp(o2), gp(in));
            break;
        }
        case S_CACF8: a[0] &= 0xFF; a[1] &= 0xFF; a[2] &= 0xFF; break;   /* u8 parameters */
        case S_C8CA4: case S_D2F4C: a[0] &= 0xFF; break;                  /* u8 parameter */
        case S_90B0: a[0] &= 0xFF; a[1] &= 0xFF; break;
        case S_CA750: break;
        case S_OPEN: if (getenv("FLOWDBG")) fprintf(stderr, "retail open '%s' %x\n", gstr(a[0]), a[1]); a[0] = sdig(gp(a[0])); break;
        case S_ERASE: case S_FORMAT: a[0] = sdig(gp(a[0])); break;
        case S_RENAME: a[0] = sdig(gp(a[0])); a[1] = sdig(gp(a[1])); break;
        case S_WRITE: { uint32_t ga = a[1]; a[1] = pr(ga); a[3] = wdig(gp(ga), (int32_t)a[2]); break; }
        case S_SETPOLY: {
            uint32_t ga = a[0];
            a[0] = pr(ga);
            if (g->id == S_SETPOLY) { gp(ga)[3] = 4; gp(ga)[7] = 0x28; }
            break;
        }
        case S_CB28C: { uint32_t ga = a[0]; a[0] = pr(ga); a[1] = dig(gp(ga), 0x1EFF); break; }
        default: break;
        }
        if (g->id == S_FIRSTFILE) {
            uint32_t e = a[1];
            int32_t r = spy_ret(S_FIRSTFILE, sdig(gp(a[0])), pr(e), 0, 0, 0, 0, 0);
            ff_run = 0;
            if (r) ff_fill(gp(e), 0);
            c->gpr[2] = r ? e : 0;
            return 1;
        }
        if (g->id == S_NEXTFILE) {
            uint32_t e = a[0];
            int32_t r = spy_ret(S_NEXTFILE, pr(e), 0, 0, 0, 0, 0, 0);
            if (!r || ff_run >= 15) { c->gpr[2] = 0; return 1; }
            ff_fill(gp(e), ++ff_run);
            c->gpr[2] = e;
            return 1;
        }
        if (g->id == S_READ) {
            c->gpr[2] = (uint32_t)do_read(a[0], pr(a[1]), gp(a[1]), a[1], (int32_t)a[2]);
            return 1;
        }
        if (g->id == S_HFREE) { do_hfree(a[0], pr(a[0])); return 1; }
        if (g->id == S_HALLOC) { c->gpr[2] = do_halloc(a[0], a[1]); return 1; }
        if (g->id == S_C7BF4) { a[0] = *cardb_r(0x4FE6); a[1] = gp(GM + 0x32A)[0]; }
        if (g->ret) c->gpr[2] = (uint32_t)spy_ret(g->id, a[0], a[1], a[2], a[3], a[4], a[5], a[6]);
        else spy(g->id, a[0], a[1], a[2], a[3], a[4], a[5], a[6]);
        if (g->id == S_C7BF4 || g->id == S_CA750) { native_side = 0; world_step(); }
        return 1;
    }
    if (t < MENU_BASE || t >= MENU_BASE + MENU_SIZE) return -1; /* unbridged boundary leaving the overlay */
    return 0;
}

/* ---- SystemMenu field table: retail offset -> host offset ---------------- */
enum { K_BYTES, K_PTR }; /* K_PTR: u32 guest word <-> host pointer, `n` words */
typedef struct { uint32_t roff, hoff, n; int kind; } Fld;
static const Fld flds[] = {
    { 0x000, HO_SystemMenu_unk0, 0x6C, K_BYTES },
    { 0x1D4, HO_SystemMenu_pGfxEnv, 1, K_PTR },
    { 0x2DC, HO_SystemMenu_unk2DC, 1, K_PTR },
    { 0x2E4, HO_SystemMenu_unk2E4, 1, K_PTR },
    { 0x308, HO_SystemMenu_renderContext, 4, K_BYTES },
    { 0x325, HO_SystemMenu_input, 7, K_BYTES },            /* input, unk326..unk32B */
    { 0x32C, HO_SystemMenu_unk32C, 1, K_PTR },
    { 0x330, HO_SystemMenu_unk330, 1, K_PTR },
    { 0x334, HO_SystemMenu_unk334, 8, K_BYTES },           /* unk334..unk33B (menu2Choice at +4) */
    { 0x33C, HO_SystemMenu_pManager, 1, K_PTR },
    { 0x364, HO_SystemMenu_windows, NWIN, K_PTR },
    { 0x39C, HO_SystemMenu_unk39C, 0x8C, K_BYTES },        /* 4-byte PSX words incl. slot table */
    { 0x428, HO_SystemMenu_pCursors, 1, K_PTR },
    { 0x42C, HO_SystemMenu_unk42C, 0x10, K_BYTES },
    { 0x44C, HO_SystemMenu_unk44C, 4, K_BYTES },
    { 0x4CC, HO_SystemMenu_unk4CC, 0x10, K_BYTES },
    { 0x4DC, HO_SystemMenu_selectedPartySlot, 1, K_BYTES },
};
#define NFLD (sizeof(flds) / sizeof(flds[0]))
static uint32_t fld_len(const Fld* f) { return f->kind == K_PTR ? f->n * 4 : f->n; }

static uint8_t sm_start[HO_SM_SIZE];
static void sm_build(void) {
    memset(SM, 0, sizeof(SM));
    for (unsigned i = 0; i < NFLD; i++) {
        const Fld* f = &flds[i];
        if (f->kind == K_BYTES) memcpy(SM + f->hoff, gp(GM + f->roff), f->n);
        else for (uint32_t k = 0; k < f->n; k++) {
            uint64_t p = r32(GM + f->roff + 4 * k);
            memcpy(SM + f->hoff + 8 * k, &p, 8);
        }
    }
    memcpy(sm_start, SM, sizeof(SM));
    g_Menu = SM;
}
/* Retail-view image of the card window / BUF as seen through host layout. */
static void card_to_native(uint8_t* dst, const uint8_t* src /* retail window */) {
    memset(dst, 0, CARDW);
    memcpy(dst, src, 0xB80);
    memcpy(dst + 0xBA4, src + 0xB94, CARDW - 0xBA4);
}
static void buf_to_native(uint8_t* dst /* BUF_SIZE */, const uint8_t* ramimg) {
    memcpy(dst, ramimg + (BUF_BASE & 0x1FFFFF), BUF_SIZE);
    card_to_native(dst + (CARD - BUF_BASE), ramimg + (CARD & 0x1FFFFF));
}

static void mirror_in(void) {
    buf_to_native(natbuf, ram);
    for (unsigned r = 0; r < NREG; r++) memcpy(regions[r].nat, gp(regions[r].base), regions[r].size);
    sm_build();
}

static uint8_t cmpbuf[BUF_SIZE];
static void compare_after(const uint8_t* after, const uint8_t* start) {
    /* guest windows (host-layout card translated); GM itself is modelled via SM */
    buf_to_native(cmpbuf, after);
    memcpy(cmpbuf, natbuf, 0); /* keep order explicit */
    {
        uint32_t lo = GM - BUF_BASE, hi = lo + GM_SIZE;
        for (uint32_t i = 0; i < BUF_SIZE; i++) {
            if (i >= lo && i < hi) continue;
            if (natbuf[i] != cmpbuf[i]) {
                fprintf(stderr, "first diff %08x native=%02x retail=%02x\n", BUF_BASE + i, natbuf[i], cmpbuf[i]);
                fail("guest buffer window");
            }
        }
    }
    /* retail card bytes 0xB80..0xB94 (TIM_IMAGE) must be untouched; native TIM zero */
    require(memcmp(after + (CARD & 0x1FFFFF) + 0xB80, start + (CARD & 0x1FFFFF) + 0xB80, 0x14) == 0, "retail TIM span untouched");
    for (unsigned i = 0; i < 0x24; i++) require(natbuf[CARD - BUF_BASE + 0xB80 + i] == 0, "native TIM span untouched");
    /* globals */
    for (unsigned r = 0; r < NREG; r++) {
        uint8_t tmp[0x2400];
        memcpy(tmp, regions[r].nat, regions[r].size);
        if (regions[r].base == RM_BASE) { /* D_801EA6F4 holds a pointer into the card window */
            uint32_t v; memcpy(&v, tmp + (0x801EA6F4u - RM_BASE), 4);
            v = nrm_n(v); memcpy(tmp + (0x801EA6F4u - RM_BASE), &v, 4);
        }
        if (memcmp(tmp, after + (regions[r].base & 0x1FFFFF), regions[r].size) != 0) {
            for (uint32_t i = 0; i < regions[r].size; i++)
                if (tmp[i] != after[(regions[r].base & 0x1FFFFF) + i]) {
                    fprintf(stderr, "first diff %08x native=%02x retail=%02x\n", regions[r].base + i,
                            tmp[i], after[(regions[r].base & 0x1FFFFF) + i]);
                    break;
                }
            { uint32_t nv, rv2; memcpy(&nv, regions[r].nat + (0x801EA6F4u - RM_BASE), 4); memcpy(&rv2, after + (0x801EA6F4u & 0x1FFFFF), 4);
              if (regions[r].base == RM_BASE) fprintf(stderr, "D_801EA6F4 native=%08x retail=%08x start=%08x\n", nv, rv2, (unsigned)*(uint32_t*)(start + (0x801EA6F4u & 0x1FFFFF))); }
            fail("global region bytes");
        }
    }
    /* SystemMenu: every modelled field, then everything outside the table */
    static uint8_t covered_r[GM_SIZE], covered_h[HO_SM_SIZE];
    memset(covered_r, 0, sizeof(covered_r)); memset(covered_h, 0, sizeof(covered_h));
    for (unsigned i = 0; i < NFLD; i++) {
        const Fld* f = &flds[i];
        for (uint32_t k = 0; k < fld_len(f); k++) covered_r[f->roff + k] = 1;
        for (uint32_t k = 0; k < (f->kind == K_PTR ? f->n * 8 : f->n); k++) covered_h[f->hoff + k] = 1;
        if (f->kind == K_BYTES) {
            if (memcmp(SM + f->hoff, after + (GM & 0x1FFFFF) + f->roff, f->n) != 0) {
                for (uint32_t k = 0; k < f->n; k++)
                    if (SM[f->hoff + k] != after[(GM & 0x1FFFFF) + f->roff + k]) {
                        fprintf(stderr, "SystemMenu +%03x native=%02x retail=%02x\n", f->roff + k, SM[f->hoff + k],
                                after[(GM & 0x1FFFFF) + f->roff + k]);
                        break;
                    }
                fail("SystemMenu field");
            }
        } else {
            for (uint32_t k = 0; k < f->n; k++) {
                uint64_t p; memcpy(&p, SM + f->hoff + 8 * k, 8);
                uint32_t rv; memcpy(&rv, after + (GM & 0x1FFFFF) + f->roff + 4 * k, 4);
                if (p != (uint64_t)rv) { fprintf(stderr, "SystemMenu +%03x native=%llx retail=%08x\n", f->roff + 4 * k, (unsigned long long)p, rv); fail("SystemMenu pointer field"); }
            }
        }
    }
    for (uint32_t i = 0; i < GM_SIZE; i++)
        if (!covered_r[i] && after[(GM & 0x1FFFFF) + i] != start[(GM & 0x1FFFFF) + i]) {
            fprintf(stderr, "retail wrote unmodelled SystemMenu +%03x\n", i);
            fail("retail wrote an unmodelled SystemMenu field (extend the table)");
        }
    for (uint32_t i = 0; i < HO_SM_SIZE; i++)
        if (!covered_h[i] && SM[i] != sm_start[i]) {
            fprintf(stderr, "native wrote outside the field table: host SystemMenu +%03x\n", i);
            fail("native wrote an unmodelled SystemMenu byte");
        }
}

#pragma GCC diagnostic ignored "-Wunused-function"
/* ---- native entry wrappers --------------------------------------------- */
extern void func_801CAE08(int);
extern void func_801CA8C0(int);
extern unsigned char func_801CB304(void);
extern int func_801CBD90(unsigned char);
extern unsigned char func_801CC6D8(void);
extern int func_801CD2AC(void);
extern int func_801C93A8(void);
extern void func_801C87C4(void);
extern void func_801CB28C(int);
extern void func_801C8CA4(unsigned char);
extern void func_801C90B0(unsigned char, unsigned char);
extern int func_801C881C(int);
extern int func_801C891C(int);
extern void func_801C8960(void);
extern int func_801C8A10(unsigned char);
extern void func_801C8BEC(void);
extern void func_801C8D1C(unsigned char);
extern int func_801C8D78(unsigned char);
extern void func_801C8EE8(void);
extern int func_801C9038(char*, void*);

typedef struct { uint32_t a0, a1, a2, a3, a4; } Args;
static uint32_t n_AE08(Args a) { func_801CAE08((int)a.a0); return 0; }
static uint32_t n_A8C0(Args a) { func_801CA8C0((int)a.a0); return 0; }
static uint32_t n_B304(Args a) { (void)a; return func_801CB304(); }
static uint32_t n_BD90(Args a) { return (uint32_t)func_801CBD90((unsigned char)a.a0); }
static uint32_t n_C6D8(Args a) { (void)a; return func_801CC6D8(); }
static uint32_t n_B28C(Args a) { func_801CB28C((int)a.a0); return 0; }
static uint32_t n_87C4(Args a) { (void)a; func_801C87C4(); return 0; }
static uint32_t n_881C(Args a) { return (uint32_t)func_801C881C((int)a.a0); }
static uint32_t n_891C(Args a) { return (uint32_t)func_801C891C((int)a.a0); }
static uint32_t n_8960(Args a) { (void)a; func_801C8960(); return 0; }
static uint32_t n_8A10(Args a) { return (uint32_t)func_801C8A10((unsigned char)a.a0); }
static uint32_t n_8BEC(Args a) { (void)a; func_801C8BEC(); return 0; }
static uint32_t n_8CA4(Args a) { func_801C8CA4((unsigned char)a.a0); return 0; }
static uint32_t n_8D1C(Args a) { func_801C8D1C((unsigned char)a.a0); return 0; }
static uint32_t n_8D78(Args a) { return (uint32_t)func_801C8D78((unsigned char)a.a0); }
static uint32_t n_8EE8(Args a) { (void)a; func_801C8EE8(); return 0; }
static uint32_t n_9038(Args a) { return (uint32_t)func_801C9038((char*)(uintptr_t)a.a0, (void*)(uintptr_t)a.a1); }
static uint32_t n_90B0(Args a) { func_801C90B0((unsigned char)a.a0, (unsigned char)a.a1); return 0; }
static uint32_t n_93A8(Args a) { (void)a; return (uint32_t)func_801C93A8(); }
static uint32_t n_D2AC(Args a) { (void)a; return (uint32_t)func_801CD2AC(); }

/* ---- case generators (write the guest image `ram`) ---------------------- */
static void card_default(void) {
    uint32_t mode = rnd() & 3;
    for (uint32_t i = 0; i < RC_SIZE; i++)
        *cardb_r(i) = mode == 0 ? (uint8_t)rnd() : mode == 1 ? 0 : mode == 2 ? 0xFF : (uint8_t)(i * 7);
    if (mode != 0)
        for (uint32_t i = 0; i < 64; i++) { uint32_t k = rnd_range(RC_SIZE); *cardb_r(k) = (uint8_t)rnd(); }
}
static void card_dir(void) { /* directory / presence / selector bytes with realistic shape */
    uint32_t sel = rnd_range(32);
    w32(CARD + 0x4F7C, sel);
    w32(CARD + 0x4F88, (rnd() & 1) ? (rnd() | 0x10000u) : (rnd() & 0xFFFFu));
    for (uint32_t i = 0; i < 0x20; i++) {
        *cardb_r(0x4FAE + i) = (rnd() % 3 == 0) ? 0xFF : (uint8_t)rnd_range(31);
        *cardb_r(0x4F8E + i) = (rnd() % 3 == 0) ? 0 : (uint8_t)rnd_range(3);
    }
    for (uint32_t i = 0; i < 8; i++) *cardb_r(0x4FE4 + i) = (rnd() % 4 == 0) ? 0 : (uint8_t)(1 + rnd_range(3));
}
static void tbl_rand(void) { /* D_801E981C is static retail data; vary it to reach the negative-offset arms */
    if (rnd() % 4 == 0) for (unsigned k = 0; k < 32; k++) w32(0x801E981Cu + 4 * k, (uint32_t)small(-0x30, 0x70));
}
static void feed_fill(void) {
    for (unsigned i = 0; i < NSPY; i++) for (unsigned k = 0; k < 64; k++) feedr[i][k] = rnd();
}
static void put_str(uint32_t cardoff, const char* s) {
    size_t n = strlen(s);
    for (size_t i = 0; i < n; i++) *cardb_r(cardoff + (uint32_t)i) = (uint8_t)s[i];
    *cardb_r(cardoff + (uint32_t)n) = 0;
}
static const char* const NAMES[] = { "BASLUS-00664A1", "BASLUS-00664A2", "BASLUS-00664B", "Q", "", "BASLUS-006640Xeno", "BASLUS-00664A1" };
static const char* const CODES[] = { "BASLUS-00664", "BASLUS-00665", "BESLES-01000", "X" };
static void put_str_ram(uint32_t addr, const char* s) {
    size_t n = strlen(s);
    for (size_t i = 0; i <= n; i++) w8(addr + (uint32_t)i, (uint8_t)s[i]);
}
static void world_gen(void) {
    memset(&world, 0, sizeof(world));
    unsigned n = (rnd() % 16 == 0) ? 190 + rnd_range(10) : rnd_range(24);
    if (n > sizeof(world.inp)) n = sizeof(world.inp);
    world.n = n;
    for (unsigned i = 0; i < n; i++) {
        world.inp[i] = (n >= 150 || rnd_range(10) < 4) ? 8 : (uint8_t)rnd_range(9);
        world.f0[i] = (rnd() % 6 == 0) ? (uint8_t)rnd() : *cardb_r(0x4FE4);
        world.f1[i] = (rnd() % 6 == 0) ? (uint8_t)rnd() : *cardb_r(0x4FE5);
    }
}
static void params_gen(void) {
    static const unsigned fp[] = { 0, 0, 0, 3, 10, 30, 60 };
    static const unsigned yes[] = { 50, 90, 100, 100 };
    P.failpct = fp[rnd_range(7)];
    P.wfail = (rnd() % 4 == 0) ? ((rnd() & 1) ? 100 : 50) : 0;
    P.ca750cap = 1 + rnd_range(7);
    P.yes = yes[rnd_range(4)];
    P.readmode = rnd_range(3);
    P.hdr3 = (rnd() % 5 == 0) ? 0 : (rnd() % 8 == 0) ? 2 : 1;
    P.thr326 = rnd_range(6);
}
/* state shared by the four card actions */
static void flow_state(void) {
    card_default(); card_dir(); tbl_rand();
    /* any directory index byte (0..0xFF) must reach a short NUL-terminated name: retail would smash its stack otherwise */
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t off = i * 0x5C + 0x18 + 0x1F;
        if (off < RC_SIZE) *cardb_r(off) = 0;
    }
    for (uint32_t i = 0; i < 32; i++) put_str(i * 0x5C + 0x18, NAMES[rnd_range(7)]);
    w8(MGR + 0x52, 0); /* the access indicator is not shown when a card action starts */
    for (uint32_t p = 0; p < 2; p++) {
        uint32_t v = (rnd() % 3 == 0) ? 0xFFFFFFFEu : (rnd() % 3 == 0) ? 0 : rnd_range(3);
        w32(CARD + 0x4F74 + 4 * p, v);
    }
    *cardb_r(0x4F8A) = (uint8_t)rnd_range(8); *cardb_r(0x4F8B) = (uint8_t)rnd_range(8);
    { static const uint8_t blk[] = { 0, 1, 1, 1, 1, 2 }; *cardb_r(0x4B97) = blk[rnd_range(6)]; }
    put_str(0x4FCE, CODES[rnd_range(4)]);
    put_str(0x4FFC, NAMES[rnd_range(7)]);
    { static const int32_t used[] = { 0, 5, 14, 15, 16, 3 };
      w32(0x801EA900u, (uint32_t)used[rnd_range(6)]); w32(0x801EA904u, (uint32_t)used[rnd_range(6)]); }
    w8(0x801E9778u, (uint8_t)rnd());
    params_gen();
    world_gen();
    w8(GM + 0x325, (uint8_t)rnd_range(9));
    w8(GM + 0x326, (uint8_t)(rnd() % 3 ? 0 : 1));
    for (unsigned i = 0; i < 3; i++) w8(MGR + 0x30 + i, (rnd() % 4 == 0) ? 0xFF : (uint8_t)rnd_range(12));
    w8(0x801E96A5u, (rnd() % 3 == 0) ? 0 : (uint8_t)rnd());
    for (unsigned e = 0; e < 0x26C / 0x14; e++)
        if (rnd() % 3 == 0) { unsigned t = rnd_range(10); w8(RGS_BASE + e * 0x14 + 2 * t, 0); w8(RGS_BASE + e * 0x14 + 2 * t + 1, 0); }
}
static void g_AE08(Args* a) {
    static const uint32_t modes[] = { 0, 1, 2, 3, 0x100, 0x101, 0x102, 0xFFFFFF01u, 0xFFFFFF02u, 0x80 };
    a->a0 = (rnd() % 4 == 0) ? rnd() : modes[rnd_range(10)];
    params_gen(); world_gen();
    for (uint32_t i = 0; i < 0x3000; i += 4) w32(HEAP_BASE + i, rnd());
    w8(MGR + 0x52, (rnd() % 3 == 0) ? 0 : (uint8_t)(1 + rnd_range(255)));
    w32(GM + 0x44C, HEAP_BASE + 0x1000 + 8 * rnd_range(0x100));
    w8(GM + 0x308, (uint8_t)(rnd() & 1));
    if (rnd() % 8 == 0) w32(GM + 0x308, rnd() & 1);
}
static void g_A8C0(Args* a) {
    a->a0 = rnd();
    card_default();
    put_str(0x4FFC, NAMES[rnd_range(7)]);
}
static void g_B304(Args* a) { (void)a; flow_state(); }
static void g_BD90(Args* a) { a->a0 = (rnd() % 3 == 0) ? 0 : (uint8_t)(1 + rnd_range(255)); flow_state(); }
static void g_C6D8(Args* a) {
    (void)a; flow_state();
    if (rnd() % 3 == 0) { /* destination card absent: reach the "insert card" wait */
        uint32_t sel = r32(CARD + 0x4F7C);
        unsigned s4 = (sel / 15) == 0;
        *cardb_r(0x4FE4 + s4) = 0;
        world.n = 0;
    }
}
static void g_D2AC(Args* a) { (void)a; flow_state(); }
static void io_state(void) {
    flow_state();
    w32(CARD + 0x4F84, rnd_range(0x10));
    for (unsigned p = 0; p < 2; p++) {
        static const uint32_t st[] = { 0xFFFFFFFFu, 0xFFFFFFFEu, 0, 1, 2 };
        w32(CARD + 0x4F74 + 4 * p, st[rnd_range(5)]);
    }
    w8(0x801E9779u, (uint8_t)rnd_range(6));
    w8(GM + 0x326, (uint8_t)rnd_range(8));
    *cardb_r(0x4FE6) = (uint8_t)rnd_range(4);
    *cardb_r(0x4F88) = (rnd() % 3) ? 0 : 1; *cardb_r(0x4F89) = (rnd() % 3) ? 0 : 1;
}
static void g_B28C(Args* a) {
    flow_state();
    a->a0 = PAYLOAD + 0x100 * rnd_range(8);
    for (unsigned i = 0; i < 0x40; i += 4) w32(a->a0 + i, rnd());
}
static void g_io(Args* a) { a->a0 = rnd() & 1; a->a1 = rnd_range(16); io_state(); }
static void g_8A10(Args* a) { a->a0 = rnd() & 1; io_state(); }
static void g_881C(Args* a) { a->a0 = (rnd() & 1) ? 0x10 : 0; io_state(); }
static void g_891C(Args* a) { a->a0 = (rnd() % 4 == 0) ? (uint32_t)small(-2, 40) : ((rnd() & 1) ? 0x10 : 0); io_state(); }
static void g_8CA4(Args* a) {
    a->a0 = (rnd() % 4 == 0) ? (uint32_t)(rnd() & 0xFF) : (rnd() & 1);
    io_state();
}
static void g_9038(Args* a) {
    static const char* const paths[] = { "bu00:BASLUS-00664A1", "bu10:BASLUS-00664A2", "bu00:" };
    a->a0 = SLOT + 0x10; a->a1 = PAYLOAD + 0x200 * rnd_range(8);
    put_str_ram(SLOT + 0x10, paths[rnd_range(3)]);
    io_state();
}
static void g_93A8(Args* a) {
    (void)a; flow_state();
    if (rnd() % 3 == 0) { *cardb_r(0x4FE4) = 0; *cardb_r(0x4FE5) = 0; } /* no card: the 59-vblank wait */
    for (unsigned p = 0; p < 2; p++) {
        *cardb_r(0x4F8A + p) = (rnd() % 4 == 0) ? 0 : (uint8_t)rnd_range(17);
        *cardb_r(0x4F8C + p) = (rnd() % 3 == 0) ? *cardb_r(0x4F8A + p) : (uint8_t)rnd();
    }
    /* block[0x123] indexes the 32-byte match table; a larger value would byte-patch the pointer stored beside it */
    for (unsigned k = 0; k < 32; k++) *cardb_r(0xB94 + k * 0x200 + 0x123) = (uint8_t)rnd_range(16);
    w8(MGR + 0x2F, (uint8_t)(rnd() & 1)); w8(CURS + HO_MenuPointerCursors_unk144, (uint8_t)(rnd() & 1));
    w8(CURS + HO_MenuPointerCursors_renderContexts, (uint8_t)(rnd() & 1));
    if (rnd() % 3 == 0) world.n = 0;
}

typedef struct {
    const char* name; uint32_t entry; unsigned insns;
    uint32_t (*native)(Args); void (*gen)(Args*); unsigned ncases; unsigned retbits;
} Fn;
static Fn fns[] = {
#ifdef FLOW_REAL_SCAN
    { "func_801C93A8", 0x801C93A8, 0x820 / 4, n_93A8, g_93A8, 1500, 32 },
#elif defined(FLOW_REAL_IO)
    { "func_801CB28C", 0x801CB28C, 30, n_B28C, g_B28C, 600, 0 },
    { "func_801C87C4", 0x801C87C4, 22, n_87C4, g_io, 200, 0 },
    { "func_801C881C", 0x801C881C, 64, n_881C, g_881C, 600, 32 },
    { "func_801C891C", 0x801C891C, 17, n_891C, g_891C, 600, 32 },
    { "func_801C8960", 0x801C8960, 44, n_8960, g_io, 600, 0 },
    { "func_801C8A10", 0x801C8A10, 119, n_8A10, g_8A10, 1500, 32 },
    { "func_801C8BEC", 0x801C8BEC, 46, n_8BEC, g_io, 1500, 0 },
    { "func_801C8CA4", 0x801C8CA4, 30, n_8CA4, g_8CA4, 600, 0 },
    { "func_801C8D1C", 0x801C8D1C, 23, n_8D1C, g_8CA4, 600, 0 },
    { "func_801C8D78", 0x801C8D78, 92, n_8D78, g_8A10, 1500, 8 },
    { "func_801C8EE8", 0x801C8EE8, 84, n_8EE8, g_io, 1500, 0 },
    { "func_801C9038", 0x801C9038, 30, n_9038, g_9038, 600, 32 },
    { "func_801C90B0", 0x801C90B0, 112, n_90B0, g_io, 1500, 0 },
#else
    { "func_801CAE08", 0x801CAE08, 223, n_AE08, g_AE08, 1500, 0 },
    { "func_801CA8C0", 0x801CA8C0, 94, n_A8C0, g_A8C0, 600, 0 },
    { "func_801CD2AC", 0x801CD2AC, 281, n_D2AC, g_D2AC, 1500, 32 },
    { "func_801CB304", 0x801CB304, 362, n_B304, g_B304, 1500, 8 },
    { "func_801CBD90", 0x801CBD90, 594, n_BD90, g_BD90, 1500, 32 },
    { "func_801CC6D8", 0x801CC6D8, 756, n_C6D8, g_C6D8, 1500, 8 },
#endif
};

/* ---- one case ------------------------------------------------------------ */
static void fill_windows(void) {
    uint32_t i;
    memcpy(gp(MENU_BASE), menu_image, MENU_SIZE);
    memset(gp(BUF_BASE), 0, BUF_SIZE);
    for (unsigned r = 0; r < NREG; r++)
        if (regions[r].randomize) for (i = 0; i < regions[r].size; i += 4) w32(regions[r].base + i, rnd());
    for (i = 0; i < GM_SIZE; i += 4) w32(GM + i, rnd());
    for (i = 0; i < 0x6C; i++) w8(MGR + i, (uint8_t)rnd());
    for (i = 0; i < 0x14C; i++) w8(CURS + i, (uint8_t)rnd());
    for (i = 0; i < 0x100; i++) w8(GFX + i, (uint8_t)rnd());
    for (unsigned k = 0; k < NWIN; k++) for (i = 0; i < 0x720; i += 4) w32(WIN + k * 0x800 + i, rnd());
    for (unsigned k = 0; k < NSLOT; k++) for (i = 0; i < 0x200; i += 4) w32(SLOT + k * 0x400 + i, rnd());
    card_default();
    w32(P_GMENU, GM);
    w32(GM + 0x1D4, GFX); w32(GM + 0x2DC, 0x80152000u); w32(GM + 0x32C, CARD); w32(GM + 0x33C, MGR);
    w32(GM + 0x428, CURS);
    for (unsigned k = 0; k < NWIN; k++) w32(GM + 0x364 + 4 * k, WIN + k * 0x800);
    for (unsigned k = 0; k < NSLOT; k++) w32(GM + 0x3A8 + 4 * k, SLOT + k * 0x400);
    w32(GM + 0x308, rnd() & 1);
    w32(GM + 0x2E4, SEDST);
    for (i = 0; i < 0x20; i += 4) w32(SEDST + i, rnd());
    heap_ptr = HEAP_BASE; heap_last = 0; heap_top = 0;
}

static void run_case(Fn* f) {
    Args a = { 0, 0, 0, 0, 0 };
    cur_name = f->name;
    memset(ram, 0, sizeof(ram));
    memcpy(gp(MENU_BASE), menu_image, MENU_SIZE);
    fill_windows();
    memset(&trace, 0, sizeof(trace));
    feed_fill();
    memset(&world, 0, sizeof(world));
    f->gen(&a);
    memset(gp(STACK - 0x1000), 0xFF, 0x1000); /* uninitialised retail stack slots read as -1 (see report) */
    static uint8_t start[0x200000], after[0x200000];
    memcpy(start, ram, sizeof(start));
    static typeof(world) world0;
    memcpy(&world0, &world, sizeof(world));

    /* retail */
    trace.n = 0; memset(fidx, 0, sizeof(fidx)); heap_ptr = HEAP_BASE; heap_last = 0; heap_top = 0; world.tick = 0; native_side = 0;
    PcPortMipsBus bus = { 0 }; PcPortMipsCpu cpu;
    bus.read = read_bus; bus.write = write_bus; bus.bridge = bridge;
    PcPortMipsCpuInit(&cpu, &bus);
    for (int r = 16; r < 24; r++) cpu.gpr[r] = 0xC0DE0000u + (uint32_t)r;
    cpu.gpr[28] = 0xC0DE001Cu; cpu.gpr[30] = 0xC0DE001Eu;
    cpu.gpr[29] = STACK; cpu.gpr[31] = HALT;
    cpu.gpr[4] = a.a0; cpu.gpr[5] = a.a1; cpu.gpr[6] = a.a2; cpu.gpr[7] = a.a3;
    w32(STACK + 16, a.a4); /* fifth argument */
    memcpy(start + (STACK & 0x1FFFFF) + 16, ram + (STACK & 0x1FFFFF) + 16, 4);
    cov_lo = f->entry; cov_hi = f->entry + f->insns * 4;
    active_cpu = &cpu;
    int rc = PcPortMipsRun(&cpu, f->entry, HALT, 8000000);
    active_cpu = NULL;
    if (rc != PC_PORT_MIPS_HALTED) fprintf(stderr, "oracle rc=%d %s pc=%08x\n", rc, cpu.error, cpu.pc);
    require(rc == PC_PORT_MIPS_HALTED, "retail run halted");
    require(cpu.gpr[29] == STACK, "retail stack restored");
    for (int r = 16; r < 24; r++) require(cpu.gpr[r] == 0xC0DE0000u + (uint32_t)r, "retail callee-saved reg");
    require(cpu.gpr[28] == 0xC0DE001Cu && cpu.gpr[30] == 0xC0DE001Eu, "retail gp/fp preserved");
    uint32_t rret = cpu.gpr[2];
    static struct { Event ev[MAXEV]; unsigned n; } rt;
    memcpy(&rt, &trace, sizeof(rt));
    memcpy(after, ram, sizeof(after));

    /* native on the identical start state */
    memcpy(ram, start, sizeof(start));
    memcpy(&world, &world0, sizeof(world));
    memset(&trace, 0, sizeof(trace));
    memset(fidx, 0, sizeof(fidx)); heap_ptr = HEAP_BASE; heap_last = 0; heap_top = 0; world.tick = 0; native_side = 1;
    mirror_in();
    fflush(stdout);
    uint32_t nret = f->native(a);
    fflush(stdout);
    native_side = 0;

    if (trace.n != rt.n) fprintf(stderr, "spy counts native=%u retail=%u world.n=%u arg=%u\n", trace.n, rt.n, world.n, a.a0);
    require(trace.n == rt.n, "spy call count");
    for (unsigned e = 0; e < trace.n; e++)
        if (memcmp(&trace.ev[e], &rt.ev[e], sizeof(Event)) != 0) {
            fprintf(stderr, "first trace diff at event %u of %u\n native id=%u a=%08x %08x %08x %08x %08x\n retail id=%u a=%08x %08x %08x %08x %08x\n",
                    e, trace.n, trace.ev[e].id, trace.ev[e].a[0], trace.ev[e].a[1], trace.ev[e].a[2], trace.ev[e].a[3], trace.ev[e].a[4],
                    rt.ev[e].id, rt.ev[e].a[0], rt.ev[e].a[1], rt.ev[e].a[2], rt.ev[e].a[3], rt.ev[e].a[4]);
            fail("spy call trace (order/arguments)");
        }
    if (f->retbits == 32 && nret != rret) fprintf(stderr, "ret native=%08x retail=%08x\n", nret, rret);
    if (f->retbits == 8 && (nret & 0xFF) != (rret & 0xFF)) fprintf(stderr, "ret native=%02x retail=%02x\n", nret & 0xFF, rret & 0xFF);
    if (f->retbits == 32) require(nret == rret, "return value");
    if (f->retbits == 8) require((nret & 0xFF) == (rret & 0xFF), "return value (u8)");
    require(g_Menu == SM, "g_Menu unchanged");
    compare_after(after, start);
    cases++;
}

int main(int argc, char** argv) {
    out = fdopen(dup(1), "w");
    if (!freopen("/dev/null", "w", stdout)) return 2; /* the port arms print diagnostics */
    require(argc >= 2, "menu image argument");
    FILE* fp = fopen(argv[1], "rb"); require(fp != NULL, "open menu image");
    require(fread(menu_image, 1, MENU_SIZE, fp) == MENU_SIZE && fgetc(fp) == EOF, "menu image size");
    fclose(fp);
    natbuf = mmap((void*)(uintptr_t)BUF_BASE, BUF_SIZE, PROT_READ | PROT_WRITE,
                  MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
    require(natbuf == (void*)(uintptr_t)BUF_BASE, "identity map of guest buffers");
    const char* only = argc >= 3 ? argv[2] : NULL;
    unsigned total_fn = 0;
    for (unsigned i = 0; i < sizeof(fns) / sizeof(fns[0]); i++) {
        Fn* f = &fns[i];
        if (only && strcmp(only, f->name) != 0) continue;
        memset(cov, 0, sizeof(cov));
        unsigned before = cases;
        for (unsigned c = 0; c < f->ncases; c++) run_case(f);
        unsigned seen = 0, missing = 0;
        for (unsigned k = 0; k < f->insns; k++) { if (cov[k]) seen++; else missing++; }
        fprintf(out, "MENU CARD FLOW RETAIL %-16s PASS cases=%u instructions=%u/%u\n", f->name, cases - before, seen, f->insns);
        if (missing) {
            fprintf(out, "  uncovered:");
            for (unsigned k = 0; k < f->insns; k++) if (!cov[k]) fprintf(out, " %08x", f->entry + 4 * k);
            fprintf(out, "\n");
        }
        total_fn++;
    }
    fprintf(out, "MENU CARD FLOW RETAIL PASS functions=%u cases=%u checks=%u\n", total_fn, cases, checks);
    fflush(out);
    return 0;
}
