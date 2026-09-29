/* Differential test: host-compiled src/battle/mainc25.c func_8007D0CC vs the
 * retail bytes executed by remu. row[p[1]] = D_800D2CB0[i] for the first
 * i < 48 with D_800D2CE0[i] == p[2] (else 0); row = D_800D3430+((idx&FF)<<6).
 * The full 64-byte row is compared (only row[p[1]] may change).
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "remu.h"

void func_8007D0CC(u8 **pp, s32 idx);

/* TU externs provided by the test. */
u8 D_800D3430[18432];
u8 D_800D2CE0[48];
u8 D_800D2CB0[48];

#define DBASE 0x800D3430u
#define E0BASE 0x800D2CE0u
#define B0BASE 0x800D2CB0u
#define PBOX 0x80181000u
#define PBYTES (PBOX + 0x10u)

static int check_one(remu_t *m, uint32_t entry, int idx,
                     uint8_t p1, uint8_t p2,
                     const uint8_t *e0, const uint8_t *b0,
                     const uint8_t *rowpat) {
    uint32_t row = (uint32_t)((idx & 0xFF) << 6);
    /* host side */
    memcpy(D_800D2CE0, e0, 48);
    memcpy(D_800D2CB0, b0, 48);
    memcpy(D_800D3430 + row, rowpat, 64);
    static u8 *host_pp_storage;
    static u8 host_bytes[8];
    host_bytes[1] = p1; host_bytes[2] = p2;
    host_pp_storage = host_bytes;
    u8 **hpp = &host_pp_storage;
    func_8007D0CC(hpp, idx);
    /* retail side */
    if (remu_poke(m, E0BASE, e0, 48)
        || remu_poke(m, B0BASE, b0, 48)
        || remu_poke(m, DBASE + row, rowpat, 64)) {
        printf("FAIL poke\n");
        return 0;
    }
    uint32_t pptr = PBYTES;
    if (remu_poke(m, PBOX, &pptr, 4)) { printf("FAIL poke p\n"); return 0; }
    uint8_t pb[8]; memset(pb, 0, sizeof pb);
    pb[1] = p1; pb[2] = p2;
    if (remu_poke(m, PBYTES, pb, sizeof pb)) { printf("FAIL poke pb\n"); return 0; }
    int rc = remu_call(m, entry, PBOX, (uint32_t)idx, 0, 0);
    if (rc != 0) {
        printf("FAIL idx=%d rc=%d stubs=%d%s\n", idx, rc,
               remu_stub_calls(m), remu_stub_log(m));
        return 0;
    }
    if (remu_stub_calls(m) != 0) {
        printf("FAIL idx=%d stubs%s\n", idx, remu_stub_log(m));
        return 0;
    }
    uint8_t got[64];
    if (remu_peek(m, DBASE + row, got, 64)) { printf("FAIL peek\n"); return 0; }
    if (memcmp(got, D_800D3430 + row, 64) != 0) {
        printf("FAIL idx=%d p=(%u,%u) row mismatch\n", idx, p1, p2);
        for (int k = 0; k < 64; k++)
            if (got[k] != D_800D3430[row + k])
                printf("  off %d retail=%02X host=%02X\n", k, got[k], D_800D3430[row + k]);
        return 0;
    }
    return 1;
}

int main(void) {
    remu_t *m = remu_create();
    if (!m) { printf("FAIL create\n"); return 1; }
    uint32_t entry = remu_load_s(m, "asm/battle/nonmatchings/main25/func_8007D0CC.s");
    if (entry != 0x8007D0CCu) { printf("FAIL load entry=%08X\n", entry); return 1; }
    int n = 0;
    uint8_t e0[48], b0[48], rowpat[64];
    int idxe[] = { 0, 1, 63, 64, 255, 256, -1, 0x1FF };
    /* structured cases: match at 0 / 47 / middle, no match */
    for (unsigned ci = 0; ci < 4; ci++) {
        for (unsigned i = 0; i < sizeof(idxe) / sizeof(idxe[0]); i++) {
            memset(e0, 0xAA, 48);
            for (int k = 0; k < 48; k++) b0[k] = (uint8_t)(k * 3 + 1);
            memset(rowpat, 0x55, 64);
            uint8_t p1 = (uint8_t)(7 + ci * 61), p2;
            if (ci == 0) { p2 = 0x11; e0[0] = 0x11; }
            else if (ci == 1) { p2 = 0x22; e0[47] = 0x22; }
            else if (ci == 2) { p2 = 0x33; e0[20] = 0x33; }
            else { p2 = 0x44; /* absent */ }
            if (!check_one(m, entry, idxe[i], p1, p2, e0, b0, rowpat)) return 1;
            n++;
        }
    }
    /* randomized */
    uint32_t s = 0xD0CCD0CCu;
    for (int i = 0; i < 120; i++) {
        for (int k = 0; k < 48; k++) {
            s = s * 1103515245u + 12345u; e0[k] = (uint8_t)(s >> 16);
            s = s * 1103515245u + 12345u; b0[k] = (uint8_t)(s >> 16);
        }
        for (int k = 0; k < 64; k++) {
            s = s * 1103515245u + 12345u; rowpat[k] = (uint8_t)(s >> 16);
        }
        s = s * 1103515245u + 12345u;
        int idx = (int)(s >> 7);
        uint8_t p1 = (uint8_t)(s >> 3), p2 = (uint8_t)(s >> 15);
        if (!check_one(m, entry, idx, p1, p2, e0, b0, rowpat)) return 1;
        n++;
    }
    printf("DIFF 8007D0CC OK (%d checks, retail-exact)\n", n);
    remu_destroy(m);
    return 0;
}
