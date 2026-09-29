/* Differential test: host-compiled src/battle/main72.c func_800AA820
 * vs the retail bytes (asm/battle/nonmatchings/main72/func_800AA820.s)
 * executed by remu. Fails on any behavioral mismatch.
 *
 * func_800AA820(arg0): dispatch to a func_800A34xx/35xx entry point:
 *   2 -> 0x800A3578, 1 -> 0x800A3514, 3 -> 0x800A35C8,
 *   anything else -> 0x800A3490 (the < 3 test is signed).
 * Retail v0 is the entry address; the host body returns the address of a
 * host stub standing in for that entry. Both sides are mapped to a
 * 0..3 selector (0=490, 1=514, 2=578, 3=5C8) and the selectors compared.
 *
 * Build (from repo root):
 *   tools/remu/run_diff.sh diff_800AA820 src/battle/main72.c
 */
#include <stdio.h>
#include <stdint.h>

#include "remu.h"

/* ---- host side: the real TU object (main72.c built with remu_shim.h so
 * INCLUDE_ASM is neutralized but every C body is identical). ---- */
#include "common.h"
void *func_800AA820(s32 arg0);

/* Retail entry points, stood in for by host stubs. */
void func_800A3490(void) {}
void func_800A3514(void) {}
void func_800A3578(void) {}
void func_800A35C8(void) {}

#define RETAIL_S "asm/battle/nonmatchings/main72/func_800AA820.s"

static int host_sel(void *p) {
    if (p == (void *)func_800A3490) return 0;
    if (p == (void *)func_800A3514) return 1;
    if (p == (void *)func_800A3578) return 2;
    if (p == (void *)func_800A35C8) return 3;
    return -1;
}

static int retail_sel(uint32_t v0) {
    switch (v0) {
    case 0x800A3490u: return 0;
    case 0x800A3514u: return 1;
    case 0x800A3578u: return 2;
    case 0x800A35C8u: return 3;
    default: return -1;
    }
}

static int check_one(remu_t *m, uint32_t entry, int32_t arg) {
    /* host side */
    int h = host_sel(func_800AA820(arg));
    if (h < 0) {
        printf("FAIL arg=%d host returned unknown pointer\n", arg);
        return 0;
    }

    /* retail side */
    int rc = remu_call(m, entry, (uint32_t)arg, 0, 0, 0);
    if (rc != 0) {
        printf("FAIL arg=%d remu rc=%d stubs=%d%s\n", arg, rc,
               remu_stub_calls(m), remu_stub_log(m));
        return 0;
    }
    if (remu_stub_calls(m) != 0) {
        printf("FAIL arg=%d unexpected stub calls%s\n", arg,
               remu_stub_log(m));
        return 0;
    }
    int r = retail_sel(remu_get_reg(m, 2));
    if (r < 0) {
        printf("FAIL arg=%d retail v0=%08X unknown\n", arg, remu_get_reg(m, 2));
        return 0;
    }
    if (h != r) {
        printf("FAIL arg=%d host_sel=%d retail_sel=%d\n", arg, h, r);
        return 0;
    }
    return 1;
}

int main(void) {
    remu_t *m = remu_create();
    if (!m) { printf("FAIL create\n"); return 1; }
    uint32_t entry = remu_load_s(m, RETAIL_S);
    if (entry != 0x800AA820u) { printf("FAIL load entry=%08X\n", entry); return 1; }

    int n = 0;
    /* edge args: dispatch points, signed boundaries, extremes */
    int32_t edges[] = { 0, 1, 2, 3, 4, -1, -2, -100, 5, 100,
                        0x7FFFFFFF, (int32_t)0x80000000, -2147483647 - 1 + 0 };
    for (unsigned i = 0; i < sizeof(edges) / sizeof(edges[0]); i++) {
        if (!check_one(m, entry, edges[i])) return 1;
        n++;
    }
    /* all single bits (and their negations) */
    for (int b = 0; b < 32; b++) {
        if (!check_one(m, entry, (int32_t)(1u << b))) return 1;
        n++;
        if (!check_one(m, entry, (int32_t)(~(1u << b)))) return 1;
        n++;
    }
    /* deterministic pseudo-random sweep */
    uint32_t s = 0xA820A820u;
    for (int i = 0; i < 200; i++) {
        s = s * 1103515245u + 12345u;
        if (!check_one(m, entry, (int32_t)s)) return 1;
        n++;
    }
    printf("DIFF 800AA820 OK (%d seeds, retail-exact)\n", n);
    remu_destroy(m);
    return 0;
}
