/* Compare the shared host body for func_800AA760 with retail MIPS. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "remu.h"

void func_800AA760(u32 index, u8 value);
u8 g_PsxRam[PSX_RAM_SIZE];

static int check(remu_t *m, uint32_t entry, uint32_t index,
                 uint32_t record, uint8_t value, uint8_t initial) {
    const uint32_t slot = 0x800D3368u + index * 4u;
    uint8_t ptr[4] = {
        (uint8_t)record, (uint8_t)(record >> 8),
        (uint8_t)(record >> 16), (uint8_t)(record >> 24)
    };
    uint8_t got;

    memset(g_PsxRam, 0, PSX_RAM_SIZE);
    if (record != 0) {
        *(uint32_t *)PSX_ADDR(slot) = record;
        *(uint8_t *)PSX_ADDR(record + 0x2A) = initial;
    } else {
        *(uint32_t *)PSX_ADDR(slot) = 0;
    }
    if (remu_poke(m, slot, ptr, sizeof(ptr)) != 0 ||
        (record != 0 && remu_poke(m, record + 0x2A, &initial, 1) != 0)) {
        puts("FAIL retail seed");
        return 0;
    }
    func_800AA760(index, value);
    if (remu_call(m, entry, index, value, 0, 0) != 0 ||
        remu_stub_calls(m) != 0) {
        printf("FAIL retail call index=%08X rc/stubs\n", index);
        return 0;
    }
    if (record == 0) return 1;
    if (remu_peek(m, record + 0x2A, &got, 1) != 0) {
        puts("FAIL retail readback");
        return 0;
    }
    if (got != *(uint8_t *)PSX_ADDR(record + 0x2A)) {
        printf("FAIL index=%08X retail=%02X host=%02X\n", index, got,
               *(uint8_t *)PSX_ADDR(record + 0x2A));
        return 0;
    }
    return 1;
}

int main(void) {
    remu_t *m = remu_create();
    uint32_t entry;
    uint32_t seed = 0xAA760u;
    unsigned i;
    if (!m) return 1;
    entry = remu_load_s(m, "asm/battle/matchings/main70/func_800AA760.s");
    if (entry != 0x800AA760u) return 1;
    for (i = 0; i < 300; i++) {
        uint32_t index;
        uint32_t record;
        uint8_t value, initial;
        seed = seed * 1664525u + 1013904223u;
        index = (i < 12) ? i : ((seed >> 8) & 0x3FFu);
        record = (i % 7 == 0) ? 0 : 0x80020000u + ((seed & 0x7FFu) * 0x40u);
        value = (uint8_t)(seed >> 16);
        initial = (uint8_t)(seed >> 24);
        if (!check(m, entry, index, record, value, initial)) return 1;
    }
    puts("DIFF 800AA760 OK (300 checks, retail-exact)");
    remu_destroy(m);
    return 0;
}
