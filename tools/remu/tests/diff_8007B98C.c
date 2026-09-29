/* Differential test: host-compiled src/battle/main22.c row copies vs the
 * retail bytes executed by remu.
 *   func_8007B98C: u16 T[p[2]] = T[p[1]]           (base D_800D3420)
 *   func_8007B9C8: u32 T[p[2]] = T[p[1]]           (base D_800D3410)
 * T-row = base + ((idx & 0xFF) << 6), p = *pp (byte indices).
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "remu.h"

void func_8007B98C(u8 **pp, s32 idx);
void func_8007B9C8(u8 **pp, s32 idx);

/* TU externs provided by the test (row table bases). */
u8 D_800D3420[18432];
u8 D_800D3410[18432];

#define B20 0x800D3420u
#define B10 0x800D3410u
#define PBOX 0x80181000u
#define PBYTES (PBOX + 0x10u)
#define REGION 18432u

static uint8_t pattern[18432];

static void make_pattern(uint32_t seed) {
    uint32_t s = seed;
    for (unsigned i = 0; i < sizeof pattern / 4; i++) {
        s = s * 1103515245u + 12345u;
        pattern[i * 4] = (uint8_t)s;
        pattern[i * 4 + 1] = (uint8_t)(s >> 8);
        pattern[i * 4 + 2] = (uint8_t)(s >> 16);
        pattern[i * 4 + 3] = (uint8_t)(s >> 24);
    }
}

typedef void (*opfn)(u8 **, s32);

static int check_one(remu_t *m, uint32_t entry, opfn fn, int which,
                     uint32_t base, u8 *hbase,
                     int idx, uint8_t b1, uint8_t b2) {
    uint32_t row = (uint32_t)((idx & 0xFF) << 6);
    /* host side */
    memcpy(hbase, pattern, sizeof pattern);
    static u8 *host_pp_storage;
    static u8 host_bytes[8];
    host_bytes[1] = b1; host_bytes[2] = b2;
    host_pp_storage = host_bytes;
    u8 **hpp = &host_pp_storage;
    fn(hpp, idx);
    uint32_t want = 0;
    if (which == 0) want = ((uint16_t *)(hbase + row))[b2];
    else if (which == 1) want = ((uint32_t *)(hbase + row))[b2];
    else want = ((uint16_t *)(hbase + row))[b2];
    /* retail side */
    if (remu_poke(m, base, pattern, sizeof pattern)) { printf("FAIL poke\n"); return 0; }
    uint32_t pptr = PBYTES;
    if (remu_poke(m, PBOX, &pptr, 4)) { printf("FAIL poke p\n"); return 0; }
    uint8_t pb[8]; memset(pb, 0, sizeof pb);
    pb[1] = b1; pb[2] = b2;
    if (remu_poke(m, PBYTES, pb, sizeof pb)) { printf("FAIL poke pb\n"); return 0; }
    int rc = remu_call(m, entry, PBOX, (uint32_t)idx, 0, 0);
    if (rc != 0) {
        printf("FAIL op=%d idx=%d rc=%d stubs=%d%s\n", which, idx, rc,
               remu_stub_calls(m), remu_stub_log(m));
        return 0;
    }
    if (remu_stub_calls(m) != 0) {
        printf("FAIL op=%d stubs%s\n", which, remu_stub_log(m));
        return 0;
    }
    uint32_t got = 0;
    uint32_t width = (which == 1) ? 4u : 2u;
    if (remu_peek(m, base + row + b2 * width, &got, width)) { printf("FAIL peek\n"); return 0; }
    if (got != want) {
        printf("FAIL op=%d idx=%d b=(%u,%u) retail=%08X host=%08X\n",
               which, idx, b1, b2, got, want);
        return 0;
    }
    return 1;
}

int main(void) {
    remu_t *m = remu_create();
    if (!m) { printf("FAIL create\n"); return 1; }
    uint32_t e16 = remu_load_s(m, "asm/battle/matchings/main22_q1/func_8007B98C.s");
    uint32_t e32 = remu_load_s(m, "asm/battle/matchings/main22_q1/func_8007B9C8.s");
    if (e16 != 0x8007B98Cu || e32 != 0x8007B9C8u) {
        printf("FAIL load\n");
        return 1;
    }
    int n = 0;
    int idxe[] = { 0, 1, 63, 64, 127, 255, 256, -1, 0x1FF, 0x7FFFFFFF, (int)0x80000000 };
    uint8_t be[] = { 0, 1, 2, 3, 7, 63, 64, 127, 255 };
    make_pattern(0x55AA55AAu);
    for (unsigned i = 0; i < sizeof(idxe) / sizeof(idxe[0]); i++) {
        for (unsigned k = 0; k < sizeof(be); k++) {
            uint8_t x1 = be[k], x2 = be[(k + 4) % 9];
            if (!check_one(m, e16, func_8007B98C, 0, B20, D_800D3420, idxe[i], x1, x2)) return 1;
            if (!check_one(m, e32, func_8007B9C8, 1, B10, D_800D3410, idxe[i], x1, x2)) return 1;
            n += 2;
        }
    }
    uint32_t s = 0x10ADBEEFu;
    for (int i = 0; i < 50; i++) {
        make_pattern(s);
        for (int r = 0; r < 3; r++) {
            s = s * 1103515245u + 12345u;
            int idx = (int)(s >> 9);
            uint8_t x1 = (uint8_t)(s >> 4), x2 = (uint8_t)(s >> 13);
            if (!check_one(m, e16, func_8007B98C, 0, B20, D_800D3420, idx, x1, x2)) return 1;
            if (!check_one(m, e32, func_8007B9C8, 1, B10, D_800D3410, idx, x1, x2)) return 1;
            n += 2;
        }
    }
    printf("DIFF 8007B98C/B9C8 OK (%d checks, retail-exact)\n", n);
    remu_destroy(m);
    return 0;
}
