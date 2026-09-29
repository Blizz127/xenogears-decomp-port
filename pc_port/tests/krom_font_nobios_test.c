#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "krom_rom.h"

static const uint8_t expected_8141[32] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0x60,0,0x30,0,0x10,0
};
static const uint8_t expected_889f[32] = {
    0,0,0x7f,0xfe,0x04,0x40,0x04,0x40,0x04,0x40,0x3f,0xfc,0x24,0x44,0x24,0x44,
    0x24,0x44,0x24,0x44,0x3f,0xfc,0x04,0x40,0x04,0x40,0x04,0x40,0xff,0xff,0,0
};
static const uint8_t *spans[8];

static void *lookup(void *arg) {
    unsigned i = (uintptr_t)arg;
    spans[i] = PcPortKromFont(0x889f, 30);
    return NULL;
}

int main(void) {
    assert(getenv("XENO_BIOS") == NULL || !*getenv("XENO_BIOS"));
    const uint8_t *ascii = PcPortKromFont(0x8141, 32);
    const uint8_t *field = PcPortKromFont(0x889f, 32);
    assert(memcmp(ascii, expected_8141, sizeof expected_8141) == 0);
    assert(memcmp(field, expected_889f, sizeof expected_889f) == 0);
    assert(PcPortKromFont(0x817f, 32) == (const uint8_t *)(intptr_t)-1);
    assert(PcPortKromFont(0xa140, 32) == (const uint8_t *)(intptr_t)-1);

    pthread_t threads[8];
    for (unsigned i = 0; i < 8; ++i)
        assert(pthread_create(&threads[i], NULL, lookup, (void *)(uintptr_t)i) == 0);
    for (unsigned i = 0; i < 8; ++i)
        assert(pthread_join(threads[i], NULL) == 0 && spans[i] == field);
    puts("PASS no-BIOS KROM fallback: JIS glyph bytes, field code, invalid codes, thread-safe spans");
    return 0;
}
