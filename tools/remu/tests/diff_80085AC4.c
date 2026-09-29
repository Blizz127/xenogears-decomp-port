/* Differential test: host-compiled src/battle/main36.c func_80085AC4
 * vs the retail bytes (asm/battle/nonmatchings/main36/func_80085AC4.s)
 * executed by remu. Fails on any behavioral mismatch.
 *
 * func_80085AC4(a0): t = a0 & 0xFF, off = 368*t: copy the halfword
 * D_800CCD36[off] to D_800CCD34[off], zero D_800CCD68[off,off-4] and
 * D_800CCD74[off,off-4,off-8]. At t = 0 the countdown stores underflow
 * the bases by up to 8 bytes; retail genuinely writes there.
 *
 * Host side: each underflowed array is preceded by an 8-byte guard whose
 * adjacency is asserted at startup (the TU sees only unsized externs, so
 * no host bounds model is violated). Retail side: one unified span from
 * 8 below the lowest base, poked once per seed.
 *
 * Build (from repo root):
 *   tools/remu/run_diff.sh diff_80085AC4 src/battle/main36.c
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "remu.h"
#include "common.h"
void func_80085AC4(u8 a0);

#define REACH (368u * 255u + 2u) /* +1 tail byte: u16 load at row 255 reads off+1 */
/* TU externs the test must provide. The four D_800CCDxx bases OVERLAP in
 * retail (e.g. D_800CCD68-4 == D_800CCD34+48 == the same byte), and one
 * call stores through all four windows -- so the host mirrors must alias
 * the same way. All four symbols are offset-aliases into one host blob
 * (assembler `=` aliases; the evenness/layout assertion in main guards
 * the u16 traffic). */
__asm__(
".pushsection .bss,\"aw\",@nobits\n"
".p2align 1\n"
".globl H_span\nH_span:\n.space 93914\n"
".globl D_800CCD34\nD_800CCD34 = H_span + 8\n"
".globl D_800CCD36\nD_800CCD36 = H_span + 10\n"
".globl D_800CCD68\nD_800CCD68 = H_span + 60\n"
".globl D_800CCD74\nD_800CCD74 = H_span + 72\n"
".popsection\n"
);
extern u8 H_span[];
extern u8 D_800CCD34[];
extern u8 D_800CCD36[];
extern u8 D_800CCD68[];
extern u8 D_800CCD74[];

#define RETAIL_S "asm/battle/nonmatchings/main36/func_80085AC4.s"
#define C34_BASE 0x800CCD34u
#define C36_BASE 0x800CCD36u
#define C68_BASE 0x800CCD68u
#define C74_BASE 0x800CCD74u
#define SPAN_BASE (0x800CCD34u - 8u)
#define SPAN_LEN ((0x800CCD74u + REACH) - (0x800CCD34u - 8u))
#define C34_OFF (C34_BASE - SPAN_BASE)
#define C36_OFF (C36_BASE - SPAN_BASE)
#define C68_OFF (C68_BASE - SPAN_BASE)
#define C74_OFF (C74_BASE - SPAN_BASE)
_Static_assert(SPAN_LEN == 93914, "SPAN_LEN baked into the H_span asm above");
_Static_assert(C34_OFF == 8 && C36_OFF == 10 && C68_OFF == 60 && C74_OFF == 72,
               "alias offsets baked into the H_span asm above");

static uint32_t lcg(uint32_t *s) {
    *s = *s * 1103515245u + 12345u;
    return *s;
}

static int check_one(remu_t *m, uint32_t entry, uint32_t seed, uint32_t a0) {
    uint32_t x = seed ^ 0x85AC479u;
    static u8 span[SPAN_LEN];
    static u8 expect[SPAN_LEN];
    static u8 back[SPAN_LEN];
    for (uint32_t j = 0; j < SPAN_LEN; j++)
        span[j] = (u8)(lcg(&x) >> 16);

    /* one blob on both sides: the four windows alias it identically */
    memcpy(H_span, span, SPAN_LEN);
    if (remu_poke(m, SPAN_BASE, span, SPAN_LEN)) { printf("FAIL poke\n"); return 0; }

    /* independent expectation: direct multiply formulation */
    uint32_t t = a0 & 0xFFu;
    uint32_t off = t * 368u;
    memcpy(expect, span, SPAN_LEN);
    expect[C34_OFF + off] = span[C36_OFF + off];
    expect[C34_OFF + off + 1] = span[C36_OFF + off + 1];
    expect[C68_OFF + off] = 0;
    expect[C68_OFF + off + 1] = 0;
    expect[C68_OFF + off - 4] = 0;
    expect[C68_OFF + off - 3] = 0;
    expect[C74_OFF + off] = 0;
    expect[C74_OFF + off + 1] = 0;
    expect[C74_OFF + off - 4] = 0;
    expect[C74_OFF + off - 3] = 0;
    expect[C74_OFF + off - 8] = 0;
    expect[C74_OFF + off - 7] = 0;

    /* host side: the aliased windows see the identical overlap */
    func_80085AC4((u8)a0);
    if (memcmp(H_span, expect, SPAN_LEN) != 0) {
        printf("FAIL seed=%08X a0=%08X host mismatch\n", seed, a0);
        for (uint32_t k = 0; k < SPAN_LEN; k++)
            if (H_span[k] != expect[k]) {
                printf("  off %u host=%02X want=%02X\n", k, H_span[k], expect[k]);
                break;
            }
        return 0;
    }

    /* retail side */
    int rc = remu_call(m, entry, a0, 0, 0, 0);
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
    if (remu_peek(m, SPAN_BASE, back, SPAN_LEN)) { printf("FAIL peek\n"); return 0; }
    if (memcmp(back, expect, SPAN_LEN) != 0) {
        printf("FAIL seed=%08X a0=%08X retail mismatch\n", seed, a0);
        for (uint32_t k = 0; k < SPAN_LEN; k++)
            if (back[k] != expect[k]) {
                printf("  off %u retail=%02X want=%02X\n", k, back[k], expect[k]);
                break;
            }
        return 0;
    }
    return 1;
}

int main(void) {
    /* alias layout: even base for the u16 traffic, exact offsets */
    if (((uintptr_t)H_span & 1u) != 0 ||
        D_800CCD34 != H_span + C34_OFF ||
        D_800CCD36 != H_span + C36_OFF ||
        D_800CCD68 != H_span + C68_OFF ||
        D_800CCD74 != H_span + C74_OFF) {
        printf("FAIL host layout\n");
        return 1;
    }
    remu_t *m = remu_create();
    if (!m) { printf("FAIL create\n"); return 1; }
    uint32_t entry = remu_load_s(m, RETAIL_S);
    if (entry != 0x80085AC4u) { printf("FAIL load entry=%08X\n", entry); return 1; }
    uint32_t s = 0x85AC1234u;
    int n = 0;
    for (uint32_t tt = 0; tt < 256; tt++) {
        for (int r = 0; r < 4; r++) {
            s = s * 1103515245u + 12345u;
            /* high bits on a0 on odd rounds: both sides mask with 0xFF */
            uint32_t a0 = tt | ((r & 1) ? 0xCD00u : 0u);
            if (!check_one(m, entry, s, a0))
                return 1;
            n++;
        }
    }
    printf("DIFF 80085AC4 OK (%d seeds, retail-exact)\n", n);
    return 0;
}
