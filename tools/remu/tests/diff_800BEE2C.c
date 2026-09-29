/* Differential test: host-compiled src/battle/main125.c func_800BEE2C
 * vs the retail bytes (asm/battle/nonmatchings/main125/func_800BEE2C.s)
 * executed by remu. Fails on any behavioral mismatch.
 *
 * func_800BEE2C(a0, a1, a2): buf = HeapAlloc(0x1000, 1);
 *   func_800AA320(a0 & 0xFFFF, a1 & 0xFFFF, a2); HeapFree(buf).
 * Retail runs the callee on the heap block as a scratch stack
 * (sp switched to buf+0xF98, then restored); on host that is a no-op.
 *
 * Retail side (remu stubs force v0=0 and return): assert rc==0, the exact
 * 3-stub sequence HeapAlloc -> func_800AA320 -> HeapFree, and sp restored.
 * Host side: stub HeapAlloc/HeapFree/func_800AA320 record the call; assert
 * alloc(0x1000, 1), callee args == masked inputs, free(ptr)==alloc return.
 *
 * Build (from repo root):
 *   tools/remu/run_diff.sh diff_800BEE2C src/battle/main125.c
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "remu.h"

/* ---- host side: the real TU object (main125.c built with remu_shim.h so
 * INCLUDE_ASM is neutralized but every C body is identical). ---- */
#include "common.h"
void func_800BEE2C(u32 arg0, u32 arg1, u32 arg2);

/* TU externs the test must provide. */
static unsigned g_allocs, g_frees;
static u_int g_allocSize, g_allocFlags;
static void *g_allocRet, *g_freePtr;
static u32 g_c0, g_c1, g_c2;
static unsigned g_callees;

void *HeapAlloc(u_int allocSize, u_int allocFlags) {
    g_allocs++;
    g_allocSize = allocSize;
    g_allocFlags = allocFlags;
    g_allocRet = malloc(allocSize ? allocSize : 1);
    return g_allocRet;
}
u_int HeapFree(void *pMem) {
    g_frees++;
    g_freePtr = pMem;
    free(pMem);
    return 0;
}
void func_800AA320(u32 a0, u32 a1, u32 a2) {
    g_callees++;
    g_c0 = a0; g_c1 = a1; g_c2 = a2;
}

#define RETAIL_S "asm/battle/nonmatchings/main125/func_800BEE2C.s"
#define HEAPALLOC_PC 0x80031BDCu
#define AA320_PC 0x800AA320u
#define HEAPFREE_PC 0x800320E8u
#define SP_INIT 0x801FFF00u

static uint32_t lcg(uint32_t *s) {
    *s = *s * 1103515245u + 12345u;
    return *s;
}

static int check_one(remu_t *m, uint32_t entry, uint32_t a0, uint32_t a1,
                     uint32_t a2) {
    char want_log[64];

    /* host side */
    g_allocs = g_frees = g_callees = 0;
    g_allocRet = g_freePtr = NULL;
    func_800BEE2C(a0, a1, a2);
    if (g_allocs != 1 || g_allocSize != 0x1000u || g_allocFlags != 1u) {
        printf("FAIL a0=%08X host alloc n=%u size=%X flags=%X\n", a0,
               g_allocs, g_allocSize, g_allocFlags);
        return 0;
    }
    if (g_callees != 1 || g_c0 != (a0 & 0xFFFFu) ||
        g_c1 != (a1 & 0xFFFFu) || g_c2 != a2) {
        printf("FAIL a0=%08X host callee n=%u args=%08X %08X %08X\n", a0,
               g_callees, g_c0, g_c1, g_c2);
        return 0;
    }
    if (g_frees != 1 || g_freePtr != g_allocRet || g_allocRet == NULL) {
        printf("FAIL a0=%08X host free n=%u ptr=%p alloc=%p\n", a0, g_frees,
               g_freePtr, g_allocRet);
        return 0;
    }

    /* retail side */
    snprintf(want_log, sizeof(want_log), " %08X %08X %08X", HEAPALLOC_PC,
             AA320_PC, HEAPFREE_PC);
    int rc = remu_call(m, entry, a0, a1, a2, 0);
    if (rc != 0) {
        printf("FAIL a0=%08X remu rc=%d stubs=%d%s\n", a0, rc,
               remu_stub_calls(m), remu_stub_log(m));
        return 0;
    }
    if (remu_stub_calls(m) != 3 ||
        strcmp(remu_stub_log(m), want_log) != 0) {
        printf("FAIL a0=%08X stub seq%s want%s\n", a0, remu_stub_log(m),
               want_log);
        return 0;
    }
    if (remu_get_reg(m, 29) != SP_INIT) {
        printf("FAIL a0=%08X sp not restored: %08X\n", a0,
               remu_get_reg(m, 29));
        return 0;
    }
    return 1;
}

int main(void) {
    remu_t *m = remu_create();
    if (!m) { printf("FAIL create\n"); return 1; }
    uint32_t entry = remu_load_s(m, RETAIL_S);
    if (entry != 0x800BEE2Cu) { printf("FAIL load entry=%08X\n", entry); return 1; }

    int n = 0;
    uint32_t edges[] = { 0x00000000u, 0x00000001u, 0xFFFFFFFFu, 0x7FFFFFFFu,
                         0x80000000u, 0x0000FFFFu, 0xFFFF0000u, 0x12345678u };
    for (unsigned i = 0; i < sizeof(edges) / sizeof(edges[0]); i++) {
        uint32_t e = edges[i];
        if (!check_one(m, entry, e, ~e, e ^ 0xA5A5A5A5u)) return 1;
        n++;
    }
    for (int b = 0; b < 32; b++) {
        uint32_t e = 1u << b;
        if (!check_one(m, entry, e, e >> 1, e | 0xFF000000u)) return 1;
        n++;
    }
    uint32_t s = 0x12345678u;
    for (int i = 0; i < 200; i++) {
        uint32_t a0 = lcg(&s), a1 = lcg(&s), a2 = lcg(&s);
        if (!check_one(m, entry, a0, a1, a2)) return 1;
        n++;
    }
    printf("DIFF 800BEE2C OK (%d seeds, retail-exact)\n", n);
    remu_destroy(m);
    return 0;
}
