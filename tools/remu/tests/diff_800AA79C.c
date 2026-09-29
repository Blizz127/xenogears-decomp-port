/* Compare the shared host body for func_800AA79C with retail MIPS. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "remu.h"

void func_800AA79C(u32 first, u32 second);
u8 g_PsxRam[PSX_RAM_SIZE];

static int check(remu_t *m, uint32_t entry, uint32_t first,
                 uint32_t second, uint32_t rec_a, uint32_t rec_b,
                 uint8_t flag_a, uint8_t flag_b) {
    uint32_t slot_a = 0x800D3368u + first * 4u;
    uint32_t slot_b = 0x800D3368u + second * 4u;
    uint32_t got_a, got_b;
    uint8_t got_flag_a, got_flag_b;
    memset(g_PsxRam, 0, PSX_RAM_SIZE);
    *(uint32_t *)PSX_ADDR(slot_a) = rec_a;
    *(uint32_t *)PSX_ADDR(slot_b) = rec_b;
    *(uint8_t *)PSX_ADDR(rec_a + 0x34) = flag_a;
    *(uint8_t *)PSX_ADDR(rec_b + 0x34) = flag_b;
    if (remu_poke(m, slot_a, &rec_a, 4) || remu_poke(m, slot_b, &rec_b, 4) ||
        remu_poke(m, rec_a + 0x34, &flag_a, 1) ||
        remu_poke(m, rec_b + 0x34, &flag_b, 1)) {
        puts("FAIL retail seed");
        return 0;
    }
    func_800AA79C(first, second);
    if (remu_call(m, entry, first, second, 0, 0) || remu_stub_calls(m)) {
        printf("FAIL retail call %08X %08X\n", first, second);
        return 0;
    }
    if (remu_peek(m, slot_a, &got_a, 4) || remu_peek(m, slot_b, &got_b, 4) ||
        remu_peek(m, rec_a + 0x34, &got_flag_a, 1) ||
        remu_peek(m, rec_b + 0x34, &got_flag_b, 1)) {
        puts("FAIL retail readback");
        return 0;
    }
    if (got_a != *(uint32_t *)PSX_ADDR(slot_a) ||
        got_b != *(uint32_t *)PSX_ADDR(slot_b) ||
        got_flag_a != *(uint8_t *)PSX_ADDR(rec_a + 0x34) ||
        got_flag_b != *(uint8_t *)PSX_ADDR(rec_b + 0x34)) {
        printf("FAIL indexes=%08X,%08X flags=%02X/%02X host=%02X/%02X\n",
               first, second, got_flag_a, got_flag_b,
               *(uint8_t *)PSX_ADDR(rec_a + 0x34),
               *(uint8_t *)PSX_ADDR(rec_b + 0x34));
        return 0;
    }
    return 1;
}

int main(void) {
    remu_t *m = remu_create();
    uint32_t entry, seed = 0xAA79Cu;
    unsigned i;
    if (!m) return 1;
    entry = remu_load_s(m, "asm/battle/matchings/main71/func_800AA79C.s");
    if (entry != 0x800AA79Cu) return 1;
    for (i = 0; i < 300; i++) {
        uint32_t first, second, rec_a, rec_b;
        seed = seed * 1664525u + 1013904223u;
        first = (i < 12) ? i : ((seed >> 8) & 0x3FFu);
        seed = seed * 1664525u + 1013904223u;
        second = (i % 11 == 0) ? first : ((i < 12) ? 11 - i : ((seed >> 9) & 0x3FFu));
        rec_a = 0x80030000u + ((seed & 0x3FFu) * 0x40u);
        rec_b = 0x80050000u + (((seed >> 10) & 0x3FFu) * 0x40u);
        if (rec_a == rec_b) rec_b += 0x40;
        if (!check(m, entry, first, second, rec_a, rec_b,
                   (uint8_t)(seed >> 16), (uint8_t)(seed >> 24))) return 1;
    }
    puts("DIFF 800AA79C OK (300 checks, retail-exact)");
    remu_destroy(m);
    return 0;
}
