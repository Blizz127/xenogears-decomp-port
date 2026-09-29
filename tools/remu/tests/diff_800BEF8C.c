/* Differential test: host-compiled src/battle/main125.c func_800BEF8C
 * vs the retail bytes (asm/battle/nonmatchings/main125/func_800BEF8C.s)
 * executed by remu. Fails on any behavioral mismatch.
 *
 * func_800BEF8C(a): pack s16 pairs into words
 *   w1 = u16(a[2]) | u16(a[0xA]) << 16, w0 = a[0xA0] | a[0xA4] << 16
 * (unsigned halves), call func_80023124(w0, w1), return (s16)result
 * sign-extended.
 *
 * func_80023124 is an external stub on both sides (remu forces v0=0;
 * the host stub records args and returns 0), so the test pins the exact
 * packed arg words against independently computed expectations as well
 * as the return value and memory preservation.
 *
 * Build (from repo root):
 *   tools/remu/run_diff.sh diff_800BEF8C src/battle/main125.c
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "remu.h"

/* ---- host side: the real TU object (main125.c built with remu_shim.h so
 * INCLUDE_ASM is neutralized but every C body is identical). ---- */
#include "common.h"
u32 func_800BEF8C(u8 *arg0);

/* TU externs the test must provide. */
u8 D_800C3EB0[36864];
u8 *D_800C3610;
void *HeapAlloc(u_int s, u_int f) { (void)s; (void)f; return NULL; }
u_int HeapFree(void *p) { (void)p; return 0; }
void func_800AA320(u32 a, u32 b, u32 c) { (void)a; (void)b; (void)c; }
void func_800245D8(u32 a, u32 b) { (void)a; (void)b; }

static s32 g_r0, g_r1;
static unsigned g_nc;
s32 func_80023124(s32 a, s32 b) {
    g_nc++;
    g_r0 = a; g_r1 = b;
    return 0;
}

#define RETAIL_S "asm/battle/nonmatchings/main125/func_800BEF8C.s"
#define RA 0x801F0000u
#define SLEN 0xB0

static uint32_t lcg(uint32_t *s) {
    *s = *s * 1103515245u + 12345u;
    return *s;
}

static int16_t rd16(const uint8_t *p) {
    int16_t v;
    memcpy(&v, p, 2);
    return v;
}

static uint16_t ru16(const uint8_t *p) {
    uint16_t v;
    memcpy(&v, p, 2);
    return v;
}

static int check_one(remu_t *m, uint32_t entry, uint32_t seed) {
    uint32_t x = seed ^ 0xBEF8C000u;
    static uint8_t h_a[SLEN];
    uint8_t sa[SLEN];

    for (int i = 0; i < SLEN; i++)
        sa[i] = (uint8_t)(lcg(&x) >> 16);
    uint32_t w1 = (uint16_t)rd16(sa + 2) |
                  ((uint32_t)(uint16_t)rd16(sa + 0xA) << 16);
    uint32_t w0 = ru16(sa + 0xA0) | ((uint32_t)ru16(sa + 0xA4) << 16);

    /* host side */
    memcpy(h_a, sa, SLEN);
    g_nc = 0;
    u32 got = func_800BEF8C(h_a);
    if (g_nc != 1 || (uint32_t)g_r0 != w0 || (uint32_t)g_r1 != w1) {
        printf("FAIL seed=%08X host callee n=%u args=%08X %08X want=%08X %08X\n",
               seed, g_nc, (uint32_t)g_r0, (uint32_t)g_r1, w0, w1);
        return 0;
    }
    if (memcmp(h_a, sa, SLEN) != 0) {
        printf("FAIL seed=%08X host clobbered input\n", seed);
        return 0;
    }

    /* retail side */
    if (remu_poke(m, RA, sa, SLEN)) { printf("FAIL poke a\n"); return 0; }
    int rc = remu_call(m, entry, RA, 0, 0, 0);
    if (rc != 0) {
        printf("FAIL seed=%08X remu rc=%d stubs=%d%s\n", seed, rc,
               remu_stub_calls(m), remu_stub_log(m));
        return 0;
    }
    if (remu_stub_calls(m) != 1 ||
        strcmp(remu_stub_log(m), " 80023124") != 0) {
        printf("FAIL seed=%08X stub log%s\n", seed, remu_stub_log(m));
        return 0;
    }
    uint32_t want_v0 = remu_get_reg(m, 2);
    if (got != want_v0) {
        printf("FAIL seed=%08X retail v0=%08X host=%08X\n", seed, want_v0,
               got);
        return 0;
    }
    uint8_t ra[SLEN];
    if (remu_peek(m, RA, ra, SLEN)) { printf("FAIL peek a\n"); return 0; }
    if (memcmp(ra, sa, SLEN) != 0) {
        printf("FAIL seed=%08X retail clobbered input\n", seed);
        return 0;
    }
    return 1;
}

int main(void) {
    remu_t *m = remu_create();
    if (!m) { printf("FAIL create\n"); return 1; }
    uint32_t entry = remu_load_s(m, RETAIL_S);
    if (entry != 0x800BEF8Cu) { printf("FAIL load entry=%08X\n", entry); return 1; }

    int n = 0;
    uint32_t edges[] = { 0x00000000u, 0x00000001u, 0xFFFFFFFFu, 0x7FFFFFFFu,
                         0x80000000u, 0x00008000u, 0x80008000u, 0x7FFF7FFFu };
    for (unsigned i = 0; i < sizeof(edges) / sizeof(edges[0]); i++) {
        if (!check_one(m, entry, edges[i])) return 1;
        n++;
    }
    for (int b = 0; b < 32; b++) {
        if (!check_one(m, entry, 1u << b)) return 1;
        n++;
    }
    uint32_t s = 0x456789ABu;
    for (int i = 0; i < 200; i++) {
        s = s * 1103515245u + 12345u;
        if (!check_one(m, entry, s)) return 1;
        n++;
    }
    printf("DIFF 800BEF8C OK (%d seeds, retail-exact)\n", n);
    remu_destroy(m);
    return 0;
}
