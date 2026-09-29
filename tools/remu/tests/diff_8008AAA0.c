/* Differential test: host-compiled src/battle/main44.c func_8008AAA0
 * vs the retail bytes (asm/battle/nonmatchings/main44/func_8008AAA0.s)
 * executed by remu. Fails on any behavioral mismatch.
 *
 * func_8008AAA0(value): splits value into 9 decimal digits
 * (d[0] = value/1e8 ... d[8] = value%10, divisor shrinking by the
 * 0xCCCCCCCD magic each round), stores them to D_800C3CF4[0..8], then
 * marks leading-zero suppression with 0xFF (if d[0]==0 it becomes 0xFF).
 * Pure leaf: no callees, no GTE/cop1/HW regs.
 *
 * Build (from repo root):
 *   tools/remu/run_diff.sh diff_8008AAA0 src/battle/main44.c
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "remu.h"

/* ---- host side: the real TU object (main44.c built with remu_shim.h so
 * INCLUDE_ASM is neutralized but every C body is identical). ---- */
#include "common.h"
void func_8008AAA0(u32 value);

/* TU externs the test must provide. */
static u8 backing[32];
__asm__(".globl D_800C3CF4\n.set D_800C3CF4, backing+0x10");
u32 ArchiveSetIndex(u32 a0, u32 a1) { (void)a0; (void)a1; return 0; }

#define RETAIL_S "asm/battle/nonmatchings/main44/func_8008AAA0.s"
#define DIG_BASE 0x800C3CF4u
#define DIG_LEN 9u

static int check_one(remu_t *m, uint32_t entry, uint32_t value,
                     const u8 poison[DIG_LEN]) {
    u8 want[DIG_LEN];

    /* host side */
    memcpy(backing + 0x10, poison, DIG_LEN);
    func_8008AAA0((u32)value);
    memcpy(want, backing + 0x10, DIG_LEN);

    /* retail side */
    if (remu_poke(m, DIG_BASE, poison, DIG_LEN)) { printf("FAIL poke\n"); return 0; }
    int rc = remu_call(m, entry, value, 0, 0, 0);
    if (rc != 0) {
        printf("FAIL value=%08X remu rc=%d stubs=%d%s\n", value, rc,
               remu_stub_calls(m), remu_stub_log(m));
        return 0;
    }
    if (remu_stub_calls(m) != 0) {
        printf("FAIL value=%08X unexpected stub calls%s\n", value,
               remu_stub_log(m));
        return 0;
    }
    static u8 got[DIG_LEN];
    if (remu_peek(m, DIG_BASE, got, DIG_LEN)) { printf("FAIL peek\n"); return 0; }
    if (memcmp(got, want, DIG_LEN) != 0) {
        printf("FAIL value=%08X digit mismatch\n", value);
        for (uint32_t k = 0; k < DIG_LEN; k++)
            if (got[k] != want[k])
                printf("  d[%u] retail=%02X host=%02X\n", k, got[k], want[k]);
        return 0;
    }
    return 1;
}

int main(void) {
    remu_t *m = remu_create();
    if (!m) { printf("FAIL create\n"); return 1; }
    uint32_t entry = remu_load_s(m, RETAIL_S);
    if (entry != 0x8008AAA0u) { printf("FAIL load entry=%08X\n", entry); return 1; }

    int n = 0;
    uint32_t edges[] = { 0x00000000u, 0x00000001u, 0x00000009u, 0x0000000Au,
                         0x00000063u, 0x00000064u, 0x05F5E0FFu, 0x05F5E100u,
                         0x05F5E101u, 0x3B9AC9FFu, 0x3B9ACA00u, 0x3B9ACA01u,
                         0xFFFFFFFFu, 0x7FFFFFFFu, 0x80000000u };
    for (unsigned i = 0; i < sizeof(edges) / sizeof(edges[0]); i++) {
        u8 poison[DIG_LEN];
        memset(poison, 0xA5, sizeof(poison));
        if (!check_one(m, entry, edges[i], poison)) return 1;
        n++;
    }
    /* powers of 10 and 2 (digit-boundary coverage) */
    uint32_t p = 1;
    for (int i = 0; i < 10; i++) {
        u8 poison[DIG_LEN];
        memset(poison, (u8)(0xC0 + i), sizeof(poison));
        if (!check_one(m, entry, p, poison)) return 1;
        n++;
        p *= 10u;
    }
    for (int b = 0; b < 32; b++) {
        u8 poison[DIG_LEN];
        memset(poison, (u8)b, sizeof(poison));
        if (!check_one(m, entry, 1u << b, poison)) return 1;
        n++;
    }
    /* deterministic pseudo-random sweep */
    uint32_t s = 0x8AAA08AAu;
    for (int i = 0; i < 200; i++) {
        u8 poison[DIG_LEN];
        s = s * 1103515245u + 12345u;
        for (uint32_t k = 0; k < DIG_LEN; k++) {
            s = s * 1103515245u + 12345u;
            poison[k] = (u8)(s >> 16);
        }
        if (!check_one(m, entry, s, poison)) return 1;
        n++;
    }
    printf("DIFF 8008AAA0 OK (%d seeds, retail-exact)\n", n);
    remu_destroy(m);
    return 0;
}
