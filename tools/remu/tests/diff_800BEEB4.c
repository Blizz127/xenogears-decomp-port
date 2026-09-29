/* Differential test: host-compiled src/battle/main125.c func_800BEEB4
 * vs the retail bytes (asm/battle/nonmatchings/main125/func_800BEEB4.s)
 * executed by remu. Fails on any behavioral mismatch.
 *
 * func_800BEEB4(mask, out, value): for i in 0..10, when bit i of mask is
 * set and table[i] (u32 at D_800C3EB0 + 0x8C8C + i*4) is non-NULL, store
 * value to entry+0x74 and append entry to out; NUL-terminate out and
 * return the append count. The mask is shifted as (mask & 0xFFFF) >> 1.
 *
 * Retail holds entry addresses in 32-bit words; the host build runs on
 * LP64, so test structs live in a MAP_32 pool whose addresses fit u32
 * (same truncation-free round-trip as the 32-bit target).
 *
 * Build (from repo root):
 *   tools/remu/run_diff.sh diff_800BEEB4 src/battle/main125.c
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#ifndef MAP_32
#define MAP_32 0x40
#endif

#include "remu.h"

/* ---- host side: the real TU object (main125.c built with remu_shim.h so
 * INCLUDE_ASM is neutralized but every C body is identical). ---- */
#include "common.h"
u32 func_800BEEB4(u32 mask, u32 *out, u32 value);

/* TU externs the test must provide (D_800C3EB0 is used; the rest keep the
 * link whole for sibling C bodies in the TU). */
u8 D_800C3EB0[36864];
u8 *D_800C3610;
void *HeapAlloc(u_int s, u_int f) { (void)s; (void)f; return NULL; }
u_int HeapFree(void *p) { (void)p; return 0; }
void func_800AA320(u32 a, u32 b, u32 c) { (void)a; (void)b; (void)c; }
s32 func_80023124(s32 a, s32 b) { (void)a; (void)b; return 0; }
void func_800245D8(u32 a, u32 b) { (void)a; (void)b; }

#define RETAIL_S "asm/battle/nonmatchings/main125/func_800BEEB4.s"
#define TAB_BASE 0x800C3EB0u
#define TAB_OFF 0x8C8Cu
#define NENT 11
#define STRUCT_SZ 0x80
#define FIELD_OFF 0x74
#define RS_BASE 0x801E0000u
#define R_OUT 0x801F1000u

static uint32_t lcg(uint32_t *s) {
    *s = *s * 1103515245u + 12345u;
    return *s;
}

static u8 *g_pool;

static int check_one(remu_t *m, uint32_t entry, uint32_t mask,
                     uint32_t seed) {
    uint32_t x = seed ^ 0xBEEB005u;
    uint32_t value = lcg(&x);
    uint32_t nullbits = lcg(&x);
    uint32_t h_out[12], h_field[NENT];
    uint32_t tab[NENT];

    for (int i = 0; i < NENT; i++) {
        u8 *st = g_pool + i * STRUCT_SZ;
        uint32_t f = lcg(&x);
        memcpy(st + FIELD_OFF, &f, 4);
        h_field[i] = f;
        if ((nullbits >> i) & 1)
            tab[i] = 0;
        else
            tab[i] = (uint32_t)(uintptr_t)st;
    }
    memset(h_out, 0xA5, sizeof(h_out));

    /* host side */
    memcpy(D_800C3EB0 + TAB_OFF, tab, sizeof(tab));
    u32 got = func_800BEEB4(mask, h_out, value);

    /* retail side */
    uint32_t rtab[NENT];
    for (int i = 0; i < NENT; i++)
        rtab[i] = tab[i] ? (RS_BASE + i * STRUCT_SZ) : 0;
    uint8_t rs[NENT * STRUCT_SZ];
    for (int i = 0; i < NENT; i++) {
        memcpy(rs + i * STRUCT_SZ, g_pool + i * STRUCT_SZ, STRUCT_SZ);
        memcpy(rs + i * STRUCT_SZ + FIELD_OFF, &h_field[i], 4);
    }
    uint8_t rout_blank[48];
    memset(rout_blank, 0xA5, sizeof(rout_blank));
    if (remu_poke(m, TAB_BASE + TAB_OFF, rtab, sizeof(rtab))) { printf("FAIL poke tab\n"); return 0; }
    if (remu_poke(m, RS_BASE, rs, sizeof(rs))) { printf("FAIL poke structs\n"); return 0; }
    if (remu_poke(m, R_OUT, rout_blank, sizeof(rout_blank))) { printf("FAIL poke out\n"); return 0; }
    int rc = remu_call(m, entry, mask, R_OUT, value, 0);
    if (rc != 0) {
        printf("FAIL seed=%08X remu rc=%d stubs=%d%s\n", seed, rc,
               remu_stub_calls(m), remu_stub_log(m));
        return 0;
    }
    if (remu_stub_calls(m) != 0) {
        printf("FAIL seed=%08X unexpected stub calls%s\n", seed,
               remu_stub_log(m));
        return 0;
    }
    uint32_t want_v0 = remu_get_reg(m, 2);
    if (got != want_v0) {
        printf("FAIL seed=%08X count host=%u retail=%u\n", seed, got, want_v0);
        return 0;
    }
    /* out words: remap retail addresses back to host pool for comparison */
    uint32_t rout[12];
    if (remu_peek(m, R_OUT, rout, sizeof(rout))) { printf("FAIL peek out\n"); return 0; }
    for (int i = 0; i < 12; i++) {
        uint32_t want = (i <= (int)got) ? h_out[i] : 0xA5A5A5A5u;
        uint32_t gotw = rout[i];
        uint32_t gotw_mapped = gotw;
        if (want == 0xA5A5A5A5u) {
            if (gotw != want) {
                printf("FAIL seed=%08X tail[%d] retail=%08X\n", seed, i,
                       gotw);
                return 0;
            }
            continue;
        }
        if (gotw != 0) {
            if ((gotw < RS_BASE) || ((gotw - RS_BASE) % STRUCT_SZ) != 0 ||
                (gotw - RS_BASE) / STRUCT_SZ >= NENT) {
                printf("FAIL seed=%08X out[%d] retail=%08X out of range\n",
                       seed, i, gotw);
                return 0;
            }
            int k = (int)((gotw - RS_BASE) / STRUCT_SZ);
            gotw_mapped = (uint32_t)(uintptr_t)(g_pool + k * STRUCT_SZ);
        }
        if (gotw_mapped != want) {
            printf("FAIL seed=%08X out[%d] retail=%08X host=%08X\n", seed, i,
                   gotw, want);
            return 0;
        }
    }
    /* struct fields */
    uint8_t rs_after[NENT * STRUCT_SZ];
    if (remu_peek(m, RS_BASE, rs_after, sizeof(rs_after))) { printf("FAIL peek structs\n"); return 0; }
    for (int i = 0; i < NENT; i++) {
        uint32_t hf, rf;
        memcpy(&hf, g_pool + i * STRUCT_SZ + FIELD_OFF, 4);
        memcpy(&rf, rs_after + i * STRUCT_SZ + FIELD_OFF, 4);
        if (hf != rf) {
            printf("FAIL seed=%08X field[%d] host=%08X retail=%08X\n", seed,
                   i, hf, rf);
            return 0;
        }
    }
    return 1;
}

int main(void) {
    g_pool = mmap(NULL, NENT * STRUCT_SZ, PROT_READ | PROT_WRITE,
                  MAP_PRIVATE | MAP_ANONYMOUS | MAP_32, -1, 0);
    if (g_pool == MAP_FAILED) { printf("FAIL mmap\n"); return 1; }
    if ((uint64_t)(uintptr_t)(g_pool + NENT * STRUCT_SZ) > 0xFFFFFFFFull) {
        printf("FAIL pool not u32-addressable: %p\n", g_pool);
        return 1;
    }
    remu_t *m = remu_create();
    if (!m) { printf("FAIL create\n"); return 1; }
    uint32_t entry = remu_load_s(m, RETAIL_S);
    if (entry != 0x800BEEB4u) { printf("FAIL load entry=%08X\n", entry); return 1; }

    int n = 0;
    uint32_t edges[] = { 0x00000000u, 0x00000001u, 0x000007FFu, 0xFFFFFFFFu,
                         0x7FFFFFFFu, 0x80000000u, 0xFFFFF800u, 0x0000ABCDu };
    for (unsigned i = 0; i < sizeof(edges) / sizeof(edges[0]); i++) {
        if (!check_one(m, entry, edges[i], 0x1000u + i)) return 1;
        n++;
    }
    for (int b = 0; b < 11; b++) {
        if (!check_one(m, entry, 1u << b, 0x2000u + (uint32_t)b)) return 1;
        n++;
    }
    uint32_t s = 0x23456789u;
    for (int i = 0; i < 200; i++) {
        s = s * 1103515245u + 12345u;
        uint32_t mask = s & 0xFFFFFFu;
        s = s * 1103515245u + 12345u;
        if (!check_one(m, entry, mask, s)) return 1;
        n++;
    }
    printf("DIFF 800BEEB4 OK (%d seeds, retail-exact)\n", n);
    remu_destroy(m);
    return 0;
}
