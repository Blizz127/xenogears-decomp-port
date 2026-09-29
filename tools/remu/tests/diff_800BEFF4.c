/* Differential test: host-compiled src/battle/main125.c func_800BEFF4
 * vs the retail bytes (asm/battle/nonmatchings/main125/func_800BEFF4.s)
 * executed by remu. Fails on any behavioral mismatch.
 *
 * func_800BEFF4(id): base = D_800C3610; unk = *(base+4);
 *   if (unk != 0 && *(base+0x24) != id):
 *       idx = ((*(unk+0xAC) & 3) << 2) | (*(unk+0xA8) >> 30);
 *       if ((D_800C3EB0 + idx*0x1C)[7] == 0)
 *           func_800245D8(unk, (s8)*(unk+0xB0));
 *   *(D_800C3610+0x24) = id; *(D_800C3610+4) = table[id]
 *   (table = u32 words at D_800C3EB0 + 0x8C8C).
 *
 * Retail holds unk in a 32-bit word; the host build runs on LP64, so the
 * test structs live in a MAP_32 pool whose addresses fit u32
 * (same truncation-free round-trip as the 32-bit target).
 * func_800245D8 is an external stub on both sides; the test pins the
 * call count and args against independently computed expectations.
 *
 * Build (from repo root):
 *   tools/remu/run_diff.sh diff_800BEFF4 src/battle/main125.c
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#ifndef MAP_32
#define MAP_32 0x40
#endif

#include "remu.h"

/* ---- host side: the real TU object (main125.c built with remu_shim.h so
 * INCLUDE_ASM is neutralized but every C body is identical). ---- */
#include "common.h"
void func_800BEFF4(u32 arg0);

/* TU externs the test must provide (D_800C3EB0/D_800C3610 are used). */
u8 D_800C3EB0[36864];
u8 *D_800C3610;
void *HeapAlloc(u_int s, u_int f) { (void)s; (void)f; return NULL; }
u_int HeapFree(void *p) { (void)p; return 0; }
void func_800AA320(u32 a, u32 b, u32 c) { (void)a; (void)b; (void)c; }
s32 func_80023124(s32 a, s32 b) { (void)a; (void)b; return 0; }

static u32 g_f0, g_f1;
static unsigned g_nf;
void func_800245D8(u32 a, u32 b) {
    g_nf++;
    g_f0 = a; g_f1 = b;
}

#define RETAIL_S "asm/battle/nonmatchings/main125/func_800BEFF4.s"
#define TAB_BASE 0x800C3EB0u
#define TAB_OFF 0x8C8Cu
#define RBASE 0x801E0000u
#define RUNK (RBASE + 0x100u)
#define BASE_SZ 0x40
#define UNK_SZ 0xC0
#define NIDX 16
#define NTAB 16

static uint32_t lcg(uint32_t *s) {
    *s = *s * 1103515245u + 12345u;
    return *s;
}

static u8 *g_pool;

static int check_one(remu_t *m, uint32_t entry, uint32_t seed) {
    uint32_t x = seed ^ 0xBEFF4000u;
    uint32_t arg0 = lcg(&x) & 0xF;
    uint32_t cur = lcg(&x) & 0xF;
    uint32_t wA8 = lcg(&x), wAC = lcg(&x);
    int8_t bB0 = (int8_t)(lcg(&x) >> 16);
    uint8_t entries[NIDX * 0x1C];
    uint32_t tab[NTAB];
    int force_null = (seed & 0x100) != 0;
    int force_same = (seed & 0x200) != 0;

    for (unsigned i = 0; i < sizeof(entries); i++)
        entries[i] = (uint8_t)(lcg(&x) >> 16);
    for (int i = 0; i < NTAB; i++)
        tab[i] = lcg(&x);
    if (force_same)
        cur = arg0;

    uint32_t idx = (((wAC & 3) << 2) | (wA8 >> 30)) & 0xF;
    int want_call = !force_null && (cur != arg0) && (entries[idx * 0x1C + 7] == 0);
    uint32_t want_a0 = (uint32_t)(uintptr_t)(g_pool + 0x100);
    uint32_t want_a1 = (uint32_t)(int32_t)bB0;

    /* host side */
    u8 *base = g_pool;
    u8 *unk = g_pool + 0x100;
    memset(base, 0, BASE_SZ);
    memset(unk, 0, UNK_SZ);
    D_800C3610 = base;
    memcpy(D_800C3EB0, entries, sizeof(entries));
    memcpy(D_800C3EB0 + TAB_OFF, tab, sizeof(tab));
    *(uint32_t *)(base + 4) = force_null ? 0 : want_a0;
    *(uint32_t *)(base + 0x24) = cur;
    *(uint32_t *)(unk + 0xA8) = wA8;
    *(uint32_t *)(unk + 0xAC) = wAC;
    *(int8_t *)(unk + 0xB0) = bB0;
    g_nf = 0;
    func_800BEFF4(arg0);
    if (g_nf != (unsigned)want_call) {
        printf("FAIL seed=%08X host calls=%u want=%d\n", seed, g_nf,
               want_call);
        return 0;
    }
    if (want_call && (g_f0 != want_a0 || g_f1 != want_a1)) {
        printf("FAIL seed=%08X host callee args=%08X %08X want=%08X %08X\n",
               seed, g_f0, g_f1, want_a0, want_a1);
        return 0;
    }
    uint32_t h_after4, h_after24;
    memcpy(&h_after4, base + 4, 4);
    memcpy(&h_after24, base + 0x24, 4);
    if (h_after24 != arg0 || h_after4 != tab[arg0]) {
        printf("FAIL seed=%08X host base after=%08X %08X want=%08X %08X\n",
               seed, h_after4, h_after24, tab[arg0], arg0);
        return 0;
    }

    /* retail side */
    uint8_t rbase[BASE_SZ], runk[UNK_SZ];
    memset(rbase, 0, sizeof(rbase));
    memset(runk, 0, sizeof(runk));
    *(uint32_t *)(rbase + 4) = force_null ? 0 : RUNK;
    *(uint32_t *)(rbase + 0x24) = cur;
    *(uint32_t *)(runk + 0xA8) = wA8;
    *(uint32_t *)(runk + 0xAC) = wAC;
    *(int8_t *)(runk + 0xB0) = bB0;
    if (remu_poke(m, TAB_BASE, entries, sizeof(entries))) { printf("FAIL poke entries\n"); return 0; }
    if (remu_poke(m, TAB_BASE + TAB_OFF, tab, sizeof(tab))) { printf("FAIL poke tab\n"); return 0; }
    if (remu_poke(m, RBASE, rbase, sizeof(rbase))) { printf("FAIL poke base\n"); return 0; }
    if (remu_poke(m, RUNK, runk, sizeof(runk))) { printf("FAIL poke unk\n"); return 0; }
    {
        uint32_t rb = RBASE;
        if (remu_poke(m, 0x800C3610u, &rb, 4)) { printf("FAIL poke 3610\n"); return 0; }
    }
    int rc = remu_call(m, entry, arg0, 0, 0, 0);
    if (rc != 0) {
        printf("FAIL seed=%08X remu rc=%d stubs=%d%s\n", seed, rc,
               remu_stub_calls(m), remu_stub_log(m));
        return 0;
    }
    if (remu_stub_calls(m) != want_call ||
        (want_call && strcmp(remu_stub_log(m), " 800245D8") != 0)) {
        printf("FAIL seed=%08X stubs=%d%s want_call=%d\n", seed,
               remu_stub_calls(m), remu_stub_log(m), want_call);
        return 0;
    }
    uint8_t rbase_after[BASE_SZ];
    if (remu_peek(m, RBASE, rbase_after, sizeof(rbase_after))) { printf("FAIL peek base\n"); return 0; }
    uint32_t r_after4, r_after24;
    memcpy(&r_after4, rbase_after + 4, 4);
    memcpy(&r_after24, rbase_after + 0x24, 4);
    if (r_after24 != arg0 || r_after4 != tab[arg0]) {
        printf("FAIL seed=%08X retail base after=%08X %08X want=%08X %08X\n",
               seed, r_after4, r_after24, tab[arg0], arg0);
        return 0;
    }
    return 1;
}

int main(void) {
    g_pool = mmap(NULL, 0x1000, PROT_READ | PROT_WRITE,
                  MAP_PRIVATE | MAP_ANONYMOUS | MAP_32, -1, 0);
    if (g_pool == MAP_FAILED) { printf("FAIL mmap\n"); return 1; }
    if ((uint64_t)(uintptr_t)(g_pool + 0x1000) > 0xFFFFFFFFull) {
        printf("FAIL pool not u32-addressable: %p\n", g_pool);
        return 1;
    }
    remu_t *m = remu_create();
    if (!m) { printf("FAIL create\n"); return 1; }
    uint32_t entry = remu_load_s(m, RETAIL_S);
    if (entry != 0x800BEFF4u) { printf("FAIL load entry=%08X\n", entry); return 1; }

    int n = 0;
    uint32_t edges[] = { 0x00000000u, 0x00000001u, 0x00000100u, 0x00000200u,
                         0x00000300u, 0xFFFFFFFFu, 0x7FFFFFFFu, 0x80000000u };
    for (unsigned i = 0; i < sizeof(edges) / sizeof(edges[0]); i++) {
        if (!check_one(m, entry, edges[i])) return 1;
        n++;
    }
    for (int b = 0; b < 32; b++) {
        if (!check_one(m, entry, 0xBEFF4000u ^ (1u << b))) return 1;
        n++;
    }
    uint32_t s = 0x56789ABCu;
    for (int i = 0; i < 200; i++) {
        s = s * 1103515245u + 12345u;
        if (!check_one(m, entry, s)) return 1;
        n++;
    }
    printf("DIFF 800BEFF4 OK (%d seeds, retail-exact)\n", n);
    remu_destroy(m);
    return 0;
}
