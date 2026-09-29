#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

void func_8009E364(void);

int main(void) {
    unsigned slot;

    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    *(u32 *)PSX_ADDR(0x800C3E00) = 0x80010000;
    *(u32 *)PSX_ADDR(0x800C3DFC) = 0x80011000;
    *(u32 *)PSX_ADDR(0x800C34B0) = 0x80012000;
    *(u8 *)PSX_ADDR(0x8001005B) = 17;
    *(u8 *)PSX_ADDR(0x80011011) = 9;

    for (slot = 0; slot < 16; ++slot) {
        *(u8 *)PSX_ADDR(0x800C3E50) = (u8)slot;
        func_8009E364();
        assert(*(u8 *)PSX_ADDR(0x80017FA0 + slot) == 2);
        assert(*(u32 *)PSX_ADDR(0x80017F6C + slot * 4) == 153);
    }

    puts("func_8009E364 host guest-RAM writes PASS (16 slots)");
    return 0;
}
