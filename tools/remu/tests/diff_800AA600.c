/* Compare the port adapter for func_800AA600 with retail MIPS. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "remu.h"

s32 func_800AA600(s32 index);
u8 g_PsxRam[PSX_RAM_SIZE];

static int check(remu_t *m, uint32_t entry, uint32_t index, uint32_t p,
                 uint32_t q, int16_t v1, int16_t v2, int16_t v3) {
    uint32_t slot = 0x800D3368u + index * 4u;
    uint32_t host_result;
    uint32_t retail_result;
    memset(g_PsxRam, 0, PSX_RAM_SIZE);
    *(uint32_t *)PSX_ADDR(slot) = p;
    *(uint32_t *)PSX_ADDR(p + 4) = q;
    *(int16_t *)PSX_ADDR(p + 0x1C) = v1;
    *(int16_t *)PSX_ADDR(q + 0x4E) = v2;
    *(int16_t *)PSX_ADDR(p + 0x24) = v3;
    if (remu_poke(m, slot, &p, 4) || remu_poke(m, p + 4, &q, 4) ||
        remu_poke(m, p + 0x1C, &v1, 2) || remu_poke(m, q + 0x4E, &v2, 2) ||
        remu_poke(m, p + 0x24, &v3, 2)) {
        puts("FAIL retail seed");
        return 0;
    }
    host_result = (uint32_t)func_800AA600((s32)index);
    if (remu_call(m, entry, index, 0, 0, 0) || remu_stub_calls(m)) {
        printf("FAIL retail call index=%08X\n", index);
        return 0;
    }
    retail_result = remu_get_reg(m, 2);
    if (retail_result != host_result) {
        printf("FAIL index=%08X retail=%08X host=%08X\n", index,
               retail_result, host_result);
        return 0;
    }
    return 1;
}

int main(void) {
    remu_t *m = remu_create();
    uint32_t entry, seed = 0xAA600u;
    unsigned i;
    if (!m) return 1;
    entry = remu_load_s(m, "asm/battle/matchings/main70/func_800AA600.s");
    if (entry != 0x800AA600u) return 1;
    for (i = 0; i < 300; i++) {
        uint32_t index, p, q;
        int16_t v1, v2, v3;
        seed = seed * 1664525u + 1013904223u;
        index = (i < 12) ? i : ((seed >> 8) & 0x3FFu);
        p = 0x80030000u + ((seed & 0x3FFu) * 0x80u);
        q = 0x80050000u + (((seed >> 10) & 0x3FFu) * 0x80u);
        seed = seed * 1664525u + 1013904223u;
        v1 = (int16_t)((int32_t)(seed & 0xFFFu) - 2048);
        seed = seed * 1664525u + 1013904223u;
        v2 = (int16_t)((int32_t)(seed & 0xFFFu) - 2048);
        seed = seed * 1664525u + 1013904223u;
        v3 = (int16_t)((int32_t)(seed % 60001u) - 30000);
        if (!check(m, entry, index, p, q, v1, v2, v3)) return 1;
    }
    puts("DIFF 800AA600 OK (300 checks, retail-exact)");
    remu_destroy(m);
    return 0;
}
