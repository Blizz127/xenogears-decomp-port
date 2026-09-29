/* Differential test: host-compiled src/battle/mainasm_10FF4.c func_80080AE4
 * (12-slot min search) vs the retail bytes executed by remu.
 *
 * Layout: B = D_800D2DD7 (init count, constrained to 0..10; 11 is not a
 * valid start because the retail wrap fires at 11 before it can re-equal B), E =
 * D_800D2DD8[i] (slot keys), halves at B+0x2F+v*2. Both sides share one
 * backing image (host symbols aliased into `backing`, emu poked at the
 * retail addresses). Seeds guarantee >= 1 qualifying slot: retail returns
 * the caller's leftover $t8 when none qualifies, which no host build can
 * reproduce (documented in the TU).
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "remu.h"

s32 func_80080AE4(u8 target);

/* Host symbols aliased into one backing image (halves are reached by raw
 * pointer arithmetic from &D_800D2DD7, so they need real adjacent backing). */
static u8 backing[0x400];
__asm__(".globl D_800D2DD7\n.set D_800D2DD7, backing+1");
__asm__(".globl D_800D2DD8\n.set D_800D2DD8, backing+2");

#define BVAR 0x800D2DD7u
#define IMG 0x330u

static int check_one(remu_t *m, uint32_t entry, uint8_t target,
                     uint8_t init, const uint8_t ev[12],
                     const uint8_t *img) {
    /* host side */
    memcpy(backing + 1, img, IMG);
    uint8_t want = func_80080AE4(target);
    /* retail side */
    if (remu_poke(m, BVAR, img, IMG)) { printf("FAIL poke\n"); return 0; }
    int rc = remu_call(m, entry, target, 0, 0, 0);
    if (rc != 0) {
        printf("FAIL target=%02X rc=%d stubs=%d%s\n", target, rc,
               remu_stub_calls(m), remu_stub_log(m));
        return 0;
    }
    if (remu_stub_calls(m) != 0) {
        printf("FAIL target=%02X stubs%s\n", target, remu_stub_log(m));
        return 0;
    }
    uint8_t got = (uint8_t)remu_get_reg(m, 2);
    if (got != want) {
        printf("FAIL target=%02X init=%u retail=%02X host=%02X\n",
               target, init, got, want);
        return 0;
    }
    (void)ev;
    return 1;
}

/* Build an image: [0]=init, [1..12]=E keys, halves at [0x2F+v*2] (s16 LE).
 * `mode`: 0 = all halves zero (everything qualifies), 1 = descending low
 * bytes, 2 = random halves (slot 0 forced to 0 to guarantee a qualifier),
 * 3 = negative halves present. */
static void make_img(uint8_t *img, uint32_t *s, uint8_t init,
                     const uint8_t ev[12], int mode) {
    memset(img, 0xAA, IMG);
    img[0] = init;
    for (int k = 0; k < 12; k++) img[1 + k] = ev[k];
    /* distinct key set for half placement */
    uint8_t seen[256]; memset(seen, 0, sizeof seen);
    for (int k = 0; k < 12; k++) {
        uint8_t v = ev[k];
        if (seen[v]) continue;
        seen[v] = 1;
        int16_t h;
        if (mode == 0) h = 0;
        else if (mode == 1) h = (int16_t)((11 - k) * 16);
        else if (mode == 2) h = (k == 0) ? 0 : (int16_t)((*s >> 9) & 0xFFFFu);
        else h = (k % 3 == 0) ? (int16_t)(int32_t)0xFFFF8000u : (int16_t)(k * 7);
        if (mode == 2) { *s = *s * 1103515245u + 12345u; }
        img[0x2F + v * 2] = (uint8_t)h;
        img[0x2F + v * 2 + 1] = (uint8_t)((uint16_t)h >> 8);
    }
}

int main(void) {
    remu_t *m = remu_create();
    if (!m) { printf("FAIL create\n"); return 1; }
    uint32_t entry = remu_load_s(m, "asm/battle/matchings/mainasm_10FF4/func_80080AE4.s");
    if (entry != 0x80080AE4u) { printf("FAIL load entry=%08X\n", entry); return 1; }
    static uint8_t img[0x330];
    int n = 0;
    /* structured: every valid init 0..10, sequential keys, target outside keys */
    for (uint8_t init = 0; init < 11; init++) {
        uint8_t ev[12];
        for (int k = 0; k < 12; k++) ev[k] = (uint8_t)(k * 3 + 1);
        uint32_t s = 1;
        make_img(img, &s, init, ev, 0);
        if (!check_one(m, entry, 0xC0, init, ev, img)) return 1;
        n++;
        make_img(img, &s, init, ev, 1);
        if (!check_one(m, entry, 0xC0, init, ev, img)) return 1;
        n++;
    }
    /* target inside keys + dup keys + negatives */
    {
        uint8_t ev[12] = { 5, 5, 9, 9, 9, 21, 30, 41, 60, 77, 90, 200 };
        uint32_t s = 7;
        for (uint8_t init = 0; init <= 11; init += 5) {
            make_img(img, &s, init, ev, 3);
            if (!check_one(m, entry, 9, init, ev, img)) return 1;
            if (!check_one(m, entry, 0xC0, init, ev, img)) return 1;
            n += 2;
        }
    }
    /* randomized (slot coverage via construction; qualifier guaranteed) */
    {
        uint32_t s = 0x80AE480u;
        for (int i = 0; i < 150; i++) {
            uint8_t ev[12];
            for (int k = 0; k < 12; k++) {
                s = s * 1103515245u + 12345u;
                ev[k] = (uint8_t)(s >> 17);
            }
            uint8_t init = (uint8_t)((s >> 5) % 11u);
            uint8_t target = (uint8_t)(s >> 11);
            ev[0] = (uint8_t)(target + 1); /* slot 0 always qualifies */
            make_img(img, &s, init, ev, 2);
            if (!check_one(m, entry, target, init, ev, img)) return 1;
            n++;
        }
    }
    printf("DIFF 80080AE4 OK (%d checks, retail-exact)\n", n);
    remu_destroy(m);
    return 0;
}
