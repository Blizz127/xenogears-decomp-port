/* Differential test: host-compiled src/battle/mainc29.c table row ops vs the
 * retail bytes executed by remu.
 *   func_8007E6A0: T[p[3]] = T[p[1]] + T[p[2]]
 *   func_8007E6F0: T[p[3]] = T[p[1]] - T[p[2]]
 *   func_8007E740: T[p[1]] = T[p[1]] * p[2]   (low 32 bits)
 * where T = D_800D3410 + ((idx & 0xFF) << 6), p = *pp (byte indices).
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "remu.h"

void func_8007E6A0(u8 **pp, s32 idx);
void func_8007E6F0(u8 **pp, s32 idx);
void func_8007E740(u8 **pp, s32 idx);

/* TU extern provided by the test (row table base). */
u8 D_800D3410[18432];

#define DBASE 0x800D3410u
#define PBOX 0x80181000u
#define PBYTES (PBOX + 0x10u)
#define REGION (256u * 64u + 1088u)

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
                     int idx, uint8_t b1, uint8_t b2, uint8_t b3) {
    uint32_t row = (uint32_t)((idx & 0xFF) << 6);
    /* host side */
    memcpy(D_800D3410, pattern, sizeof pattern);
    static u8 *host_pp_storage;
    static u8 host_bytes[8];
    host_bytes[1] = b1; host_bytes[2] = b2; host_bytes[3] = b3;
    host_pp_storage = host_bytes;
    u8 **hpp = &host_pp_storage;
    fn(hpp, idx);
    uint32_t *ht = (uint32_t *)(D_800D3410 + row);
    uint32_t want = (which == 2) ? ht[b1] : ht[b3];
    /* retail side */
    if (remu_poke(m, DBASE, pattern, sizeof pattern)) { printf("FAIL poke\n"); return 0; }
    uint32_t pptr = PBYTES;
    if (remu_poke(m, PBOX, &pptr, 4)) { printf("FAIL poke p\n"); return 0; }
    uint8_t pb[8]; memset(pb, 0, sizeof pb);
    pb[1] = b1; pb[2] = b2; pb[3] = b3;
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
    uint32_t off = row + ((which == 2 ? b1 : b3) * 4u);
    if (remu_peek(m, DBASE + off, &got, 4)) { printf("FAIL peek\n"); return 0; }
    if (got != want) {
        printf("FAIL op=%d idx=%d b=(%u,%u,%u) retail=%08X host=%08X\n",
               which, idx, b1, b2, b3, got, want);
        return 0;
    }
    return 1;
}

int main(void) {
    remu_t *m = remu_create();
    if (!m) { printf("FAIL create\n"); return 1; }
    uint32_t e_add = remu_load_s(m, "asm/battle/matchings/main29/func_8007E6A0.s");
    uint32_t e_sub = remu_load_s(m, "asm/battle/matchings/main29/func_8007E6F0.s");
    uint32_t e_mul = remu_load_s(m, "asm/battle/matchings/main29/func_8007E740.s");
    if (e_add != 0x8007E6A0u || e_sub != 0x8007E6F0u || e_mul != 0x8007E740u) {
        printf("FAIL load\n");
        return 1;
    }
    int n = 0;
    int idxe[] = { 0, 1, 63, 64, 127, 255, 256, -1, 0x1FF, 0x7FFFFFFF, (int)0x80000000 };
    uint8_t be[] = { 0, 1, 2, 3, 4, 63, 64, 127, 255 };
    make_pattern(0x12345678u);
    for (unsigned i = 0; i < sizeof(idxe) / sizeof(idxe[0]); i++) {
        for (unsigned k = 0; k < sizeof(be); k++) {
            if (!check_one(m, e_add, func_8007E6A0, 0, idxe[i], be[k], be[(k + 3) % 9], be[(k + 5) % 9])) return 1;
            if (!check_one(m, e_sub, func_8007E6F0, 1, idxe[i], be[k], be[(k + 3) % 9], be[(k + 5) % 9])) return 1;
            if (!check_one(m, e_mul, func_8007E740, 2, idxe[i], be[k], be[(k + 3) % 9], 0)) return 1;
            n += 3;
        }
    }
    uint32_t s = 0xABCDEF01u;
    for (int i = 0; i < 60; i++) {
        make_pattern(s);
        for (int r = 0; r < 3; r++) {
            s = s * 1103515245u + 12345u;
            int idx = (int)(s >> 8);
            uint8_t x1 = (uint8_t)(s >> 3), x2 = (uint8_t)(s >> 11), x3 = (uint8_t)(s >> 19);
            if (!check_one(m, e_add, func_8007E6A0, 0, idx, x1, x2, x3)) return 1;
            if (!check_one(m, e_sub, func_8007E6F0, 1, idx, x1, x2, x3)) return 1;
            if (!check_one(m, e_mul, func_8007E740, 2, idx, x1, x2, x3)) return 1;
            n += 3;
        }
    }
    printf("DIFF 8007E6A0/E6F0/E740 OK (%d checks, retail-exact)\n", n);
    remu_destroy(m);
    return 0;
}
