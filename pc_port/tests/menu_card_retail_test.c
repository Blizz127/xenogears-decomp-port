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
#define BUF_SIZE 0x51000u
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

#define STR2(x) #x
#define STR(x) STR2(x)
#define REGION(id) uint8_t id[id##_SIZE] __attribute__((aligned(16), used))
#define ALIAS(sym, reg, addr) \
    __asm__(".globl " #sym "\n.set " #sym ", " #reg "+(" STR(addr) "-" STR(reg##_BASE) ")")
#define RC2_SIZE RC_SIZE2
#define RC2_BASE RC_BASE
REGION(RA); REGION(RB); REGION(RGS); REGION(RC2); REGION(RM);
ALIAS(D_80059460, RA, 0x80059460); ALIAS(D_80059488, RA, 0x80059488);
ALIAS(D_8005A3A0, RB, 0x8005A3A0);
ALIAS(g_GameState, RGS, 0x8006D634);
ALIAS(D_8006F958, RC2, 0x8006F958);
ALIAS(D_801E96A5, RM, 0x801E96A5); ALIAS(D_801E9778, RM, 0x801E9778);
ALIAS(D_801E9779, RM, 0x801E9779); ALIAS(D_801E977A, RM, 0x801E977A);
ALIAS(D_801E981C, RM, 0x801E981C); ALIAS(D_801E9820, RM, 0x801E9820);
ALIAS(D_801EA6D0, RM, 0x801EA6D0); ALIAS(D_801EA6F4, RM, 0x801EA6F4);
ALIAS(D_801EA6F8, RM, 0x801EA6F8); ALIAS(D_801EA8FC, RM, 0x801EA8FC);
ALIAS(D_801EA900, RM, 0x801EA900);
/* g_Menu is a host pointer natively and a u32 guest word in the retail image. */
void* g_Menu;

typedef struct { uint32_t base, size; uint8_t* nat; int randomize; } Region;
static const Region regions[] = {
    { RA_BASE, RA_SIZE, RA, 1 }, { RB_BASE, RB_SIZE, RB, 1 }, { RGS_BASE, RGS_SIZE, RGS, 1 },
    { RC_BASE, RC_SIZE2, RC2, 1 }, { RM_BASE, RM_SIZE, RM, 0 },
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
    fprintf(stderr, "MENU CARD RETAIL FAIL fn=%s case=%u %s\n", cur_name, cases, why);
    fprintf(out, "MENU CARD RETAIL FAIL fn=%s case=%u %s\n", cur_name, cases, why);
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
static int32_t rnd_val(void) {
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
enum { S_C7BF4 = 1, S_E198, S_CLUT, S_DEC, S_ENC, S_E4A28, S_DISC, S_2675C, S_ADDPRIM, S_RTP3, S_RTP4,
       S_D22F4, S_D2484, S_D2AC, S_6D8, S_B304, S_BD90, S_E91C4, NSPY };
#define MAXEV 600
typedef struct { uint32_t id, a[7]; } Event;
static struct { Event ev[MAXEV]; unsigned n; } trace;
static int32_t feed[16];
static unsigned feed_pos;
static uint32_t gs_digest(uint8_t* gs);

/* world hook state (func_801C7BF4 stand-in for the card scan) */
static struct { unsigned tick; uint8_t inp[256]; uint8_t f0[256]; uint8_t f1[256]; unsigned n; } world;
static int native_side;

static void spy(uint32_t id, uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5, uint32_t a6) {
    require(trace.n < MAXEV, "spy count");
    trace.ev[trace.n++] = (Event){ id, { a0, a1, a2, a3, a4, a5, a6 } };
}
static int32_t spy_ret(uint32_t id, uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5, uint32_t a6) {
    unsigned k = feed_pos;
    spy(id, a0, a1, a2, a3, a4, a5, a6);
    feed_pos++;
    return feed[k & 15];
}

static void world_step(void) {
    unsigned t = world.tick++;
    uint8_t in = t < world.n ? world.inp[t] : 4 /* MENU_INPUT_CONFIRM */;
    if (native_side) {
        SM[HO_SystemMenu_input] = in;
        if (t < world.n) { *cardb_n(0x4FE4) = world.f0[t]; *cardb_n(0x4FE5) = world.f1[t]; }
    } else {
        w8(GM + 0x325, in);
        if (t < world.n) { *cardb_r(0x4FE4) = world.f0[t]; *cardb_r(0x4FE5) = world.f1[t]; }
    }
}
/* deterministic decode/encode stand-ins (act on the stack buffers) */
static uint32_t dig(const uint8_t* p, unsigned n) {
    uint32_t h = 2166136261u;
    for (unsigned i = 0; i < n; i++) { h ^= p[i]; h *= 16777619u; }
    return h;
}
static void dec_fill(uint8_t* o, const uint8_t* in, unsigned n) {
    for (unsigned i = 0; i < 0x14; i++) o[i] = (uint8_t)((i < n * 2 ? in[i] : 0x5A) * 7 + i + n);
}
static void enc_fill(uint8_t* o, const uint8_t* in) {
    for (unsigned i = 0; i < 0x14; i++) o[i] = (uint8_t)(in[i] * 5 + 0x33 + i);
}

/* native spy implementations (names as the TU declares them) */
void func_801C7BF4(void) { spy(S_C7BF4, 0, 0, 0, 0, 0, 0, 0); world_step(); }
void func_801CE198(int a, void* b, void* c, int d) {
    spy(S_E198, (uint32_t)a, nrm_n((uintptr_t)b), nrm_n((uintptr_t)c), (uint32_t)d, 0, 0, 0);
}
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
void AddPrim(void* ot, void* p) { spy(S_ADDPRIM, nrm_n((uintptr_t)ot), nrm_n((uintptr_t)p), 0, 0, 0, 0, 0); }
void func_801E91C4(void* p) { spy(S_E91C4, nrm_n((uintptr_t)p), 0, 0, 0, 0, 0, 0); }
void func_801D22F4(int a) { spy(S_D22F4, (uint32_t)a, 0, 0, 0, 0, 0, 0); }
void func_801D2484(void) { spy(S_D2484, 0, 0, 0, 0, 0, 0, 0); }
int func_801CD2AC(void) { return spy_ret(S_D2AC, 0, 0, 0, 0, 0, 0, 0); }
int func_801CC6D8(void) { return spy_ret(S_6D8, 0, 0, 0, 0, 0, 0, 0); }
int func_801CB304(void) { return spy_ret(S_B304, 0, 0, 0, 0, 0, 0, 0); }
int func_801CBD90(int a) { return spy_ret(S_BD90, (uint32_t)a, 0, 0, 0, 0, 0, 0); }

static uint32_t gs_digest(uint8_t* gs) { return dig(gs, RGS_SIZE); }

typedef struct { uint32_t addr, id, nargs, stackargs, ret; } Target;
static const Target targets[] = {
    { 0x801C7BF4, S_C7BF4, 0, 0, 0 }, { 0x801CE198, S_E198, 4, 0, 0 }, { 0x80043A58, S_CLUT, 2, 0, 1 },
    { 0x80033B34, S_DEC, 3, 0, 0 }, { 0x80033C20, S_ENC, 2, 0, 0 }, { 0x801E4A28, S_E4A28, 1, 0, 0 },
    { 0x80028530, S_DISC, 0, 0, 1 }, { 0x8002675C, S_2675C, 4, 3, 1 }, { 0x80043B48, S_ADDPRIM, 2, 0, 0 },
    { 0x801E91C4, S_E91C4, 1, 0, 0 }, { 0x801D22F4, S_D22F4, 1, 0, 0 }, { 0x801D2484, S_D2484, 0, 0, 0 },
    { 0x801CD2AC, S_D2AC, 0, 0, 1 }, { 0x801CC6D8, S_6D8, 0, 0, 1 }, { 0x801CB304, S_B304, 0, 0, 1 },
    { 0x801CBD90, S_BD90, 1, 0, 1 },
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
static int bridge(void* o, PcPortMipsCpu* c, uint32_t t) {
    (void)o;
    for (unsigned i = 0; i < sizeof(targets) / sizeof(targets[0]); i++) {
        if (targets[i].addr != t) continue;
        const Target* g = &targets[i];
        uint32_t a[7] = { 0 };
        for (unsigned k = 0; k < g->nargs && k < 4; k++) a[k] = c->gpr[4 + k];
        for (unsigned k = 0; k < g->stackargs; k++) a[4 + k] = r32(c->gpr[29] + 16 + 4 * k);
        unsigned k = feed_pos;
        /* normalise pointer-valued arguments exactly as the native spies do */
        switch (g->id) {
        case S_E198: a[1] = nrm_r(a[1]); a[2] = nrm_r(a[2]); break;
        case S_2675C: a[2] = nrm_r(a[2]); break;
        case S_ADDPRIM: a[0] = nrm_r(a[0]); a[1] = nrm_r(a[1]); break;
        case S_E91C4: a[0] = nrm_r(a[0]); break;
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
        default: break;
        }
        spy(g->id, a[0], a[1], a[2], a[3], a[4], a[5], a[6]);
        if (g->id == S_C7BF4) { native_side = 0; world_step(); }
        if (g->ret) { c->gpr[2] = (uint32_t)feed[k & 15]; feed_pos++; }
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
    { 0x308, HO_SystemMenu_renderContext, 4, K_BYTES },
    { 0x325, HO_SystemMenu_input, 7, K_BYTES },            /* input, unk326..unk32B */
    { 0x32C, HO_SystemMenu_unk32C, 1, K_PTR },
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

/* ---- native entry wrappers --------------------------------------------- */
extern void func_801C8324(unsigned char);
extern void func_801C9270(unsigned char);
extern int func_801C9BCC(int);
extern int func_801C9D34(int);
extern int func_801CD710(unsigned char);
extern unsigned func_801CAA38(unsigned char);
extern void func_801CF5E4(int, int, int);
extern void func_801CB184(void);
extern void func_801CBA4C(unsigned char*, int, unsigned char);
extern void func_801CF8D8(void);
extern void func_801D3FF8(unsigned char, int, int, int);
extern void func_801D433C(unsigned char, int, int, int, int);
extern void func_801D4688(unsigned char, int, int, int);
extern void func_801D49D0(unsigned char, int, int, int, int);

typedef struct { uint32_t a0, a1, a2, a3, a4; } Args;
static uint32_t n_8324(Args a) { func_801C8324((unsigned char)a.a0); return 0; }
static uint32_t n_9270(Args a) { func_801C9270((unsigned char)a.a0); return 0; }
static uint32_t n_9BCC(Args a) { return (uint32_t)func_801C9BCC((int)a.a0); }
static uint32_t n_9D34(Args a) { return (uint32_t)func_801C9D34((int)a.a0); }
static uint32_t n_D710(Args a) { return (uint32_t)func_801CD710((unsigned char)a.a0); }
static uint32_t n_AA38(Args a) { return func_801CAA38((unsigned char)a.a0); }
static uint32_t n_F5E4(Args a) { func_801CF5E4((int)a.a0, (int)a.a1, (int)a.a2); return 0; }
static uint32_t n_B184(Args a) { (void)a; func_801CB184(); return 0; }
static uint32_t n_BA4C(Args a) { func_801CBA4C((unsigned char*)(uintptr_t)a.a0, (int)a.a1, (unsigned char)a.a2); return 0; }
static uint32_t n_F8D8(Args a) { (void)a; func_801CF8D8(); return 0; }
static uint32_t n_3FF8(Args a) { func_801D3FF8((unsigned char)a.a0, (int)a.a1, (int)a.a2, (int)a.a3); return 0; }
static uint32_t n_433C(Args a) { func_801D433C((unsigned char)a.a0, (int)a.a1, (int)a.a2, (int)a.a3, (int)a.a4); return 0; }
static uint32_t n_4688(Args a) { func_801D4688((unsigned char)a.a0, (int)a.a1, (int)a.a2, (int)a.a3); return 0; }
static uint32_t n_49D0(Args a) { func_801D49D0((unsigned char)a.a0, (int)a.a1, (int)a.a2, (int)a.a3, (int)a.a4); return 0; }

/* ---- case generators (write the guest image `ram`) ---------------------- */
static void feed_fill(void) { for (unsigned i = 0; i < 16; i++) feed[i] = rnd_val(); }

static void card_default(void) {
    uint32_t mode = rnd() & 3;
    for (uint32_t i = 0; i < RC_SIZE; i++)
        *cardb_r(i) = mode == 0 ? (uint8_t)rnd() : mode == 1 ? 0 : mode == 2 ? 0xFF : (uint8_t)(i * 7);
    if (mode != 0)
        for (uint32_t i = 0; i < 64; i++) *cardb_r(rnd_range(RC_SIZE)) = (uint8_t)rnd();
}
static void card_dir(void) { /* directory / presence / selector bytes with realistic shape */
    uint32_t sel = rnd_range(32);
    w32(CARD + 0x4F7C, sel);
    w32(CARD + 0x4F88, (rnd() & 1) ? (rnd() | 0x10000u) : (rnd() & 0xFFFFu));
    for (uint32_t i = 0; i < 0x20; i++) {
        *cardb_r(0x4FAE + i) = (rnd() % 3 == 0) ? 0xFF : (uint8_t)rnd_range(31);
        *cardb_r(0x4F8E + i) = (rnd() % 3 == 0) ? 0 : (uint8_t)rnd_range(3);
    }
    for (uint32_t i = 0; i < 8; i++) *cardb_r(0x4FE4 + i) = (rnd() % 3 == 0) ? 0 : (uint8_t)(1 + rnd_range(3));
}

static void g_8324(Args* a) {
    a->a0 = rnd_range(3) | ((rnd() & 1) ? rnd() << 8 : 0);
    for (unsigned s = 0; s < 3; s++) {
        uint32_t o = GM + s * 0x24;
        int full = (rnd() % 4) == 0;
        for (unsigned k = 0; k < 8; k++) w32(o + 4 * k, (uint32_t)(full ? rnd_val() : small(-300, 300)));
        if (rnd() & 1) w32(o + 0x10, 0x100);
        else if (rnd() & 1) w32(o + 0x14, 0x100);
        w8(o + 0x20, (rnd() % 3 == 0) ? (uint8_t)rnd() : (uint8_t)rnd_range(2));
        w8(o + 0x21, (rnd() % 3 == 0) ? (uint8_t)rnd() : (uint8_t)rnd_range(2));
        w8(o + 0x22, (rnd() % 8 == 0) ? (uint8_t)rnd() : (uint8_t)small(0, 12));
        w8(o + 0x23, (uint8_t)rnd());
    }
}
static void g_9270(Args* a) {
    a->a0 = rnd_range(2);
    card_dir();
    uint8_t code[12];
    for (unsigned i = 0; i < 12; i++) code[i] = (uint8_t)rnd();
    for (unsigned i = 0; i < 12; i++) *cardb_r(0x4FCE + i) = code[i];
    for (unsigned s = 0; s < 31; s++)
        if (rnd() & 1) for (unsigned i = 0; i < 12; i++) *cardb_r(s * 0x5C + 0x18 + i) = code[i];
    for (unsigned i = 0; i < 0x20; i++) w8(RM_BASE + (0x801EA6D0u - RM_BASE) + i, (rnd() % 3 == 0) ? (uint8_t)rnd() : 0);
    w32(0x801EA6F4u, rnd());
    /* block[0x123] indexes the 32-byte match table; a larger value would byte-patch the pointer stored beside it */
    for (unsigned s = 0; s < 31; s++) *cardb_r(0xB94 + s * 0x200 + 0x123) = (uint8_t)rnd_range(16);
    if (rnd() % 5 == 0) for (unsigned i = 0; i < 0x20; i++) *cardb_r(0x4FAE + i) = (rnd() & 1) ? 0xFF : *cardb_r(0x4FAE + i);
}
static void tbl_rand(void) { /* D_801E981C is static retail data; vary it to reach the negative-offset arms */
    if (rnd() % 4 == 0) for (unsigned k = 0; k < 32; k++) w32(0x801E981Cu + 4 * k, (uint32_t)small(-0x30, 0x70));
}
static void g_mode3(Args* a) {
    static const int32_t pick[] = { 0, 1, 2, 3, -1, 0x100, 5 };
    a->a0 = (rnd() % 5 == 0) ? (uint32_t)pick[rnd_range(7)] : rnd_range(3);
    card_dir(); tbl_rand();
    if (rnd() % 4 == 0) for (unsigned i = 0; i < 0x20; i++) *cardb_r(0x4FAE + i) = 0xFF;
    if (rnd() % 4 == 0) for (unsigned i = 0; i < 8; i++) *cardb_r(0x4FE4 + i) = 1;
}
static void g_D710(Args* a) {
    static const uint32_t ch[] = { 0, 1, 2, 3, 4, 0xFF };
    a->a0 = rnd() & 0xFF;
    w8(GM + 0x338, (rnd() % 6 == 0) ? (uint8_t)rnd() : (uint8_t)ch[rnd_range(6)]);
    w32(0x80059460u, (rnd() & 1) ? 2 : (uint32_t)rnd_val());
    for (unsigned i = 0; i < 16; i++) feed[i] = (rnd() % 3 == 0) ? 0 : (rnd() % 3 == 0) ? 0x100 : (rnd() % 2) ? 1 : (int32_t)rnd();
    w8(MGR + 0x2F, (uint8_t)rnd());
}
static void g_AA38(Args* a) {
    a->a0 = (rnd() & 1) ? 0 : (uint8_t)(1 + rnd_range(255));
    card_default();
    *cardb_r(0x4FE4) = (uint8_t)(rnd() % 3 ? 1 : 0); *cardb_r(0x4FE5) = (uint8_t)(rnd() % 3 ? 2 : 0);
    w8(0x801E977Au, (rnd() & 1) ? (uint8_t)rnd() : 0);
    w8(0x801E9778u, (uint8_t)rnd());
    w8(0x801EA8FCu, (uint8_t)rnd());
    memset(&world, 0, sizeof(world));
    unsigned n = (rnd() % 8 == 0) ? 190 + rnd_range(10) : rnd_range(24);
    if (n > sizeof(world.inp)) n = sizeof(world.inp);
    world.n = n;
    for (unsigned i = 0; i < n; i++) {
        world.inp[i] = (n >= 150 || rnd_range(10) < 4) ? 8 : (uint8_t)rnd_range(9);
        world.f0[i] = (rnd() % 6 == 0) ? (uint8_t)rnd() : *cardb_r(0x4FE4);
        world.f1[i] = (rnd() % 6 == 0) ? (uint8_t)rnd() : *cardb_r(0x4FE5);
    }
    w8(CURS + 0x142, (uint8_t)rnd()); w8(CURS + 0x143, (uint8_t)rnd());
    w8(GM + 0x325, (uint8_t)rnd_range(9));
}
static void g_F5E4(Args* a) {
    uint32_t first = rnd_range(26);
    uint32_t cnt = 1 + rnd_range(6);
    a->a0 = first; a->a1 = rnd_range(10);
    a->a2 = (rnd() % 8 == 0 && first > 0) ? first - 1 : first + cnt;
    card_default();
    for (uint32_t i = 0; i < 32; i++) {
        *cardb_r(i * 0x5C + 0x58) = (rnd() % 3 == 0) ? 0 : (uint8_t)(1 + rnd_range(2));
        *cardb_r(0xB97 + i * 0x200) = (uint8_t)rnd_range(4);
    }
    w32(GM + 0x4CC, (rnd() % 4 == 0) ? rnd_range(30) : rnd_range(22));
    for (uint32_t i = 0; i < NSLOT; i++) { w32(GM + 0x3A8 + 4 * i, SLOT + i * 0x400); }
}
static void g_B184(Args* a) {
    (void)a;
    for (unsigned e = 0; e < 0x26C / 0x14; e++) {
        if (rnd() % 3 == 0) {
            unsigned t = rnd_range(10);
            w8(RGS_BASE + e * 0x14 + 2 * t, 0); w8(RGS_BASE + e * 0x14 + 2 * t + 1, 0);
        }
    }
}
static void g_BA4C(Args* a) {
    a->a0 = PAYLOAD; a->a1 = rnd(); a->a2 = rnd() & 0xFF;
    for (unsigned i = 0; i < 3; i++) w8(MGR + 0x30 + i, (rnd() % 4 == 0) ? 0xFF : (uint8_t)rnd_range(12));
    w8(0x801E96A5u, (rnd() % 3 == 0) ? 0 : (uint8_t)rnd());
    for (unsigned i = 0; i < 0x1000; i += 4) w32(PAYLOAD + i, rnd());
    g_B184(a);
}
static void g_F8D8(Args* a) {
    (void)a;
    card_default(); card_dir(); tbl_rand();
    w8(GM + 0x4D8, (rnd() % 3 == 0) ? 0 : (uint8_t)rnd_range(3));
    w8(MGR + 0x68, (uint8_t)(rnd() % 4 ? 1 : 0)); w8(MGR + 0x2F, (uint8_t)(rnd() % 2));
    for (unsigned i = 0; i < 16; i++) feed[i] = small(0, 4);
}
static void g_win(Args* a, int five) {
    unsigned k = rnd_range(NWIN);
    a->a0 = k;
    int full = rnd() % 5 == 0;
    a->a1 = (uint32_t)(full ? rnd_val() : small(-50, 400));
    a->a2 = (uint32_t)(full ? rnd_val() : small(-50, 300));
    a->a3 = (uint32_t)(full ? rnd_val() : small(0, 400));
    a->a4 = (uint32_t)(full ? rnd_val() : small(0, 300));
    (void)five;
}
static void g_3FF8(Args* a) { g_win(a, 0); }
static void g_433C(Args* a) { g_win(a, 1); }
static void g_4688(Args* a) { g_win(a, 0); }
static void g_49D0(Args* a) { g_win(a, 1); }

typedef struct {
    const char* name; uint32_t entry; unsigned insns;
    uint32_t (*native)(Args); void (*gen)(Args*); unsigned ncases; unsigned retbits;
} Fn;
static Fn fns[] = {
    { "func_801C8324", 0x801C8324, 126, n_8324, g_8324, 1500, 0 },
    { "func_801C9270", 0x801C9270, 78, n_9270, g_9270, 1500, 0 },
    { "func_801C9BCC", 0x801C9BCC, 90, n_9BCC, g_mode3, 1500, 32 },
    { "func_801C9D34", 0x801C9D34, 112, n_9D34, g_mode3, 1500, 32 },
    { "func_801CD710", 0x801CD710, 67, n_D710, g_D710, 1500, 32 },
    { "func_801CAA38", 0x801CAA38, 176, n_AA38, g_AA38, 1500, 32 },
    { "func_801CF5E4", 0x801CF5E4, 189, n_F5E4, g_F5E4, 1500, 0 },
    { "func_801CB184", 0x801CB184, 66, n_B184, g_B184, 400, 0 },
    { "func_801CBA4C", 0x801CBA4C, 209, n_BA4C, g_BA4C, 400, 0 },
    { "func_801CF8D8", 0x801CF8D8, 156, n_F8D8, g_F8D8, 1500, 0 },
    { "func_801D3FF8", 0x801D3FF8, 209, n_3FF8, g_3FF8, 1500, 0 },
    { "func_801D433C", 0x801D433C, 211, n_433C, g_433C, 1500, 0 },
    { "func_801D4688", 0x801D4688, 210, n_4688, g_4688, 1500, 0 },
    { "func_801D49D0", 0x801D49D0, 211, n_49D0, g_49D0, 1500, 0 },
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
}

static void run_case(Fn* f) {
    Args a = { 0, 0, 0, 0, 0 };
    cur_name = f->name;
    memset(ram, 0, sizeof(ram));
    memcpy(gp(MENU_BASE), menu_image, MENU_SIZE);
    fill_windows();
    memset(&trace, 0, sizeof(trace));
    feed_fill(); feed_pos = 0;
    memset(&world, 0, sizeof(world));
    f->gen(&a);
    static uint8_t start[0x200000], after[0x200000];
    memcpy(start, ram, sizeof(start));
    static int32_t feed0[16];
    memcpy(feed0, feed, sizeof(feed));
    static typeof(world) world0;
    memcpy(&world0, &world, sizeof(world));

    /* retail */
    trace.n = 0; feed_pos = 0; world.tick = 0; native_side = 0;
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
    int rc = PcPortMipsRun(&cpu, f->entry, HALT, 400000);
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
    memcpy(feed, feed0, sizeof(feed));
    memcpy(&world, &world0, sizeof(world));
    memset(&trace, 0, sizeof(trace));
    feed_pos = 0; world.tick = 0; native_side = 1;
    mirror_in();
    fflush(stdout);
    uint32_t nret = f->native(a);
    fflush(stdout);
    native_side = 0;

    if (trace.n != rt.n) fprintf(stderr, "spy counts native=%u retail=%u world.n=%u arg=%u\n", trace.n, rt.n, world.n, a.a0);
    require(trace.n == rt.n, "spy call count");
    require(memcmp(trace.ev, rt.ev, sizeof(Event) * trace.n) == 0, "spy call trace (order/arguments)");
    if (f->retbits == 32 && nret != rret) fprintf(stderr, "ret native=%08x retail=%08x a0=%08x choice=%02x d59460=%08x feed0=%08x feed1=%08x\n", nret, rret, a.a0, ram[(GM+0x338)&0x1FFFFF], r32(0x80059460u), feed0[0], feed0[1]);
    if (f->retbits == 32) require(nret == rret, "return value");
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
        fprintf(out, "MENU CARD RETAIL %-16s PASS cases=%u instructions=%u/%u\n", f->name, cases - before, seen, f->insns);
        if (missing) {
            fprintf(out, "  uncovered:");
            for (unsigned k = 0; k < f->insns; k++) if (!cov[k]) fprintf(out, " %08x", f->entry + 4 * k);
            fprintf(out, "\n");
        }
        total_fn++;
    }
    fprintf(out, "MENU CARD RETAIL PASS functions=%u cases=%u checks=%u\n", total_fn, cases, checks);
    fflush(out);
    return 0;
}
