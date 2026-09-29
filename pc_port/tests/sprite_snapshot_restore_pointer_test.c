/* Retail 80021D50..80021EB8: restore packed 32-bit sprite pointers.
 * Reproduces the field15 post-battle core's concatenated +20/+24 pointer.
 * The real restore runs; animation callbacks provide deterministic ticks. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void func_80021D50(void *, void *);
int D_80059198;
static _Alignas(8) uint8_t sprite[0x180], saved[0x30], model[0x20], table[0x10];
static unsigned ticks, target;
static uint32_t get32(const void *p) { uint32_t v; memcpy(&v, p, 4); return v; }
static void put32(void *p, uint32_t v) { memcpy(p, &v, 4); }
static void put16(void *p, uint16_t v) { memcpy(p, &v, 2); }
static uint16_t get16(const void *p) { uint16_t v; memcpy(&v, p, 2); return v; }

void func_800245D8(void *p, int animation) {
    assert(p == sprite && animation == -2 && D_80059198 == 0);
    assert(get16(model + 6) == 0x1234 && get16(model + 8) == 0xFEDC);
    assert(get16(model + 10) == 0x9876);
    assert(get16(sprite + 0x82) == 0x2468 && get16(sprite + 0x2C) == 0x1357);
    assert(sprite[0xB0] == 0xAB);
}

void AnimScriptTick(void *p) {
    assert(p == sprite && D_80059198 == 0 && ticks < target);
    /* Check integration performed after each preceding tick, including wrap. */
    assert(get32(sprite) == 0xFFFFFFFEu + ticks * 3u);
    assert(get32(sprite + 8) == 7u + ticks * 5u);
    assert(get32(sprite + 4) == 10u + ticks * 2u + ticks * (ticks - 1u) / 2u);
    assert(get32(sprite + 0x10) == 2u + ticks);
    ++ticks;
    put32(sprite + 0xA8, ticks << 22);
}

int main(int argc, char **argv) {
    unsigned mode = argc > 1 ? (unsigned)strtoul(argv[1], NULL, 0) : 3;
    assert(sizeof(void *) == 8 && (uintptr_t)model <= UINT32_MAX);
    assert((uintptr_t)table <= UINT32_MAX);
    for (target = 0; target <= 3; target += 3) {
        memset(sprite, 0, sizeof(sprite)); memset(saved, 0, sizeof(saved));
        memset(model, 0xA5, sizeof(model)); memset(table, 0x5A, sizeof(table));
        put32(sprite + 0x20, (uint32_t)(uintptr_t)model);
        put32(sprite + 0x24, mode & 1 ? 0x007CBC78 : 0);
        put32(sprite + 0x7C, (uint32_t)(uintptr_t)table);
        /* Restore writes +80 before loading +7C, so poison via saved +10. */
        put16(saved + 0x10, mode & 2 ? 0x8000 : 0);
        put16(saved + 0x14, 0xFFFE); put16(saved + 0x16, 0x12AB);
        put16(saved + 0x18, target);
        put32(saved, 0xDEADBEEF); put32(saved + 4, 0x87654321);
        put32(saved + 8, 0x12345678);
        put32(saved + 0x1C, 0x76543210); put32(saved + 0x20, 0xABCDEF98);
        put16(saved + 0x24, 0x1234); put16(saved + 0x26, 0xFEDC);
        put16(saved + 0x28, 0x9876); put16(saved + 0x2A, 0x1357);
        put16(saved + 0x2C, 0x2468);
        put32(sprite, 0xFFFFFFFE); put32(sprite + 4, 10); put32(sprite + 8, 7);
        put32(sprite + 0xC, 3); put32(sprite + 0x10, 2);
        put32(sprite + 0x14, 5); put32(sprite + 0x1C, 1);
        ticks = 0; D_80059198 = 17;
        func_80021D50(sprite, saved);
        assert(ticks == target && D_80059198 == 17);
        assert(memcmp(sprite, saved, 12) == 0);
        assert(get32(table) == 0x76543210 && get32(table + 4) == 0xABCDEF98);
        assert(get32(table + 8) == 0x5A5A5A5A && get32(model) == 0xA5A5A5A5);
        assert(get32(model + 12) == 0xA5A5A5A5);
        assert(get32(sprite + 0x24) == (mode & 1 ? 0x007CBC78u : 0));
        assert(get16(sprite + 0x80) == (mode & 2 ? 0x8000 : 0));
    }
    printf("SPRITE SNAPSHOT RESTORE PASS pointer_mode=%u ticks=0,3\n", mode);
    return 0;
}
