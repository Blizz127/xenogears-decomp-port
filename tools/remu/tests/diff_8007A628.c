/* Differential test: host-compiled src/battle/main14.c func_8007A628
 * vs the retail bytes (asm/battle/nonmatchings/main14/func_8007A628.s)
 * executed by remu. Fails on any behavioral mismatch.
 *
 * func_8007A628(a0, a1): 1 iff D_800D2DCC[a0 & 0xFF] != 0,
 * D_800C3EB7[28 * (a0 & 0xFF)] == 0, (D_800CCD64[368 * (a0 & 0xFF)] & 0xC002) == 0,
 * and ((a1 & 0xFF) != 0 or (D_800CCD6C[368 * (a0 & 0xFF)] & 0x20) == 0).
 *
 * The retail regions overlap (D_800D2DCC lies inside the D_800CCD64 span),
 * so the test models one unified span, poked once per seed; each host
 * mirror array is sliced out of that span.
 *
 * Build (from repo root):
 *   tools/remu/run_diff.sh diff_8007A628 src/battle/main14.c
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "remu.h"
#include "common.h"
u32 func_8007A628(u8 a0, u8 a1);

#define REACH (368u * 255u + 2u) /* +1 tail byte: u16 load at row 255 reads off+1 */
/* TU externs the test must provide (host mirrors, sliced from the span). */
u8 D_800D2DCC[256];
u8 D_800C3EB7[7141];
__attribute__((aligned(2))) u8 D_800CCD64[REACH];
__attribute__((aligned(2))) u8 D_800CCD6C[REACH];

#define RETAIL_S "asm/battle/nonmatchings/main14/func_8007A628.s"
#define E3_BASE 0x800D2DCCu
#define E3_LEN 256u
#define B7_BASE 0x800C3EB7u
#define B7_LEN 7141u
#define C64_BASE 0x800CCD64u
#define C64_LEN REACH
#define C6C_BASE 0x800CCD6Cu
#define C6C_LEN REACH
#define SPAN_BASE 0x800C3EB7u
#define SPAN_LEN (0x800E3BFEu - 0x800C3EB7u)
#define E3_OFF (E3_BASE - SPAN_BASE)
#define B7_OFF (B7_BASE - SPAN_BASE)
#define C64_OFF (C64_BASE - SPAN_BASE)
#define C6C_OFF (C6C_BASE - SPAN_BASE)

static uint32_t lcg(uint32_t *s) {
    *s = *s * 1103515245u + 12345u;
    return *s;
}

static int check_one(remu_t *m, uint32_t entry, uint32_t seed,
                     uint32_t a0, uint32_t a1, int verify_mem) {
    uint32_t x = seed ^ 0x7A62879u;
    static u8 span[SPAN_LEN];
    for (uint32_t j = 0; j < SPAN_LEN; j++)
        span[j] = (u8)(lcg(&x) >> 16);

    memcpy(D_800D2DCC, span + E3_OFF, E3_LEN);
    memcpy(D_800C3EB7, span + B7_OFF, B7_LEN);
    memcpy(D_800CCD64, span + C64_OFF, C64_LEN);
    memcpy(D_800CCD6C, span + C6C_OFF, C6C_LEN);
    if (remu_poke(m, SPAN_BASE, span, SPAN_LEN)) { printf("FAIL poke\n"); return 0; }

    /* independent expectation: direct multiplies, not the TU shift idiom */
    uint32_t t = a0 & 0xFFu;
    uint32_t h64 = (uint32_t)span[C64_OFF + t * 368u] |
                   ((uint32_t)span[C64_OFF + t * 368u + 1] << 8);
    uint32_t h6c = (uint32_t)span[C6C_OFF + t * 368u] |
                   ((uint32_t)span[C6C_OFF + t * 368u + 1] << 8);
    uint32_t want;
    if (span[E3_OFF + t] == 0)
        want = 0;
    else if (span[B7_OFF + t * 28u] != 0)
        want = 0;
    else if ((h64 & 0xC002u) != 0)
        want = 0;
    else if ((a1 & 0xFFu) != 0)
        want = 1;
    else
        want = ((h6c & 0x20u) == 0);

    uint32_t got = func_8007A628((u8)a0, (u8)a1);
    int rc = remu_call(m, entry, a0, a1, 0, 0);
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
    uint32_t rv0 = remu_get_reg(m, 2);
    if (got != want || rv0 != want) {
        printf("FAIL seed=%08X a0=%08X a1=%08X host=%08X retail=%08X want=%08X\n",
               seed, a0, a1, got, rv0, want);
        return 0;
    }
    if (verify_mem) {
        /* pure loads: retail memory must be untouched */
        static u8 back[SPAN_LEN];
        if (remu_peek(m, SPAN_BASE, back, SPAN_LEN) ||
            memcmp(back, span, SPAN_LEN) != 0) {
            printf("FAIL seed=%08X retail store\n", seed);
            for (uint32_t k = 0; k < SPAN_LEN; k++)
                if (back[k] != span[k]) {
                    printf("  off %u retail=%02X want=%02X\n", k, back[k], span[k]);
                    break;
                }
            return 0;
        }
    }
    return 1;
}

int main(void) {
    remu_t *m = remu_create();
    if (!m) { printf("FAIL create\n"); return 1; }
    uint32_t entry = remu_load_s(m, RETAIL_S);
    if (entry != 0x8007A628u) { printf("FAIL load entry=%08X\n", entry); return 1; }
    static const uint32_t a1s[] = { 0, 1, 2, 0x7Fu, 0x80u, 0xFFu, 0x100u, 0x1FFu, 0x1234u };
    uint32_t s = 0x7A628234u;
    for (uint32_t a0t = 0; a0t < 256; a0t++) {
        for (int r = 0; r < 4; r++) {
            s = s * 1103515245u + 12345u;
            uint32_t seed = s;
            /* high bits on a0 on odd rounds: both sides mask with 0xFF */
            uint32_t a0 = a0t | ((r & 1) ? 0xAB00u : 0u);
            for (unsigned k = 0; k < 10; k++) {
                uint32_t a1 = (k < 9) ? a1s[k] : (seed ^ 0xFF00FF00u);
                if (!check_one(m, entry, seed ^ (k * 0x9E3779B9u), a0, a1, k == 0))
                    return 1;
            }
        }
    }
    printf("DIFF 8007A628 OK (%d seeds, retail-exact)\n", 256 * 4 * 10);
    return 0;
}
