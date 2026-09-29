/* Differential test: host-compiled src/battle/main73.c func_800B00D0
 * (zero nine words ending at D_800C3BCC) vs the retail bytes executed by
 * remu. Both sides clear [D-32, D+4).
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "remu.h"

void func_800B00D0(void);

/* Retail writes 32 bytes BEFORE the D_800C3BCC symbol (offsets -32..+3), so
 * the host symbol is aliased 32 bytes into a 40-byte backing store. The
 * emulator side seeds/reads the same window at the retail address. */
static u8 backing[40];
__asm__(".globl D_800C3BCC\n.set D_800C3BCC, backing+32");

#define DVAR 0x800C3BCCu

static int check_one(remu_t *m, uint32_t entry, uint32_t seed) {
    /* host side */
    for (int k = 0; k < 40; k++) backing[k] = (u8)((seed >> ((k % 4) * 8)) + k);
    func_800B00D0();
    /* retail side */
    uint8_t pat[36];
    for (int k = 0; k < 36; k++) pat[k] = (uint8_t)((seed >> ((k % 4) * 8)) + k);
    if (remu_poke(m, DVAR - 32, pat, sizeof pat)) { printf("FAIL poke\n"); return 0; }
    int rc = remu_call(m, entry, 0, 0, 0, 0);
    if (rc != 0) {
        printf("FAIL seed=%08X rc=%d stubs=%d%s\n", seed, rc,
               remu_stub_calls(m), remu_stub_log(m));
        return 0;
    }
    if (remu_stub_calls(m) != 0) {
        printf("FAIL seed=%08X stubs%s\n", seed, remu_stub_log(m));
        return 0;
    }
    uint8_t got[36];
    remu_peek(m, DVAR - 32, got, sizeof got);
    for (int k = 0; k < 36; k++) {
        if (got[k] != 0 || backing[k] != 0) {
            printf("FAIL seed=%08X off=%d retail=%02X host=%02X\n",
                   seed, k - 32, got[k], backing[k]);
            return 0;
        }
    }
    return 1;
}

int main(void) {
    remu_t *m = remu_create();
    if (!m) { printf("FAIL create\n"); return 1; }
    uint32_t entry = remu_load_s(m, "asm/battle/matchings/main73/func_800B00D0.s");
    if (entry != 0x800B00D0u) { printf("FAIL load entry=%08X\n", entry); return 1; }
    int n = 0;
    uint32_t edges[] = { 0, 1, 0xFFFFFFFFu, 0xDEADBEEFu, 0x80000000u, 0x7FFFFFFFu };
    for (unsigned i = 0; i < sizeof(edges) / sizeof(edges[0]); i++) {
        if (!check_one(m, entry, edges[i])) return 1;
        n++;
    }
    uint32_t s = 0x243F6A88u;
    for (int i = 0; i < 120; i++) {
        s = s * 1103515245u + 12345u;
        if (!check_one(m, entry, s)) return 1;
        n++;
    }
    printf("DIFF 800B00D0 OK (%d patterns, retail-exact)\n", n);
    remu_destroy(m);
    return 0;
}
